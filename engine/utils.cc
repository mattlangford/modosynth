#include "engine/utils.hh"

namespace engine {
bool is_in_rectangle(const Eigen::Vector2f& point, const Eigen::Vector2f& bottom_left,
                     const Eigen::Vector2f& top_right) {
    return point.x() >= bottom_left.x() && point.x() < top_right.x() && point.y() >= bottom_left.y() &&
           point.y() < top_right.y();
}
}  // namespace engine
