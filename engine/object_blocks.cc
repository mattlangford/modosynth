#include "engine/object_blocks.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"

namespace engine {
namespace {
static std::string vertex_shader_text = R"(
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

static std::string fragment_shader_text = R"(
#version 330
in vec2 uv;
out vec4 fragment;
uniform sampler2D sampler;
void main()
{
    fragment = vec4(uv.x, uv.y, 0.0, 1.0); //texture(sampler, uv);
}
)";
}  // namespace

//
// #############################################################################
//

BlockObjectManager::BlockObjectManager()
    : shader_(vertex_shader_text, fragment_shader_text), pool_(std::make_unique<ListObjectPool<BlockObject>>()) {}

//
// #############################################################################
//

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

void BlockObjectManager::init() {
    // important to initialize at the start
    shader_.init();

    float x = 100;
    float y = 300;
    float h = 64;
    float w = 128;

    vertices_.emplace_back(Vertex{{x + w, y}, {1.0, 1.0}});      // top right
    vertices_.emplace_back(Vertex{{x + w, y + h}, {1.0, 0.0}});  // bottom right
    vertices_.emplace_back(Vertex{{x, y}, {0.0, 1.0}});          // top left
    vertices_.emplace_back(Vertex{{x, y + h}, {0.0, 0.0}});      // bottom left

    int world_position_loc = glGetAttribLocation(shader_.get_program_id(), "world_position");
    int vertex_uv_loc = glGetAttribLocation(shader_.get_program_id(), "vertex_uv");
    int screen_from_world_loc_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");

    unsigned int vertex_buffer;
    gl_safe(glGenBuffers, 1, &vertex_buffer);
    gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
    gl_safe(glBufferData, GL_ARRAY_BUFFER, sizeof(Vertex) * vertices_.size(), vertices_.data(), GL_STATIC_DRAW);

    gl_safe(glGenVertexArrays, 1, &vertex_array_index_);
    gl_safe(glBindVertexArray, vertex_array_index_);
    gl_safe(glEnableVertexAttribArray, world_position_loc);
    gl_safe(glVertexAttribPointer, world_position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*)offsetof(Vertex, pos));
    gl_safe(glEnableVertexAttribArray, vertex_uv_loc);
    gl_safe(glVertexAttribPointer, vertex_uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    gl_safe(glBindVertexArray, 0);
}

//
// #############################################################################
//

void BlockObjectManager::render(const Eigen::Matrix3f& screen_from_world) {
    shader_.activate();

    gl_safe(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());
    gl_safe(glBindVertexArray, vertex_array_index_);
    gl_safe(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
}

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

void BlockObjectManager::handle_keyboard_event(const KeyboardEvent& event) {
     // no-op for blocks
}
}  // namespace engine
