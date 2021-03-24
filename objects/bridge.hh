#pragma once
#include <chrono>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "objects/components.hh"
#include "objects/events.hh"
#include "synth/node.hh"
#include "synth/runner.hh"
#include "synth/samples.hh"

namespace objects {
class Bridge {
public:
    Bridge(const BlockLoader& loader)
        : loader_(loader), audio_buffer_(synth::Samples::kSampleRate), previous_(Clock::now()){};

public:
    ComponentManager& component_manager() { return component_; };
    const ComponentManager& component_manager() const { return component_; };
    EventManager& event_manager() { return event_; };
    const EventManager& event_manager() const { return event_; };

    synth::ThreadSafeBuffer& audio_buffer() { return audio_buffer_; }

public:
    void process() {
        // TODO it'd be easy to add an needs_update type bool to skip doing this if we don't need to
        auto previous_wrappers = std::move(wrappers_);
        component_.run_system<SynthNode>(
            [&](const ecs::Entity&, const SynthNode& node) { add_node(node, previous_wrappers); });

        // Update values for all the input wrappers
        component_.run_system<SynthInput>(
            [&](const ecs::Entity&, const SynthInput& input) { update_node_value(input); });

        const auto now = Clock::now();
        auto duration = now - previous_;
        // runner_.run_for(duration, flatten_wrappers(nodes_));
        previous_ = now;

        // TODO Right now this assumes there's at max one speaker which breaks down pretty quickly
        bool any_flushed = false;
        component_.run_system<SynthOutput>([&](const ecs::Entity&, const SynthOutput& output) {
            flush_output(output);
            any_flushed = true;
        });
        if (!any_flushed) flush_empty(duration);
    }

private:
    struct NodeWrapper {
        size_t id;
        std::unique_ptr<synth::GenericNode> node;

        using InputAndNode = std::pair<size_t, synth::GenericNode*>;
        std::vector<std::vector<InputAndNode>> outputs;
    };
    using Wrappers = std::unordered_map<size_t, NodeWrapper>;

    void add_node(const SynthNode& node, Wrappers& previous) {
        auto& wrapper = wrappers_[node.id];
        wrapper.id = node.id;
        wrapper.node = from_previous_or_spawn(node, previous);
        wrapper.outputs.resize(wrapper.node->num_outputs());
    }

    void update_node_value(const SynthInput& input) {
        auto& wrapper = wrapper_from_node(input.parent);
        // TODO probably could get rid of this dynamic cast, but that'd add complexity
        synth::InjectorNode& injector = *dynamic_cast<synth::InjectorNode*>(wrapper.node.get());
        injector.set_value(input.value);
    }

    void add_connection(const SynthConnection& connection) {
        auto& from = wrapper_from_node(connection.from);
        auto& outputs = from.outputs;

        if (outputs.size() < connection.from_port)
            throw std::runtime_error("Not enough outputs defined to add connection.");

        auto& wrapper = wrapper_from_node(connection.to);
        outputs[connection.from_port].push_back({connection.to_port, wrapper.node.get()});
    }

    void flush_output(const SynthOutput& output) {
        auto& wrapper = wrapper_from_node(output.parent);
        // TODO probably could get rid of this dynamic cast, but that'd add complexity
        synth::EjectorNode& ejector = *dynamic_cast<synth::EjectorNode*>(wrapper.node.get());
        for (auto sample : ejector.stream().flush_new()) audio_buffer_.push(sample);
    }

    void flush_empty(const std::chrono::nanoseconds& duration) {
        for (size_t i = 0; i < synth::Samples::samples_from_time(duration); ++i) audio_buffer_.push(0.f);
    }

    ///
    /// @brief Load the node from the previous node map or generate it from scratch
    ///
    std::unique_ptr<synth::GenericNode> from_previous_or_spawn(const SynthNode& node, Wrappers& previous_wrappers) {
        auto it = previous_wrappers.find(node.id);
        if (it == previous_wrappers.end()) {
            return loader_.get(node.name).spawn_synth_node();
        }

        auto& [_, wrapper] = *it;
        if (wrapper.node == nullptr)
            throw std::runtime_error("Found a nullptr in previous wrappers. Maybe there as a duplicate ID?");
        return std::move(wrapper.node);
    }

    NodeWrapper& wrapper_from_node(const ecs::Entity& entity) {
        auto& wrapper = wrappers_[component_.get<SynthNode>(entity).id];
        if (wrapper.node == nullptr) throw std::runtime_error("Found a nullptr when updating node values.");
        return wrapper;
    }

private:
    const BlockLoader& loader_;
    ComponentManager component_;
    EventManager event_;

    synth::Runner runner_;

    synth::ThreadSafeBuffer audio_buffer_;

    std::unordered_map<size_t, NodeWrapper> wrappers_;
    // TODO Stream support
    // std::unordered_map<std::string, synth::Stream> streams_;

    using Clock = std::chrono::steady_clock;
    Clock::time_point previous_;
};
}  // namespace objects
