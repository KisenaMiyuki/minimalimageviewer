#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Still JPEG XL frames decoded to tightly packed 8-bit straight RGBA.
struct JxlDecodedImage {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgba;
};

enum class JxlDecodeStatus {
    Ok,
    Error,
    TooLarge,
};

// Decodes the first frame of a JPEG XL codestream or container.
// Animated files return the first frame. Color is converted to sRGB.
JxlDecodeStatus DecodeJxlToRgba(const uint8_t* data, size_t size, JxlDecodedImage& image, std::string* error);
