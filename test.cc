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

static const Vertex vertices[4] = {{{0.f, 0.f}, {1.f, 0.f, 0.f}},
                                   {{1.0f, 0.0f}, {0.f, 1.f, 0.f}},
                                   {{0.f, 1.0f}, {0.f, 0.f, 1.f}},
                                   {{1.f, 1.f}, {1.f, 1.f, 1.f}}};

static const char* vertex_shader_text =
    "#version 330\n"
    "uniform mat4 MVP;\n"
    "in vec2 vPos;\n"
    "out vec2 uv;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = MVP * vec4(vPos.x - 0.5, vPos.y - 0.5, 0.0, 1.0);\n"
    "    uv = vPos;\n"
    "}\n";

static const char* fragment_shader_text =
    "#version 330\n"
    "in vec2 uv;\n"
    "out vec4 fragment;\n"
    "uniform sampler2D sampler;\n"
    "void main()\n"
    "{\n"
    "    fragment = texture(sampler, uv).rgba;\n"
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

    int compiled = 0;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        int length = 0;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &length);

        // The maxLength includes the NULL character
        std::string log;
        log.resize(length);
        glGetShaderInfoLog(vertex_shader, length, &length, &log[0]);

        // We don't need the shader anymore.
        glDeleteShader(vertex_shader);

        throw std::runtime_error("Failed to compile shader: " + log);
    }

    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    with_error_check(glShaderSource, fragment_shader, 1, &fragment_shader_text, NULL);
    with_error_check(glCompileShader, fragment_shader);

    compiled = 0;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        int length = 0;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length);

        // The maxLength includes the NULL character
        std::string log;
        log.resize(length);
        glGetShaderInfoLog(fragment_shader, length, &length, &log[0]);

        // We don't need the shader anymore.
        glDeleteShader(fragment_shader);

        throw std::runtime_error("Failed to compile shader: " + log);
    }

    const GLuint program = glCreateProgram();
    with_error_check(glAttachShader, program, vertex_shader);
    with_error_check(glAttachShader, program, fragment_shader);
    with_error_check(glLinkProgram, program);

    int linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        int length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        std::string log;
        log.resize(length);
        glGetProgramInfoLog(program, length, &length, &log[0]);

        // We don't need the program anymore.
        glDeleteProgram(program);
        // Don't leak shaders either.
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        throw std::runtime_error("Failed to link shaders: " + log);
    }

    const GLint mvp_location = glGetUniformLocation(program, "MVP");
    const GLint vpos_location = glGetAttribLocation(program, "vPos");
    // const GLint vcol_location = glGetAttribLocation(program, "vCol");

    GLuint vertex_array;
    with_error_check(glGenVertexArrays, 1, &vertex_array);
    with_error_check(glBindVertexArray, vertex_array);
    with_error_check(glEnableVertexAttribArray, vpos_location);
    with_error_check(glVertexAttribPointer, vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                     (void*)offsetof(Vertex, pos));
    // with_error_check(glEnableVertexAttribArray, vcol_location);
    // with_error_check(glVertexAttribPointer, vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
    //                  (void*)offsetof(Vertex, col));

    unsigned int texture;
    with_error_check(glGenTextures, 1, &texture);
    with_error_check(glBindTexture, GL_TEXTURE_2D, texture);
    with_error_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    with_error_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    with_error_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    with_error_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    std::vector<uint8_t> data;
    for (size_t i = 0; i < 256 * 256; ++i) {
        data.push_back(0xFF);
        data.push_back(0x00);
        data.push_back(0x00);
        data.push_back(0xFF);
    }
    with_error_check(glPixelStorei, GL_UNPACK_ALIGNMENT, 1);
    with_error_check(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGB, bitmap.get_width(), bitmap.get_height(), 0, GL_BGR,
                     GL_UNSIGNED_BYTE, bitmap.get_pixels().data());

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
        with_error_check(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
