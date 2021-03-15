#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include "synth/buffer.hh"
#include "synth/node.hh"
#include "synth/runner.hh"
#include "synth/samples.hh"
#include "synth/stream.hh"

namespace synth {

struct Identifier {
    size_t id;
    size_t port;
};
class Bridge {
public:
    Bridge() = default;
    ~Bridge() { stop_processing_thread(); }

    // When rolling back due to an input change, we'll go back at max this far
    inline static constexpr std::chrono::nanoseconds kMaxRollbackFadeTime{std::chrono::milliseconds{1}};

    // When buffering stream data, we'll make sure the output has at least this much time populated
    inline static constexpr std::chrono::milliseconds kMinOutputStreamBufferTime{1};

    // When updating the stream input buffers, we'll make sure they have at least this much time buffered
    inline static constexpr std::chrono::milliseconds kMinInputStreamBufferTime{5};

public:
    using NodeFactory = std::function<std::unique_ptr<GenericNode>()>;

    void add_factory(const std::string& name, NodeFactory factory) { factories_[name] = std::move(factory); }

    NodeFactory* get_factory(const std::string& name) {
        auto it = factories_.find(name);
        return it == factories_.end() ? nullptr : &it->second;
    }

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
                auto& stream_ptr = streams_[ptr->stream_name()];
                if (!stream_ptr) stream_ptr = std::make_unique<Stream>(kMaxRollbackFadeTime);
                ptr->set_stream(*stream_ptr);
            }

            return id;
        }
        throw std::runtime_error(name + " factory not found!");
    }

    void connect(const Identifier& from, const Identifier& to) {
        std::lock_guard lock(mutex_);
        runner_.connect(from.id, from.port, to.id, to.port);
    }

    void set_value(size_t index, float value) { queued_values_.emplace(index, value); }

    ThreadSafeBuffer& get_stream_output(const std::string& stream_name) {
        if (auto it = streams_.find(stream_name); it != streams_.end()) return streams_[stream_name]->output();
        throw std::runtime_error(std::string("Unable to find stream: ") + stream_name);
    }

    void start_processing_thread() {
        shutdown_ = false;
        processing_thread_ = std::thread([this]() { process_thread(); });
    }
    void stop_processing_thread() {
        shutdown_ = true;
        if (processing_thread_.joinable()) processing_thread_.join();
    }

    void process() {
        debug("timestamp: " << timestamp_.count());
        constexpr size_t kMinInputStreamBufferSize = kMinInputStreamBufferTime / Samples::kBatchIncrement;

        size_t min_buffered_batches = flush_stream_if_needed();

        debug("min_buffered_batches: " << min_buffered_batches);

        // If there are any values queued up we'll need to send the relevant values out to the injectors. Queue::empty()
        // is not generally thread safe, but here I'm only using it as a hint if there are things to do or not.
        if (!queued_values_.empty()) {
            // Lock and swap the queue
            std::queue<std::pair<size_t, float>> local_queued_values;
            {
                std::scoped_lock lock{mutex_};
                std::swap(local_queued_values, queued_values_);
            }

            debug("swapped queued, size: " << local_queued_values.size());

            while (!local_queued_values.empty()) {
                auto& [index, value] = local_queued_values.front();
                if (auto it = injectors_.find(index); it != injectors_.end()) {
                    it->second->set_value(value);
                }
                local_queued_values.pop();
            }

            // Revert back a few batches so that we can fade between
            const std::chrono::nanoseconds min_buffered_time = min_buffered_batches * Samples::kBatchIncrement;
            auto rollback_time = std::min(kMaxRollbackFadeTime, min_buffered_time);
            timestamp_ -= rollback_time;
            debug("new timestamp" << timestamp_);
        }

        // Every call to next() will increase the number of buffered batches by 1. If there are enough batches,
        // this loop will be skipped entirely
        for (; min_buffered_batches < kMinInputStreamBufferSize; min_buffered_batches++) {
            runner_.next(timestamp_);
            timestamp_ += Samples::kBatchIncrement;
        }
    }

private:
    void process_thread() {
        while (!shutdown_) {
            process();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    size_t flush_stream_if_needed() {
        // Keep 1ms buffered
        constexpr size_t kMinOutputStreamBufferSize = kMinOutputStreamBufferTime / Samples::kSampleIncrement;

        size_t min_buffered_batches = 0;
        for (const auto& [name, stream] : streams_) {
            // Flush the stream if the current output buffer size is too small
            if (stream->output().size() < kMinOutputStreamBufferSize) {
                stream->flush_samples(kMinOutputStreamBufferTime);
            }
            debug("After flushing, stream: " << name << " has output size: " << stream->output.size()
                                             << " and buffered size: " << stream->buffered_batches());

            // Then compute the number of generated batches left in the stream
            min_buffered_batches = std::min(min_buffered_batches, stream->buffered_batches());
        }
        return min_buffered_batches;
    }

private:
    bool shutdown_ = true;
    std::thread processing_thread_;

    // Guards the runner and the queued values
    std::mutex mutex_;
    Runner runner_;
    std::queue<std::pair<size_t, float>> queued_values_;

    std::chrono::nanoseconds timestamp_{0};

    std::unordered_map<std::string, NodeFactory> factories_;

    std::unordered_map<size_t, InjectorNode*> injectors_;

    std::unordered_map<std::string, std::unique_ptr<Stream>> streams_;

    // std::vector<EjectorNode*> ejectors_;
};

}  // namespace synth
