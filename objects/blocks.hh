#pragma once
#include <Eigen/Dense>
#include <filesystem>
#include <map>
#include <memory>
#include <unordered_map>

#include "ecs/entity.hh"
#include "objects/components.hh"
#include "synth/node.hh"

namespace objects {

//
// #############################################################################
//

struct Config {
    Config(const std::filesystem::path& path);

    std::string texture_path;
    std::string port_texture_path;

    struct BlockConfig {
        std::string name;
        Eigen::Vector2i uv;
        Eigen::Vector2i dim;
    };

    const BlockConfig& get(const std::string& name) const;

    std::unordered_map<std::string, BlockConfig> blocks;
};

//
// #############################################################################
//

class Factory {
public:
    virtual ~Factory() = default;

    ///
    /// @brief Called during init and can be used to pull config information out of the full config
    ///
    virtual void load_config(const Config& config) = 0;

    ///
    /// @brief Spawn this block and all associated entities
    ///
    virtual std::vector<ecs::Entity> spawn_entities(objects::ComponentManager& manager) const = 0;

    ///
    /// @brief Spawn the synth node
    ///
    virtual std::unique_ptr<synth::GenericNode> spawn_synth_node() const = 0;
};

//
// #############################################################################
//

class SimpleBlockFactory : public Factory {
public:
    struct Config {
        std::string name;
        size_t inputs;
        size_t outputs;
    };

public:
    SimpleBlockFactory(Config config);
    ~SimpleBlockFactory() override = default;

public:
    void load_config(const objects::Config& config) override;
    std::vector<ecs::Entity> spawn_entities(objects::ComponentManager& manager) const override;

private:
    std::vector<ecs::Entity> spawn_ports(const ecs::Entity& parent, objects::ComponentManager& manager) const;

private:
    Config config_;

    Eigen::Vector2f uv_;
    Eigen::Vector2f dim_;
};

//
// #############################################################################
//

class BlockLoader {
public:
    BlockLoader(const std::filesystem::path& config_path);

public:
    void add_factory(const std::string& name, std::unique_ptr<Factory> factory);

    const Config& config() const;

    const Factory& get(const std::string& name) const;

    std::vector<std::string> textures() const;
    std::vector<std::string> names() const;

    size_t size() const;

private:
    Config config_;
    std::map<std::string, std::unique_ptr<Factory>> factories_;
};

//
// #############################################################################
//

BlockLoader default_loader();

}  // namespace objects
