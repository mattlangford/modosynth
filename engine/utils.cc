#include "engine/utils.hh"

namespace engine {
bool is_in_rectangle(const Eigen::Vector2f& point, const Eigen::Vector2f& top_left,
                     const Eigen::Vector2f& bottom_right) {
    return point.x() >= top_left.x() && point.x() < bottom_right.x() && point.y() >= top_left.y() &&
           point.y() < bottom_right.y();
}
}  // namespace engine
