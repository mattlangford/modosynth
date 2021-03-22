#include "objects/blocks.hh"

#include <iostream>

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
}  // namespace objects
