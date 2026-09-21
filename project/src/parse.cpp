#include <string>
#include "event.h"

namespace nano_edr {

bool IsBlankOrComment(const std::string* line) {
    if (!line) {
        return true;
    }
    const std::string& s = *line;
    std::size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        ++i;
    }
    if (i == s.size() || s[i] == '#' || s[i] == ';') {
        return true;
    }
    return false;
}


bool ParseEventLine(const std::string* line, Event* out) {
    if (!line || !out) {
        return false;
    }
    if (IsBlankOrComment(line)) {
        return false;
    }
    *out = Event{};
    bool have_ts = false, have_type = false, have_pid = false;
    const std::string& s = *line;
    std::size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
            ++i;
        }
        if (i >= s.size()) {
            break;
        }
        std::size_t key_start = i;
        while (i < s.size() && s[i] != '=' && s[i] != ' ' && s[i] != '\t') {
            ++i;
        }
        if (i >= s.size() || s[i] != '=') {
            return false;
        }
        std::string key = s.substr(key_start, i - key_start);
        if (key.empty()) {
            return false;
        }
        ++i;
        std::size_t value_start = i;
        std::size_t value_len = 0;
        if (i < s.size() && s[i] == '"') {
            ++i;
            value_start = i;
            while (i < s.size() && s[i] != '"') {
                ++i;
            }
            if (i >= s.size()) {
                return false;
            }
            value_len = i - value_start;
            ++i;
            if (i < s.size() && s[i] != ' ' && s[i] != '\t') {
                return false;
            }
        } else {
            while (i < s.size() && s[i] != ' ' && s[i] != '\t') {
                ++i;
            }
            value_len = i - value_start;
        }
        std::string value = s.substr(value_start, value_len);
        if (key == "ts" && !(have_ts)) {
            out->ts = value;
            have_ts = true;
        } else if (key == "type" && !(have_type)) {
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
    return have_ts && have_type;
}

}  // namespace nano_edr
