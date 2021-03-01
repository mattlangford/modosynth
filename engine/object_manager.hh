#pragma once
#include <Eigen/Dense>
#include <memory>

namespace engine {

struct MouseEvent {
    // Mouse state information
    bool clicked;
    bool was_clicked;
    inline bool pressed() const { return clicked && !was_clicked; }
    inline bool released() const { return !clicked && was_clicked; }
    inline bool held() const { return clicked && was_clicked; }

    // Absolute position/scroll of the mouse
    Eigen::Vector2f mouse_position;
    float scroll;

    // Relative position/scroll of the mouse since the last event
    Eigen::Vector2f delta_position;
    float dscroll;

    // Special modifier keys
    bool control;
    bool shift;
};

struct KeyboardEvent {
    // Keyboard state information
    char key;
    bool clicked;
    bool was_clicked;
    inline bool pressed() const { return clicked && !was_clicked; }
    inline bool released() const { return !clicked && was_clicked; }
    inline bool held() const { return clicked && was_clicked; }

    // Special modifier keys
    bool control;
    bool shift;
};

//
// #############################################################################
//

class AbstractObjectManager {
public:
    virtual ~AbstractObjectManager() = 0;

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
