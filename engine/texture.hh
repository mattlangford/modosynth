#pragma once
#include <filesystem>
#include <vector>
#include "engine/bitmap.hh"

namespace engine {

struct Texture
{
    unsigned int id;
    Bitmap bitmap;
};

//
// #############################################################################
//

class TextureManager {
public:
    TextureManager(const std::filesystem::path& yml_path);

    void init();
    void activate();

    const std::vector<Texture>& get_textures();

private:
    std::vector<Texture> textures_;

};
}  // namespace engine
