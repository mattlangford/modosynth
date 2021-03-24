#pragma once
#include "objects/components.hh"
#include "objects/events.hh"

namespace objects {
class Bridge {
public:
    ComponentManager& components_manager() { return components_; };
    const ComponentManager& components_manager() const { return components_; };
    EventManager& events_manager() { return events_; };
    const EventManager& events_manager() const { return events_; };

private:
    ComponentManager components_;
    EventManager events_;
};
}  // namespace objects
