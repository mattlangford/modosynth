#pragma once
#include <Eigen/Dense>
#include <filesystem>
#include <vector>

#include "engine/buffer.hh"
#include "engine/object_manager.hh"
#include "engine/pool.hh"
#include "objects/ports.hh"

namespace objects {

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
    double alpha_;
    double beta_;
    float x_offset_ = 0;
    float y_offset_ = 0;
};

struct BlockObject;

struct CatenaryObject {
    static constexpr size_t kNumSteps = 2;
    std::vector<Eigen::Vector2f> calculate_points() {
        const Eigen::Vector2f current_start = start();
        const Eigen::Vector2f current_end = end();
        double min_length = 1.01 * (current_end - current_start).norm();
        solver.reset(current_start, current_end, std::max(solver.length(), min_length));
        if (!solver.solve()) throw std::runtime_error("Unable to update CatenaryObject, did not converge.");
        return solver.trace(kNumSteps);;
    }

    const BlockObject* parent_start = nullptr;
    const BlockObject* parent_end = nullptr;
    Eigen::Vector2f offset_start;
    Eigen::Vector2f offset_end;

    Eigen::Vector2f start() const;
    Eigen::Vector2f end() const;

    CatenarySolver solver;

    engine::ObjectId object_id;
    size_t vertex_index;
    size_t element_index;
};

//
// #############################################################################
//

class CableObjectManager final : public engine::AbstractSingleShaderObjectManager {
public:
    CableObjectManager(std::shared_ptr<PortsObjectManager> ports_manager);
    virtual ~CableObjectManager() = default;

protected:
    void init_with_vao() override;

    void render_with_vao() override;

private:
    const PortsObject* get_active_port(const Eigen::Vector2f& position, Eigen::Vector2f& offset) const;
    void spawn_object(CatenaryObject object);
    void despawn_object(CatenaryObject& object);

    void populate_ebo(size_t vertex_index);

public:
    void update(float dt) override;

    void handle_mouse_event(const engine::MouseEvent& event) override;

    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

private:
    std::shared_ptr<PortsObjectManager> ports_manager_;

    std::unique_ptr<engine::AbstractObjectPool<CatenaryObject>> pool_;

    CatenaryObject* selected_ = nullptr;

    engine::VertexArrayObject vao_;
    engine::Buffer<float, 2> vbo_;
    engine::Buffer<unsigned int> ebo_;
};
}  // namespace objects
