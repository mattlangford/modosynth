#pragma once

#include <tuple>

#include "synth/node.hh"

namespace object::blocks {
float remap(float raw, const std::tuple<float, float>& from, const std::tuple<float, float>& to);

class VoltageControlledOscillator final : public synth::AbstractNode<2, 1> {
public:
    inline static const std::string kName = "Voltage Controlled Oscillator";

public:
    VoltageControlledOscillator(size_t count = 0);

public:
    enum class Shape : uint8_t {
        kSin = 0,
        kSquare = 1,
        kMax = 2,
    };

    void invoke(const Inputs& inputs, Outputs& outputs) override;

    float sample(float frequency, float shape);

private:
    double phase_increment(float frequency);

private:
    double phase = 0.0;
};
}  // namespace object::blocks
