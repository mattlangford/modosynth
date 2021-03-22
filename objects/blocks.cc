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
        block_config.name = block["name"].as<std::string>();
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
}  // namespace objects
