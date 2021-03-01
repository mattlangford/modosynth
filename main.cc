#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <cmath>
#include <iostream>
#include <list>

#include "engine/bitmap.hh"
#include "engine/gl.hh"
#include "engine/objects.hh"
#include "engine/shader.hh"
#include "engine/window.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

static engine::Window window_control{kHeight, kWidth};

void scroll_callback(GLFWwindow* /*window*/, double scroll_x, double scroll_y) {
    window_control.update_scroll(scroll_x, scroll_y);
}

void cursor_position_callback(GLFWwindow* /*window*/, double pos_x, double pos_y) {
    window_control.update_mouse_position(pos_x, pos_y);
}

void mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int /*mods*/) {
    if (button != GLFW_MOUSE_BUTTON_RIGHT) return;

    if (action == GLFW_PRESS) {
        window_control.click();
    } else if (action == GLFW_RELEASE) {
        window_control.release();
    }
}

void key_callback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS) window_control.reset();
}

typedef struct Vertex {
    float pos[2];
    float uv[2];
} Vertex;

int main() {
    engine::Bitmap bitmap{"/Users/mlangford/Downloads/test.bmp"};
    std::vector<Vertex> vertices;
    float x = 100;
    float y = 300;
    vertices.emplace_back(Vertex{{x + bitmap.get_width(), y}, {1.0, 1.0}});                        // top right
    vertices.emplace_back(Vertex{{x + bitmap.get_width(), y + bitmap.get_height()}, {1.0, 0.0}});  // bottom right
    vertices.emplace_back(Vertex{{x, y}, {0.0, 1.0}});                                             // top left
    vertices.emplace_back(Vertex{{x, y + bitmap.get_height()}, {0.0, 0.0}});                       // bottom left

    glfwSetErrorCallback(
        [](int code, const char* desc) { std::cerr << "GLFW Error (" << code << "):" << desc << "\n"; });

    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Window", NULL, NULL);
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

    std::cout << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";

    gl_safe(glEnable, GL_BLEND);
    gl_safe(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int program = link_shaders();

    const int screen_from_world_loc = glGetUniformLocation(program, "screen_from_world");
    const int world_position_loc = glGetAttribLocation(program, "world_position");
    const int vertex_uv_loc = glGetAttribLocation(program, "vertex_uv");

    unsigned int vertex_buffer;
    gl_safe(glGenBuffers, 1, &vertex_buffer);
    gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
    gl_safe(glBufferData, GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    unsigned int vertex_array;
    gl_safe(glGenVertexArrays, 1, &vertex_array);
    gl_safe(glBindVertexArray, vertex_array);
    gl_safe(glEnableVertexAttribArray, world_position_loc);
    gl_safe(glVertexAttribPointer, world_position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*)offsetof(Vertex, pos));
    gl_safe(glEnableVertexAttribArray, vertex_uv_loc);
    gl_safe(glVertexAttribPointer, vertex_uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    unsigned int texture;
    gl_safe(glGenTextures, 1, &texture);
    gl_safe(glBindTexture, GL_TEXTURE_2D, texture);
    gl_safe(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl_safe(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gl_safe(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_safe(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    gl_safe(glPixelStorei, GL_UNPACK_ALIGNMENT, 1);
    gl_safe(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, bitmap.get_width(), bitmap.get_height(), 0, GL_BGRA,
            GL_UNSIGNED_BYTE, bitmap.get_pixels().data());

    while (!glfwWindowShouldClose(window)) {
        gl_safe(glClear, GL_COLOR_BUFFER_BIT);
        gl_safe(glClearColor, 0.1f, 0.2f, 0.2f, 1.0f);

        Eigen::Matrix3f screen_from_world = window_control.get_screen_from_world();

        gl_safe(glUseProgram, program);
        gl_safe(glUniformMatrix3fv, screen_from_world_loc, 1, GL_FALSE, screen_from_world.data());
        gl_safe(glBindVertexArray, vertex_array);
        gl_safe(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
