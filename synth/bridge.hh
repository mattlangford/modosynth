#pragma once
#include <functional>
#include <memory>
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

    ThreadSafeBuffer& get_stream_output(const std::string& stream_name) {
        if (auto it = streams_.find(stream_name); it != streams_.end()) return streams_[stream_name].output();
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
