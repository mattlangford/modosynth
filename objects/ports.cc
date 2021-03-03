#include "objects/ports.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"
#include "objects/blocks.hh"

namespace objects {
namespace {
static std::string vertex_shader_text = R"(
#version 330
uniform mat3 screen_from_world;
uniform vec2 object_position;

in vec2 port_offset;

void main()
{
    vec2 world_position = object_position + port_offset;
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, 0.0, 1.0);
}
)";

static std::string fragment_shader_text = R"(
#version 330
out vec4 fragment;

void main()
{
    fragment = vec4(1.0, 0.6, 0.1, 1.0);
}
)";
}  // namespace

PortsObject PortsObject::from_block(const BlockObject& parent) {
    const float width = parent.config.px_dim.x();
    const float height = parent.config.px_dim.y();

    // We'll only use this much of the height for putting ports
    const float effective_height = 0.9 * height;

    const size_t inputs = parent.config.inputs;
    const size_t outputs = parent.config.outputs;

    if (inputs > effective_height || outputs > effective_height) {
        throw std::runtime_error("Too many ports for the given object!");
    }

    const float input_spacing = effective_height / inputs;
    const float output_spacing = effective_height / outputs;

    std::vector<Eigen::Vector2f> offsets;
    for (size_t i = 0; i < inputs; ++i) offsets.push_back(Eigen::Vector2f{0, i * input_spacing});
    for (size_t i = 0; i < outputs; ++i) offsets.push_back(Eigen::Vector2f{width, i * output_spacing});

    return PortsObject{{}, {}, std::move(offsets), parent};
}

//
// #############################################################################
//

PortsObjectManager::PortsObjectManager()
    : engine::AbstractSingleShaderObjectManager(vertex_shader_text, fragment_shader_text) {}

//
// #############################################################################
//

void PortsObjectManager::init_with_vao() {
    buffer_.init(glGetAttribLocation(get_shader().get_program_id(), "port_offset"));
    object_position_loc_ = glGetUniformLocation(get_shader().get_program_id(), "object_position");
}

//
// #############################################################################
//

void PortsObjectManager::render_with_vao() {
    if (pool_->empty()) return;

    for (auto object : pool_->iterate()) {
        Eigen::Vector2f object_position = object->parent_block.top_left();
        gl_safe(glUniform2f, object_position_loc_, object_position.x(), object_position.y());

        gl_safe(glDrawElements, GL_POINTS, object->offsets.size(), GL_UNSIGNED_INT, buffer_.offset(object->buffer_id));
    }
}

//
// #############################################################################
//

void PortsObjectManager::spawn_object(PortsObject object_) {
    if (object_.offsets.empty()) throw std::runtime_error("Trying to spawn PortsObject with no ports!");

    auto [id, object] = pool_->add(std::move(object_));
    tobject.pool_id = id;

    std::optional<size_t> starting_index;

    bind_vao();
    for (const Eigen::Vector2f& offset : object.offsets) {
        size_t index = buffer_.add(engine::Point{offset});
        if (!starting_index) starting_index = index;
    }

    // Since we checked for ports before, this is okay to do
    object.buffer_id = *starting_index;
}

//
// #############################################################################
//

void BlockObjectManager::despawn_object(const engine::ObjectId& id) { pool_->remove(id); }

//
// #############################################################################
//

// no-ops
void PortsObjectManager::update(float) {}
void PortsObjectManager::handle_mouse_event(const engine::MouseEvent&) {}
void PortsObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
