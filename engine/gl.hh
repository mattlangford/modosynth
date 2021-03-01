#pragma once

#include <string>
namespace engine {
void check_gl_error(std::string action = "");
}  // namespace engine
#define gl_safe(func, ...)                                                                                         \
    func(__VA_ARGS__);                                                                                             \
    ::engine::check_gl_error(std::string("\nline ") + std::to_string(__LINE__) + ": " + std::string(#func) + "(" + \
                             std::string(#__VA_ARGS__) + ")")
