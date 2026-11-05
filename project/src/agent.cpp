#include <print>
#include <string>

#include "agent.h"
#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(bool quiet)
    : quiet_(quiet), engine_(BuildAgentRules()) {}


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
    WindowEntry entry;
    entry.ts   = event.ts();
    entry.type = event.type();
    entry.pid  = event.pid();
    window_.PushBack(entry);
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


void Agent::OnSkipped() { ++total_skipped_; }
void Agent::OnBroken() { ++total_broken_; }
void Agent::OnTick() { }


void Agent::PrintContext() const {
    if (window_.size() >= 2) {
        const WindowEntry& prev = window_[window_.size() - 2];
        const WindowEntry& last = window_[window_.size() - 1];
        std::print("[CTX] -2: {} {} pid={}\n", prev.ts.ms, prev.type, prev.pid);
        std::print("[CTX] -1: {} {} pid={}\n", last.ts.ms, last.type, last.pid);
    } else if (window_.size() == 1) {
        const WindowEntry& last = window_[0];
        std::print("[CTX] -1: {} {} pid={}\n", last.ts.ms, last.type, last.pid);
    }
}


void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий {} всего, детектов {}, пропущено {}, битых {}\n", 
        total_events_, total_detects_, total_skipped_, total_broken_);
    std::print("[STAT] в окне {} из {} мест\n", window_.size(), RingBuffer<WindowEntry, kWindowCapacity>::capacity());
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

}  // namespace nano_edr