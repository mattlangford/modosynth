#include "objects/components.hh"

namespace objects {
//
// #############################################################################
//

Eigen::Vector2f world_position(const Transform& tf, ComponentManager& manager) {
    if (!tf.parent) return tf.from_parent;
    const auto& parent_tf = manager.get<TexturedBox>(*tf.parent).bottom_left;
    return tf.from_parent + world_position(parent_tf, manager);
}

//
// #############################################################################
//

SynthConnection connection_from_cable(const Cable& cable, ComponentManager& manager) {
    const auto& start = cable.start.parent.value();
    ecs::Entity from = manager.get<TexturedBox>(start).bottom_left.parent.value();
    size_t from_port = manager.get<CableSource>(start).index;

    const auto& end = cable.end.parent.value();
    ecs::Entity to = manager.get<TexturedBox>(end).bottom_left.parent.value();
    size_t to_port = manager.get<CableSink>(end).index;

    return {from, from_port, to, to_port};
}
}  // namespace objects
