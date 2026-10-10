#ifndef NANO_EDR_FILE_SOURCE_H
#define NANO_EDR_FILE_SOURCE_H

#include <string>

#include "agent.h"

namespace nano_edr {

class FileSource {
 public:
    explicit FileSource(const std::string& path);
    void Run(Agent* agent);

 private:
    std::string path_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_FILE_SOURCE_H