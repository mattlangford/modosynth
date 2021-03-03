#pragma once
#include <Eigen/Dense>
#include <memory>

#include "engine/events.hh"
#include "engine/shader.hh"

namespace engine {
class AbstractObjectManager {
public:
    virtual ~AbstractObjectManager() = default;

    ///
    /// @brief Initialize the object manager
    ///
    virtual void init() = 0;

    ///
    /// @brief Render objects held by the manager
    ///
    virtual void render(const Eigen::Matrix3f& screen_from_world) = 0;

    ///
    /// @brief Update all of the objects held by the manager
    ///
    virtual void update(float dt) = 0;

    ///
    /// @brief Update mouse/keyboard events for each object
    ///
    virtual void handle_mouse_event(const MouseEvent& event) = 0;
    virtual void handle_keyboard_event(const KeyboardEvent& event) = 0;
};

//
// #############################################################################
//

///
/// @brief Provides basic Vertex Array Object support as well as automatically setting the screen_from_world matrix
///
class AbstractSingleShaderObjectManager : public AbstractObjectManager {
public:
    AbstractSingleShaderObjectManager(std::string vertex, std::string fragment,
                                      std::optional<std::string> geometry = std::nullopt);
    virtual ~AbstractSingleShaderObjectManager() = default;

    void init() override final;

    void render(const Eigen::Matrix3f& screen_from_world) override final;

protected:
    virtual void init_with_vao() = 0;
    virtual void render_with_vao() = 0;

    void bind_vao();
    const Shader& get_shader() const;

private:
    unsigned int vertex_array_object_;
    int screen_from_world_loc_;

    Shader shader_;
};
}  // namespace engine
