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
    buffer.erase();  // Clear any image data
    buffer = Magick::Image();  // Release associated memory
}

cImage *cImageMagickWrapper::CreateImage(int width, int height, bool PreserveAspect) {
    int w {0}, h {0};
    w = buffer.columns();
    h = buffer.rows();
    if (width == 0)
        width = w;
    if (height == 0)
        height = h;
    if (PreserveAspect) {
        unsigned scale_w = 1000 * width / w;
        unsigned scale_h = 1000 * height / h;
        if (scale_w > scale_h)
            width = w * height / h;
        else
            height = h * width / w;
    }
#ifndef IMAGEMAGICK7
    const Magick::PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
#endif
    cImage *image = new cImage(cSize(width, height));
    tColor *imgData = (tColor *)image->Data();
    if (w != width || h != height) {
        ImageScaler scaler;
        scaler.SetImageParameters(imgData, width, width, height, w, h);
#ifdef IMAGEMAGICK7
        unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
        const double QuantumScale {255.0 / QuantumRange};  // Eliminates repeated division operations in tight loops
        for (int iy {0}; iy < h; ++iy) {
            for (int ix {0}; ix < w; ++ix) {
                Color c = buffer.pixelColor(ix, iy);
                r = static_cast<unsigned char>(c.quantumRed() * QuantumScale);
                g = static_cast<unsigned char>(c.quantumGreen() * QuantumScale);
                b = static_cast<unsigned char>(c.quantumBlue() * QuantumScale);
                o = static_cast<unsigned char>(c.quantumAlpha() * QuantumScale);
                scaler.PutSourcePixel(static_cast<int>(b), static_cast<int>(g), static_cast<int>(r),
                                      static_cast<int>(o));
            }
        }
#else
        const double RGBScale {(MaxRGB + 1) / 256};  // Eliminates repeated division operations in tight loops
        for (const void *pixels_end = &pixels[w * h]; pixels < pixels_end; ++pixels)
            scaler.PutSourcePixel(static_cast<int>(pixels->blue / RGBScale),
                                  static_cast<int>(pixels->green / RGBScale),
                                  static_cast<int>(pixels->red / RGBScale),
                                  ~static_cast<unsigned char>(pixels->opacity / RGBScale));
#endif
        return image;
    }
#ifdef IMAGEMAGICK7
    unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
    const double QuantumScale {255.0 / QuantumRange};  // Eliminates repeated division operations in tight loops
    for (int iy {0}; iy < h; ++iy) {
        for (int ix {0}; ix < w; ++ix) {
            Color c = buffer.pixelColor(ix, iy);
            r = static_cast<unsigned char>(c.quantumRed() * QuantumScale);
            g = static_cast<unsigned char>(c.quantumGreen() * QuantumScale);
            b = static_cast<unsigned char>(c.quantumBlue() * QuantumScale);
            o = static_cast<unsigned char>(c.quantumAlpha() * QuantumScale);
            *imgData++ = (static_cast<int>(o) << 24) | (static_cast<int>(r) << 16) | (static_cast<int>(g) << 8) |
                         static_cast<int>(b);
        }
    }
#else
    const double RGBScale {(MaxRGB + 1) / 256};  // Eliminates repeated division operations in tight loops
    for (const void *pixels_end = &pixels[width * height]; pixels < pixels_end; ++pixels)
        *imgData++ =
            ((~static_cast<int>(pixels->opacity / RGBScale) << 24) | (static_cast<int>(pixels->green / RGBScale) << 8) |
             (static_cast<int>(pixels->red / RGBScale) << 16) | static_cast<int>(pixels->blue / RGBScale));
#endif
    return image;
}

cImage cImageMagickWrapper::CreateImageCopy() {
    int w {0}, h {0};
    w = buffer.columns();
    h = buffer.rows();
    cImage image(cSize(w, h));
    tColor col {0};
#ifndef IMAGEMAGICK7
    const Magick::PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
#else
    unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
    const double QuantumScale {255.0 / QuantumRange};  // Eliminates repeated division operations in tight loops
#endif
    for (int iy {0}; iy < h; ++iy) {
        for (int ix {0}; ix < w; ++ix) {
#ifdef IMAGEMAGICK7
            Color c = buffer.pixelColor(ix, iy);
            r = c.quantumRed() * QuantumScale;
            g = c.quantumGreen() * QuantumScale;
            b = c.quantumBlue() * QuantumScale;
            o = c.quantumAlpha() * QuantumScale;
            col = (static_cast<int>(o) << 24) | (static_cast<int>(r) << 16) | (static_cast<int>(g) << 8) |
                               (static_cast<int>(b));
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
    } catch (const Magick::Exception& e) {  // Log specific error
        esyslog("flatPlus: cImageMagickWrapper::LoadImage() Error: %s\n", e.what());
        return false;
    } catch (const std::exception& e) {  // Log general error
        esyslog("flatPlus: cImageMagickWrapper::LoadImage() Error: %s\n", e.what());
        return false;
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
