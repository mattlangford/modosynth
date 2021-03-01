#include "engine/events.hh"

#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>

namespace engine {

//
// #############################################################################
//

static MouseEventManager* mouse_instance = nullptr;
static KeyboardEventManager* keyboard_instance = nullptr;

//
// #############################################################################
//

MouseEventManager::MouseEventManager(CallbackFunction callback) : callback_(std::move(callback)) {
    if (mouse_instance) throw std::runtime_error("Trying to create multiple MouseEventManager. Only one allowed.");
    mouse_instance = this;
}

//
// #############################################################################
//

MouseEventManager::~MouseEventManager() { mouse_instance = nullptr; }

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

KeyboardEventManager::KeyboardEventManager(CallbackFunction callback) : callback_(std::move(callback)) {
    if (keyboard_instance)
        throw std::runtime_error("Trying to create multiple KeyboardEventManagers. Only one allowed.");
    keyboard_instance = this;
}

//
// #############################################################################
//

KeyboardEventManager::~KeyboardEventManager() { keyboard_instance = nullptr; }

//
// #############################################################################
//

void KeyboardEventManager::key_callback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int mods) {
    KeyboardEvent event;
    event.was_clicked = was_clicked;

    event.clicked = action == GLFW_PRESS;
    event.control = (mods | GLFW_MOD_CONTROL) != 0;
    event.shift = (mods | GLFW_MOD_SHIFT) != 0;

    event.enter = false;
    event.space = false;
    event.backspace = false;
    event.tab = false;
    event.right_arrow = false;
    event.left_arrow = false;
    event.up_arrow = false;
    event.down_arrow = false;
    event.escape = false;

    switch (key) {
        case GLFW_KEY_ENTER:
            event.enter = true;
            break;
        case GLFW_KEY_SPACE:
            event.space = true;
            break;
        case GLFW_KEY_TAB:
            event.tab = true;
            break;
        case GLFW_KEY_BACKSPACE:
            event.backspace = true;
            break;
        case GLFW_KEY_RIGHT:
            event.right_arrow = true;
            break;
        case GLFW_KEY_LEFT:
            event.left_arrow = true;
            break;
        case GLFW_KEY_UP:
            event.up_arrow = true;
            break;
        case GLFW_KEY_DOWN:
            event.down_arrow = true;
            break;
        case GLFW_KEY_ESCAPE:
            event.escape = true;
            break;
        default:
            if (key > 32 && key < 100) event.key = std::tolower(key);
            break;
    }

    callback_(event);

    was_clicked = event.clicked;
}

//
// #############################################################################
//

void setup_glfw_callbacks(GLFWwindow* window) {
    glfwSetScrollCallback(window, [](GLFWwindow* w, double x, double y) {
        if (mouse_instance) mouse_instance->scroll_callback(w, x, y);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
        if (mouse_instance) mouse_instance->cursor_position_callback(w, x, y);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods) {
        if (mouse_instance) mouse_instance->mouse_button_callback(w, button, action, mods);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
        if (keyboard_instance) keyboard_instance->key_callback(w, key, scancode, action, mods);
    });
}
}  // namespace engine
