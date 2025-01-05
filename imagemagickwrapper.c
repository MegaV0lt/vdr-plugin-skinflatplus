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
    const PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
#endif
    cImage *image = new cImage(cSize(width, height));
    tColor *imgData = (tColor *)image->Data();
    if (w != width || h != height) {
        ImageScaler scaler;
        scaler.SetImageParameters(imgData, width, width, height, w, h);
#ifdef IMAGEMAGICK7
        unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
        for (int iy {0}; iy < h; ++iy) {
            for (int ix {0}; ix < w; ++ix) {
                Color c = buffer.pixelColor(ix, iy);
                /*unsigned char*/ r = c.quantumRed() * 255 / QuantumRange;
                /*unsigned char*/ g = c.quantumGreen() * 255 / QuantumRange;
                /*unsigned char*/ b = c.quantumBlue() * 255 / QuantumRange;
                /*unsigned char*/ o = c.quantumAlpha() * 255 / QuantumRange;
                scaler.PutSourcePixel((int(b)), (int(g)), (int(r)), (int(o)));
            }
        }
#else
        for (const void *pixels_end = &pixels[w * h]; pixels < pixels_end; ++pixels)
            scaler.PutSourcePixel(pixels->blue / ((MaxRGB + 1) / 256), pixels->green / ((MaxRGB + 1) / 256),
                                  pixels->red / ((MaxRGB + 1) / 256),
                                  ~((unsigned char)(pixels->opacity / ((MaxRGB + 1) / 256))));
#endif
        return image;
    }
#ifdef IMAGEMAGICK7
    unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
    for (int iy {0}; iy < h; ++iy) {
        for (int ix {0}; ix < w; ++ix) {
            Color c = buffer.pixelColor(ix, iy);
            /*unsigned char*/ r = c.quantumRed() * 255 / QuantumRange;
            /*unsigned char*/ g = c.quantumGreen() * 255 / QuantumRange;
            /*unsigned char*/ b = c.quantumBlue() * 255 / QuantumRange;
            /*unsigned char*/ o = c.quantumAlpha() * 255 / QuantumRange;
            *imgData++ = ((int(o) << 24) | (int(r) << 16) | (int(g) << 8) | (int(b)));
        }
    }
#else
    for (const void *pixels_end = &pixels[width * height]; pixels < pixels_end; ++pixels)
        *imgData++ =
            ((~int(pixels->opacity / ((MaxRGB + 1) / 256)) << 24) | (int(pixels->green / ((MaxRGB + 1) / 256)) << 8) |
             (int(pixels->red / ((MaxRGB + 1) / 256)) << 16) | (int(pixels->blue / ((MaxRGB + 1) / 256))));
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
    const PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
#else
    unsigned char r {}, g {}, b {}, o {};  // Initialise outside of for loop
#endif
    for (int iy {0}; iy < h; ++iy) {
        for (int ix {0}; ix < w; ++ix) {
#ifdef IMAGEMAGICK7
            Color c = buffer.pixelColor(ix, iy);
            /*unsigned char*/ r = c.quantumRed() * 255 / QuantumRange;
            /*unsigned char*/ g = c.quantumGreen() * 255 / QuantumRange;
            /*unsigned char*/ b = c.quantumBlue() * 255 / QuantumRange;
            /*unsigned char*/ o = c.quantumAlpha() * 255 / QuantumRange;
            /* tColor */ col = (int(o) << 24) | (int(r) << 16) | (int(g) << 8) | (int(b));
#else
            /* tColor */ col = (~int(pixels->opacity * 255 / MaxRGB) << 24) | (int(pixels->green * 255 / MaxRGB) << 8) |
                         (int(pixels->red * 255 / MaxRGB) << 16) | (int(pixels->blue * 255 / MaxRGB));
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
    try {
        buffer.read(fullpath);
    } catch (...) {
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
