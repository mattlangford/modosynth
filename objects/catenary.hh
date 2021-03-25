#pragma once
#include <Eigen/Dense>
#include <vector>

namespace objects {

class CatenarySolver {
public:
    CatenarySolver();

    bool maybe_reset(Eigen::Vector2f start, Eigen::Vector2f end, float length);

    bool solve(double tol = 1E-3, size_t max_iter = 100);

    std::vector<Eigen::Vector2f> trace(size_t points) const;

    const double& length() const;
    bool flipped() const;

private:
    double f(double x) const;
    constexpr static double sq(double in) { return in * in; }

    bool flipped_ = false;
    Eigen::Vector2f start_ = Eigen::Vector2f::Zero();
    Eigen::Vector2f end_ = Eigen::Vector2f::Zero();
    double length_ = -1.0;
    double alpha_ = 0.0;
    double beta_ = 0.0;
    float x_offset_ = 0;
    float y_offset_ = 0;
};
}  // namespace objects
