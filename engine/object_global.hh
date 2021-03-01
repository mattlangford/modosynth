#pragma once
#include "engine/object_blocks.hh"
#include "engine/object_manager.hh"

namespace engine {
class GlobalObjectManager : AbstractObjectManager {
public:
    GlobalObjectManager() = default;
    ~GlobalObjectManager() override = default;

public:
    void init() override;

    void render(const Eigen::Matrix3f& screen_from_world) override;

    void update(float dt) override;

    void handle_mouse_event(const MouseEvent& event) override;

    void handle_keyboard_event(const KeyboardEvent& event) override;

private:
    std::tuple<BlockObjectManager> managers_;
};
}  // namespace engine
