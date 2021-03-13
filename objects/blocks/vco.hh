#pragma once

#include "synth/node.hh"

namespace object::blocks {
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

    void invoke(const synth::Context& context, const Inputs& inputs, Outputs& outputs) const override;

    using Batch = std::array<float, synth::Samples::kBatchSize>;

    static Batch compute_batch(const Shape shape, const float frequency, const std::chrono::nanoseconds& t);

private:
    static Batch constant_batch(float value);

    static Batch square_batch(const float frequency, const std::chrono::nanoseconds& t);

    static Batch sin_batch(const float frequency, const std::chrono::nanoseconds& t);
};
}  // namespace object::blocks
