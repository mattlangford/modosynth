#pragma once

#include <limits>
#include <optional>

namespace engine {
class VertexArrayObject {
private:
    class ScopedBinder {
    public:
        ScopedBinder(const VertexArrayObject& parent);
        ~ScopedBinder();

    private:
        const VertexArrayObject& parent_;
    };

public:
    VertexArrayObject() = default;
    ~VertexArrayObject();

    void init();
    void bind() const;
    void unbind() const;

    ScopedBinder scoped_bind() const;

private:
    std::optional<unsigned int> handle_;
};
}  // namespace engine

#define scoped_vao_bind(vao) auto _vao_##__LINE__ = vao.scoped_bind()
#define scoped_vao_ptr_bind(vao) auto _vao_##__LINE__ = vao->scoped_bind()
