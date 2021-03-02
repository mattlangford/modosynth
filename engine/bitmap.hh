#pragma once
#include <filesystem>
#include <vector>

namespace engine {
class Bitmap {
private:
    struct FileHeader {
        // Should always be "BM" (19778)
        uint16_t type;

        // Size of the file in bytes
        uint32_t size;

        // Should be zero
        uint16_t reserved1;
        uint16_t reserved2;

        // Offset from beginning of file to image data (in bits, should be 1078)
        uint32_t offset;
    } __attribute__((packed));
    static_assert(sizeof(FileHeader) == 14);

    // NOTE: This can actually be quite a bit larger, but I'm only extracting a few fields
    struct InfoHeader {
        // Size of this info header (should be 40)
        uint32_t size;

        uint32_t width;
        uint32_t height;
        uint16_t planes;
        uint16_t bits_per_pixel;
        uint32_t compression;

        uint32_t image_size;

        uint32_t num_colors;
        uint32_t important_colors;
    };

public:
    // using Color = uint8_t;
    struct Color {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t alpha;
    };
    static_assert(sizeof(Color) == 4);

public:
    Bitmap(const std::filesystem::path& path);

    size_t get_width() const;
    size_t get_height() const;
    const std::vector<Color>& get_pixels() const;

private:
    FileHeader parse_file_header(std::istream& stream) const;
    InfoHeader parse_info_header(std::istream& stream) const;
    std::vector<Color> parse_pixels(std::istream& stream, const InfoHeader& header) const;

private:
    FileHeader file_header_;
    InfoHeader info_header_;

    // Row major starting from the TOP left of the image (this is flipped from how its stored on disk)
    std::vector<Color> pixels_;
};
}  // namespace engine
