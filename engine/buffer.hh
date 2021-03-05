#pragma once
#include <OpenGL/gl3.h>

#include <Eigen/Dense>
#include <iostream>
#include <variant>
#include <vector>

#include "engine/gl.hh"

namespace engine {

//
// #############################################################################
//

template <typename T, int Dim>
struct Point {
    using Storage = Eigen::Matrix<T, Dim, 1>;
    Storage point;
};

template <typename T, int Dim>
struct Line {
    using Storage = Eigen::Matrix<T, Dim, 1>;
    Storage start;
    Storage end;
};

template <typename T, int Dim>
struct Triangle {
    using Storage = Eigen::Matrix<T, Dim, 1>;
    Storage p0;
    Storage p1;
    Storage p2;
};

template <typename T, int Dim>
struct Quad {
    using Storage = Eigen::Matrix<T, Dim, 1>;
    Storage top_left;
    Storage top_right;
    Storage bottom_left;
    Storage bottom_right;
};

template <typename T, int Dim>
using Primitive = std::variant<Point<T, Dim>, Line<T, Dim>, Triangle<T, Dim>, Quad<T, Dim>>;

//
// #############################################################################
//

template <typename T, int Dim>
class Buffer {
public:
    Buffer() = default;
    ~Buffer() { reset_buffers(); }

public:
    void init(int location) {
        gl_safe(glGenBuffers, 1, &vertex_buffer_);
        gl_safe(glGenBuffers, 1, &element_buffer_);

        gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_);
        gl_safe(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, element_buffer_);

        gl_safe(glEnableVertexAttribArray, location);
        gl_safe(glVertexAttribPointer, location, Dim, GL_FLOAT, GL_FALSE, 0, 0);
    }

public:
    // TODO: maybe add_batch?
    size_t add(const Primitive<T, Dim>& primitive) {
        const size_t index = get_index_count();
        std::visit(AddImpl{vertices_, indices_}, primitive);

        // Since a new element was added, we'll need to update the buffers. Assume neither will be updated often at
        // first,
        gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_);
        gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_STATIC_DRAW);
        gl_safe(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, element_buffer_);
        gl_safe(glBufferData, GL_ELEMENT_ARRAY_BUFFER, size_in_bytes(indices_), indices_.data(), GL_STATIC_DRAW);

        return index;
    }

    void update(const Primitive<T, Dim>& primitive, size_t& index) {
        update_batch(primitive, index);
        finish_batch();
    }

    void update_batch(const Primitive<T, Dim>& primitive, size_t& index) {
        std::visit(UpdateImpl{index, vertices_}, primitive);
    }

    void finish_batch() {
        // Only need to update the vertex array since the number of elements didn't change
        gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_);
        gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_STREAM_DRAW);
    }

    void print() const {
        for (unsigned int index : indices_)
            std::cout << "Index: " << index << ", v: " << vertices_[index].transpose() << "\n";
    }
    void bind() {
        gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_);
        gl_safe(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, element_buffer_);
    }
    size_t get_index_count() const { return indices_.size(); }

private:
    void reset_buffers() {
        if (vertex_buffer_ > 0) {
            glDeleteBuffers(1, &vertex_buffer_);
        }
        if (element_buffer_ > 0) {
            glDeleteBuffers(1, &element_buffer_);
        }
    }

    template <typename Data>
    static size_t size_in_bytes(const std::vector<Data>& vec) {
        return vec.size() * sizeof(Data);
    }

    using Point = Point<T, Dim>;
    using Line = Line<T, Dim>;
    using Triangle = Triangle<T, Dim>;
    using Quad = Quad<T, Dim>;
    using Primitive = Primitive<T, Dim>;
    using Storage = Eigen::Matrix<T, Dim, 1>;

    struct AddImpl {
        std::vector<Storage>& vertices;
        std::vector<unsigned int>& indices;

        void operator()(const Point& p) {
            indices.emplace_back(vertices.size());
            vertices.emplace_back(p.point);
        }
        void operator()(const Line& l) {
            const size_t start = vertices.size();
            vertices.emplace_back(l.start);
            vertices.emplace_back(l.end);

            indices.emplace_back(start);
            indices.emplace_back(start + 1);
        }
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

    struct UpdateImpl {
        size_t& index;
        std::vector<Storage>& vertices;

        void operator()(const Point& p) { vertices.at(index++) = p.point; }
        void operator()(const Line& l) {
            vertices.at(index++) = l.start;
            vertices.at(index++) = l.end;
        }
        void operator()(const Triangle&) { throw std::runtime_error("Only supporting Quads in primitive variant."); }
        void operator()(const Quad& q) {
            vertices.at(index++) = q.top_left;
            vertices.at(index++) = q.top_right;
            vertices.at(index++) = q.bottom_left;
            vertices.at(index++) = q.bottom_right;
        }
    };

private:
    unsigned int vertex_buffer_ = -1;
    unsigned int element_buffer_ = -1;

    std::vector<Storage> vertices_;
    std::vector<unsigned int> indices_;
};

#define define_for(type, type_short, dim)                       \
    using Point##dim##D##type_short = Point<type, dim>;         \
    using Line##dim##D##type_short = Line<type, dim>;           \
    using Triangle##dim##D##type_short = Triangle<type, dim>;   \
    using Quad##dim##D##type_short = Quad<type, dim>;           \
    using Primitive##dim##D##type_short = Primitive<type, dim>; \
    using Buffer##dim##D##type_short = Buffer<type, dim>;

define_for(float, f, 2);
define_for(float, f, 3);

}  // namespace engine
