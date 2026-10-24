#include "rule.h"

namespace nano_edr {

RuleBase::RuleBase(const std::string& id, Severity severity)
    : id_(id), severity_(severity) {}


void RuleBase::AddAction(const std::string& action) {
    actions_.push_back(action);
}


MatchRule::MatchRule(const std::string& id, Severity severity)
    : RuleBase(id, severity) {}


void MatchRule::AddCondition(std::unique_ptr<ICondition> condition) {
    conditions_.push_back(std::move(condition));
}


bool MatchRule::Check(const Event& event) {
    for (const auto& condition : conditions_) {
        if (!condition->Matches(event)) {
            return false;
        }
    }
    CountHit();
    return true;
}

SequenceRule::SequenceRule(const std::string& id, Severity severity, uint64_t window_ms)
    : RuleBase(id, severity), window_ms_(window_ms) {}


void SequenceRule::SetFirst(std::unique_ptr<ICondition> condition) {
    first_ = std::move(condition);
}


void SequenceRule::SetSecond(std::unique_ptr<ICondition> condition) {
    second_ = std::move(condition);
}


bool SequenceRule::Check(const Event& event) {
    if (!first_ || !second_) {
        return false;
    }
    const uint64_t now = event.ts().ms;
    std::size_t write = 0;
    for (std::size_t i = 0; i < pending_.size(); ++i) {
        if (now - pending_[i].ts_ms <= window_ms_) {
            pending_[write++] = pending_[i];
        }
    }
    pending_.resize(write);
    if (second_->Matches(event)) {
        std::size_t match_index = pending_.size();  // размер = «не нашли»
        for (std::size_t i = 0; i < pending_.size(); ++i) {
            bool pid_ok = !same_pid_ || pending_[i].pid == event.pid();
            if (pid_ok) {
                match_index = i;
                break;
            }
        }
        if (match_index < pending_.size()) {
            pending_.erase(pending_.begin() + match_index);
            CountHit();
            return true;
        }
    }
    if (first_->Matches(event)) {
        pending_.push_back({now, event.pid()});
    }
    return false;
}

}  // namespace nano_edr