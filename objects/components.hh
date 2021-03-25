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

    bool shift = false;
    bool control = false;

    static Selectable require_shift() { return {false, true, false}; }
    static Selectable require_control() { return {false, false, true}; }
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
};

struct SynthNode {
    size_t id = -1;
    std::string name = "unknown";
};
struct SynthInput {
    ecs::Entity parent;
    float value;
};
struct SynthOutput {
    ecs::Entity parent;
    std::string stream_name;

    // This is updated with the values from the previous synth cycle
    std::vector<float> samples;
};
struct SynthConnection {
    ecs::Entity from;
    size_t from_port;

    ecs::Entity to;
    size_t to_port;
};

using ComponentManager = ecs::ComponentManager<TexturedBox, Moveable, Selectable, CableSource, CableSink, Cable,
                                               SynthNode, SynthInput, SynthOutput, SynthConnection>;

//
// #############################################################################
//

Eigen::Vector2f world_position(const Transform& tf, const ComponentManager& manager);

//
// #############################################################################
//

SynthConnection connection_from_cable(const Cable& cable, const ComponentManager& manager);
}  // namespace objects
