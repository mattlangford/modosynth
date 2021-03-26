#pragma once

#include "synth/node.hh"

namespace objects::blocks {
class Amplifier final : public synth::AbstractNode<2, 1> {
public:
    inline static const std::string kName = "Amplifier";

public:
    Amplifier(size_t count) : AbstractNode{kName + std::to_string(count)} {}

public:
    void invoke(const Inputs& inputs, Outputs& outputs) override {
        auto& input = inputs[0];
        auto& level = inputs[1];

        outputs[0].populate_samples([&](size_t i) { return 10 * level.samples[i] * input.samples[i]; });
    }
};

//
// #############################################################################
//

class AmpFactory : public SimpleBlockFactory {
public:
    AmpFactory()
        : SimpleBlockFactory([] {
              SimpleBlockFactory::Config config;
              config.name = Amplifier::kName;
              config.inputs = 2;
              config.outputs = 1;
              return config;
          }()) {}
    ~AmpFactory() override = default;

public:
    std::unique_ptr<synth::GenericNode> spawn_synth_node() const override {
        static size_t counter = 0;
        return std::make_unique<Amplifier>(counter);
    }
};
}  // namespace objects::blocks
