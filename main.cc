#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <cmath>
#include <iostream>
#include <list>

#include "engine/bitmap.hh"
#include "engine/objects.hh"
#include "engine/shader.hh"
#include "engine/window.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

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
        case GL_INVALID_OPERATION:
            ss << GL_INVALID_OPERATION << " (GL_INVALID_OPERATION).";
            break;
        default:
            ss << error;
            break;
    }
    throw std::runtime_error(ss.str());
}

#define with_error_check(func, ...)                                                                      \
    func(__VA_ARGS__);                                                                                   \
    check_gl_error(std::string("\nline ") + std::to_string(__LINE__) + ": " + std::string(#func) + "(" + \
                   std::string(#__VA_ARGS__) + ")")

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

    with_error_check(glEnable, GL_BLEND);
    with_error_check(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int program = link_shaders();

    const int screen_from_world_loc = glGetUniformLocation(program, "screen_from_world");
    const int world_position_loc = glGetAttribLocation(program, "world_position");
    const int vertex_uv_loc = glGetAttribLocation(program, "vertex_uv");

    unsigned int vertex_buffer;
    with_error_check(glGenBuffers, 1, &vertex_buffer);
    with_error_check(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
    with_error_check(glBufferData, GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    unsigned int vertex_array;
    with_error_check(glGenVertexArrays, 1, &vertex_array);
    with_error_check(glBindVertexArray, vertex_array);
    with_error_check(glEnableVertexAttribArray, world_position_loc);
    with_error_check(glVertexAttribPointer, world_position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                     (void*)offsetof(Vertex, pos));
    with_error_check(glEnableVertexAttribArray, vertex_uv_loc);
    with_error_check(glVertexAttribPointer, vertex_uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                     (void*)offsetof(Vertex, uv));

    unsigned int texture;
    with_error_check(glGenTextures, 1, &texture);
    with_error_check(glBindTexture, GL_TEXTURE_2D, texture);
    with_error_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    with_error_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    with_error_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    with_error_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    with_error_check(glPixelStorei, GL_UNPACK_ALIGNMENT, 1);
    with_error_check(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, bitmap.get_width(), bitmap.get_height(), 0, GL_BGRA,
                     GL_UNSIGNED_BYTE, bitmap.get_pixels().data());

    while (!glfwWindowShouldClose(window)) {
        with_error_check(glClear, GL_COLOR_BUFFER_BIT);
        with_error_check(glClearColor, 0.1f, 0.2f, 0.2f, 1.0f);

        Eigen::Matrix3f screen_from_world = window_control.get_screen_from_world();

        with_error_check(glUseProgram, program);
        with_error_check(glUniformMatrix3fv, screen_from_world_loc, 1, GL_FALSE, screen_from_world.data());
        with_error_check(glBindVertexArray, vertex_array);
        with_error_check(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
