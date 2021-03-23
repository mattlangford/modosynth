#pragma once

#include <vector>

#include "ecs/entity.hh"
#include "ecs/events.hh"

namespace objects {

class Factory;

struct Spawn {
    std::vector<ecs::Entity> entities;
    const Factory* factory;  // factory used to spawn these entities
};
struct Despawn {
    ecs::Entity entity;
};
struct Connect {
    ecs::Entity entity;
};

using EventManager = ecs::EventManager<Spawn, Despawn, Connect>;

}  // namespace objects
