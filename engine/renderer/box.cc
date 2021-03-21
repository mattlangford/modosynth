#include "engine/renderer/box.hh"

namespace engine::renderer {
namespace {
static std::string vertex_shader_text = R"(
#version 330
uniform mat3 screen_from_world;
layout (location = 0) in vec2 world_position;
layout (location = 1) in vec4 in_color;

out vec4 color;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, 0.0, 1.0);
    color = in_color;
}
)";

static std::string fragment_shader_text = R"(
#version 330
in vec4 color;
out vec4 fragment;

void main()
{
    fragment = color;
}
)";
}  // namespace

//
// #############################################################################
//

BoxRenderer::BoxRenderer() : shader_(vertex_shader_text, fragment_shader_text) {}

//
// #############################################################################
//

void BoxRenderer::init() {
    shader_.init();
    screen_from_world_loc_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");

    vao_.init();

    position_buffer_.init(GL_ARRAY_BUFFER, 0, vao_);
    color_buffer_.init(GL_ARRAY_BUFFER, 1, vao_);

    position_buffer_.resize(8 * 4);
    color_buffer_.resize(8 * 4);
}

//
// #############################################################################
//

void BoxRenderer::operator()(const Box& box, const Eigen::Matrix3f& screen_from_world) {
    shader_.activate();
    gl_check(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());

    set_color(box.color);
    set_position(box.center, box.dim);

    gl_check_with_vao(vao_, glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
}

//
// #############################################################################
//

void BoxRenderer::set_color(const Eigen::Vector3f& color) {
    auto batch = color_buffer_.batched_updater();
    const Eigen::Vector4f with_alpha{color[0], color[1], color[2], 1.0};
    for (size_t i = 0; i < 4; ++i) batch.element(i) = with_alpha;
}

//
// #############################################################################
//

void BoxRenderer::set_position(const Eigen::Vector2f& center, const Eigen::Vector2f& dim) {
    auto batch = position_buffer_.batched_updater();

    // top left
    batch.element(0) = center + Eigen::Vector2f{-dim.x(), dim.y()};
    // top right
    batch.element(1) = center + Eigen::Vector2f{dim.x(), dim.y()};
    // bottom left
    batch.element(2) = center + Eigen::Vector2f{-dim.x(), -dim.y()};
    // bottom right
    batch.element(3) = center + Eigen::Vector2f{dim.x(), -dim.y()};
}
};  // namespace engine::renderer
