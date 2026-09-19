#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <map>
#include <vector>
#include <cstring>

#include "parse.h"
#include "event_list.h"

using nano_edr::Event;
using nano_edr::EventList;
using nano_edr::EventNode;
using nano_edr::IsBlankOrComment;
using nano_edr::ParseEventLine;
using nano_edr::ListPushBack;

namespace {

void PrintContext(const EventList& window) {
    if (window.size == 0) {
        return;
    }
    std::vector<const EventNode*> nodes;
    nodes.reserve(window.size);
    for (const EventNode* it = window.head; it != nullptr; it = it->next) {
        nodes.push_back(it);
    }
    if (nodes.size() >= 2) {
        const EventNode* prev = nodes[nodes.size() - 2];
        const EventNode* last = nodes[nodes.size() - 1];
        std::print("[CTX] -2: ts={} type={} pid={}\n", prev->event.ts, prev->event.type, prev->event.pid);
        std::print("[CTX] -1: ts={} type={} pid={}\n", last->event.ts, last->event.type, last->event.pid);
    } else {
        const EventNode* last = nodes[0];
        std::print("[CTX] -1: ts={} type={} pid={}\n", last->event.ts, last->event.type, last->event.pid);
    }
}

}


int main(int argc, char** argv) {
    std::string path;
    bool quiet = false;
    std::size_t window_size = 64;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            quiet = true;
        } else if (arg == "--window-size") {
            if (i + 1 < argc) {
                ++i;
                const char* s = argv[i];
                std::size_t value = 0;
                auto [ptr, ec] = std::from_chars(s, s + std::strlen(s), value);
                if (ec == std::errc{}) {
                    window_size = value;
                }
            }
        } else if (path.empty()) {
            path = arg;
        }
    }
    if (path.empty()) {
        std::print(stderr, "использование: nano-edr <журнал.log> [--quiet] [--window-size N]\n");
        return 2;
    }
    std::ifstream log(path);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", argv[1]);
        return 2;
    }
    long long lines = 0;
    std::string line;
    const std::vector<std::string> signs = {"wscript.exe", ".locked", "certutil.exe", "\\Startup\\"};
    std::map<std::string, long long> type_counts;
    EventList window;
    window.capacity = window_size;
    while (std::getline(log, line)) {
        ++lines;
        if (IsBlankOrComment(&line)) {
            continue;
        }
        Event ev;
        if (!(ParseEventLine(&line, &ev))) {
            continue;
        }
        bool have_detects = false;
        for (const std::string& sign : signs) {
            if (line.find(sign) != std::string::npos) {
                have_detects = true;
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, sign, line);
            }
        }
        if (have_detects && !quiet) {
            PrintContext(window);
        }
        ListPushBack(&window, &ev);
        ++type_counts[ev.type];
    }
    if (!quiet) {
        std::print("строк {} всего\n", lines);
        std::print("события по типам:\n");
        for (const auto& [type, count] : type_counts) {
            std::print(" {}: {}\n", type, count);
        }
    }
    return 0;
}