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
layout (location = 0) in vec3 world_position;
layout (location = 1) in vec4 uv; // vec2: Foreground vec2: Background

out vec2 uv_fg;
out vec2 uv_bg;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, world_position.z, 1.0);

    uv_fg = vec2(uv[0], uv[1]);
    uv_bg = vec2(uv[2], uv[3]);
}
)";

static std::string fragment_shader_text = R"(
#version 330
in vec2 uv_fg;
in vec2 uv_bg;

out vec4 fragment;
uniform sampler2D sampler;
void main()
{
    fragment = texture(sampler, uv_bg);
    fragment += texture(sampler, uv_fg);

    fragment = min(vec4(1.0, 1.0, 1.0, 1.0), fragment);
    fragment = max(vec4(0.0, 0.0, 0.0, 0.0), fragment);
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
        block_config.foreground_start.x() = block["foreground_start"][0].as<int>();
        block_config.foreground_start.y() = block["foreground_start"][1].as<int>();
        block_config.background_start.x() = block["background_start"][0].as<int>();
        block_config.background_start.y() = block["background_start"][1].as<int>();
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
    vertex_.init(GL_ARRAY_BUFFER, 0, vao());
    uv_.init(GL_ARRAY_BUFFER, 1, vao());
}

//
// #############################################################################
//

void BlockObjectManager::render_with_vao() {
    if (pool_->empty()) return;

    texture_.activate();

    auto objects = pool_->iterate();

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
    auto vertices = vertex_.batched_updater();
    auto uv_batch = uv_.batched_updater();
    for (auto* object : pool_->iterate()) {
        if (!object->needs_update) continue;

        vertices.elements<4>(object->vertex_index) = coords(*object);
        uv_batch.elements<4>(object->vertex_index) = uv(*object);
        object->needs_update = false;
    }
}

//
// #############################################################################
//

void BlockObjectManager::handle_mouse_event(const engine::MouseEvent& event) {
    if (event.right || !event.clicked) {
        if (selected_) {
            selected_->needs_update = true;
            selected_->z = next_z();
            selected_ = nullptr;
        }
        return;
    }

    if (selected_) {
        // bring the selected object to the front
        selected_->z = 0.0;
        selected_->needs_update = true;

        // Rotate the foreground
        if (event.shift) {
            selected_->rotation += event.delta_position.y();
        } else {
            selected_->offset.head(2) += event.delta_position;
        }
    } else if (event.pressed()) {
        // only select a new one if the mouse was just clicked
        selected_ = select(event.mouse_position);
    }
}

//
// #############################################################################
//

void BlockObjectManager::handle_keyboard_event(const engine::KeyboardEvent& event) {
    if (event.pressed()) {
        return;
    }

    auto spawn_impl = [this](size_t index) {
        spawn_object(BlockObject{{}, {}, {}, true, config_.blocks[index], Eigen::Vector2f{100, 200}, next_z(), 0});
    };

    if (event.space) {
        static size_t id = 0;
        spawn_impl(id++);
        return;
    }

    size_t index = event.key - '0';
    if (index < config_.blocks.size()) {
        spawn_impl(index);
    }
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

Eigen::Matrix<float, 3, 4> BlockObjectManager::coords(const BlockObject& block) const {
    const Eigen::Vector2f bottom_left = block.bottom_left();
    const Eigen::Vector2f top_right = bottom_left + block.config.px_dim.cast<float>();

    Eigen::Matrix<float, 3, 4> quad;
    quad.col(0) = Eigen::Vector3f(bottom_left.x(), top_right.y(), block.z);
    quad.col(1) = Eigen::Vector3f{top_right.x(), top_right.y(), block.z};
    quad.col(2) = Eigen::Vector3f{bottom_left.x(), bottom_left.y(), block.z};
    quad.col(3) = Eigen::Vector3f{top_right.x(), bottom_left.y(), block.z};
    return quad;
}

//
// #############################################################################
//

Eigen::Matrix<float, 4, 4> BlockObjectManager::uv(const BlockObject& block) const {
    const Eigen::Vector2f texture_dim{texture_.bitmap().get_width(), texture_.bitmap().get_height()};

    auto compute = [&](const Eigen::Vector2i& start) -> Eigen::Matrix<float, 2, 4> {
        const Eigen::Vector2f top_left = start.cast<float>().cwiseQuotient(texture_dim);
        const Eigen::Vector2f bottom_right = (start + block.config.px_dim).cast<float>().cwiseQuotient(texture_dim);

        Eigen::Matrix<float, 2, 4> quad;
        quad.col(0) = top_left;
        quad.col(1) = Eigen::Vector2f{bottom_right.x(), top_left.y()};
        quad.col(2) = Eigen::Vector2f{top_left.x(), bottom_right.y()};
        quad.col(3) = bottom_right;
        return quad;
    };

    Eigen::Matrix<float, 2, 4> foreground = compute(block.config.foreground_start);
    Eigen::Matrix<float, 2, 4> background = compute(block.config.background_start);

    if (block.rotation != 0.f) {
        Eigen::Matrix2f rotation = Eigen::Matrix2f::Identity();
        float s = std::sin(block.rotation);
        float c = std::cos(block.rotation);
        rotation << c, -s, s, c;

        Eigen::Matrix<float, 2, 1> center = foreground.rowwise().mean();
        for (int i = 0; i < foreground.cols(); ++i) foreground.col(i) -= center;
        foreground = (rotation * foreground).eval();
        for (int i = 0; i < foreground.cols(); ++i) foreground.col(i) += center;
    }

    Eigen::Matrix<float, 4, 4> result;
    result.row(0) = foreground.row(0);
    result.row(1) = foreground.row(1);
    result.row(2) = background.row(0);
    result.row(3) = background.row(1);
    return result;
}

//
// #############################################################################
//

void BlockObjectManager::spawn_object(BlockObject object_) {
    auto [id, object] = pool_->add(std::move(object_));
    object.id = id;
    object.element_index = elements_.size();
    object.vertex_index = vertex_.elements();

    {
        auto elements = elements_.batched_updater();
        auto vertices = vertex_.batched_updater();
        auto uv_batch = uv_.batched_updater();

        vertices.elements<4>(object.vertex_index) = coords(object);
        uv_batch.elements<4>(object.vertex_index) = uv(object);

        // First triangle
        elements_.push_back(object.vertex_index + 0);
        elements_.push_back(object.vertex_index + 1);
        elements_.push_back(object.vertex_index + 2);

        // Second triangle
        elements_.push_back(object.vertex_index + 3);
        elements_.push_back(object.vertex_index + 2);
        elements_.push_back(object.vertex_index + 1);
    }

    ports_manager_->spawn_object(PortsObject::from_block(object));
}

//
// #############################################################################
//

float BlockObjectManager::next_z() { return current_z_ -= kZInc; }

}  // namespace objects
