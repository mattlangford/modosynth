#pragma once
#include <OpenGL/gl3.h>

#include <Eigen/Dense>
#include <array>
#include <iostream>
#include <optional>
#include <variant>
#include <vector>

#include "engine/gl.hh"
#include "engine/vao.hh"

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
class Buffer_ {
public:
    Buffer_() = default;
    ~Buffer_() { reset_buffers(); }

public:
    void init(int location) { location_ = location; }

public:
    // TODO: maybe add_batch?
    size_t add(const Primitive<T, Dim>& primitive) {
        const size_t index = get_index_count();
        std::visit(AddImpl{vertices_, indices_}, primitive);

        reset_buffers();
        buffer_ids_.emplace();
        auto& [vertex, element] = *buffer_ids_;

        gl_safe(glGenBuffers, 2, buffer_ids_->data());

        // Since a new element was added, we'll need to update the buffers. Assume neither will be updated often at
        // first,
        gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex);
        gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_STATIC_DRAW);
        gl_safe(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, element);
        gl_safe(glBufferData, GL_ELEMENT_ARRAY_BUFFER, size_in_bytes(indices_), indices_.data(), GL_STATIC_DRAW);
        gl_safe(glEnableVertexAttribArray, location_);
        gl_safe(glVertexAttribPointer, location_, Dim, GL_FLOAT, GL_FALSE, 0, 0);

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
        if (!buffer_ids_) {
            return;
        }
        auto& [vertex, element] = *buffer_ids_;

        // Only need to update the vertex array since the number of elements didn't change
        gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex);
        gl_safe(glBufferData, GL_ARRAY_BUFFER, size_in_bytes(vertices_), vertices_.data(), GL_STREAM_DRAW);
    }

    void print() const {
        for (unsigned int index : indices_)
            std::cout << "Index: " << index << ", v: " << vertices_[index].transpose() << "\n";
    }
    void bind() {
        auto& [vertex, element] = *buffer_ids_;
        gl_safe(glBindBuffer, GL_ARRAY_BUFFER, vertex);
        gl_safe(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, element);
    }
    size_t get_index_count() const { return indices_.size(); }

private:
    void reset_buffers() {
        if (buffer_ids_) {
            gl_safe(glDeleteBuffers, 2, buffer_ids_->data());
            buffer_ids_.reset();
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
    int location_;
    std::optional<std::array<unsigned int, 2>> buffer_ids_;

    std::vector<Storage> vertices_;
    std::vector<unsigned int> indices_;
};

#define define_for(type, type_short, dim)                       \
    using Point##dim##D##type_short = Point<type, dim>;         \
    using Line##dim##D##type_short = Line<type, dim>;           \
    using Triangle##dim##D##type_short = Triangle<type, dim>;   \
    using Quad##dim##D##type_short = Quad<type, dim>;           \
    using Primitive##dim##D##type_short = Primitive<type, dim>; \
    using Buffer##dim##D##type_short = Buffer_<type, dim>;

define_for(float, f, 2);
define_for(float, f, 3);



///
/// @brief Handles getting data to the GPU. T is assumed to be something 
///
template <typename T>
class Buffer
{

public:
    Buffer() = default;
    ~Buffer();

public:
    ///
    /// @brief Initialize with the vertex attribute index, the stride (in elements) between elements, and a VAO to bind
    ///
    void init(unsigned int index, size_t stride, std::shared_ptr<VertexArrayObject>& vao)
    {
        index_ = index;
        stride_ = stride;
        vao_ = std::move(vao);

        handle_.emplace();
        scoped_vao_ptr_bind(vao_);
        gl_safe(glGenBuffers, 1, &*handle_);
    }

    void bind()
    {
    }

    void unbind()
    {
    }

private:
    void rebind()
    {
    }

    void sync()
    {
    }

    class BatchedUpdateBuffer
    {
    private:
        BatchedUpdateBuffer(Buffer<T>& parent) : parent_(parent), initial_start_(parent_.data_.data()) {}

        ~BatchedUpdateBuffer()
        {
            // Reallocation happened
            if (initial_start_ != parent_.data_.data())
            {
                parent_.rebind();
            }
            // Data was only modified
            else if (modified_)
            {
                parent_.sync();
            }
        }

    public:
        void reserve(size_t new_size)
        {
            parent_.data_.reserve(new_size);
        }
        void resize(size_t new_size)
        {
            parent_.data_.resize(new_size);
        }
        void push_back(const T& t)
        {
            parent_.data_.push_back(t);
        }
        T& operator[](size_t index)
        {
            modified_ = true;
            return parent_.data_.at(index);
        }

    public:
        // modified but not reallocated
        bool modified_ = false;
        Buffer<T> parent_;
        const T* initial_start_;
    };

public:
    void reserve(size_t new_size);
    void resize(size_t new_size);
    void push_back(const T& t);
    T& operator[](size_t index);
    const T& operator[](size_t index) const;

    BatchedUpdateBuffer batched_updater();


private:
    unsigned int index_;
    size_t stride_;
    std::shared_ptr<VertexArrayObject> vao_;

    std::optional<unsigned int> handle_;
    std::vector<T> data_;
};

}  // namespace engine
