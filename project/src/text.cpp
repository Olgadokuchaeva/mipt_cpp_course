#include "text.h"

#include <cctype>

namespace nano_edr {

namespace {

char ToLowerChar(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

}  // namespace


bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (ToLowerChar(a[i]) != ToLowerChar(b[i])) {
            return false;
        }
    }
    return true;
}


bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    if (needle.size() > haystack.size()) {
        return false;
    }
    for (std::size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.size(); ++j) {
            if (ToLowerChar(haystack[i + j]) != ToLowerChar(needle[j])) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}


bool EndsWithIgnoreCase(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) {
        return false;
    }
    std::size_t offset = s.size() - suffix.size();
    for (std::size_t i = 0; i < suffix.size(); ++i) {
        if (ToLowerChar(s[offset + i]) != ToLowerChar(suffix[i])) {
            return false;
        }
    }
    return true;
}

}  // namespace nano_edr