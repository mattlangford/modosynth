#include "engine/texture.hh"
#include "yaml-cpp/yaml.h"
#include <iostream>

namespace engine {

//
// #############################################################################
//

TextureManager::TextureManager(const std::filesystem::path& yml_path)
{
    auto yml = YAML::LoadFile(yml_path);

    Bitmap data(yml["path"].as<std::string>());

    for (const auto& block : yml["blocks"])
    {
        std::string name = block["name"].as<std::string>();
        size_t x = block["coord"][0].as<size_t>();
        size_t y = block["coord"][1].as<size_t>();
        size_t dim_x = block["dim"][0].as<size_t>();
        size_t dim_y = block["dim"][1].as<size_t>();

        size_t in = block["in"].as<size_t>();
        size_t out = block["out"].as<size_t>();
    }
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
