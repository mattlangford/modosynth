#pragma once
#include <Eigen/Dense>
#include <memory>

#include "engine/events.hh"

namespace engine {
class AbstractObjectManager {
public:
    virtual ~AbstractObjectManager() = default;

    ///
    /// @brief Initialize the object manager
    ///
    virtual void init() = 0;

    ///
    /// @brief Render objects held by the manager
    ///
    virtual void render(const Eigen::Matrix3f& screen_from_world) = 0;

    ///
    /// @brief Update all of the objects held by the manager
    ///
    virtual void update(float dt) = 0;

    ///
    /// @brief Update mouse/keyboard events for each object
    ///
    virtual void handle_mouse_event(const MouseEvent& event) = 0;
    virtual void handle_keyboard_event(const KeyboardEvent& event) = 0;
};
}  // namespace engine
