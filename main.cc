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
        if (clicked)
        {
            double current_scale = scale();
            top_left -= increment * current_scale;
            bottom_right -= increment * current_scale;
        }
    }

    void click() {
        clicked = true;
    }

    void release() { clicked = false; }

    void update_scroll(double /*x*/, double y) {
        double current_scale = scale();
        std::cout << current_scale << "\n";
        if ((current_scale < 0.25 && y > 0) || (current_scale > 10.0 && y < 0))
        {
            return;
        }

        Eigen::Vector2d min_top_left = mouse_position - kMinDim / 2;
        Eigen::Vector2d min_bottom_right = mouse_position + kMinDim / 2;

        double zoom_factor = std::max(std::min(0.1 * y, 0.1), -0.1);
        top_left += zoom_factor * (min_top_left - top_left);
        bottom_right += zoom_factor * (min_bottom_right - bottom_right);

        update_mouse_position();
    }

    void set_model_view_matrix() {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        // this creates a canvas to do 2D drawing on
        double left = top_left.x();
        double right = bottom_right.x();
        double bottom = bottom_right.y();
        double top = top_left.y();
        glOrtho(left, right, bottom, top, 0.0, 1.0);

        glBegin(GL_POINTS);
        glColor3d(1, 0, 0);
        for (int i = -5; i < 5; ++i)
            for (int j = -5; j < 5; ++j)
                glVertex2d(mouse_position.x() + i, mouse_position.y() + j);
        glEnd();
    }

    void reset()
    {
        top_left = Eigen::Vector2d{0, 0};
        bottom_right = kInitalDim;
    }

    double scale() const
    {
        return (bottom_right - top_left).norm() / kInitalDim.norm();
    }

    void update_mouse_position()
    {
        Eigen::Vector2d screen_position{previous_position.x() / kWidth, previous_position.y() / kHeight};
        mouse_position = screen_position.cwiseProduct(bottom_right - top_left) + top_left;
    }


    const Eigen::Vector2d kInitalDim{kWidth, kHeight};
    const Eigen::Vector2d kMinDim{0.1 * kWidth, 0.1 * kHeight};
    const Eigen::Vector2d kMaxDim{2.0 * kWidth, 2.0 * kHeight};

    // Corners
    Eigen::Vector2d top_left {0, 0};
    Eigen::Vector2d bottom_right {kWidth, kHeight};

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

void key_callback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        pan_and_zoom.reset();
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
