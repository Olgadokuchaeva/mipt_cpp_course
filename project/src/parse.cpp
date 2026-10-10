#include <string>

#include "event.h"

namespace nano_edr {

namespace {

bool IsSpace(char c) {
    return (c == ' ' || c == '\t');
}

} // namespace

bool IsBlankOrComment(const std::string& line) {
    std::size_t i = 0;
    while (i < line.size() && IsSpace(line[i])) {
        ++i;
    }
    if (i == line.size() || line[i] == '#' || line[i] == ';') {
        return true;
    }
    return false;
}


bool ParseEventParts(const std::string& line, EventParts* out) {
    if (IsBlankOrComment(line)) {
        return false;
    }
    *out = EventParts{};
    bool have_ts = false;
    bool have_type = false;
    bool have_pid = false;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && IsSpace(line[i])) {
            ++i;
        }
        if (i >= line.size()) {
            break;
        }
        std::size_t key_start = i;
        while (i < line.size() && line[i] != '=' && line[i] != ' ' && line[i] != '\t') {
            ++i;
        }
        if (i >= line.size() || line[i] != '=') {
            return false;
        }
        std::string key = line.substr(key_start, i - key_start);
        if (key.empty()) {
            return false;
        }
        ++i;
        std::size_t value_start = i;
        std::size_t value_len = 0;
        if (i < line.size() && line[i] == '"') {
            ++i;
            value_start = i;
            while (i < line.size() && line[i] != '"') {
                ++i;
            }
            if (i >= line.size()) {
                return false;
            }
            value_len = i - value_start;
            ++i;
            if (i < line.size() && line[i] != ' ' && line[i] != '\t') {
                return false;
            }
        } else {
            while (i < line.size() && line[i] != ' ' && line[i] != '\t') {
                ++i;
            }
            value_len = i - value_start;
        }
        std::string value = line.substr(value_start, value_len);
        if (key == "ts" && !(have_ts) && !(value.empty())) {
            out->ts = value;
            have_ts = true;
        } else if (key == "type" && !(have_type) && !(value.empty())) {
            out->type = value;
            have_type = true;
        } else if (key == "pid" && !(have_pid)) {
            out->pid = value;
            have_pid = true;
        } else {
            Field field;
            field.key = key;
            field.value = value;
            out->fields.push_back(field);
        }
    }
    return true;
}

}  // namespace nano_edr
