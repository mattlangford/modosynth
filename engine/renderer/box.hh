#pragma once
#include "engine/buffer.hh"
#include "engine/gl.hh"
#include "engine/shader.hh"
#include "engine/vao.hh"

namespace engine::renderer {

struct Box {
    Eigen::Vector2f center;
    Eigen::Vector2f dim;
    Eigen::Vector3f color;
};

class BoxRenderer {
public:
    BoxRenderer();
    ~BoxRenderer() = default;

    BoxRenderer(const BoxRenderer&) = delete;
    BoxRenderer(BoxRenderer&&) = delete;
    BoxRenderer& operator=(const BoxRenderer&) = delete;
    BoxRenderer& operator=(BoxRenderer&&) = delete;

public:
    void init();
    void operator()(const Box& box, const Eigen::Matrix3f& screen_from_world);

private:
    void set_color(const Eigen::Vector3f& color);
    void set_position(const Eigen::Vector2f& center, const Eigen::Vector2f& dim);

private:
    engine::Shader shader_;
    int screen_from_world_loc_;

    engine::VertexArrayObject vao_;
    engine::Buffer<float, 2> position_buffer_;
    engine::Buffer<float, 4> color_buffer_;
};
}  // namespace engine::renderer
