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

///
/// @brief Handles getting data to the GPU
///
template <typename T, size_t Stride = 1>
class Buffer {
    static_assert(std::is_arithmetic_v<T>);
    static_assert(Stride >= 1);

public:
    Buffer() = default;

    ~Buffer() {
        if (handle_) {
            gl_check(glDeleteBuffers, 1, &handle());
        }
    }

public:
    void init(GLenum target, const VertexArrayObject& vao) {
        vao_ = &vao;
        set_vertex_attribute_ = []() {};
        target_ = target;

        handle_.emplace();
        gl_check(glGenBuffers, 1, &handle());
    }

    void init(GLenum target, unsigned int index, const VertexArrayObject& vao) {
        init(target, vao);

        // Enable this index, only needs to be done once per index
        gl_check_with_vao(this->vao(), glEnableVertexAttribArray, index);

        set_vertex_attribute_ = [index, this]() {
            gl_check_with_vao(this->vao(), glVertexAttribPointer, index, Stride, enum_type<T>, GL_FALSE, 0, nullptr);
        };
    }

    void unbind() { gl_check(glBindBuffer, target_, 0); }

private:
    const VertexArrayObject& vao() {
        if (!vao_) throw std::runtime_error("VertexArrayObject not bound, has Buffer::init() been called?");
        return *vao_;
    }
    unsigned int& handle() {
        if (!handle_) throw std::runtime_error("No buffer generated, has Buffer::init() been called?");
        return *handle_;
    }

    ///
    /// @brief Deletes the current buffer and generates a new one. This should be called anytime the size changes
    ///
    void rebind() {
        // Some buffer targets require the VAO to be bound (GL_ELEMENT_ARRAY_BUFFER for example)
        scoped_vao_ptr_bind(vao_);
        if (handle_) gl_check(glDeleteBuffers, 1, &handle());

        handle_.emplace();
        gl_check(glGenBuffers, 1, &handle());
        sync();
        set_vertex_attribute_();
    }

    ///
    /// @brief Used when the size of the buffer doesn't change, only the data within
    ///
    void sync() {
        gl_check(glBindBuffer, target_, handle());
        gl_check(glBufferData, target_, sizeof(T) * data_.size(), data_.data(),
                 dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        dynamic_ = true;  // in case sync is called more than once...
    }

    class BatchedUpdateBuffer {
    public:
        BatchedUpdateBuffer(Buffer<T, Stride>& parent) : parent_(parent), initial_start_(parent_.data_.data()) {}
        ~BatchedUpdateBuffer() { finish(); }

        void finish() {
            // Reallocation happened
            if (initial_start_ != parent_.data_.data()) {
                parent_.rebind();
                initial_start_ = parent_.data_.data();
            }
            // Data was only modified
            else if (modified_) {
                parent_.sync();
                modified_ = false;
            }

            parent_.unbind();
        }

    public:
        void reserve(size_t new_size) { parent_.data_.reserve(new_size); }
        void resize(size_t new_size) { parent_.data_.resize(new_size); }
        size_t size() const { return parent_.data_.size(); }
        void push_back(const T& t) {
            modified_ = true;
            parent_.data_.push_back(t);
        }
        T& operator[](size_t index) {
            modified_ = true;
            return parent_.data_.at(index);
        }
        template <size_t Rows>
        Eigen::Map<Eigen::Matrix<T, Stride, static_cast<int>(Rows)>> elements(size_t index) {
            modified_ = true;

            // Since the map doesn't handle resizing, we'll do that here. Could use resize() but I want to make sure the
            // size doubling happens automatically.
            size_t required_size = Stride * (index + Rows);
            while (parent_.size() < required_size) push_back({});

            return Eigen::Map<Eigen::Matrix<T, Stride, static_cast<int>(Rows)>>{&parent_.data_.at(Stride * index)};
        }
        Eigen::Map<Eigen::Matrix<T, Stride, 1>> element(size_t index) { return elements<1>(index); }

    private:
        // modified but not reallocated
        bool modified_ = false;
        Buffer<T, Stride>& parent_;
        T* initial_start_;
    };

    // Mapping between Type and the GLenum that represents that type
    template <typename Type>
    static constexpr GLenum enum_type = -1;
    template <>
    static constexpr GLenum enum_type<float> = GL_FLOAT;
    template <>
    static constexpr GLenum enum_type<double> = GL_DOUBLE;
    template <>
    static constexpr GLenum enum_type<unsigned int> = GL_UNSIGNED_INT;

public:
    void reserve(size_t new_size) { batched_updater().reserve(new_size); }
    void resize(size_t new_size) { batched_updater().resize(new_size); }
    void push_back(const T& t) { batched_updater().push_back(t); }
    T& operator[](size_t index) { return batched_updater()[index]; }

    const T& operator[](size_t index) const { return data_[index]; }
    const T& element(size_t index) const { return data_[Stride * index]; }

    size_t size() const { return data_.size(); }
    size_t elements() const { return data_.size() / Stride; }

    BatchedUpdateBuffer batched_updater() { return {*this}; }

    std::string print() const {
        std::stringstream res;
        res << "[";
        for (size_t i = 0; i < data_.size(); ++i) {
            if ((i % Stride == 0) && i > 0) {
                res << "]\n[";
            }
            res << data_[i] << ", ";
        }
        res << "]";
        return res.str();
    }

private:
    GLenum target_;
    const VertexArrayObject* vao_ = nullptr;  // we don't own this
    std::function<void()> set_vertex_attribute_;

    std::optional<unsigned int> handle_ = std::nullopt;
    std::vector<T> data_;
    bool dynamic_ = false;  // assume it's static, this will change after the first sync()
};

}  // namespace engine
