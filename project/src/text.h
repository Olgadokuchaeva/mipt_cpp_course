#ifndef NANO_EDR_TEXT_H
#define NANO_EDR_TEXT_H

#include <string>

namespace nano_edr {

bool EqualsIgnoreCase(const std::string& a, const std::string& b);

bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle);

bool EndsWithIgnoreCase(const std::string& s, const std::string& suffix);

}  // namespace nano_edr

#endif  // NANO_EDR_TEXT_H