#include "./imagecache.h"
#include <libgen.h>

#include "./config.h"
#include "./displaychannel.h"
#include "./displaymenu.h"
#include "./displaymessage.h"
#include "./displayreplay.h"
#include "./displaytracks.h"
#include "./displayvolume.h"


cImageCache::cImageCache() {
    m_OverFlow = false;
}

cImageCache::~cImageCache() {
}

void cImageCache::Create(void) {
    for (int i {0}; i < MAX_IMAGE_CACHE; ++i) {
        CacheImage[i] = NULL;
        CacheName[i] = "";
        CacheWidth[i] = -1;
        CacheHeight[i] = -1;
    }

    m_InsertIndex = 0;
}

void cImageCache::Clear(void) {
    for (int i {0}; i < MAX_IMAGE_CACHE; ++i) {
        if (CacheImage[i] != NULL)
            delete CacheImage[i];
    }

    m_InsertIndex = 0;
}

bool cImageCache::RemoveFromCache(std::string Name) {
    // bool found = false;
    // char *bname;
    std::string BaseFileName("");
    for (int i {0}; i < MAX_IMAGE_CACHE; ++i) {
        // bname = basename((char *)CacheName[i].c_str());  // TODO: Improve
        // imagecache.c:45:26: error: ‘reinterpret_cast’ from type ‘const char*’ to type ‘char*’ casts away qualifiers
        //   45 |         bname = basename(reinterpret_cast<char *>(CacheName[i].c_str()));
        BaseFileName = CacheName[i].substr(CacheName[i].find_last_of("/") + 1);  // Part after the last '/'

        // if (!strcmp(bname, Name.c_str())) {
        if (BaseFileName == Name) {
            dsyslog("flatPlus: RemoveFromCache - %s", CacheName[i].c_str());
            CacheImage[i] = NULL;
            CacheName[i] = "";
            CacheWidth[i] = -1;
            CacheHeight[i] = -1;
            return true;
        }
    }
    return false;
}

cImage* cImageCache::GetImage(std::string Name, int Width, int Height) {
    // dsyslog("flatPlus: Imagecache search for image %s Width %d Height %d", Name.c_str(), Width, Height);
    for (int i {0}; i < MAX_IMAGE_CACHE; ++i) {
        // dsyslog("flatPlus: Imagecache index %d image %s Width %d Height %d", index, CacheName[i].c_str(),
        //          CacheWidth[i], CacheHeight[i]);
        if (CacheName[i] == Name && CacheWidth[i] == Width && CacheHeight[i] == Height)
            return CacheImage[i];
    }
    return NULL;
}

void cImageCache::InsertImage(cImage *Image, std::string Name, int Width, int Height) {
    // if (m_OverFlow) return;  // TODO; Leave cache as is or refill?

    CacheImage[m_InsertIndex] = Image;
    CacheName[m_InsertIndex] = Name;
    CacheWidth[m_InsertIndex] = Width;
    CacheHeight[m_InsertIndex] = Height;

    ++m_InsertIndex;
    if (m_InsertIndex >= MAX_IMAGE_CACHE) {
        isyslog("flatPlus: Imagecache overflow, increase MAX_IMAGE_CACHE");
        m_InsertIndex = 0;
        m_OverFlow = true;
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
    dsyslog("flatPlus: Imagecache pre load images time: %d ms", tick2 - tick1);
    dsyslog("flatPlus: Imagecache pre loaded images %d / %d", GetCacheCount(), MAX_IMAGE_CACHE);
}
