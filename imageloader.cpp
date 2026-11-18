/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./imageloader.h"

#include <vdr/tools.h>

#include <algorithm>
#include <array>
#include <string>

#include "./flat.h"

using Magick::Image;
using Magick::Geometry;

// External declaration of global logo overwrite flag
extern bool g_LogoBgOverwrite;

namespace {

// Composite, namespaced keys for the path resolution cache in cImageCache.
// Prefixes ('L', 'I', 'P') prevent collisions between logos, icons and posters.
// '\x1f' (unit separator) is used as a field delimiter.
std::string LogoPathKey(const char *logo) {
    std::string key {"L"};
    key += *Config.LogoPath;
    key += '\x1f';
    key += logo;
    return key;
}

std::string IconPathKey(const char *icon) {
    std::string key {"I"};
    key += *Config.IconPath;
    key += '\x1f';
    key += Setup.OSDTheme;
    key += '\x1f';
    key += icon;
    return key;
}

std::string PosterPathKey(const cString &RecPath) {
    std::string key {"P"};
    key += *itoa(Config.TVScraperSearchLocalPosters);
    key += '\x1f';
    key += *RecPath;
    return key;
}

}  // namespace

cImageLoader::cImageLoader() {}

cImageLoader::~cImageLoader() {}

// Cache lookup: GetImage -> LoadImage -> CreateImage -> InsertImage.
cImage *cImageLoader::LoadAndScale(const cString &File, int width, int height, bool IsIcon) {
    cImage *img {ImgCache.GetImage(File, width, height, IsIcon)};
    if (img) return img;

    if (!LoadImage(*File)) return nullptr;

    img = CreateImage(width, height);
    if (!img) return nullptr;

    ImgCache.InsertImage(img, File, width, height, IsIcon);
    return img;
}

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
 * @param Quiet If true, do not log an error message if the logo is not found.
 * @return The loaded and scaled logo, or nullptr if the logo could not be loaded.
 */
cImage* cImageLoader::GetLogo(const char *logo, int width, int height, bool Quiet) {
    if (width <= 0 || height <= 0 || isempty(logo)) return nullptr;

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: cImageLoader::GetLogo() '%s' %dx%d", logo, width, height);
    cTimeMs Timer;  // Start timer
#endif

    const std::string Key {LogoPathKey(logo)};

    // Fast path: reuse a previously resolved file path (avoids the variant loop/stat calls).
    cString CachedFile {""};
    if (ImgCache.ResolvePath(Key, CachedFile)) {
#ifdef DEBUGIMAGELOADTIME
        dsyslog("flatPlus: GetLogo() resolve cache HIT '%s' -> '%s'", logo, *CachedFile);
#endif
        if (isempty(*CachedFile)) return nullptr;  // Cached negative result
        return LoadAndScale(CachedFile, width, height, false);
    }

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: GetLogo() resolve cache MISS '%s'", logo);
#endif

    // Plain logo without converting to lower including '/'
    cString File {cString::sprintf("%s/%s.%s", *Config.LogoPath, logo, *m_LogoExtension)};
    std::string LogoLower {""};
    cImage *img {nullptr};
    for (int8_t i {0}; i < 3; ++i) {      // Run up to three times (0..2)
        if (i == 1) {                   // Second try (Plain logo not found)
            LogoLower.reserve(64);  // Filename without path
            LogoLower = logo;
            ToLowerCase(LogoLower);  // Convert to lowercase (A-Z)
            File = cString::sprintf("%s/%s.%s", *Config.LogoPath, LogoLower.c_str(), *m_LogoExtension);
        } else if (i == 2) {            // Third try. Search for lowercase logo with '~' for path '/'
            // Only convert '/' to '~' if the logo contains a '/' (e.g. 'logo 1/2.png')
            if (LogoLower.find('/') == std::string::npos) {
                isyslog("flatPlus: cImageLoader::GetLogo() %s/%s.%s could not be loaded (no '/' in logo name)",
                        *Config.LogoPath, logo, *m_LogoExtension);
                ImgCache.CacheResolvedPath(Key, cString(""));
                return nullptr;
            }

            std::replace(LogoLower.begin(), LogoLower.end(), '/', '~');
            File = cString::sprintf("%s/%s.%s", *Config.LogoPath, LogoLower.c_str(), *m_LogoExtension);
        }

        img = LoadAndScale(File, width, height, false);

#ifdef DEBUGIMAGELOADTIME
        dsyslog("   search in cache #%d: %ld ms", i + 1, Timer.Elapsed());
        Timer.Set();  // Reset timer
#endif

        if (img) {  // Image found in imagecache or loaded from disk
            ImgCache.CacheResolvedPath(Key, File);
            return img;
        }
    }  // for

    if (!Quiet)
        isyslog("flatPlus: cImageLoader::GetLogo() %s/%s.%s could not be loaded", *Config.LogoPath, logo,
                *m_LogoExtension);
    ImgCache.CacheResolvedPath(Key, cString(""));
    return nullptr;  // No image; so return nullptr
}

/**
 * @brief Load an icon from a file and scale it to the given width and height.
 *
 * This function tries to load the icon from the configured icon path and
 * scales it to the given width and height. If the icon is not found, the
 * function logs an error message and returns nullptr.
 *
 * @param cIcon The name of the icon (without path).
 * @param width The desired width of the icon.
 * @param height The desired height of the icon.
 * @return The loaded and scaled icon, or nullptr if the icon could not be loaded.
 */
cImage* cImageLoader::GetIcon(const char *cIcon, int width, int height) {
    if (width <= 0 || height <= 0 || isempty(cIcon)) return nullptr;

    const std::string Key {IconPathKey(cIcon)};

    // Fast path: reuse a previously resolved file path (avoids the theme/default probing).
    cString CachedFile {""};
    if (ImgCache.ResolvePath(Key, CachedFile)) {
#ifdef DEBUGIMAGELOADTIME
        dsyslog("flatPlus: GetIcon() resolve cache HIT '%s' -> '%s'", cIcon, *CachedFile);
#endif
        if (isempty(*CachedFile)) return nullptr;  // Cached negative result
        return LoadAndScale(CachedFile, width, height, true);
    }

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: GetIcon() resolve cache MISS '%s'", cIcon);
#endif

    const cString ThemeFile {cString::sprintf("%s/%s/%s.%s", *Config.IconPath, Setup.OSDTheme, cIcon,
                                              *m_LogoExtension)};
    const cString DefaultFile {cString::sprintf("%s/%s/%s.%s", *Config.IconPath, "default", cIcon,
                                                *m_LogoExtension)};

    // Try theme-specific path first, then fall back to 'default'.
    for (const cString &File : {ThemeFile, DefaultFile}) {
#ifdef DEBUGIMAGELOADTIME
        dsyslog("flatPlus: cImageLoader::GetIcon() '%s'", *File);
#endif
        cImage *img {ImgCache.GetImage(File, width, height, true)};
        if (img) {
            ImgCache.CacheResolvedPath(Key, File);
            return img;
        }

        if (!LoadImage(*File)) continue;  // Not found, try next candidate

        img = CreateImage(width, height);
        if (!img) {
            dsyslog("flatPlus: cImageLoader::GetIcon() '%s' 'CreateImage' failed", *File);
            ImgCache.CacheResolvedPath(Key, File);
            return nullptr;
        }

        ImgCache.InsertImage(img, File, width, height, true);
        ImgCache.CacheResolvedPath(Key, File);
        return img;
    }

    isyslog("flatPlus: cImageLoader::GetIcon() '%s' could not be loaded", *DefaultFile);
    ImgCache.CacheResolvedPath(Key, cString(""));
    return nullptr;
}

cImage* cImageLoader::GetFile(const char *cFile, int width, int height) {
    // 'cFile' is already a resolved full path (e.g. from SearchRecordingPoster() or the actor
    // thumb path). There is no path-variant probing here, so the path resolution cache does not
    // apply: The positive case is covered by the image cache (ImgCache.GetImage() below), and
    // negatives are prevented upstream (SearchRecordingPoster() caches its misses).
    if (width <= 0 || height <= 0 || isempty(cFile)) return nullptr;

    const cString File {cFile};

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: cImageLoader::GetFile() '%s'", *File);
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
        isyslog("flatPlus: cImageLoader::GetFile() '%s' could not be loaded", *File);
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

    dsyslog("flatPlus: cImageLoader::GetFile() '%s' 'CreateImage' failed", *File);
    return nullptr;
}

/**
 * @brief Load 'logo_background' from the configured logo path or theme directory.
 *
 * If g_LogoBgOverwrite is true, load the logo from the configured logo path.
 * Otherwise, load the logo from the theme directory. g_LogoBgOverwrite is set
 * in cFlatDisplayChannel::PreLoadImages().
 *
 * @param Width The desired width of the logo.
 * @param Height The desired height of the logo.
 * @return The loaded and scaled logo, or nullptr if the logo could not be loaded.
 */
cImage* cImageLoader::GetLogoBg(int Width, int Height) {
    static constexpr const char *LogoBgName {"logo_background"};
    return (g_LogoBgOverwrite) ? GetLogo(LogoBgName, Width, Height) : GetIcon(LogoBgName, Width, Height);
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

    const std::string Key {PosterPathKey(RecPath)};

    // Fast path: reuse a previously resolved poster path (avoids up to 12 stat calls).
    cString CachedFound {""};
    if (ImgCache.ResolvePath(Key, CachedFound)) {
#ifdef DEBUGIMAGELOADTIME
        dsyslog("flatPlus: SearchRecordingPoster() resolve cache HIT '%s' -> '%s'", *RecPath, *CachedFound);
#endif
        if (isempty(*CachedFound)) {
            dsyslog("flatPlus: cImageLoader::SearchRecordingPoster() No image found for %s", *RecPath);
            return false;
        }
        found = CachedFound;
        return true;
    }

#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: SearchRecordingPoster() resolve cache MISS '%s'", *RecPath);
#endif

    static constexpr std::array<const char *, 4> Images {"banner.jpg", "poster.jpg", "fanart.jpg", "cover_vdr.jpg"};
    for (const char* const &Image : Images) {
        if (CheckImageExistence(RecPath, Image, found)) {
            ImgCache.CacheResolvedPath(Key, found);
            return true;
        }
    }

    dsyslog("flatPlus: cImageLoader::SearchRecordingPoster() No image found for %s", *RecPath);
    ImgCache.CacheResolvedPath(Key, cString(""));
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
bool cImageLoader::CheckImageExistence(const cString &RecPath, const cString &Image, cString &found) const {
    cString ManualPoster = cString::sprintf("%s/%s", *RecPath, *Image);
    if (LastModifiedTime(*ManualPoster)) {
        found = ManualPoster;
        return true;
    }
    if (Config.TVScraperSearchLocalPosters == 2) return false;  // Search only in recording folder

    ManualPoster = cString::sprintf("%s/../../../%s", *RecPath, *Image);
    if (LastModifiedTime(*ManualPoster)) {
        found = ManualPoster;
        return true;
    }
    ManualPoster = cString::sprintf("%s/../../%s", *RecPath, *Image);
    if (LastModifiedTime(*ManualPoster)) {
        found = ManualPoster;
        return true;
    }
    return false;
}
