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

#include <string>

#include "./flat.h"

using namespace Magick;

cImageLoader::cImageLoader() {}

cImageLoader::~cImageLoader() {}

cImage* cImageLoader::LoadLogo(const char *logo, int width, int height) {
    if (width == 0 || height == 0) return nullptr;

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: cImageLoader::LoadLogo() '%s' %dx%d", logo, width, height);
    cTimeMs Timer;  // Start timer
#endif

    // Plain logo without converting to lower including '/'
    cString File = cString::sprintf("%s/%s.%s", *Config.LogoPath, logo, *m_LogoExtension);
    std::string LogoLower {""};
    bool success {false};
    cImage *img {nullptr};
    for (uint i {0}; i < 3; ++i) {      // Run up to three times (0..2)
        if (i == 1) {                   // Second try (Plain logo not found)
            LogoLower.reserve(64);  // Filename without path
            LogoLower = logo;
            ToLowerCase(LogoLower);  // Convert to lowercase (A-Z)
            File = cString::sprintf("%s/%s.%s", *Config.LogoPath, LogoLower.c_str(), *m_LogoExtension);
        } else if (i == 2) {            // Third try. Search for lowercase logo with '~' for path '/'
            std::replace(LogoLower.begin(), LogoLower.end(), '/', '~');
            File = cString::sprintf("%s/%s.%s", *Config.LogoPath, LogoLower.c_str(), *m_LogoExtension);
        }

        img = ImgCache.GetImage(*File, width, height);  // Check if image is in imagecache

#ifdef DEBUGIMAGELOADTIME
        dsyslog("   search in cache #%d: %ld ms", i + 1, Timer.Elapsed());
#endif

        if (img) return img;  // Image found in imagecache

#ifdef DEBUGIMAGELOADTIME
        Timer.Set();  // Reset timer
#endif

        success = LoadImage(*File);  // Try to load image from disk
        if (!success) {              // Image not found on disk
            if (i == 2)              // Third try and not found
                isyslog("flatPlus: cImageLoader::LoadLogo() %s/%s.%s could not be loaded", *Config.LogoPath, logo,
                        *m_LogoExtension);
            continue;
        }

#ifdef DEBUGIMAGELOADTIME
        dsyslog("   load file from disk #%d: %ld ms", i + 1, Timer.Elapsed());
        Timer.Set();  // Reset timer
#endif

        img = CreateImage(width, height);
        if (!img) continue;

#ifdef DEBUGIMAGELOADTIME
        dsyslog("   scale logo #%d: %ld ms", i + 1, Timer.Elapsed());
#endif

        ImgCache.InsertImage(img, *File, width, height);  // Add image to imagecache
        return img;  // Image loaded from disk
    }  // for

    return nullptr;  // No image; so return nullptr
}

cImage* cImageLoader::LoadIcon(const char *cIcon, int width, int height) {
    if (width == 0 || height == 0) return nullptr;

    const cString File[] {cString::sprintf("%s/%s/%s.%s", *Config.IconPath, Setup.OSDTheme, cIcon, *m_LogoExtension),
                          cString::sprintf("%s/%s/%s.%s", *Config.IconPath, "default", cIcon, *m_LogoExtension)};
    cImage *img {nullptr};

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: cImageLoader::LoadIcon() '%s' %dx%d", *File[0], width, height);
    cTimeMs Timer;  // Start timer
#endif

    img = ImgCache.GetImage(*File[0], width, height);

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   search in cache: %ld ms", Timer.Elapsed());
#endif

    if (img)
        return img;  // Image found in imagecache
    else
        img = ImgCache.GetImage(*File[1], width, height);

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   search in cache (default): %ld ms", Timer.Elapsed());
#endif

    if (img) return img;  // Image found in imagecache

#ifdef DEBUGIMAGELOADTIME
    Timer.Set();  // Reset timer
#endif

    bool success = LoadImage(*File[0]);

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   load file from disk: %ld ms", Timer.Elapsed());
    Timer.Set();  // Reset timer
#endif

    bool DefaultPath {false};
    if (!success) {  // Search for logo in default folder
        success = LoadImage(*File[1]);

#ifdef DEBUGIMAGELOADTIME
        dsyslog("   load file from disk (default): %ld ms", Timer.Elapsed());
        Timer.Set();  // Reset timer
#endif

        if (!success) {
            isyslog("flatPlus: cImageLoader::LoadIcon() '%s' and '%s' could not be loaded", *File[0], *File[1]);
            return nullptr;
        }

        DefaultPath = true;
    }  // if (!success)

    img = CreateImage(width, height);

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   scale logo: %ld ms", Timer.Elapsed());
#endif

    if (img) {
        ImgCache.InsertImage(img, *File[DefaultPath ? 1 : 0], width, height);
        return img;
    }

    dsyslog("flatPlus: cImageLoader::LoadIcon() '%s' 'CreateImage' failed", *File[DefaultPath ? 1 : 0]);
    return nullptr;
}

cImage* cImageLoader::LoadFile(const char *cFile, int width, int height) {
    if (width == 0 || height == 0) return nullptr;

    const cString File = cFile;
    cImage *img {nullptr};

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: cImageLoader::LoadFile() '%s'", *File);
    cTimeMs Timer;  // Start timer
#endif

    img = ImgCache.GetImage(*File, width, height);

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   search in cache: %ld ms", Timer.Elapsed());
#endif

    if (img) return img;  // Image found in imagecache

#ifdef DEBUGIMAGELOADTIME
    Timer.Set();  // Reset timer
#endif

    const bool success = LoadImage(*File);
    if (!success) {
        isyslog("flatPlus: cImageLoader::LoadFile() '%s' could not be loaded", *File);
        return nullptr;
    }

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   load file from disk: %ld ms", Timer.Elapsed());
    Timer.Set();  // Reset timer
#endif

    img = CreateImage(width, height);

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   scale logo: %ld ms", Timer.Elapsed());
#endif

    if (img) {
        ImgCache.InsertImage(img, *File, width, height);
        return img;
    }

    dsyslog("flatPlus: cImageLoader::LoadFile() '%s' 'CreateImage' failed", *File);
    return nullptr;
}

void cImageLoader::ToLowerCase(std::string &str) {
    for (auto &ch : str) {
        if (ch <= 'Z' && ch >= 'A')  // Check for 'Z' first. Small letters start at 97
            ch += 32;  // Or: ch ^= 1 << 5;
    }
}

// TODO: Improve? What images are to expect?
bool cImageLoader::SearchRecordingPoster(const cString &RecPath, cString &found) {
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
