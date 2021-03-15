#pragma once
#include <array>
#include <chrono>
#include <iostream>
#include <string>

namespace synth {

constexpr uint64_t kSamplesRate = 44000;
constexpr bool kDebug = false;

struct Samples {
    Samples(float fill = 0.f) { std::fill(samples.begin(), samples.end(), fill); }

    static constexpr size_t kBatchSize = 512;
    static constexpr std::chrono::nanoseconds kSampleIncrement{1'000'000'000 / kSamplesRate};
    static constexpr std::chrono::nanoseconds kBatchIncrement{kBatchSize * kSampleIncrement};
    static_assert(kSampleIncrement.count() > 1);
    std::array<float, kBatchSize> samples;

    ///
    /// @brief Populate the samples array with a generator. The function should take the sample number within the batch
    ///
    template <typename F>
    void populate_samples(F f) {
        for (size_t i = 0; i < kBatchSize; ++i) samples[i] = f(i);
    }

    void sum(const Samples& rhs, float weight = 1.0) {
        for (size_t i = 0; i < kBatchSize; ++i) samples[i] += weight * rhs.samples[i];
    }
    void combine(float weight, const Samples& rhs, float rhs_weight) {
        for (size_t i = 0; i < kBatchSize; ++i) samples[i] = weight * samples[i] + rhs_weight * rhs.samples[i];
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
    void set_value(float value) { value_ = value; };
    float get_value() const { return value_; }

    const std::string& name() const { return name_; }

public:
    virtual size_t num_inputs() const = 0;
    virtual size_t num_outputs() const = 0;

    virtual void add_input(size_t input_index) = 0;
    virtual void set_input(size_t index, const Samples& input) = 0;
    virtual Samples get_output(size_t index) const = 0;

    // Return if it was ready to run
    virtual bool invoke(const Context& context) = 0;

private:
    std::string name_;
    float value_ = 0.0;
};

///
/// @brief Node which can inject values into the graph
///
class InjectorNode : GenericNode {
public:
    using GenericNode::GenericNode;
    ~InjectorNode() override = default;

    size_t num_inputs() const final { return 0; }
    size_t num_outputs() const final { return 1; }

    // always ready
    bool invoke(const Context&) final { return true; }

    void add_input(size_t) final { throw std::runtime_error("InjectorNode::add_input()"); }
    void set_input(size_t, const Samples&) final { throw std::runtime_error("InjectorNode::set_input()"); };
    Samples get_output(size_t) const final { return Samples{value_.load()}; }

public:
    void set_value(float value) { value_.store(value); }

private:
    std::atomic<float> value_;
};

///
/// @brief Node which can output values from the graph
///
// class EjectorNode : GenericNode
// {
// public:
//     using GenericNode::GenericNode;
//     ~EjectorNode() override = default;
//
//     size_t num_inputs() const final { return 1; }
//     size_t num_outputs() const final { return 0; }
//
//     bool invoke(const Context&) final {
//         if () return;
//     }
//
//     void add_input(size_t) final { default_counter++; }
//     void set_input(size_t, const Samples& data) final {
//         input_counter_--;
//
//         if (input_)
//             input_->sum(data);
//         else
//             input_ = data;
//     };
//     Samples get_output(size_t) final { throw std::runtime_error("EjectorNode::get_output()"); }
// public:
//     void flush_buffer();
//
// private:
//     int default_counter_ = 0;
//     int input_counter_ = 0;
//     std::optional<Samples> input_ = std::nullopt;
//
//     std::chrono::nanoseconds timestamp_;
//     std::vector<Samples> buffer_;
// };

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

    bool invoke(const Context& context) final {
        if (kDebug) std::cerr << name() << "::invoke()\n";
        if (!ready()) {
            if (kDebug) std::cerr << name() << "::invoke() not ready\n";
            return false;
        }

        // Pass to the user implemented function
        invoke(context, next_inputs_, outputs_);

        // Reset the internal state
        counters_ = initial_counters_;
        next_inputs_ = {};
        return true;
    }

    void set_input(size_t input_index, const Samples& incoming_samples) final {
        if (kDebug)
            std::cerr << name() << "::set_input(input_index=" << input_index << ") counter: " << counters_[input_index]
                      << "\n";

        auto& next_input = next_inputs_[input_index];
        for (size_t i = 0; i < next_input.samples.size(); ++i) {
            // Sum new inputs with the current inputs
            next_input.samples[i] += incoming_samples.samples[i];
        }

        counters_[input_index] -= 1;
    }

    Samples get_output(size_t index) const final { return outputs_[index]; }

protected:
    virtual void invoke(const Context&, const Inputs& inputs, Outputs& outputs) const {
        return invoke(inputs, outputs);
    }
    virtual void invoke(const Inputs&, Outputs&) const {};

private:
    bool ready() const {
        for (auto& count : counters_) {
            if (kDebug) std::cerr << name() << "::ready() count:" << count << "\n";

            if (count < 0) throw std::runtime_error(name() + "::ready() found a negative counter!");

            if (count != 0) return false;
        }
        return true;
    }

private:
    std::array<int, kInputs> initial_counters_;
    std::array<int, kInputs> counters_;
    Inputs next_inputs_;
    Outputs outputs_;
};
}  // namespace synth
