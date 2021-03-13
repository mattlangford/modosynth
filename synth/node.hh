#pragma once
#include <array>
#include <chrono>
#include <iostream>
#include <string>

namespace synth {

constexpr uint64_t kSampleRate = 44000;
constexpr bool kDebug = false;

struct Samples {
    Samples() { std::fill(samples.begin(), samples.end(), 0.f); }

    static constexpr size_t kBatchSize = 5;
    static constexpr size_t kBatchNsIncrement = kBatchSize * 1'000'000'000 / (kSampleRate);
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

///
/// These functions are invoked directly by the runner
///
class GenericNode {
public:
    GenericNode(std::string name) : name_(name) {}
    virtual ~GenericNode() = default;

public:
    virtual size_t num_inputs() const = 0;
    virtual size_t num_outputs() const = 0;

    virtual void add_input(size_t input_index) = 0;
    virtual bool ready() const = 0;
    virtual void invoke(const Context& context) = 0;
    virtual void accept(size_t index, const Samples& incoming_samples) = 0;
    virtual void send(size_t output_index, size_t input_index, GenericNode& to) const = 0;
    virtual void set_value(float) = 0;

    const std::string& name() const { return name_; }

private:
    std::string name_;
};

template <size_t kInputs, size_t kOutputs>
class AbstractNode : public GenericNode {
public:
    using Inputs = std::array<Samples, kInputs>;
    using Outputs = std::array<Samples, kOutputs>;

public:
    AbstractNode(std::string name) : GenericNode(std::move(name)) {
        std::fill(initial_counters_.begin(), initial_counters_.end(), 0);
        counters_ = initial_counters_;
    }
    ~AbstractNode() override = default;

public:
    size_t num_inputs() const final { return kInputs; }
    size_t num_outputs() const final { return kOutputs; }

    void add_input(size_t input_index) final {
        if (kDebug) std::cerr << name() << "::add_input(input_index=" << input_index << ")\n";
        initial_counters_.at(input_index)++;
        counters_.at(input_index)++;
    }

    bool ready() const final {
        for (size_t i = 0; i < counters_.size(); ++i) {
            auto count = counters_[i];
            if (kDebug) std::cerr << name() << "::ready() count:" << count << "\n";

            if (count < 0) throw std::runtime_error(name() + "::ready() found a negative counter!");

            if (count != 0) return false;

            auto init_count = initial_counters_[i];
            if (count == 0 && init_count == 0) return false;  // there is an unconnected port
        }
        return true;
    }

    void invoke(const Context& context) final {
        if (kDebug) std::cerr << name() << "::invoke()\n";
        // Pass to the user implemented function
        invoke(context, next_inputs_, outputs_);

        // Reset the internal state
        counters_ = initial_counters_;
        next_inputs_ = {};
    }

    void accept(size_t input_index, const Samples& incoming_samples) final {
        if (kDebug)
            std::cerr << name() << "::accept(input_index=" << input_index << ") counter: " << counters_[input_index]
                      << "\n";

        if (counters_[input_index] == 0) {
            if (kDebug) std::cerr << name() << " is dropping new data.\n";
            return;
        }

        auto& next_input = next_inputs_[input_index];
        for (size_t i = 0; i < next_input.samples.size(); ++i) {
            // Sum new inputs with the current inputs
            next_input.samples[i] += incoming_samples.samples[i];
        }

        counters_[input_index] -= 1;
    }

    void send(size_t output_index, size_t input_index, GenericNode& to) const final {
        if (kDebug)
            std::cerr << name() << "::send(output_index=" << output_index << ", input_index=" << input_index
                      << ", to=" << to.name() << ")\n";
        to.accept(input_index, outputs_[output_index]);
    }

    // default to a no-op
    void set_value(float) override {}

protected:
    virtual void invoke(const Context&, const Inputs& inputs, Outputs& outputs) const {
        return invoke(inputs, outputs);
    }
    virtual void invoke(const Inputs&, Outputs&) const {};

private:
    std::array<int, kInputs> initial_counters_;
    std::array<int, kInputs> counters_;
    Inputs next_inputs_;
    Outputs outputs_;
};
}  // namespace synth
