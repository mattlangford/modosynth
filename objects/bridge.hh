#pragma once
#include <memory>
#include <chrono>
#include <vector>

#include "objects/components.hh"
#include "objects/events.hh"
#include "synth/node.hh"
#include "synth/runner.hh"

namespace objects {
class Bridge {
public:
    ComponentManager& components_manager() { return components_; };
    const ComponentManager& components_manager() const { return components_; };
    EventManager& events_manager() { return events_; };
    const EventManager& events_manager() const { return events_; };

public:
    void update()
    {
        auto now = Clock::now();
    }

private:
    void update_node(const ecs::Entity& entity, const SynthNode& node)
    {
    }
    void update_node(const ecs::Entity& entity, const SynthInput& input)
    {
    }

    void update_connection(const ecs::Entity& entity, const Cable& cable)
    {
    }

    void push_output(const ecs::Entity& entity, const SynthOutput& output)
    {
    }

private:
    ComponentManager components_;
    EventManager events_;

    struct NodeWrapper
    {
        size_t id;

        std::unique_ptr<GenericNode> node;

        using InputAndNode = std::pair<size_t, GenericNode*>;
        std::vector<std::vector<InputAndNode>> outputs;
    };
    std::vector<NodeWrapper> nodes_;

    using Clock = std::chrono::steady_clock;
    Clock::time_point previous_;
};
}  // namespace objects
