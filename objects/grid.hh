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

class GridObjectManager : public engine::AbstractObjectManager {
public:
    GridObjectManager(const size_t grid_width, const size_t grid_height);
    virtual ~GridObjectManager() = default;

public:
    void init() override;

    void render(const Eigen::Matrix3f& screen_from_world) override;

    void update(float dt) override;

    void handle_mouse_event(const engine::MouseEvent& event) override;

    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

private:
    const size_t width_;
    const size_t height_;

    unsigned int vertex_array_object_;
    int screen_from_world_loc_;

    engine::Shader shader_;
    engine::Buffer2Df buffer_;
};
}  // namespace objects
