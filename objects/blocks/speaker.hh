#pragma once

#include "synth/node.hh"

namespace objects::blocks {
class Speaker final : public synth::EjectorNode {
public:
    inline static const std::string kStreamName = "/speaker";
    inline static const std::string kName = "Speaker";

public:
    Speaker(size_t count) : EjectorNode{kName + std::to_string(count)} {}
};

//
// #############################################################################
//

class SpeakerFactory final : public SimpleBlockFactory {
public:
    SpeakerFactory()
        : SimpleBlockFactory([] {
              SimpleBlockFactory::Config config;
              config.name = "Speaker";
              config.inputs = 1;
              config.outputs = 0;
              return config;
          }()) {}
    ~SpeakerFactory() override = default;

public:
    Spawn spawn_entities(objects::ComponentManager& manager) const override {
        Spawn spawn = SimpleBlockFactory::spawn_entities(manager);
        spawn.entities.push_back(manager.spawn(SynthOutput{spawn.primary, Speaker::kStreamName, {}}));
        return spawn;
    }

    std::unique_ptr<synth::GenericNode> spawn_synth_node() const override {
        static size_t counter = 0;
        return std::make_unique<Speaker>(counter++);
    }

private:
    Eigen::Vector2f foreground_uv_;
};
}  // namespace objects::blocks
