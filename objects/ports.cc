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
uniform vec2 object_position;

in vec2 port_offset;

void main()
{
    gl_Position = vec4(object_position + port_offset, 0.0, 1.0);
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

static std::string geometry_shader_text = R"(
#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 64) out;

uniform mat3 screen_from_world;
uniform float radius;

// We have to do the world->screen transform in the geometry shader since the whole shape needs to be transformed
vec4 to_screen(vec2 world_position)
{
    vec3 world = vec3(world_position, 1.0);
    vec3 screen = screen_from_world * world;
    return vec4(screen.xy, 0.0, 1.0);
}

void main() {
    int count = 25;
    float angle_inc = 2 * 3.1415926592 / count;

    // // top right
    // gl_Position = to_screen(gl_in[0].gl_Position.xy + vec2(half_dim.x, -half_dim.y));
    // EmitVertex();

    // // bottom left
    // gl_Position = to_screen(gl_in[0].gl_Position.xy + vec2(-half_dim.x, half_dim.y));
    // EmitVertex();

    // // top left
    // gl_Position = to_screen(gl_in[0].gl_Position.xy - half_dim);
    // EmitVertex();

    vec4 center = to_screen(gl_in[0].gl_Position.xy);

    // Safe, GLfloats can represent small integers exactly
    float angle = 0;
    for (int i = 0; i <= count; i++, angle += angle_inc) {
        gl_Position = to_screen(gl_in[0].gl_Position.xy + radius * vec2(cos(angle), -sin(angle)));
        EmitVertex();

        gl_Position = center;
        EmitVertex();
    }

    EndPrimitive();
})";

static constexpr float kPortRadius = 1.5;  // in px
}  // namespace

//
// #############################################################################
//

PortsObject PortsObject::from_block(const BlockObject& parent) {
    const float width = parent.config.px_dim.x();
    const float height = parent.config.px_dim.y();

    // We'll only use this much of the height for putting ports
    const float effective_height = 0.9 * height;
    const float top_spacing = (height - effective_height) / 2.0;

    const size_t inputs = parent.config.inputs;
    const size_t outputs = parent.config.outputs;

    if (inputs > effective_height || outputs > effective_height) {
        throw std::runtime_error("Too many ports for the given object!");
    }

    // Pixels between each port
    const float input_spacing = effective_height / (inputs + 1);
    const float output_spacing = effective_height / (outputs + 1);

    // Create a bit of overlap between bock and port
    const float radius_offset = 0.1 * kPortRadius;

    std::vector<Eigen::Vector2f> offsets;
    for (size_t i = 0; i < inputs; ++i)
        offsets.push_back(Eigen::Vector2f{-radius_offset, (i + 1) * input_spacing + top_spacing});
    for (size_t i = 0; i < outputs; ++i)
        offsets.push_back(Eigen::Vector2f{width + radius_offset, (i + 1) * output_spacing + top_spacing});

    return PortsObject{{}, {}, std::move(offsets), parent};
}

//
// #############################################################################
//

PortsObjectManager::PortsObjectManager()
    : engine::AbstractSingleShaderObjectManager(vertex_shader_text, fragment_shader_text, geometry_shader_text),
      pool_(std::make_unique<engine::ListObjectPool<PortsObject>>()) {}

//
// #############################################################################
//

void PortsObjectManager::init_with_vao() {
    buffer_.init(glGetAttribLocation(get_shader().get_program_id(), "port_offset"));
    object_position_loc_ = glGetUniformLocation(get_shader().get_program_id(), "object_position");

    get_shader().activate();
    gl_safe(glUniform1f, glGetUniformLocation(get_shader().get_program_id(), "radius"), kPortRadius);
}

//
// #############################################################################
//

void PortsObjectManager::render_with_vao() {
    if (pool_->empty()) return;

    for (auto object : pool_->iterate()) {
        Eigen::Vector2f object_position = object->parent_block.top_left();
        gl_safe(glUniform2f, object_position_loc_, object_position.x(), object_position.y());

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
        buffer_.add(engine::Point{offset});
    }
}

//
// #############################################################################
//

void PortsObjectManager::despawn_object(const engine::ObjectId& id) { pool_->remove(id); }

//
// #############################################################################
//

void PortsObjectManager::handle_mouse_event(const engine::MouseEvent& event) {
    if (event.right || !event.clicked) {
        return;
    }

    if (pool_->empty()) return;

    // TODO Iterating backwards so we select the most recently added object easier
    for (auto object : pool_->iterate()) {
        for (size_t offset = 0; offset < object->offsets.size(); ++offset) {
            const Eigen::Vector2f center = object->parent_block.top_left() + object->offsets[offset];
            if ((center - event.mouse_position).squaredNorm() <= kPortRadius * kPortRadius) {
                std::cout << "Clicked!\n";
            }
        }
    }
}

// no-ops
void PortsObjectManager::update(float) {}
void PortsObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
