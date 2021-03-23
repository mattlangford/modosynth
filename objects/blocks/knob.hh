#pragma once

#include "objects/blocks.hh"
#include "synth/node.hh"

namespace objects::blocks {
class Knob final : public synth::InjectorNode {
public:
    inline static const std::string kName = "Knob";

public:
    Knob(size_t count) : InjectorNode{kName + std::to_string(count)} {}
};

//
// #############################################################################
//

class KnobFactory : public SimpleBlockFactory {
public:
    KnobFactory()
        : SimpleBlockFactory([] {
              SimpleBlockFactory::Config config;
              config.name = "Knob";
              config.inputs = 0;
              config.outputs = 1;
              return config;
          }()) {}
    ~KnobFactory() override = default;

public:
    void load_config(const objects::Config& config) override {
        SimpleBlockFactory::load_config(config);
        foreground_uv_ = config.get("KnobForeground").uv.cast<float>();
    }

    std::vector<ecs::Entity> spawn_entities(objects::ComponentManager& manager) const override {
        std::vector<ecs::Entity> result = SimpleBlockFactory::spawn_entities(manager);
        auto& parent = result.front();

        result.push_back(manager.spawn(
            TexturedBox{Transform{parent, Eigen::Vector2f::Zero()}, dim(), foreground_uv_, 0}, Rotateable{1.0}));

        return result;
    }

    std::unique_ptr<synth::GenericNode> spawn_synth_node() const override {
        static size_t counter = 0;
        return std::make_unique<Knob>(counter++);
    }

private:
    Eigen::Vector2f foreground_uv_;
};
}  // namespace objects::blocks
