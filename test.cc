#include <cmath>
#include <random>

#include "engine/buffer.hh"
#include "engine/object_manager.hh"
#include "engine/shader.hh"
#include "engine/vao.hh"
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

class CatenarySolver {
public:
    CatenarySolver() {}

    void reset(Eigen::Vector2f start, Eigen::Vector2f end, float length) {
        if (start.x() > end.x()) std::swap(start, end);
        start_ = start;
        diff_.x() = end.x() - start.x();
        diff_.y() = end.y() - start.y();
        beta_ = 10.0;
        length_ = length;
        x_offset_ = 0;
        y_offset_ = 0;
    }

    bool solve(double tol = 1E-3, size_t max_iter = 100) {
        size_t iter = 0;

        // Function we're optimizing which relates the free parameter (b) to the size of the opening we need.
        // [1] https://foggyhazel.wordpress.com/2018/02/12/catenary-passing-through-2-points/
        auto y = [this](double b) {
            return 1.0 / std::sqrt(2 * sq(b) * std::sinh(1 / (2 * sq(b))) - 1) -
                   1.0 / std::sqrt(std::sqrt(sq(length_) - sq(diff_.y())) / diff_.x() - 1);
        };
        // Derivative according to sympy
        auto dy = [](double b) {
            return (-2 * b * sinh(1 / (2 * sq(b))) + cosh(1.0 / (2.0 * sq(b))) / b) /
                   std::pow(2.0 * sq(b) * std::sinh(1.0 / (2.0 * sq(b))) - 1.0, 3.0 / 2.0);
        };

        // Newton iteration
        for (; iter < max_iter; ++iter) {
            float y_val = y(beta_);
            if (abs(y_val) < tol) break;
            beta_ -= y_val / dy(beta_);
        }

        // Since b = sqrt(h / a), we can solve easily for a
        alpha_ = diff_.x() * sq(beta_);
        x_offset_ = 0.5 * (diff_.x() + alpha_ * std::log((length_ - diff_.y()) / (length_ + diff_.y())));
        y_offset_ = -f(0);
        return iter < max_iter;
    }

    std::vector<Eigen::Vector2f> trace(size_t points) {
        if (points <= 1) throw std::runtime_error("Calling CatenarySolver::trace() with too few point steps");
        std::vector<Eigen::Vector2f> result;
        result.reserve(points);

        double step_size = diff_.x() / (points - 1);
        double x = 0;
        for (size_t point = 0; point < points; ++point, x += step_size) {
            result.push_back({x + start_.x(), f(x) + start_.y()});
        }
        return result;
    }

    double& length() { return length_; }

private:
    double f(double x) const { return alpha_ * std::cosh((x - x_offset_) / alpha_) + y_offset_; }
    constexpr static double sq(double in) { return in * in; }

    Eigen::Vector2f start_;
    Eigen::Vector2d diff_;
    double length_;
    float beta_;
    float alpha_;
    float y_offset_ = 0;
    float x_offset_ = 0;
};

struct CatenaryObject {
    static constexpr size_t kNumSteps = 32;
    std::vector<Eigen::Vector2f> calculate_points() {
        solver.reset(start, end, std::max(solver.length(), min_length()));
        if (!solver.solve()) throw std::runtime_error("Unable to update CatenaryObject, did not converge.");

        auto points = solver.trace(kNumSteps);
        return points;
    }

    double min_length() const { return 1.01 * (end - start).norm(); }

    void draw_lines(engine::VertexArrayObject& vao) const {
        const void* indices = reinterpret_cast<void*>(sizeof(unsigned int) * start_element_index);

        // Each step will generate a triangle, so render 3 times as many
        gl_check_with_vao(vao, glDrawElements, GL_TRIANGLES, 3 * kNumSteps, GL_UNSIGNED_INT, indices);
    }
    void draw_points(engine::VertexArrayObject& vao) const {
        gl_check_with_vao(vao, glDrawArrays, GL_POINTS, start_vertex_index / 2, kNumSteps);
    }

    Eigen::Vector2f start;
    Eigen::Vector2f end;
    CatenarySolver solver;
    size_t start_vertex_index;
    size_t start_element_index;
    bool needs_update;
    size_t id;
};

class TestObjectManager final : public engine::AbstractObjectManager {
public:
    TestObjectManager()
        : shader_(vertex_shader_text, fragment_shader_text, geometry_shader_text),
          point_shader_(vertex_shader_text, fragment_shader_text, point_geometry_shader_text) {}
    virtual ~TestObjectManager() = default;

    void spawn_object(Eigen::Vector2f start, Eigen::Vector2f end) {
        static size_t id = 0;
        objects_.emplace_back(CatenaryObject{start, end, {}, vbo_.size(), ebo_.size(), true, id++});

        // The element index is the starting vertex. The VBO starts x,y, so to get points we have to divide by 2
        size_t vertex_index = vbo_.size() / 2;
        vbo_.resize(vbo_.size() + 2 * CatenaryObject::kNumSteps);

        // Now add in the elements needed to render this object
        auto elements = ebo_.batched_updater();
        for (size_t i = 0; i < CatenaryObject::kNumSteps - 2; ++i) {
            elements.push_back(vertex_index + i);
            elements.push_back(vertex_index + i + 1);
            elements.push_back(vertex_index + i + 2);
        }
        // Add the last element in, this one needs to be special so we don't duplicate points in the vbo
        elements.push_back(vertex_index + CatenaryObject::kNumSteps - 2);
        elements.push_back(vertex_index + CatenaryObject::kNumSteps - 1);
        elements.push_back(vertex_index + CatenaryObject::kNumSteps - 1);
    }

    void spawn_random_object() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(-1000, 1000);
        spawn_object({dis(gen), dis(gen)}, {dis(gen), dis(gen)});
    }

    void init() override {
        shader_.init();
        point_shader_.init();

        sfw_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");
        engine::throw_on_gl_error("glGetUniformLocation");

        vao_.init();
        vbo_.init(GL_ARRAY_BUFFER, 0, 2, vao_);
        ebo_.init(GL_ELEMENT_ARRAY_BUFFER, vao_);
    }

    void render(const Eigen::Matrix3f& screen_from_world) override {
        if (ebo_.size() == 0) return;

        shader_.activate();
        gl_check(glUniformMatrix3fv, sfw_, 1, GL_FALSE, screen_from_world.data());

        for (const auto& object : objects_) object.draw_lines(vao_);

        point_shader_.activate();
        gl_check(glUniformMatrix3fv, sfw_, 1, GL_FALSE, screen_from_world.data());

        for (const auto& object : objects_) object.draw_points(vao_);
    }

    void update(float) override {
        auto vertices = vbo_.batched_updater();

        for (auto& object : objects_) {
            if (!object.needs_update) continue;

            object.needs_update = false;
            size_t index = object.start_vertex_index;
            for (auto point : object.calculate_points()) {
                vertices[index++] = point.x();
                vertices[index++] = point.y();
            }
        }
    }

    void handle_mouse_event(const engine::MouseEvent& event) override {
        if (selected_ && point_ && event.held()) {
            *point_ = event.mouse_position;
            selected_->needs_update = true;
        } else if (event.pressed()) {
            for (auto& object : objects_) {
                constexpr float kClickRadius = 100;
                if ((event.mouse_position - object.start).squaredNorm() < kClickRadius * kClickRadius) {
                    point_ = &object.start;
                    selected_ = &object;
                } else if ((event.mouse_position - object.end).squaredNorm() < kClickRadius * kClickRadius) {
                    point_ = &object.end;
                    selected_ = &object;
                }
            }
        } else {
            point_ = nullptr;
            selected_ = nullptr;
        }
    }

    void handle_keyboard_event(const engine::KeyboardEvent& event) override {
        if (event.space && event.pressed()) {
            spawn_random_object();
        }
    }

    CatenaryObject* selected_;
    Eigen::Vector2f* point_;

    std::vector<CatenaryObject> objects_;

    engine::Shader shader_;
    engine::Shader point_shader_;
    int sfw_;
    engine::VertexArrayObject vao_;
    engine::Buffer<float> vbo_;
    engine::Buffer<unsigned int> ebo_;
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
