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

size_t Bitmap::get_width() const { return info_header_.width; }

//
// #############################################################################
//

size_t Bitmap::get_height() const { return info_header_.height; }

//
// #############################################################################
//

auto Bitmap::get_pixels() const -> const std::vector<Color>& { return pixels_; }

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

    if (header.bits_per_pixel != 24) {
        throw std::runtime_error("Only supporting 24 bits per pixel.");
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
    std::vector<uint8_t> pixel_bytes;
    pixel_bytes.resize(header.image_size);
    stream.read(reinterpret_cast<char*>(pixel_bytes.data()), pixel_bytes.size());

    const size_t num_pixels = header.width * header.height;

    std::vector<Color> pixels;
    pixels.reserve(3 * num_pixels);

    size_t index = 0;
    for (size_t row = 0; row < header.height; ++row) {
        // Make sure the row starting indices 4 byte aligned
        if (index % 4 != 0) {
            index += std::remainder(index, 4);
        }

        for (size_t col = 0; col < header.width; ++col) {
            auto& pixel = pixels.emplace_back();
            pixel.blue = pixel_bytes[index++];
            pixel.green = pixel_bytes[index++];
            pixel.red = pixel_bytes[index++];
        }
    }

    return pixels;
}
