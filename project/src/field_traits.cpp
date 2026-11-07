#include "field_traits.h"

#include <charconv>

#include "text.h"

namespace nano_edr {

bool FieldTraits<std::string>::Parse(const std::string& text, std::string* out) {
    *out = text;
    return true;
}

bool FieldTraits<uint64_t>::Parse(const std::string& text, uint64_t* out) {
    uint64_t result = 0;
    const char* first = text.data();
    const char* last  = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(first, last, result);
    if (ec != std::errc{} || ptr != last) {
        return false;
    }
    *out = result;
    return true;
}

bool FieldTraits<bool>::Parse(const std::string& text, bool* out) {
    if (EqualsIgnoreCase(text, "1") ||
        EqualsIgnoreCase(text, "true") ||
        EqualsIgnoreCase(text, "yes")) {
        *out = true;
        return true;
    }
    if (EqualsIgnoreCase(text, "0") ||
        EqualsIgnoreCase(text, "false") ||
        EqualsIgnoreCase(text, "no")) {
        *out = false;
        return true;
    }
    return false;
}

}  // namespace nano_edr