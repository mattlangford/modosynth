#pragma once

#include "synth/node.hh"

namespace object::blocks {
class Knob final : public synth::InjectorNode {
public:
    inline static const std::string kName = "Knob";

public:
    Knob(size_t count) : InjectorNode{kName + std::to_string(count)} {}
};
}  // namespace object::blocks
