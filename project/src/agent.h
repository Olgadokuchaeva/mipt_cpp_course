#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <string>
#include <vector>

#include "event.h"
#include "event_list.h"
#include "os.h"

namespace nano_edr {

class Agent {
 public:
    Agent(std::size_t window_size, bool quiet, const std::vector<std::string>& disabled = {});

    void HandleEvent(const Event& event);
    void PrintSummary() const;

    static void Trampoline(const os_event* ev, void* ctx) noexcept;

 private:
    struct TypeCount {
        std::string type;
        long long count = 0;
    };

    EventList window_;
    bool quiet_;
    std::vector<std::string> disabled_;
    long long total_events_ = 0;
    long long total_detects_ = 0;
    std::vector<TypeCount> stats_;

    void PrintContext() const;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H