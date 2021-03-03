#include "objects/grid.hh"

#include <OpenGL/gl3.h>

#include "engine/gl.hh"
#include <iostream>

namespace objects {
namespace {
static std::string vertex_shader_text = R"(
#version 330
uniform mat3 screen_from_world;
in vec2 world_position;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, 0.0, 1.0);
}
)";

static std::string fragment_shader_text = R"(
#version 330
out vec4 fragment;

void main()
{
    fragment = vec4(0.3, 0.3, 0.3, 1.0);
}
)";

static std::string geometry_shader_text = R"(
#version 330 core
layout (lines) in;
layout (line_strip, max_vertices = 256) out;

out float color;

void vertical_line(float x) {
    gl_Position = vec4(x, 1.0, 0.0, 1.0);
    EmitVertex();
    gl_Position = vec4(x, -1.0, 0.0, 1.0);
    EmitVertex();
    EndPrimitive();
}

void horizontal_line(float y) {
    gl_Position = vec4(1.0, y, 0.0, 1.0);
    EmitVertex();
    gl_Position = vec4(-1.0, y, 0.0, 1.0);
    EmitVertex();
    EndPrimitive();
}

void main() {
    float start_x = gl_in[0].gl_Position.x;
    float end_x = gl_in[1].gl_Position.x;
    float start_y = gl_in[0].gl_Position.y;
    float end_y = gl_in[1].gl_Position.y;

    float grid_width = end_x - start_x;
    // Note: this will be negative since Y is inverted, The math works itself out with the negative though.
    float grid_height = end_y - start_y;

    float top_left_x = -1.0;
    float top_left_y = 1.0;
    float bottom_right_x = 1.0;
    float bottom_right_y = -1.0;

    // Note: unlike std::fmod(), this keeps the sign of the rhs (so mod(-0.1, 1.0) is 0.9, instead of -0.1)
    start_x = top_left_x + mod(start_x, grid_width);
    start_y = top_left_y + mod(start_y, grid_height);

    while (start_x <= bottom_right_x)
    {
        vertical_line(start_x);
        start_x += grid_width;
    }
    while (start_y >= bottom_right_y)
    {
        horizontal_line(start_y);
        start_y += grid_height;
    }
})";
}  // namespace

//
// #############################################################################
//

GridObjectManager::GridObjectManager(const size_t grid_width, const size_t grid_height)
    : width_(grid_width),
      height_(grid_height),
      shader_(vertex_shader_text, fragment_shader_text, geometry_shader_text) {}

//
// #############################################################################
//

void GridObjectManager::init() {
    shader_.init();

    gl_safe(glGenVertexArrays, 1, &vertex_array_object_);
    gl_safe(glBindVertexArray, vertex_array_object_);

    buffer_.init(glGetAttribLocation(shader_.get_program_id(), "world_position"));
    buffer_.add(engine::Line{Eigen::Vector2f::Zero(), Eigen::Vector2f{width_, height_}});

    gl_safe(glBindVertexArray, 0);

    screen_from_world_loc_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");
}

//
// #############################################################################
//

void GridObjectManager::render(const Eigen::Matrix3f& screen_from_world) {
    shader_.activate();

    Eigen::Vector2f start = (screen_from_world * Eigen::Vector3f{0, 0, 1.0}).head(2);
    Eigen::Vector2f end = (screen_from_world * Eigen::Vector3f{width_, height_, 1.0}).head(2);

    gl_safe(glBindVertexArray, vertex_array_object_);
    gl_safe(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());
    gl_safe(glDrawElements, GL_LINES, 2, GL_UNSIGNED_INT, nullptr);
    gl_safe(glBindVertexArray, 0);
}

//
// #############################################################################
//

// no-ops
void GridObjectManager::update(float) {}
void GridObjectManager::handle_mouse_event(const engine::MouseEvent&) {}
void GridObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
