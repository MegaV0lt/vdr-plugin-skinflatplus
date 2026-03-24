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

cImageCache::cImageCache() :
    ImageCache(kMaxImageCache),  // Initialize vector with fixed size
    IconCache(kMaxIconCache) {}

cImageCache::~cImageCache() {}  // std::unique_ptr handles memory deallocation automatically

void cImageCache::Create() {
    // Reset Image and Icon caches to default empty state
    // The Clear() method already performs the necessary reset for each ImageData element,
    // making it suitable for initializing the cache to an empty state.
    Clear();

    // m_InsertIndex and m_InsertIconIndex are already set to 0 by Clear()
    // but explicitly setting them here ensures clarity if Clear() logic changes.
    m_InsertIndex = 0;
    m_InsertIconIndex = 0;
}

void cImageCache::Clear() {
    // std::unique_ptr handles memory deallocation automatically when ImageData objects are destroyed
    // or overwritten. To explicitly clear and reset, we can re-initialize or reset unique_ptrs.
    // For a full clear, simply re-creating the vectors or resetting each unique_ptr is effective.
    for (auto &data : ImageCache) {
        data.Image.reset();  // Release ownership and delete the managed object
        data.Name = "";
        data.Width = -1;
        data.Height = -1;
    }
    for (auto &data : IconCache) {
        data.Image.reset();  // Release ownership and delete the managed object
        data.Name = "";
        data.Width = -1;
        data.Height = -1;
    }

    m_InsertIndex = 0;
    m_InsertIconIndex = 0;
}

bool cImageCache::RemoveFromCache(const cString &Name) {
    std::string_view svBaseFileName {""}, svDataName{""};
    std::string_view svName {*Name};

    for (auto &data : ImageCache) {
        // Check if the ImageData entry is valid (not marked as empty)
        svDataName = *data.Name;  // Get the name from the cache entry
        if (data.Image == nullptr && svDataName.empty()) {
            // This assumes that an empty name and null image means the end of valid entries
            // or an explicitly empty slot. If "" is a valid name, this logic needs adjustment.
            break;
        }

        // Find the last '/' and extract the base filename
        const std::size_t LastSlashPos = svDataName.find_last_of('/');
        if (LastSlashPos != std::string_view::npos) {
            svBaseFileName = svDataName.substr(LastSlashPos + 1);
        } else {
            svBaseFileName = svDataName;  // No slash, so the whole name is the base filename
        }

        if (svBaseFileName == svName) {
            dsyslog("flatPlus: RemoveFromCache: %s", *data.Name);
            data.Image.reset();  // This deletes the cImage object and sets Image to nullptr
            data.Name = "-Empty!-";  // Mark as empty because "" is for end of cache
            data.Width = -1;
            data.Height = -1;
            return true;
        }
    }
    return false;
}

cImage* cImageCache::GetImage(const cString &Name, int Width, int Height, bool IsIcon) const {
    std::string_view svDataName {""};
    std::string_view svName {*Name};
    const auto &cache = IsIcon ? IconCache : ImageCache;

    for (const auto &data : cache) {
        svDataName = *data.Name;  // Get the name from the cache entry
        // Check if the ImageData entry is valid (not marked as empty)
        if (data.Image == nullptr && svDataName.empty()) {
            break;  // No more valid images in cache
        }
        if (svDataName == svName && data.Width == Width && data.Height == Height) {
            return data.Image.get();  // Return the cached image if found
        }
    }
    return nullptr;
}

void cImageCache::InsertIntoCache(ImageData *Cache, std::size_t &InsertIndex, const std::size_t MaxSize,
                                  std::size_t BaseIndex, cImage *Image, const cString &Name, int Width, int Height) {
    // std::unique_ptr will automatically delete the old image if one exists when a new one is assigned
    Cache[InsertIndex].Image = std::unique_ptr<cImage>(Image);  // Store image in cache
    Cache[InsertIndex].Name = Name;
    Cache[InsertIndex].Width = Width;
    Cache[InsertIndex].Height = Height;

    if (++InsertIndex >= MaxSize) {
        isyslog("flatPlus: Cache overflow, increase Cachesize (%ld)", MaxSize);
        isyslog("flatPlus: Refilling cache keeping %ld pre loaded icons", BaseIndex);
        InsertIndex = BaseIndex;  // Keep images loaded at start
    }
}

void cImageCache::InsertImage(cImage *Image, const cString &Name, int Width, int Height, bool IsIcon) {
    // dsyslog("flatPlus: Imagecache insert image %s Width %d Height %d", Name.c_str(), Width, Height);
    if (IsIcon) {  // Insert into icon cache
        InsertIntoCache(IconCache.data(), m_InsertIconIndex, kMaxIconCache, m_InsertIconIndexBase, Image, Name, Width,
                        Height);
    } else {  // Insert into image cache
        InsertIntoCache(ImageCache.data(), m_InsertIndex, kMaxImageCache, m_InsertIndexBase, Image, Name, Width,
                        Height);
    }
}

// Preload images and icons
// This function is called at startup to load images and icons into the cache
// to speed up the display of the menu and other components.
void cImageCache::PreLoadImage() {
    cTimeMs Timer;  // Start timer

    cFlatDisplayChannel DisplayChannel(false);
    // Called first. Also used to determine if 'logo_background' should be loaded from logo path or theme path
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
