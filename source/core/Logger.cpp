#include "core/Logger.h"

#include <iostream>

namespace core {

void Logger::Info(std::string_view message) {
    std::cout << "[INFO] " << message << '\n' << std::flush;
}

} // namespace core
