#include "engine/texture.hh"

#include <iostream>

#include "engine/gl.hh"

namespace engine {

//
// #############################################################################
//

Texture::Texture(const std::filesystem::path& texture) : id_(-1), bitmap_(texture) {}

//
// #############################################################################
//

void Texture::init() {
    gl_check(glGenTextures, 1, &id_);
    gl_check(glBindTexture, GL_TEXTURE_2D, id_);
    gl_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gl_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_check(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    gl_check(glPixelStorei, GL_UNPACK_ALIGNMENT, 1);
    gl_check(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, bitmap_.get_width(), bitmap_.get_height(), 0, GL_BGRA,
            GL_UNSIGNED_BYTE, bitmap_.get_pixels().data());
}

//
// #############################################################################
//

void Texture::activate() {
    if (id_ < 0) {
        throw std::runtime_error("Texture not initialized, did you call Texture::init()?");
    }

    gl_check(glBindTexture, GL_TEXTURE_2D, id_);
}

//
// #############################################################################
//

const Bitmap& Texture::bitmap() const { return bitmap_; }
}  // namespace engine
