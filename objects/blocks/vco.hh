#pragma once

#include <tuple>

#include "objects/blocks.hh"
#include "synth/node.hh"

namespace objects::blocks {

float remap(float raw, const std::tuple<float, float>& from, const std::tuple<float, float>& to);

class VoltageControlledOscillator final : public synth::AbstractNode<2, 1> {
public:
    inline static const std::string kName = "Voltage Controlled Oscillator";

public:
    VoltageControlledOscillator(float f_min, float f_max, size_t count = 0);

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
    std::tuple<float, float> frequency_;
    double phase_ = 0.0;
};

//
// #############################################################################
//

class VCOFactory : public SimpleBlockFactory {
public:
    VCOFactory();
    ~VCOFactory() override = default;
};
}  // namespace objects::blocks
