#pragma once
#include <Eigen/Dense>

namespace engine {
bool is_in_rectangle(const Eigen::Vector2f& point, const Eigen::Vector2f& top_left,
                     const Eigen::Vector2f& bottom_right);
}
