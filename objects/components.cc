#include "objects/components.hh"

namespace objects {
//
// #############################################################################
//

Eigen::Vector2f world_position(const Transform& tf, const ComponentManager& manager) {
    if (!tf.parent) return tf.from_parent;
    const auto& parent_tf = manager.get<TexturedBox>(*tf.parent).bottom_left;
    return tf.from_parent + world_position(parent_tf, manager);
}

//
// #############################################################################
//

SynthConnection connection_from_cable(const Cable& cable, const ComponentManager& manager) {
    const auto& start = cable.start.parent.value();
    auto [from_box, from_cable] = manager.get<TexturedBox, CableSource>(start);
    const ecs::Entity& from = from_box.bottom_left.parent.value();
    const size_t& from_port = from_cable.index;

    const auto& end = cable.end.parent.value();
    auto [to_box, to_cable] = manager.get<TexturedBox, CableSource>(end);
    const ecs::Entity& to = to_box.bottom_left.parent.value();
    const size_t& to_port = to_cable.index;

    return {from, from_port, to, to_port};
}
}  // namespace objects
