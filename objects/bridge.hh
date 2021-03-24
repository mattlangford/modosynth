#pragma once
#include <memory>
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
    void handle_spawn(const Spawn& spawn);
    void handle_undo_spawn(const Spawn& spawn);
    void handle_connect(const Connect& spawn);
    void handle_undo_connect(const Connect& spawn);

    void buffer_samples();

private:
    ComponentManager components_;
    EventManager events_;

    std::vector<std::unique_ptr<GenericNode>> nodes_;
};
}  // namespace objects
