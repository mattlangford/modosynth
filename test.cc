#include <cmath>

#include "engine/object_manager.hh"
#include "engine/shader.hh"
#include "engine/window.hh"

namespace {
constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

static std::string vertex_shader_text = R"(
#version 330
layout (location = 0) in vec2 world_position;

void main()
{
    gl_Position = vec4(world_position.x, world_position.y, 0.0, 1.0);
}
)";

static std::string fragment_shader_text = R"(
#version 330
out vec4 fragment;

void main()
{
    fragment = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static std::string geometry_shader_text = R"(
#version 330
layout(triangles) in;
layout(triangle_strip, max_vertices = 10) out;

uniform mat3 screen_from_world;

vec4 to_screen(vec2 world)
{
    vec3 screen = screen_from_world * vec3(world.x, world.y, 1.0);
    return vec4(screen.x, screen.y, 0.0, 1.0);
}

void main()
{
    vec2 start = gl_in[0].gl_Position.xy;
    vec2 end = gl_in[1].gl_Position.xy;

    vec2 normal = 5 * normalize(vec2(-(end.y - start.y), end.x - start.x));

    // Draw the main section
    gl_Position = to_screen(start - normal);
    EmitVertex();
    gl_Position = to_screen(end - normal);
    EmitVertex();
    gl_Position = to_screen(start + normal);
    EmitVertex();
    gl_Position = to_screen(end + normal);
    EmitVertex();
    EndPrimitive();

    // Then the end cap (which connects to the next line)
    vec2 next = gl_in[2].gl_Position.xy;
    vec2 next_normal = 5 * normalize(vec2(-(next.y - end.y), next.x - end.x));

    gl_Position = to_screen(end - normal);
    EmitVertex();
    gl_Position = to_screen(end - next_normal);
    EmitVertex();
    gl_Position = to_screen(end);
    EmitVertex();
    EndPrimitive();

    gl_Position = to_screen(end + normal);
    EmitVertex();
    gl_Position = to_screen(end + next_normal);
    EmitVertex();
    gl_Position = to_screen(end);
    EmitVertex();
    EndPrimitive();
}

)";

template <typename Data>
size_t size_in_bytes(const std::vector<Data>& vec) {
    return vec.size() * sizeof(Data);
}
}  // namespace

/// Mostly from https://foggyhazel.wordpress.com/2018/02/12/catenary-passing-through-2-points/
class CatenarySolver {
public:
    CatenarySolver(Eigen::Vector2f start, Eigen::Vector2f end, float length) {
        // Assign the start/end correctly based on their order. Assume start.x() comes before end.x()
        if (start.x() <= end.x()) {
            start_ = end;
            end_ = start;
        } else if (end.x() < start.x()) {
            start_ = end;
            end_ = start;
        } else {
            start_ = start;
            end_ = end + Eigen::Vector2f{1E-5, 1E-5};
        }

        length_ = length;
    }

    constexpr static float sq(float in) { return in * in; }

    float y(float x) const {
        const float& x0 = start_.x();
        const float& y0 = start_.y();
        const float& x1 = end_.x();
        const float& y1 = end_.y();
        return 2 * x * std::sinh((-x0 + x1) / (2 * x)) - std::sqrt(4 * sq((-x0 + x1)) + 3 * sq((-y0 + y1)));
    }
    float dy(float x) const {
        float x0 = start_.x();
        float x1 = end_.x();

        return 2 * std::sinh((-x0 + x1) / (2 * x)) - (-x0 + x1) * std::cosh((-x0 + x1) / (2 * x)) / x;
    }

    void solve(float tol = 1E-3, size_t max_iter = 100) {
        float diff = 1E5;
        float x = 1.f;

        for (size_t iter = 0; (iter < max_iter) && (diff <= tol); ++iter) {
            x = x - y(x) / dy(x);
            diff = y(x);
            std::cout << "Diff at x=" << x << " is " << diff << "\n";
        }

        alpha_ = x;

        float h = abs(end_.x() - start_.x());
        float v = abs(end_.y() - start_.y());

        k_ = start_.x() + 0.5 * (h + alpha_ * std::log((length_ + v) / (length_ - v)));
        c_ = end_.y() - alpha_ * std::cosh((start_.x() - k_) / alpha_);
    }

    std::vector<Eigen::Vector2f> trace(size_t steps) {
        std::vector<Eigen::Vector2f> result;
        result.reserve(steps);

        float step_size = (end_.x() - start_.x()) / steps;
        float x = start_.x();
        for (size_t step = 0; step < steps; ++step, x += step_size) {
            result.push_back({x, alpha_ * std::cosh((x - k_) / alpha_) + c_});
        }
        return result;
    }

private:
    Eigen::Vector2f start_;
    Eigen::Vector2f end_;
    float length_;
    float alpha_;
    float c_;
    float k_;
};

class TestObjectManager final : public engine::AbstractObjectManager {
public:
    TestObjectManager() : shader_(vertex_shader_text, fragment_shader_text, geometry_shader_text) {}
    virtual ~TestObjectManager() = default;

    void init() override {
        shader_.init();

        sfw_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");
        engine::throw_on_gl_error("glGetUniformLocation");

        points_.resize(3);
        points_[0] = {100, 200};
        points_[1] = {500, 300};
        points_[2] = {900, 100};

        vertices_.resize((points_.size() + 1) * 2);
        update_vertices();

        elements_.emplace_back(0);
        elements_.emplace_back(1);
        elements_.emplace_back(2);

        elements_.emplace_back(1);
        elements_.emplace_back(2);
        elements_.emplace_back(3);

        gl_safe(glGenVertexArrays, 1, &vao_);
        gl_safe(glBindVertexArray, vao_);

        gl_safe(glGenBuffers, 1, &vbo_);
        gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vbo_);
        gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_STATIC_DRAW);

        gl_safe(glEnableVertexAttribArray, 0);
        gl_safe(glVertexAttribPointer, 0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        gl_safe(glGenBuffers, 1, &ebo_);
        gl_safe(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, ebo_);
        gl_safe(glBufferData, GL_ELEMENT_ARRAY_BUFFER, size_in_bytes(elements_), elements_.data(), GL_STATIC_DRAW);

        gl_safe(glBindVertexArray, 0);
    }

    void render(const Eigen::Matrix3f& screen_from_world) override {
        shader_.activate();

        gl_safe(glUniformMatrix3fv, sfw_, 1, GL_FALSE, screen_from_world.data());

        gl_safe(glBindVertexArray, vao_);

        update_vertices();
        gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vbo_);
        gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_DYNAMIC_DRAW);
        gl_safe(glDrawElements, GL_TRIANGLES, elements_.size(), GL_UNSIGNED_INT, (void*)0);

        gl_safe(glBindVertexArray, 0);
        gl_safe(glUseProgram, 0);
    }

    void update(float) override {}

    void handle_mouse_event(const engine::MouseEvent& event) override {
        if (point_ && event.held()) {
            *point_ = event.mouse_position;
        } else if (event.pressed()) {
            for (size_t i = 0; i < points_.size(); ++i) {
                Eigen::Vector2f& coord = points_[i];

                if ((event.mouse_position - coord).squaredNorm() < 5 * 5) {
                    point_ = &coord;
                }
            }
        } else {
            point_ = nullptr;
        }
    }

    void handle_keyboard_event(const engine::KeyboardEvent&) override {}

    void update_vertices() {
        if (points_.empty()) return;

        constexpr size_t stride = 2;
        for (size_t i = 0; i < points_.size(); ++i) {
            const Eigen::Vector2f& point = points_[i];

            size_t el = 0;
            vertices_[stride * i + el++] = point.x();
            vertices_[stride * i + el++] = point.y();
        }

        size_t el = 0;
        vertices_[stride * points_.size() + el++] = points_.back().x();
        vertices_[stride * points_.size() + el++] = points_.back().y();
    }

    Eigen::Vector2f* point_;
    std::vector<Eigen::Vector2f> points_;

    std::vector<float> vertices_;
    std::vector<unsigned int> elements_;

    engine::Shader shader_;
    int sfw_;
    unsigned int vao_;
    unsigned int vbo_;
    unsigned int ebo_;
};

int main() {
    engine::GlobalObjectManager object_manager;
    object_manager.add_manager(std::make_shared<TestObjectManager>());
    engine::Window window{kWidth, kHeight, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }

    exit(EXIT_SUCCESS);
}
