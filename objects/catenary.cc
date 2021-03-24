#include "objects/catenary.hh"

#include <iostream>

namespace objects {

//
// #############################################################################
//

CatenarySolver::CatenarySolver() {}

//
// #############################################################################
//

bool CatenarySolver::maybe_reset(Eigen::Vector2f start, Eigen::Vector2f end, float length) {
    flipped_ = start.x() > end.x();
    if (flipped_) {
        std::swap(start, end);
    }

    if (start == start_ && end == end_) {
        return false;
    }

    start_ = start;
    end_ = end;
    beta_ = 10.0;
    length_ = length;
    x_offset_ = 0;
    y_offset_ = 0;
    return true;
}

//
// #############################################################################
//

bool CatenarySolver::solve(double tol, size_t max_iter) {
    size_t iter = 0;

    const Eigen::Vector2f diff = end_ - start_;

    // Function we're optimizing which relates the free parameter (b) to the size of the opening we need.
    // [1] https://foggyhazel.wordpress.com/2018/02/12/catenary-passing-through-2-points/
    auto y = [this, &diff](double b) {
        return 1.0 / std::sqrt(2 * sq(b) * std::sinh(1 / (2 * sq(b))) - 1) -
               1.0 / std::sqrt(std::sqrt(sq(length_) - sq(diff.y())) / diff.x() - 1);
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
    alpha_ = diff.x() * sq(beta_);
    x_offset_ = 0.5 * (diff.x() + alpha_ * std::log((length_ - diff.y()) / (length_ + diff.y())));
    y_offset_ = -f(0);
    return iter < max_iter;
}

//
// #############################################################################
//

std::vector<Eigen::Vector2f> CatenarySolver::trace(size_t points) const {
    if (points <= 1) throw std::runtime_error("Calling CatenarySolver::trace() with too few point steps");
    std::vector<Eigen::Vector2f> result;
    result.reserve(points);

    const Eigen::Vector2f diff = end_ - start_;

    double step_size = diff.x() / (points - 1);
    double x = 0;
    for (size_t point = 0; point < points; ++point, x += step_size) {
        result.push_back({x + start_.x(), f(x) + start_.y()});
    }

    if (flipped()) {
        std::reverse(result.begin(), result.end());
    }

    return result;
}

//
// #############################################################################
//

double& CatenarySolver::length() { return length_; }
bool CatenarySolver::flipped() const { return flipped_; }

//
// #############################################################################
//

double CatenarySolver::f(double x) const { return alpha_ * std::cosh((x - x_offset_) / alpha_) + y_offset_; }

}  // namespace objects
