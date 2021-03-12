#pragma once
#include <array>
#include <chrono>

namespace synth {

constexpr uint64_t kSampleRate = 44000;

struct Samples {
    static constexpr size_t kBatchSize = 100;
    std::array<float, kBatchSize> samples;

    ///
    /// @brief Populate the samples array with a generator. The function should take the sample number within the batch
    ///
    template <typename F>
    void populate_samples(F f) {
        for (size_t i = 0; i < kBatchSize; ++i) samples[i] = f(i);
    }
};

struct Context {
    std::chrono::nanoseconds timestamp;
};

template <size_t kInputs, size_t kOutputs>
class AbstractNode {
public:
    using Inputs = std::array<Samples, kInputs>;
    using Outputs = std::array<Samples, kOutputs>;

public:
    ~AbstractNode() = default;

public:
    static constexpr size_t num_inputs() { return kInputs; }
    static constexpr size_t num_outputs() { return kOutputs; }

public:
    virtual void invoke(const Context&, const Inputs& inputs, Outputs& outputs) const {
        return invoke(inputs, outputs);
    }
    virtual void invoke(const Inputs&, Outputs&) const {};
    virtual void set_value(float) {}
};

//
// #############################################################################
//

class VoltageSourceNode : AbstractNode<0, 1> {
public:
    ~VoltageSourceNode() = default;

public:
    void invoke(const Context& context, const Inputs&, Outputs& outputs) const override {
        auto& [voltage] = outputs;
        std::fill(voltage.samples.begin(), voltage.samples.end(), value_);
    }

    virtual void set_value(float value) override { value_ = value; }

private:
    float value_ = 0.0;
};

}  // namespace synth
