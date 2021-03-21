#pragma once
#include "engine/buffer.hh"
#include "engine/gl.hh"
#include "engine/shader.hh"
#include "engine/texture.hh"
#include "engine/vao.hh"

namespace engine::renderer {

struct Box {
    Eigen::Vector2f center;
    Eigen::Vector2f dim;

    Eigen::Vector2f uv_center;
    size_t texture_index;
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
    size_t add_texture(Texture texture);

public:
    void init();
    void draw(const Box& box, const Eigen::Matrix3f& screen_from_world);

private:
    void set_uv(const Eigen::Vector2f& center, const Eigen::Vector2f& dim);
    void set_position(const Eigen::Vector2f& center, const Eigen::Vector2f& dim);

private:
    engine::Shader shader_;
    int screen_from_world_loc_;

    engine::VertexArrayObject vao_;
    engine::Buffer<float, 2> position_buffer_;
    engine::Buffer<float, 2> uv_buffer_;

    std::vector<Texture> textures_;
};
}  // namespace engine::renderer
