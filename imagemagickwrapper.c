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
        static constexpr uint16_t kScaleFactor {1000};
        const uint32_t scale_w = kScaleFactor * width / w;
        const uint32_t scale_h = kScaleFactor * height / h;
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
    static constexpr uint64_t kScale16 {1ULL << 16};
    static constexpr uint64_t kQuantumScaleInt {(255ULL * kScale16) / QuantumRange};

    // When scaling/resizing required
    if (w != width || h != height) {
        ImageScaler scaler;
        scaler.SetImageParameters(imgData, width, width, height, w, h);

        for (int iy {0}; iy < h; ++iy) {
            for (int ix {0}; ix < w; ++ix) {
                Magick::Color px = buffer.pixelColor(ix, iy);
                unsigned char r = static_cast<unsigned char>((px.quantumRed()   * kQuantumScaleInt) >> 16);
                unsigned char g = static_cast<unsigned char>((px.quantumGreen() * kQuantumScaleInt) >> 16);
                unsigned char b = static_cast<unsigned char>((px.quantumBlue()  * kQuantumScaleInt) >> 16);
                unsigned char o = static_cast<unsigned char>((px.quantumAlpha() * kQuantumScaleInt) >> 16);
                scaler.PutSourcePixel(b, g, r, o);
            }
        }
        return image.release();
    }

    // "Fast path": Sizes match, just convert directly
    for (int iy {0}, pixel {0}, pix {w * h}; iy < h; ++iy) {
        for (int ix {0}; ix < w; ++ix, ++pixel) {
            Magick::Color px = buffer.pixelColor(ix, iy);
            unsigned char r = static_cast<unsigned char>((px.quantumRed()   * kQuantumScaleInt) >> 16);
            unsigned char g = static_cast<unsigned char>((px.quantumGreen() * kQuantumScaleInt) >> 16);
            unsigned char b = static_cast<unsigned char>((px.quantumBlue()  * kQuantumScaleInt) >> 16);
            unsigned char o = static_cast<unsigned char>((px.quantumAlpha() * kQuantumScaleInt) >> 16);
            *imgData++ = (o << 24) | (r << 16) | (g << 8) | b;
        }
    }
    return image.release();
#else
    static constexpr uint64_t kMaxRGB {65535};  // Magick <=6 uses 16-bit depth (MaxRGB = 65535)
    // ImageMagick <=6: Use PixelPacket
    const Magick::PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
    if (w != width || h != height) {
        ImageScaler scaler;
        scaler.SetImageParameters(imgData, width, width, height, w, h);
        for (const void *pixels_end = &pixels[w * h]; pixels < pixels_end; ++pixels)
            scaler.PutSourcePixel(pixels->blue / ((kMaxRGB + 1) / 256),
                                  pixels->green / ((kMaxRGB + 1) / 256),
                                  pixels->red / ((kMaxRGB + 1) / 256),
                                  ~(static_cast<unsigned char>(pixels->opacity / ((kMaxRGB + 1) / 256))));

        return image.release();
    }

    // Fast path: Sizes match, just go
    for (const void *pixels_end = &pixels[width * height]; pixels < pixels_end; ++pixels)
        *imgData++ = ((~static_cast<int>(pixels->opacity / ((kMaxRGB + 1) / 256)) << 24) |
                      (static_cast<int>(pixels->green / ((kMaxRGB + 1) / 256)) << 8) |
                      (static_cast<int>(pixels->red / ((kMaxRGB + 1) / 256)) << 16) |
                      (static_cast<int>(pixels->blue / ((kMaxRGB + 1) / 256)) ));

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
    static constexpr uint64_t kScale16 = 1UL << 16;
    static constexpr uint64_t kQuantumScaleInt = (255UL * kScale16) / QuantumRange;
    for (int iy {0}, pixel {0}, pix {w * h}; iy < h; ++iy) {
        for (int ix {0}; ix < w; ++ix, ++pixel) {
            Magick::Color px = buffer.pixelColor(ix, iy);
            unsigned char r = static_cast<unsigned char>((px.quantumRed()   * kQuantumScaleInt) >> 16);
            unsigned char g = static_cast<unsigned char>((px.quantumGreen() * kQuantumScaleInt) >> 16);
            unsigned char b = static_cast<unsigned char>((px.quantumBlue()  * kQuantumScaleInt) >> 16);
            unsigned char o = static_cast<unsigned char>((px.quantumAlpha() * kQuantumScaleInt) >> 16);
            *imgData++ = (o << 24) | (r << 16) | (g << 8) | b;
        }
    }
#else
    static constexpr uint64_t kMaxRGB {65535};  // Magick <=6 uses 16-bit depth (MaxRGB = 65535)
    const Magick::PixelPacket *src = buffer.getConstPixels(0, 0, w, h);
    if (!src) return image;
    static constexpr uint64_t kScaleFactor {1UL << 16};
    static constexpr uint64_t kRGBScaleInt {((kMaxRGB + 1UL) * kScaleFactor) / 256UL};
    for (int pixel {0}, pix {w * h}; pixel < pix; ++pixel, ++src) {
        *imgData++ =
            (((~static_cast<int>((src->opacity * kScaleFactor) / kRGBScaleInt)) & 0xFF) << 24) |
            (static_cast<int>((src->red       * kScaleFactor) / kRGBScaleInt) << 16) |
            (static_cast<int>((src->green     * kScaleFactor) / kRGBScaleInt) << 8)  |
            (static_cast<int>((src->blue      * kScaleFactor) / kRGBScaleInt));
    }
#endif
    return image;
}

bool cImageMagickWrapper::LoadImage(const char *fullpath) {
    if ((fullpath == nullptr) || (strlen(fullpath) < 5))
        return false;

    // Check if file exists
    if (LastModifiedTime(fullpath) == 0)
        return false;

    try {
        buffer.read(fullpath);
    } catch (const Magick::Warning &w) {
        esyslog("flatPlus: cImageMagickWrapper::LoadImage() Magick::Warning: %s", w.what());
    } catch (const Magick::Exception &e) {
        esyslog("flatPlus: cImageMagickWrapper::LoadImage() Magick::Exception: %s", e.what());
        return false;  // Signal error
    } catch (const std::exception &e) {
        esyslog("flatPlus: cImageMagickWrapper::LoadImage() std::exception: %s", e.what());
        return false;  // Signal error
    } catch (...) {
        esyslog("flatPlus: cImageMagickWrapper::LoadImage() Unknown error occurred.");
        return false;  // Signal error
    }

    return true;
}

Color cImageMagickWrapper::Argb2Color(tColor col) {
    const tIndex alpha = (col & 0xFF000000) >> 24;
    const tIndex red   = (col & 0x00FF0000) >> 16;
    const tIndex green = (col & 0x0000FF00) >> 8;
    const tIndex blue  = (col & 0x000000FF);
#ifdef IMAGEMAGICK7
    const Color color(QuantumRange * red / 255, QuantumRange * green / 255, QuantumRange * blue / 255,
                      QuantumRange * alpha / 255);
#else
    static constexpr uint64_t kMaxRGB {65535};  // Magick <=6 uses 16-bit depth (MaxRGB = 65535)
    const Color color(kMaxRGB * red / 255, kMaxRGB * green / 255, kMaxRGB * blue / 255,
                      kMaxRGB * (0xFF - alpha) / 255);
#endif
    return color;
}
