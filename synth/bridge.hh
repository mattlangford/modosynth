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
        streams_["/speaker"] = std::make_unique<Stream>();
    }
    ~Bridge() { stop_processing_thread(); }

    // When buffering stream data, we'll make sure the output has at least this much data
    static constexpr std::chrono::nanoseconds kOutputBufferTime = std::chrono::milliseconds(15);
    static_assert(Samples::batches_from_time(kOutputBufferTime) > 0);

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
                if (!stream_ptr) stream_ptr = std::make_unique<Stream>();
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

    void process(const size_t batches) {
        debug("Generating " << batches << " batches");
        // This is not generally thread safe, but here I'm only using it as a hint if there are things to do or not.
        if (!queued_values_.empty()) {
            // Lock and swap the queue
            std::queue<std::pair<size_t, float>> local_queued_values;
            {
                std::scoped_lock lock{mutex_};
                std::swap(local_queued_values, queued_values_);
            }

            debug("Swapped queued, size: " << local_queued_values.size());

            // Push new values for all of the injector nodes
            while (!local_queued_values.empty()) {
                auto& [index, value] = local_queued_values.front();
                if (auto it = injectors_.find(index); it != injectors_.end()) {
                    it->second->set_value(value);
                }
                local_queued_values.pop();
            }
        }

        // Every call to next() will increase the number of buffered batches by 1. If there are enough batches,
        // this loop will be skipped entirely.
        {
            std::scoped_lock lock{mutex_};
            for (size_t batch = 0; batch < batches; ++batch) {
                runner_.next(timestamp_);
                timestamp_ += Samples::kBatchIncrement;
            }
        }

        // Now we can flush all of the streams which will populate them
        flush_streams();
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
            // Lets check out much buffered data we've got so far, if we've got enough still buffered then we don't
            // need to process any further
            size_t min_buffered_samples = -1;
            for (const auto& [name, stream] : streams_) {
                min_buffered_samples = std::min(min_buffered_samples, stream->output().size());
            }
            const size_t min_buffered_batches = min_buffered_samples / Samples::kBatchSize;

            constexpr size_t kOutputBufferSize = Samples::batches_from_time(kOutputBufferTime);
            if (min_buffered_batches > kOutputBufferSize) {
                std::this_thread::sleep_for(0.3 * kOutputBufferTime);
                continue;
            }

            const size_t batches_to_generate = kOutputBufferSize - min_buffered_batches;
            process(batches_to_generate);
        }
    }

    void flush_streams() {
        for (const auto& [name, stream] : streams_) {
            // Flush the stream if the current output buffer size is too small
            if (stream->output().size() < Samples::samples_from_time(kOutputBufferTime)) {
                stream->flush();
            }
        }
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
