#include "engine/renderer/line.hh"

namespace engine::renderer {
namespace {
static std::string vertex_shader_text = R"(
#version 330
layout (location = 0) in vec2 world_position;

void main()
{
    gl_Position = vec4(world_position.x, world_position.y, 0.0, 1.0);
}
)";

static std::string fragment_shader_text = R"(
#version 330
out vec4 fragment;

void main()
{
    fragment = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static std::string geometry_shader_text = R"(
#version 330
layout(triangles) in;
layout(triangle_strip, max_vertices = 10) out;

uniform mat3 screen_from_world;

vec4 to_screen(vec2 world)
{
    vec3 screen = screen_from_world * vec3(world.x, world.y, 1.0);
    return vec4(screen.x, screen.y, 0.0, 1.0);
}

void main()
{
    float thickness = 1.0;
    vec2 start = gl_in[0].gl_Position.xy;
    vec2 end = gl_in[1].gl_Position.xy;

    vec2 normal = thickness * normalize(vec2(-(end.y - start.y), end.x - start.x));

    // Draw the main section
    gl_Position = to_screen(start - normal);
    EmitVertex();
    gl_Position = to_screen(end - normal);
    EmitVertex();
    gl_Position = to_screen(start + normal);
    EmitVertex();
    gl_Position = to_screen(end + normal);
    EmitVertex();
    EndPrimitive();

    // Then the end cap (which connects to the next line)
    vec2 next = gl_in[2].gl_Position.xy;
    vec2 next_normal = thickness * normalize(vec2(-(next.y - end.y), next.x - end.x));

    gl_Position = to_screen(end - normal);
    EmitVertex();
    gl_Position = to_screen(end - next_normal);
    EmitVertex();
    gl_Position = to_screen(end);
    EmitVertex();
    EndPrimitive();

    gl_Position = to_screen(end + normal);
    EmitVertex();
    gl_Position = to_screen(end + next_normal);
    EmitVertex();
    gl_Position = to_screen(end);
    EmitVertex();
    EndPrimitive();
}
)";
}  // namespace

//
// #############################################################################
//

LineRenderer::LineRenderer() : shader_(vertex_shader_text, fragment_shader_text, geometry_shader_text) {}

//
// #############################################################################
//

void LineRenderer::init() {
    shader_.init();
    screen_from_world_loc_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");

    vao_.init();
    position_buffer_.init(GL_ARRAY_BUFFER, 0, vao_);
    element_buffer_.init(GL_ELEMENT_ARRAY_BUFFER, vao_);
}

//
// #############################################################################
//

void LineRenderer::draw(const Line& line, const Eigen::Matrix3f& screen_from_world) {
    shader_.activate();
    gl_check(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());

    set_position(line.segments);
    set_elements(line.segments.size());

    // Each step will generate a triangle, so render 3 times as many
    gl_check_with_vao(vao_, glDrawElements, GL_TRIANGLES, 3 * (line.segments.size()) - 1, GL_UNSIGNED_INT, 0);
}

//
// #############################################################################
//

void LineRenderer::set_position(const std::vector<Eigen::Vector2f>& traced) {
    auto batch = position_buffer_.batched_updater();

    size_t required_size = 2 * traced.size();
    if (batch.size() < required_size) {
        batch.resize(required_size);
    }

    for (size_t i = 0; i < traced.size(); ++i) {
        batch.element(i) = traced[i];
    }
}

//
// #############################################################################
//

void LineRenderer::set_elements(size_t num_segments) {
    auto batch = element_buffer_.batched_updater();

    size_t required_size = 3 * (num_segments - 1);
    if (batch.size() < required_size) {
        batch.resize(required_size);
    }

    size_t index = 0;

    for (size_t i = 0; i < num_segments - 1; ++i) {
        batch[index++] = i;
        batch[index++] = i + 1;
        batch[index++] = i + 2;
    }

    // Replace the final element so that we don't have to duplicate a point in the vertex buffer
    batch[index - 1] = num_segments - 1;
}
}  // namespace engine::renderer
