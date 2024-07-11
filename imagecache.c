/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./imagecache.h"
#include <libgen.h>

#include "./config.h"
#include "./displaychannel.h"
#include "./displaymenu.h"
#include "./displaymessage.h"
#include "./displayreplay.h"
#include "./displaytracks.h"
#include "./displayvolume.h"

cImageCache::cImageCache() {}

cImageCache::~cImageCache() {}

void cImageCache::Create(void) {
    for (uint i {0}; i < MAX_IMAGE_CACHE; ++i) {
        CacheImage[i] = nullptr;
        CacheName[i] = "";
        CacheWidth[i] = -1;
        CacheHeight[i] = -1;
    }

    m_InsertIndex = 0;
}

void cImageCache::Clear(void) {
    for (uint i {0}; i < MAX_IMAGE_CACHE; ++i) {
        // if (CacheImage[i] != NULL)  //* 'delete' already checks for NULL
            delete CacheImage[i];
    }

    m_InsertIndex = 0;
}

bool cImageCache::RemoveFromCache(std::string Name) {
    std::string BaseFileName {""};
    BaseFileName.reserve(32);
    for (uint i {0}; i < MAX_IMAGE_CACHE; ++i) {
        BaseFileName = CacheName[i].substr(CacheName[i].find_last_of('/') + 1);  // Part after the last '/'

        if (BaseFileName == Name) {
            dsyslog("flatPlus: RemoveFromCache - %s", CacheName[i].c_str());
            CacheImage[i] = nullptr;
            CacheName[i] = "";
            CacheWidth[i] = -1;
            CacheHeight[i] = -1;
            return true;
        }
    }
    return false;
}

cImage* cImageCache::GetImage(const std::string &Name, int Width, int Height) {
    // dsyslog("flatPlus: Imagecache search for image %s Width %d Height %d", Name.c_str(), Width, Height);
    for (uint i {0}; i < MAX_IMAGE_CACHE; ++i) {
        // dsyslog("flatPlus: Imagecache index %d image %s Width %d Height %d", index, CacheName[i].c_str(),
        //          CacheWidth[i], CacheHeight[i]);
        if (CacheName[i] == Name && CacheWidth[i] == Width && CacheHeight[i] == Height)
            return CacheImage[i];
    }
    return NULL;
}

void cImageCache::InsertImage(cImage *Image, const std::string &Name, int Width, int Height) {
    // if (m_OverFlow) return;  //? Leave cache as is or refill?

    CacheImage[m_InsertIndex] = Image;
    CacheName[m_InsertIndex] = Name;
    CacheWidth[m_InsertIndex] = Width;
    CacheHeight[m_InsertIndex] = Height;

    ++m_InsertIndex;
    if (m_InsertIndex >= MAX_IMAGE_CACHE) {
        isyslog("flatPlus: Imagecache overflow, increase MAX_IMAGE_CACHE (%d)", MAX_IMAGE_CACHE);
        isyslog("flatPlus: Refilling imagecache keeping %d pre loaded images", m_InsertIndexBase);
        m_InsertIndex = m_InsertIndexBase;  // Keep pre loaded images (Loaded at start)
        // m_OverFlow = true;
    }
}

void cImageCache::PreLoadImage(void) {
    uint32_t tick1 = GetMsTicks();

    cFlatDisplayChannel DisplayChannel(false);
    DisplayChannel.PreLoadImages();

    cFlatDisplayMenu Display_Menu;
    Display_Menu.PreLoadImages();

    cFlatDisplayReplay DisplayReplay(false);
    DisplayReplay.PreLoadImages();

    cFlatDisplayVolume DisplayVolume;
    DisplayVolume.PreLoadImages();

    uint32_t tick2 = GetMsTicks();
    m_InsertIndexBase = GetCacheCount();
    dsyslog("flatPlus: Imagecache pre load images time: %d ms", tick2 - tick1);
    dsyslog("flatPlus: Imagecache pre loaded images %d / %d", m_InsertIndexBase, MAX_IMAGE_CACHE);
}
