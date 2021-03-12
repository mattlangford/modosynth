#include "engine/object_manager.hh"

#include <iostream>

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
    vao_.init();

    // Call the child function
    init_with_vao();

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

    gl_check(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());

    // Call the child function to do the actual rendering
    render_with_vao();

    gl_check(glUseProgram, 0);
}

//
// #############################################################################
//

const Shader& AbstractSingleShaderObjectManager::shader() const { return shader_; }
const VertexArrayObject& AbstractSingleShaderObjectManager::vao() const { return vao_; }
}  // namespace engine
