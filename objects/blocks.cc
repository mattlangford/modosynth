#include "objects/blocks.hh"

#include <iostream>
#include <memory>

#include "objects/blocks/vco.hh"
#include "yaml-cpp/yaml.h"

namespace objects {
Config::Config(const std::filesystem::path& path) {
    const auto root = YAML::LoadFile(path);
    texture_path = root["texture_path"].as<std::string>();
    port_texture_path = root["port_texture_path"].as<std::string>();

    for (const auto& block : root["blocks"]) {
        BlockConfig block_config;
        block_config.uv.x() = block["uv"][0].as<int>();
        block_config.uv.y() = block["uv"][1].as<int>();
        block_config.dim.x() = block["dim"][0].as<size_t>();
        block_config.dim.y() = block["dim"][1].as<size_t>();
        blocks[block["name"].as<std::string>()] = block_config;
    }
}

//
// #############################################################################
//

const Config::BlockConfig& Config::get(const std::string& name) const {
    auto it = blocks.find(name);
    if (it == blocks.end()) throw std::runtime_error("Can't find a block config for: '" + name + "' in Config::get()");
    return it->second;
}

//
// #############################################################################
//

std::vector<ecs::Entity> Factory::spawn_ports(const ecs::Entity& parent, bool is_input, size_t count,
                                              objects::ComponentManager& manager) const {
    // TODO
    const float width = 32;
    const float height = 16;

    if (count > height) throw std::runtime_error("Too many ports for the given object!");

    const size_t spacing = height / (count + 1);

    const Eigen::Vector2f dim = Eigen::Vector2f{3.0, 3.0};
    const Eigen::Vector2f uv = Eigen::Vector2f{0.0, 0.0};

    std::vector<ecs::Entity> entities;
    entities.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        if (is_input) {
            Eigen::Vector2f offset{-3.0, height - (i + 1) * spacing - 1.5};
            entities.push_back(manager.spawn(TexturedBox{Transform{parent, offset}, dim, uv, 1}, CableSink{i}));
        } else {
            Eigen::Vector2f offset{width, height - (i + 1) * spacing - 1.5};
            entities.push_back(manager.spawn(TexturedBox{Transform{parent, offset}, dim, uv, 1}, CableSource{i}));
        }
    }

    return entities;
}

//
// #############################################################################
//

BlockLoader::BlockLoader(const std::filesystem::path& config_path) : config_(config_path) {}

//
// #############################################################################
//

void BlockLoader::add_factory(const std::string& name, std::unique_ptr<Factory> factory) {
    factory->load_config(config_);
    factories_[name] = std::move(factory);
}

//
// #############################################################################
//

const Config& BlockLoader::config() const { return config_; }

//
// #############################################################################
//

const Factory& BlockLoader::get(const std::string& name) const {
    auto it = factories_.find(name);
    if (it == factories_.end()) throw std::runtime_error("Unable to find '" + name + "'");
    return *it->second;
}

//
// #############################################################################
//

std::vector<std::string> BlockLoader::textures() const {
    return std::vector<std::string>{config().texture_path, config().port_texture_path};
}

//
// #############################################################################
//

std::vector<std::string> BlockLoader::names() const {
    std::vector<std::string> names;
    names.reserve(size());
    for (const auto& [name, _] : factories_) names.emplace_back(name);
    return names;
}

//
// #############################################################################
//

size_t BlockLoader::size() const { return factories_.size(); }

//
// #############################################################################
//

BlockLoader default_loader() {
    BlockLoader loader("objects/blocks.yml");
    loader.add_factory("VCO", std::make_unique<blocks::VCOFactory>());
    return loader;
}

}  // namespace objects
