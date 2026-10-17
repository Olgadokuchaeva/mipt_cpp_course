#include "agent_rules.h"

namespace nano_edr {

std::vector<MatchRule> BuildAgentRules() {
    std::vector<MatchRule> rules;

    {
        MatchRule rule("script_host_from_temp", Severity::kHigh);
        rule.AddCondition(Condition::Equals("type", {"process_start"}));
        rule.AddCondition(
            Condition::EndsWith("image", {"wscript.exe", "cscript.exe"}));
        rule.AddCondition(Condition::Contains("cmdline", {
            "\\appdata\\local\\temp\\", "/appdata/local/temp/",
            "\\windows\\temp\\", "/windows/temp/",
            "%temp%", "%tmp%"
        }));
        rule.AddAction("kill_process");
        rules.push_back(rule);
    }

    {
        MatchRule rule("lolbin_download", Severity::kHigh);
        rule.AddCondition(Condition::Equals("type", {"process_start"}));
        rule.AddCondition(
            Condition::EndsWith("image", {"certutil.exe", "bitsadmin.exe"}));
        rule.AddCondition(Condition::Contains("cmdline", {
            "urlcache", "transfer", "http:", "https:"
        }));
        rule.AddAction("kill_process");
        rules.push_back(rule);
    }

    {
        MatchRule rule("hidden_powershell", Severity::kMedium);
        rule.AddCondition(Condition::Equals("type", {"process_start"}));
        rule.AddCondition(
            Condition::EndsWith("image", {"powershell.exe", "pwsh.exe"}));
        rule.AddCondition(Condition::Contains("cmdline", {
            "-w hidden", "-windowstyle hidden", "-enc", "-encodedcommand"
        }));
        rule.AddAction("kill_process");
        rules.push_back(rule);
    }

    {
        MatchRule rule("autostart_write", Severity::kHigh);
        rule.AddCondition(Condition::Equals("type", {"file_create", "file_write"}));
        rule.AddCondition(Condition::Contains(
            "path", {"\\start menu\\programs\\startup\\",
                     "/start menu/programs/startup/"}));
        rule.AddAction("quarantine_file");
        rules.push_back(rule);
    }

    {
        MatchRule rule("autostart_write", Severity::kHigh);
        rule.AddCondition(Condition::Equals("type", {"file_move"}));
        rule.AddCondition(Condition::Contains(
            "to", {"\\start menu\\programs\\startup\\",
                   "/start menu/programs/startup/"}));
        rule.AddAction("quarantine_file");
        rules.push_back(rule);
    }

    {
        MatchRule rule("ransom_extension", Severity::kCritical);
        rule.AddCondition(Condition::Equals("type", {"file_create", "file_write"}));
        rule.AddCondition(Condition::EndsWith("path", {".locked"}));
        rule.AddAction("quarantine_file");
        rules.push_back(rule);
    }

    {
        MatchRule rule("ransom_extension", Severity::kCritical);
        rule.AddCondition(Condition::Equals("type", {"file_move"}));
        rule.AddCondition(Condition::EndsWith("to", {".locked"}));
        rule.AddAction("quarantine_file");
        rules.push_back(rule);
    }

    return rules;
}

}  // namespace nano_edr