#include <fstream>
#include <stdexcept>

#include "sources.h"
#include "parse.h"

namespace nano_edr {

FileSource::FileSource(const std::string& path) : path_(path) {}


void FileSource::Run(IEventHandler* handler) {
    std::ifstream log(path_);
    if (!log) {
        throw std::runtime_error("не удалось открыть журнал: " + path_);
    }
    std::string line;
    while (std::getline(log, line)) {
        if (IsBlankOrComment(line)) {
            handler->OnSkipped();
            continue;
        }
        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            handler->OnBroken();
            continue;
        }
        try {
            Event event(parts);
            handler->HandleEvent(event);
        } catch (const std::exception& e) {
            handler->OnBroken();
        }
    }
}

}  // namespace nano_edr