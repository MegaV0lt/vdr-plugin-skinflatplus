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

cImageCache::cImageCache()
    : ImageCache(kMaxImageCache),  // Initialize vector with fixed size
      IconCache(kMaxIconCache) {}

cImageCache::~cImageCache() = default;

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

    m_ImageIndex.clear();
    m_IconIndex.clear();

    m_InsertIndex = 0;
    m_InsertIconIndex = 0;
}

static std::string_view BaseNameFromCacheName(std::string_view full) {
    const std::size_t lastSlash = full.find_last_of('/');
    return (lastSlash != std::string_view::npos) ? full.substr(lastSlash + 1) : full;
}

cImage *cImageCache::FindImage(const cString &Name, int Width, int Height, bool IsIcon) const {
    const ImageKey key {Name, static_cast<int16_t>(Width), static_cast<int16_t>(Height)};

    const auto &idx = IsIcon ? m_IconIndex : m_ImageIndex;
    const auto it = idx.find(key);
    if (it != idx.end()) {
        const std::size_t slot = it->second;
        const auto &cache = IsIcon ? IconCache : ImageCache;
        if (slot < cache.size()) {
            const ImageData &data = cache[slot];
            if (data.Image && data.Width == Width && data.Height == Height &&
                std::strcmp(*data.Name, *Name) == 0) {
                return data.Image.get();
            }
        }
    }

    // Fallback to linear search if not found in index (should not happen if index is maintained correctly)
    const auto &cache = IsIcon ? IconCache : ImageCache;
    for (const auto &data : cache) {
        if (!data.Image) continue;
        if (data.Width != Width || data.Height != Height) continue;
        if (std::strcmp(*data.Name, *Name) != 0) continue;
        return data.Image.get();
    }

    return nullptr;
}

cImage *cImageCache::GetImage(const cString &Name, int Width, int Height, bool IsIcon) const {
    const cImage *img {FindImage(Name, Width, Height, IsIcon)};
    if (img) return const_cast<cImage *>(img);

    return nullptr;
}

void cImageCache::InsertImage(cImage *Image, const cString &Name, int Width, int Height, bool IsIcon) {
    if (!Image) return;

    if (FindImage(Name, Width, Height, IsIcon)) {  // Image already in cache
        delete Image;
        return;
    }

    if (IsIcon) {
        // Remove any previous mapping that points to the slot we are about to overwrite.
        const std::size_t slot = m_InsertIconIndex;
        for (auto it = m_IconIndex.begin(); it != m_IconIndex.end();) {
            if (it->second == slot) it = m_IconIndex.erase(it);
            else
                ++it;
        }

        InsertIntoCache(IconCache.data(), m_InsertIconIndex, kMaxIconCache, m_InsertIconIndexBase, Image, Name,
                         Width, Height);

        const ImageKey key {Name, static_cast<int16_t>(Width), static_cast<int16_t>(Height)};
        m_IconIndex[key] = slot;
    } else {
        const std::size_t slot = m_InsertIndex;
        for (auto it = m_ImageIndex.begin(); it != m_ImageIndex.end();) {
            if (it->second == slot) it = m_ImageIndex.erase(it);
            else
                ++it;
        }

        InsertIntoCache(ImageCache.data(), m_InsertIndex, kMaxImageCache, m_InsertIndexBase, Image, Name, Width,
                         Height);

        const ImageKey key {Name, static_cast<int16_t>(Width), static_cast<int16_t>(Height)};
        m_ImageIndex[key] = slot;
    }
}

void cImageCache::InsertIntoCache(ImageData *Cache, std::size_t &InsertIndex, const std::size_t MaxSize,
                                   std::size_t BaseIndex, cImage *Image, const cString &Name, int Width,
                                   int Height) {
    Cache[InsertIndex].Image = std::unique_ptr<cImage>(Image);
    Cache[InsertIndex].Name = Name;
    Cache[InsertIndex].Width = Width;
    Cache[InsertIndex].Height = Height;

    if (++InsertIndex >= MaxSize) {
        isyslog("flatPlus: Cache overflow, increase Cachesize (%ld)", MaxSize);
        isyslog("flatPlus: Refilling cache keeping %ld pre loaded icons", BaseIndex);
        InsertIndex = BaseIndex;  // Keep images loaded at start
    }
}

bool cImageCache::RemoveFromCache(const cString &Name) {
    // Preserve old behavior: remove entries by base filename (ignoring path)
    const std::string_view svName {*Name};

    bool removedAny {false};

    for (std::size_t slot {0}; slot < ImageCache.size(); ++slot) {
        auto &data = ImageCache[slot];
        if (!data.Image) continue;

        std::string_view fullName {*data.Name};
        const std::string_view baseName = BaseNameFromCacheName(fullName);
        if (baseName != svName) continue;

        const ImageKey key {data.Name, data.Width, data.Height};
        m_ImageIndex.erase(key);

        dsyslog("flatPlus: RemoveFromCache: %s", *data.Name);
        data.Image.reset();
        data.Name = "";
        data.Width = -1;
        data.Height = -1;

        removedAny = true;
    }

    return removedAny;
}

// Preload images and icons
// This function is called at startup to load images and icons into the cache
// to speed up the display of the menu and other components.
void cImageCache::PreLoadImage() {
    cTimeMs Timer;

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
