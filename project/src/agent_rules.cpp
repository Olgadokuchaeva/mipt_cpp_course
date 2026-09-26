#include <string>

#include "agent_rules.h"
#include "fields.h"
#include "rules.h"

namespace nano_edr {

namespace {

bool ScriptHostFromTemp(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }
    const std::string& image = GetRequiredField(event, "image");
    std::string image_norm = NormalizePath(image);
    bool is_wscript = image_norm.size() >= 11 && image_norm.compare(image_norm.size() - 11, 11, "wscript.exe") == 0;
    bool is_cscript = image_norm.size() >= 11 && image_norm.compare(image_norm.size() - 11, 11, "cscript.exe") == 0;
    if (!is_wscript && !is_cscript) {
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
    const std::string& image = GetRequiredField(event, "image");
    std::string image_norm = NormalizePath(image);
    bool is_certutil = image_norm.size() >= 12 && image_norm.compare(image_norm.size() - 12, 12, "certutil.exe") == 0;
    bool is_bitsadmin = image_norm.size() >= 13 && image_norm.compare(image_norm.size() - 13, 13, "bitsadmin.exe") == 0;
    if (!is_certutil && !is_bitsadmin) {
        return false;
    }
    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) {
        return false;
    }
    std::string cmd_norm = NormalizePath(*cmdline);
    return cmd_norm.find("urlcache") != std::string::npos ||
           cmd_norm.find("transfer") != std::string::npos ||
           cmd_norm.find("http:") != std::string::npos ||
           cmd_norm.find("https:") != std::string::npos;
}


bool HiddenPowershell(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }
    const std::string& image = GetRequiredField(event, "image");
    std::string image_norm = NormalizePath(image);
    bool is_powershell = image_norm.size() >= 14 && image_norm.compare(image_norm.size() - 14, 14, "powershell.exe") == 0;
    bool is_pwsh = image_norm.size() >= 8 && image_norm.compare(image_norm.size() - 8, 8, "pwsh.exe") == 0;
    if (!is_powershell && !is_pwsh) {
        return false;
    }
    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) {
        return false;
    }
    std::string cmd_norm = NormalizePath(*cmdline);
    return cmd_norm.find("-w hidden") != std::string::npos ||
           cmd_norm.find("-windowstyle hidden") != std::string::npos ||
           cmd_norm.find("-enc") != std::string::npos ||
           cmd_norm.find("-encodedcommand") != std::string::npos;
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