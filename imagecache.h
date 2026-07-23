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

    int GetCacheCount() const {
      return m_InsertIndex;
    }

    int GetIconCacheCount() const {
      return m_InsertIconIndex;
    }

    cImage *GetImage(const cString &Name, int Width, int Height, bool IsIcon = false) const;
    void InsertImage(cImage *Image, const cString &Name, int Width, int Height, bool IsIcon = false);

    void PreLoadImage();

    struct ImageKey {
        cString Name;
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
    std::unordered_map<ImageKey, std::size_t, ImageKeyHash> m_ImageIndex;
    std::unordered_map<ImageKey, std::size_t, ImageKeyHash> m_IconIndex;

    std::size_t m_InsertIndex {0};      // Imagecache index
    std::size_t m_InsertIndexBase {0};  // Imagecache after first fill at start

    std::size_t m_InsertIconIndex {0};      // Iconcache index
    std::size_t m_InsertIconIndexBase {0};  // Icon cache after first fill at start

    cImage *FindImage(const cString &Name, int Width, int Height, bool IsIcon) const;
    void InsertIntoCache(ImageData *Cache, std::size_t &InsertIndex, const std::size_t MaxSize, std::size_t BaseIndex,  // NOLINT
                         cImage *Image, const cString &Name, int Width, int Height);
};
