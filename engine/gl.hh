#include <OpenGL/gl3.h>

#include <sstream>
#include <string>

namespace engine {
void check_gl_error(std::string action = "") {
    std::stringstream ss;
    if (!action.empty()) ss << action << " failed. ";
    ss << "Error code: 0x" << std::hex;

    auto error = glGetError();
    switch (error) {
        case GL_NO_ERROR:
            return;
        case GL_INVALID_ENUM:
            ss << GL_INVALID_ENUM << " (GL_INVALID_ENUM).";
            break;
        case GL_INVALID_VALUE:
            ss << GL_INVALID_VALUE << " (GL_INVALID_VALUE).";
            break;
        case GL_INVALID_OPERATION:
            ss << GL_INVALID_OPERATION << " (GL_INVALID_OPERATION).";
            break;
        default:
            ss << error;
            break;
    }
    throw std::runtime_error(ss.str());
}
}  // namespace engine
#define gl_safe(func, ...)                                                                                         \
    func(__VA_ARGS__);                                                                                             \
    ::engine::check_gl_error(std::string("\nline ") + std::to_string(__LINE__) + ": " + std::string(#func) + "(" + \
                             std::string(#__VA_ARGS__) + ")")
