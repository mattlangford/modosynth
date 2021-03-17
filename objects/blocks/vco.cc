#include "objects/blocks/vco.hh"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "objects/blocks/knob.hh"
#include "synth/debug.hh"

namespace object::blocks {

//
// #############################################################################
//

float remap(float raw, const std::tuple<float, float>& from, const std::tuple<float, float>& to) {
    const auto& [from_min, from_max] = from;
    const auto& [to_min, to_max] = to;

    float from_range = from_max - from_min;
    float to_range = to_max - to_min;

    // between [0, 1]
    float normalized = (std::clamp(raw, from_min, from_max) - from_min) / from_range;
    return normalized * to_range + to_min;
}

//
// #############################################################################
//

VoltageControlledOscillator::VoltageControlledOscillator(size_t count) : AbstractNode{kName + std::to_string(count)} {}

//
// #############################################################################
//

void VoltageControlledOscillator::invoke(const Inputs& inputs, Outputs& outputs) {
    auto& frequencies = inputs[0].samples;
    auto& shapes = inputs[1].samples;
    auto& output = outputs[0];

    output.populate_samples([&](size_t i) {
        return sample(remap(frequencies[i], {-1.0, 1.0}, {10, 10000}),
                      remap(shapes[i], {-1.0, 1.0}, {0.0, static_cast<int>(Shape::kMax) - 1}));
    });
}

//
// #############################################################################
//

float VoltageControlledOscillator::sample(float frequency, float shape) {
    constexpr auto kMax = static_cast<int>(Shape::kMax);
    int discrete_shape = static_cast<int>(shape);
    float percent = shape - discrete_shape;
    const Shape shape0 = static_cast<Shape>(discrete_shape);
    const Shape shape1 = static_cast<Shape>((discrete_shape + 1) % kMax);

    auto sample_with_shape = [phase = this->phase](const Shape s) -> float {
        switch (s) {
            case Shape::kSin:
                return std::sin(phase);
            case Shape::kSquare:
                return std::fmod(phase, 2 * M_PI) < M_PI ? -1.f : 1.f;
            case Shape::kMax:
            default:
                throw std::runtime_error("Invalid shape in VCO::sample()!");
                break;
        }
    };

    phase += phase_increment(frequency);
    std::cout << "f: " << frequency << "shape0: " << (int)shape0 << " shape1: " << (int)shape1 << " p: " << percent
              << "\n";
    return percent * sample_with_shape(shape0) + (1.0 - percent) * sample_with_shape(shape1);
}

//
// #############################################################################
//

double VoltageControlledOscillator::phase_increment(float frequency) {
    return 2.0 * M_PI * static_cast<double>(frequency) / synth::Samples::kSampleRate;
}
}  // namespace object::blocks
