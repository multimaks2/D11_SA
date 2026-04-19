#pragma once

#include <string_view>

namespace core {

class Logger {
public:
    static void Info(std::string_view message);
};

} // namespace core
