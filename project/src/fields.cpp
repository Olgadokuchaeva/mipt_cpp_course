#include "fields.h"
#include "text.h"

#include <cctype>
#include <stdexcept>
#include <charconv>


namespace nano_edr {

namespace {

const std::string* LookupField(const Event& event, const std::string& key) {
    if (key == "ts") {
        return &event.raw_ts();
    }
    if (key == "type") {
        return &event.type();
    }
    if (key == "pid") {
        return &event.pid();
    }
    for (const Field& f : event.fields()) {
        if (f.key == key) {
            return &f.value;
        }
    }
    return nullptr;
}

char ToLowerChar(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

}  // namespace

const std::string* FindField(const Event& event, const std::string& key) {
    return LookupField(event, key);
}


const std::string& GetRequiredField(const Event& event, const std::string& key) {
    const std::string* value = LookupField(event, key);
    if (value == nullptr) {
        throw std::invalid_argument("обязательное поле отсутствует: " + key);
    }
    return *value;
}


bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    const std::string* value = LookupField(event, key);
    if (value == nullptr) {
        return false;
    }
    uint64_t result = 0;
    const char* first = value->data();
    const char* last = value->data() + value->size();
    auto [ptr, ec] = std::from_chars(first, last, result);
    if (ec != std::errc{} || ptr != last) {
        return false;
    }
    *out = result;
    return true;
}


uint64_t GetIntField(const Event& event, const std::string& key, uint64_t fallback) {
    uint64_t value = 0;
    if (GetIntField(event, key, &value)) {
        return value;
    }
    return fallback;
}


bool IsProcessStart(const Event& event) {
    return event.type() == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type() == "file_write";
}

bool IsNetConnect(const Event& event) {
    return event.type() == "net_connect";
}


bool PathEndsWith(const Event& event, const std::string& suffix) {
    const std::string* path = FindField(event, "path");
    if (path == nullptr) {
        return false;
    }
    return EndsWithIgnoreCase(*path, suffix);
}


bool CommandLineContains(const Event& event, const std::string& needle) {
    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) {
        return false;
    }
    return ContainsIgnoreCase(*cmdline, needle);
}

std::string NormalizePath(const std::string& path) {
    std::string result;
    result.reserve(path.size());
    std::size_t i = 0;
    while (i < path.size()) {
        if (path[i] == '%') {
            std::size_t end = path.find('%', i + 1);
            if (end != std::string::npos) {
                std::string name = path.substr(i + 1, end - i - 1);
                for (char& c : name) {
                    c = ToLowerChar(c);
                }
                if (name == "temp" || name == "tmp") {
                    result += "\\appdata\\local\\temp";
                    i = end + 1;
                    continue;
                }
            }
        }
        char c = path[i];
        if (c == '/') {
            c = '\\';
        } else {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (c == '\\' && !result.empty() && result.back() == '\\') {
            ++i;
            continue;
        }
        result.push_back(c);
        ++i;
    }
    return result;
}

}  // namespace nano_edr

