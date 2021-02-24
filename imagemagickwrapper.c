#include "imagemagickwrapper.h"
#include "imagescaler.h"
#include <sstream>
#include <string>

#ifdef IMAGEMAGICK
#if MagickLibVersion >= 0x700
#define IMAGEMAGICK7
#info ImageMagick Version 7.0 + is used
#endif
#endif

cImageMagickWrapper::cImageMagickWrapper() {}

cImageMagickWrapper::~cImageMagickWrapper() {}

cImage *cImageMagickWrapper::CreateImage(int width, int height, bool preserveAspect) {
  int w, h;
#ifdef IMAGEMAGICK7
  unsigned offset;
#endif
  w = buffer.columns();
  h = buffer.rows();
  if (width == 0)
    width = w;
  if (height == 0)
    height = h;
  if (preserveAspect) {
    unsigned scale_w = 1000 * width / w;
    unsigned scale_h = 1000 * height / h;
    if (scale_w > scale_h)
      width = w * height / h;
    else
      height = h * width / w;
  }
#ifdef IMAGEMAGICK7
  const Quantum *pixels = buffer.getConstPixels(0, 0, w, h);
#else
  const PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
#endif
  cImage *image = new cImage(cSize(width, height));
  tColor *imgData = (tColor *)image->Data();
  if (w != width || h != height) {
    ImageScaler scaler;
    scaler.SetImageParameters(imgData, width, width, height, w, h);
#ifdef IMAGEMAGICK7
    for (int iy = 0; iy < h; ++iy) {
      for (int ix = 0; ix < w; ++ix) {
        offset = buffer.channels() * (w * iy + ix);
        scaler.PutSourcePixel(
            pixels[offset + 2] / ((QuantumRange + 1) / 256), pixels[offset + 1] / ((QuantumRange + 1) / 256),
            pixels[offset + 0] / ((QuantumRange + 1) / 256), pixels[offset + 3] / ((QuantumRange + 1) / 256));
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
  for (int iy = 0; iy < h; ++iy) {
    for (int ix = 0; ix < w; ++ix) {
      offset = buffer.channels() * (w * iy + ix);
      *imgData++ = ((int(pixels[offset + 3] / ((QuantumRange + 1) / 256)) << 24) |
                    (int(pixels[offset + 1] / ((QuantumRange + 1) / 256)) << 8) |
                    (int(pixels[offset + 0] / ((QuantumRange + 1) / 256)) << 16) |
                    (int(pixels[offset + 2] / ((QuantumRange + 1) / 256))));
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
  int w, h;
#ifdef IMAGEMAGICK7
  unsigned offset;
#endif
  w = buffer.columns();
  h = buffer.rows();
  cImage image(cSize(w, h));
#ifdef IMAGEMAGICK7
  const Quantum *pixels = buffer.getConstPixels(0, 0, w, h);
#else
  const PixelPacket *pixels = buffer.getConstPixels(0, 0, w, h);
#endif
  for (int iy = 0; iy < h; ++iy) {
    for (int ix = 0; ix < w; ++ix) {
#ifdef IMAGEMAGICK7
      offset = buffer.channels() * (w * iy + ix);
      tColor col =
          (int(pixels[offset + 3] * 255 / QuantumRange) << 24) | (int(pixels[offset + 1] * 255 / QuantumRange) << 8) |
          (int(pixels[offset + 0] * 255 / QuantumRange) << 16) | (int(pixels[offset + 2] * 255 / QuantumRange));
#else
      tColor col = (~int(pixels->opacity * 255 / MaxRGB) << 24) | (int(pixels->green * 255 / MaxRGB) << 8) |
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
  if ((fullpath == NULL) || (strlen(fullpath) < 5))
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
