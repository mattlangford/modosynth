#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <vector>

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

typedef struct Vertex {
    float pos[2];
    float col[3];
} Vertex;

static const Vertex vertices[3] = {
    {{-0.6f, -0.4f}, {1.f, 0.f, 0.f}}, {{0.6f, -0.4f}, {0.f, 1.f, 0.f}}, {{0.f, 0.6f}, {0.f, 0.f, 1.f}}};

static const char* vertex_shader_text =
    "#version 330\n"
    "uniform mat4 MVP;\n"
    "in vec3 vCol;\n"
    "in vec2 vPos;\n"
    "out vec3 color;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
    "    color = vCol;\n"
    "}\n";

static const char* fragment_shader_text =
    "#version 330\n"
    "in vec3 color;\n"
    "out vec4 fragment;\n"
    "void main()\n"
    "{\n"
    "    fragment = vec4(color, 1.0);\n"
    "}\n";

static void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main() {
    Bitmap bitmap{"/Users/mlangford/Downloads/sample_640×426.bmp"};

    glfwSetErrorCallback([](int code, const char* desc) { std::cerr << "Error (" << code << "):" << desc << "\n"; });

    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow* window = glfwCreateWindow(1000, 1000, "Window", NULL, NULL);
    if (!window) {
        std::cerr << "Unable to create window!\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    std::cout << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";

    GLuint vertex_buffer;
    with_error_check(glGenBuffers, 1, &vertex_buffer);
    with_error_check(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
    with_error_check(glBufferData, GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    with_error_check(glShaderSource, vertex_shader, 1, &vertex_shader_text, NULL);
    with_error_check(glCompileShader, vertex_shader);

    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    with_error_check(glShaderSource, fragment_shader, 1, &fragment_shader_text, NULL);
    with_error_check(glCompileShader, fragment_shader);

    const GLuint program = glCreateProgram();
    with_error_check(glAttachShader, program, vertex_shader);
    with_error_check(glAttachShader, program, fragment_shader);
    with_error_check(glLinkProgram, program);

    const GLint mvp_location = glGetUniformLocation(program, "MVP");
    const GLint vpos_location = glGetAttribLocation(program, "vPos");
    const GLint vcol_location = glGetAttribLocation(program, "vCol");

    GLuint vertex_array;
    with_error_check(glGenVertexArrays, 1, &vertex_array);
    with_error_check(glBindVertexArray, vertex_array);
    with_error_check(glEnableVertexAttribArray, vpos_location);
    with_error_check(glVertexAttribPointer, vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                     (void*)offsetof(Vertex, pos));
    with_error_check(glEnableVertexAttribArray, vcol_location);
    with_error_check(glVertexAttribPointer, vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                     (void*)offsetof(Vertex, col));

    float i = 0;
    while (!glfwWindowShouldClose(window)) {
        i += 0.01;
        int width, height;
        with_error_check(glfwGetFramebufferSize, window, &width, &height);

        with_error_check(glViewport, 0, 0, width, height);
        with_error_check(glClear, GL_COLOR_BUFFER_BIT);

        float mvp[16] = {0};
        mvp[0] = cos(i);
        mvp[1] = -sin(i);
        mvp[4] = sin(i);
        mvp[5] = cos(i);
        mvp[10] = 1;
        mvp[15] = 1;

        with_error_check(glUseProgram, program);
        with_error_check(glUniformMatrix4fv, mvp_location, 1, GL_FALSE, (const GLfloat*)&mvp);
        with_error_check(glBindVertexArray, vertex_array);
        with_error_check(glDrawArrays, GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
