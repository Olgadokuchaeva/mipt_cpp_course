#include <fstream>
#include <stdexcept>

#include "sources.h"
#include "parse.h"

namespace nano_edr {

FileSource::FileSource(const std::string& path) : path_(path) {}


void FileSource::Run(Agent* agent) {
    std::ifstream log(path_);
    if (!log) {
        throw std::runtime_error("не удалось открыть журнал: " + path_);
    }
    std::string line;
    while (std::getline(log, line)) {
        if (IsBlankOrComment(line)) {
            continue;
        }
        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            continue;
        }
        try {
            Event event(parts);
            agent->HandleEvent(event);
        } catch (const std::exception&) {
            continue;
        }
    }
}

}  // namespace nano_edr