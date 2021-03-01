#include "engine/object_blocks.hh"

namespace engine {

std::string vertex_shader_text = R"(
#version 330
uniform mat3 screen_from_world;
in vec2 world_position;
in vec2 vertex_uv;

out vec2 uv;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, 0.0, 1.0);
    uv = vertex_uv;
}
)";

std::string fragment_shader_text = R"(
#version 330
in vec2 uv;
out vec4 fragment;
uniform sampler2D sampler;
void main()
{
    fragment = texture(sampler, uv);
}
)";

void BlockObjectManager::spawn_object(BlockObject object_) {
    auto [id, object] = pool_->add(std::move(object_));
    object.id = id;
}

//
// #############################################################################
//

void BlockObjectManager::despawn_object(const ObjectId& id) { pool_->remove(id); }

//
// #############################################################################
//

void BlockObjectManager::init() {}

//
// #############################################################################
//

void BlockObjectManager::render(const Eigen::Matrix3f& screen_from_world) {}

//
// #############################################################################
//

void BlockObjectManager::update(float dt) {
    // no-op for blocks
}

//
// #############################################################################
//

void BlockObjectManager::handle_mouse_event(const MouseEvent& event) {}

//
// #############################################################################
//

void BlockObjectManager::handle_keyboard_event(const KeyboardEvent& event) {}
}  // namespace engine
