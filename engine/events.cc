#include "engine/events.hh"

#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace engine {

//
// #############################################################################
//

MouseEventManager* MouseEventManager::instance_ = nullptr;

//
// #############################################################################
//

MouseEventManager::MouseEventManager(CallbackFunction callback) : callback_(std::move(callback)) {
    if (instance_) throw std::runtime_error("Trying to create another MouseEventManager. Only one allowed.");
    instance_ = this;
}

//
// #############################################################################
//

MouseEventManager::~MouseEventManager() { instance_ = nullptr; }

//
// #############################################################################
//

void MouseEventManager::scroll_callback(GLFWwindow* /* window */, double /* scroll_x */, double scroll_y) {
    MouseEvent event;

    // None of this can change in the scroll callback
    event.clicked = is_clicked_;
    event.was_clicked = is_clicked_;
    event.mouse_position = position_;
    event.delta_position = Eigen::Vector2f::Zero();
    event.control = control_;
    event.shift = shift_;
    event.right = right_;

    scroll_ += scroll_y;

    event.scroll = scroll_;
    event.delta_scroll = scroll_y;

    callback_(event);
}

//
// #############################################################################
//

void MouseEventManager::cursor_position_callback(GLFWwindow* /*window*/, double pos_x, double pos_y) {
    MouseEvent event;

    // None of this can change in the position callback
    event.clicked = is_clicked_;
    event.was_clicked = is_clicked_;
    event.control = control_;
    event.shift = shift_;
    event.scroll = scroll_;
    event.delta_scroll = 0;
    event.right = right_;

    position_ = Eigen::Vector2f{pos_x, pos_y};

    event.mouse_position = position_;
    event.delta_position =
        previous_position_ ? Eigen::Vector2f{event.mouse_position - *previous_position_} : Eigen::Vector2f::Zero();

    previous_position_ = position_;

    callback_(event);
}

//
// #############################################################################
//

void MouseEventManager::mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int mods) {
    MouseEvent event;

    // None of this can change in the position callback
    event.mouse_position = position_;
    event.scroll = scroll_;
    event.delta_position = Eigen::Vector2f::Zero();
    event.delta_scroll = 0;
    event.was_clicked = is_clicked_;

    is_clicked_ = action == GLFW_PRESS;
    right_ = button == GLFW_MOUSE_BUTTON_RIGHT;
    control_ = (mods | GLFW_MOD_CONTROL) != 0;
    shift_ = (mods | GLFW_MOD_SHIFT) != 0;

    event.clicked = is_clicked_;
    event.control = control_;
    event.shift = shift_;
    event.right = right_;

    callback_(event);
}

//
// #############################################################################
//

MouseEventManager* MouseEventManager::instance_ptr() { return instance_; }

//
// #############################################################################
//

KeyboardEventManager::KeyboardEventManager(CallbackFunction callback) : callback_(std::move(callback)) {}

//
// #############################################################################
//

void KeyboardEventManager::key_callback(GLFWwindow* /*window*/, int /*key*/, int /*scancode*/, int /*action*/,
                                        int /*mods*/) {}

//
// #############################################################################
//

void setup_glfw_callbacks(GLFWwindow* window) {
    glfwSetScrollCallback(window, [](GLFWwindow* w, double x, double y) {
        if (auto ptr = MouseEventManager::instance_ptr()) ptr->scroll_callback(w, x, y);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
        if (auto ptr = MouseEventManager::instance_ptr()) ptr->cursor_position_callback(w, x, y);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods) {
        if (auto ptr = MouseEventManager::instance_ptr()) ptr->mouse_button_callback(w, button, action, mods);
    });

    // glfwSetKeyCallback(window, key_callback);
}
}  // namespace engine
