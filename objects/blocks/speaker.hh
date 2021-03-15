#pragma once

#include "synth/node.hh"

namespace object::blocks {
class Speaker final : public synth::EjectorNode {
public:
    inline static const std::string kName = "Speaker";

public:
    Speaker(size_t count) : EjectorNode{kName + std::to_string(count), "/speaker"} {}
};
}  // namespace object::blocks
