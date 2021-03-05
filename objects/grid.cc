#include "objects/grid.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"

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
layout (line_strip, max_vertices = 512) out;

out float color;

void vertical_line(float x) {
    gl_Position = vec4(x, 1.0, 1.0, 1.0);
    EmitVertex();
    gl_Position = vec4(x, -1.0, 1.0, 1.0);
    EmitVertex();
    EndPrimitive();
}

void horizontal_line(float y) {
    gl_Position = vec4(1.0, y, 1.0, 1.0);
    EmitVertex();
    gl_Position = vec4(-1.0, y, 1.0, 1.0);
    EmitVertex();
    EndPrimitive();
}

float move_near_zero(float value, float step)
{
    float diff = value;
    while (diff >= step)
        diff -= step;
    while (diff < 0)
        diff += step;
    return diff;
}

void main() {
    float start_x = gl_in[0].gl_Position.x;
    float end_x = gl_in[1].gl_Position.x;
    float start_y = gl_in[0].gl_Position.y;
    float end_y = gl_in[1].gl_Position.y;

    float grid_width = end_x - start_x;
    // Note: this will be negative since Y is inverted
    float grid_height = abs(end_y - start_y);

    start_x = move_near_zero(start_x, grid_width);
    start_y = move_near_zero(start_y, grid_height);
    vertical_line(start_x);
    horizontal_line(start_y);

    float positive_x = start_x;
    float negative_x = start_x;
    while (positive_x <= 1.5)
    {
        positive_x += grid_width;
        negative_x -= grid_width;
        vertical_line(positive_x);
        vertical_line(negative_x);
    }

    float positive_y = start_y;
    float negative_y = start_y;
    while (positive_y <= 1.5)
    {
        positive_y += grid_height;
        negative_y -= grid_height;
        horizontal_line(positive_y);
        horizontal_line(negative_y);
    }

    return;
})";
}  // namespace

//
// #############################################################################
//

GridObjectManager::GridObjectManager(const size_t grid_width, const size_t grid_height)
    : engine::AbstractSingleShaderObjectManager(vertex_shader_text, fragment_shader_text, geometry_shader_text),
      width_(grid_width),
      height_(grid_height) {}

//
// #############################################################################
//

void GridObjectManager::init_with_vao() {
    buffer_.init(glGetAttribLocation(get_shader().get_program_id(), "world_position"));
    buffer_.add(engine::Line2Df{Eigen::Vector2f::Zero(), Eigen::Vector2f{width_, height_}});
}

//
// #############################################################################
//

void GridObjectManager::render_with_vao() { gl_safe(glDrawElements, GL_LINES, 2, GL_UNSIGNED_INT, nullptr); }

//
// #############################################################################
//

// no-ops
void GridObjectManager::update(float) {}
void GridObjectManager::handle_mouse_event(const engine::MouseEvent&) {}
void GridObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
