/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./imageloader.h"
#include <dirent.h>
#include <math.h>

#include <iostream>
#include <string>

#include "./flat.h"

using namespace Magick;

cImageLoader::cImageLoader() {}

cImageLoader::~cImageLoader() {}

cImage* cImageLoader::LoadLogo(const char *logo, int width, int height) {
    if (width == 0 || height == 0) return nullptr;

    // Plain logo without converting to lower including '/'
    cString File = cString::sprintf("%s/%s.%s", *Config.LogoPath, logo, *m_LogoExtension);
    std::string LogoLower {""};
    LogoLower.reserve(128);
    bool success {false};
    cImage *img {nullptr};
    for (uint i {0}; i < 3; ++i) {      // Run up to three times (0..2)
        if (i == 1) {                   // Second try (Plain logo not found)
            LogoLower = logo;
            ToLowerCase(LogoLower);  // Convert to lowercase (A-Z)
            File = cString::sprintf("%s/%s.%s", *Config.LogoPath, LogoLower.c_str(), *m_LogoExtension);
        } else if (i == 2) {            // Third try. Search for lowercase logo with '~' for path '/'
            const std::size_t LogoLowerLength {LogoLower.length()};
            for (std::size_t i {0}; i < LogoLowerLength; ++i) {
                if (LogoLower[i] == '/')
                    LogoLower[i] = '~';
            }
            File = cString::sprintf("%s/%s.%s", *Config.LogoPath, LogoLower.c_str(), *m_LogoExtension);
        }
#ifdef DEBUGIMAGELOADTIME
        dsyslog("flatPlus: cImageLoader::LoadLogo() '%s'", *File);
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

        success = LoadImage(*File);  // Try to load image from disk
        if (!success) {              // Image not found on disk
            if (i == 2)              // Third try and not found
                dsyslog("flatPlus: cImageLoader::LoadLogo() %s/%s.%s could not be loaded", *Config.LogoPath, logo,
                        *m_LogoExtension);
            continue;
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
    return nullptr;  // No image; so return nullptr
}

cImage* cImageLoader::LoadIcon(const char *cIcon, int width, int height) {
    if (width == 0 || height == 0) return nullptr;

    cString File = cString::sprintf("%s/%s/%s.%s", *Config.IconPath, Setup.OSDTheme, cIcon, *m_LogoExtension);

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: cImageLoader::LoadIcon() '%s'", *File);
#endif

    cImage *img {nullptr};

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
        File = cString::sprintf("%s/%s/%s.%s", *Config.IconPath, "default", cIcon, *m_LogoExtension);
#ifdef DEBUGIMAGELOADTIME
        dsyslog("flatPlus: cImageLoader::LoadIcon() '%s'", *File);
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
            dsyslog("flatPlus: cImageLoader::LoadIcon() '%s' could not be loaded", *File);
            return nullptr;
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
            dsyslog("flatPlus: cImageLoader::LoadIcon() '%s' 'CreateImage' failed", *File);
            return nullptr;
        }

        ImgCache.InsertImage(img, *File, width, height);
        return img;
}

cImage* cImageLoader::LoadFile(const char *cFile, int width, int height) {
    if (width == 0 || height == 0) return nullptr;

    const cString File = cFile;
#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: cImageLoader::LoadFile() '%s'", *File);
#endif

    cImage *img {nullptr};
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

    const bool success = LoadImage(*File);
    if (!success) {
        dsyslog("flatPlus: cImageLoader::LoadFile() '%s' could not be loaded", *File);
        return nullptr;
    }
#ifdef DEBUGIMAGELOADTIME
    uint32_t tick4 = GetMsTicks();
    dsyslog("   load file from disk: %d ms", tick4 - tick3);
#endif

#ifdef DEBUGIMAGELOADTIME
    uint32_t tick5 = GetMsTicks();
#endif

    img = CreateImage(width, height);
    if (!img) return nullptr;

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

// TODO: Improve? What images are to expect?
bool cImageLoader::SearchRecordingPoster(const cString RecPath, cString &found) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cImageLoader::SearchRecordingPoster()");
#endif

    //* Search for cover_vdr.jpg, poster.jpg, banner.jpg and fanart.jpg
    cString ManualPoster {""};
    for (const cString &Image : RecordingImages) {
        ManualPoster = cString::sprintf("%s/%s", *RecPath, *Image);
        if (std::filesystem::exists(*ManualPoster)) {
            // dsyslog("flatPlus: Image found in %s/%s", *RecPath, *Image);
            found = ManualPoster;
            return true;
        }
        ManualPoster = cString::sprintf("%s/../../../%s", *RecPath, *Image);
        if (std::filesystem::exists(*ManualPoster)) {
            // dsyslog("flatPlus: Image found in %s/../../../%s", *RecPath, *Image);
            found = ManualPoster;
            return true;
        }
        ManualPoster = cString::sprintf("%s/../../%s", *RecPath, *Image);
        if (std::filesystem::exists(*ManualPoster)) {
            // dsyslog("flatPlus: Image found in %s/../../%s", *RecPath, *Image);
            found = ManualPoster;
            return true;
        }
    }
    dsyslog("flatPlus: cImageLoader::SearchRecordingPoster() No image found in %s or above.", *RecPath);
    return false;
}
