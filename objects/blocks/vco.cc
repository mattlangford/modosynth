#include "objects/blocks/vco.hh"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "objects/blocks/knob.hh"

namespace object::blocks {
namespace {
float compute_freq(float raw) {
    constexpr float kMinFreq = 10;
    constexpr float kMaxFreq = 10'000;
    constexpr float kRange = kMaxFreq - kMinFreq;

    float percent = (raw - Knob::kMin) / Knob::kRange;
    return percent * kRange + kMinFreq;
}
}  // namespace

//
// #############################################################################
//

VoltageControlledOscillator::VoltageControlledOscillator(size_t count) : AbstractNode{kName + std::to_string(count)} {}

//
// #############################################################################
//

void VoltageControlledOscillator::invoke(const synth::Context& context, const Inputs& inputs, Outputs& outputs) const {
    // Both of these are assumed to be constant during these samples
    const float frequency = compute_freq(inputs[0].samples[0]);
    const float shape = inputs[1].samples[0];
    auto& output = outputs[0].samples;

    int discrete_shape = static_cast<int>(shape);
    float percent = shape - discrete_shape;

    int max = static_cast<int>(Shape::kMax);
    const Shape shape0 = static_cast<Shape>(discrete_shape % max);
    const float mult0 = percent;
    const Shape shape1 = static_cast<Shape>((discrete_shape + 1) % max);
    const float mult1 = 1.0 - percent;

    output = compute_batch(shape0, frequency, context.timestamp);
    auto batch1 = compute_batch(shape1, frequency, context.timestamp);
    for (size_t i = 0; i < batch1.size(); ++i) {
        output[i] = mult0 * output[i] + mult1 * batch1[i];
    }
}

//
// #############################################################################
//

auto VoltageControlledOscillator::compute_batch(const Shape shape, const float frequency,
                                                const std::chrono::nanoseconds& t) -> Batch {
    Batch batch;
    switch (shape) {
        case Shape::kSin:
            batch = sin_batch(frequency, t);
            break;
        case Shape::kSquare:
            batch = square_batch(frequency, t);
            break;
        case Shape::kMax:
        default:
            batch = constant_batch(-1.0);  // error case, really
            break;
    }
    return batch;
}

//
// #############################################################################
//

auto VoltageControlledOscillator::constant_batch(float value) -> Batch {
    Batch batch;
    for (size_t i = 0; i < batch.size(); ++i) batch[i] = value;
    return batch;
}

//
// #############################################################################
//

auto VoltageControlledOscillator::square_batch(const float frequency, const std::chrono::nanoseconds& t) -> Batch {
    Batch batch;

    std::chrono::duration<float> period{1.0 / frequency};
    std::chrono::duration<float> half_period{0.5 * period};

    std::chrono::nanoseconds mod_with{static_cast<int>(1E9 * period.count()) + 1};
    std::chrono::nanoseconds ns = t % mod_with;
    std::chrono::duration<float> next_switch = ns < half_period ? half_period : period;
    bool value = ns >= half_period;

    for (size_t i = 0; i < batch.size(); ++i, ns += synth::Samples::kSampleIncrement) {
        if (ns >= next_switch) {
            value = !value;
            next_switch += half_period;
        }
        batch[i] = static_cast<float>(value);
    }

    return batch;
}

//
// #############################################################################
//

auto VoltageControlledOscillator::sin_batch(const float frequency, const std::chrono::nanoseconds& t) -> Batch {
    Batch batch;
    const float term = 2 * M_PI * frequency;

    std::chrono::nanoseconds ns = t;
    for (size_t i = 0; i < batch.size(); ++i, ns += synth::Samples::kSampleIncrement) {
        batch[i] = std::sin(term * std::chrono::duration<float>(ns).count());
    }
    return batch;
}
}  // namespace object::blocks
