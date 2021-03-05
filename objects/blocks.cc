#include "objects/blocks.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"
#include "engine/utils.hh"
#include "objects/ports.hh"
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

BlockObjectManager::BlockObjectManager(const std::filesystem::path& config_path,
                                       std::shared_ptr<PortsObjectManager> ports_manager)
    : engine::AbstractSingleShaderObjectManager(vertex_shader_text, fragment_shader_text),
      config_(config_path),
      ports_manager_(std::move(ports_manager)),
      texture_(config_.texture_path),
      pool_(std::make_unique<engine::ListObjectPool<BlockObject>>()) {}

//
// #############################################################################
//

void BlockObjectManager::init_with_vao() {
    texture_.init();

    vertex_.init(glGetAttribLocation(get_shader().get_program_id(), "world_position"));
    uv_.init(glGetAttribLocation(get_shader().get_program_id(), "vertex_uv"));
}

//
// #############################################################################
//

void BlockObjectManager::render_with_vao() {
    if (pool_->empty()) return;

    texture_.activate();

    size_t index = 0;
    size_t num_objects = 0;
    for (auto object : pool_->iterate()) {
        vertex_.update_batch(coords(*object), index);
        num_objects++;
    }
    vertex_.finish_batch();

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

    static size_t id = 0;
    spawn_object(BlockObject{{}, config_.blocks[id++ % 2], Eigen::Vector2f{100, 200}});
}

//
// #############################################################################
//

BlockObject* BlockObjectManager::select(const Eigen::Vector2f& position) const {
    if (pool_->empty()) return nullptr;

    auto is_in_object = [&position](const BlockObject& object) {
        Eigen::Vector2f top_left = object.top_left();
        Eigen::Vector2f bottom_right = top_left + object.config.px_dim.cast<float>();
        return engine::is_in_rectangle(position, top_left, bottom_right);
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

engine::Quad2Df BlockObjectManager::coords(const BlockObject& block) const {
    engine::Quad2Df quad;
    const Eigen::Vector2f top_left = block.top_left();
    const Eigen::Vector2f bottom_right = top_left + block.config.px_dim.cast<float>();

    quad.top_left = top_left;
    quad.top_right = Eigen::Vector2f{bottom_right.x(), top_left.y()};
    quad.bottom_left = Eigen::Vector2f{top_left.x(), bottom_right.y()};
    quad.bottom_right = bottom_right;
    return quad;
}

//
// #############################################################################
//

engine::Quad2Df BlockObjectManager::uv(const BlockObject& block) const {
    engine::Quad2Df quad;
    const Eigen::Vector2f texture_dim{texture_.bitmap().get_width(), texture_.bitmap().get_height()};
    const Eigen::Vector2f top_left = block.config.px_start.cast<float>().cwiseQuotient(texture_dim);
    const Eigen::Vector2f bottom_right =
        (block.config.px_start + block.config.px_dim).cast<float>().cwiseQuotient(texture_dim);

    quad.top_left = top_left;
    quad.top_right = Eigen::Vector2f{bottom_right.x(), top_left.y()};
    quad.bottom_left = Eigen::Vector2f{top_left.x(), bottom_right.y()};
    quad.bottom_right = bottom_right;
    return quad;
}

//
// #############################################################################
//

void BlockObjectManager::spawn_object(BlockObject object_) {
    auto [id, object] = pool_->add(std::move(object_));
    object.id = id;

    bind_vao();
    vertex_.add(coords(object));
    uv_.add(uv(object));

    ports_manager_->spawn_object(PortsObject::from_block(object));
}

//
// #############################################################################
//

void BlockObjectManager::despawn_object(const engine::ObjectId& id) { pool_->remove(id); }

//
// #############################################################################
//

}  // namespace objects
