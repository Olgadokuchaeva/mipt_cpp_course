#include <cctype>
#include <stdexcept>
#include <charconv>

#include "event.h"

namespace nano_edr {

const std::string* FindField(const Event& event, const std::string& key) {
    for (const Field& f : event.fields) {
        if (f.key == key) {
            return &f.value;
        }
    }
    return nullptr;
}


const std::string& GetRequiredField(const Event& event, const std::string& key) {
    const std::string* value = FindField(event, key);
    if (value == nullptr) {
        throw std::invalid_argument("обязательное поле отсутствует: " + key);
    }
    return *value;
}


bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    const std::string* value = FindField(event, key);
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
    return event.type == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type == "file_write";
}

bool IsNetConnect(const Event& event) {
    return event.type == "net_connect";
}


namespace {

std::string ToLowerAndSlash(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        if (c == '/') {
            result.push_back('\\');
        } else {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return result;
}

} // namespace


bool PathEndsWith(const Event& event, const std::string& suffix) {
    const std::string* path = FindField(event, "path");
    if (path == nullptr) {
        return false;
    }
    std::string lower_path = ToLowerAndSlash(*path);
    std::string lower_suffix = ToLowerAndSlash(suffix);
    if (lower_suffix.size() > lower_path.size()) {
        return false;
    }
    return lower_path.compare(lower_path.size() - lower_suffix.size(), lower_suffix.size(), lower_suffix) == 0;
}


bool CommandLineContains(const Event& event, const std::string& search_str) {
    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) {
        return false;
    }
    std::string lower_cmd = ToLowerAndSlash(*cmdline);
    std::string lower_search_str = ToLowerAndSlash(search_str);
    return lower_cmd.find(lower_search_str) != std::string::npos;
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
                std::string lower_name = ToLowerAndSlash(name);
                if (lower_name == "temp" || lower_name == "tmp") {
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

