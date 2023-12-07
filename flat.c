#include <vdr/osd.h>
#include <vdr/menu.h>
// #include <memory>

#include "./flat.h"

#include "./displaychannel.h"
#include "./displaymenu.h"
#include "./displaymessage.h"
#include "./displayreplay.h"
#include "./displaytracks.h"
#include "./displayvolume.h"


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
static bool g_MenuActive = false;
bool g_FirstDisplay = true;
time_t g_RemoteTimersLastRefresh = 0;

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
    g_MenuActive = true;
    return menu;
}
/*
// shared_ptr ist der bequemere Smart Pointer, vllt ist std::unique_ptr die bessere Alternative
std::shared_ptr<cSkinDisplayChannel> cFlat::DisplayChannel(bool WithInfo) {
    return std::make_shared<cFlatDisplayChannel>(WithInfo);
}

std::shared_ptr<cOsdItem> cMenuSetupSubMenu::InfoItem(const char *label, const char *value) {
    // was ist wenn, min. einer der beiden Parameter nullptr ist?
    std::shared_ptr<cOsdItem> retval = std::make_shared<cOsdItem>(cString::sprintf("%s: %s", label, value));
    retval->SetSelectable(false);
    g_MenuActive = true;
    displayMenu = retval;
    return retval;
}
*/
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

// void inline PixmapFill(cPixmap *pixmap, tColor Color);  // See flat.h

cPlugin *GetScraperPlugin(void) {
    static cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
    if (!pScraper)  // If it doesn't exit, try tvscraper
        pScraper = cPluginManager::GetPlugin("tvscraper");
    return pScraper;
}

cString GetAspectIcon(int ScreenWidth, double ScreenAspect) {
    if (Config.ChannelSimpleAspectFormat && ScreenWidth > 720)
        return (ScreenWidth > 1920) ? "uhd" : "hd";  // UHD or HD

    if (ScreenAspect == 16.0/9.0) return "169";
    if (ScreenAspect == 4.0/3.0) return "43";
    if (ScreenAspect == 20.0/11.0 || ScreenAspect == 15.0/11.0) return "169w";
    if (ScreenAspect == 2.21) return "221";

    return "unknown_asp";
}

cString GetScreenResolutionIcon(int ScreenWidth, int ScreenHeight, double ScreenAspect) {
    cString res("unknown_res");
    switch (ScreenWidth) {
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
                    ScreenWidth, ScreenHeight, ScreenAspect);
            break;
    }
    return res;
}

cString GetFormatIcon(int ScreenWidth) {
    if (ScreenWidth > 1920) return "uhd";
    if (ScreenWidth > 720) [[likely]] return "hd";

    return "sd";  // 720 and below is considered sd
}

cString GetRecordingerrorIcon(int RecInfoErrors) {
    int RecErrIconThreshold = Config.MenuItemRecordingShowRecordingErrorsThreshold;

    if (RecInfoErrors < 0) return "recording_untested";  // -1 Untestet recording
    if (RecInfoErrors == 0) return "recording_ok";       // No errors
    if (RecInfoErrors < RecErrIconThreshold) return "recording_warning";
    if (RecInfoErrors >= RecErrIconThreshold) return "recording_error";

    return "";
}

cString GetRecordingseenIcon(int FrameTotal, int FrameResume) {
    double FrameSeen = FrameResume * 1.0 / FrameTotal;
    double SeenThreshold = Config.MenuItemRecordingSeenThreshold * 100.0;
    // dsyslog("flatPlus: Config.MenuItemRecordingSeenThreshold: %.2f\n", SeenThreshold);

    if (FrameSeen >= SeenThreshold) return "recording_seen_10";

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

void InsertComponents(const cComponents *Components, cString &Text, cString &Audio, cString &Subtitle, bool NewLine) {  // NOLINT
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
                // Workaround for wrongfully used stream type X 02 05 for AC3
                audio_type = (p->type == 5) ? "AC3" : "MP2";
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
    int NumConflicts {0};
    cPlugin *pEpgSearch = cPluginManager::GetPlugin("epgsearch");
    if (pEpgSearch) {
        Epgsearch_lastconflictinfo_v1_0 ServiceData;
        ServiceData.nextConflict = 0;
        ServiceData.relevantConflicts = 0;
        ServiceData.totalConflicts = 0;
        pEpgSearch->Service("Epgsearch-lastconflictinfo-v1.0", &ServiceData);
        if (ServiceData.relevantConflicts > 0) {
            NumConflicts = ServiceData.relevantConflicts;
        }
    }  // pEpgSearch
    return NumConflicts;
}

bool GetCuttedLengthMarks(const cRecording *Recording, cString &Text, cString &Cutted, bool AddText) {  // NOLINT
    cMarks marks;
    // From skinElchiHD - Avoid triggering index generation for recordings with empty/missing index
    bool IsCutted = false, HasMarks = false;
    cIndexFile *index = NULL;
    if (Recording->NumFrames() > 0) {
        HasMarks = marks.Load(Recording->FileName(), Recording->FramesPerSecond(), Recording->IsPesRecording()) &&
                   marks.Count();
        index = new cIndexFile(Recording->FileName(), false, Recording->IsPesRecording());
        // cIndexFile *index(Recording->FileName(), false, Recording->IsPesRecording());
    }

    // For AddText
    int LastIndex {0};
    uint64_t RecSize {0};

    int CuttedLength {0};
    int32_t CutinFrame {0};
    uint64_t RecSizeCutted {0}, CutinOffset {0};
    uint64_t FileSize[100000];
    uint16_t MaxFiles = (Recording->IsPesRecording()) ? 999 : 65535;
    FileSize[0] = 0;

    int i {0}, rc {0};
    struct stat filebuf;
    cString FileName("");

    do {
        ++i;
        if (Recording->IsPesRecording())
            FileName = cString::sprintf("%s/%03d.vdr", Recording->FileName(), i);
        else
            FileName = cString::sprintf("%s/%05d.ts", Recording->FileName(), i);
        rc = stat(*FileName, &filebuf);
        if (rc == 0)
            FileSize[i] = FileSize[i - 1] + filebuf.st_size;
        else {
            if (ENOENT != errno) {
                esyslog("flatPlus: Error determining file size of \"%s\" %d (%s)", (const char *)FileName, errno,
                        strerror(errno));
                if (AddText) RecSize = 0;
            }
        }
    } while (i <= MaxFiles && !rc);
    if (AddText) RecSize = FileSize[i - 1];

    if (HasMarks && index) {
        uint16_t FileNumber;
        off_t FileOffset;

        bool cutin = true;
        int32_t position {0};
        cMark *mark = marks.First();
        while (mark) {
            position = mark->Position();
            index->Get(position, &FileNumber, &FileOffset);
            if (cutin) {
                CutinFrame = position;
                cutin = false;
                CutinOffset = FileSize[FileNumber - 1] + FileOffset;
            } else {
                CuttedLength += position - CutinFrame;
                cutin = true;
                RecSizeCutted += FileSize[FileNumber - 1] + FileOffset - CutinOffset;
            }
            cMark *NextMark = marks.Next(mark);
            mark = NextMark;
        }
        if (!cutin) {
            CuttedLength += index->Last() - CutinFrame;
            index->Get(index->Last() - 1, &FileNumber, &FileOffset);
            RecSizeCutted += FileSize[FileNumber - 1] + FileOffset - CutinOffset;
        }
    }
    if (index) {
        if (AddText) {
            LastIndex = index->Last();
            Text.Append(
                cString::sprintf("%s: %s", tr("Length"), *IndexToHMSF(LastIndex, false, Recording->FramesPerSecond())));
            if (HasMarks)
                Text.Append(cString::sprintf(" (%s: %s)", tr("cutted"),
                                             *IndexToHMSF(CuttedLength, false, Recording->FramesPerSecond())));
            Text.Append("\n");
        } else if (HasMarks) {
            Cutted = IndexToHMSF(CuttedLength, false, Recording->FramesPerSecond());
            IsCutted = true;
        }
    }
    delete index;

    if (AddText) {
        if (RecSize > MEGABYTE(1023))
            Text.Append(cString::sprintf("%s: %.2f GB", tr("Size"), static_cast<float>(RecSize) / MEGABYTE(1024)));
        else
            Text.Append(cString::sprintf("%s: %lld MB", tr("Size"), RecSize / MEGABYTE(1)));

        if (HasMarks) {
            if (RecSize > MEGABYTE(1023))
                Text.Append(
                    cString::sprintf(" (%s: %.2f GB)", tr("cutted"), static_cast<float>(RecSizeCutted) / MEGABYTE(1024)));
            else
                Text.Append(cString::sprintf(" (%s: %lld MB)", tr("cutted"), RecSizeCutted / MEGABYTE(1)));
        }
        Text.Append(cString::sprintf("\n%s: %d, %s: %d\n", trVDR("Priority"), Recording->Priority(), trVDR("Lifetime"),
                                     Recording->Lifetime()));

        if (LastIndex) {
            Text.Append(cString::sprintf("%s: %s, %s: ~%.2f MBit/s (Video + Audio)", tr("format"),
                                         (Recording->IsPesRecording() ? "PES" : "TS"), tr("bit rate"),
                                         static_cast<float>(RecSize) / LastIndex * Recording->FramesPerSecond() * 8 /
                                             MEGABYTE(1)));
        }
    }  // AddText
    return IsCutted;
}
