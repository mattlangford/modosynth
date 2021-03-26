#pragma once

#include "objects/blocks.hh"
#include "synth/node.hh"

namespace objects::blocks {
class Button final : public synth::InjectorNode {
public:
    inline static const std::string kName = "Button";

public:
    Button(size_t count) : InjectorNode{kName + std::to_string(count)} {}
};

//
// #############################################################################
//

class ButtonFactory final : public SimpleBlockFactory {
public:
    ButtonFactory()
        : SimpleBlockFactory([] {
              SimpleBlockFactory::Config config;
              config.name = "Button";
              config.inputs = 0;
              config.outputs = 1;
              return config;
          }()) {}
    ~ButtonFactory() override = default;

public:
    void load_config(const objects::Config& config) override {
        SimpleBlockFactory::load_config(config);
        foreground_uv_ = config.get("ButtonForeground").uv.cast<float>();
    }

    Spawn spawn_entities(objects::ComponentManager& manager) const override {
        Spawn spawn = SimpleBlockFactory::spawn_entities(manager);
        spawn.entities.push_back(
            manager.spawn(TexturedBox{Transform{spawn.primary, Eigen::Vector2f::Zero()}, dim(), foreground_uv_, 0},
                          SynthInput{spawn.primary, 0.0, SynthInput::kButton}, Selectable::require_shift()));
        return spawn;
    }

    std::unique_ptr<synth::GenericNode> spawn_synth_node() const override {
        static size_t counter = 0;
        return std::make_unique<Button>(counter++);
    }

private:
    Eigen::Vector2f foreground_uv_;
};
}  // namespace objects::blocks
