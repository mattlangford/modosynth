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
    Bridge() {
        // TODO: This needs to be there so the audio loop works
        streams_["/speaker"] = std::make_unique<Stream>(kMaxRollbackFadeTime);
    }
    ~Bridge() { stop_processing_thread(); }

    // When rolling back due to an input change, we'll go back at max this far
    inline static constexpr auto kMaxRollbackFadeTime = Samples::time_from_batches(2);

    // When buffering stream data, we'll make sure the output has at least this much data
    static constexpr size_t kOutputBufferSize = 5;  // batches

    // When updating the stream input buffers, we'll make sure they have at least this much time buffered
    static constexpr size_t kInputBufferSizeSize = 20;  // batches

public:
    using NodeFactory = std::function<std::unique_ptr<GenericNode>()>;

    void add_factory(const std::string& name, NodeFactory factory) { factories_[name] = std::move(factory); }

    NodeFactory* get_factory(const std::string& name) {
        auto it = factories_.find(name);
        return it == factories_.end() ? nullptr : &it->second;
    }

public:
    size_t spawn(const std::string& name) {
        std::lock_guard lock(mutex_);
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
                std::cout << "Loaded stream: '" << ptr->stream_name() << "'" << std::endl;
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
        debug("============== Process timestamp: " << timestamp_);
        // Lets check out much buffered data we've got so far
        size_t min_buffered_batches = -1;
        for (const auto& [name, stream] : streams_) {
            min_buffered_batches = std::min(min_buffered_batches, stream->buffered_batches());
        }
        debug("min_buffered_batches: " << min_buffered_batches);

        // is not generally thread safe, but here I'm only using it as a hint if there are things to do or not.
        if (!queued_values_.empty()) {
            // Lock and swap the queue
            std::queue<std::pair<size_t, float>> local_queued_values;
            {
                std::scoped_lock lock{mutex_};
                std::swap(local_queued_values, queued_values_);
            }

            debug("Swapped queued, size: " << local_queued_values.size());

            while (!local_queued_values.empty()) {
                auto& [index, value] = local_queued_values.front();
                if (auto it = injectors_.find(index); it != injectors_.end()) {
                    it->second->set_value(value);
                }
                local_queued_values.pop();
            }

            // Revert back a few batches so that we can fade between
            const std::chrono::nanoseconds min_buffered_time = Samples::time_from_batches(min_buffered_batches);
            auto rollback_time = std::min(kMaxRollbackFadeTime, min_buffered_time);
            timestamp_ -= rollback_time;
            debug("New timestamp: " << timestamp_ << " rollback_time: " << rollback_time << " min("
                                    << kMaxRollbackFadeTime << ", " << min_buffered_time << ")");
        }

        // Every call to next() will increase the number of buffered batches by 1. If there are enough batches,
        // this loop will be skipped entirely
        {
            std::scoped_lock lock{mutex_};
            for (; min_buffered_batches < kInputBufferSizeSize; min_buffered_batches++) {
                runner_.next(timestamp_);
                timestamp_ += Samples::kBatchIncrement;
            }
        }

        // Now we can flush all of the streams
        flush_stream_if_needed();
    }

    void clear_streams() {
        std::lock_guard lock(mutex_);
        for (auto& [name, stream] : streams_) {
            stream->clear();
        }
    }

private:
    void process_thread() {
        while (!shutdown_) {
            process();
        }
    }

    size_t flush_stream_if_needed() {
        size_t min_buffered_batches = -1;
        for (const auto& [name, stream] : streams_) {
            constexpr size_t kOutputBufferSampleSize = kOutputBufferSize * Samples::kBatchSize;
            // Flush the stream if the current output buffer size is too small
            if (stream->output().size() < kOutputBufferSampleSize) {
                debug("Stream: " << name << " flushing " << kOutputBufferSize);
                stream->flush_batches(kOutputBufferSize);
            }

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
