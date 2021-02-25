#include <GLFW/glfw3.h>

#include <Eigen/Dense>
#include <cmath>
#include <iostream>

#include "bitmap.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

void init_view() {
    // set up view
    // glViewport(-100, -100, kWidth + 100, kHeight + 100);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // this creates a canvas to do 2D drawing on
    glOrtho(0.0, kWidth, kHeight, 0.0, 0.0, 1.0);
}

struct PanAndZoom {
    void update_mouse_position(double x, double y) {
        Eigen::Vector2d position(x, y);
        update_mouse_position_incremental(position - previous_position);
        previous_position = position;

        update_mouse_position();
    }

    void update_mouse_position_incremental(Eigen::Vector2d increment) {
        if (clicked) {
            center -= scale() * increment;
        }
    }

    void update_scroll(double /*x*/, double y) {
        double zoom_factor = -y;
        Eigen::Vector2d new_half_dim = half_dim + zoom_factor * half_dim;

        if (new_half_dim.x() < kMinHalfDim.x() || new_half_dim.y() < kMinHalfDim.y()) {
            new_half_dim = kMinHalfDim;
        } else if (new_half_dim.x() > kMaxHalfDim.x() || new_half_dim.y() > kMaxHalfDim.y()) {
            new_half_dim = kMaxHalfDim;
        }

        double translate_factor = new_half_dim.norm() / half_dim.norm() - 1;
        center += translate_factor * (center - mouse_position);
        half_dim = new_half_dim;

        update_mouse_position();
    }

    void click() { clicked = true; }

    void release() { clicked = false; }

    void set_model_view_matrix() {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        Eigen::Vector2d top_left = center - half_dim;
        Eigen::Vector2d bottom_right = center + half_dim;
        glOrtho(top_left.x(), bottom_right.x(), bottom_right.y(), top_left.y(), 0.0, 1.0);

        glBegin(GL_QUADS);
        glColor3d(1.0, 0.0, 0.0);
        glVertex2d(mouse_position.x() - 5, mouse_position.y() - 5);
        glVertex2d(mouse_position.x() + 5, mouse_position.y() - 5);
        glVertex2d(mouse_position.x() + 5, mouse_position.y() + 5);
        glVertex2d(mouse_position.x() - 5, mouse_position.y() + 5);
        glEnd();
    }

    void reset() {
        center = kInitialHalfDim;
        half_dim = kInitialHalfDim;
        update_mouse_position();
    }

    double scale() const { return (half_dim).norm() / kInitialHalfDim.norm(); }

    void update_mouse_position() {
        Eigen::Vector2d top_left = center - half_dim;
        Eigen::Vector2d bottom_right = center + half_dim;
        Eigen::Vector2d screen_position{previous_position.x() / kWidth, previous_position.y() / kHeight};
        mouse_position = screen_position.cwiseProduct(bottom_right - top_left) + top_left;
    }

    const Eigen::Vector2d kInitialHalfDim = 0.5 * Eigen::Vector2d{kWidth, kHeight};
    const Eigen::Vector2d kMinHalfDim = 0.5 * Eigen::Vector2d{0.1 * kWidth, 0.1 * kHeight};
    const Eigen::Vector2d kMaxHalfDim = 0.5 * Eigen::Vector2d{3.0 * kWidth, 3.0 * kHeight};

    Eigen::Vector2d center = kInitialHalfDim;
    Eigen::Vector2d half_dim = kInitialHalfDim;

    Eigen::Vector2d previous_position;

    Eigen::Vector2d mouse_position;

    bool clicked = false;
};

static PanAndZoom pan_and_zoom{};

void scroll_callback(GLFWwindow* /*window*/, double scroll_x, double scroll_y) {
    pan_and_zoom.update_scroll(scroll_x, scroll_y);
}

void cursor_position_callback(GLFWwindow* /*window*/, double pos_x, double pos_y) {
    pan_and_zoom.update_mouse_position(pos_x, pos_y);
}

void mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int /*mods*/) {
    if (button != GLFW_MOUSE_BUTTON_RIGHT) return;

    if (action == GLFW_PRESS) {
        pan_and_zoom.click();
    } else if (action == GLFW_RELEASE) {
        pan_and_zoom.release();
    }
}

void key_callback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS) pan_and_zoom.reset();
}

int main() {
    Bitmap bitmap{"/Users/mlangford/Downloads/sample_640×426.bmp"};
    auto pixesl = bitmap.get_pixels();

    GLFWwindow* window;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(kWidth, kHeight, "Window", NULL, NULL);
    if (!window) {
        std::cerr << "Unable to create window!\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    init_view();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        pan_and_zoom.set_model_view_matrix();

        glBegin(GL_LINES);
        glColor3d(1, 1, 1);
        glVertex2d(100.0, 100.0);
        glVertex2d(200.0, 200.0);
        glVertex2d(100.0, 200.0);
        glVertex2d(200.0, 100.0);
        glEnd();

        // glRasterPos2d(pan_x, pan_y);
        // glPixelZoom(zoom, zoom);
        // glPixelStorei(GL_PACK_ALIGNMENT, 1);
        // glDrawPixels(bitmap.get_width(), bitmap.get_height(), GL_BGR, GL_UNSIGNED_BYTE, bitmap.get_pixels().data());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();

    // Exit program
    exit(EXIT_SUCCESS);
}
