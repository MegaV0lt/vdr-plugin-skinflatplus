#pragma once

#define X_DISPLAY_MISSING

#include <Magick++.h>
#include <vdr/osd.h>

using namespace Magick;

class cImageMagickWrapper {
public:
    cImageMagickWrapper();
    ~cImageMagickWrapper();
protected:
    Image buffer;
    Color Argb2Color(tColor col);
    cImage *CreateImage(int width, int height, bool preserveAspect = true);
    cImage CreateImageCopy(void);
    bool LoadImage(const char *fullpath);
};
