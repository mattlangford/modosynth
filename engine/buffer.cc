#include "engine/buffer.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"

namespace engine {
template <typename T>
size_t size_in_bytes(const std::vector<T>& vec) {
    return vec.size() * sizeof(T);
}

//
// #############################################################################
//

Buffer2Df::Buffer2Df() {}

//
// #############################################################################
//

Buffer2Df::~Buffer2Df() { reset_buffers(); }

//
// #############################################################################
//

void Buffer2Df::init(int location) {
    gl_safe(glGenBuffers, 1, &vertex_buffer_);
    gl_safe(glGenBuffers, 1, &element_buffer_);

    gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_);
    // gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_STATIC_DRAW);

    gl_safe(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, element_buffer_);
    // gl_safe(glBufferData, GL_ELEMENT_ARRAY_BUFFER, size_in_bytes(indices_), indices_.data(), GL_STATIC_DRAW);

    gl_safe(glEnableVertexAttribArray, location);
    gl_safe(glVertexAttribPointer, location, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

//
// #############################################################################
//

size_t Buffer2Df::add(const Primitive& primitive) {
    struct AddImpl {
        std::vector<Eigen::Vector2f>& vertices;
        std::vector<unsigned int>& indices;

        void operator()(const Point&) { throw std::runtime_error("Only supporting Quads in primitive variant."); }
        void operator()(const Line&) { throw std::runtime_error("Only supporting Quads in primitive variant."); }
        void operator()(const Triangle&) { throw std::runtime_error("Only supporting Quads in primitive variant."); }
        void operator()(const Quad& q) {
            enum Ordering : int8_t { kTopLeft = 0, kTopRight = 1, kBottomLeft = 2, kBottomRight = 3, kSize = 4 };
            const size_t start = vertices.size();

            // 4 coordinates per object
            vertices.emplace_back(q.top_left);
            vertices.emplace_back(q.top_right);
            vertices.emplace_back(q.bottom_left);
            vertices.emplace_back(q.bottom_right);

            // Upper triangle
            indices.emplace_back(start + Ordering::kTopLeft);
            indices.emplace_back(start + Ordering::kTopRight);
            indices.emplace_back(start + Ordering::kBottomLeft);

            // Lower triangle
            indices.emplace_back(start + Ordering::kBottomRight);
            indices.emplace_back(start + Ordering::kBottomLeft);
            indices.emplace_back(start + Ordering::kTopRight);
        }
    };

    const size_t index = vertices_.size();
    std::visit(AddImpl{vertices_, indices_}, primitive);

    // Since a  new element was added, we'll need to update the buffers. Assume neither will be updated often at first,
    gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_);
    gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_STATIC_DRAW);
    gl_safe(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, element_buffer_);
    gl_safe(glBufferData, GL_ELEMENT_ARRAY_BUFFER, size_in_bytes(indices_), indices_.data(), GL_STATIC_DRAW);

    return index;
}

//
// #############################################################################
//

void Buffer2Df::update(const Primitive& primitive, size_t& index) {
    struct UpdateImpl {
        size_t& index;
        std::vector<Eigen::Vector2f>& vertices;

        void operator()(const Point&) { throw std::runtime_error("Only supporting Quads in primitive variant."); }
        void operator()(const Line&) { throw std::runtime_error("Only supporting Quads in primitive variant."); }
        void operator()(const Triangle&) { throw std::runtime_error("Only supporting Quads in primitive variant."); }
        void operator()(const Quad& q) {
            vertices.at(index++) = q.top_left;
            vertices.at(index++) = q.top_right;
            vertices.at(index++) = q.bottom_left;
            vertices.at(index++) = q.bottom_right;
        }
    };

    std::visit(UpdateImpl{index, vertices_}, primitive);

    // Only need to update the vertex array since the number of elements didn't change
    gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_);
    gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_STREAM_DRAW);
}

//
// #############################################################################
//

void Buffer2Df::reset_buffers() {
    if (vertex_buffer_ > 0) {
        glDeleteBuffers(1, &vertex_buffer_);
    }
    if (element_buffer_ > 0) {
        glDeleteBuffers(1, &element_buffer_);
    }
}
}  // namespace engine
