#pragma once

#include <Eigen/Dense>
#include <optional>
#include <string>
#include <vector>

#include "ecs/components.hh"
#include "ecs/entity.hh"
#include "objects/catenary.hh"

namespace objects {

struct Transform {
    std::optional<ecs::Entity> parent;
    Eigen::Vector2f from_parent;
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

//
// #############################################################################
//

inline Eigen::Vector2f world_position(const Transform& tf, ComponentManager& manager) {
    if (!tf.parent) return tf.from_parent;
    const auto& parent_tf = manager.get<TexturedBox>(*tf.parent).bottom_left;
    return tf.from_parent + world_position(parent_tf, manager);
}
}  // namespace objects
