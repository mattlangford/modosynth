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

class GridObjectManager final : public engine::AbstractSingleShaderObjectManager {
public:
    GridObjectManager(const size_t grid_width, const size_t grid_height);
    virtual ~GridObjectManager() = default;

protected:
    void init_with_vao() override;

    void render_with_vao() override;

public:
    void update(float dt) override;

    void handle_mouse_event(const engine::MouseEvent& event) override;

    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

private:
    const size_t width_;
    const size_t height_;

    engine::Buffer2Df buffer_;
};
}  // namespace objects
