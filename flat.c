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
        *ptr++ = *p++;
        start++;
    }
    *ptr++ = '\0';

    return buf;
}

char *GetFilenameWithoutext(char *fullfilename) {
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
            esyslog("flatPlus: Could not create pixmap of size %i x %i",
                    DrawPort.Size().Width(), DrawPort.Size().Height());
            cRect NewDrawPort = DrawPort;
            int width = std::min(DrawPort.Size().Width(), osd->MaxPixmapSize().Width());
            int height = std::min(DrawPort.Size().Height(), osd->MaxPixmapSize().Height());
            NewDrawPort.SetSize(width, height);
            if (cPixmap *pixmap = osd->CreatePixmap(Layer, ViewPort, NewDrawPort)) {
                esyslog("flatPlus: Created pixmap with reduced size %i x %i", width, height);
                return pixmap;
            } else {
                esyslog("flatPlus: Could not create pixmap with reduced size %i x %i", width, height);
            }
        }
    }
    esyslog("flatPlus: No osd! Could not create pixmap with size %i x %i", width, height);
    return NULL;
}

cPlugin *GetScraperPlugin(void) {
    static cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
    if (!pScraper)  // If it doesn't exit, try tvscraper
        pScraper = cPluginManager::GetPlugin("tvscraper");
    return pScraper;
}

cString GetAspectIcon(int screenWidth, double screenAspect) {
    cString asp("unknown_asp");                     // ???
    if (Config.ChannelSimpleAspectFormat && screenWidth > 720) {
        asp = (screenWidth > 1920) ? "uhd" : "hd";  // UHD or HD
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

cString GetScreenResolutionIcon(int screenWidth, int screenHeight, double screenAspect) {
    cString res("unknown_res");
    switch (screenWidth) {
        case 7680:                        // 7680×4320 (UHD-2 / 8K)
            res = "7680x4320"; break;
        case 3840:                        // 3840×2160 (UHD-1 / 4K)
            res = "3840x2160"; break;
        // case 2560;                        // 2560x1440 (QHD)
        //    res = "2560x1440"; break;      // TODO: Is that used somewhere on sat/cable?
        case 1920:                        // 1920x1080 (HD1080 Full HDTV)
            res = "1920x1080"; break;
        case 1440:                        // 1440x1080 (HD1080 DV)
            res = "1440x1080"; break;
        case 1280:                        // 1280x720 (HD720)
            res = "1280x720"; break;
        case 960:                         // 960x720 (HD720 DV)
            res = "960x720"; break;
        case 720:                         // 720x576 (PAL)
            res = "720x576"; break;
        case 704:                         // 704x576 (PAL)
            res = "704x576"; break;
        case 544:                         // 544x576 (PAL)
            res = "544x576"; break;
        case 528:                         // 528x576 (PAL)
            res = "528x576"; break;
        case 480:                         // 480x576 (PAL SVCD)
            res = "480x576"; break;
        case 352:                         // 352x576 (PAL CVD)
            res = "352x576"; break;
        default:
            dsyslog("flatPlus: Unkown resolution Width: %d Height: %d Aspect: %.2f\n",
                    screenWidth, screenHeight, screenAspect);
            break;
    }
    return res;
}

cString GetFormatIcon(int screenWidth) {
    cString iconName("sd");  // 720 and below is considered sd
    if (screenWidth > 1920)
        iconName = "uhd";
    else if (screenWidth > 720)
        iconName = "hd";

    return iconName;
}

cString GetRecordingerrorIcon(int recInfoErrors) {
    int RecErrIconThreshold = Config.MenuItemRecordingShowRecordingErrorsThreshold;

    cString RecErrorIcon("");
    if (recInfoErrors < 0)  // -1 Untestet recording
        RecErrorIcon = "recording_untested";
    else if (recInfoErrors == 0)  // No errors
        RecErrorIcon = "recording_ok";
    else if (recInfoErrors < RecErrIconThreshold)
        RecErrorIcon = "recording_warning";
    else if (recInfoErrors >= RecErrIconThreshold)
        RecErrorIcon = "recording_error";

    return RecErrorIcon;
}
