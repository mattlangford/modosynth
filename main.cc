#include <GLFW/glfw3.h>

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
        current_x = x;
        current_y = y;

        if (clicked)
        {
            center_dx += current_x - previous_x;
            center_dy += current_y - previous_y;
            center_x += current_x - previous_x;
            center_y += current_y - previous_y;
        }
    }

    void click() {
        previous_x = current_x;
        previous_y = current_y;
        clicked = true;
    }

    void release() { clicked = false; }

    void update_scroll(double /*x*/, double y) {
        zoom_x = current_x - center_x;
        zoom_y = current_y - center_y;

        dzoom += y;
        current_zoom += y;
    }

    void set_model_view_matrix() {
        glMatrixMode(GL_MODELVIEW);
        glTranslatef(center_dx, center_dy, 0.0);

        // Compute the zoom matrix
        constexpr double kMiddleX = kWidth / 2;
        constexpr double kMiddleY = kHeight / 2;
        double x = zoom_x - kMiddleX;
        double y = zoom_y - kMiddleY;

        if (dzoom != 0)
        {
        }

        glBegin(GL_POINTS);
        glColor3d(1, 0, 0);
        for (int i = -5; i < 5; ++i)
            for (int j = -5; j < 5; ++j)
                glVertex2d(zoom_x + i, zoom_y + j);
        glEnd();

        previous_x = current_x;
        previous_y = current_y;
        previous_zoom = current_zoom;
        center_dx = 0;
        center_dy = 0;
        dzoom = 0;
    }

    double center_x = 0;
    double center_y = 0;
    double center_dx = 0;
    double center_dy = 0;
    double dzoom = 0;

    double current_x = 0;
    double current_y = 0;
    double current_zoom = 0;

    double previous_x = 0;
    double previous_y = 0;
    double previous_zoom = 0;

    double zoom_x = 0;
    double zoom_y = 0;

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
