#pragma once
#include <filesystem>
#include <vector>

#include "engine/bitmap.hh"

namespace engine {

//
// #############################################################################
//

class Texture {
public:
    Texture(const std::filesystem::path& texture);

    void init();
    void activate();

    const Bitmap& bitmap() const;

private:
    unsigned int id_;
    const Bitmap bitmap_;
};
}  // namespace engine
