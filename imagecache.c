/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./imagecache.h"

#include "./config.h"
#include "./displaychannel.h"
#include "./displaymenu.h"
#include "./displaymessage.h"
#include "./displayreplay.h"
#include "./displaytracks.h"
#include "./displayvolume.h"

cImageCache::cImageCache() {}

cImageCache::~cImageCache() {}

void cImageCache::Create() {
    std::fill(std::begin(CacheImage), std::end(CacheImage), nullptr);
    std::fill(std::begin(CacheName), std::end(CacheName), "");
    std::fill(std::begin(CacheWidth), std::end(CacheWidth), -1);
    std::fill(std::begin(CacheHeight), std::end(CacheHeight), -1);

    m_InsertIndex = 0;
}

void cImageCache::Clear() {
    std::for_each(std::begin(CacheImage), std::end(CacheImage), [](auto &p) { delete p; });
    m_InsertIndex = 0;
}

bool cImageCache::RemoveFromCache(const std::string &Name) {
    std::string_view BaseFileName {""};
    for (uint i {0}; i < MaxImageCache; ++i) {
        BaseFileName = CacheName[i].substr(CacheName[i].find_last_of('/') + 1);  // Part after the last '/'

        if (BaseFileName == Name) {
            dsyslog("flatPlus: RemoveFromCache - %s", CacheName[i].c_str());
            delete CacheImage[i];
            CacheImage[i] = nullptr;
            CacheName[i] = "-Empty!-";  // Mark as empty because "" is for end of cache
            CacheWidth[i] = -1;
            CacheHeight[i] = -1;
            return true;
        }
    }
    return false;
}

cImage* cImageCache::GetImage(const std::string &Name, int Width, int Height) const {
    // dsyslog("flatPlus: Imagecache search for image %s Width %d Height %d", Name.c_str(), Width, Height);
    for (uint i {0}; i < MaxImageCache; ++i) {
        if (CacheName[i].empty())
            break;  // No more images in cache;
        // dsyslog("flatPlus: Imagecache index %d image %s Width %d Height %d", i, CacheName[i].c_str(),
        //          CacheWidth[i], CacheHeight[i]);
        if (CacheName[i] == Name && CacheWidth[i] == Width && CacheHeight[i] == Height)
            return CacheImage[i];
    }
    return nullptr;
}

void cImageCache::InsertImage(cImage *Image, const std::string &Name, int Width, int Height) {
    if (CacheImage[m_InsertIndex] != nullptr)
        delete CacheImage[m_InsertIndex];

    CacheImage[m_InsertIndex] = Image;
    CacheName[m_InsertIndex].reserve(Name.length());
    CacheName[m_InsertIndex] = Name;
    CacheWidth[m_InsertIndex] = Width;
    CacheHeight[m_InsertIndex] = Height;

    if (++m_InsertIndex >= MaxImageCache) {
        isyslog("flatPlus: Imagecache overflow, increase MaxImageCache (%d)", MaxImageCache);
        isyslog("flatPlus: Refilling imagecache keeping %d pre loaded images", m_InsertIndexBase);
        m_InsertIndex = m_InsertIndexBase;  // Keep images loaded at start
    }
}

void cImageCache::PreLoadImage() {
    cTimeMs Timer;  // Start timer

    cFlatDisplayChannel DisplayChannel(false);
    DisplayChannel.PreLoadImages();

    cFlatDisplayMenu Display_Menu;
    Display_Menu.PreLoadImages();

    cFlatDisplayReplay DisplayReplay(false);
    DisplayReplay.PreLoadImages();

    cFlatDisplayVolume DisplayVolume;
    DisplayVolume.PreLoadImages();

    m_InsertIndexBase = GetCacheCount();
    dsyslog("flatPlus: Imagecache pre load images time: %ld ms", Timer.Elapsed());
    dsyslog("flatPlus: Imagecache pre loaded images %d / %d", m_InsertIndexBase, MaxImageCache);
}
