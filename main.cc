#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <cmath>
#include <iostream>

#include "bitmap.hh"

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

struct Window {
    void update_mouse_position(double x, double y) {
        Eigen::Vector2f position(x, y);
        update_mouse_position_incremental(position - previous_position);
        previous_position = position;

        update_mouse_position();
    }

    void update_mouse_position_incremental(Eigen::Vector2f increment) {
        if (clicked) {
            double current_scale = scale();
            center.x() -= current_scale * increment.x();
            center.y() -= current_scale * increment.y();
        }
    }

    void update_scroll(double /*x*/, double y) {
        double zoom_factor = 0.1 * -y;
        Eigen::Vector2f new_half_dim = half_dim + zoom_factor * half_dim;

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

    Eigen::Matrix3f get_screen_from_world() const {
        using Transform = Eigen::Matrix3f;

        Transform translate = Transform::Identity();
        Transform scale = Transform::Identity();

        translate(0, 2) = -center.x();
        translate(1, 2) = -center.y();
        scale.diagonal() = Eigen::Vector3f{1 / half_dim.x(), -1 / half_dim.y(), 1};

        return scale * translate;
        // glOrtho(top_left.x(), bottom_right.x(), bottom_right.y(), top_left.y(), 0.0, 1.0);
    }

    void reset() {
        center = kInitialHalfDim;
        half_dim = kInitialHalfDim;
        update_mouse_position();
    }

    double scale() const { return (half_dim).norm() / kInitialHalfDim.norm(); }

    void update_mouse_position() {
        Eigen::Vector2f top_left = center - half_dim;
        Eigen::Vector2f bottom_right = center + half_dim;
        Eigen::Vector2f screen_position{previous_position.x() / kWidth, previous_position.y() / kHeight};
        mouse_position = screen_position.cwiseProduct(bottom_right - top_left) + top_left;
    }

    const Eigen::Vector2f kInitialHalfDim = 0.5 * Eigen::Vector2f{kWidth, kHeight};
    const Eigen::Vector2f kMinHalfDim = 0.5 * Eigen::Vector2f{0.1 * kWidth, 0.1 * kHeight};
    const Eigen::Vector2f kMaxHalfDim = 0.5 * Eigen::Vector2f{3.0 * kWidth, 3.0 * kHeight};

    Eigen::Vector2f center = kInitialHalfDim;
    Eigen::Vector2f half_dim = kInitialHalfDim;

    Eigen::Vector2f previous_position;

    Eigen::Vector2f mouse_position;

    bool clicked = false;
};

static Window window_control{};

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

int compile_shader(int type, const std::string& source) {
    // Create an empty shader handle
    int shader = glCreateShader(type);

    // Send the vertex shader source code to GL
    const char* data = source.c_str();
    glShaderSource(shader, 1, &data, 0);

    // Compile the vertex shader
    glCompileShader(shader);

    int compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        int length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        // The maxLength includes the NULL character
        std::string log;
        log.resize(length);
        glGetShaderInfoLog(shader, length, &length, &log[0]);

        // We don't need the shader anymore.
        glDeleteShader(shader);

        throw std::runtime_error("Failed to compile shader: " + log);
    }
    return shader;
}

std::string vertex_shader_text = R"(
#version 330
uniform mat3 screen_from_world;
in vec2 world_position;
in vec2 vertex_uv;

out vec2 uv;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, 0.0, 1.0);
    uv = vertex_uv;
}
)";

std::string fragment_shader_text = R"(
#version 330
in vec2 uv;
out vec4 fragment;
uniform sampler2D sampler;
void main()
{
    fragment = texture(sampler, uv);
}
)";

int link_shaders() {
    auto vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_text);
    auto fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_text);
    if (vertex_shader < 0 || fragment_shader < 0) {
        throw std::runtime_error("Failed to build at least one of the shaders.");
    }

    // Vertex and fragment shaders are successfully compiled.
    // Now time to link them together into a program.
    // Get a program object.
    int program = glCreateProgram();

    // Attach our shaders to our program
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    // Link our program
    glLinkProgram(program);

    // Note the different functions here: glGetProgram* instead of glGetShader*.
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

    // Always detach shaders after a successful link.
    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);
    return program;
}
typedef struct Vertex {
    float pos[2];
    float uv[2];
} Vertex;

int main() {
    Bitmap bitmap{"/Users/mlangford/Downloads/test.bmp"};
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
    with_error_check(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGB, bitmap.get_width(), bitmap.get_height(), 0, GL_BGR,
                     GL_UNSIGNED_BYTE, bitmap.get_pixels().data());

    while (!glfwWindowShouldClose(window)) {
        with_error_check(glClear, GL_COLOR_BUFFER_BIT);

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
