#pragma once

#include "synth/node.hh"

namespace objects::blocks {
class Speaker final : public synth::EjectorNode {
public:
    inline static const std::string kName = "Speaker";
    inline static const std::string kStreamName = "/speaker";

public:
    Speaker(size_t count) : EjectorNode{kName + std::to_string(count), kStreamName} {}
};
}  // namespace objects::blocks
