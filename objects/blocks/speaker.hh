#pragma once

#include "synth/node.hh"

namespace object::blocks {
class Speaker final : public synth::AbstractNode<1, 0> {
public:
    inline static const std::string kName = "Speaker";

public:
    Speaker(size_t count) : AbstractNode{kName + std::to_string(count)} {}

public:
    void invoke(const Inputs& inputs, Outputs&) const override {
        for (auto& sample : inputs[0].samples) std::cout << sample << ", ";
        std::cout << std::endl;
    }
};
}  // namespace object::blocks
