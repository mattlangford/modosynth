#pragma once
#include <Eigen/Dense>
#include <filesystem>
#include <map>
#include <vector>

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

    virtual void load_config(const Config& config) = 0;
    virtual void spawn_entity() = 0;
    virtual void spawn_node() = 0;
};

//
// #############################################################################
//

class BlockLoader {
public:
    BlockLoader(const std::filesystem::path& config_path) : config_(config_path) {}

public:
    void add_factory(const std::string& name, std::unique_ptr<Factory> factory) {
        factory->load_config(config_);
        factories_[name] = std::move(factory);
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
}  // namespace objects
