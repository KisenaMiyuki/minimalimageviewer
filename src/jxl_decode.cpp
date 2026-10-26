#include "jxl_decode.h"

#ifndef JXL_STATIC_DEFINE
#define JXL_STATIC_DEFINE
#endif
#ifndef JXL_CMS_STATIC_DEFINE
#define JXL_CMS_STATIC_DEFINE
#endif

#include <jxl/cms.h>
#include <jxl/codestream_header.h>
#include <jxl/color_encoding.h>
#include <jxl/decode.h>
#include <jxl/types.h>

#include <cstring>
#include <new>

namespace {

constexpr uint64_t kMaxJxlPixels = 64ull * 1000ull * 1000ull;

struct DecoderGuard {
    JxlDecoder* dec = nullptr;
    ~DecoderGuard() {
        if (dec) {
            JxlDecoderDestroy(dec);
        }
    }
};

void SetError(std::string* error, const char* message) {
    if (error) {
        *error = message;
    }
}

void SetSrgbEncoding(JxlColorEncoding& encoding, bool gray) {
    std::memset(&encoding, 0, sizeof(encoding));
    encoding.color_space = gray ? JXL_COLOR_SPACE_GRAY : JXL_COLOR_SPACE_RGB;
    encoding.white_point = JXL_WHITE_POINT_D65;
    encoding.primaries = JXL_PRIMARIES_SRGB;
    encoding.transfer_function = JXL_TRANSFER_FUNCTION_SRGB;
    encoding.rendering_intent = JXL_RENDERING_INTENT_RELATIVE;
}

bool ExpandToRgba(const std::vector<uint8_t>& src, uint32_t width, uint32_t height, uint32_t channels, std::vector<uint8_t>& rgba) {
    const uint64_t pixels = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
    if (channels == 0 || src.size() < pixels * channels) {
        return false;
    }

    rgba.resize(static_cast<size_t>(pixels * 4ull));
    const uint8_t* in = src.data();
    uint8_t* out = rgba.data();

    if (channels == 4) {
        std::memcpy(out, in, rgba.size());
        return true;
    }

    for (uint64_t i = 0; i < pixels; ++i) {
        const uint8_t* p = in + i * channels;
        uint8_t* d = out + i * 4ull;
        if (channels == 3) {
            d[0] = p[0];
            d[1] = p[1];
            d[2] = p[2];
            d[3] = 255;
        }
        else if (channels == 2) {
            d[0] = p[0];
            d[1] = p[0];
            d[2] = p[0];
            d[3] = p[1];
        }
        else if (channels == 1) {
            d[0] = p[0];
            d[1] = p[0];
            d[2] = p[0];
            d[3] = 255;
        }
        else {
            return false;
        }
    }
    return true;
}

}  // namespace

JxlDecodeStatus DecodeJxlToRgba(const uint8_t* data, size_t size, JxlDecodedImage& image, std::string* error) {
    image = {};
    if (!data || size < 2) {
        SetError(error, "empty JPEG XL buffer");
        return JxlDecodeStatus::Error;
    }

    const JxlSignature signature = JxlSignatureCheck(data, size);
    if (signature != JXL_SIG_CODESTREAM && signature != JXL_SIG_CONTAINER) {
        SetError(error, "not a JPEG XL image");
        return JxlDecodeStatus::Error;
    }

    DecoderGuard guard;
    guard.dec = JxlDecoderCreate(nullptr);
    if (!guard.dec) {
        SetError(error, "out of memory");
        return JxlDecodeStatus::Error;
    }

    if (JxlDecoderSetCms(guard.dec, *JxlGetDefaultCms()) != JXL_DEC_SUCCESS) {
        SetError(error, "color management setup failed");
        return JxlDecodeStatus::Error;
    }
    JxlDecoderSetUnpremultiplyAlpha(guard.dec, JXL_TRUE);

    if (JxlDecoderSubscribeEvents(guard.dec, JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING | JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS) {
        SetError(error, "could not subscribe to decoder events");
        return JxlDecodeStatus::Error;
    }

    JxlDecoderSetInput(guard.dec, data, size);
    JxlDecoderCloseInput(guard.dec);

    JxlBasicInfo info = {};
    bool haveInfo = false;
    bool wroteFrame = false;
    std::vector<uint8_t> packed;
    uint32_t channels = 0;

    for (;;) {
        const JxlDecoderStatus status = JxlDecoderProcessInput(guard.dec);
        if (status == JXL_DEC_ERROR) {
            SetError(error, "JPEG XL decode failed");
            return JxlDecodeStatus::Error;
        }
        if (status == JXL_DEC_NEED_MORE_INPUT) {
            SetError(error, "truncated JPEG XL file");
            return JxlDecodeStatus::Error;
        }
        if (status == JXL_DEC_BASIC_INFO) {
            if (JxlDecoderGetBasicInfo(guard.dec, &info) != JXL_DEC_SUCCESS) {
                SetError(error, "could not read JPEG XL header");
                return JxlDecodeStatus::Error;
            }
            haveInfo = true;
            if (info.xsize == 0 || info.ysize == 0) {
                SetError(error, "JPEG XL image has no pixels");
                return JxlDecodeStatus::Error;
            }
            const uint64_t pixels = static_cast<uint64_t>(info.xsize) * static_cast<uint64_t>(info.ysize);
            if (pixels > kMaxJxlPixels) {
                SetError(error, "JPEG XL image is too large");
                return JxlDecodeStatus::TooLarge;
            }
            if (info.intensity_target > 255.0f) {
                JxlDecoderSetDesiredIntensityTarget(guard.dec, 255.0f);
            }
            continue;
        }
        if (status == JXL_DEC_COLOR_ENCODING) {
            if (!haveInfo && JxlDecoderGetBasicInfo(guard.dec, &info) == JXL_DEC_SUCCESS) {
                haveInfo = true;
            }
            JxlColorEncoding encoding;
            SetSrgbEncoding(encoding, haveInfo && info.num_color_channels == 1);
            if (JxlDecoderSetOutputColorProfile(guard.dec, &encoding, nullptr, 0) != JXL_DEC_SUCCESS) {
                SetError(error, "could not convert JPEG XL to sRGB");
                return JxlDecodeStatus::Error;
            }
            continue;
        }
        if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            if (!haveInfo) {
                SetError(error, "JPEG XL header missing");
                return JxlDecodeStatus::Error;
            }
            channels = info.num_color_channels == 1 ? 1u : 3u;
            if (info.alpha_bits != 0) {
                channels += 1u;
            }
            if (channels < 1 || channels > 4) {
                SetError(error, "unsupported JPEG XL channel count");
                return JxlDecodeStatus::Error;
            }

            JxlPixelFormat format = {};
            format.num_channels = channels;
            format.data_type = JXL_TYPE_UINT8;
            format.endianness = JXL_NATIVE_ENDIAN;
            format.align = 0;

            size_t bufferSize = 0;
            if (JxlDecoderImageOutBufferSize(guard.dec, &format, &bufferSize) != JXL_DEC_SUCCESS) {
                SetError(error, "could not size JPEG XL output");
                return JxlDecodeStatus::Error;
            }
            const uint64_t expect = static_cast<uint64_t>(info.xsize) * static_cast<uint64_t>(info.ysize) * channels;
            if (bufferSize != expect) {
                SetError(error, "unexpected JPEG XL stride");
                return JxlDecodeStatus::Error;
            }

            try {
                packed.resize(bufferSize);
            }
            catch (const std::bad_alloc&) {
                SetError(error, "out of memory");
                return JxlDecodeStatus::Error;
            }
            if (JxlDecoderSetImageOutBuffer(guard.dec, &format, packed.data(), packed.size()) != JXL_DEC_SUCCESS) {
                SetError(error, "could not set JPEG XL output buffer");
                return JxlDecodeStatus::Error;
            }
            continue;
        }
        if (status == JXL_DEC_FULL_IMAGE) {
            wroteFrame = true;
            // Still images finish here. Animations keep the first composited frame.
            break;
        }
        if (status == JXL_DEC_SUCCESS) {
            break;
        }
        SetError(error, "unexpected JPEG XL decoder status");
        return JxlDecodeStatus::Error;
    }

    if (!wroteFrame || !haveInfo) {
        SetError(error, "JPEG XL file contained no image");
        return JxlDecodeStatus::Error;
    }

    try {
        if (!ExpandToRgba(packed, info.xsize, info.ysize, channels, image.rgba)) {
            SetError(error, "could not expand JPEG XL pixels");
            return JxlDecodeStatus::Error;
        }
    }
    catch (const std::bad_alloc&) {
        SetError(error, "out of memory");
        return JxlDecodeStatus::Error;
    }

    image.width = info.xsize;
    image.height = info.ysize;
    return JxlDecodeStatus::Ok;
}
