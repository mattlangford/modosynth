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

// struct Ports {
//     ObjectId pool_id;
//
//     size_t buffer_id;
//
//     // The offsets from the parent object to each port represented by this object
//     std::vector<Eigen::Vector2f> input_offsets;
//     std::vector<Eigen::Vector2f> output_offsets;
//
//     // The parent object this belongs to
//     const BlockObject& parent_object;
//
// };

PortsObject PortsObject::from_block(const BlockObject& parent) { throw; }

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
    auto [id, object] = pool_->add(std::move(object_));
    object.pool_id = id;

    bind_vao();
    for (const Eigen::Vector2f& offset : object.offsets) {
        buffer_.add(engine::Point{offset});
    }
}

//
// #############################################################################
//

void BlockObjectManager::despawn_object(const engine::ObjectId& id) { pool_->remove(id); }

// no-ops
void PortsObjectManager::update(float) {}
void PortsObjectManager::handle_mouse_event(const engine::MouseEvent&) {}
void PortsObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
