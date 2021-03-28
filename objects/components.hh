#pragma once

#include <Eigen/Dense>
#include <filesystem>
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
struct Removeable {
    std::vector<ecs::Entity> childern;
};

struct CableNode {
    bool source;
    size_t index;

    static CableNode make_source(size_t i) { return {true, i}; }
    static CableNode make_sink(size_t i) { return {false, i}; }

    inline bool is_source() const { return source; }
    inline bool is_sink() const { return !source; }
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

    enum Type : uint8_t { kKnob = 0, kButton = 1 };
    Type type;
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

using ComponentManager = ecs::ComponentManager<TexturedBox, Moveable, Selectable, CableNode, Cable, SynthNode,
                                               SynthInput, SynthOutput, SynthConnection, Removeable>;

Eigen::Vector2f world_position(const Transform& tf, const ComponentManager& manager);

SynthConnection connection_from_cable(const Cable& cable, const ComponentManager& manager);

void save(const std::filesystem::path& path, const ComponentManager& manager);

void load(const std::filesystem::path& path, ComponentManager& manager);
}  // namespace objects
