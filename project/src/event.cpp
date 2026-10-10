#include <charconv>
#include <stdexcept>

#include "event.h"

namespace nano_edr {

Event::Event(const EventParts& parts) {
    if (parts.ts.empty()) {
        throw std::invalid_argument("пустое время события");
    }
    uint64_t ms = 0;
    const char* first = parts.ts.data();
    const char* last  = parts.ts.data() + parts.ts.size();
    auto [ptr, ec] = std::from_chars(first, last, ms);
    if (ec != std::errc{} || ptr != last) {
        throw std::invalid_argument("неверное время события: " + parts.ts);
    }
    if (parts.type.empty()) {
        throw std::invalid_argument("пустой тип события");
    }
    ts_ = Timestamp{ms};
    raw_ts_ = parts.ts;
    type_ = parts.type;
    pid_ = parts.pid;
    fields_ = parts.fields;
}


std::string ToString(const Event& event) {
    std::string result;
    result += "ts=";
    result += event.raw_ts();
    result += " type=";
    result += event.type();
    if (!event.pid().empty()) {
        result += " pid=";
        result += event.pid();
    }
    for (const Field& f : event.fields()) {
        result += ' ';
        result += f.key;
        result += '=';
        result += f.value;
    }
    return result;
}


std::ostream& operator<<(std::ostream& out, const Event& event) {
    return out << ToString(event);
}

}  // namespace nano_edr