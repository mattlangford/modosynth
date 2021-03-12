#pragma once

#include "synth/node.hh"

namespace object::blocks {
class Knob final : public synth::AbstractNode<0, 1> {
public:
    inline static const std::string kName = "Knob";

public:
    Knob(size_t count) : AbstractNode{kName + std::to_string(count)} {}

public:
    void invoke(const Inputs&, Outputs& outputs) const override {
        auto& samples = outputs[0].samples;
        std::fill(samples.begin(), samples.end(), value_);
    }

    void set_value(float value) override { value_ = value; }

private:
    float value_ = 0.0;
};
}  // namespace object::blocks
