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

#include <memory>  // For std::unique_ptr
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
      return m_InsertIndex + 1;
    }

    int GetIconCacheCount() const {
      return m_InsertIconIndex + 1;
    }

    cImage *GetImage(const cString &Name, int Width, int Height, bool IsIcon = false) const;
    void InsertImage(cImage *Image, const cString &Name, int Width, int Height, bool IsIcon = false);

    void PreLoadImage();

 private:
    std::vector<ImageData> ImageCache;
    std::vector<ImageData> IconCache;

    std::size_t m_InsertIndex {0};      // Imagecache index
    std::size_t m_InsertIndexBase {0};  // Imagecache after first fill at start

    std::size_t m_InsertIconIndex {0};      // Iconcache index
    std::size_t m_InsertIconIndexBase {0};  // Icon cache after first fill at start

    void InsertIntoCache(ImageData *Cache, std::size_t &InsertIndex, const std::size_t MaxSize, std::size_t BaseIndex,  // NOLINT
                         cImage *Image, const cString &Name, int Width, int Height);
};
