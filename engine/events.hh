#pragma once

#include <Eigen/Dense>
#include <functional>
#include <optional>

struct GLFWwindow;

namespace engine {

struct MouseEvent {
    // Mouse state information
    bool clicked = false;
    bool was_clicked = false;
    inline bool pressed() const { return clicked && !was_clicked; }
    inline bool released() const { return !clicked && was_clicked; }
    inline bool held() const { return clicked && was_clicked; }

    // Absolute position/scroll of the mouse
    Eigen::Vector2f mouse_position = Eigen::Vector2f::Zero();
    float scroll = 0.0;

    // Relative position/scroll of the mouse since the last event
    Eigen::Vector2f delta_position = Eigen::Vector2f::Zero();
    float delta_scroll = 0.0;

    // Special modifier keys
    bool control = false;
    bool shift = false;
    bool right = false;
    inline bool any_modifiers() const { return control || shift || right; }
};

class MouseEventManager {
public:
    using CallbackFunction = std::function<void(const MouseEvent&)>;

public:
    MouseEventManager(CallbackFunction callback);
    ~MouseEventManager();

    void scroll_callback(GLFWwindow* window, double scroll_x, double scroll_y);
    void cursor_position_callback(GLFWwindow* window, double pos_x, double pos_y);
    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

private:
    CallbackFunction callback_;

    bool is_clicked_ = false;
    bool right_ = false;
    bool control_ = false;
    bool shift_ = false;
    float scroll_ = 0.0;
    Eigen::Vector2f position_ = Eigen::Vector2f::Zero();
    std::optional<Eigen::Vector2f> previous_position_;
};

//
// #############################################################################
//

struct KeyboardEvent {
    // Keyboard state information
    char key = '\0';
    bool clicked = false;
    bool was_clicked = false;
    inline bool pressed() const { return clicked && !was_clicked; }
    inline bool released() const { return !clicked && was_clicked; }
    inline bool held() const { return clicked && was_clicked; }

    // Special modifier keys
    bool control = false;
    bool shift = false;
    bool enter = false;
    bool space = false;
    bool tab = false;
    bool backspace = false;
    bool right_arrow = false;
    bool left_arrow = false;
    bool up_arrow = false;
    bool down_arrow = false;
    bool escape = false;
    inline bool any_modifiers() const {
        return control || shift || enter || space || tab || backspace || right_arrow || left_arrow || up_arrow ||
               down_arrow || escape;
    }
};

class KeyboardEventManager {
public:
    using CallbackFunction = std::function<void(const KeyboardEvent&)>;

public:
    KeyboardEventManager(CallbackFunction callback);
    ~KeyboardEventManager();

    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    CallbackFunction callback_;
    bool was_clicked = false;
};

void setup_glfw_callbacks(GLFWwindow* window);
}  // namespace engine
