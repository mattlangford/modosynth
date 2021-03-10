#pragma once
#include <Eigen/Dense>
#include <filesystem>
#include <vector>

#include "engine/buffer.hh"
#include "engine/object_manager.hh"
#include "engine/pool.hh"
#include "engine/shader.hh"
#include "engine/texture.hh"

namespace objects {

class PortsObjectManager;

//
// #############################################################################
//

struct Config {
    Config(const std::filesystem::path& path);

    std::string texture_path;

    struct BlockConfig {
        std::string name;
        std::string description;
        // Where the texture in the texture path, used for UV mapping
        Eigen::Vector2i px_start;
        Eigen::Vector2i px_dim;
        size_t inputs;
        size_t outputs;
    };

    std::vector<BlockConfig> blocks;
};

//
// #############################################################################
//

struct BlockObject {
    engine::ObjectId id;
    size_t element_index;
    size_t vertex_index;
    bool needs_update = true;

    // This is a reference to the BlockObjectManager owned configuration
    const Config::BlockConfig& config;

    Eigen::Vector2f offset;
    float z;

    // To get it more pixel-y, this will only move in single pixel increments.
    inline Eigen::Vector2f bottom_left() const { return offset.cast<int>().cast<float>(); };
};

//
// #############################################################################
//

class BlockObjectManager final : public engine::AbstractSingleShaderObjectManager {
public:
    BlockObjectManager(const std::filesystem::path& path_config, std::shared_ptr<PortsObjectManager> ports_manager);
    virtual ~BlockObjectManager() = default;

protected:
    void init_with_vao() override;

    void render_with_vao() override;

public:
    void update(float dt) override;

    void handle_mouse_event(const engine::MouseEvent& event) override;

    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

private:
    BlockObject* select(const Eigen::Vector2f& position) const;

    Eigen::Matrix<float, 3, 4> coords(const BlockObject& block) const;
    Eigen::Matrix<float, 2, 4> uv(const BlockObject& block) const;

    void spawn_object(BlockObject object_);

    float next_z();

private:
    const Config config_;

    std::shared_ptr<PortsObjectManager> ports_manager_;

    BlockObject* selected_ = nullptr;

    // closer points are at 0.0, farther points are at 1.0
    float current_z_ = 1.0;
    constexpr static float kZInc = 1E-5;

    engine::Texture texture_;

    std::unique_ptr<engine::AbstractObjectPool<BlockObject>> pool_;

    engine::Buffer<unsigned int> elements_;
    engine::Buffer<float, 3> vertex_;
    engine::Buffer<float, 2> uv_;
};
}  // namespace objects
