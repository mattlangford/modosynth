#include "engine/window.hh"

namespace engine {

//
// #############################################################################
//

Window::Window(size_t width, size_t height, GlobalObjectManager object_manager)
    : mouse_([this](const MouseEvent& event) { handle_mouse_event(event); }),
      keyboard_([this](const KeyboardEvent& event) { handle_keyboard_event(event); }),
      kWindowDim{width, height},
      kInitialHalfDim_{0.5 * Eigen::Vector2f{width, height}},
      kMinHalfDim_{0.5 * Eigen::Vector2f{0.1 * width, 0.1 * height}},
      kMaxHalfDim_{0.5 * Eigen::Vector2f{2.0 * width, 2.0 * height}},
      center_{kInitialHalfDim_},
      half_dim_{kInitialHalfDim_},
      object_manager_(std::move(object_manager)) {}

//
// #############################################################################
//

Window::~Window() {
    glfwDestroyWindow(window_);
    glfwTerminate();
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
    window_ = glfwCreateWindow(kWindowDim.x(), kWindowDim.y(), "Window", NULL, NULL);
    if (!window_) {
        std::cerr << "Unable to create window_!\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window_);

    setup_glfw_callbacks(window_);

    std::cout << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";

    gl_safe(glEnable, GL_BLEND);
    gl_safe(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    object_manager_.init();
}

//
// #############################################################################
//

bool Window::render_loop() {
    gl_safe(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_safe(glClearColor, 0.1f, 0.2f, 0.2f, 1.0f);

    object_manager_.render(get_screen_from_world());

    glfwSwapBuffers(window_);
    glfwPollEvents();
    return !glfwWindowShouldClose(window_);
}

//
// #############################################################################
//

void Window::handle_mouse_event(const MouseEvent& event) {
    // Since the scale changes, changing the relative movement of the cursor makes things seem more natural
    MouseEvent scaled_event = event;
    scaled_event.delta_position *= scale();

    // Get the mouse position on the pan'd/zoom'd screen
    const Eigen::Vector2f screen_position = event.mouse_position.cwiseQuotient(kWindowDim);
    const Eigen::Vector2f top_left = center_ - half_dim_;
    scaled_event.mouse_position = screen_position.cwiseProduct(2 * half_dim_) + top_left;

    if (event.right && event.clicked) {
        center_ -= scaled_event.delta_position;
    }

    if (event.delta_scroll != 0.0) {
        double zoom_factor = 0.05 * -event.delta_scroll;
        Eigen::Vector2f new_half_dim_ = half_dim_ + zoom_factor * half_dim_;

        if (new_half_dim_.x() < kMinHalfDim_.x() || new_half_dim_.y() < kMinHalfDim_.y()) {
            new_half_dim_ = kMinHalfDim_;
        } else if (new_half_dim_.x() > kMaxHalfDim_.x() || new_half_dim_.y() > kMaxHalfDim_.y()) {
            new_half_dim_ = kMaxHalfDim_;
        }

        double translate_factor = new_half_dim_.norm() / half_dim_.norm() - 1;
        center_ -= translate_factor * (scaled_event.mouse_position - center_);
        half_dim_ = new_half_dim_;
    }

    object_manager_.handle_mouse_event(scaled_event);
}

//
// #############################################################################
//

void Window::handle_keyboard_event(const KeyboardEvent& event) {
    if (event.escape) {
        glfwSetWindowShouldClose(window_, true);
    } else if (event.key == 'r') {
        reset();
    }

    object_manager_.handle_keyboard_event(event);
}

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
}

//
// #############################################################################
//

double Window::scale() const { return (half_dim_).norm() / kInitialHalfDim_.norm(); }

//
// #############################################################################
//

}  // namespace engine
