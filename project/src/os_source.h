#ifndef NANO_EDR_OS_SOURCE_H
#define NANO_EDR_OS_SOURCE_H

#include <string>

#include "event_source.h"
#include "os.h"
#include "os_handle.h"

namespace nano_edr {

class OsSource : public IEventSource {
 public:
    explicit OsSource(const std::string& config_path);

    void Run(IEventHandler* handler) override;

 private:
    OsHandle handle_;

    static void Trampoline(const os_event* ev, void* ctx) noexcept;
    static Event MakeEvent(const os_event* ev);
};

}  // namespace nano_edr

#endif  // NANO_EDR_OS_SOURCE_H