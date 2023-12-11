#include "./imageloader.h"
#include <dirent.h>
#include <math.h>

#include <iostream>
#include <string>

#include "./flat.h"

using namespace Magick;

cImageLoader::cImageLoader() {
    LogoExtension = "png";
}

cImageLoader::~cImageLoader() {
}

cImage* cImageLoader::LoadLogo(const char *logo, int width, int height) {
    if ((width == 0) || (height == 0)) return NULL;

    // Plain logo without converting to lower including '/'
    cString File = cString::sprintf("%s%s.%s", *Config.LogoPath, logo, *LogoExtension);
    cImage *img = NULL;
    for (int i {0}; i < 2; ++i) {
        if (i == 1) {  // Second try (Plain logo not found)
            std::string LogoLower = logo;
            ToLowerCase(LogoLower);  // Convert to lowercase (A-Z)
            File = cString::sprintf("%s%s.%s", *Config.LogoPath, LogoLower.c_str(), *LogoExtension);
        }
        #ifdef DEBUGIMAGELOADTIME
            dsyslog("flatPlus: ImageLoader LoadLogo %s", *File);
            uint32_t tick1 = GetMsTicks();
        #endif

        img = ImgCache.GetImage(*File, width, height);  // Check if image is in imagecache

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick2 = GetMsTicks();
            dsyslog("   search in cache: %d ms", tick2 - tick1);
        #endif

        if (img) return img;  // Image found in imagecache

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick3 = GetMsTicks();
        #endif

        bool success = LoadImage(*File);  // Try to load image from disk

        if (!success) {
            if (i == 1)  // Second try
                dsyslog("flatPlus: ImageLoader LoadLogo: %s (%s.%s) could not be loaded", *File, logo,
                        *LogoExtension);
            continue;  // Image not found on disk
        }

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick4 = GetMsTicks();
            dsyslog("   load file from disk: %d ms", tick4 - tick3);
            uint32_t tick5 = GetMsTicks();
        #endif

        img = CreateImage(width, height);

        if (!img) continue;

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick6 = GetMsTicks();
            dsyslog("   scale logo: %d ms", tick6 - tick5);
        #endif

        ImgCache.InsertImage(img, *File, width, height);  // Add image to imagecache
        return img;  // Image loaded from disk
    }  // for
    return NULL;  // No image; so return Null
}

cImage* cImageLoader::LoadIcon(const char *cIcon, int width, int height) {
    if ((width == 0) || (height == 0)) return NULL;

    cString File = cString::sprintf("%s%s/%s.%s", *Config.IconPath, Setup.OSDTheme, cIcon, *LogoExtension);

    #ifdef DEBUGIMAGELOADTIME
        dsyslog("flatPlus: ImageLoader LoadIcon %s", *File);
    #endif

    cImage *img = NULL;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick1 = GetMsTicks();
    #endif

    img = ImgCache.GetImage(*File, width, height);

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick2 = GetMsTicks();
        dsyslog("   search in cache: %d ms", tick2 - tick1);
    #endif

    if (img) return img;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick3 = GetMsTicks();
    #endif

    bool success = LoadImage(*File);

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick4 = GetMsTicks();
        dsyslog("   load file from disk: %d ms", tick4 - tick3);
    #endif

    if (!success) {  // Search for logo in default folder
        File = cString::sprintf("%s%s/%s.%s", *Config.IconPath, "default", cIcon, *LogoExtension);
        #ifdef DEBUGIMAGELOADTIME
            dsyslog("flatPlus: ImageLoader LoadIcon %s", *File);
            uint32_t tick5 = GetMsTicks();
        #endif

        img = ImgCache.GetImage(*File, width, height);

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick6 = GetMsTicks();
            dsyslog("   search in cache: %d ms", tick6 - tick5);
        #endif

        if (img) return img;

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick7 = GetMsTicks();
        #endif

        success = LoadImage(*File);

        #ifdef DEBUGIMAGELOADTIME
            uint32_t tick8 = GetMsTicks();
            dsyslog("   load file from disk: %d ms", tick8 - tick7);
        #endif

        if (!success) {
            dsyslog("flatPlus: ImageLoader LoadIcon: %s could not be loaded", *File);
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
        if (!img) {
            dsyslog("flatPlus: ImageLoader LoadIcon: %s 'CreateImage' failed", *File);
            return NULL;
        }

        ImgCache.InsertImage(img, *File, width, height);
        return img;
}

cImage* cImageLoader::LoadFile(const char *cFile, int width, int height) {
    if ((width == 0) || (height == 0)) return NULL;

    cString File = cFile;
    #ifdef DEBUGIMAGELOADTIME
        dsyslog("flatPlus: ImageLoader LoadFile %s", *File);
    #endif

    cImage *img = NULL;
    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick1 = GetMsTicks();
    #endif

    img = ImgCache.GetImage(*File, width, height);

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick2 = GetMsTicks();
        dsyslog("   search in cache: %d ms", tick2 - tick1);
    #endif

    if (img) return img;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick3 = GetMsTicks();
    #endif

    bool success = LoadImage(*File);

    if (!success) {
        dsyslog("flatPlus: ImageLoader LoadFile: %s could not be loaded", *File);
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

    if (!img) return NULL;

    #ifdef DEBUGIMAGELOADTIME
        uint32_t tick6 = GetMsTicks();
        dsyslog("   scale logo: %d ms", tick6 - tick5);
    #endif

    ImgCache.InsertImage(img, *File, width, height);
    return img;
}

void cImageLoader::ToLowerCase(std::string &str) {
    for (auto &ch : str) {
        if (ch >= 'A' && ch <= 'Z')
            ch += 32;  // Or: ch ^= 1 << 5;
    }
}

bool cImageLoader::FileExits(const std::string &name) {
    struct stat Buffer;
    return (stat (name.c_str(), &Buffer) == 0);
}

bool cImageLoader::SearchRecordingPoster(cString RecPath, cString &found) {
    cString ManualPoster = cString::sprintf("%s/cover_vdr.jpg", *RecPath);
    if (FileSize(*ManualPoster) != -1) {
        dsyslog("flatPlus: Poster found in %s/cover_vdr.jpg", *RecPath);
        found = ManualPoster;
        return true;
    }
    ManualPoster = cString::sprintf("%s/../../../cover_vdr.jpg", *RecPath);
    if (FileSize(*ManualPoster) != -1) {
        dsyslog("flatPlus: Poster found in %s/../../../cover_vdr.jpg", *RecPath);
        found = ManualPoster;
        return true;
    }
    ManualPoster = cString::sprintf("%s/../../cover_vdr.jpg", *RecPath);
    if (FileSize(*ManualPoster) != -1) {
        dsyslog("flatPlus: Poster found in %s/../../cover_vdr.jpg", *RecPath);
        found = ManualPoster;
        return true;
    }
    return false;
}
