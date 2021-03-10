#pragma once
#include <Eigen/Dense>
#include <filesystem>
#include <vector>

#include "engine/buffer.hh"
#include "engine/object_manager.hh"
#include "engine/shader.hh"

namespace objects {

//
// #############################################################################
//

class GridObjectManager final : public engine::AbstractObjectManager {
public:
    GridObjectManager(const size_t grid_width, const size_t grid_height);
    virtual ~GridObjectManager() = default;

protected:
    void init() override;

    void render(const Eigen::Matrix3f& screen_from_world) override;

public:
    void update(float dt) override;

    void handle_mouse_event(const engine::MouseEvent& event) override;

    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

private:
    const size_t width_;
    const size_t height_;

    int screen_from_world_location_;

    engine::Shader shader_;
    engine::VertexArrayObject vao_;
    engine::Buffer<float, 2> buffer_;
};
}  // namespace objects
