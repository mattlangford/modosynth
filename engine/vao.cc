#include "engine/vao.hh"

#include "engine/gl.hh"

namespace engine {

//
// #############################################################################
//

VertexArrayObject::ScopedBinder::ScopedBinder(const VertexArrayObject& parent) : parent_(parent) { parent_.bind(); }

//
// #############################################################################
//

VertexArrayObject::ScopedBinder::~ScopedBinder() { parent_.unbind(); }

//
// #############################################################################
//

VertexArrayObject::~VertexArrayObject() {
    if (handle_) {
        glDeleteVertexArrays(1, &*handle_);
    }
}

//
// #############################################################################
//

void VertexArrayObject::init() {
    auto& handle = handle_.emplace();
    gl_check(glGenVertexArrays, 1, &handle);
}

//
// #############################################################################
//

void VertexArrayObject::bind() const {
    if (!handle_) throw std::runtime_error("VertexArrayObject::bind() called before VertexArrayObject::init()");
    gl_check(glBindVertexArray, *handle_);
}

//
// #############################################################################
//

void VertexArrayObject::unbind() const { gl_check(glBindVertexArray, 0); }

//
// #############################################################################
//

VertexArrayObject::ScopedBinder VertexArrayObject::scoped_bind() const { return ScopedBinder{*this}; }
}  // namespace engine
