#include <vdr/osd.h>
#include <vdr/menu.h>

#include "flat.h"

#include "displaychannel.h"
#include "displaymenu.h"
#include "displaymessage.h"
#include "displayreplay.h"
#include "displaytracks.h"
#include "displayvolume.h"

#include "services/epgsearch.h"

#include "services/epgsearch.h"

/* Possible values of the stream content descriptor according to ETSI EN 300 468 */
enum stream_content {
    sc_reserved        = 0x00,
    sc_video_MPEG2     = 0x01,
    sc_audio_MP2       = 0x02,  // MPEG 1 Layer 2 audio
    sc_subtitle        = 0x03,
    sc_audio_AC3       = 0x04,
    sc_video_H264_AVC  = 0x05,
    sc_audio_HEAAC     = 0x06,
    sc_video_H265_HEVC = 0x09,  // stream content 0x09, extension 0x00
    sc_audio_AC4       = 0x19,  // stream content 0x09, extension 0x10
};

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

cPixmap *CreatePixmap(cOsd *osd, cString Name, int Layer, const cRect &ViewPort, const cRect &DrawPort) {
    if (!osd) {
        esyslog("flatPlus: No osd! Could not create pixmap \"%s\" with size %i x %i", *Name, DrawPort.Size().Width(),
                DrawPort.Size().Height());
        return NULL;
    }

    if (cPixmap *pixmap = osd->CreatePixmap(Layer, ViewPort, DrawPort)) {
        // dsyslog("flatPlus: Created pixmap \"%s\" with size %i x %i", *Name, DrawPort.Size().Width(),
        //        DrawPort.Size().Height());
        return pixmap;
    }

    esyslog("flatPlus: Could not create pixmap \"%s\" of size %i x %i", *Name,
            DrawPort.Size().Width(), DrawPort.Size().Height());
    cRect NewDrawPort = DrawPort;
    int width = std::min(DrawPort.Size().Width(), osd->MaxPixmapSize().Width());
    int height = std::min(DrawPort.Size().Height(), osd->MaxPixmapSize().Height());
    NewDrawPort.SetSize(width, height);
    if (cPixmap *pixmap = osd->CreatePixmap(Layer, ViewPort, NewDrawPort)) {
        esyslog("flatPlus: Created pixmap \"%s\" with reduced size %i x %i", *Name, width, height);
        return pixmap;
    }

    esyslog("flatPlus: Could not create pixmap \"%s\" with reduced size %i x %i", *Name, width, height);
    return NULL;
}

// void PixmapFill(cPixmap *pixmap, tColor Color);  // See flat.h

cPlugin *GetScraperPlugin(void) {
    static cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
    if (!pScraper)  // If it doesn't exit, try tvscraper
        pScraper = cPluginManager::GetPlugin("tvscraper");
    return pScraper;
}

cString GetAspectIcon(int screenWidth, double screenAspect) {
    if (Config.ChannelSimpleAspectFormat && screenWidth > 720)
        return (screenWidth > 1920) ? "uhd" : "hd";  // UHD or HD

    if (screenAspect == 16.0/9.0) return "169";
    if (screenAspect == 4.0/3.0) return "43";
    if (screenAspect == 20.0/11.0 || screenAspect == 15.0/11.0) return "169w";
    if (screenAspect == 2.21) return "221";

    return "unknown_asp";
}

cString GetScreenResolutionIcon(int screenWidth, int screenHeight, double screenAspect) {
    cString res("unknown_res");
    switch (screenWidth) {
        case 7680: res = "7680x4320"; break;  // 7680×4320 (UHD-2 / 8K)
        case 3840: res = "3840x2160"; break;  // 3840×2160 (UHD-1 / 4K)
        // case 2560: res = "2560x1440"; break;  // 2560x1440 (QHD) Is that used somewhere on sat/cable?
        case 1920: res = "1920x1080"; break;  // 1920x1080 (HD1080 Full HDTV)
        case 1440: res = "1440x1080"; break;  // 1440x1080 (HD1080 DV)
        case 1280: res = "1280x720"; break;   // 1280x720 (HD720)
        case 960: res = "960x720"; break;     // 960x720 (HD720 DV)
        case 720: res = "720x576"; break;     // 720x576 (PAL)
        case 704: res = "704x576"; break;     // 704x576 (PAL)
        case 544: res = "544x576"; break;     // 544x576 (PAL)
        case 528: res = "528x576"; break;     // 528x576 (PAL)
        case 480: res = "480x576"; break;     // 480x576 (PAL SVCD)
        case 352: res = "352x576"; break;     // 352x576 (PAL CVD)
        default:
            dsyslog("flatPlus: Unkown resolution Width: %d Height: %d Aspect: %.2f\n",
                    screenWidth, screenHeight, screenAspect);
            break;
    }
    return res;
}

cString GetFormatIcon(int screenWidth) {
    if (screenWidth > 1920) return "uhd";
    if (screenWidth > 720) [[likely]] return "hd";

    return "sd";  // 720 and below is considered sd
}

cString GetRecordingerrorIcon(int recInfoErrors) {
    int RecErrIconThreshold = Config.MenuItemRecordingShowRecordingErrorsThreshold;

    if (recInfoErrors < 0) return "recording_untested";  // -1 Untestet recording
    if (recInfoErrors == 0) return "recording_ok";       // No errors
    if (recInfoErrors < RecErrIconThreshold) return "recording_warning";
    if (recInfoErrors >= RecErrIconThreshold) return "recording_error";

    return "";
}

cString GetRecordingseenIcon(int frameTotal, int frameResume) {
    double FrameSeen = frameResume * 1.0 / frameTotal;
    double seenThreshold = Config.MenuItemRecordingSeenThreshold * 100.0;
    // dsyslog("flatPlus: Config.MenuItemRecordingSeenThreshold: %.2f\n", seenThreshold);

    if (FrameSeen >= seenThreshold) return "recording_seen_10";

    if (FrameSeen < 0.1) return "recording_seen_0";
    if (FrameSeen < 0.2) return "recording_seen_1";
    if (FrameSeen < 0.3) return "recording_seen_2";
    if (FrameSeen < 0.4) return "recording_seen_3";
    if (FrameSeen < 0.5) return "recording_seen_4";
    if (FrameSeen < 0.6) return "recording_seen_5";
    if (FrameSeen < 0.7) return "recording_seen_6";
    if (FrameSeen < 0.8) return "recording_seen_7";
    if (FrameSeen < 0.9) return "recording_seen_8";
    if (FrameSeen < 0.98) return "recording_seen_9";

    return "recording_seen_10";
}

void InsertComponents(const cComponents *Components, cString &Text, cString &Audio, cString &Subtitle, bool NewLine) {
    cString audio_type("");
    for (int i {0}; i < Components->NumComponents(); ++i) {
        const tComponent *p = Components->Component(i);
        switch (p->stream) {
        case sc_video_MPEG2:
            if (NewLine) Text.Append("\n");
            if (p->description)
                Text.Append(cString::sprintf("%s: %s (MPEG2)", tr("Video"), p->description));
            else
                Text.Append(cString::sprintf("%s: MPEG2", tr("Video")));
            break;
        case sc_video_H264_AVC:
            if (NewLine) Text.Append("\n");
            if (p->description)
                Text.Append(cString::sprintf("%s: %s (H.264)", tr("Video"), p->description));
            else
                Text.Append(cString::sprintf("%s: H.264", tr("Video")));
            break;
        case sc_video_H265_HEVC:  // Might be not always correct because stream_content_ext (must be 0x0) is
                                  // not available in tComponent
            if (NewLine) Text.Append("\n");
            if (p->description)
                Text.Append(cString::sprintf("%s: %s (H.265)", tr("Video"), p->description));
            else
                Text.Append(cString::sprintf("%s: H.265", tr("Video")));
            break;
        case sc_audio_MP2:
        case sc_audio_AC3:
        case sc_audio_HEAAC:
            if (!isempty(*Audio)) Audio.Append(", ");
            switch (p->stream) {
            case sc_audio_MP2:
                if (p->type == 5)  // Workaround for wrongfully used stream type X 02 05 for AC3
                    audio_type = "AC3";
                else
                    audio_type = "MP2";
                break;
            case sc_audio_AC3: audio_type = "AC3"; break;
            case sc_audio_HEAAC: audio_type = "HEAAC"; break;
            }  // switch p->stream
            if (p->description)
                Audio.Append(cString::sprintf("%s (%s)", p->description, p->language));
            else
                Audio.Append(cString::sprintf("%s (%s)", p->language, *audio_type));
            break;
        case sc_subtitle:
            if (!isempty(*Subtitle)) Subtitle.Append(", ");
            if (p->description)
                Subtitle.Append(cString::sprintf("%s (%s)", p->description, p->language));
            else
                Subtitle.Append(p->language);
            break;
        }  // switch
    }  // for
}

int GetEpgsearchConflichts(void) {
    int numConflicts {0};
    cPlugin *pEpgSearch = cPluginManager::GetPlugin("epgsearch");
    if (pEpgSearch) {
        Epgsearch_lastconflictinfo_v1_0 *serviceData = new Epgsearch_lastconflictinfo_v1_0;
        if (serviceData) {
            serviceData->nextConflict = 0;
            serviceData->relevantConflicts = 0;
            serviceData->totalConflicts = 0;
            pEpgSearch->Service("Epgsearch-lastconflictinfo-v1.0", serviceData);
            if (serviceData->relevantConflicts > 0) {
                numConflicts = serviceData->relevantConflicts;
            }
            delete serviceData;
        }
    }  // pEpgSearch
    return numConflicts;
}
