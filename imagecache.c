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

bool cImageCache::RemoveFromCache(const cString &Name) {
    std::string_view BaseFileName {""}, DataNameView {""};
    std::string_view NameView {Name};
    for (auto &data : ImageCache) {
        DataNameView = *data.Name;
        if (DataNameView.empty()) break;  // No more images in cache
        BaseFileName = DataNameView.substr(DataNameView.find_last_of('/') + 1);  // Part after the last '/'
        if (BaseFileName == NameView) {
            dsyslog("flatPlus: RemoveFromCache: %s", *data.Name);
            delete data.Image;
            data.Image = nullptr;
            data.Name = "-Empty!-";  // Mark as empty because "" is for end of cache
            data.Width = -1;
            data.Height = -1;
            return true;
        }
    }
    return false;
}

cImage* cImageCache::GetImage(const cString &Name, int Width, int Height, bool IsIcon) const {
    std::string_view DataNameView {""};
    std::string_view NameView {Name};
    // dsyslog("flatPlus: Imagecache search for image %s Width %d Height %d", Name.c_str(), Width, Height);
    for (const auto &data : IsIcon ? IconCache : ImageCache) {
        DataNameView = *data.Name;
        // dsyslog("flatPlus: Imagecache image %s Width %d Height %d", data.Name.c_str(), data.Width, data.Height);
        if (DataNameView.empty()) break;  // No more mages in cache;
        if (DataNameView == NameView && data.Width == Width && data.Height == Height) { return data.Image; }
    }
    return nullptr;
}

void cImageCache::InsertImage(cImage *Image, const cString &Name, int Width, int Height, bool IsIcon) {
    // dsyslog("flatPlus: Imagecache insert image %s Width %d Height %d", Name.c_str(), Width, Height);
    if (IsIcon) {  // Insert into icon cache
        if (IconCache[m_InsertIconIndex].Image != nullptr)
            delete IconCache[m_InsertIconIndex].Image;  // Delete old image if exists

        IconCache[m_InsertIconIndex].Image = Image;     // Store image in cache
        IconCache[m_InsertIconIndex].Name = Name;
        IconCache[m_InsertIconIndex].Width = Width;
        IconCache[m_InsertIconIndex].Height = Height;

        if (++m_InsertIconIndex >= MaxIconCache) {
            isyslog("flatPlus: Iconcache overflow, increase MaxIconCache (%ld)", MaxIconCache);
            isyslog("flatPlus: Refilling iconcache keeping %ld pre loaded icons", m_InsertIconIndexBase);
            m_InsertIconIndex = m_InsertIconIndexBase;  // Keep images loaded at start
        }
    } else {  // Insert into image cache
        if (ImageCache[m_InsertIndex].Image != nullptr)
            delete ImageCache[m_InsertIndex].Image;  // Delete old image if exists

        ImageCache[m_InsertIndex].Image = Image;     // Store image in cache
        ImageCache[m_InsertIndex].Name = Name;
        ImageCache[m_InsertIndex].Width = Width;
        ImageCache[m_InsertIndex].Height = Height;

        if (++m_InsertIndex >= MaxImageCache) {
            isyslog("flatPlus: Imagecache overflow, increase MaxImageCache (%ld)", MaxImageCache);
            isyslog("flatPlus: Refilling imagecache keeping %ld pre loaded images", m_InsertIndexBase);
            m_InsertIndex = m_InsertIndexBase;  // Keep images loaded at start
        }
    }
}

// Preload images and icons
// This function is called at startup to load images and icons into the cache
// to speed up the display of the menu and other components.
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
    dsyslog("flatPlus: Imagecache pre loaded %ld images and %ld icons", m_InsertIndexBase, m_InsertIconIndexBase);
}
