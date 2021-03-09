#include "engine/vao.hh"

#include "engine/gl.hh"

namespace engine {

//
// #############################################################################
//

VertexArrayObject::ScopedBinder::ScopedBinder(VertexArrayObject& parent) : parent_(parent) { parent_.bind(); }

//
// #############################################################################
//

VertexArrayObject::ScopedBinder::~ScopedBinder() { parent_.unbind(); }

//
// #############################################################################
//

VertexArrayObject::~VertexArrayObject() {
    if (handle_) glDeleteVertexArrays(1, &*handle_);
}

//
// #############################################################################
//

void VertexArrayObject::init() {
    auto& handle = handle_.emplace();
    gl_safe(glGenVertexArrays, 1, &handle);
}

//
// #############################################################################
//

void VertexArrayObject::bind() {
    if (!handle_) throw std::runtime_error("VertexArrayObject::bind() called before VertexArrayObject::init()");
    gl_safe(glBindVertexArray, *handle_);
}

//
// #############################################################################
//

void VertexArrayObject::unbind() { gl_safe(glBindVertexArray, 0); }

//
// #############################################################################
//

VertexArrayObject::ScopedBinder VertexArrayObject::scoped_bind() { return ScopedBinder{*this}; }
}  // namespace engine
