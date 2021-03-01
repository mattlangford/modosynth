#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Eigen/Dense>

namespace engine {
class Window {
public:
    Window(size_t height, size_t width)
        : height_(height),
          width_(width),
          kInitialHalfDim_{0.5 * Eigen::Vector2f{width_, height_}},
          kMinHalfDim_{0.5 * Eigen::Vector2f{0.1 * width_, 0.1 * height_}},
          kMaxHalfDim_{0.5 * Eigen::Vector2f{3.0 * width_, 3.0 * height_}},
          center_{kInitialHalfDim_},
          half_dim_{kInitialHalfDim_} {}

    void update_mouse_position(double x, double y) {
        Eigen::Vector2f position(x, y);
        update_mouse_position_incremental(position - previous_position__);
        previous_position__ = position;

        BlockObjectManager m{};
        update_mouse_position();
    }

    void update_mouse_position_incremental(Eigen::Vector2f increment) {
        if (clicked_) {
            double current_scale = scale();
            center_.x() -= current_scale * increment.x();
            center_.y() -= current_scale * increment.y();
        }
    }

    void update_scroll(double /*x*/, double y) {
        double zoom_factor = 0.1 * -y;
        Eigen::Vector2f new_half_dim_ = half_dim_ + zoom_factor * half_dim_;

        if (new_half_dim_.x() < kMinHalfDim_.x() || new_half_dim_.y() < kMinHalfDim_.y()) {
            new_half_dim_ = kMinHalfDim_;
        } else if (new_half_dim_.x() > kMaxHalfDim_.x() || new_half_dim_.y() > kMaxHalfDim_.y()) {
            new_half_dim_ = kMaxHalfDim_;
        }

        double translate_factor = new_half_dim_.norm() / half_dim_.norm() - 1;
        center_ += translate_factor * (center_ - mouse_position);
        half_dim_ = new_half_dim_;

        update_mouse_position();
    }

    void click() { clicked_ = true; }

    void release() { clicked_ = false; }

    Eigen::Matrix3f get_screen_from_world() const {
        using Transform = Eigen::Matrix3f;

        Transform translate = Transform::Identity();
        Transform scale = Transform::Identity();

        translate(0, 2) = -center_.x();
        translate(1, 2) = -center_.y();
        scale.diagonal() = Eigen::Vector3f{1 / half_dim_.x(), -1 / half_dim_.y(), 1};

        return scale * translate;
    }

    void reset() {
        center_ = kInitialHalfDim_;
        half_dim_ = kInitialHalfDim_;
        update_mouse_position();
    }

    double scale() const { return (half_dim_).norm() / kInitialHalfDim_.norm(); }

    void update_mouse_position() {
        Eigen::Vector2f top_left = center_ - half_dim_;
        Eigen::Vector2f bottom_right = center_ + half_dim_;
        Eigen::Vector2f screen_position{previous_position__.x() / width_, previous_position__.y() / height_};
        mouse_position = screen_position.cwiseProduct(bottom_right - top_left) + top_left;
    }

private:
    const size_t height_;
    const size_t width_;

    const Eigen::Vector2f kInitialHalfDim_;
    const Eigen::Vector2f kMinHalfDim_;
    const Eigen::Vector2f kMaxHalfDim_;

    Eigen::Vector2f center_;
    Eigen::Vector2f half_dim_;

    Eigen::Vector2f previous_position__;

    Eigen::Vector2f mouse_position;

    bool clicked_ = false;
};
}  // namespace engine
