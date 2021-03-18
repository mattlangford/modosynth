#pragma once
#include <cstdint>

namespace ecs {
class Entity {
public:
    using Id = uint16_t;
    static Entity spawn();
    static Entity spawn_with(Id id);

    const Id& id() const;

private:
    static Id counter_;

    Entity(Id id);
    Id id_;
};
}  // namespace ecs
