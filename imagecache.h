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
#include <string>

#define MAX_IMAGE_CACHE     1024
#define LOGO_PRE_CACHE      192
//! Note: LOGO_PRE_CACHE is used twice! One for 'displaychannel' and one for 'menu'
//! You must double the value for the real amount of pre cached logos

class cImageCache {
 private:
    cImage *CacheImage[MAX_IMAGE_CACHE];
    std::string CacheName[MAX_IMAGE_CACHE];
    int CacheWidth[MAX_IMAGE_CACHE];
    int CacheHeight[MAX_IMAGE_CACHE];

    int m_InsertIndex {0};      // Imagecache index
    int m_InsertIndexBase {0};  // Imagecache after first fill at start
    // bool m_OverFlow = false;    // Set when cache is full

 public:
    cImageCache();
    ~cImageCache();

    void Create(void);
    void Clear(void);
    bool RemoveFromCache(std::string Name);

    int GetCacheCount(void) {
      // if (m_OverFlow) return MAX_IMAGE_CACHE;
      return m_InsertIndex + 1;
    }

    cImage *GetImage(const std::string &Name, int Width, int Height);
    void InsertImage(cImage *Image, const std::string &Name, int Width, int Height);

    void PreLoadImage(void);
};
