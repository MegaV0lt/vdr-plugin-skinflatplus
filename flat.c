#include <vdr/osd.h>
#include <vdr/menu.h>

#include "flat.h"

#include "displaychannel.h"
#include "displaymenu.h"
#include "displaymessage.h"
#include "displayreplay.h"
#include "displaytracks.h"
#include "displayvolume.h"

class cFlatConfig Config;
class cImageCache imgCache;

cTheme Theme;
static bool menuActive = false;
bool firstDisplay = true;
time_t remoteTimersLastRefresh = 0;

cFlat::cFlat(void) : cSkin("flatPlus", &::Theme) {
    displayMenu = NULL;
}

const char *cFlat::Description(void) {
    return "flatPlus";
}

cSkinDisplayChannel *cFlat::DisplayChannel(bool WithInfo) {
    return new cFlatDisplayChannel(WithInfo);
}

cSkinDisplayMenu *cFlat::DisplayMenu(void) {
    cFlatDisplayMenu *menu = new cFlatDisplayMenu;
    displayMenu = menu;
    menuActive = true;
    return menu;
}

cSkinDisplayReplay *cFlat::DisplayReplay(bool ModeOnly) {
    return new cFlatDisplayReplay(ModeOnly);
}

cSkinDisplayVolume *cFlat::DisplayVolume(void) {
    return new cFlatDisplayVolume;
}

cSkinDisplayTracks *cFlat::DisplayTracks(const char *Title, int NumTracks, const char * const *Tracks) {
    return new cFlatDisplayTracks(Title, NumTracks, Tracks);
}

cSkinDisplayMessage *cFlat::DisplayMessage(void) {
    return new cFlatDisplayMessage;
}


char * substr(char * string, int start, int end)
{
    char * p = &string[start];
    char * buf = (char*) malloc(strlen(p) + 1);
    char * ptr = buf;
    if (!buf) return NULL;

    while(*p != '\0' && start < end) {
        *ptr ++ = *p++;
        start ++;
    }
    *ptr++ = '\0';

    return buf;
}

char *GetFilenameWithoutext(char * fullfilename)
{
    int i = 0, size = 0;

    while(fullfilename[i] != '\0') {
        if (fullfilename[i] == '.') {
            size = i;
        }
        i++;
    }
    return substr(fullfilename, 0, size);
}

cPixmap *CreatePixmap(cOsd *osd, int Layer, const cRect &ViewPort, const cRect &DrawPort) {
      if (osd) {
        if (cPixmap *pixmap = osd->CreatePixmap(Layer, ViewPort, DrawPort)) {
            return pixmap;
        } else {
            esyslog("skin flatPlus: Could not create pixmap of size %i x %i", DrawPort.Size().Width(), DrawPort.Size().Height());
            cRect NewDrawPort = DrawPort;
            int width = std::min(DrawPort.Size().Width(), osd->MaxPixmapSize().Width());
            int height = std::min(DrawPort.Size().Height(), osd->MaxPixmapSize().Height());
            NewDrawPort.SetSize(width, height);
            if (cPixmap *pixmap = osd->CreatePixmap(Layer, ViewPort, NewDrawPort)) {
                esyslog("skin flatPlus: Create pixmap with reduced size %i x %i", width, height);
                return pixmap;
            } else {
                esyslog("skin flatPlus: Could not create pixmap with reduced size %i x %i", width, height);
            }
        }
    }
    return NULL;
}

cPlugin *GetScraperPlugin(void) {
    static cPlugin *pScraper = GetScraperPlugin();
    if( !pScraper )  // If it doesn't exit, try tvscraper
        pScraper = cPluginManager::GetPlugin("tvscraper");
    return pScraper;
}
