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

    // This is a reference to the BlockObjectManager owned configuration
    const Config::BlockConfig& config;

    Eigen::Vector2f offset;

    // To get it more pixel-y, this will only move in single pixel increments.
    inline Eigen::Vector2f top_left() const { return offset.cast<int>().cast<float>(); };
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

    engine::Quad coords(const BlockObject& block) const;
    engine::Quad uv(const BlockObject& block) const;

    void spawn_object(BlockObject object_);

    void despawn_object(const engine::ObjectId& id);

private:
    const Config config_;

    std::shared_ptr<PortsObjectManager> ports_manager_;

    BlockObject* selected_ = nullptr;

    engine::Texture texture_;

    std::unique_ptr<engine::AbstractObjectPool<BlockObject>> pool_;

    engine::Buffer2Df vertex_;
    engine::Buffer2Df uv_;
};
}  // namespace objects
