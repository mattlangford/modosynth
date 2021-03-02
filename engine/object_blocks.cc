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
    fragment = vec4(1.0, 1.0, 0.0, 1.0); //texture(sampler, uv);
}
)";
}  // namespace

//
// #############################################################################
//

BlockObjectManager::BlockObjectManager()
    : shader_(vertex_shader_text, fragment_shader_text), texture_manager_("/Users/mlangford/Documents/code/modosynth/engine/texture.yml"), pool_(std::make_unique<ListObjectPool<BlockObject>>()) {}

//
// #############################################################################
//

void BlockObjectManager::spawn_object(BlockObject object_) {
    const size_t vertices_size = vertices_.size();

    auto [id, object] = pool_->add(std::move(object_));
    object.id = id;

    // Assume ordering will be top_left, top_right, bottom_left, bottom_right
    vertices_.resize(vertices_size + 4);

    vertices_[vertices_size + 0].u = 0.f;
    vertices_[vertices_size + 0].v = 1.f;

    vertices_[vertices_size + 1].u = 1.f;
    vertices_[vertices_size + 1].v = 1.f;

    vertices_[vertices_size + 2].u = 0.f;
    vertices_[vertices_size + 2].v = 0.f;

    vertices_[vertices_size + 3].u = 1.f;
    vertices_[vertices_size + 3].v = 0.f;

    indices_.emplace_back(vertices_size + 0);  // top left
    indices_.emplace_back(vertices_size + 1);  // top right
    indices_.emplace_back(vertices_size + 2);  // bottom left

    indices_.emplace_back(vertices_size + 3);  // bottom right
    indices_.emplace_back(vertices_size + 2);  // bottom left
    indices_.emplace_back(vertices_size + 1);  // top right

    bind_vertex_data();
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

    BlockObject object;
    object.top_left = {100, 200};
    object.dims = {128, 64};
    spawn_object(object);

    bind_vertex_data();
}

//
// #############################################################################
//

void BlockObjectManager::render(const Eigen::Matrix3f& screen_from_world) {
    if (pool_->empty()) return;

    shader_.activate();

    size_t index = 0;
    for (auto object : pool_->iterate()) {
        const Eigen::Vector2f top_left = object->get_top_left();
        const Eigen::Vector2f bottom_right = object->get_bottom_right();

        auto& top_left_vertex = vertices_.at(index++);
        auto& top_right_vertex = vertices_.at(index++);
        auto& bottom_left_vertex = vertices_.at(index++);
        auto& bottom_right_vertex = vertices_.at(index++);

        top_left_vertex.x = top_left.x();
        top_left_vertex.y = top_left.y();
        top_right_vertex.x = bottom_right.x();
        top_right_vertex.y = top_left.y();
        bottom_left_vertex.x = top_left.x();
        bottom_left_vertex.y = bottom_right.y();
        bottom_right_vertex.x = bottom_right.x();
        bottom_right_vertex.y = bottom_right.y();
    }

    gl_safe(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());
    gl_safe(glBindVertexArray, vertex_array_index_);
    gl_safe(glBufferData, GL_ARRAY_BUFFER, sizeof(Vertex) * vertices_.size(), vertices_.data(), GL_STREAM_DRAW);
    gl_safe(glDrawElements, GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, (void*)0);
}

//
// #############################################################################
//

void BlockObjectManager::update(float /* dt */) {
    // no-op for blocks
}

//
// #############################################################################
//

void BlockObjectManager::handle_mouse_event(const MouseEvent& event) {
    if (event.right || !event.clicked) {
        selected_ = nullptr;
        return;
    }

    if (selected_) {
        selected_->top_left += event.delta_position;
    } else {
        selected_ = select(event.mouse_position);
    }
}

//
// #############################################################################
//

void BlockObjectManager::handle_keyboard_event(const KeyboardEvent& event) {
    if (!event.space) {
        return;
    }
    if (event.pressed()) {
        return;
    }

    BlockObject object;
    object.top_left = {100, 200};
    object.dims = {128, 64};
    spawn_object(object);
}

//
// #############################################################################
//

BlockObject* BlockObjectManager::select(const Eigen::Vector2f& position) const {
    if (pool_->empty()) return nullptr;

    auto is_in_object = [&position](const BlockObject& object) {
        Eigen::Vector2f top_left = object.get_top_left();
        Eigen::Vector2f bottom_right = object.get_bottom_right();

        return position.x() >= top_left.x() && position.x() < bottom_right.x() && position.y() >= top_left.y() &&
               position.y() < bottom_right.y();
    };

    // Iterating backwards like this will ensure we always select the newest object
    for (auto object : pool_->iterate()) {
        if (is_in_object(*object)) {
            return object;
        }
    }
    return nullptr;
}

//
// #############################################################################
//

void BlockObjectManager::bind_vertex_data() {
    if (vertex_buffer_index_ > 0) {
        glDeleteBuffers(1, &vertex_buffer_index_);
    }
    if (element_buffer_index_ > 0) {
        glDeleteBuffers(1, &element_buffer_index_);
    }
    if (vertex_array_index_ > 0) {
        glDeleteVertexArrays(1, &vertex_array_index_);
    }

    int world_position_loc = glGetAttribLocation(shader_.get_program_id(), "world_position");
    int vertex_uv_loc = glGetAttribLocation(shader_.get_program_id(), "vertex_uv");
    screen_from_world_loc_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");

    gl_safe(glGenBuffers, 1, &vertex_buffer_index_);
    gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_index_);
    gl_safe(glBufferData, GL_ARRAY_BUFFER, sizeof(Vertex) * vertices_.size(), vertices_.data(), GL_STREAM_DRAW);

    gl_safe(glGenVertexArrays, 1, &vertex_array_index_);
    gl_safe(glBindVertexArray, vertex_array_index_);

    gl_safe(glGenBuffers, 1, &element_buffer_index_);
    gl_safe(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, element_buffer_index_);
    gl_safe(glBufferData, GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices_.size(), indices_.data(),
            GL_STATIC_DRAW);

    gl_safe(glEnableVertexAttribArray, world_position_loc);
    gl_safe(glVertexAttribPointer, world_position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*)offsetof(Vertex, x));
    gl_safe(glEnableVertexAttribArray, vertex_uv_loc);
    gl_safe(glVertexAttribPointer, vertex_uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    // Unbind the attribute array
    gl_safe(glBindVertexArray, 0);
}
}  // namespace engine
