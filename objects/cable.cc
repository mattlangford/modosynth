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
layout(location = 0) in vec2 world_position;

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

Eigen::Vector2f CableObject::start() const { return parent_start.bottom_left() + offset_start; }
Eigen::Vector2f CableObject::end() const { return parent_end.bottom_left() + offset_end; }

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

void CableObjectManager::init_with_vao() { buffer_.init(GL_ARRAY_BUFFER, 0, 2, vao()); }

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

void CableObjectManager::render_from_buffer(engine::Buffer<float>& buffer) const {
    const auto objects = pool_->iterate();

    {
        auto points = buffer.batched_updater();
        size_t index = 0;
        for (const auto* object : objects) {
            const Eigen::Vector2f start = object->start();
            const Eigen::Vector2f end = object->end();
            points[index++] = start.x();
            points[index++] = start.y();
            points[index++] = end.x();
            points[index++] = end.y();
        }

        if (building_) {
            const Eigen::Vector2f start = building_->parent_start.bottom_left() + building_->offset_start;
            const Eigen::Vector2f end = building_->end;

            points.push_back(start.x());
            points.push_back(start.y());
            points.push_back(end.x());
            points.push_back(end.y());
        }
    }

    for (const auto* object : objects) {
        gl_check_with_vao(vao(), glDrawArrays, GL_LINES, object->element_index, 2);
    }

    if (building_) {
        gl_check_with_vao(vao(), glDrawArrays, GL_LINES, buffer_.elements(), 2);
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
    object.element_index = buffer_.elements();

    const Eigen::Vector2f start = object.start();
    const Eigen::Vector2f end = object.end();
    buffer_.push_back(start.x());
    buffer_.push_back(start.y());
    buffer_.push_back(end.x());
    buffer_.push_back(end.y());
}

//
// #############################################################################
//

// no-ops
void CableObjectManager::update(float) {}
void CableObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
