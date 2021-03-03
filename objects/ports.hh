#pragma once
#include <Eigen/Dense>
#include <vector>

#include "engine/buffer.hh"
#include "engine/object_manager.hh"
#include "engine/pool.hh"

namespace objects {

struct BlockObject;

struct PortsObject {
    engine::ObjectId pool_id;

    size_t buffer_id;

    // The offsets from the parent object to each port represented by this object
    const std::vector<Eigen::Vector2f> offsets;

    // The parent block object this belongs to
    const BlockObject& parent_block;

    static PortsObject from_block(const BlockObject& parent);
};

//
// #############################################################################
//

class PortsObjectManager final : public engine::AbstractSingleShaderObjectManager {
public:
    PortsObjectManager();
    virtual ~PortsObjectManager() = default;

protected:
    void init_with_vao() override;
    void render_with_vao() override;

public:
    void update(float dt) override;
    void handle_mouse_event(const engine::MouseEvent& event) override;
    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

public:
    void spawn_object(PortsObject object_);
    void despawn_object(const engine::ObjectId& id);

private:
    std::unique_ptr<engine::AbstractObjectPool<PortsObject>> pool_;

    int object_position_loc_;

    engine::Buffer2Df buffer_;
};
}  // namespace objects
