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
    float thickness = 2;
    vec2 start = gl_in[0].gl_Position.xy;
    vec2 end = gl_in[1].gl_Position.xy;

    vec2 normal = thickness * normalize(vec2(-(end.y - start.y), end.x - start.x));

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
    vec2 next_normal = thickness * normalize(vec2(-(next.y - end.y), next.x - end.x));

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

static std::string point_geometry_shader_text = R"(
#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 10) out;

uniform mat3 screen_from_world;

vec4 to_screen(vec2 world)
{
    vec3 screen = screen_from_world * vec3(world.x, world.y, 1.0);
    return vec4(screen.x, screen.y, 0.0, 1.0);
}

void main()
{
    float size = 5;

    gl_Position = to_screen(gl_in[0].gl_Position.xy + vec2(-size, size));
    EmitVertex();
    gl_Position = to_screen(gl_in[0].gl_Position.xy + vec2(size, size));
    EmitVertex();
    gl_Position = to_screen(gl_in[0].gl_Position.xy + vec2(-size, -size));
    EmitVertex();
    gl_Position = to_screen(gl_in[0].gl_Position.xy + vec2(size, -size));
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
        start_ = start;
        diff_.x() = end.x() - start.x();
        diff_.y() = -end.y() - -start.y();

        length_ = length;
    }

    constexpr static float sq(float in) { return in * in; }

    float y(float x) const {
        float h = diff_.x();
        float v = diff_.y();

        return 1.0/std::sqrt(2*x*std::sinh(h/(2*x))/h - 1) - 1/std::sqrt(std::sqrt(sq(length_) - sq(v))/h - 1);
    }
    float dy(float x) const {
        float h = diff_.x();
        return (-std::sinh(h / (2 * x)) / h + std::cosh(h / (2 * x)) / (2 * x)) /
               std::pow(2 * x * std::sinh(h / (2 * x)) / h - 1, 3.0 / 2.0);
    }

    float f(float x) const
    {
        return alpha_ * std::cosh((x - x_offset_) / alpha_) + y_offset_;
    }

    bool solve(float x0 = 10.f, float tol = 1E-3, size_t max_iter = 100) {
        float diff = 1E5;
        float x = x0;
        size_t iter = 0;
        for (; (iter < max_iter) && (abs(diff) > tol); ++iter) {
            std::cout << "Trying x=" << x << " y=" << y(x) << " (dy=" << dy(x) << ") after " << iter
                      << " iterations \n";
            x = x - y(x) / dy(x);
            diff = y(x);
            if (std::isnan(diff))
            {
                throw std::runtime_error("Got NaN in CatenarySolver::solve()");
            }
        }

        std::cout << "Converged to x=" << x << " y=" << y(x) << " after " << iter << " iterations \n";
        alpha_ = x;

        float h = diff_.x();
        float v = diff_.y();

        x_offset_ = 0.5 * (h + alpha_ * std::log((length_ - v) / (length_ + v)));
        y_offset_ = -f(0);
        return iter < max_iter;
    }

    std::vector<Eigen::Vector2f> trace(size_t steps) {
        std::vector<Eigen::Vector2f> result;
        result.reserve(steps);

        float step_size = diff_.x() / (steps - 1);
        float x = 0;
        for (size_t step = 0; step < steps; ++step, x += step_size) {
            result.push_back({x + start_.x(), -f(x) + start_.y()});
        }
        return result;
    }

private:
    Eigen::Vector2f start_;
    Eigen::Vector2f diff_;
    float length_;
    float alpha_;
    float y_offset_ = 0;
    float x_offset_ = 0;
};

class TestObjectManager final : public engine::AbstractObjectManager {
public:
    TestObjectManager()
        : shader_(vertex_shader_text, fragment_shader_text, geometry_shader_text),
          point_shader_(vertex_shader_text, fragment_shader_text, point_geometry_shader_text) {}
    virtual ~TestObjectManager() = default;

    static constexpr size_t kNumSteps = 20;
    void init() override {
        shader_.init();
        point_shader_.init();

        sfw_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");
        engine::throw_on_gl_error("glGetUniformLocation");

        start_ = {100, 200};
        end_ = {500, 500};
        length_ = 2 * (end_ - start_).norm();
        std::cout << "Initial length: " << length_ << "\n";

        vertices_.resize(64);
        populate_vertices();

        for (size_t i = 0; i < kNumSteps; ++i) {
            elements_.emplace_back(i);
            elements_.emplace_back(i + 1);
            elements_.emplace_back(i + 2);
        }

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

        if (updated_)
        {
            populate_vertices();
            gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vbo_);
            gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_DYNAMIC_DRAW);
        }
        gl_safe(glDrawElements, GL_TRIANGLES, elements_.size(), GL_UNSIGNED_INT, (void*)0);

        gl_safe(glBindVertexArray, 0);
        gl_safe(glUseProgram, 0);

        point_shader_.activate();
        gl_safe(glUniformMatrix3fv, sfw_, 1, GL_FALSE, screen_from_world.data());

        gl_safe(glBindVertexArray, vao_);
        gl_safe(glDrawArrays, GL_POINTS, 0, kNumSteps);
        gl_safe(glBindVertexArray, 0);
    }

    void update(float) override {}

    void handle_mouse_event(const engine::MouseEvent& event) override {
        updated_ = false;
        if (point_ && event.held()) {
            updated_ = true;
            float new_length = 1.5 * (end_ - start_).norm();
            if (new_length > length_) {
                std::cout << "Increasing length from " << length_ << " to " << new_length << "\n";
                length_ = new_length;
            }

            *point_ = event.mouse_position;

        } else if (event.pressed()) {
            if ((event.mouse_position - start_).squaredNorm() < 5 * 5) {
                point_ = &start_;
            } else if ((event.mouse_position - end_).squaredNorm() < 5 * 5) {
                point_ = &end_;
            }
        } else {
            point_ = nullptr;
        }
    }

    void handle_keyboard_event(const engine::KeyboardEvent&) override {}

    void populate_vertices() {
        CatenarySolver solver{start_, end_, length_};
        if (!solver.solve()) {
            throw std::runtime_error("CatenarySolver unable to converge!");
        }
        auto points = solver.trace(kNumSteps);

        constexpr size_t stride = 2;
        for (size_t i = 0; i < points.size(); ++i) {
            const Eigen::Vector2f& point = points[i];

            size_t el = 0;
            vertices_[stride * i + el++] = point.x();
            vertices_[stride * i + el++] = point.y();
        }

        size_t el = 0;
        vertices_[stride * points.size() + el++] = end_.x();
        vertices_[stride * points.size() + el++] = end_.y();
    }

    Eigen::Vector2f* point_;
    float length_;
    Eigen::Vector2f start_;
    Eigen::Vector2f end_;
    bool updated_ = false;

    std::vector<float> vertices_;
    std::vector<unsigned int> elements_;

    engine::Shader shader_;
    engine::Shader point_shader_;
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
