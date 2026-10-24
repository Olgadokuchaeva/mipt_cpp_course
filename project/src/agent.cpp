#include <print>
#include <string>

#include "agent.h"
#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(std::size_t window_size, bool quiet)
    : window_(window_size), quiet_(quiet), engine_(BuildAgentRules()) {}


void Agent::HandleEvent(const Event& event) {
    ++total_events_;
    std::vector<const IRule*> fired;
    std::size_t detects = engine_.ProcessEvent(event, &fired);
    total_detects_ += detects;
    if (detects > 0 && !quiet_) {
        PrintContext();
    }
    for (const IRule* rule : fired) {
        std::print("[DETECT] {}  {}  ts={} pid={}\n",
                   SeverityName(rule->severity()),
                   rule->id(),
                   event.raw_ts(),
                   event.pid());
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


void Agent::OnSkipped() {
    ++total_skipped_;
}


void Agent::OnBroken() {
    ++total_broken_;
}


void Agent::OnTick() {}


void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий {} всего, детектов {}, пропущено {}\n", total_events_, total_detects_, total_skipped_);
    std::print("события по типам:\n");
    for (const TypeCount& s : stats_) {
        std::print(" {}: {}\n", s.type, s.count);
    }
    std::print("правила, детектов:\n");
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        const IRule& rule = engine_.rule(i);
        std::print(" {:24} {}\n", rule.id(), rule.hits());
    }
}


void Agent::PrintContext() const {
    if (prev_event_ != nullptr) {
        std::print("[CTX] -2: {}\n", ToString(*prev_event_));
    }
    if (last_event_ != nullptr) {
        std::print("[CTX] -1: {}\n", ToString(*last_event_));
    }
}

}  // namespace nano_edr