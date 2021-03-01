#include "engine/gl.hh"

#include <OpenGL/gl3.h>

#include <sstream>

namespace engine {
void throw_on_gl_error(std::string action) {
    std::stringstream ss;
    if (!action.empty()) ss << action << " failed. ";
    ss << "Error code: 0x" << std::hex;

    std::string error = get_gl_error();
    if (error.empty()) return;
    ss << error;
    throw std::runtime_error(ss.str());
}

//
// #############################################################################
//

#define error_string_case(enum_value) \
    case enum_value:                  \
        return std::to_string(enum_value) + " (" + #enum_value ")"
std::string get_gl_error() {
    auto error = glGetError();
    switch (error) {
        case GL_NO_ERROR:
            return "";
            error_string_case(GL_INVALID_VALUE);
            error_string_case(GL_INVALID_ENUM);
            error_string_case(GL_INVALID_OPERATION);
        default:
            return std::to_string(error);
    }
}
}  // namespace engine
