#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <cmath>
#include <iostream>
#include <list>

#include "engine/bitmap.hh"
#include "engine/shader.hh"

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

//
// #############################################################################
//

struct ObjectId {
    explicit ObjectId(size_t key_) : key(key_) {}
    bool operator!=(const ObjectId& rhs) const { return key != rhs.key; }
    size_t key;
};

template <typename Object_>
class AbstractObjectPool {
public:
    using Object = Object_;

    virtual ~AbstractObjectPool() = default;

    virtual std::pair<ObjectId, Object&> add(Object object) = 0;
    virtual void remove(const ObjectId& id) = 0;

    virtual Object& get(const ObjectId& id) = 0;
    virtual const Object& get(const ObjectId& id) const = 0;

    virtual ObjectId first() const = 0;
    virtual ObjectId last() const = 0;
    virtual ObjectId next(const ObjectId& id) const = 0;
};

template <typename Object_>
class ListObjectPool : public AbstractObjectPool<Object_> {
private:
    using iterator = typename std::list<Object_>::iterator;

    ObjectId id_from_it(const iterator& it) const { return ObjectId{*reinterpret_cast<size_t*>(&*it)}; }

    iterator it_from_id(const ObjectId& id) const { return reinterpret_cast<const iterator&>(id.key); }

public:
    using Object = Object_;

    ~ListObjectPool() override = default;

    std::pair<ObjectId, Object&> add(Object object) override {
        Object& emplaced = pool_.emplace_back(std::move(object));
        return {last(), emplaced};
    }

    void remove(const ObjectId& id) override { pool_.erase(it_from_id(id)); }
    Object& get(const ObjectId& id) override { return *it_from_id(id); }
    const Object& get(const ObjectId& id) const override { return *it_from_id(id); }
    ObjectId first() const override { return it_to_id(pool_.begin()); }
    ObjectId last() const override { return it_to_id(pool_.end()); }
    ObjectId next(const ObjectId& id) const override { return it_from_id(it_from_id(id)++); }

private:
    std::list<Object_> pool_;
};

//
// #############################################################################
//

struct BlockObject {
    ObjectId id;
    engine::Bitmap texture;

    float top_left_x;
    float top_left_y;

    float get_top_left_x() const { return top_left_x; }
    float get_top_left_y() const { return top_left_x + texture.get_width(); }
    float get_bottom_right_x() const { return top_left_y; }
    float get_bottom_right_y() const { return top_left_y + texture.get_height(); }
};

class BlockObjectManager {
public:
    BlockObjectManager() {}

    BlockObject* get_selected_object(float x, float y) const;

    void render(const Eigen::Matrix3f& screen_from_world) const {
        const auto& pool = *pool_;

        // 1. load up shaders
        // 2. allocate point data
        // 3. populate point data
        for (auto it = pool.first(); it != pool.last(); it = pool.next(it)) {
        }
    }

    void add_object(BlockObject object_) {
        auto [id, object] = pool_->add(std::move(object_));
        object.id = id;
    }

    void remove_object(const ObjectId& id) { pool_->remove(id); }

private:
    void init();
    void activate_shaders();
    void draw_points(const Eigen::Matrix3f& screen_from_world, const std::vector<float>& world_points);

private:
    std::unique_ptr<AbstractObjectPool<BlockObject>> pool_;
};

struct Window {
    void update_mouse_position(double x, double y) {
        Eigen::Vector2f position(x, y);
        update_mouse_position_incremental(position - previous_position);
        previous_position = position;

        BlockObjectManager m{};
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
