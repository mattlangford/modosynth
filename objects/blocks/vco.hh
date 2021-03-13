#pragma once

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "synth/node.hh"

namespace object::blocks {
class VoltageControlledOscillator final : public synth::AbstractNode<2, 1> {
public:
    inline static const std::string kName = "Voltage Controlled Oscillator";

public:
    VoltageControlledOscillator(size_t count) : AbstractNode{kName + std::to_string(count)} {}

public:
    enum class Shape : uint8_t {
        kSin = 0,
        kSquare = 1,
        kMax = 2,
    };

    void invoke(const synth::Context& context, const Inputs& inputs, Outputs& outputs) const override {
        // Both of these are assumed to be constant during these samples
        const float frequency = inputs[0].samples[0];
        const float shape = inputs[1].samples[0];
        auto& output = outputs[0].samples;

        int discrete_shape = static_cast<int>(shape);
        float percent = shape - discrete_shape;

        const Shape shape0 = static_cast<Shape>(discrete_shape % static_cast<int>(Shape::kMax));
        const float mult0 = percent;
        const Shape shape1 = static_cast<Shape>((discrete_shape + 1) % static_cast<int>(Shape::kMax));
        const float mult1 = 1.0 - percent;

        output = compute_batch(shape0, frequency, context.timestamp);
        auto batch1 = compute_batch(shape1, frequency, context.timestamp);
        for (size_t i = 0; i < batch1.size(); ++i) {
            output[i] = mult0 * output[i] + mult1 * batch1[i];
        }
    }

    using Batch = std::array<float, synth::Samples::kBatchSize>;

    inline static Batch compute_batch(const Shape shape, const float frequency,
                                      const std::chrono::nanoseconds& timestamp) {
        Batch batch;
        switch (shape) {
            case Shape::kSin:
                batch = sin_batch(frequency, timestamp);
                break;
            case Shape::kSquare:
                batch = square_batch(frequency, timestamp);
                break;
            case Shape::kMax:
            default:
                batch = constant_batch(-1.0);  // error case, really
                break;
        }
        return batch;
    }

private:
    static Batch constant_batch(float value) {
        Batch batch;
        for (size_t i = 0; i < batch.size(); ++i) batch[i] = value;
        return batch;
    }

    inline static Batch square_batch(const float frequency, const std::chrono::nanoseconds& timestamp) {
        Batch batch;

        std::chrono::duration<float> half_period{1.0 / (2.0 * frequency)};
        std::chrono::duration<float> period{1.0 / frequency};

        std::chrono::nanoseconds mod_with{static_cast<int>(1E9 * period.count()) + 1};
        std::chrono::nanoseconds ns = timestamp % mod_with;
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

    static Batch sin_batch(const float frequency, const std::chrono::nanoseconds& timestamp) {
        Batch batch;
        const float term = 2 * M_PI * frequency;

        std::chrono::nanoseconds ns = timestamp;
        for (size_t i = 0; i < batch.size(); ++i, ns += synth::Samples::kSampleIncrement) {
            batch[i] = std::sin(term * std::chrono::duration<float>(ns).count());
        }
        return batch;
    }
};
}  // namespace object::blocks
