#include "bitmap.hh"

#include <cmath>
#include <fstream>
#include <iostream>

//
// #############################################################################
//

namespace {
template <typename T>
T read(std::istream& stream) {
    T t;
    stream.read(reinterpret_cast<char*>(&t), sizeof(T));
    return t;
}
}  // namespace

//
// #############################################################################
//

Bitmap::Bitmap(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);

    file_header_ = parse_file_header(stream);
    if (file_header_.size != std::filesystem::file_size(path)) {
        throw std::runtime_error("Header size mismatch.");
    }

    info_header_ = parse_info_header(stream);

    pixels_ = parse_pixels(stream, info_header_);
}

//
// #############################################################################
//

auto Bitmap::parse_file_header(std::istream& stream) const -> FileHeader {
    FileHeader header = read<FileHeader>(stream);
    if (header.type != 19778)  // "BM"
    {
        throw std::runtime_error("Invalid header type.");
    }
    if (header.reserved1 != 0 || header.reserved2 != 0) {
        throw std::runtime_error("Reserved bytes should be zero.");
    }
    return header;
}

//
// #############################################################################
//

auto Bitmap::parse_info_header(std::istream& stream) const -> InfoHeader {
    InfoHeader header = read<InfoHeader>(stream);

    switch (header.size) {
        // The header we're using can only be parsed if the header size is one of these:
        case 40:
        case 52:
        case 56:
        case 108:
        case 124:
            break;
        default:
            throw std::runtime_error("Invalid InfoHeader size: " + std::to_string(header.size));
    }

    if (header.planes != 1) {
        throw std::runtime_error("Only supporting 1 plane.");
    }

    if (header.compression != 0) {
        throw std::runtime_error("Unable to handle compression.");
    }

    // We've read to the end of the InfoHeader, but the actual info header will be longer (since we didn't read some
    // of the data). Seek to the end for color and pixel parsing.
    stream.seekg(header.size - sizeof(InfoHeader), std::ios::cur);

    return header;
}

//
// #############################################################################
//

auto Bitmap::parse_pixels(std::istream& stream, const InfoHeader& header) const -> std::vector<Color> {
    std::vector<char> pixel_bytes;
    pixel_bytes.resize(header.image_size);
    stream.read(pixel_bytes.data(), pixel_bytes.size());

    const size_t num_pixels = header.width * header.height;
    const size_t row_bytes = std::ceil(header.bits_per_pixel * header.width / 32) / 4;

    std::vector<Color> pixels;
    pixels.reserve(num_pixels);

    for (size_t row = 0; row < header.height; ++row) {
        size_t index = row * row_bytes;
        for (size_t col = 0; col < header.height; ++col) {
            auto& pixel = pixels.emplace_back();
            pixel.blue = pixel_bytes.at(index++);
            pixel.green = pixel_bytes.at(index++);
            pixel.red = pixel_bytes.at(index++);
        }
    }

    return pixels;
}
