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
    const size_t vertices_size = vertices_.size();

    auto [id, object] = pool_->add(std::move(object_));
    object.id = id;

    vertices_.resize(vertices_size + 2);
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

    bind_vertex_data();

    gl_safe(glBindVertexArray, 0);
}

//
// #############################################################################
//

void BlockObjectManager::render(const Eigen::Matrix3f& screen_from_world) {
    if (pool_->empty()) return;

    shader_.activate();

    size_t index = 0;
    for (auto object : pool_->iterate()) {
        auto& top_left_vertex = vertices_.at(index++);
        auto& bottom_right_vertex = vertices_.at(index++);

        const Eigen::Vector2f top_left = object->get_top_left();
        const Eigen::Vector2f bottom_right = object->get_bottom_right();
        top_left_vertex.pos[0] = 100;            // top_left.x();
        top_left_vertex.pos[1] = 200;            // top_left.y();
        bottom_right_vertex.pos[0] = 100 + 129;  // bottom_right.x();
        bottom_right_vertex.pos[1] = 200 + 64;   // bottom_right.y();
    }

    std::cout << vertices_.at(0).pos[0] << ", " << vertices_.at(0).pos[1] << "\n";

    gl_safe(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());
    gl_safe(glBindVertexArray, vertex_array_index_);
    gl_safe(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
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
    if (!event.clicked) return;

    if (selected_) {
        selected_->top_left += event.delta_position;
    } else {
        selected_ = select(event.mouse_position);
    }

    std::cout << "selected: " << (long)selected_ << "\n";
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

    std::cout << "Spawning new object!\n";
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
               position.y() < top_left.y();
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
    if (vertex_buffer_index_ < 0) {
        glDeleteBuffers(1, &vertex_buffer_index_);
    }
    if (vertex_array_index_ < 0) {
        glDeleteVertexArrays(1, &vertex_array_index_);
    }

    int world_position_loc = glGetAttribLocation(shader_.get_program_id(), "world_position");
    int vertex_uv_loc = glGetAttribLocation(shader_.get_program_id(), "vertex_uv");
    screen_from_world_loc_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");

    gl_safe(glGenBuffers, 1, &vertex_buffer_index_);
    gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_index_);
    gl_safe(glBufferData, GL_ARRAY_BUFFER, sizeof(Vertex) * vertices_.size(), vertices_.data(), GL_DYNAMIC_DRAW);

    gl_safe(glGenVertexArrays, 1, &vertex_array_index_);
    gl_safe(glBindVertexArray, vertex_array_index_);
    gl_safe(glEnableVertexAttribArray, world_position_loc);
    gl_safe(glVertexAttribPointer, world_position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*)offsetof(Vertex, pos));
    gl_safe(glEnableVertexAttribArray, vertex_uv_loc);
    gl_safe(glVertexAttribPointer, vertex_uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
}
}  // namespace engine
