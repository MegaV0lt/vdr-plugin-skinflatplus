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
      IconCache(kMaxIconCache), m_ReverseImageIndex(kMaxImageCache),  // Reverse index: slot -> ImageKey
      m_ReverseIconIndex(kMaxIconCache) {}

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
    m_PathCache.clear();
    m_PathBaseCache.clear();

    m_ReverseImageIndex.assign(kMaxImageCache, ImageKey {});
    m_ReverseIconIndex.assign(kMaxIconCache, ImageKey {});

    m_InsertIndex = 0;
    m_InsertIconIndex = 0;
    m_PathInsertIndexBase = 0;
}

bool cImageCache::ResolvePath(const std::string &Key, cString &Out) const {
    const auto it {m_PathCache.find(Key)};
    if (it == m_PathCache.end()) return false;

    Out = it->second;
    return true;
}

void cImageCache::CacheResolvedPath(const std::string &Key, const cString &Path) {
    if (m_PathCache.size() >= kMaxPathCache) {
        // Keep the entries computed after preloading (see PreLoadImage) and drop the rest,
        // mirroring how the image cache preserves its preloaded base on overflow.
        dsyslog("flatPlus: cImageCache::CacheResolvedPath() cache full (%ld), keeping %ld pre loaded path entries",
                m_PathCache.size(), m_PathInsertIndexBase);
        m_PathCache = m_PathBaseCache;
    }
    m_PathCache[Key] = Path;
}

static std::string_view BaseNameFromCacheName(std::string_view full) {
    const std::size_t lastSlash {full.find_last_of('/')};
    return (lastSlash != std::string_view::npos) ? full.substr(lastSlash + 1) : full;
}

cImage *cImageCache::GetImage(const cString &Name, int Width, int Height, bool IsIcon) const {
    const ImageKey key {Name, static_cast<int16_t>(Width), static_cast<int16_t>(Height)};

    auto &index = IsIcon ? m_IconIndex : m_ImageIndex;
    const auto it {index.find(key)};
    if (it != index.end()) {
        const std::size_t slot {it->second};
        const auto &cache {IsIcon ? IconCache : ImageCache};
        if (slot < cache.size()) {
            const ImageData &data {cache[slot]};
            if (data.Image && data.Width == Width && data.Height == Height &&
                std::strcmp(*data.Name, *Name) == 0) {
                return data.Image.get();
            }
        }
    }

    // Fallback to linear search if not found in index
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cImageCache::GetImage() Not found in index, performing linear search for %s (%dx%d)", *Name,
            Width, Height);
#endif
    const auto &cache {IsIcon ? IconCache : ImageCache};
    for (std::size_t slot {0}; slot < cache.size(); ++slot) {
        const ImageData &data {cache[slot]};
        if (data.Image && data.Width == Width && data.Height == Height &&
            std::strcmp(*data.Name, *Name) == 0) {
            // Cache the result in the index for future O(1) lookups
#ifdef DEBUGFUNCSCALL
            dsyslog(
                "flatPlus: cImageCache::GetImage() Found in linear search, caching index for %s (%dx%d) at slot %ld",
                *Name, Width, Height, slot);
#endif
            index[key] = slot;
            return data.Image.get();
        }
    }

    return nullptr;
}

void cImageCache::InsertImage(cImage *Image, const cString &Name, int Width, int Height, bool IsIcon) {
    if (!Image) return;

    if (IsIcon) {
        const std::size_t slot {m_InsertIconIndex};

        // Remove any previous mapping that points to the slot we are about to overwrite.
        // O(1) via the reverse index instead of a linear scan of m_IconIndex.
        const ImageKey &oldKey {m_ReverseIconIndex[slot]};
        if (oldKey.Width != -1) { m_IconIndex.erase(oldKey); }

        InsertIntoCache(IconCache.data(), m_InsertIconIndex, kMaxIconCache, m_InsertIconIndexBase, Image, Name,
                         Width, Height);

        const ImageKey key {Name, static_cast<int16_t>(Width), static_cast<int16_t>(Height)};
        m_ReverseIconIndex[slot] = key;
        m_IconIndex[key] = slot;
    } else {
        const std::size_t slot {m_InsertIndex};

        // Remove any previous mapping that points to the slot we are about to overwrite.
        // O(1) via the reverse index instead of a linear scan of m_ImageIndex.
        const ImageKey &oldKey {m_ReverseImageIndex[slot]};
        if (oldKey.Width != -1) { m_ImageIndex.erase(oldKey); }

        InsertIntoCache(ImageCache.data(), m_InsertIndex, kMaxImageCache, m_InsertIndexBase, Image, Name, Width,
                         Height);

        const ImageKey key {Name, static_cast<int16_t>(Width), static_cast<int16_t>(Height)};
        m_ReverseImageIndex[slot] = key;
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
        isyslog("flatPlus: Refilling cache keeping %ld pre loaded images", BaseIndex);
        InsertIndex = BaseIndex;  // Keep pre-loaded images. Keep old images until they are overwritten by new ones.
    }
}

// Remove an image from the imagecache by its base filename (ignoring path). Used for removing a channel logo.
bool cImageCache::RemoveFromCache(const cString &Name) {
    // Preserve old behavior: remove entries by base filename (ignoring path)
    const std::string_view svName {*Name};
    bool removedAny {false};

    for (std::size_t slot {0}; slot < ImageCache.size(); ++slot) {
        auto &data {ImageCache[slot]};
        if (!data.Image) continue;

        std::string_view fullName {*data.Name};
        const std::string_view baseName {BaseNameFromCacheName(fullName)};
        if (baseName != svName) continue;

        const ImageKey key {data.Name, data.Width, data.Height};
        m_ImageIndex.erase(key);
        m_ReverseImageIndex[slot] = ImageKey {};

        dsyslog("flatPlus: RemoveFromCache: %s", *data.Name);
        data.Image.reset();
        data.Name = "";
        data.Width = -1;
        data.Height = -1;

        removedAny = true;
    }

    // Invalidate logo path resolution entries so a re-added logo is re-probed on disk instead of being served from a
    // stale (possibly negative) cache entry. This is intentionally broad (all 'L' entries): RLFC is a rare admin
    // command and may be used precisely to force a refresh when a logo file was added/replaced on disk.
    for (auto it {m_PathCache.begin()}; it != m_PathCache.end();) {
        if (!it->first.empty() && it->first.front() == 'L')
            it = m_PathCache.erase(it);
        else
            ++it;
    }

    return removedAny;
}

// Preload images and icons
// This function is called at startup to load images and icons into the cache to speed up the display of the menu and
// other components.
void cImageCache::PreLoadImage() {
    cTimeMs Timer;

    // Only one OSD may be open at a time, so scope each object to close its OSD before the next one is called.
    // DisplayChannel.PreLoadImages() is also used to determine if 'logo_background' should be loaded from logo or theme
    // path
    { cFlatDisplayChannel DisplayChannel(false); DisplayChannel.PreLoadImages(); }
    { cFlatDisplayMenu Display_Menu; Display_Menu.PreLoadImages(); }
    { cFlatDisplayReplay DisplayReplay(false); DisplayReplay.PreLoadImages(); }
    { cFlatDisplayVolume DisplayVolume; DisplayVolume.PreLoadImages(); }

    m_InsertIndexBase = GetCacheCount();
    m_InsertIconIndexBase = GetCacheCount(true);

    // Snapshot the path resolution cache so preloaded entries are preserved on overflow.
    m_PathBaseCache = m_PathCache;
    m_PathInsertIndexBase = m_PathCache.size();

    dsyslog("flatPlus: Imagecache pre loaded %ld images and %ld icons", m_InsertIndexBase, m_InsertIconIndexBase);
    dsyslog("flatPlus: Imagecache pre loaded %ld path cache entries", m_PathInsertIndexBase);
    dsyslog("flatPlus: Imagecache pre load images and icons time: %ld ms", Timer.Elapsed());
}
