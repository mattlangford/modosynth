#include "ecs/entity.hh"

namespace ecs {

//
// #############################################################################
//

Entity Entity::spawn() { return {counter_++}; }
Entity Entity::spawn_with(Entity::Id id) {
    counter_ = id + 1;
    return {id};
}

//
// #############################################################################
//

const Entity::Id& Entity::id() const { return id_; }

//
// #############################################################################
//

Entity::Entity(Entity::Id id) : id_(id) {}

//
// #############################################################################
//

Entity::Id Entity::counter_ = 0;

}  // namespace ecs
