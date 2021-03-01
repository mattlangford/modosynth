#include "engine/bitmap.hh"
#include "engine/gl.hh"
#include "engine/object_global.hh"
#include "engine/shader.hh"
#include "engine/window.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

typedef struct Vertex {
    float pos[2];
    float uv[2];
} Vertex;

int main() {
    engine::Window window{kHeight, kWidth};

    window.init();

    while (window.render_loop()) {
    }

    exit(EXIT_SUCCESS);
}

/*
int main() {
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
*/
