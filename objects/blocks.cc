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
    auto [id, object] = pool_->add(std::move(object_));
    object.id = id;

    gl_safe(glBindVertexArray, vertex_array_index_);
    vertex_.add(coords(object));
    uv_.add(uv(object));
    gl_safe(glBindVertexArray, 0);
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

    gl_safe(glGenVertexArrays, 1, &vertex_array_index_);
    gl_safe(glBindVertexArray, vertex_array_index_);

    vertex_.init(glGetAttribLocation(shader_.get_program_id(), "world_position"));
    uv_.init(glGetAttribLocation(shader_.get_program_id(), "vertex_uv"));

    gl_safe(glBindVertexArray, 0);

    screen_from_world_loc_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");
}

//
// #############################################################################
//

void BlockObjectManager::render(const Eigen::Matrix3f& screen_from_world) {
    if (pool_->empty()) return;

    shader_.activate();
    texture_.activate();

    gl_safe(glBindVertexArray, vertex_array_index_);
    gl_safe(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());

    size_t index = 0;
    size_t num_objects = 0;
    for (auto object : pool_->iterate()) {
        vertex_.update(coords(*object), index);
        num_objects++;
    }

    gl_safe(glDrawElements, GL_TRIANGLES, 3 * 2 * num_objects, GL_UNSIGNED_INT, (void*)0);
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
        selected_->real_offset += event.delta_position;
        selected_->offset = selected_->real_offset.cast<int>().cast<float>();
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
    object.real_offset = {100, 200};
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

engine::Quad BlockObjectManager::coords(const BlockObject& block) const {
    engine::Quad quad;
    const auto& config = config_.blocks[block.config_id];

    const Eigen::Vector2f top_left = block.offset;
    const Eigen::Vector2f bottom_right = top_left + config.px_dim.cast<float>();

    quad.top_left = top_left;
    quad.top_right = Eigen::Vector2f{bottom_right.x(), top_left.y()};
    quad.bottom_left = Eigen::Vector2f{top_left.x(), bottom_right.y()};
    quad.bottom_right = bottom_right;
    return quad;
}

//
// #############################################################################
//

engine::Quad BlockObjectManager::uv(const BlockObject& block) const {
    engine::Quad quad;
    const auto& config = config_.blocks[block.config_id];

    const Eigen::Vector2f texture_dim{texture_.bitmap().get_width(), texture_.bitmap().get_height()};
    const Eigen::Vector2f top_left = config.px_start.cast<float>().cwiseQuotient(texture_dim);
    const Eigen::Vector2f bottom_right = (config.px_start + config.px_dim).cast<float>().cwiseQuotient(texture_dim);

    quad.top_left = top_left;
    quad.top_right = Eigen::Vector2f{bottom_right.x(), top_left.y()};
    quad.bottom_left = Eigen::Vector2f{top_left.x(), bottom_right.y()};
    quad.bottom_right = bottom_right;
    return quad;
}
}  // namespace objects
