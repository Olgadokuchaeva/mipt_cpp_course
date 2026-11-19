#include <charconv>
#include <stdexcept>
#include <utility>

#include "event.h"
#include "copy_stats.h"

namespace nano_edr {

EventType EventTypeFromString(const std::string& type) {
    if (type == "process_start") {
        return EventType::kProcessStart;
    }
    if (type == "process_end") {
        return EventType::kProcessEnd;
    }
    if (type == "file_create") {
        return EventType::kFileCreate;
    }
    if (type == "file_write") {
        return EventType::kFileWrite;
    }
    if (type == "file_delete") {
        return EventType::kFileDelete;
    }
    if (type == "file_move") {
        return EventType::kFileMove;
    }
    if (type == "net_connect") {
        return EventType::kNetConnect;
    }
    return EventType::kOther;
}

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
    event_type_ = EventTypeFromString(parts.type);
    pid_ = parts.pid;
    fields_ = parts.fields;
    copy_stats().event_strings_copied += 3 + 2 * fields_.size();
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

Timestamp Event::Validate(const std::string& raw_ts, const std::string& type) {
    if (raw_ts.empty()) {
        throw std::invalid_argument("пустое время события");
    }
    uint64_t ms = 0;
    auto [ptr, ec] = std::from_chars(raw_ts.data(), raw_ts.data() + raw_ts.size(), ms);
    if (ec != std::errc{} || ptr != raw_ts.data() + raw_ts.size()) {
        throw std::invalid_argument("неверное время события: " + raw_ts);
    }
    if (type.empty()) {
        throw std::invalid_argument("пустой тип события");
    }
    return Timestamp{ms};
}

Event::Event(EventParts&& parts) {
    ts_ = Validate(parts.ts, parts.type);
    raw_ts_ = std::move(parts.ts);
    type_ = std::move(parts.type);
    event_type_ = EventTypeFromString(type_);
    pid_ = std::move(parts.pid);
    fields_ = std::move(parts.fields);
        copy_stats().event_strings_moved += 3 + 2 * fields_.size();
}


std::ostream& operator<<(std::ostream& out, const Event& event) {
    return out << ToString(event);
}

}  // namespace nano_edr