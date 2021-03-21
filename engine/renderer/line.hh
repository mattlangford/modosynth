#pragma once
#include <Eigen/Dense>
#include <vector>

#include "engine/buffer.hh"
#include "engine/gl.hh"
#include "engine/shader.hh"
#include "engine/vao.hh"

namespace engine::renderer {
struct Line {
    std::vector<Eigen::Vector2f> segments;
};

//
// #############################################################################
//

class LineRenderer {
public:
    LineRenderer();

    LineRenderer(const LineRenderer&) = delete;
    LineRenderer(LineRenderer&&) = delete;
    LineRenderer& operator=(const LineRenderer&) = delete;
    LineRenderer& operator=(LineRenderer&&) = delete;

public:
    void init();
    void draw(const Line& line, const Eigen::Matrix3f& screen_from_world);

private:
    void set_position(const std::vector<Eigen::Vector2f>& segments);
    void set_elements(size_t num_segments);

private:
    engine::Shader shader_;
    int screen_from_world_loc_;

    engine::VertexArrayObject vao_;
    engine::Buffer<float, 2> position_buffer_;
    engine::Buffer<unsigned int> element_buffer_;
};
}  // namespace engine::renderer
