#include "objects/blocks.hh"

#include <iostream>
#include <memory>

#include "objects/blocks/amplifier.hh"
#include "objects/blocks/button.hh"
#include "objects/blocks/filter.hh"
#include "objects/blocks/knob.hh"
#include "objects/blocks/speaker.hh"
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

SimpleBlockFactory::SimpleBlockFactory(Config config) : config_(std::move(config)) {}

//
// #############################################################################
//

void SimpleBlockFactory::load_config(const objects::Config& config) {
    auto& block_config = config.get(config_.name);
    uv_ = block_config.uv.cast<float>();
    dim_ = block_config.dim.cast<float>();
}

//
// #############################################################################
//

const std::string& SimpleBlockFactory::name() const { return config_.name; }

//
// #############################################################################
//

Spawn SimpleBlockFactory::spawn_entities(objects::ComponentManager& manager) const {
    const Eigen::Vector2f location{100, 200};
    auto block = manager.spawn(TexturedBox{Transform{std::nullopt, location}, dim_, uv_, 0}, Selectable{},
                               Moveable{location, true}, SynthNode{});

    std::vector<ecs::Entity> entities;
    for (auto e : spawn_ports(block, manager)) {
        entities.push_back(e);
    }

    return {block, std::move(entities)};
}
//
// #############################################################################
//

std::vector<ecs::Entity> SimpleBlockFactory::spawn_ports(const ecs::Entity& parent, ComponentManager& manager) const {
    const float width = dim_.x();
    const float height = dim_.y();

    const size_t input_spacing = height / (config_.inputs + 1);
    const size_t output_spacing = height / (config_.outputs + 1);

    const Eigen::Vector2f dim = Eigen::Vector2f{3.0, 3.0};
    const Eigen::Vector2f uv = Eigen::Vector2f{0.0, 0.0};

    std::vector<ecs::Entity> entities;
    entities.reserve(config_.inputs + config_.outputs);

    for (size_t i = 0; i < config_.inputs; ++i) {
        Eigen::Vector2f offset{-3.0, height - (i + 1) * input_spacing - 1.5};
        entities.push_back(manager.spawn(TexturedBox{Transform{parent, offset}, dim, uv, 1}, CableSink{i}));
    }
    for (size_t i = 0; i < config_.outputs; ++i) {
        Eigen::Vector2f offset{width, height - (i + 1) * output_spacing - 1.5};
        entities.push_back(manager.spawn(TexturedBox{Transform{parent, offset}, dim, uv, 1}, CableSource{i}));
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

void BlockLoader::add_factory(std::unique_ptr<Factory> factory) {
    factory->load_config(config_);
    factories_[factory->name()] = std::move(factory);
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
    loader.add_factory(std::make_unique<blocks::VCOFactory>());
    loader.add_factory(std::make_unique<blocks::LFOFactory>());
    loader.add_factory(std::make_unique<blocks::SpeakerFactory>());
    loader.add_factory(std::make_unique<blocks::KnobFactory>());
    loader.add_factory(std::make_unique<blocks::ButtonFactory>());
    loader.add_factory(std::make_unique<blocks::AmpFactory>());
    loader.add_factory(std::make_unique<blocks::HPFFactory>());
    loader.add_factory(std::make_unique<blocks::LPFFactory>());
    return loader;
}
}  // namespace objects
