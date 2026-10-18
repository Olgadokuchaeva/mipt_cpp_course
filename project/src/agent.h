#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <string>
#include <vector>

#include "event.h"
#include "event_list.h"
#include "os.h"
#include "rule.h"


namespace nano_edr {

class Agent {
 public:
    Agent(std::size_t window_size, bool quiet);

    void HandleEvent(const Event& event);
    void PrintSummary() const;
    void SkipEvent(const char* reason) noexcept;

    static void Trampoline(const os_event* ev, void* ctx) noexcept;

 private:
    struct TypeCount {
        std::string type;
        std::size_t count = 0;
    };

    EventList window_;
    bool quiet_;
    const Event* prev_event_ = nullptr;
    const Event* last_event_ = nullptr;
    std::size_t total_events_ = 0;
    std::size_t total_detects_ = 0;
    std::size_t total_skipped_ = 0;
    std::vector<TypeCount> stats_;
    std::vector<MatchRule> rules_;

    static Event MakeEvent(const os_event* ev);

    void PrintContext() const;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H