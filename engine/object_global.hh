#pragma once
#include "engine/object_manager.hh"
#include "engine/objects/blocks.hh"

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

    size_t add_manager(std::shared_ptr<AbstractObjectManager> manager);
    std::shared_ptr<AbstractObjectManager> get_manager(size_t index) const;

private:
    std::vector<std::shared_ptr<AbstractObjectManager>> managers_;
};
}  // namespace engine
