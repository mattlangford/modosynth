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
      world_from_screen_(get_screen_from_world().inverse()),
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
    window_ = glfwCreateWindow(kWindowDim.x(), kWindowDim.y(), "ModoSynth", NULL, NULL);
    if (!window_) {
        std::cerr << "Unable to create window_!\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window_);

    setup_glfw_callbacks(window_);

    std::cout << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";

    gl_check(glEnable, GL_BLEND);
    gl_check(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
    gl_check(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_check(glClearColor, 0.1f, 0.2f, 0.2f, 1.0f);

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

    // Monitor for changes in the viewport, this is an optimization so we can reuse the old data if nothing changes
    bool screen_change = false;

    // The mouse frame is reported from the top left of the screen with +Y going down. Here we get the mouse position
    // in the world. First normalize the mouse position to be in (0, 1)
    Eigen::Vector2f mouse = event.mouse_position.cwiseQuotient(kWindowDim);
    if (mouse.x() < 0.0 || mouse.x() >= 1.0 || mouse.y() < 0.0 || mouse.y() >= 1.0) {
        // off screen, don't worry about anything else
        return;
    }
    // Next scale to be between (-1, 1)
    mouse = 2.f * (mouse - Eigen::Vector2f{0.5f, 0.5f});

    // Then invert the Y axis, this makes it screen coordinates now: (-1, 1) at the top left, (0, 0) at the center,
    // and (1, -1) at the bottom right
    mouse.y() *= -1.f;
    const Eigen::Vector3f screen_mouse{mouse.x(), mouse.y(), 1.0f};
    const Eigen::Vector2f world_mouse = (world_from_screen_ * screen_mouse).head(2);
    const Eigen::Vector2f previous_world_mouse =
        (world_from_screen_ * previous_screen_mouse_.value_or(screen_mouse)).head(2);
    previous_screen_mouse_ = screen_mouse;

    // Update the positions to be in the "world" frame since those are more useful generally to the object managers. The
    // delta is re-computed with respect to the previous mouse position
    scaled_event.delta_position = world_mouse - previous_world_mouse;
    scaled_event.mouse_position = world_mouse;

    if (!event.control && !event.shift && event.right && event.clicked) {
        screen_change = true;
        center_ -= scaled_event.delta_position;
    }

    if (event.delta_scroll != 0.0) {
        screen_change = true;
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

    // If there was something that changed our view, update the screen_from_world matrix, This is an optimization so we
    // don't invert a matrix on every mouse event.
    if (screen_change) {
        world_from_screen_ = get_screen_from_world().inverse();
    }
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
    scale.diagonal() = Eigen::Vector3f{1 / half_dim_.x(), 1 / half_dim_.y(), 1};

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
