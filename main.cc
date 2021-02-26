#include <GLFW/glfw3.h>

#include <Eigen/Dense>
#include <cmath>
#include <iostream>

#include "bitmap.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

void check_gl_error(std::string action="") {
    auto error = glGetError();
    if (error != GL_NO_ERROR) {
        std::stringstream ss;
        if (!action.empty())
            ss << action << " failed. ";
        ss << "Error code: 0x" << std::hex << error;
        throw std::runtime_error(ss.str());
    }
}

void init_view() {
    // set up view
    // glViewport(-100, -100, kWidth + 100, kHeight + 100);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // this creates a canvas to do 2D drawing on
    glOrtho(0.0, kWidth, kHeight, 0.0, -1.0, 1.0);
    glGetError();
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

int link_shaders() {
    std::string vertex = R"(
#version 110
attribute vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
})";
    std::string fragment = R"(
#version 110
uniform vec3 color;

void main() {
    gl_FragColor = vec4(color, 1.0);
})";

    // auto vertex_shader = compile_shader(GL_VERTEX_SHADER, std::string(vertex_shader_text));
    // auto fragment_shader = compile_shader(GL_FRAGMENT_SHADER, std::string(fragment_shader_text));
    auto vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex);
    auto fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment);
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

int main() {
    Bitmap bitmap{"/Users/mlangford/Downloads/sample_640×426.bmp"};
    auto pixesl = bitmap.get_pixels();
    GLFWwindow* window;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        exit(EXIT_FAILURE);
    }

    // Upgrade to a newer GLSL version for the shaders
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(kWidth, kHeight, "Window", NULL, NULL);
    if (!window) {
        std::cerr << "Unable to create window!\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    check_gl_error("window");

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    check_gl_error("callbacks");

    init_view();
    check_gl_error("init_view");

    float vertices[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f};

    unsigned int vbo;
    glGenBuffers(1, &vbo);
    check_gl_error("glGenBuffers");

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    check_gl_error("glBindBuffer");

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    check_gl_error("glBufferData");

    int program = link_shaders();
    check_gl_error("link_shaders");
    glUseProgram(program);

    // int texture;
    // glGenTextures(1, &texture);
    // glBindTexture(GL_TEXTURE_2D, texture);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_BGR, checkImageWidth, checkImageHeight, 0, GL_BGR, GL_UNSIGNED_BYTE,
    //              bitmap.get_pixels().data());

    // then set our vertex attributes pointers
    int position = glGetAttribLocation(program, "position");
    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    check_gl_error("glVertexAttribPointer");
    glEnableVertexAttribArray(position);
    check_gl_error("glEnableVertexAttribArray");

    int color = glGetUniformLocation(program, "color");
    check_gl_error("glGetUniformLocation");
    glUniform3f(color, 1.0f, 0.0f, 1.0f);
    check_gl_error("glUniform3f");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
        continue;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 1);

        // pan_and_zoom.set_model_view_matrix();

        // glBegin(GL_LINES);
        // glColor3d(1, 1, 1);
        // glVertex2d(100.0, 100.0);
        // glVertex2d(200.0, 200.0);
        // glVertex2d(100.0, 200.0);
        // glVertex2d(200.0, 100.0);
        // glEnd();

        // // glRasterPos2d(pan_x, pan_y);
        // // glPixelZoom(zoom, zoom);
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
