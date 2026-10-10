#include <print>
#include <string>

#include "agent.h"
#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(std::size_t window_size, bool quiet, const std::vector<std::string>& disabled)
    : window_(window_size), quiet_(quiet), disabled_(disabled) {}


void Agent::HandleEvent(const Event& event) {
    ++total_events_;
    const Rule* all_rules = AgentRules();
    std::size_t all_count = AgentRuleCount();
    std::vector<Rule> active_rules;
    for (std::size_t i = 0; i < all_count; ++i) {
        bool skip = false;
        for (const std::string& d : disabled_) {
            if (d == all_rules[i].id) {
                skip = true;
                break;
            }
        }
        if (!skip) {
            active_rules.push_back(all_rules[i]);
        }
    }
    std::size_t detects = CheckRules(event, active_rules.data(), active_rules.size());
    total_detects_ += static_cast<long long>(detects);
    if (detects > 0 && !quiet_) {
        PrintContext();
    }
    window_.PushBack(event);
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
    std::print("событий {} всего, детектов {}\n", total_events_, total_detects_);
    std::print("события по типам:\n");
    for (const TypeCount& s : stats_) {
        std::print(" {}: {}\n", s.type, s.count);
    }
}


void Agent::PrintContext() const {
    if (window_.size() == 0) {
        return;
    }
    const EventNode* prev = nullptr;
    const EventNode* last = nullptr;
    for (const EventNode* it = window_.head(); it != nullptr; it = it->next) {
        prev = last;
        last = it;
    }
    if (prev != nullptr) {
        std::print("[CTX] -2: {}\n", ToString(prev->event));
    }
    if (last != nullptr) {
        std::print("[CTX] -1: {}\n", ToString(last->event));
    }
}


void Agent::Trampoline(const os_event* ev, void* ctx) noexcept {
    try {
        Agent* self = static_cast<Agent*>(ctx);
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
            f.key   = ev->fields[i].key;
            f.value = ev->fields[i].value;
            parts.fields.push_back(f);
        }
        Event event(parts);
        self->HandleEvent(event);
    } catch (...) {
        
    }
}

}  // namespace nano_edr