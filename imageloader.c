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

using Magick::Image;
using Magick::Geometry;

cImageLoader::cImageLoader() {}

cImageLoader::~cImageLoader() {}

/**
 * @brief Load a logo from a file and scale it to the given width and height.
 *
 * This function tries to load the logo from the configured logo path and
 * scales it to the given width and height. If the logo is not found, the
 * function logs an error message and returns nullptr.
 *
 * @param logo The name of the logo (without path).
 * @param width The desired width of the logo.
 * @param height The desired height of the logo.
 * @return The loaded and scaled logo, or nullptr if the logo could not be loaded.
 */
cImage* cImageLoader::LoadLogo(const char *logo, int width, int height) {
    if (width < 0 || height < 0 || isempty(logo)) return nullptr;

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
        Timer.Set();  // Reset timer
#endif

        if (img) return img;  // Image found in imagecache

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
    if (width < 0 || height < 0 || isempty(cIcon)) return nullptr;

    cString File = cString::sprintf("%s/%s/%s.%s", *Config.IconPath, Setup.OSDTheme, cIcon, *m_LogoExtension);

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: cImageLoader::LoadIcon() '%s'", *File);
    cTimeMs Timer;  // Start timer
#endif

    cImage *img {ImgCache.GetImage(*File, width, height)};

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   search in cache: %d ms", Timer.Elapsed());
    Timer.Set();  // Reset timer
#endif

    if (img) return img;

    bool success {LoadImage(*File)};

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   load file from disk: %d ms", Timer.Elapsed());
#endif

    if (!success) {  // Search for logo in default folder
        File = cString::sprintf("%s/%s/%s.%s", *Config.IconPath, "default", cIcon, *m_LogoExtension);

#ifdef DEBUGIMAGELOADTIME
        dsyslog("flatPlus: cImageLoader::LoadIcon() '%s'", *File);
        Timer.Set();  // Reset timer
#endif

        img = ImgCache.GetImage(*File, width, height);

#ifdef DEBUGIMAGELOADTIME
        dsyslog("   search in cache: %d ms", Timer.Elapsed());
        Timer.Set();  // Reset timer
#endif

        if (img) return img;

        success = LoadImage(*File);

#ifdef DEBUGIMAGELOADTIME
        dsyslog("   load file from disk: %d ms", Timer.Elapsed());
        Timer.Set();  // Reset timer
#endif

        if (!success) {
            isyslog("flatPlus: cImageLoader::LoadIcon() '%s' could not be loaded", *File);
            return nullptr;
        }
    }

    img = CreateImage(width, height);

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   scale logo: %d ms", Timer.Elapsed());
#endif

    if (!img) {
        dsyslog("flatPlus: cImageLoader::LoadIcon() '%s' 'CreateImage' failed", *File);
        return nullptr;
    }

    ImgCache.InsertImage(img, *File, width, height);
    return img;
}

cImage* cImageLoader::LoadFile(const char *cFile, int width, int height) {
    if (width < 0 || height < 0 || isempty(cFile)) return nullptr;

    const cString File = cFile;

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: cImageLoader::LoadFile() '%s'", *File);
    cTimeMs Timer;  // Start timer
#endif

    cImage *img {ImgCache.GetImage(*File, width, height)};

#ifdef DEBUGIMAGELOADTIME
    dsyslog("   search in cache: %ld ms", Timer.Elapsed());
    Timer.Set();  // Reset timer
#endif

    if (img) return img;  // Image found in imagecache

    const bool success {LoadImage(*File)};
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
    dsyslog("   scale image: %ld ms", Timer.Elapsed());
#endif

    if (img) {
        ImgCache.InsertImage(img, *File, width, height);
        return img;
    }

    dsyslog("flatPlus: cImageLoader::LoadFile() '%s' 'CreateImage' failed", *File);
    return nullptr;
}

/**
 * @brief Convert a string to lower case.
 *
 * This function is used to convert file names to lower case when searching for images
 *
 * @param str The string to convert to lower case
 */
void cImageLoader::ToLowerCase(std::string &str) {
    for (auto &ch : str) {
        if (ch <= 'Z' && ch >= 'A')  // Check for 'Z' first. Small letters start at 97
            ch |= 0x20;  // Bitwise OR operation. ASCII values of uppercase letters are contiguous and can
                         // be converted to lowercase by setting the 5th bit (0x20)
    }
}

// TODO: Improve? What images are to expect?
bool cImageLoader::SearchRecordingPoster(const cString &RecPath, cString &found) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cImageLoader::SearchRecordingPoster()");
#endif

    static const cString RecordingImages[4] {"cover_vdr.jpg", "poster.jpg", "banner.jpg", "fanart.jpg"};
    for (const cString &Image : RecordingImages) {
        if (CheckImageExistence(RecPath, Image, found)) {
            return true;
        }
    }

    dsyslog("flatPlus: cImageLoader::SearchRecordingPoster() No image found in %s or above.", *RecPath);
    return false;
}

/**
 * @brief Check if a certain image exists in the recording's folder.
 *
 * Given a recording path and an image name, this function checks if the image exists in the recording's folder,
 * in the parent folder, or in the grand parent folder. If the image is found, its full path is stored in @a found.
 *
 * @param RecPath The path to the recording.
 * @param Image The image name to check for.
 * @param found The full path to the found image, if any.
 *
 * @return true if the image exists, false otherwise.
 */
bool cImageLoader::CheckImageExistence(const cString &RecPath, const cString &Image, cString &found) {
    cString ManualPoster = cString::sprintf("%s/%s", *RecPath, *Image);
    if (std::filesystem::exists(*ManualPoster)) {
        found = ManualPoster;
        return true;
    }
    ManualPoster = cString::sprintf("%s/../../../%s", *RecPath, *Image);
    if (std::filesystem::exists(*ManualPoster)) {
        found = ManualPoster;
        return true;
    }
    ManualPoster = cString::sprintf("%s/../../%s", *RecPath, *Image);
    if (std::filesystem::exists(*ManualPoster)) {
        found = ManualPoster;
        return true;
    }
    return false;
}
