#include "objects/blocks.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"
#include "yaml-cpp/yaml.h"

namespace objects {
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
    fragment = texture(sampler, uv);
}
)";
}  // namespace

//
// #############################################################################
//

Config::Config(const std::filesystem::path& path) {
    const auto root = YAML::LoadFile(path);
    texture_path = root["texture_path"].as<std::string>();

    for (const auto& block : root["blocks"]) {
        BlockConfig block_config;
        block_config.name = block["name"].as<std::string>();
        block_config.description = block["description"].as<std::string>();
        block_config.px_start.x() = block["px_start"][0].as<int>();
        block_config.px_start.y() = block["px_start"][1].as<int>();
        block_config.px_dim.x() = block["px_dim"][0].as<size_t>();
        block_config.px_dim.y() = block["px_dim"][1].as<size_t>();
        block_config.inputs = block["inputs"].as<size_t>();
        block_config.outputs = block["outputs"].as<size_t>();

        blocks.emplace_back(block_config);
    }
}

//
// #############################################################################
//

BlockObjectManager::BlockObjectManager(const std::filesystem::path& config_path)
    : config_(config_path),
      shader_(vertex_shader_text, fragment_shader_text),
      texture_(config_.texture_path),
      pool_(std::make_unique<engine::ListObjectPool<BlockObject>>()) {}

//
// #############################################################################
//

void BlockObjectManager::spawn_object(BlockObject object_) {
    const size_t vertices_size = vertices_.size();

    auto texture_id = 0;

    auto [id, object] = pool_->add(std::move(object_));
    object.id = id;

    // Assume ordering will be top_left, top_right, bottom_left, bottom_right
    vertices_.resize(vertices_size + 4);

    if (texture_id == 0) {
        // top left
        vertices_[vertices_size + 0].u = 0.f;
        vertices_[vertices_size + 0].v = 0.f;

        // top right
        vertices_[vertices_size + 1].u = 1.f;
        vertices_[vertices_size + 1].v = 0.f;

        // bottom left
        vertices_[vertices_size + 2].u = 0.f;
        vertices_[vertices_size + 2].v = 1. / 3.;

        // bottom right
        vertices_[vertices_size + 3].u = 1.f;
        vertices_[vertices_size + 3].v = 1. / 3.;
    } else {
        // top left
        vertices_[vertices_size + 0].u = 0.f;
        vertices_[vertices_size + 0].v = 1. / 3.;

        // top right
        vertices_[vertices_size + 1].u = 1.f;
        vertices_[vertices_size + 1].v = 1. / 3.;

        // bottom left
        vertices_[vertices_size + 2].u = 0.f;
        vertices_[vertices_size + 2].v = 1.f;

        // bottom right
        vertices_[vertices_size + 3].u = 1.f;
        vertices_[vertices_size + 3].v = 1.f;
    }

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

void BlockObjectManager::despawn_object(const engine::ObjectId& id) { pool_->remove(id); }

//
// #############################################################################
//

void BlockObjectManager::init() {
    // important to initialize at the start
    shader_.init();
    texture_.init();

    bind_vertex_data();
}

//
// #############################################################################
//

void BlockObjectManager::render(const Eigen::Matrix3f& screen_from_world) {
    if (pool_->empty()) return;

    shader_.activate();
    texture_.activate();

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

void BlockObjectManager::handle_mouse_event(const engine::MouseEvent& event) {
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

void BlockObjectManager::handle_keyboard_event(const engine::KeyboardEvent& event) {
    if (!event.space) {
        return;
    }
    if (event.pressed()) {
        return;
    }

    BlockObject object;
    object.top_left = {100, 200};
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
}  // namespace objects
