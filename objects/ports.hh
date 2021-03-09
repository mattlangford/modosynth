#pragma once
#include <Eigen/Dense>
#include <vector>

#include "engine/buffer.hh"
#include "engine/object_manager.hh"
#include "engine/pool.hh"
#include "engine/texture.hh"

namespace objects {

struct BlockObject;

struct PortsObject {
    engine::ObjectId pool_id;

    size_t element_index;

    // The offsets from the parent object to each port represented by this object
    const std::vector<Eigen::Vector2f> offsets;

    // The parent block object this belongs to
    const BlockObject& parent_block;

    static PortsObject from_block(const BlockObject& parent);
};

//
// #############################################################################
//

static constexpr float kPortWidth = 3.0;   // in px
static constexpr float kPortHeight = 3.0;  // in px
static constexpr float kHalfPortWidth = 0.5 * kPortWidth;
static constexpr float kHalfPortHeight = 0.5 * kPortHeight;

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

    const engine::AbstractObjectPool<PortsObject>& pool() const;

private:
    std::unique_ptr<engine::AbstractObjectPool<PortsObject>> pool_;

    int object_position_loc_;

    engine::Buffer<float> buffer_;

    engine::Texture texture_;
};
}  // namespace objects
