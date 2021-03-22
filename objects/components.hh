#pragma once

#include <Eigen/Dense>
#include <optional>
#include <vector>

#include "ecs/components.hh"
#include "ecs/entity.hh"

namespace objects {
struct TexturedBox;
struct Transform {
    std::optional<ecs::Entity> parent;
    Eigen::Vector2f from_parent;

    template <typename... Components>
    Eigen::Vector2f world_position(ecs::ComponentManager<Components...>& manager) const {
        if (!parent) return from_parent;

        const auto& parent_tf = manager.template get<TexturedBox>(*parent).bottom_left;
        return from_parent + parent_tf.world_position(manager);
    }
};
struct TexturedBox {
    Transform bottom_left;
    Eigen::Vector2f dim;
    Eigen::Vector2f uv;
    size_t texture_index;
};

struct Moveable {
    Eigen::Vector2f position;
    bool snap_to_pixel = true;
};
struct Selectable {
    bool selected = false;
};

struct CableSource {
    size_t index;
};
struct CableSink {
    size_t index;
};
struct Cable {
    Transform start;
    Transform end;

    objects::CatenarySolver solver;

    // A sort of cache so we don't have recompute the catenary
    Eigen::Vector2f previous_start;
    Eigen::Vector2f previous_end;
};

struct SynthNode {
    std::string name;
    size_t id;
};

using ComponentManager =
    ecs::ComponentManager<TexturedBox, Moveable, Selectable, CableSource, CableSink, Cable, SynthNode>;

}  // namespace objects
