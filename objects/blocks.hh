#pragma once
#include <Eigen/Dense>
#include <filesystem>
#include <map>
#include <vector>

#include "engine/buffer.hh"
#include "engine/object_manager.hh"
#include "engine/pool.hh"
#include "engine/shader.hh"
#include "engine/texture.hh"

namespace synth {
class Bridge;
}

namespace objects {

class PortsObjectManager;

//
// #############################################################################
//

struct Config {
    Config(const std::filesystem::path& path);

    std::string texture_path;
    std::string port_texture_path;

    struct BlockConfig {
        std::string name;
        std::string full_name;
        // Where the texture in the texture path, used for UV mapping
        Eigen::Vector2i foreground_start;
        Eigen::Vector2i background_start;
        Eigen::Vector2i px_dim;
        size_t inputs;
        size_t outputs;
    };

    std::vector<BlockConfig> blocks;
};

//
// #############################################################################
//

class Factory {
public:
    virtual ~Factory() = default;

    virtual void spawn_entity() = 0;
    virtual void spawn_node() = 0;

protected:
    const Config::BlockConfig& get_block_config() const { return config_; }

public:
    void set_config(Config::BlockConfig config) { config_ = config; }

private:
    Config::BlockConfig config_;
};

//
// #############################################################################
//

class BlockLoader {
public:
    BlockLoader(const std::filesystem::path& config_path) : config_(config_path) {}

public:
    void add_factory(const std::string& name, std::unique_ptr<Factory> factory) {
        for (const auto& block : config().blocks) {
            if (block.name == name) {
                factory->set_config(block);
                factories_[std::move(name)] = std::move(factory);
                return;
            }
        }

        std::cerr << "Unable to associate '" << name << "' with config, ignoring it.\n";
    }

    const Config& config() const { return config_; }

    Factory& get(const std::string& name) {
        auto it = factories_.find(name);
        if (it == factories_.end()) throw std::runtime_error("Unable to find '" + name + "'");
        return *it->second;
    }

private:
    Config config_;
    std::map<std::string, std::unique_ptr<Factory>> factories_;
};

//
// #############################################################################
//

struct BlockObject {
    engine::ObjectId id = {};
    size_t element_index = -1;
    size_t vertex_index = -1;
    bool needs_update = true;

    // This is a reference to the BlockObjectManager owned configuration
    const Config::BlockConfig* config;

    Eigen::Vector2f offset = Eigen::Vector2f::Zero();
    float z = 0.0;
    float rotation = 0.0;  // only the foreground though

    size_t synth_id = -1;

    // To get it more pixel-y, this will only move in single pixel increments.
    inline Eigen::Vector2f bottom_left() const { return offset.cast<int>().cast<float>(); };
};

//
// #############################################################################
//

class BlockObjectManager final : public engine::AbstractSingleShaderObjectManager {
public:
    BlockObjectManager(const std::filesystem::path& config_path, PortsObjectManager& ports_manager,
                       synth::Bridge& bridge);
    ~BlockObjectManager() override = default;

protected:
    void init_with_vao() override;

    void render_with_vao() override;

public:
    void update(float dt) override;

    void handle_mouse_event(const engine::MouseEvent& event) override;

    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

public:
    const Config& config() const;

    BlockObject& spawn_object(BlockObject object_);

private:
    BlockObject* select(const Eigen::Vector2f& position) const;

    Eigen::Matrix<float, 3, 4> coords(const BlockObject& block) const;
    Eigen::Matrix<float, 4, 4> uv(const BlockObject& block) const;

    float next_z();

private:
    const Config config_;

    PortsObjectManager& ports_manager_;
    synth::Bridge& bridge_;

    BlockObject* selected_ = nullptr;

    // closer points are at 0.0, farther points are at 1.0
    float current_z_ = 1.0;
    constexpr static float kZInc = 1E-5;

    engine::Texture texture_;

    std::unique_ptr<engine::AbstractObjectPool<BlockObject>> pool_;

    engine::Buffer<unsigned int> elements_;
    engine::Buffer<float, 3> vertex_;
    engine::Buffer<float, 4> uv_;
};
}  // namespace objects
