#include "engine/window.hh"

namespace engine {

//
// #############################################################################
//

void scroll_callback(GLFWwindow* /*window*/, double scroll_x, double scroll_y) {
    Window::instance().update_scroll(scroll_x, scroll_y);
}

//
// #############################################################################
//

void cursor_position_callback(GLFWwindow* /*window*/, double pos_x, double pos_y) {
    Window::instance().update_mouse_position(pos_x, pos_y);
}

//
// #############################################################################
//

void mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int /*mods*/) {
    if (button != GLFW_MOUSE_BUTTON_RIGHT) return;

    if (action == GLFW_PRESS) {
        Window::instance().set_clicked(true);
    } else if (action == GLFW_RELEASE) {
        Window::instance().set_clicked(false);
    }
}
//
// #############################################################################
//

void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        Window::instance().reset();
    else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

//
// #############################################################################
//

Window* Window::instance_ = nullptr;

//
// #############################################################################
//

Window::Window(size_t height, size_t width)
    : height_(height),
      width_(width),
      kInitialHalfDim_{0.5 * Eigen::Vector2f{width_, height_}},
      kMinHalfDim_{0.5 * Eigen::Vector2f{0.1 * width_, 0.1 * height_}},
      kMaxHalfDim_{0.5 * Eigen::Vector2f{3.0 * width_, 3.0 * height_}},
      center_{kInitialHalfDim_},
      half_dim_{kInitialHalfDim_} {
    if (instance_) {
        throw std::runtime_error("Window already created!");
    }
    instance_ = this;
}

//
// #############################################################################
//

Window::~Window() {
    instance_ = nullptr;
    glfwDestroyWindow(window_);
    glfwTerminate();
}

//
// #############################################################################
//

Window* Window::instance_ptr() { return instance_; }
Window& Window::instance() {
    if (!instance_) throw std::runtime_error("No window instance set!");
    return *instance_;
}

//
// #############################################################################
//

void Window::init() {
    glfwSetErrorCallback(
        [](int code, const char* desc) { std::cerr << "GLFW Error (" << code << "):" << desc << "\n"; });

    if (!glfwInit()) exit(EXIT_FAILURE);

    // Run with OpenGL version 4.1, the other hints are needed to create the window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window_ = glfwCreateWindow(width_, height_, "Window", NULL, NULL);
    if (!window_) {
        std::cerr << "Unable to create window_!\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window_);

    glfwSetCursorPosCallback(window_, cursor_position_callback);
    glfwSetMouseButtonCallback(window_, mouse_button_callback);
    glfwSetScrollCallback(window_, scroll_callback);
    glfwSetKeyCallback(window_, key_callback);

    std::cout << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";

    gl_safe(glEnable, GL_BLEND);
    gl_safe(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//
// #############################################################################
//

bool Window::render_loop() {
    gl_safe(glClear, GL_COLOR_BUFFER_BIT);
    gl_safe(glClearColor, 0.1f, 0.2f, 0.2f, 1.0f);

    // Eigen::Matrix3f screen_from_world = get_screen_from_world();

    // gl_safe(glUseProgram, program_);
    // gl_safe(glUniformMatrix3fv, screen_from_world_loc, 1, GL_FALSE, screen_from_world.data());
    // gl_safe(glBindVertexArray, vertex_array);
    // gl_safe(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);

    glfwSwapBuffers(window_);
    glfwPollEvents();
    return !glfwWindowShouldClose(window_);
}

//
// #############################################################################
//

void Window::update_mouse_position(double x, double y) {
    Eigen::Vector2f position(x, y);
    update_mouse_position_incremental(position - previous_position__);
    previous_position__ = position;

    update_mouse_position();
}

//
// #############################################################################
//

void Window::update_mouse_position_incremental(Eigen::Vector2f increment) {
    if (clicked_) {
        double current_scale = scale();
        center_.x() -= current_scale * increment.x();
        center_.y() -= current_scale * increment.y();
    }
}

//
// #############################################################################
//

void Window::update_scroll(double /*x*/, double y) {
    double zoom_factor = 0.1 * -y;
    Eigen::Vector2f new_half_dim_ = half_dim_ + zoom_factor * half_dim_;

    if (new_half_dim_.x() < kMinHalfDim_.x() || new_half_dim_.y() < kMinHalfDim_.y()) {
        new_half_dim_ = kMinHalfDim_;
    } else if (new_half_dim_.x() > kMaxHalfDim_.x() || new_half_dim_.y() > kMaxHalfDim_.y()) {
        new_half_dim_ = kMaxHalfDim_;
    }

    double translate_factor = new_half_dim_.norm() / half_dim_.norm() - 1;
    center_ += translate_factor * (center_ - mouse_position);
    half_dim_ = new_half_dim_;

    update_mouse_position();
}

//
// #############################################################################
//

void Window::set_clicked(bool clicked) { clicked_ = clicked; }

//
// #############################################################################
//

Eigen::Matrix3f Window::get_screen_from_world() const {
    using Transform = Eigen::Matrix3f;

    Transform translate = Transform::Identity();
    Transform scale = Transform::Identity();

    translate(0, 2) = -center_.x();
    translate(1, 2) = -center_.y();
    scale.diagonal() = Eigen::Vector3f{1 / half_dim_.x(), -1 / half_dim_.y(), 1};

    return scale * translate;
}

//
// #############################################################################
//

void Window::reset() {
    center_ = kInitialHalfDim_;
    half_dim_ = kInitialHalfDim_;
    update_mouse_position();
}

//
// #############################################################################
//

double Window::scale() const { return (half_dim_).norm() / kInitialHalfDim_.norm(); }

//
// #############################################################################
//

void Window::update_mouse_position() {
    Eigen::Vector2f top_left = center_ - half_dim_;
    Eigen::Vector2f bottom_right = center_ + half_dim_;
    Eigen::Vector2f screen_position{previous_position__.x() / width_, previous_position__.y() / height_};
    mouse_position = screen_position.cwiseProduct(bottom_right - top_left) + top_left;
}
}  // namespace engine
