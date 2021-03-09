#pragma once

#include <OpenGL/gl3.h>

#include <sstream>
#include <string>

#include "engine/vao.hh"

namespace engine {
std::string get_gl_error();
void throw_on_gl_error(std::string action = "");
}  // namespace engine
#define gl_check(func, ...)                                                                           \
    func(__VA_ARGS__);                                                                               \
    do {                                                                                             \
        std::stringstream ss;                                                                        \
        ss << "\n\t" << __FILE__ << "(" << __LINE__ << "): " << #func << "(" << #__VA_ARGS__ << ")"; \
        ::engine::throw_on_gl_error(ss.str());                                                       \
    } while (false)

#define gl_check_with_vao(vao, func, ...) \
    do { \
        scoped_vao_bind(vao); \
        gl_check(func, __VA_ARGS__);\
    } while(false)\
