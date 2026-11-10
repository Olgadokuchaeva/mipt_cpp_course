#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <string>
#include <vector>

#include "event.h"
#include "event_source.h"
#include "ring_buffer.h"
#include "rule_engine.h"

namespace nano_edr {

struct WindowEntry {
    Timestamp ts;
    std::string type;
    std::string pid;
};

class Agent : public IEventHandler {
 public:
    static constexpr std::size_t kWindowCapacity = 256;

    explicit Agent(bool quiet);

    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;

    void PrintSummary() const;

 private:
    struct TypeCount {
        std::string type;
        std::size_t count = 0;
    };

    RingBuffer<WindowEntry, kWindowCapacity> window_;
    bool quiet_;
    RuleEngine engine_;

    std::size_t total_events_ = 0;
    std::size_t total_detects_ = 0;
    std::size_t total_skipped_ = 0;
    std::size_t total_broken_ = 0;
    std::vector<TypeCount> stats_;

    void PrintContext() const;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H