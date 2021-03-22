#include "engine/renderer/grid.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"

namespace engine::renderer {
namespace {
static std::string vertex_shader_text = R"(
#version 330
layout(location = 0) in vec2 world_position;
uniform mat3 screen_from_world;

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

Grid::Grid(const size_t grid_width, const size_t grid_height)
    : width_(grid_width),
      height_(grid_height),
      shader_(vertex_shader_text, fragment_shader_text, geometry_shader_text) {}

//
// #############################################################################
//

void Grid::init() {
    shader_.init();
    vao_.init();
    buffer_.init(GL_ARRAY_BUFFER, 0, vao_);
    auto buffer = buffer_.batched_updater();
    buffer.push_back(0);        // x0
    buffer.push_back(0);        // y0
    buffer.push_back(width_);   // x1
    buffer.push_back(height_);  // y1

    screen_from_world_location_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");
    engine::throw_on_gl_error("glGetUniformLocation");
}

//
// #############################################################################
//

void Grid::render(const Eigen::Matrix3f& screen_from_world) {
    shader_.activate();
    gl_check(glUniformMatrix3fv, screen_from_world_location_, 1, GL_FALSE, screen_from_world.data());
    gl_check_with_vao(vao_, glDrawArrays, GL_LINES, 0, 2);
}

//
// #############################################################################
//

// no-ops
void Grid::update(float) {}
void Grid::handle_mouse_event(const engine::MouseEvent&) {}
void Grid::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace engine::renderer
