#include "response.h"
#include "field_traits.h"

#include <algorithm>
#include <cctype>
#include <print>
#include <string>
#include <utility>

#include "fields.h"

namespace nano_edr {

namespace {

bool IsSystemPath(const std::string& path) {
    auto lower = [](std::string s) {
        for (char& c : s) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    };
    const std::string p = lower(path);
    return p.starts_with("c:\\windows\\") || p.starts_with("c:\\program files");
}

std::string GetReason(const std::string& rule_id, const std::string& suffix = "") {
    return suffix.empty() ? rule_id : rule_id + ", " + suffix;
}

}  // namespace

// --- ResponsePlanner -------------------------------------------------------

ResponsePlanner::ResponsePlanner(const EntityModel* model,
                                 std::string quarantine_dir)
    : model_(model), quarantine_dir_(std::move(quarantine_dir)) {}

std::string ResponsePlanner::QuarantinePath(const std::string& path,
                                            const std::string& pid) const {
    if (quarantine_dir_.empty() || path.empty()) {
        return "";
    }
    const std::size_t pos = path.find_last_of("\\/");
    const std::string filename = (pos == std::string::npos) ? path : path.substr(pos + 1);
    return quarantine_dir_ + "\\" + pid + "-" + filename;
}

ResponsePlan ResponsePlanner::Plan(const Detection& detection) const {
    ResponsePlan plan;
    if (!model_) {
        return plan;
    }
    auto add_action = [&](ActionKind kind, const std::string& target,
                          const std::string& to, const std::string& reason) {
        if (target.empty()) {
            return;
        }
        Action act;
        act.kind = kind;
        act.reason = GetReason(detection.rule, reason);
        if (kind == ActionKind::kKillProcess) {
            act.pid = target;
            const ProcessRecord* process = model_->FindProcess(target);
            if (process) {
                act.expected_start = process->start;
            }
        } else {
            act.path = target;
            act.to = to;
        }
        plan.Add(std::move(act));
    };
    for (const std::string& action : detection.actions) {
        if (action == "kill_process") {
            const std::vector<std::string> tree = model_->GetProcessTree(detection.pid);
            for (auto it = tree.rbegin(); it != tree.rend(); ++it) {
                if (*it == detection.pid) {
                    add_action(ActionKind::kKillProcess, *it, "", "");
                } else {
                    add_action(ActionKind::kKillProcess, *it, "", "потомок " + detection.pid);
                }
            }
        } else if (action == "quarantine_source") {
            const FileRecord* source = model_->FindSpawnSource(detection.pid);
            if (source && !source->path.empty() && !IsSystemPath(source->path)) {
                const std::string to = QuarantinePath(source->path, detection.pid);
                if (!to.empty()) {
                    add_action(ActionKind::kQuarantineFile, source->path, to, "источник запуска");
                }
            }
        } else if (action == "quarantine_image") {
            const ProcessRecord* process = model_->FindProcess(detection.pid);
            if (process && !process->image.empty() && !IsSystemPath(process->image)) {
                const std::string to = QuarantinePath(process->image, detection.pid);
                if (!to.empty()) {
                    add_action(ActionKind::kQuarantineFile, process->image, to, "образ процесса");
                }
            }
        } else if (action == "delete_dropped") {
            const std::vector<std::string> tree = model_->GetProcessTree(detection.pid);
            for (const std::string& pid : tree) {
                const std::vector<std::string> files = model_->GetFilesCreatedBy(pid, true);
                for (const std::string& path : files) {
                    if (!IsSystemPath(path)) {
                        add_action(ActionKind::kDeleteFile, path, "", "принесён " + pid);
                    }
                }
            }
        } else if (action == "delete_evidence") {
            for (const auto& event : detection.evidence) {
                std::string path;
                if (event->event_type() == EventType::kFileMove) {
                    const std::string* to_value = FindField(*event, "to");
                    if (to_value) {
                        path = *to_value;
                    }
                } else {
                    const std::string* path_value = FindField(*event, "path");
                    if (path_value) {
                        path = *path_value;
                    }
                }
                if (!path.empty() && !IsSystemPath(path)) {
                    add_action(ActionKind::kDeleteFile, path, "", "файл из детекта");
                }
            }
        }
    }
    return plan;
}

// --- Responder -------------------------------------------------------------

Responder::Responder(OsHandle* os, bool dry_run)
    : os_(os), dry_run_(dry_run) {}

bool Responder::ResolveStart(const std::string& pid, Timestamp planned, uint64_t* out) {
    if (planned.ms != 0) {
        *out = planned.ms;
        return true;
    }
    if (!os_) {
        return false;
    }
    uint64_t pid_num = 0;
    if (!FieldTraits<uint64_t>::Parse(pid, &pid_num)) {
        return false;
    }
    os_process_info info{};
    ++stats_.queries;
    if (os_->ProcessInfo(static_cast<uint32_t>(pid_num), &info) == OS_OK && info.start_time != 0) {
        *out = info.start_time;
        return true;
    }
    return false;
}

void Responder::Kill(const Action& action) {
    if (!os_) {
        return;
    }
    uint64_t pid_num = 0;
    if (!FieldTraits<uint64_t>::Parse(action.pid, &pid_num)) {
        return;
    }
    uint64_t start = 0;
    if (!ResolveStart(action.pid, action.expected_start, &start)) {
        std::print("[ACT] kill pid={} — процесса уже нет\n", action.pid);
        ++stats_.kill_gone;
        return;
    }
    const os_status status = os_->Kill(static_cast<uint32_t>(pid_num), start);
    if (status == OS_OK) {
        ++stats_.killed;
        std::print("[ACT] kill pid={} start={} OS_OK\n", action.pid, start);
    } else if (status == OS_NO_SUCH_PROCESS) {
        ++stats_.kill_gone;
        std::print("[ACT] kill pid={} — процесса уже нет\n", action.pid);
    } else if (status == OS_PID_REUSED) {
        ++stats_.kill_reused;
        std::print("[ACT] kill pid={} — номер переиспользован\n", action.pid);
    } else {
        ++stats_.kill_denied;
        std::print("[ACT] kill pid={} {}\n", action.pid, os_status_str(status));
    }
}

void Responder::MoveToQuarantine(const Action& action) {
    if (!os_) {
        return;
    }
    const os_status status = os_->MoveFile(action.path, action.to);
    if (status == OS_OK) {
        ++stats_.quarantined;
        std::print("[ACT] quarantine {} -> {} OS_OK\n", action.path, action.to);
    } else if (status == OS_BUSY) {
        ++stats_.file_busy;
        std::print("[ACT] quarantine {} — файл занят\n", action.path);
    } else if (status == OS_NO_SUCH_FILE) {
        ++stats_.file_gone;
        std::print("[ACT] quarantine {} — файла уже нет\n", action.path);
    } else {
        ++stats_.file_denied;
        std::print("[ACT] quarantine {} {}\n", action.path, os_status_str(status));
    }
}

void Responder::Delete(const Action& action) {
    if (!os_) {
        return;
    }
    const os_status status = os_->DeleteFile(action.path);
    if (status == OS_OK) {
        ++stats_.deleted;
        std::print("[ACT] delete {} OS_OK\n", action.path);
    } else if (status == OS_BUSY) {
        ++stats_.file_busy;
        std::print("[ACT] delete {} — файл занят\n", action.path);
    } else if (status == OS_NO_SUCH_FILE) {
        ++stats_.file_gone;
        std::print("[ACT] delete {} — файла уже нет\n", action.path);
    } else {
        ++stats_.file_denied;
        std::print("[ACT] delete {} {}\n", action.path, os_status_str(status));
    }
}

void Responder::Execute(const ResponsePlan& plan) {
    stats_.planned += plan.size();
    for (const Action& action : plan.actions()) {
        if (action.kind != ActionKind::kKillProcess) {
            continue;
        }
        if (std::find(done_.begin(), done_.end(), action.target()) != done_.end()) {
            ++stats_.skipped_repeat;
            continue;
        }
        done_.push_back(action.target());
        if (dry_run_ || !os_) {
            std::print("[PLAN] {}\n", ToString(action));
        } else {
            Kill(action);
        }
    }
    for (const Action& action : plan.actions()) {
        if (action.kind == ActionKind::kKillProcess) {
            continue;
        }
        if (std::find(done_.begin(), done_.end(), action.target()) != done_.end()) {
            ++stats_.skipped_repeat;
            continue;
        }
        done_.push_back(action.target());
        if (dry_run_ || !os_) {
            std::print("[PLAN] {}\n", ToString(action));
        } else if (action.kind == ActionKind::kQuarantineFile) {
            MoveToQuarantine(action);
        } else if (action.kind == ActionKind::kDeleteFile) {
            Delete(action);
        }
    }
}

bool Responder::Recognize(const Event& event) {
    const std::string* actor = FindField(event, "actor");
    if (actor && !actor->empty()) {
        if (self_pid_.empty()) {
            self_pid_ = *actor;
        }
        ++stats_.own_events;
        return true;
    }
    if (!self_pid_.empty() && event.pid() == self_pid_) {
        ++stats_.own_events;
        return true;
    }
    return false;
}

}  // namespace nano_edr
