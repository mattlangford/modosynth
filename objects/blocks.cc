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

    auto [id, object] = pool_->add(std::move(object_));
    object.id = id;

    add_space_for_new_object();

    // Populate UV coordinates once, they don't change
    size_t index = vertices_size;
    assign_uv(object, index);

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
        assign_coords(*object, index);
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
        selected_->offset += event.delta_position;
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
    static size_t id = 0;
    object.offset = {100, 200};
    object.config_id = id++ % 2;
    spawn_object(object);
}

//
// #############################################################################
//

BlockObject* BlockObjectManager::select(const Eigen::Vector2f& position) const {
    if (pool_->empty()) return nullptr;

    auto is_in_object = [&position, this](const BlockObject& object) {
        Eigen::Vector2f top_left = object.offset;
        Eigen::Vector2f bottom_right = top_left + config_.blocks[object.config_id].px_dim.cast<float>();

        return position.x() >= top_left.x() && position.x() < bottom_right.x() && position.y() >= top_left.y() &&
               position.y() < bottom_right.y();
    };

    // TODO Iterating backwards so we select the most recently added object easier
    BlockObject* select_object;
    for (auto object : pool_->iterate()) {
        if (is_in_object(*object)) {
            select_object = object;
        }
    }
    return select_object;
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
            (void*)offsetof(Vertex, coords));
    gl_safe(glEnableVertexAttribArray, vertex_uv_loc);
    gl_safe(glVertexAttribPointer, vertex_uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    // Unbind the attribute array
    gl_safe(glBindVertexArray, 0);
}

//
// #############################################################################
//

void BlockObjectManager::assign_coords(const BlockObject& block, size_t& index) {
    auto& top_left_vertex = vertices_.at(index++);
    auto& top_right_vertex = vertices_.at(index++);
    auto& bottom_left_vertex = vertices_.at(index++);
    auto& bottom_right_vertex = vertices_.at(index++);

    const auto& config = config_.blocks[block.config_id];

    const Eigen::Vector2f top_left = block.offset;
    const Eigen::Vector2f bottom_right = top_left + config.px_dim.cast<float>();

    top_left_vertex.coords = top_left;
    top_right_vertex.coords = Eigen::Vector2f{bottom_right.x(), top_left.y()};
    bottom_left_vertex.coords = Eigen::Vector2f{top_left.x(), bottom_right.y()};
    bottom_right_vertex.coords = bottom_right;
}

//
// #############################################################################
//

void BlockObjectManager::assign_uv(const BlockObject& block, size_t& index) {
    auto& top_left_vertex = vertices_.at(index++);
    auto& top_right_vertex = vertices_.at(index++);
    auto& bottom_left_vertex = vertices_.at(index++);
    auto& bottom_right_vertex = vertices_.at(index++);

    const auto& config = config_.blocks[block.config_id];

    const Eigen::Vector2f texture_dim{texture_.bitmap().get_width(), texture_.bitmap().get_height()};
    const Eigen::Vector2f top_left = config.px_start.cast<float>().cwiseQuotient(texture_dim);
    const Eigen::Vector2f bottom_right = (config.px_start + config.px_dim).cast<float>().cwiseQuotient(texture_dim);

    top_left_vertex.uv = top_left;
    top_right_vertex.uv = Eigen::Vector2f{bottom_right.x(), top_left.y()};
    bottom_left_vertex.uv = Eigen::Vector2f{top_left.x(), bottom_right.y()};
    bottom_right_vertex.uv = bottom_right;
}

//
// #############################################################################
//

void BlockObjectManager::add_space_for_new_object() {
    enum Ordering : int8_t { kTopLeft = 0, kTopRight = 1, kBottomLeft = 2, kBottomRight = 3, kSize = 4 };
    size_t start = vertices_.size();

    // 4 coordinates per object
    vertices_.emplace_back(Vertex{});
    vertices_.emplace_back(Vertex{});
    vertices_.emplace_back(Vertex{});
    vertices_.emplace_back(Vertex{});

    // Upper triangle
    indices_.emplace_back(start + Ordering::kTopLeft);
    indices_.emplace_back(start + Ordering::kTopRight);
    indices_.emplace_back(start + Ordering::kBottomLeft);

    // Lower triangle
    indices_.emplace_back(start + Ordering::kBottomRight);
    indices_.emplace_back(start + Ordering::kBottomLeft);
    indices_.emplace_back(start + Ordering::kTopRight);
}
}  // namespace objects
