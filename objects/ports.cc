#include "objects/ports.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"
#include "engine/utils.hh"
#include "objects/blocks.hh"

namespace objects {
namespace {
static std::string vertex_shader_text = R"(
#version 330
uniform vec3 object_position;

in vec2 port_offset;

void main()
{
    gl_Position = vec4(object_position.xy + port_offset, object_position.z, 1.0);
}
)";

static std::string fragment_shader_text = R"(
#version 330
uniform sampler2D sampler;
in vec2 uv;
out vec4 fragment;

void main()
{
    fragment = texture(sampler, uv);
}
)";

static std::string geometry_shader_text = R"(
#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

uniform mat3 screen_from_world;
uniform vec2 half_dim;

out vec2 uv;

// We have to do the world->screen transform in the geometry shader since the whole shape needs to be transformed
vec4 to_screen(vec2 world_position, float z)
{
    vec3 world = vec3(world_position, 1.0);
    vec3 screen = screen_from_world * world;
    return vec4(screen.xy, z, 1.0);
}

void main() {
    float z = gl_in[0].gl_Position.z;

    // bottom right
    uv = vec2(1, 1);
    gl_Position = to_screen(gl_in[0].gl_Position.xy + half_dim, z);
    EmitVertex();

    // top right
    uv = vec2(1, 0);
    gl_Position = to_screen(gl_in[0].gl_Position.xy + vec2(half_dim.x, -half_dim.y), z);
    EmitVertex();

    // bottom left
    uv = vec2(0, 1);
    gl_Position = to_screen(gl_in[0].gl_Position.xy + vec2(-half_dim.x, half_dim.y), z);
    EmitVertex();

    // top left
    uv = vec2(0, 0);
    gl_Position = to_screen(gl_in[0].gl_Position.xy - half_dim, z);
    EmitVertex();

    EndPrimitive();
})";
}  // namespace

//
// #############################################################################
//

PortsObject PortsObject::from_block(const BlockObject& parent) {
    const float width = parent.config.px_dim.x();
    const float height = parent.config.px_dim.y();

    // We'll only use this much of the height for putting ports
    const size_t effective_height = height;
    const size_t top_spacing = height - effective_height;

    const size_t inputs = parent.config.inputs;
    const size_t outputs = parent.config.outputs;

    if (inputs > effective_height || outputs > effective_height) {
        throw std::runtime_error("Too many ports for the given object!");
    }

    const size_t input_spacing = effective_height / (inputs + 1);
    const size_t output_spacing = effective_height / (outputs + 1);

    std::vector<Eigen::Vector2f> offsets;
    for (size_t i = 0; i < inputs; ++i)
        offsets.push_back(Eigen::Vector2f{-kHalfPortWidth, (i + 1) * input_spacing + top_spacing});
    for (size_t i = 0; i < outputs; ++i)
        offsets.push_back(Eigen::Vector2f{width + kHalfPortWidth, (i + 1) * output_spacing + top_spacing});

    return PortsObject{{}, {}, std::move(offsets), parent};
}

//
// #############################################################################
//

PortsObjectManager::PortsObjectManager()
    : engine::AbstractSingleShaderObjectManager(vertex_shader_text, fragment_shader_text, geometry_shader_text),
      pool_(std::make_unique<engine::ListObjectPool<PortsObject>>()),
      texture_("objects/ports.bmp") {}

//
// #############################################################################
//

void PortsObjectManager::init_with_vao() {
    texture_.init();

    buffer_.init(glGetAttribLocation(get_shader().get_program_id(), "port_offset"));
    object_position_loc_ = glGetUniformLocation(get_shader().get_program_id(), "object_position");

    get_shader().activate();
    gl_safe(glUniform2f, glGetUniformLocation(get_shader().get_program_id(), "half_dim"), kHalfPortWidth,
            kHalfPortHeight);
}

//
// #############################################################################
//

void PortsObjectManager::render_with_vao() {
    if (pool_->empty()) return;
    texture_.activate();

    for (auto object : pool_->iterate()) {
        Eigen::Vector2f object_position = object->parent_block.top_left();
        gl_safe(glUniform3f, object_position_loc_, object_position.x(), object_position.y(), object->parent_block.z);

        // The pointer here doesn't actually point to anything. It's an offset into the current element array but needs
        // to be a pointer for some reason (and have the right size).
        const void* offset_ptr = reinterpret_cast<void*>(sizeof(unsigned int) * object->buffer_id);
        gl_safe(glDrawElements, GL_POINTS, object->offsets.size(), GL_UNSIGNED_INT, offset_ptr);
    }
}

//
// #############################################################################
//

void PortsObjectManager::spawn_object(PortsObject object_) {
    if (object_.offsets.empty()) throw std::runtime_error("Trying to spawn PortsObject with no ports!");

    auto [id, object] = pool_->add(std::move(object_));
    object.pool_id = id;
    object.buffer_id = buffer_.get_index_count();

    bind_vao();
    for (const Eigen::Vector2f& offset : object.offsets) {
        buffer_.add(engine::Point2Df{offset});
    }
}

//
// #############################################################################
//

const engine::AbstractObjectPool<PortsObject>& PortsObjectManager::pool() const { return *pool_; }

//
// #############################################################################
//

// no-ops
void PortsObjectManager::update(float) {}
void PortsObjectManager::handle_mouse_event(const engine::MouseEvent&) {}
void PortsObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
