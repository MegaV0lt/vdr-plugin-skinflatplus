/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./imagemagickwrapper.h"

#include <memory>
#include <cstring>

#include "./imagescaler.h"

#ifdef IMAGEMAGICK
#if MagickLibVersion >= 0x700
#define IMAGEMAGICK7
#endif
#endif

cImageMagickWrapper::cImageMagickWrapper() {}
cImageMagickWrapper::~cImageMagickWrapper() {}

cImage *cImageMagickWrapper::CreateImage(int width, int height, bool PreserveAspect) {
    const int w = buffer.columns();  // Narrowing conversion
    const int h = buffer.rows();
    if (width == 0) width = w;
    if (height == 0) height = h;
    // Validate output dimensions
    if (width <= 0 || height <= 0) {
        esyslog("flatPlus: cImageMagickWrapper::CreateImage() Invalid target dimensions: %dx%d", width, height);
        return nullptr;
    }

    if (PreserveAspect) {
        static constexpr uint16_t SCALE_FACTOR {1000};
        uint32_t scale_w = SCALE_FACTOR * width / w;
        uint32_t scale_h = SCALE_FACTOR * height / h;
        if (scale_w > scale_h)
            width = w * height / h;
        else
            height = h * width / w;
    }

    // Allocate image with RAII
    auto image = std::make_unique<cImage>(cSize(width, height));
    // tColor *imgData = reinterpret_cast<tColor *>(image->Data());
    tColor *imgData = (tColor *)image->Data();
    if (!imgData) {
        esyslog("flatPlus: cImageMagickWrapper::CreateImage() failed to allocate memory for image data");
        return nullptr;
    }

#ifdef IMAGEMAGICK7
    using Quantum = Magick::Quantum;
    static constexpr uint64_t SCALE16 = 1ULL << 16;
    const uint64_t QuantumScaleInt = (255ULL * SCALE16) / QuantumRange;
    const Quantum *px = buffer.getConstPixels(0, 0, w, h);
    if (!px) {
        esyslog("flatPlus: cImageMagickWrapper::CreateImage() failed to get pixel data");
        return nullptr;
    }

    // MagickCore expects: [r,g,b,a] or [cmyk a] etc--RGB(A) order. Confirm layout!
    const int quantum_channels = buffer.channels();

    // When scaling/resizing required
    if (w != width || h != height) {
        ImageScaler scaler;
        scaler.SetImageParameters(imgData, width, width, height, w, h);

        const Quantum *src = px;
        unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
        for (int iy {0}; iy < h; ++iy) {
            for (int ix {0}; ix < w; ++ix) {
                r = static_cast<unsigned char>((src[0] * QuantumScaleInt) >> 16);
                g = static_cast<unsigned char>((src[1] * QuantumScaleInt) >> 16);
                b = static_cast<unsigned char>((src[2] * QuantumScaleInt) >> 16);
                o = quantum_channels > 3
                    ? static_cast<unsigned char>((src[3] * QuantumScaleInt) >> 16)
                    : 0xff;
                scaler.PutSourcePixel(b, g, r, o);
                src += quantum_channels;
            }
        }
        return image.release();
    }

    // "Fast path": sizes match, just convert directly
    {
        const Quantum *src = px;
        unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
        for (int pixel {0}, pix {w * h}; pixel < pix; ++pixel) {
            r = static_cast<unsigned char>((src[0] * QuantumScaleInt) >> 16);
            g = static_cast<unsigned char>((src[1] * QuantumScaleInt) >> 16);
            b = static_cast<unsigned char>((src[2] * QuantumScaleInt) >> 16);
            o = quantum_channels > 3
                ? static_cast<unsigned char>((src[3] * QuantumScaleInt) >> 16)
                : 0xff;
            *imgData++ = (o << 24) | (r << 16) | (g << 8) | b;
            src += quantum_channels;
        }
        return image.release();
    }
#else
    // ImageMagick <=6: use PixelPacket
    const Magick::PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
    if (!pixels) {
        esyslog("flatPlus: cImageMagickWrapper::CreateImage() failed to get pixel data");
        return nullptr;
    }
    const Magick::PixelPacket *src = pixels;
    static constexpr uint64_t SCALE_FACTOR = 1ULL << 16;
    const uint64_t RGBScaleInt = ((MaxRGB + 1UL) * SCALE_FACTOR) / 256UL;

    if (w != width || h != height) {
        ImageScaler scaler;
        scaler.SetImageParameters(imgData, width, width, height, w, h);
        int blue {}, green {}, red {}, alpha {};  // Initialise outside of for loop
        for (int pixel {0}, pix {w * h}; pixel < pix; ++pixel, ++src) {
            blue  = (src->blue   * SCALE_FACTOR) / RGBScaleInt;
            green = (src->green  * SCALE_FACTOR) / RGBScaleInt;
            red   = (src->red    * SCALE_FACTOR) / RGBScaleInt;
            alpha = ~(static_cast<unsigned char>((src->opacity * SCALE_FACTOR) / RGBScaleInt));
            scaler.PutSourcePixel(blue, green, red, alpha);
        }
        return image.release();
    }

    // Fast path: sizes match, just go
    for (int pixel {0}, pix {w * h}; pixel < pix; ++pixel, ++src) {
        *imgData++ =
            ((~static_cast<int>((src->opacity * SCALE_FACTOR) / RGBScaleInt) << 24) |
             (static_cast<int>((src->red      * SCALE_FACTOR) / RGBScaleInt) << 16) |
             (static_cast<int>((src->green    * SCALE_FACTOR) / RGBScaleInt) << 8)  |
             (static_cast<int>((src->blue     * SCALE_FACTOR) / RGBScaleInt)));
    }
    return image.release();
#endif
}

cImage cImageMagickWrapper::CreateImageCopy() {
    const int w = buffer.columns();
    const int h = buffer.rows();
    cImage image(cSize(w, h));
    tColor *imgData = (tColor *)image.Data();
#ifdef IMAGEMAGICK7
    using Quantum = Magick::Quantum;
    static constexpr uint64_t SCALE16 = 1UL << 16;
    const uint64_t QuantumScaleInt = (255UL * SCALE16) / QuantumRange;
    const Quantum *src = buffer.getConstPixels(0, 0, w, h);
    if (!src) return image;
    const int quantum_channels = buffer.channels();
    unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
    for (int pixel {0}, pix {w * h}; pixel < pix; ++pixel, src += quantum_channels) {
        r = static_cast<unsigned char>((src[0] * QuantumScaleInt) >> 16);
        g = static_cast<unsigned char>((src[1] * QuantumScaleInt) >> 16);
        b = static_cast<unsigned char>((src[2] * QuantumScaleInt) >> 16);
        o = quantum_channels > 3
                ? static_cast<unsigned char>((src[3] * QuantumScaleInt) >> 16)
                : 0xff;
        *imgData++ = (o << 24) | (r << 16) | (g << 8) | b;
    }
#else
    const Magick::PixelPacket *src = buffer.getConstPixels(0, 0, w, h);
    if (!src) return image;
    static constexpr uint64_t SCALE_FACTOR = 1UL << 16;
    const uint64_t RGBScaleInt = ((MaxRGB + 1UL) * SCALE_FACTOR) / 256UL;
    for (int pixel {0}, pix {w * h}; pixel < pix; ++pixel, ++src) {
        *imgData++ =
            ((~static_cast<int>((src->opacity * SCALE_FACTOR) / RGBScaleInt) << 24) |
            (static_cast<int>((src->red       * SCALE_FACTOR) / RGBScaleInt) << 16) |
            (static_cast<int>((src->green     * SCALE_FACTOR) / RGBScaleInt) << 8)  |
            (static_cast<int>((src->blue      * SCALE_FACTOR) / RGBScaleInt)));
    }
#endif
    return image;
}

bool cImageMagickWrapper::LoadImage(const char *fullpath) {
    if ((fullpath == nullptr) || (strlen(fullpath) < 5))
        return false;

    // Check if file exists
    struct stat st;
    if (stat(fullpath, &st) != 0)
        return false;

    try {
        buffer.read(fullpath);
    } catch (const Magick::Exception &e) {
        esyslog("flatPlus: cImageMagickWrapper::LoadImage() Magick::Exception: %s\n", e.what());
        return false;  // Signal error
    } catch (const std::exception &e) {
        esyslog("flatPlus: cImageMagickWrapper::LoadImage() std::exception: %s\n", e.what());
        return false;  // Signal error
    } catch (...) {
        esyslog("flatPlus: cImageMagickWrapper::LoadImage() Unknown error occurred.");
        return false;  // Signal error
    }

    return true;
}

Color cImageMagickWrapper::Argb2Color(tColor col) {
    tIndex alpha = (col & 0xFF000000) >> 24;
    tIndex red   = (col & 0x00FF0000) >> 16;
    tIndex green = (col & 0x0000FF00) >> 8;
    tIndex blue  = (col & 0x000000FF);
#ifdef IMAGEMAGICK7
    Color color(QuantumRange * red / 255, QuantumRange * green / 255, QuantumRange * blue / 255,
                QuantumRange * alpha / 255);
#else
    Color color(MaxRGB * red / 255, MaxRGB * green / 255, MaxRGB * blue / 255, MaxRGB * (0xFF - alpha) / 255);
#endif
    return color;
}
