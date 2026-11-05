#include <stdexcept>

#include "os_source.h"

namespace nano_edr {

namespace {

constexpr uint32_t kWaitMs = 1000;

}  // namespace

OsSource::OsSource(const std::string& config_path)
    : handle_(config_path) {}


void OsSource::Run(IEventHandler* handler) {
    handle_.Subscribe(&OsSource::Trampoline, handler);
    handle_.Start();
    os_status status = handle_.Wait(kWaitMs);
    while (status == OS_TIMEOUT) {
        handler->OnTick();
        status = handle_.Wait(kWaitMs);
    }
    if (status != OS_OK) {
        throw std::runtime_error(std::string("os_wait: ") + os_status_str(status));
    }
}


Event OsSource::MakeEvent(const os_event* ev) {
    EventParts parts;
    parts.ts = std::to_string(ev->ts);
    if (ev->type != nullptr) {
        parts.type = ev->type;
    }
    if (ev->pid == 0) {
        parts.pid = "";
    } else {
        parts.pid = std::to_string(ev->pid);
    }
    for (std::size_t i = 0; i < ev->field_count; ++i) {
        Field f;
        f.key = ev->fields[i].key ? ev->fields[i].key : "";
        f.value = ev->fields[i].value ? ev->fields[i].value : "";
        parts.fields.push_back(f);
    }
    return Event(parts);
}


void OsSource::Trampoline(const os_event* ev, void* ctx) noexcept {
    IEventHandler* handler = static_cast<IEventHandler*>(ctx);
    try {
        Event event = MakeEvent(ev);
        handler->HandleEvent(event);
    } catch (const std::exception&) {
        handler->OnBroken();
    } catch (...) {
        handler->OnBroken();
    }
}

}  // namespace nano_edr