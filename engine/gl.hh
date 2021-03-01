#pragma once

#include <sstream>
#include <string>

namespace engine {
std::string get_gl_error();
void throw_on_gl_error(std::string action = "");
}  // namespace engine
#define gl_safe(func, ...)                                                                           \
    func(__VA_ARGS__);                                                                               \
    do {                                                                                             \
        std::stringstream ss;                                                                        \
        ss << "\n\t" << __FILE__ << "(" << __LINE__ << "): " << #func << "(" << #__VA_ARGS__ << ")"; \
        ::engine::throw_on_gl_error(ss.str());                                                       \
    } while (false)
