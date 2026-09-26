#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <cstring>
#include <charconv>

#include "parse.h"
#include "event_list.h"
#include "rules.h"
#include "agent_rules.h"

using nano_edr::Event;
using nano_edr::EventList;
using nano_edr::EventNode;
using nano_edr::IsBlankOrComment;
using nano_edr::ParseEventLine;
using nano_edr::ListPushBack;
using nano_edr::CheckRules;
using nano_edr::AgentRules;
using nano_edr::AgentRuleCount;

namespace {

const int kMaxTypes = 64;

void PrintContext(const EventList& window) {
    if (window.size == 0) {
        return;
    }
    const EventNode* prev = nullptr;
    const EventNode* last = nullptr;
    for (const EventNode* it = window.head; it != nullptr; it = it->next) {
        prev = last;
        last = it;
    }
    if (prev != nullptr) {
        std::print("[CTX] -2: ts={} type={} pid={}\n", prev->event.ts, prev->event.type, prev->event.pid);
    }
    if (last != nullptr) {
        std::print("[CTX] -1: ts={} type={} pid={}\n", last->event.ts, last->event.type, last->event.pid);
    }
}


int FindType(const std::string* known_types, int types_seen, const std::string& type) {
    for (int k = 0; k < types_seen; ++k) {
        if (known_types[k] == type) {
            return k;
        }
    }
    return -1;
}

void PrintSummary(long long lines, long long detects, const std::string* types, const long long* counts, int types_seen) {
    std::print("строк {} всего, детектов {}\n", lines, detects);
    std::print("события по типам:\n");
    for (int k = 0; k < types_seen; ++k) {
        std::print(" {}: {}\n", types[k], counts[k]);
    }
}

int Run(const std::string& path, bool quiet, std::size_t window_size) {
    std::ifstream log(path);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", path);
        return 2;
    }
    const nano_edr::Rule* rules = AgentRules();
    const std::size_t rule_count = AgentRuleCount();
    long long lines = 0;
    long long total_detects = 0;
    std::string line;
    std::string known_types[kMaxTypes];
    long long type_counts[kMaxTypes] = {0};
    int types_seen = 0;
    EventList window;
    window.capacity = window_size;
    while (std::getline(log, line)) {
        ++lines;
        if (IsBlankOrComment(&line)) {
            continue;
        }
        Event ev;
        if (!ParseEventLine(&line, &ev)) {
            continue;
        }
        std::size_t detects = CheckRules(ev, rules, rule_count);
        total_detects += static_cast<long long>(detects);
        if (detects > 0 && !quiet) {
            PrintContext(window);
        }
        ListPushBack(&window, &ev);
        int idx = FindType(known_types, types_seen, ev.type);
        if (idx == -1 && types_seen < kMaxTypes) {
            idx = types_seen;
            known_types[idx] = ev.type;
            ++types_seen;
        }
        if (idx != -1) {
            ++type_counts[idx];
        }
    }
    if (!quiet) {
        PrintSummary(lines, total_detects, known_types, type_counts, types_seen);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::string path;
        bool quiet = false;
        std::size_t window_size = 64;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--quiet") {
                quiet = true;
            } else if (arg == "--window-size" && i + 1 < argc) {
                ++i;
                const char* s = argv[i];
                std::size_t value = 0;
                auto [ptr, ec] = std::from_chars(s, s + std::strlen(s), value);
                if (ec == std::errc{}) {
                    window_size = value;
                }
            } else if (path.empty()) {
                path = arg;
            }
        }
        if (path.empty()) {
            std::print(stderr, "использование: nano-edr <журнал.log> [--quiet] [--window-size N]\n");
            return 2;
        }
        return Run(path, quiet, window_size);
    } catch (const std::exception& error) {
        std::print(stderr, "ошибка: {}\n", error.what());
        return 1;
    }
}