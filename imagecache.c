#include "imagecache.h"
#include "config.h"
#include <libgen.h>

#include "displaychannel.h"
#include "displaymenu.h"
#include "displaymessage.h"
#include "displayreplay.h"
#include "displaytracks.h"
#include "displayvolume.h"


cImageCache::cImageCache() {
    Overflow = false;
}

cImageCache::~cImageCache() {

}

void cImageCache::Create(void) {
    for(int i = 0; i < MAX_IMAGE_CACHE; i++) {
        CacheImage[i] = NULL;
        CacheName[i] = "";
        CacheWidth[i] = -1;
        CacheHeight[i] = -1;
    }

    InsertIndex = 0;
}

void cImageCache::Clear(void) {
    for(int i = 0; i < MAX_IMAGE_CACHE; i++) {
        if (CacheImage[i] != NULL)
            delete CacheImage[i];
    }

    InsertIndex = 0;
}

bool cImageCache::RemoveFromCache(std::string Name) {
    bool found = false;
    for(int index = 0; index < MAX_IMAGE_CACHE; index++ ) {
        char *bname;
        bname = basename((char *)CacheName[index].c_str());
        if (!strcmp(bname, Name.c_str())) {
            found = true;
            dsyslog("skin flatPlus: RemoveFromCache - %s", CacheName[index].c_str() );
            CacheImage[index] = NULL;
            CacheName[index] = "";
            CacheWidth[index] = -1;
            CacheHeight[index] = -1;
        }
    }
    return found;
}

cImage* cImageCache::GetImage(std::string Name, int Width, int Height) {
    //dsyslog("imagecache search for image %s Width %d Height %d", Name.c_str(), Width, Height);
    for(int index = 0; index < MAX_IMAGE_CACHE; index++ ) {
        //dsyslog("imagecache index %d image %s Width %d Height %d", index, CacheName[index].c_str(), CacheWidth[index], CacheHeight[index]);
        if (CacheName[index] == Name && CacheWidth[index] == Width && CacheHeight[index] == Height)
            return CacheImage[index];
    }
    return NULL;
}

void cImageCache::InsertImage(cImage *Image, std::string Name, int Width, int Height) {
    CacheImage[InsertIndex] = Image;
    CacheName[InsertIndex] = Name;
    CacheWidth[InsertIndex] = Width;
    CacheHeight[InsertIndex] = Height;

    InsertIndex++;
    if (InsertIndex >= MAX_IMAGE_CACHE) {
        isyslog("skin flatPlus: Imagecache overflow, increase MAX_IMAGE_CACHE");
        InsertIndex = 0;
        Overflow = true;
    }
}

void cImageCache::PreLoadImage(void) {
    uint32_t tick1 = GetMsTicks();

    cFlatDisplayChannel DisplayChannel(false);
    DisplayChannel.PreLoadImages();

    cFlatDisplayMenu DisplayMenu;
    DisplayMenu.PreLoadImages();

    cFlatDisplayReplay DisplayReplay(false);
    DisplayReplay.PreLoadImages();

    cFlatDisplayVolume DisplayVolume;
    DisplayVolume.PreLoadImages();

    uint32_t tick2 = GetMsTicks();
    dsyslog("skin flatPlus: Imagecache pre load images time: %d ms", tick2 - tick1);
    dsyslog("skin flatPlus: Imagecache pre loaded images %d / %d", getCacheCount(), MAX_IMAGE_CACHE);
}
