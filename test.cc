#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <sstream>

#include "bitmap.hh"

void check_gl_error(std::string action = "") {
    std::stringstream ss;
    if (!action.empty()) ss << action << " failed. ";
    ss << "Error code: 0x" << std::hex;

    auto error = glGetError();
    switch (error) {
        case GL_NO_ERROR:
            return;
        case GL_INVALID_ENUM:
            ss << GL_INVALID_ENUM << " (GL_INVALID_ENUM).";
            break;
        case GL_INVALID_VALUE:
            ss << GL_INVALID_VALUE << " (GL_INVALID_VALUE).";
            break;
        default:
            ss << error;
            break;
    }
    throw std::runtime_error(ss.str());
}

int main() {
    Bitmap bitmap{"/Users/mlangford/Downloads/sample_640×426.bmp"};
    GLFWwindow* window;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        exit(EXIT_FAILURE);
    }

    // Upgrade to a newer GLSL version for the shaders
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(1000, 1000, "Window", NULL, NULL);
    if (!window) {
        std::cerr << "Unable to create window!\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    check_gl_error("window");

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bitmap.get_width(), bitmap.get_height(), 0, GL_BGR, GL_UNSIGNED_BYTE, bitmap.get_pixels().data());
    glEnable(GL_TEXTURE_2D);
    check_gl_error("Image mapping");

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 1000, 1000, 0, -1.0, 1.0);
        glGetError();

        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0, 0.0); glVertex2f(100, 100);
            glTexCoord2f(1.0, 0.0); glVertex2f(200, 100);
            glTexCoord2f(1.0, 1.0); glVertex2f(200, 200);
            glTexCoord2f(0.0, 1.0); glVertex2f(100, 200);
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
