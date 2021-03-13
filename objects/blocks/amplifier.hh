#pragma once

#include "synth/node.hh"

namespace object::blocks {
class Amplifier final : public synth::AbstractNode<2, 1> {
public:
    inline static const std::string kName = "Amplifier";

public:
    Amplifier(size_t count) : AbstractNode{kName + std::to_string(count)} {}

public:
    void invoke(const Inputs& inputs, Outputs& outputs) const override {
        auto& input = inputs[0];
        auto& level = inputs[1];

        outputs[0].populate_samples([&](size_t i) { return 10 * level.samples[i] * input.samples[i]; });
    }
};
}  // namespace object::blocks
