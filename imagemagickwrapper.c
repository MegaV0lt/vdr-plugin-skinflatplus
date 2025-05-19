/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./imagemagickwrapper.h"

// #include <sstream>
// #include <string>

#include "./imagescaler.h"

#ifdef IMAGEMAGICK
#if MagickLibVersion >= 0x700
#define IMAGEMAGICK7
#endif
#endif

cImageMagickWrapper::cImageMagickWrapper() {
}

cImageMagickWrapper::~cImageMagickWrapper() {
}

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
        constexpr unsigned SCALE_FACTOR {1000};
        unsigned scale_w = SCALE_FACTOR * width / w;
        unsigned scale_h = SCALE_FACTOR * height / h;
        if (scale_w > scale_h)
            width = w * height / h;
        else
            height = h * width / w;
    }
#ifdef IMAGEMAGICK7
    // Use integer arithmetic instead of floating-point division
    const uint64_t QuantumScaleInt {(255ULL * SCALE_FACTOR) / QuantumRange};
#else
    const Magick::PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
    // Use integer arithmetic instead of floating-point division
    constexpr uint64_t SCALE_FACTOR {1ULL << 16};  // Use a power of 2 for efficient scaling
    const uint64_t RGBScaleInt {((MaxRGB + 1UL) * SCALE_FACTOR) / 256UL};
#endif
    cImage *image = new cImage(cSize(width, height));
    if (!image) {
        esyslog("flatPlus: cImageMagickWrapper::CreateImage() failed to allocate memory for image");
        return nullptr;
    }

    tColor *imgData = (tColor *)image->Data();
    if (!imgData) {
        esyslog("flatPlus: cImageMagickWrapper::CreateImage() failed to allocate memory for image data");
        delete image;  // Clean up allocated memory
        return nullptr;
    }

    if (w != width || h != height) {
        ImageScaler scaler;
        scaler.SetImageParameters(imgData, width, width, height, w, h);
#ifdef IMAGEMAGICK7
        unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
        for (int iy {0}; iy < h; ++iy) {
            for (int ix {0}; ix < w; ++ix) {
                Color c = buffer.pixelColor(ix, iy);
                r = static_cast<unsigned char>((c.quantumRed() * QuantumScaleInt) >> 16);
                g = static_cast<unsigned char>((c.quantumGreen() * QuantumScaleInt) >> 16);
                b = static_cast<unsigned char>((c.quantumBlue() * QuantumScaleInt) >> 16);
                o = static_cast<unsigned char>((c.quantumAlpha() * QuantumScaleInt) >> 16);
                scaler.PutSourcePixel(static_cast<int>(b), static_cast<int>(g), static_cast<int>(r),
                                      static_cast<int>(o));
            }
        }
#else
        for (const void *pixels_end = &pixels[w * h]; pixels < pixels_end; ++pixels)
            scaler.PutSourcePixel(static_cast<int>((pixels->blue * SCALE_FACTOR) / RGBScaleInt),
                                  static_cast<int>((pixels->green * SCALE_FACTOR) / RGBScaleInt),
                                  static_cast<int>((pixels->red * SCALE_FACTOR) / RGBScaleInt),
                                  ~static_cast<unsigned char>((pixels->opacity * SCALE_FACTOR) / RGBScaleInt));
#endif
        return image;
    }
#ifdef IMAGEMAGICK7
    unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
    for (int iy {0}; iy < h; ++iy) {
        for (int ix {0}; ix < w; ++ix) {
            Color c = buffer.pixelColor(ix, iy);
            r = static_cast<unsigned char>((c.quantumRed() * QuantumScaleInt) >> 16);
            g = static_cast<unsigned char>((c.quantumGreen() * QuantumScaleInt) >> 16);
            b = static_cast<unsigned char>((c.quantumBlue() * QuantumScaleInt) >> 16);
            o = static_cast<unsigned char>((c.quantumAlpha() * QuantumScaleInt) >> 16);
            *imgData++ = (static_cast<int>(o) << 24) | (static_cast<int>(r) << 16) | (static_cast<int>(g) << 8) |
                         static_cast<int>(b);
        }
    }
#else
    for (const void *pixels_end = &pixels[width * height]; pixels < pixels_end; ++pixels)
        *imgData++ =
            ((~static_cast<int>((pixels->opacity * SCALE_FACTOR) / RGBScaleInt) << 24) |
             (static_cast<int>((pixels->green * SCALE_FACTOR) / RGBScaleInt) << 8) |
             (static_cast<int>((pixels->red * SCALE_FACTOR) / RGBScaleInt) << 16) |
             static_cast<int>((pixels->blue * SCALE_FACTOR) / RGBScaleInt));
#endif
    return image;
}

cImage cImageMagickWrapper::CreateImageCopy() {
    const int w = buffer.columns();  // Narrowing conversion
    const int h = buffer.rows();
    cImage image(cSize(w, h));
    tColor col {0};
#ifndef IMAGEMAGICK7
    const Magick::PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
#else
    unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
    constexpr u_int64_t SCALE_FACTOR = 1UL << 16;
    const u_int64_t QuantumScaleInt = (255UL * SCALE_FACTOR) / QuantumRange;
#endif
    for (int iy {0}; iy < h; ++iy) {
        for (int ix {0}; ix < w; ++ix) {
#ifdef IMAGEMAGICK7
            Color c = buffer.pixelColor(ix, iy);
            r = static_cast<unsigned char>((c.quantumRed() * QuantumScaleInt) >> 16);
            g = static_cast<unsigned char>((c.quantumGreen() * QuantumScaleInt) >> 16);
            b = static_cast<unsigned char>((c.quantumBlue() * QuantumScaleInt) >> 16);
            o = static_cast<unsigned char>((c.quantumAlpha() * QuantumScaleInt) >> 16);
            col = (static_cast<int>(o) << 24) | (static_cast<int>(r) << 16) | (static_cast<int>(g) << 8) |
                         static_cast<int>(b);

#else
            col = (~static_cast<int>(pixels->opacity * 255 / MaxRGB) << 24) |
                  (static_cast<int>(pixels->green * 255 / MaxRGB) << 8) |
                  (static_cast<int>(pixels->red * 255 / MaxRGB) << 16) |
                  (static_cast<int>(pixels->blue * 255 / MaxRGB));
#endif
            image.SetPixel(cPoint(ix, iy), col);
#ifndef IMAGEMAGICK7
            ++pixels;
#endif
        }
    }
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
    tIndex red = (col & 0x00FF0000) >> 16;
    tIndex green = (col & 0x0000FF00) >> 8;
    tIndex blue = (col & 0x000000FF);
#ifdef IMAGEMAGICK7
    Color color(QuantumRange * red / 255, QuantumRange * green / 255, QuantumRange * blue / 255,
                QuantumRange * alpha / 255);
#else
    Color color(MaxRGB * red / 255, MaxRGB * green / 255, MaxRGB * blue / 255, MaxRGB * (0xFF - alpha) / 255);
#endif
    return color;
}
