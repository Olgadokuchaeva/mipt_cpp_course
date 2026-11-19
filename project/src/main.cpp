#include <memory>
#include <print>
#include <string>

#include "agent.h"
#include "event_source.h"
#include "os_source.h"
#include "sources.h"
#include "alert_sink.h"
#include "edr_handle.h"
#include "field_traits.h"
#include "copy_stats.h"

namespace {

struct Options {
    std::string path;
    bool quiet = false;
    bool force_file = false;
    std::string out;
    bool copies = false;
    bool dry_run = false;
};

int ParseArgs(int argc, char** argv, Options* opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            opts->quiet = true;
        } else if (arg == "--out") {
            if (i + 1 >= argc) {
                std::print(stderr, "--out требует путь\n");
                return 2;
            }
            ++i;
            opts->out = argv[i];
        } else if (arg == "--dry-run") {
            opts->dry_run = true;
        } else if (arg == "--copies") {
            opts->copies = true;
        } else if (arg == "--file") {
            if (!opts->path.empty()) {
                std::print(stderr, "путь уже задан\n");
                return 2;
            }
            if (i + 1 >= argc) {
                std::print(stderr, "--file требует путь\n");
                return 2;
            }
            ++i;
            opts->path = argv[i];
            opts->force_file = true;
        } else if (!arg.empty() && arg[0] == '-') {
            std::print(stderr, "неизвестный флаг: {}\n", arg);
            return 2;
        } else if (opts->path.empty()) {
            opts->path = arg;
        } else {
            std::print(stderr, "лишний аргумент: {}\n", arg);
            return 2;
        }
    }
    if (opts->path.empty()) {
        std::print(stderr, "использование: nano-edr [--quiet] [--file] [--out <файл>] [--copies] [--dry-run] <путь>\n");
        return 2;
    }
    return 0;
}

}  // namespace


int main(int argc, char** argv) {
    Options opts;
    int code = ParseArgs(argc, argv, &opts);
    if (code != 0) {
        return code;
    }
    try {
        nano_edr::EdrHandle edr(opts.path);
        const std::string text = edr.Config("dedup_period_ms", "10000");
        uint64_t dedup_period_ms = 0;
        if (!nano_edr::FieldTraits<uint64_t>::Parse(text, &dedup_period_ms)) {
            std::print(stderr, "[WARN] dedup_period_ms не разбирается: {}\n", text);
            dedup_period_ms = 10000;
        }
        std::unique_ptr<nano_edr::IEventSource> source;
        if (opts.force_file) {
            source = std::make_unique<nano_edr::FileSource>(opts.path);
        } else {
            source = std::make_unique<nano_edr::OsSource>(opts.path);
        }
        std::string qdir = edr.Config("quarantine_dir", "C:\\quarantine");
        nano_edr::Agent agent(opts.quiet, dedup_period_ms, source->os(), qdir, opts.dry_run);
        nano_edr::FanOutSink targets;
        targets.Add(nano_edr::ConsoleSink());
        if (!opts.out.empty()) {
            targets.Add(nano_edr::FileSink(opts.out));
        }
        targets.Add(nano_edr::EdrSink(&edr));
        agent.Subscribe(std::move(targets));
        source->Run(&agent);
        if (edr.Flush() != EDR_OK) {
            std::print(stderr, "[WARN] детекты не отправлены\n");
        }
        agent.PrintSummary();
        if (opts.copies) {
            std::print("[COPY] строк события скопировано {}, перемещено {}\n",
                       nano_edr::copy_stats().event_strings_copied,
                       nano_edr::copy_stats().event_strings_moved);
            std::print("[COPY] записей WindowEntry скопировано {}, перемещено {}\n",
                       nano_edr::copy_stats().window_copied,
                       nano_edr::copy_stats().window_moved);
            std::print("[COPY] событий скопировано в детекты {}\n",
                       nano_edr::copy_stats().evidence_copied);
            std::print("[COPY] копий всего {}\n",
                       nano_edr::copy_stats().total());
        }
    } catch (const std::exception& error) {
        std::print(stderr, "ошибка: {}\n", error.what());
        return 1;
    }
    return 0;
}