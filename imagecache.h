/* -*- c++ -*-
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/osd.h>
#include <vdr/skins.h>

#include <cstring>
#include <memory>  // For std::unique_ptr
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>  // For std::vector

static constexpr std::size_t kMaxImageCache {1024};  // Image cache including two times 'kLogoPreCache'
static constexpr std::size_t kMaxIconCache {512};    // Icon cache (Skin icons)
static constexpr std::size_t kLogoPreCache {192};    // First x channel logos
//! Note: 'kLogoPreCache' is used twice! One for 'displaychannel' and one for 'menu'
//! You must double the value for the real amount of pre cached logos

struct ImageData {
    std::unique_ptr<cImage> Image {nullptr};  // cImage* Image {nullptr};
    cString Name {""};  // Including full path
    int16_t Width {-1};
    int16_t Height {-1};
};

class cImageCache {
 public:
    cImageCache();
    ~cImageCache();

    void Create();
    void Clear();
    bool RemoveFromCache(const cString &Name);

    int GetCacheCount(bool IconCache = false) const {
      return IconCache ? m_InsertIconIndex : m_InsertIndex;
    }

    cImage *GetImage(const cString &Name, int Width, int Height, bool IsIcon = false) const;
    void InsertImage(cImage *Image, const cString &Name, int Width, int Height, bool IsIcon = false);

    // File-path resolution cache. Avoids repeated stat()/LastModifiedTime() disk access
    // by caching which file path variant (or negative result) a lookup resolves to.
    // 'Key' is a caller-built, namespaced composite string (see cImageLoader).
    // Returns true on a cache hit and writes the resolved path to 'Out'.
    // 'Out' may be empty, indicating a cached negative result (path does not exist).
    bool ResolvePath(const std::string &Key, cString &Out) const;  // NOLINT (readability-convert-member-functions-to-static)
    void CacheResolvedPath(const std::string &Key, const cString &Path);

    void PreLoadImage();

    struct ImageKey {
        cString Name {""};
        int16_t Width {0};
        int16_t Height {0};

        bool operator==(const ImageKey &other) const {
            return Width == other.Width && Height == other.Height && std::strcmp(*Name, *other.Name) == 0;
        }
    };

    struct ImageKeyHash {
        std::size_t operator()(const ImageKey &k) const noexcept {
            const std::string_view sv {*k.Name};
            const std::size_t h1 {std::hash<std::string_view>{}(sv)};
            const std::size_t h2 {std::hash<int16_t>{}(k.Width)};
            const std::size_t h3 {std::hash<int16_t>{}(k.Height)};
            return ((h1 ^ (h2 << 1)) ^ (h3 << 1));
        }
    };

 private:
    std::vector<ImageData> ImageCache;
    std::vector<ImageData> IconCache;

    // O(1) lookup indexes for (Name, Width, Height)->slot
    mutable std::unordered_map<ImageKey, std::size_t, ImageKeyHash> m_ImageIndex;
    mutable std::unordered_map<ImageKey, std::size_t, ImageKeyHash> m_IconIndex;

    // O(1) lookup cache for resolved file paths. Value is the full path, or empty
    // for a cached negative result (file does not exist).
    mutable std::unordered_map<std::string, cString> m_PathCache;
    // Snapshot of m_PathCache taken after PreLoadImage(). Kept when m_PathCache
    // overflows, so preloaded entries survive (mirrors the image cache base index).
    mutable std::unordered_map<std::string, cString> m_PathBaseCache;
    static constexpr std::size_t kMaxPathCache {2048};  // Upper bound; reduced to base on overflow

    std::size_t m_InsertIndex {0};      // Imagecache index
    std::size_t m_InsertIndexBase {0};  // Imagecache after first fill at start

    std::size_t m_InsertIconIndex {0};      // Iconcache index
    std::size_t m_InsertIconIndexBase {0};  // Icon cache after first fill at start

    std::size_t m_PathInsertIndexBase {0};  // Path cache entries present after preload (kept on overflow)

    void InsertIntoCache(ImageData *Cache, std::size_t &InsertIndex, const std::size_t MaxSize, std::size_t BaseIndex,  // NOLINT
                         cImage *Image, const cString &Name, int Width, int Height);
};
