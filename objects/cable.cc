#include "objects/cable.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"
#include "engine/utils.hh"
#include "objects/blocks.hh"
#include "objects/ports.hh"

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
    fragment = vec4(0.8, 0.8, 0.8, 1.0);
}
)";
}  // namespace

//
// #############################################################################
//

CableObjectManager::CableObjectManager(std::shared_ptr<PortsObjectManager> ports_manager)
    : engine::AbstractSingleShaderObjectManager(vertex_shader_text, fragment_shader_text),
      ports_manager_(std::move(ports_manager)),
      pool_(std::make_unique<engine::ListObjectPool<CableObject>>()) {}

//
// #############################################################################
//

void CableObjectManager::init_with_vao() {
    buffer_.init(glGetAttribLocation(get_shader().get_program_id(), "world_position"));
}

//
// #############################################################################
//

void CableObjectManager::render_with_vao() {
    if (building_) {
        auto local_buffer = buffer_;
        render_from_buffer(local_buffer);
    } else {
        render_from_buffer(buffer_);
    }
}

//
// #############################################################################
//

void CableObjectManager::render_from_buffer(engine::Buffer2Df& buffer) const {
    const auto objects = pool_->iterate();

    size_t index = 0;
    for (const auto* object : objects) {
        engine::Line2Df line;
        line.start = object->parent_start.bottom_left() + object->offset_start;
        line.end = object->parent_end.bottom_left() + object->offset_end;
        buffer.update_batch(line, index);
    }

    size_t building_id = -1;
    if (building_) {
        engine::Line2Df line;
        line.start = building_->parent_start.bottom_left() + building_->offset_start;
        line.end = building_->end;

        building_id = buffer.get_index_count();

        // add will do the same thing "finish batch" does in terms of copying
        buffer.add(line);
    } else {
        buffer.finish_batch();
    }

    for (const auto* object : objects) {
        // 2 triangles per object
        gl_check(glDrawElements, GL_LINES, 2, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * object->buffer_id));
    }

    if (building_) {
        gl_check(glDrawElements, GL_LINES, 2, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * building_id));
    }
}

//
// #############################################################################
//

const PortsObject* CableObjectManager::get_active_port(const Eigen::Vector2f& position, Eigen::Vector2f& offset) const {
    const Eigen::Vector2f half_dim{kHalfPortWidth, kHalfPortHeight};

    // TODO Iterating backwards so we select the most recently added object easier
    const PortsObject* selected_object = nullptr;
    for (const auto* object : ports_manager_->pool().iterate()) {
        for (size_t this_offset = 0; this_offset < object->offsets.size(); ++this_offset) {
            const Eigen::Vector2f center = object->parent_block.bottom_left() + object->offsets[this_offset];
            const Eigen::Vector2f bottom_left = center - half_dim;
            const Eigen::Vector2f bottom_right = center + half_dim;

            if (engine::is_in_rectangle(position, bottom_left, bottom_right)) {
                offset = object->offsets[this_offset];
                selected_object = object;
            }
        }
    }
    return selected_object;
}

//
// #############################################################################
//

void CableObjectManager::handle_mouse_event(const engine::MouseEvent& event) {
    if (event.right || (!event.clicked && !event.was_clicked)) {
        building_.reset();
    } else if (!event.any_modifiers() && event.pressed()) {
        Eigen::Vector2f offset;
        if (auto ptr = get_active_port(event.mouse_position, offset)) {
            building_.emplace(BuildingCableObject{ptr->parent_block, offset, event.mouse_position});
        }
    } else if (building_ && event.released()) {
        // Check for ports, if we're on one finalize current building object
        Eigen::Vector2f offset;
        if (auto ptr = get_active_port(event.mouse_position, offset)) {
            spawn_object(
                CableObject{{}, {}, building_->parent_start, building_->offset_start, ptr->parent_block, offset});
        }

        // Either it finished or wasn't connected, either way we reset
        building_.reset();
    } else if (building_ && event.held()) {
        building_->end = event.mouse_position;
    }
}

//
// #############################################################################
//

void CableObjectManager::spawn_object(CableObject object_) {
    auto [id, object] = pool_->add(std::move(object_));
    object.object_id = id;
    object.buffer_id = buffer_.get_index_count();

    bind_vao();
    buffer_.add(engine::Line2Df{});
}

//
// #############################################################################
//

// no-ops
void CableObjectManager::update(float) {}
void CableObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
