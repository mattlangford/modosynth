#include "engine/texture.hh"
#include <iostream>

namespace engine {

//
// #############################################################################
//

TextureManager::TextureManager(const std::filesystem::path& textures_folder)
{
    for (const auto & entry : std::filesystem::directory_iterator(textures_folder))
    {
        if (entry.path().extension() == "bmp")
        {
            unsigned int invalid_index = -1;
            textures_.emplace_back(Texture{invalid_index, Bitmap{entry}});
        }
    }

    std::cout << textures_.size() << " entries loaded\n";
}

//
// #############################################################################
//

void TextureManager::init()
{
}

//
// #############################################################################
//

void TextureManager::activate()
{
}

//
// #############################################################################
//

const std::vector<Texture>& TextureManager::get_textures()
{
    return textures_;
}
}  // namespace engine
