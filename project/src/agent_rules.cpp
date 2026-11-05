#include "agent_rules.h"

#include <memory>
#include <vector>

#include "conditions.h"
#include "event.h"
#include "rule.h"

namespace nano_edr {

namespace {

std::unique_ptr<ICondition> TempPathContains() {
    auto any = std::make_unique<AnyOf>();
    any->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    any->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    any->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    any->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    any->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    any->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    return any;
}


std::unique_ptr<IRule> MakeScriptHostFromTemp() {
    auto rule = std::make_unique<MatchRule>(
        "script_host_from_temp", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kProcessStart}));
    auto image = std::make_unique<AnyOf>();
    image->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    image->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    rule->AddCondition(std::move(image));
    rule->AddCondition(TempPathContains());
    rule->AddAction("kill_process");
    return rule;
}


std::unique_ptr<IRule> MakeLolbinDownload() {
    auto rule = std::make_unique<MatchRule>(
        "lolbin_download", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kProcessStart}));
    auto image = std::make_unique<AnyOf>();
    image->Add(std::make_unique<FieldEndsWith>("image", "certutil.exe"));
    image->Add(std::make_unique<FieldEndsWith>("image", "bitsadmin.exe"));
    rule->AddCondition(std::move(image));
    auto cmd = std::make_unique<AnyOf>();
    cmd->Add(std::make_unique<FieldContains>("cmdline", "urlcache"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "transfer"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "http:"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "https:"));
    rule->AddCondition(std::move(cmd));
    rule->AddAction("kill_process");
    return rule;
}


std::unique_ptr<IRule> MakeHiddenPowershell() {
    auto rule = std::make_unique<MatchRule>(
        "hidden_powershell", Severity::kMedium);
    rule->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kProcessStart}));
    auto image = std::make_unique<AnyOf>();
    image->Add(std::make_unique<FieldEndsWith>("image", "powershell.exe"));
    image->Add(std::make_unique<FieldEndsWith>("image", "pwsh.exe"));
    rule->AddCondition(std::move(image));
    auto cmd = std::make_unique<AnyOf>();
    cmd->Add(std::make_unique<FieldContains>("cmdline", "-w hidden"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "-windowstyle hidden"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "-enc"));
    cmd->Add(std::make_unique<FieldContains>("cmdline", "-encodedcommand"));
    rule->AddCondition(std::move(cmd));
    rule->AddAction("kill_process");
    return rule;
}


std::unique_ptr<IRule> MakeAutostartWrite() {
    auto rule = std::make_unique<MatchRule>(
        "autostart_write", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{
            EventType::kFileCreate,
            EventType::kFileWrite,
            EventType::kFileMove}));
    auto path = std::make_unique<AnyOf>();
    path->Add(std::make_unique<FieldContains>("path", "\\start menu\\programs\\startup\\"));
    path->Add(std::make_unique<FieldContains>("path", "/start menu/programs/startup/"));
    path->Add(std::make_unique<FieldContains>("to", "\\start menu\\programs\\startup\\"));
    path->Add(std::make_unique<FieldContains>("to", "/start menu/programs/startup/"));
    rule->AddCondition(std::move(path));
    rule->AddAction("quarantine_file");
    return rule;
}


std::unique_ptr<IRule> MakeRansomExtension() {
    auto rule = std::make_unique<MatchRule>(
        "ransom_extension", Severity::kCritical);
    rule->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{
            EventType::kFileCreate,
            EventType::kFileWrite,
            EventType::kFileMove}));
    auto path = std::make_unique<AnyOf>();
    path->Add(std::make_unique<FieldEndsWith>("path", ".locked"));
    path->Add(std::make_unique<FieldEndsWith>("to", ".locked"));
    rule->AddCondition(std::move(path));
    rule->AddAction("quarantine_file");
    return rule;
}

std::unique_ptr<IRule> MakeScriptHostBeacon() {
    auto rule = std::make_unique<SequenceRule>(
        "script_host_beacon", Severity::kCritical, 10000);
    auto start = std::make_unique<AllOf>();
    start->Add(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kProcessStart}));
    auto image = std::make_unique<AnyOf>();
    image->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    image->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    start->Add(std::move(image));
    start->Add(TempPathContains());
    rule->SetFirst(std::move(start));
    rule->SetSecond(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kNetConnect}));
    rule->AddAction("kill_process");
    return rule;
}

}  // namespace


RuleEngine BuildAgentRules() {
    RuleEngine engine;
    engine.AddRule(MakeScriptHostFromTemp());
    engine.AddRule(MakeLolbinDownload());
    engine.AddRule(MakeHiddenPowershell());
    engine.AddRule(MakeAutostartWrite());
    engine.AddRule(MakeRansomExtension());
    engine.AddRule(MakeScriptHostBeacon());
    return engine;
}

}  // namespace nano_edr