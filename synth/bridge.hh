#pragma once
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>

#include "synth/buffer.hh"
#include "synth/debug.hh"
#include "synth/node.hh"
#include "synth/runner.hh"
#include "synth/samples.hh"

namespace synth {

struct Identifier {
    size_t id;
    size_t port;
};

class Stream {
public:
    Stream() : raw_samples_(100, true), output_(std::make_unique<ThreadSafeBuffer>(Samples::kSampleRate)) {}

    void add_samples(const std::chrono::nanoseconds& timestamp, const Samples& samples) {
        if (timestamp <= end_time_) {
            // Update the element instead of adding a new one. Average the current sample with the new one.
            raw_samples_[index_of_timestamp(timestamp)].combine(0.5, samples.samples, 0.5);
            return;
        }

        end_time_ = timestamp;  // store the timestamp of the start of this batch
        raw_samples_.push(samples);
    }

    void flush_samples(const std::chrono::nanoseconds& amount_to_flush) {
        for (size_t i = 0; i < static_cast<size_t>(amount_to_flush / Samples::kBatchIncrement); ++i) {
            auto element = raw_samples_.pop();
            std::cout << "Popping : " << i << " value? " << element.has_value() << "\n";
            if (!element) {
                default_flush();
                continue;
            }
            for (auto sample : element->samples) {
                output_->push(sample);
            }
        }
    }

    size_t index_of_timestamp(const std::chrono::nanoseconds& timestamp) {
        if (raw_samples_.empty() || !end_time_) throw std::runtime_error("Can't get index without adding samples.");

        // The start time of the oldest element in the buffer
        std::chrono::nanoseconds start_time = *end_time_ - Samples::kBatchIncrement * (raw_samples_.size() - 1);

        std::cout << "start_time: " << start_time.count() << " end_time_: " << end_time_->count()
                  << ", t=" << timestamp.count() << "\n";

        if (timestamp < start_time) throw std::runtime_error("Asking for timestamp before start of buffer.");

        // 0 will be the oldest entry, raw_samples_.size() - 1 will be the latest
        return (timestamp - start_time) / Samples::kBatchIncrement;
    }

    ThreadSafeBuffer& buffer() { return *output_; }

private:
    void default_flush() {
        throttled(1.0, "Warning: Stream::flush_samples() with end time past end of buffer. Padding with 0s.");
        for (size_t el = 0; el < Samples::kBatchSize; ++el) output_->push(0);
    }

private:
    // The most recent samples timestamp (other timestamps are calculated with respect to this
    std::optional<std::chrono::nanoseconds> end_time_;
    Buffer<Samples> raw_samples_;

    // Kept as a pointer so that the address is always valid
    std::unique_ptr<ThreadSafeBuffer> output_;
};

class Bridge {
public:
    Bridge() = default;
    ~Bridge() {
        shutdown_ = true;
        if (processing_.joinable()) processing_.join();
    }

public:
    using NodeFactory = std::function<std::unique_ptr<GenericNode>()>;

    void add_factory(const std::string& name, NodeFactory factory) { factories_[name] = std::move(factory); }

public:
    size_t spawn(const std::string& name) {
        if (auto* f_ptr = get_factory(name)) {
            std::unique_ptr<GenericNode> node = (*f_ptr)();
            GenericNode* node_ptr = node.get();
            size_t id = runner_.spawn(std::move(node));

            // We'll save input and output nodes here
            if (auto ptr = dynamic_cast<InjectorNode*>(node_ptr)) {
                injectors_[id] = ptr;
            } else if (auto ptr = dynamic_cast<EjectorNode*>(node_ptr)) {
                ejectors_.push_back(ptr);
            }

            return id;
        }
        throw std::runtime_error(name + " factory not found!");
    }

    void connect(const Identifier& from, const Identifier& to) { runner_.connect(from.id, from.port, to.id, to.port); }

    void next() {
        runner_.next(timestamp_);
        timestamp_ += Samples::kBatchIncrement;
    }

    void set_value(size_t index, float value) {
        if (auto it = injectors_.find(index); it != injectors_.end()) {
            it->second->set_value(value);
        }
    }

    NodeFactory* get_factory(const std::string& name) {
        auto it = factories_.find(name);
        return it == factories_.end() ? nullptr : &it->second;
    }

    ThreadSafeBuffer& get_stream_buffer(const std::string& stream_name) {
        if (auto it = streams_.find(stream_name); it != streams_.end()) return streams_[stream_name].buffer();
        throw std::runtime_error(std::string("Unable to find stream: ") + stream_name);
    }

private:
    void processing() {
        while (!shutdown_) {
        }
    }

    void update_streams() {}

private:
    bool shutdown_;
    std::thread processing_;

    Runner runner_;
    std::chrono::nanoseconds timestamp_{0};

    std::unordered_map<std::string, NodeFactory> factories_;

    std::unordered_map<size_t, InjectorNode*> injectors_;
    std::vector<EjectorNode*> ejectors_;  // we don't need to index here

    std::unordered_map<std::string, Stream> streams_;

    // std::vector<EjectorNode*> ejectors_;
};

}  // namespace synth
