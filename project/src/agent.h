#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <string>
#include <vector>

#include "event.h"
#include "event_list.h"
#include "event_source.h"
#include "rule_engine.h"


namespace nano_edr {

class Agent : public IEventHandler {
 public:
    Agent(std::size_t window_size, bool quiet);

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

    EventList window_;
    bool quiet_;
    RuleEngine engine_;
    const Event* prev_event_ = nullptr;
    const Event* last_event_ = nullptr;
    std::size_t total_events_ = 0;
    std::size_t total_detects_ = 0;
    std::size_t total_skipped_ = 0;
    std::size_t total_broken_ = 0;
    std::vector<TypeCount> stats_;
    std::vector<MatchRule> rules_;

    void PrintContext() const;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H