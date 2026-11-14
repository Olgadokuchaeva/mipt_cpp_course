#include <print>
#include <string>

#include "agent.h"
#include "agent_rules.h"
#include "rules.h"
#include "dedup_sink.h"

namespace nano_edr {

Agent::Agent(bool quiet, uint64_t dedup_period_ms)
    : quiet_(quiet), 
      engine_(BuildAgentRules(&model_)), 
      dedup_period_ms_(dedup_period_ms) {
    engine_.SetModel(&model_);
}


void Agent::HandleEvent(const Event& event) {
    ++total_events_;
    context_printed_ = false;
    model_.Observe(event);
    std::vector<const IRule*> fired;
    std::size_t detects = engine_.ProcessEvent(event, &fired);
    total_detects_ += detects;
    WindowEntry entry;
    entry.ts = event.ts();
    entry.type = event.type();
    entry.pid = event.pid();
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
    if (window_.empty()) {
        return;
    }
    if (window_.size() >= 2) {
        const WindowEntry& prev = window_[window_.size() - 2];
        std::print("[CTX] -2: {} {} pid={}\n", prev.ts.ms, prev.type, prev.pid);
    }
    const WindowEntry& last = window_.back();
    std::print("[CTX] -1: {} {} pid={}\n", last.ts.ms, last.type, last.pid);
}


void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий {} всего, детектов {}, пропущено {}, битых {}\n", 
        total_events_, total_detects_, total_skipped_, total_broken_);
    std::print("[STAT] в окне {} из {} мест\n", window_.size(), RingBuffer<WindowEntry, kWindowCapacity>::capacity());
    std::print("[STAT] в модели процессов {}, файлов {}\n", model_.process_count(), model_.file_count());
    std::print("события по типам:\n");
    for (const TypeCount& s : stats_) {
        std::print(" {}: {}\n", s.type, s.count);
    }
    if (suppressed_ > 0) {
        std::print("[STAT] подавлено дубликатов {} (срок {} мс)\n", suppressed_, dedup_period_ms_);
    }
    std::print("правила, детектов:\n");
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        const IRule& rule = engine_.rule(i);
        std::print(" {:24} {}\n", rule.id(), rule.hits());
    }
}

void Agent::Subscribe(AlertSink sink) {
    FanOutSink all;
    all.Add([this](const Detection&) { PrintContextOnce(); });
    all.Add(std::move(sink));
    engine_.Subscribe(DedupSink(std::move(all), dedup_period_ms_, &suppressed_));
}

void Agent::PrintContextOnce() {
    if (quiet_ || context_printed_) {
        return;
    }
    context_printed_ = true;
    PrintContext();
}

}  // namespace nano_edr