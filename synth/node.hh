#pragma once
#include <array>
#include <chrono>
#include <iostream>
#include <string>

#include "synth///debug.hh"
#include "synth/samples.hh"
#include "synth/stream.hh"

namespace synth {

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
    const std::string& name() const { return name_; }

public:
    virtual size_t num_inputs() const = 0;
    virtual size_t num_outputs() const = 0;

    virtual void connect(size_t input_index) = 0;
    virtual void add_input(size_t index, const Samples& input) = 0;
    virtual Samples get_output(size_t index) const = 0;

    // Return if it was ready to run
    virtual bool invoke(const Context& context) = 0;

private:
    std::string name_;
};

///
/// @brief Node which can inject values into the graph
///
class InjectorNode : public GenericNode {
public:
    using GenericNode::GenericNode;
    ~InjectorNode() override = default;

    size_t num_inputs() const final { return 0; }
    size_t num_outputs() const final { return 1; }

    // always ready
    bool invoke(const Context&) final { return true; }

    void connect(size_t) final { throw std::runtime_error("InjectorNode::add_input()"); }
    void add_input(size_t, const Samples&) final { throw std::runtime_error("InjectorNode::set_input()"); };
    Samples get_output(size_t) const final { return Samples{value_.load()}; }

public:
    void set_value(float value) { value_.store(value); }

private:
    std::atomic<float> value_ = {0};
};

///
/// @brief Node which can output values from the graph
///
class EjectorNode : public GenericNode {
public:
    EjectorNode(std::string node_name, const std::string& stream_name)
        : GenericNode{node_name}, stream_name_(stream_name) {}
    using GenericNode::GenericNode;
    ~EjectorNode() override = default;

public:
    size_t num_inputs() const final { return 1; }
    size_t num_outputs() const final { return 0; }

    bool invoke(const Context& context) final {
        if (input_counter_ > 0) return false;
        input_counter_ = default_counter_;

        if (stream_) {
            stream_->add_samples(context.timestamp, samples_);
        }

        samples_ = Samples{0};

        return true;
    }

    void connect(size_t) final { default_counter_++; }
    void add_input(size_t, const Samples& data) final {
        input_counter_--;
        samples_.sum(data.samples);
    };
    Samples get_output(size_t) const final { throw std::runtime_error("EjectorNode::get_output()"); }

public:
    const std::string& stream_name() const { return stream_name_; }
    void set_stream(Stream& stream) { stream_ = &stream; }

private:
    const std::string stream_name_;

    int default_counter_ = 0;
    int input_counter_ = 0;

    Samples samples_;
    Stream* stream_ = nullptr;
};

//
// #############################################################################
//

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

    void connect(size_t input_index) final {
        // debug(name() << "::add_input(input_index=" << input_index << ")");
        initial_counters_.at(input_index)++;
        counters_.at(input_index)++;
    }

    bool invoke(const Context& context) final {
        // debug(name() << "::invoke()");

        if (!ready()) {
            // debug(name() << "::invoke() not ready");
            return false;
        }

        // Pass to the user implemented function
        invoke(context, next_inputs_, outputs_);

        // Reset the internal state
        counters_ = initial_counters_;
        next_inputs_ = {};
        return true;
    }

    void add_input(size_t input_index, const Samples& incoming_samples) final {
        // debug(name() << "::set_input(input_index=" << input_index << ") counter: " << counters_[input_index]);

        auto& next_input = next_inputs_[input_index];
        for (size_t i = 0; i < next_input.samples.size(); ++i) {
            // Sum new inputs with the current inputs
            next_input.samples[i] += incoming_samples.samples[i];
        }

        counters_[input_index] -= 1;
    }

    Samples get_output(size_t index) const final { return outputs_[index]; }

protected:
    virtual void invoke(const Context&, const Inputs& inputs, Outputs& outputs) { return invoke(inputs, outputs); }
    virtual void invoke(const Inputs&, Outputs&){};

private:
    bool ready() const {
        for (auto& count : counters_) {
            // debug(name() << "::ready() count:" << count);

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
