#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "event.h"
#include "event_source.h"
#include "ring_buffer.h"
#include "rule_engine.h"
#include "entity_model.h"
#include "window_entry.h"
#include "response.h"
#include "os_handle.h"

namespace nano_edr {

class Agent : public IEventHandler {
 public:
    static constexpr std::size_t kWindowCapacity = 256;

    explicit Agent(bool quiet, uint64_t dedup_period_ms, OsHandle* os, std::string quarantine_dir, bool dry_run);

    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;

    void PrintSummary() const;
    void Subscribe(AlertSink sink);

 private:
    struct TypeCount {
        std::string type;
        std::size_t count = 0;
    };

    RingBuffer<WindowEntry, kWindowCapacity> window_;
    bool quiet_;
    EntityModel model_;
    RuleEngine engine_;
    uint64_t dedup_period_ms_{};
    std::size_t suppressed_{};
    std::size_t total_events_ = 0;
    std::size_t total_detects_ = 0;
    std::size_t total_skipped_ = 0;
    std::size_t total_broken_ = 0;
    std::vector<TypeCount> stats_;

    void PrintContext() const;
    void PrintContextOnce();

    bool context_printed_ = false;

    ResponsePlanner planner_;
    Responder responder_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H