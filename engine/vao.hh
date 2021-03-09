#pragma once

#include <limits>
#include <optional>

namespace engine {
class VertexArrayObject {
private:
    class ScopedBinder {
    public:
        ScopedBinder(VertexArrayObject& parent);
        ~ScopedBinder();

    private:
        VertexArrayObject& parent_;
    };

public:
    VertexArrayObject() = default;
    ~VertexArrayObject();

    void init();
    void bind();
    void unbind();

    ScopedBinder scoped_bind();

private:
    std::optional<unsigned int> handle_;
};

#define scoped_vao_bind(vao) auto _vao_##__LINE__ = vao.scoped_bind()
}  // namespace engine
