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

char *substr(char *string, int start, int end) {
    char *p = &string[start];
    char *buf = (char*) malloc(strlen(p) + 1);
    char *ptr = buf;
    if (!buf) return NULL;

    while (*p != '\0' && start < end) {
        *ptr ++ = *p++;
        start ++;
    }
    *ptr++ = '\0';

    return buf;
}

char *GetFilenameWithoutext(char * fullfilename) {
    int i = 0, size = 0;

    while (fullfilename[i] != '\0') {
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
            esyslog("flatPlus: Could not create pixmap of size %i x %i", DrawPort.Size().Width(), DrawPort.Size().Height());
            cRect NewDrawPort = DrawPort;
            int width = std::min(DrawPort.Size().Width(), osd->MaxPixmapSize().Width());
            int height = std::min(DrawPort.Size().Height(), osd->MaxPixmapSize().Height());
            NewDrawPort.SetSize(width, height);
            if (cPixmap *pixmap = osd->CreatePixmap(Layer, ViewPort, NewDrawPort)) {
                esyslog("flatPlus: Create pixmap with reduced size %i x %i", width, height);
                return pixmap;
            } else {
                esyslog("flatPlus: Could not create pixmap with reduced size %i x %i", width, height);
            }
        }
    }
    return NULL;
}

cPlugin *GetScraperPlugin(void) {
    static cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
    if (!pScraper)  // If it doesn't exit, try tvscraper
        pScraper = cPluginManager::GetPlugin("tvscraper");
    return pScraper;
}

cString GetSimpleAspectIcon(void) {
    cString asp("unknown_asp");                // ???
    if (Config.ChannelSimpleAspectFormat && screenWidth > 720) {
        switch (screenWidth) {                 // No aspect for HD
            case 7680:
            case 3840:
                asp = "uhd"; break;
            default:
                asp = "hd"; break;
        }
    } else {
        if (screenAspect == 4.0/3.0)
            asp = "43";
        else if (screenAspect == 16.0/9.0)
            asp = "169";
        else if (screenAspect == 20.0/11.0 || screenAspect == 15.0/11.0)
            asp = "169w";
        else if (screenAspect == 2.21)
            asp = "221";
    }
    return asp;
}

cString GetScreenResolutionIcon(void) {
    cString res("");
    switch (screenWidth) {
        case 7680:                        // 7680×4320 (UHD-2 / 8K)
            res = "7680x4320"; break;
        case 3840:                        // 3840×2160 (UHD-1 / 4K)
            res = "3840x2160"; break;
        case 1920:                        // 1920x1080 (HD1080 Full HDTV)
            res = "1920x1080"; break;
        case 1440:                        // 1440x1080 (HD1080 DV)
            res = "1440x1080"; break;
        case 1280:                        // 1280x720 (HD720)
            res = "1280x720"; break;
        case 960:                         // 960x720 (HD720 DV)
            res = "960x720"; break;
        case 704:                         // 704x576 (PAL)
            res = "704x576"; break;
        case 720:                         // 720x576 (PAL)
            res = "720x576"; break;
        case 544:                         // 544x576 (PAL)
            res = "544x576"; break;
        case 528:                         // 528x576 (PAL)
            res = "528x576"; break;
        case 480:                         // 480x576 (PAL SVCD)
            res = "480x576"; break;
        case 352:                         // 352x576 (PAL CVD)
            res = "352x576"; break;
        default:
            res = "unknown_res";
            dsyslog("flatPlus: Unkown resolution Width: %d Height: %d Aspect: %.2f\n", 
                    screenWidth, screenHeight, screenAspect);
            break;
    }
    return res;
}

cString GetFormatIcon(void) {
    cString iconName(""); // Show Format
    switch (screenWidth) {
    case 7680:
    case 3840:
        iconName = "uhd";
        break;
    case 1920:
    case 1440:
    case 1280:
        iconName = "hd";
        break;
    case 720:
        iconName = "sd";
        break;
    default:
        iconName = "sd";
        break;
    }
    return iconName;
}
