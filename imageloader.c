#include "imageloader.h"
#include <math.h>
#include <string>
#include <dirent.h>
#include <iostream>
#include "flat.h"

using namespace Magick;

cImageLoader::cImageLoader() {
    logoExtension = "png";
}

cImageLoader::~cImageLoader() {
}

cImage* cImageLoader::LoadLogo(const char *logo, int width, int height) {
    if ((width == 0) || (height == 0))
        return NULL;

    std::string logoLower = logo;
    toLowerCase(logoLower);
    cString File = cString::sprintf("%s/%s.%s", *Config.logoPath, logoLower.c_str(), *logoExtension);
    #ifdef DEBUGIMAGELOADTIME
        dsyslog("imageloader load logo %s", *File);
    #endif

    cImage *img;
    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick1 = GetMsTicks();
    #endif

    img = imgCache.GetImage(*File, width, height);

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick2 = GetMsTicks();
        dsyslog("   search in cache: %d ms", tick2 - tick1);
    #endif
    if (img)
        return img;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick3 = GetMsTicks();
    #endif

    bool success = LoadImage(File);

    if (!success) {
        dsyslog("imageloader LoadLogo: %s could not be loaded", *File);
        return NULL;
    }
    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick4 = GetMsTicks();
        dsyslog("   load file from disk: %d ms", tick4 - tick3);
    #endif

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick5 = GetMsTicks();
    #endif

    img = CreateImage(width, height);

    if (!img)
        return NULL;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick6 = GetMsTicks();
        dsyslog("   scale logo: %d ms", tick6 - tick5);
    #endif

    imgCache.InsertImage(img, *File, width, height);
    return img;
}

cImage* cImageLoader::LoadIcon(const char *cIcon, int width, int height) {
    if ((width == 0) || (height == 0))
        return NULL;

    cString File = cString::sprintf("%s%s/%s.%s", *Config.iconPath, Setup.OSDTheme, cIcon, *logoExtension);

    #ifdef DEBUGIMAGELOADTIME
        dsyslog("imageloader load icon %s", *File);
    #endif

    cImage *img;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick1 = GetMsTicks();
    #endif

    img = imgCache.GetImage(*File, width, height);

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick2 = GetMsTicks();
        dsyslog("   search in cache: %d ms", tick2 - tick1);
    #endif
    if (img)
        return img;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick3 = GetMsTicks();
    #endif

    bool success = LoadImage(File);

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick4 = GetMsTicks();
        dsyslog("   load file from disk: %d ms", tick4 - tick3);
    #endif

    if (!success) {
        File = cString::sprintf("%s%s/%s.%s", *Config.iconPath, "default", cIcon, *logoExtension);
        #ifdef DEBUGIMAGELOADTIME
            dsyslog("imageloader load icon %s", *File);
            uint32_t tick5 = GetMsTicks();
        #endif

        img = imgCache.GetImage(*File, width, height);

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick6 = GetMsTicks();
            dsyslog("   search in cache: %d ms", tick6 - tick5);
        #endif
        if (img)
            return img;

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick7 = GetMsTicks();
        #endif

        success = LoadImage(File);

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick8 = GetMsTicks();
            dsyslog("   load file from disk: %d ms", tick8 - tick7);
        #endif

        if (!success) {
            dsyslog("imageloader LoadIcon: %s could not be loaded", *File);
            return NULL;
        }
    }
    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick9 = GetMsTicks();
    #endif

    img = CreateImage(width, height);

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick10 = GetMsTicks();
        dsyslog("   scale logo: %d ms", tick10 - tick9);
    #endif
        if (!img)
            return NULL;

        imgCache.InsertImage(img, *File, width, height);
        return img;
}

cImage* cImageLoader::LoadFile(const char *cFile, int width, int height) {
    if ((width == 0) || (height == 0))
        return NULL;

    cString File = cFile;
    #ifdef DEBUGIMAGELOADTIME
        dsyslog("imageloader load file %s", *File);
    #endif

    cImage *img;
    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick1 = GetMsTicks();
    #endif

    img = imgCache.GetImage(*File, width, height);

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick2 = GetMsTicks();
        dsyslog("   search in cache: %d ms", tick2 - tick1);
    #endif
    if (img)
        return img;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick3 = GetMsTicks();
    #endif

    bool success = LoadImage(File);

    if (!success) {
        dsyslog("imageloader LoadFile: %s could not be loaded", *File);
        return NULL;
    }
    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick4 = GetMsTicks();
        dsyslog("   load file from disk: %d ms", tick4 - tick3);
    #endif

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick5 = GetMsTicks();
    #endif

    img = CreateImage(width, height);

    if (!img)
        return NULL;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick6 = GetMsTicks();
        dsyslog("   scale logo: %d ms", tick6 - tick5);
    #endif

    imgCache.InsertImage(img, *File, width, height);
    return img;
}

void cImageLoader::toLowerCase(std::string &str) {
    const int length = str.length();
    for (int i {0}; i < length; ++i) {
        str[i] = std::tolower(str[i]);
    }
}

bool cImageLoader::FileExits(const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

bool cImageLoader::SearchRecordingPoster(cString recPath, cString &found) {
    cString manualPoster = cString::sprintf("%s/cover_vdr.jpg", *recPath);
    if (FileSize(*manualPoster) != -1) {
        dsyslog("Poster found in %s/cover_vdr.jpg", *recPath);
        found = manualPoster;
        return true;
    }
    manualPoster = cString::sprintf("%s/../../../cover_vdr.jpg", *recPath);
    if (FileSize(*manualPoster) != -1) {
        dsyslog("Poster found in %s/../../../cover_vdr.jpg", *recPath);
        found = manualPoster;
        return true;
    }
    manualPoster = cString::sprintf("%s/../../cover_vdr.jpg", *recPath);
    if (FileSize(*manualPoster) != -1) {
        dsyslog("Poster found in %s/../../cover_vdr.jpg", *recPath);
        found = manualPoster;
        return true;
    }
    return false;
}
