/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/osd.h>
#include <vdr/skins.h>

#include <array>
#include <string>

constexpr int MaxImageCache {1024};  // Image cache including two times 'LogoPreCache'
constexpr int MaxIconCache {1024};   // Icon cache (Skin icons)
constexpr int LogoPreCache {192};    // First x channel logos
//! Note: 'LogoPreCache' is used twice! One for 'displaychannel' and one for 'menu'
//! You must double the value for the real amount of pre cached logos

struct ImageData {
    cImage* Image {nullptr};
    std::string Name {""};  // Including full path
    int Width {-1};
    int Height {-1};
};

class cImageCache {
 public:
    cImageCache();
    ~cImageCache();

    void Create();
    void Clear();
    bool RemoveFromCache(const std::string &Name);

    int GetCacheCount() const {
      return m_InsertIndex + 1;
    }

    int GetIconCacheCount() const {
      return m_InsertIconIndex + 1;
    }

    cImage *GetImage(const std::string &Name, int Width, int Height, bool IsIcon = false) const;
    void InsertImage(cImage *Image, const std::string &Name, int Width, int Height, bool IsIcon = false);

    void PreLoadImage();

 private:
    std::array<ImageData, MaxImageCache> ImageCache;
    std::array<ImageData, MaxIconCache> IconCache;

    uint m_InsertIndex {0};      // Imagecache index
    uint m_InsertIndexBase {0};  // Imagecache after first fill at start

    uint m_InsertIconIndex {0};      // Imagecache index
    uint m_InsertIconIndexBase {0};  // Imagecache after first fill at start
};
