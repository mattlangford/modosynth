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
    float thickness = 0.25;
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

Eigen::Vector2f CatenaryObject::start() const { return offset_start + (parent_start ? parent_start->bottom_left() : Eigen::Vector2f::Zero()); }
Eigen::Vector2f CatenaryObject::end() const { return offset_end + (parent_end ? parent_end->bottom_left() : Eigen::Vector2f::Zero()); }

//
// #############################################################################
//

CableObjectManager::CableObjectManager(std::shared_ptr<PortsObjectManager> ports_manager)
    : engine::AbstractSingleShaderObjectManager(vertex_shader_text, fragment_shader_text, geometry_shader_text),
      ports_manager_(std::move(ports_manager)),
      pool_(std::make_unique<engine::ListObjectPool<CatenaryObject>>()) {}

//
// #############################################################################
//

void CableObjectManager::init_with_vao() {
    vao_.init();
    vbo_.init(GL_ARRAY_BUFFER, 0, vao_);
    ebo_.init(GL_ELEMENT_ARRAY_BUFFER, vao_);
}

//
// #############################################################################
//

void CableObjectManager::render_with_vao() {
    if (ebo_.size() == 0) return;
    for (const auto* object : pool_->iterate()){
        const void* indices = reinterpret_cast<void*>(sizeof(unsigned int) * object->element_index);

        // Each step will generate a triangle, so render 3 times as many
        gl_check_with_vao(vao_, glDrawElements, GL_TRIANGLES, 3 * CatenaryObject::kNumSteps, GL_UNSIGNED_INT, indices);
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
//

void CableObjectManager::update(float) {
    auto vertices = vbo_.batched_updater();

    for (auto* object : pool_->iterate()) {
        size_t index = object->vertex_index;
        for (Eigen::Vector2f point : object->calculate_points()) {
            vertices.element(index++) = point;
        }
    }

}

//
// #############################################################################
//

void CableObjectManager::handle_mouse_event(const engine::MouseEvent& event) {
    if (event.any_modifiers() || event.right) return;

    if (event.pressed()) {
        Eigen::Vector2f offset;
        if (auto ptr = get_active_port(event.mouse_position, offset)) {
            CatenaryObject object;
            object.parent_start = &ptr->parent_block;
            object.offset_start = offset;
            object.offset_end = event.mouse_position;
            spawn_object(std::move(object));
        }
    } else if (selected_ && event.released()) {
        // Check for ports, if we're on one finalize current building object
        Eigen::Vector2f offset;
        if (auto ptr = get_active_port(event.mouse_position, offset)) {
            selected_->parent_end = &ptr->parent_block;
            selected_->offset_end = offset;
        }
        selected_ = nullptr;
    } else if (selected_ && event.held()) {
        selected_->offset_end = event.mouse_position;
    }
}

//
// #############################################################################
//

void CableObjectManager::spawn_object(CatenaryObject object_) {
    auto [id, object] = pool_->add(std::move(object_));
    object.object_id = id;
    object.vertex_index = vbo_.elements();
    object.element_index = ebo_.elements();

    vbo_.resize(vbo_.size() + 2 * CatenaryObject::kNumSteps);

    // Now add in the elements needed to render this object
    auto elements = ebo_.batched_updater();
    for (size_t i = 0; i < CatenaryObject::kNumSteps - 2; ++i) {
        elements.push_back(object.vertex_index + i);
        elements.push_back(object.vertex_index + i + 1);
        elements.push_back(object.vertex_index + i + 2);
    }
    // Add the last element in, this one needs to be special so we don't duplicate points in the vbo
    elements.push_back(object.vertex_index + CatenaryObject::kNumSteps - 2);
    elements.push_back(object.vertex_index + CatenaryObject::kNumSteps - 1);
    elements.push_back(object.vertex_index + CatenaryObject::kNumSteps - 1);

    selected_ = &object;
}

//
// #############################################################################
//

void CableObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
