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
          kInitialHalfDim{0.5 * Eigen::Vector2f{width_, height_}},
          kMinHalfDim{0.5 * Eigen::Vector2f{0.1 * width_, 0.1 * height_}},
          kMaxHalfDim{0.5 * Eigen::Vector2f{3.0 * width_, 3.0 * height_}},
          center{kInitialHalfDim},
          half_dim{kInitialHalfDim} {}

    void update_mouse_position(double x, double y) {
        Eigen::Vector2f position(x, y);
        update_mouse_position_incremental(position - previous_position);
        previous_position = position;

        BlockObjectManager m{};
        update_mouse_position();
    }

    void update_mouse_position_incremental(Eigen::Vector2f increment) {
        if (clicked) {
            double current_scale = scale();
            center.x() -= current_scale * increment.x();
            center.y() -= current_scale * increment.y();
        }
    }

    void update_scroll(double /*x*/, double y) {
        double zoom_factor = 0.1 * -y;
        Eigen::Vector2f new_half_dim = half_dim + zoom_factor * half_dim;

        if (new_half_dim.x() < kMinHalfDim.x() || new_half_dim.y() < kMinHalfDim.y()) {
            new_half_dim = kMinHalfDim;
        } else if (new_half_dim.x() > kMaxHalfDim.x() || new_half_dim.y() > kMaxHalfDim.y()) {
            new_half_dim = kMaxHalfDim;
        }

        double translate_factor = new_half_dim.norm() / half_dim.norm() - 1;
        center += translate_factor * (center - mouse_position);
        half_dim = new_half_dim;

        update_mouse_position();
    }

    void click() { clicked = true; }

    void release() { clicked = false; }

    Eigen::Matrix3f get_screen_from_world() const {
        using Transform = Eigen::Matrix3f;

        Transform translate = Transform::Identity();
        Transform scale = Transform::Identity();

        translate(0, 2) = -center.x();
        translate(1, 2) = -center.y();
        scale.diagonal() = Eigen::Vector3f{1 / half_dim.x(), -1 / half_dim.y(), 1};

        return scale * translate;
    }

    void reset() {
        center = kInitialHalfDim;
        half_dim = kInitialHalfDim;
        update_mouse_position();
    }

    double scale() const { return (half_dim).norm() / kInitialHalfDim.norm(); }

    void update_mouse_position() {
        Eigen::Vector2f top_left = center - half_dim;
        Eigen::Vector2f bottom_right = center + half_dim;
        Eigen::Vector2f screen_position{previous_position.x() / width_, previous_position.y() / height_};
        mouse_position = screen_position.cwiseProduct(bottom_right - top_left) + top_left;
    }

private:
    const size_t height_;
    const size_t width_;

    const Eigen::Vector2f kInitialHalfDim;
    const Eigen::Vector2f kMinHalfDim;
    const Eigen::Vector2f kMaxHalfDim;

    Eigen::Vector2f center = kInitialHalfDim;
    Eigen::Vector2f half_dim = kInitialHalfDim;

    Eigen::Vector2f previous_position;

    Eigen::Vector2f mouse_position;

    bool clicked = false;
};
}  // namespace engine
