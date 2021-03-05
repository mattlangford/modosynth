#include "engine/object_manager.hh"

#include "engine/gl.hh"

namespace engine {
//
// #############################################################################
//

AbstractSingleShaderObjectManager::AbstractSingleShaderObjectManager(std::string vertex, std::string fragment,
                                                                     std::optional<std::string> geometry)
    : shader_(std::move(vertex), std::move(fragment), std::move(geometry)) {}

//
// #############################################################################
//

void AbstractSingleShaderObjectManager::init() {
    shader_.init();

    gl_safe(glGenVertexArrays, 1, &vertex_array_object_);
    gl_safe(glBindVertexArray, vertex_array_object_);

    // Call the child function
    init_with_vao();

    gl_safe(glBindVertexArray, 0);

    screen_from_world_loc_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");
    if (const std::string error = get_gl_error(); !error.empty()) {
        throw std::runtime_error(
            std::string("AbstractSingleShaderObjectManager assumes an input variable named 'screen_from_world', but it "
                        "appears there was an error when loading it: ") +
            error);
    }
}

//
// #############################################################################
//

void AbstractSingleShaderObjectManager::render(const Eigen::Matrix3f& screen_from_world) {
    shader_.activate();

    gl_safe(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());

    gl_safe(glBindVertexArray, vertex_array_object_);

    // Call the child function to do the actual rendering
    render_with_vao();

    gl_safe(glBindVertexArray, 0);
    gl_safe(glUseProgram, 0);
}

//
// #############################################################################
//

void AbstractSingleShaderObjectManager::bind_vao() { gl_safe(glBindVertexArray, vertex_array_object_); }

//
// #############################################################################
//

const Shader& AbstractSingleShaderObjectManager::get_shader() const { return shader_; }
}  // namespace engine
