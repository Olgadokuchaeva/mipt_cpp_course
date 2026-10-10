#include <stdexcept>

#include "os_source.h"

namespace nano_edr {

OsSource::OsSource(const std::string& config_path)
    : handle_(config_path) {}


void OsSource::Run(Agent* agent) {
    handle_.Subscribe(&Agent::Trampoline, agent);
    handle_.Start();
    os_status status;
    while ((status = handle_.Wait(1000)) == OS_TIMEOUT) {
    }
    if (status != OS_OK) {
        throw std::runtime_error(std::string("os_wait: ") + os_status_str(status));
    }
}

}  // namespace nano_edr