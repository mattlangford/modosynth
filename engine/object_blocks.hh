#pragma once
#include <Eigen/Dense>

#include "engine/object_manager.hh"
#include "engine/pool.hh"

namespace engine {

//
// #############################################################################
//

struct BlockObject {
    ObjectId id;
    size_t texture_id;

    Eigen::Vector2d top_left;
    Eigen::Vector2d dims;

    inline Eigen::Vector2d get_top_left() const { return top_left; }
    inline Eigen::Vector2d get_bottom_right() const { return top_left + dims; }
};

//
// #############################################################################
//

class BlockObjectManager : AbstractObjectManager {
public:
    BlockObjectManager() = default;
    virtual ~BlockObjectManager() = default;

    void spawn_object(BlockObject object_);

    void despawn_object(const ObjectId& id);

public:
    void init() override;

    void render(const Eigen::Matrix3f& screen_from_world) override;

    void update(float dt) override;

    void handle_mouse_event(const MouseEvent& event) override;

    void handle_keyboard_event(const KeyboardEvent& event) override;

private:
    std::unique_ptr<AbstractObjectPool<BlockObject>> pool_;
};
}  // namespace engine
