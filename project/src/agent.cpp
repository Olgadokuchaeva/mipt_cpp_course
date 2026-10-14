#include <print>
#include <string>

#include "agent.h"
#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(std::size_t window_size, bool quiet)
    : window_(window_size), quiet_(quiet) {}


void Agent::HandleEvent(const Event& event) {
    ++total_events_;
    const Rule* rules = AgentRules();
    std::size_t rule_count = AgentRuleCount();
    std::size_t detects = CheckRules(event, rules, rule_count);
    total_detects_ += detects;
    if (detects > 0 && !quiet_) {
        PrintContext();
    }
    window_.PushBack(event);
    if (window_.capacity() == 1) {
        prev_event_ = nullptr;
        last_event_ = &window_.back()->event;
    } else {
        prev_event_ = last_event_;
        last_event_ = &window_.back()->event;
    }
    bool found = false;
    for (TypeCount& s : stats_) {
        if (s.type == event.type()) {
            ++s.count;
            found = true;
            break;
        }
    }
    if (!found) {
        stats_.push_back({event.type(), 1});
    }
}


void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий {} всего, детектов {}, пропущено {}\n", total_events_, total_detects_, total_skipped_);
    std::print("события по типам:\n");
    for (const TypeCount& s : stats_) {
        std::print(" {}: {}\n", s.type, s.count);
    }
}

void Agent::SkipEvent(const char* reason) noexcept {
    ++total_skipped_;
    std::print(stderr, "[SKIP] {}\n", reason);
}


void Agent::PrintContext() const {
    if (prev_event_ != nullptr) {
        std::print("[CTX] -2: {}\n", ToString(*prev_event_));
    }
    if (last_event_ != nullptr) {
        std::print("[CTX] -1: {}\n", ToString(*last_event_));
    }
}

Event Agent::MakeEvent(const os_event* ev) {
    EventParts parts;
    parts.ts = std::to_string(ev->ts);
    if (ev->type != nullptr) {
        parts.type = ev->type;
    }
    if (ev->pid == 0) {
        parts.pid = "";
    } else {
        parts.pid = std::to_string(ev->pid);
    }
    for (std::size_t i = 0; i < ev->field_count; ++i) {
        Field f;
        f.key = ev->fields[i].key ? ev->fields[i].key : "";
        f.value = ev->fields[i].value ? ev->fields[i].value : "";
        parts.fields.push_back(f);
    }
    return Event(parts);
}


void Agent::Trampoline(const os_event* ev, void* ctx) noexcept {
    Agent* self = static_cast<Agent*>(ctx);
    try {
        Event event = MakeEvent(ev);
        self->HandleEvent(event);
    } catch (const std::exception& e) {
        self->SkipEvent(e.what());
    } catch (...) {
        self->SkipEvent("неизвестное исключение");
    }
}

}  // namespace nano_edr