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

namespace objects {
class Bridge {
public:
    Bridge(const BlockLoader& loader) : loader_(loader){};

public:
    ComponentManager& components_manager() { return components_; };
    const ComponentManager& components_manager() const { return components_; };
    EventManager& events_manager() { return events_; };
    const EventManager& events_manager() const { return events_; };

public:
    void update() { auto now = Clock::now(); }

private:
    struct NodeWrapper {
        size_t id;
        std::unique_ptr<synth::GenericNode> node;

        using InputAndNode = std::pair<size_t, synth::GenericNode*>;
        std::vector<std::vector<InputAndNode>> outputs;
    };
    using Nodes = std::unordered_map<size_t, NodeWrapper>;

    void add_node(const SynthNode& node, Nodes& previous) {
        auto& wrapper = nodes_[node.id];
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

    void push_output(const ecs::Entity& entity, const SynthOutput& output) {}

    ///
    /// @brief Load the node from the previous node map or generate it from scratch
    ///
    std::unique_ptr<synth::GenericNode> from_previous_or_spawn(const SynthNode& node, Nodes& previous_nodes) {
        auto it = previous_nodes.find(node.id);
        if (it == previous_nodes.end()) {
            return loader_.get(node.name).spawn_synth_node();
        }

        auto& [_, wrapper] = *it;
        if (wrapper.node == nullptr)
            throw std::runtime_error("Found a nullptr in previous nodes. Maybe there as a duplicate ID?");
        return std::move(wrapper.node);
    }

    NodeWrapper& wrapper_from_node(const ecs::Entity& entity) {
        auto& wrapper = nodes_[components_.get<SynthNode>(entity).id];
        if (wrapper.node == nullptr) throw std::runtime_error("Found a nullptr when updating node values.");
        return wrapper;
    }

private:
    const BlockLoader& loader_;
    ComponentManager components_;
    EventManager events_;

    std::unordered_map<size_t, NodeWrapper> nodes_;

    using Clock = std::chrono::steady_clock;
    Clock::time_point previous_;
};
}  // namespace objects
