#include <GLFW/glfw3.h>

#include <iostream>

#include "bitmap.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

void init_view() {
    // set up view
    // glViewport(0, 0, kWidth, kHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // this creates a canvas to do 2D drawing on
    glOrtho(0.0, kWidth, 0.0, kHeight, 0.0, 1.0);
}

int main() {
    Bitmap bitmap{"/Users/mlangford/Downloads/sample_640×426.bmp"};
    auto pixesl = bitmap.get_pixels();

    GLFWwindow *window;

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

    init_view();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glRasterPos2i(100, 300);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glDrawPixels(bitmap.get_width(), bitmap.get_height(), GL_BGR, GL_UNSIGNED_BYTE, bitmap.get_pixels().data());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();

    // Exit program
    exit(EXIT_SUCCESS);
}
