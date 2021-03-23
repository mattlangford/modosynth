#pragma once

#include "synth/node.hh"

namespace objects::blocks {
class Speaker final : public synth::EjectorNode {
public:
    inline static const std::string kStreamName = "/speaker";

public:
    Speaker(size_t count) : EjectorNode{kName + std::to_string(count), kStreamName} {}
};

//
// #############################################################################
//

class SpeakerFactory : public SimpleBlockFactory {
public:
    SpeakerFactory() : SimpleBlockFactory([] {
          SimpleBlockFactory::Config config;
          config.name = "Speaker";
          config.inputs = 0;
          config.outputs = 1;
          return config;
      }()) {}

    ~SpeakerFactory() override = default;

public:
    std::unique_ptr<synth::GenericNode> spawn_synth_node() const override
    {
        static size_t counter = 0;
        return std::make_unique<Speaker>(counter++);
    }
};
}  // namespace objects::blocks
