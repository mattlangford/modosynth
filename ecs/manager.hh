#pragma once
#include <assert.h>

#include <bitset>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ecs/entity.hh"
#include "ecs/events.hh"
#include "ecs/utils.hh"

namespace ecs {

template <typename... T>
struct Components {};
template <typename... T>
struct Events {};

///
/// @brief Events the manager will emit when an object is spawned or destroyed
///
struct Spawn {
    const Entity& entity;
};
struct Despawn {
    const Entity& entity;
    std::vector<size_t> components;  // as a vector since we don't know size info here
};

//
// #############################################################################
//

template <typename Components, typename Events>
class Manager;

template <typename... Component, typename... Event>
class Manager<Components<Component...>, Events<Event...>> {
public:
public:
    template <typename... C>
    Entity spawn_with(C... components) {
        auto entity = component_manager().spawn_with(std::move(components)...);
        event_manager().trigger(Spawn{entity});
        return entity;
    }

    void despawn(const Entity& entity) {
        // Trigger the event before removing, since handlers may require component data
        event_manager().trigger(Despawn{entity});
        component_manager().despawn(entity);
    }

    void undo_spawn(const Spawn& spawn) { despawn(spawn.entity); }
    void undo_despawn(const Spawn& spawn) { despawn(spawn.entity); }

    auto& event_manager() { return events_; };
    auto& component_manager() { return components_; };

private:
    ComponentManager<Component...> components_;
    EventManager<Spawn, Despawn, Event...> events_;
};
}  // namespace ecs
