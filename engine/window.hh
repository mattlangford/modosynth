#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Eigen/Dense>
#include <iostream>

#include "engine/gl.hh"

namespace engine {

///
/// @brief callbacks for the GLFW events that need to be handled. These will use the global Window instance.
///
void scroll_callback(GLFWwindow* /*window*/, double scroll_x, double scroll_y);
void cursor_position_callback(GLFWwindow* /*window*/, double pos_x, double pos_y);
void mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int /*mods*/);
void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/);

//
// #############################################################################
//

class Window {
private:
    static Window* instance_;

public:
    Window(size_t height, size_t width);
    ~Window();

public:
    static Window* instance_ptr();
    static Window& instance();

public:
    void init();
    bool render_loop();
    void reset();

public:
    void update_mouse_position(double x, double y);
    void update_mouse_position_incremental(Eigen::Vector2f increment);
    void update_scroll(double x, double y);
    void set_clicked(bool clicked);

private:
    double scale() const;
    void update_mouse_position();
    Eigen::Matrix3f get_screen_from_world() const;

private:
    GLFWwindow* window_;
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
