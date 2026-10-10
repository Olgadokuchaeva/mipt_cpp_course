#include <charconv>
#include <cstring>
#include <print>
#include <string>
#include <vector>

#include "agent.h"
#include "agent_rules.h"
#include "sources.h"
#include "os_source.h"
#include "rules.h"

namespace {

struct Args {
    std::string path;
    bool quiet = false;
    std::size_t window_size = 64;
    bool file = false;
    std::vector<std::string> disabled;
};

int ParseArgs(int argc, char** argv, Args* args) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            args->quiet = true;
        } else if (arg == "--window-size" && i + 1 < argc) {
            ++i;
            const char* s = argv[i];
            std::size_t value = 0;
            auto [ptr, ec] = std::from_chars(s, s + std::strlen(s), value);
            if (ec == std::errc{}) {
                args->window_size = value;
            }
        } else if (arg == "--disable") {
            if (i + 1 >= argc) {
                std::print(stderr, "--disable требует имя правила\n");
                return 2;
            }
            ++i;
            std::string name = argv[i];
            const nano_edr::Rule* rules = nano_edr::AgentRules();
            std::size_t count = nano_edr::AgentRuleCount();
            bool known = false;
            for (std::size_t k = 0; k < count; ++k) {
                if (name == rules[k].id) {
                    known = true;
                    break;
                }
            }
            if (!known) {
                std::print(stderr, "неизвестное правило: {}\n", name);
                return 2;
            }
            args->disabled.push_back(name);
        } else if (arg == "--file") {
            if (!args->path.empty()) {
                std::print(stderr, "путь уже задан\n");
                return 2;
            }
            if (i + 1 >= argc) {
                std::print(stderr, "--file требует путь\n");
                return 2;
            }
            ++i;
            args->path = argv[i];
            args->file = true;
        } else if (args->path.empty()) {
            args->path = arg;
        } else {
            std::print(stderr, "лишний аргумент: {}\n", arg);
            return 2;
        }
    }
    if (args->path.empty()) {
        std::print(stderr, "использование: nano-edr [--quiet] [--window-size N] [--disable <имя>] [--file] <путь>\n");
        return 2;
    }
    return 0;
}

}  // namespace


int main(int argc, char** argv) {
    Args args;
    int code = ParseArgs(argc, argv, &args);
    if (code != 0) {
        return code;
    }
    try {
        nano_edr::Agent agent(args.window_size, args.quiet, args.disabled);
        if (args.file) {
            nano_edr::FileSource source(args.path);
            source.Run(&agent);
        } else {
            nano_edr::OsSource source(args.path);
            source.Run(&agent);
        }
        agent.PrintSummary();
    } catch (const std::exception& error) {
        std::print(stderr, "ошибка: {}\n", error.what());
        return 1;
    }
    return 0;
}