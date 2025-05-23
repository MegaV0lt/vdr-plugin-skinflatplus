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
    std::fill(std::begin(ImageCache), std::end(ImageCache), ImageData{nullptr, "", -1, -1});
    std::fill(std::begin(IconCache), std::end(IconCache), ImageData{nullptr, "", -1, -1});

    m_InsertIndex = 0;
    m_InsertIconIndex = 0;
}

void cImageCache::Clear() {
    std::for_each(std::begin(ImageCache), std::end(ImageCache), [](auto &data) { delete data.Image; });
    std::for_each(std::begin(IconCache), std::end(IconCache), [](auto &data) { delete data.Image; });

    m_InsertIndex = 0;
    m_InsertIconIndex = 0;
}

bool cImageCache::RemoveFromCache(const std::string &Name) {
    std::string_view BaseFileName {""};
    for (uint i {0}; i < MaxImageCache; ++i) {
        BaseFileName = ImageCache[i].Name.substr(ImageCache[i].Name.find_last_of('/') + 1);  // Part after the last '/'

        if (BaseFileName == Name) {
            dsyslog("flatPlus: RemoveFromCache - %s", ImageCache[i].Name.c_str());
            delete ImageCache[i].Image;
            ImageCache[i].Image = nullptr;
            ImageCache[i].Name = "-Empty!-";  // Mark as empty because "" is for end of cache
            ImageCache[i].Width = -1;
            ImageCache[i].Height = -1;
            return true;
        }
    }
    return false;
}

cImage* cImageCache::GetImage(const std::string &Name, int Width, int Height) const {
    // dsyslog("flatPlus: Imagecache search for image %s Width %d Height %d", Name.c_str(), Width, Height);
    for (uint i {0}; i < MaxImageCache; ++i) {
        if (ImageCache[i].Name.empty())
            break;  // No more images in cache;
        // dsyslog("flatPlus: Imagecache index %d image %s Width %d Height %d", i, ImageCache[i].Name.c_str(),
        //          ImageCache[i].Width, ImageCache[i].Height);
        if (ImageCache[i].Name == Name && ImageCache[i].Width == Width && ImageCache[i].Height == Height)
            return ImageCache[i].Image;
    }
    return nullptr;
}

cImage* cImageCache::GetIcon(const std::string &Name, int Width, int Height) const {
    // dsyslog("flatPlus: Iconcache search for icon %s Width %d Height %d", Name.c_str(), Width, Height);
    for (uint i {0}; i < MaxIconCache; ++i) {
        if (IconCache[i].Name.empty())
            break;  // No more icons in cache;
        // dsyslog("flatPlus: Iconcache index %d icon %s Width %d Height %d", i, IconCache[i].Name.c_str(),
        //          IconCache[i].Width, IconCache[i].Height);
        if (IconCache[i].Name == Name && IconCache[i].Width == Width && IconCache[i].Height == Height)
            return IconCache[i].Image;
    }
    return nullptr;
}

void cImageCache::InsertImage(cImage *Image, const std::string &Name, int Width, int Height) {
    if (ImageCache[m_InsertIndex].Image != nullptr)
        delete ImageCache[m_InsertIndex].Image;

    ImageCache[m_InsertIndex].Image = Image;
    ImageCache[m_InsertIndex].Name.reserve(Name.length());
    ImageCache[m_InsertIndex].Name = Name;
    ImageCache[m_InsertIndex].Width = Width;
    ImageCache[m_InsertIndex].Height = Height;

    if (++m_InsertIndex >= MaxImageCache) {
        isyslog("flatPlus: Imagecache overflow, increase MaxImageCache (%d)", MaxImageCache);
        isyslog("flatPlus: Refilling imagecache keeping %d pre loaded images", m_InsertIndexBase);
        m_InsertIndex = m_InsertIndexBase;  // Keep images loaded at start
    }
}

void cImageCache::InsertIcon(cImage *Image, const std::string &Name, int Width, int Height) {
    if (IconCache[m_InsertIconIndex].Image != nullptr)
        delete IconCache[m_InsertIconIndex].Image;

    IconCache[m_InsertIconIndex].Image = Image;
    IconCache[m_InsertIconIndex].Name.reserve(Name.length());
    IconCache[m_InsertIconIndex].Name = Name;
    IconCache[m_InsertIconIndex].Width = Width;
    IconCache[m_InsertIconIndex].Height = Height;

    if (++m_InsertIconIndex >= MaxIconCache) {
        isyslog("flatPlus: Iconcache overflow, increase MaxIconCache (%d)", MaxIconCache);
        isyslog("flatPlus: Refilling iconcache keeping %d pre loaded icons", m_InsertIconIndexBase);
        m_InsertIconIndex = m_InsertIconIndexBase;  // Keep icons loaded at start
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
    m_InsertIconIndexBase = GetIconCacheCount();
    dsyslog("flatPlus: Imagecache pre load images and icons time: %ld ms", Timer.Elapsed());
    dsyslog("flatPlus: Imagecache pre loaded %d images and %d icons", m_InsertIndexBase, m_InsertIconIndexBase);
}
