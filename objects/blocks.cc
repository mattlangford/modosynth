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
in vec3 world_position;
in vec2 vertex_uv;

out vec2 uv;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, world_position.z, 1.0);
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

    elements_.init(GL_ELEMENT_ARRAY_BUFFER, vao());
    vertex_.init(GL_ARRAY_BUFFER, glGetAttribLocation(shader().get_program_id(), "world_position"), 3, vao());
    uv_.init(GL_ARRAY_BUFFER, glGetAttribLocation(shader().get_program_id(), "vertex_uv"), 2, vao());
}

//
// #############################################################################
//

void BlockObjectManager::render_with_vao() {
    if (pool_->empty()) return;

    texture_.activate();

    auto objects = pool_->iterate();

    {
        size_t index = 0;
        auto vertices = vertex_.batched_updater();
        for (const auto* object : objects) {
            auto c = coords(*object);
            vertices[index++] = c.top_left.x();
            vertices[index++] = c.top_left.y();
            vertices[index++] = c.top_left.z();

            vertices[index++] = c.top_right.x();
            vertices[index++] = c.top_right.y();
            vertices[index++] = c.top_right.z();

            vertices[index++] = c.bottom_left.x();
            vertices[index++] = c.bottom_left.y();
            vertices[index++] = c.bottom_left.z();

            vertices[index++] = c.bottom_right.x();
            vertices[index++] = c.bottom_right.y();
            vertices[index++] = c.bottom_right.z();
        }
    }

    std::sort(objects.begin(), objects.end(),
              [](const BlockObject* const lhs, const BlockObject* const rhs) { return lhs->z > rhs->z; });

    for (const auto* object : objects) {
        // 3 vertices per triangle, 2 triangles per object
        gl_check_with_vao(vao(), glDrawElements, GL_TRIANGLES, 3 * 2, GL_UNSIGNED_INT,
                          (void*)(sizeof(unsigned int) * object->element_index));
    }
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
        if (selected_) {
            selected_->z = next_z();
            selected_ = nullptr;
        }
        return;
    }

    if (selected_) {
        // bring the selected object to the front
        selected_->z = 0.0;
        selected_->offset.head(2) += event.delta_position;
    } else if (!event.any_modifiers() && event.pressed()) {
        // only select a new one if the mouse was just clicked
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
    spawn_object(
        BlockObject{{}, {}, config_.blocks[id++ % config_.blocks.size()], Eigen::Vector2f{100, 200}, next_z()});
}

//
// #############################################################################
//

BlockObject* BlockObjectManager::select(const Eigen::Vector2f& position) const {
    if (pool_->empty()) return nullptr;

    auto is_in_object = [&position](const BlockObject& object) {
        Eigen::Vector2f bottom_left = object.bottom_left();
        Eigen::Vector2f top_right = bottom_left + object.config.px_dim.cast<float>();
        return engine::is_in_rectangle(position, bottom_left, top_right);
    };

    BlockObject* select_object = nullptr;
    for (auto object : pool_->iterate()) {
        if ((select_object && object->z > select_object->z) || !is_in_object(*object)) continue;

        select_object = object;
    }
    return select_object;
}

//
// #############################################################################
//

engine::Quad3Df BlockObjectManager::coords(const BlockObject& block) const {
    engine::Quad3Df quad;
    const Eigen::Vector2f bottom_left = block.bottom_left();
    const Eigen::Vector2f top_right = bottom_left + block.config.px_dim.cast<float>();

    quad.top_left = Eigen::Vector3f(bottom_left.x(), top_right.y(), block.z);
    quad.top_right = Eigen::Vector3f{top_right.x(), top_right.y(), block.z};
    quad.bottom_left = Eigen::Vector3f{bottom_left.x(), bottom_left.y(), block.z};
    quad.bottom_right = Eigen::Vector3f{top_right.x(), bottom_left.y(), block.z};
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
    object.element_index = elements_.size();

    size_t vertex_index = vertex_.elements();
    {
        auto elements = elements_.batched_updater();
        auto vertices = vertex_.batched_updater();
        auto uv_batch = uv_.batched_updater();

        auto c = coords(object);
        vertices.push_back(c.top_left.x());
        vertices.push_back(c.top_left.y());
        vertices.push_back(c.top_left.z());
        vertices.push_back(c.top_right.x());
        vertices.push_back(c.top_right.y());
        vertices.push_back(c.top_right.z());
        vertices.push_back(c.bottom_left.x());
        vertices.push_back(c.bottom_left.y());
        vertices.push_back(c.bottom_left.z());
        vertices.push_back(c.bottom_right.x());
        vertices.push_back(c.bottom_right.y());
        vertices.push_back(c.bottom_right.z());

        elements_.push_back(vertex_index + 0);
        elements_.push_back(vertex_index + 1);
        elements_.push_back(vertex_index + 2);

        elements_.push_back(vertex_index + 3);
        elements_.push_back(vertex_index + 2);
        elements_.push_back(vertex_index + 1);

        auto u = uv(object);
        uv_batch.push_back(u.top_left.x());
        uv_batch.push_back(u.top_left.y());
        uv_batch.push_back(u.top_right.x());
        uv_batch.push_back(u.top_right.y());
        uv_batch.push_back(u.bottom_left.x());
        uv_batch.push_back(u.bottom_left.y());
        uv_batch.push_back(u.bottom_right.x());
        uv_batch.push_back(u.bottom_right.y());
    }

    ports_manager_->spawn_object(PortsObject::from_block(object));
}

//
// #############################################################################
//

float BlockObjectManager::next_z() { return current_z_ -= kZInc; }

}  // namespace objects
