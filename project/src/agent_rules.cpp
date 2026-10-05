#include <string>

#include "agent_rules.h"
#include "fields.h"
#include "rules.h"

namespace nano_edr {

namespace {

bool ImageEndsWith(const Event& event, const std::string& name) {
    const std::string& image = GetRequiredField(event, "image");
    std::string image_norm = NormalizePath(image);
    return image_norm.size() >= name.size() && image_norm.compare(image_norm.size() - name.size(), name.size(), name) == 0;
}

bool ScriptHostFromTemp(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }
    if (!ImageEndsWith(event, "wscript.exe") && !ImageEndsWith(event, "cscript.exe")) {
        return false;
    }
    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) {
        return false;
    }
    std::string cmd_norm = NormalizePath(*cmdline);
    return cmd_norm.find("\\appdata\\local\\temp\\") != std::string::npos ||
           cmd_norm.find("\\windows\\temp\\") != std::string::npos;
}


bool LolbinDownload(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }
    if (!ImageEndsWith(event, "certutil.exe") && !ImageEndsWith(event, "bitsadmin.exe")) {
        return false;
    }
    return CommandLineContains(event, "urlcache") || 
           CommandLineContains(event, "transfer") ||
           CommandLineContains(event, "http:") ||
           CommandLineContains(event, "https:");
}


bool HiddenPowershell(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }
    if (!ImageEndsWith(event, "powershell.exe") && !ImageEndsWith(event, "pwsh.exe")) {
        return false;
    }
    return CommandLineContains(event, "-w hidden") || 
           CommandLineContains(event, "-windowstyle hidden") ||
           CommandLineContains(event, "-enc") ||
           CommandLineContains(event, "-encodedcommand");
}


bool AutostartWrite(const Event& event) {
    if (event.type != "file_create" && event.type != "file_write" && event.type != "file_move") {
        return false;
    }
    const std::string* value = nullptr;
    if (event.type == "file_move") {
        value = FindField(event, "to");
    } else {
        value = FindField(event, "path");
    }
    if (value == nullptr) {
        return false;
    }
    std::string value_norm = NormalizePath(*value);
    return value_norm.find("\\start menu\\programs\\startup\\") != std::string::npos;
}


bool RansomExtension(const Event& event) {
    if (event.type != "file_create" && event.type != "file_write" && event.type != "file_move") {
        return false;
    }
    const std::string* value = nullptr;
    if (event.type == "file_move") {
        value = FindField(event, "to");
    } else {
        value = FindField(event, "path");
    }
    if (value == nullptr) {
        return false;
    }
    std::string value_norm = NormalizePath(*value);
    return value_norm.size() >= 7 && value_norm.compare(value_norm.size() - 7, 7, ".locked") == 0;
}


constexpr Rule kRules[] = {
    {"script_host_from_temp", ScriptHostFromTemp, Severity::kHigh},
    {"lolbin_download", LolbinDownload, Severity::kHigh},
    {"hidden_powershell", HiddenPowershell, Severity::kMedium},
    {"autostart_write", AutostartWrite, Severity::kHigh},
    {"ransom_extension", RansomExtension, Severity::kCritical},
};

}  // namespace

const Rule* AgentRules() {
    return kRules;
}


size_t AgentRuleCount() {
    return sizeof(kRules) / sizeof(kRules[0]);
}

}  // namespace nano_edr