#include "detection.h"
#include <string>
#include "rules.h"

namespace nano_edr {

std::string ToString(const Detection& detection) {
    std::string result;
    result += "[DETECT] ";
    result += SeverityName(detection.severity);
    result += "  ";
    result += detection.rule;
    result += "  ts=";
    result += std::to_string(detection.ts.ms);
    result += " pid=";
    result += detection.pid;
    if (!detection.chain.empty()) {
        result += " chain=";
        result += detection.chain;
    }
    return result;
}

}  // namespace nano_edr