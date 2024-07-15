/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include <vdr/osd.h>
#include <vdr/menu.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "./flat.h"

#include "./displaychannel.h"
#include "./displaymenu.h"
#include "./displaymessage.h"
#include "./displayreplay.h"
#include "./displaytracks.h"
#include "./displayvolume.h"

#include "./services/epgsearch.h"

/* Possible values of the stream content descriptor according to ETSI EN 300 468 */
enum stream_content {
    sc_reserved        = 0x00,
    sc_video_MPEG2     = 0x01,
    sc_audio_MP2       = 0x02,  // MPEG 1 Layer 2 audio
    sc_subtitle        = 0x03,
    sc_audio_AC3       = 0x04,
    sc_video_H264_AVC  = 0x05,
    sc_audio_HEAAC     = 0x06,
    sc_video_H265_HEVC = 0x09,  // Stream content 0x09, extension 0x00
    sc_audio_AC4       = 0x19,  // Stream content 0x09, extension 0x10
};

class cFlatConfig Config;
class cImageCache ImgCache;

cTheme Theme;
static bool m_MenuActive {false};
time_t m_RemoteTimersLastRefresh = 0;

cFlat::cFlat(void) : cSkin("flatPlus", &::Theme) {
    Display_Menu = NULL;
}

const char *cFlat::Description(void) {
    return "flatPlus";
}

cSkinDisplayChannel *cFlat::DisplayChannel(bool WithInfo) {
    return new cFlatDisplayChannel(WithInfo);
}

cSkinDisplayMenu *cFlat::DisplayMenu(void) {
    cFlatDisplayMenu *menu = new cFlatDisplayMenu;
    Display_Menu = menu;
    m_MenuActive = true;
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
    /* if (!osd) {
        esyslog("flatPlus: No osd! Could not create pixmap \"%s\" with size %i x %i", *Name, DrawPort.Size().Width(),
                DrawPort.Size().Height());
        return NULL;
    } */

    if (cPixmap *pixmap = osd->CreatePixmap(Layer, ViewPort, DrawPort)) {
        // dsyslog("flatPlus: Created pixmap \"%s\" with size %i x %i", *Name, DrawPort.Size().Width(),
        //        DrawPort.Size().Height());
        return pixmap;
    }  // Everything runs according to the plan

    esyslog("flatPlus: Could not create pixmap \"%s\" of size %i x %i", *Name,
            DrawPort.Size().Width(), DrawPort.Size().Height());
    cRect NewDrawPort = DrawPort;
    const int width = std::min(DrawPort.Size().Width(), osd->MaxPixmapSize().Width());
    const int height = std::min(DrawPort.Size().Height(), osd->MaxPixmapSize().Height());
    NewDrawPort.SetSize(width, height);
    if (cPixmap *pixmap = osd->CreatePixmap(Layer, ViewPort, NewDrawPort)) {
        esyslog("flatPlus: Created pixmap \"%s\" with reduced size %i x %i", *Name, width, height);
        return pixmap;
    }

    esyslog("flatPlus: Could not create pixmap \"%s\" with reduced size %i x %i", *Name, width, height);
    return NULL;
}

// void inline PixmapFill(cPixmap *Pixmap, tColor Color);  //* See flat.h

cPlugin *GetScraperPlugin(void) {
    static cPlugin *pScraper = cPluginManager::GetPlugin("tvscraper");
    if (!pScraper)  // If it doesn't exit, try scraper2vdr
        pScraper = cPluginManager::GetPlugin("scraper2vdr");
    return pScraper;
}

cString GetAspectIcon(int ScreenWidth, double ScreenAspect) {
    if (Config.ChannelSimpleAspectFormat && ScreenWidth > 720)
        return (ScreenWidth > 1920) ? "uhd" : "hd";  // UHD or HD

    if (ScreenAspect == 16.0 / 9.0) return "169";
    if (ScreenAspect == 4.0 / 3.0) return "43";
    if (ScreenAspect == 20.0 / 11.0 || ScreenAspect == 15.0 / 11.0) return "169w";
    if (ScreenAspect == 2.21) return "221";

    dsyslog("flatPlus: Unknown screen aspect %.2f", ScreenAspect);
    return "unknown_asp";
}

cString GetScreenResolutionIcon(int ScreenWidth, int ScreenHeight) {
    cString res("unknown_res");
    switch (ScreenWidth) {
        case 7680: res = "7680x4320"; break;  // 7680×4320 (UHD-2 / 8K)
        case 3840: res = "3840x2160"; break;  // 3840×2160 (UHD-1 / 4K)
        // case 2560: res = "2560x1440"; break;  //* 2560x1440 (QHD) Is that used somewhere on sat/cable?
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
            dsyslog("flatPlus: Unkown screen resolution: %d x %d", ScreenWidth, ScreenHeight);
            break;
    }
    return res;
}

cString GetFormatIcon(int ScreenWidth) {
    if (ScreenWidth > 1920) return "uhd";
    if (ScreenWidth > 720) [[likely]] return "hd";

    return "sd";  // 720 and below is considered sd
}

cString GetRecordingFormatIcon(const cRecording *Recording) {
    // From skin ElchiHD
    #if APIVERSNUM >= 20605
        const uint16_t FrameWidth = Recording->Info()->FrameWidth();
        if (FrameWidth > 0) {
            if (FrameWidth > 1920) return "uhd";  // TODO: Separate images
            if (FrameWidth > 720) return "hd";
            return "sd";  // 720 and below is considered sd
        }
        else  // NOLINT
    #endif
        {   // Find radio and H.264/H.265 streams.
            //! Detection FAILED for RTL, SAT1 etc. They do not send a video component :-(
            if (Recording->Info()->Components()) {
                const cComponents *Components = Recording->Info()->Components();
                int i {-1}, NumComponents = Components->NumComponents();
                while (++i < NumComponents) {
                    const tComponent *p = Components->Component(i);
                    switch (p->stream) {
                        case sc_video_MPEG2:     return "sd";
                        case sc_video_H264_AVC:  return "hd";
                        case sc_video_H265_HEVC: return "uhd";
                        default:                 break;
                    }
                }
            }
        }
    return "";  // Nothing found
}

cString GetRecordingerrorIcon(int RecInfoErrors) {
    if (RecInfoErrors == 0) return "recording_ok";       // No errors
    if (RecInfoErrors < 0) return "recording_untested";  // -1 Untested recording

    const int RecErrIconThreshold = Config.MenuItemRecordingShowRecordingErrorsThreshold;
    if (RecInfoErrors < RecErrIconThreshold) return "recording_warning";
    if (RecInfoErrors >= RecErrIconThreshold) return "recording_error";

    return "";
}

cString GetRecordingseenIcon(int FrameTotal, int FrameResume) {
    const double FrameSeen = FrameResume * 1.0 / FrameTotal;
    const double SeenThreshold = Config.MenuItemRecordingSeenThreshold * 100.0;
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

void SetMediaSize(cSize &MediaSize, const cSize &ContentSize) {  // NOLINT
    const uint Aspect = MediaSize.Width() / MediaSize.Height();  // <1 = Poster, >1 = Portrait, >5 = Banner
    //* Aspect of image is preserved in LoadFile()
    if (Aspect < 1) {                                     //* Poster (For example 680x1000 = 0.68)
        MediaSize.SetHeight(ContentSize.Height() * 0.7);  // Max 70% of pixmap height
        // dsyslog("flatPlus: New poster max size %d x %d", MediaSize.Width(), MediaSize.Height());
    } else if (Aspect < 4) {                              //* Portrait (For example 1920x1080 = 1.77)
        MediaSize.SetWidth(ContentSize.Width() / 3);      // Max 1/3 of pixmap width
        // dsyslog("flatPlus: New portrait max size %d x %d", MediaSize.Width(), MediaSize.Height());
    } else {                                              //* Banner (Usually 758x140 = 5.41)
        MediaSize.SetWidth(ContentSize.Width() * (1.0 / (1920.0 / 758)));  // To get 758 width @ 1920
        // dsyslog("flatPlus: New banner max size %d x %d", MediaSize.Width(), MediaSize.Height());
    }
}

void InsertComponents(const cComponents *Components, cString &Text, cString &Audio, cString &Subtitle,  // NOLINT
                      bool NewLine) {
    cString AudioType {""};
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
                AudioType = (p->type == 5) ? "AC3" : "MP2";
                break;
            case sc_audio_AC3: AudioType = "AC3"; break;
            case sc_audio_HEAAC: AudioType = "HEAAC"; break;
            }  // switch p->stream
            if (p->description)
                Audio.Append(cString::sprintf("%s (%s)", p->description, p->language));
            else
                Audio.Append(cString::sprintf("%s (%s)", p->language, *AudioType));
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

void InsertAuxInfos(const cRecordingInfo *RecInfo, cString &Text, bool InfoLine) {  // NOLINT
    #ifdef DEBUGFUNCSCALL
        dsyslog("flatPlus: cFlat::InsertAuxInfo()");
    #endif

    std::string Buffer {XmlSubstring(RecInfo->Aux(), "<epgsearch>", "</epgsearch>")};
    std::string Channel {""}, Searchtimer {""};
    Channel.reserve(32);
    Searchtimer.reserve(32);
    if (!Buffer.empty()) {
        Channel = XmlSubstring(Buffer, "<channel>", "</channel>");
        Searchtimer = XmlSubstring(Buffer, "<searchtimer>", "</searchtimer>");
        if (Searchtimer.empty())
            Searchtimer = XmlSubstring(Buffer, "<Search timer>", "</Search timer>");
    }

    Buffer = XmlSubstring(RecInfo->Aux(), "<tvscraper>", "</tvscraper>");
    std::string Causedby {""}, Reason {""};
    Causedby.reserve(32);
    Reason.reserve(32);
    if (!Buffer.empty()) {
        Causedby = XmlSubstring(Buffer, "<causedBy>", "</causedBy>");
        Reason = XmlSubstring(Buffer, "<reason>", "</reason>");
    }

    Buffer = XmlSubstring(RecInfo->Aux(), "<vdradmin-am>", "</vdradmin-am>");
    std::string Pattern {""};
    Pattern.reserve(32);
    if (!Buffer.empty()) {
        Pattern = XmlSubstring(Buffer, "<pattern>", "</pattern>");
    }

    if (InfoLine) {
        if ((!Channel.empty() && !Searchtimer.empty()) || (!Causedby.empty() && !Reason.empty()) ||
             !Pattern.empty())
            Text.Append(cString::sprintf("\n\n%s:", tr("additional information")));  // Show info line
    }

    if (!Channel.empty() && !Searchtimer.empty()) {  // EpgSearch
        Text.Append(cString::sprintf("\nEPGsearch: %s: %s, %s: %s", tr("channel"), Channel.c_str(),
                                         tr("search pattern"), Searchtimer.c_str()));
    }

    if (!Causedby.empty() && !Reason.empty()) {  // TVScraper
        Text.Append(cString::sprintf("\nTVScraper: %s: %s, %s: ", tr("caused by"), Causedby.c_str(), tr("reason")));
        if (Reason == "improve")
            Text.Append(tr("improve"));
        else if (Reason == "collection")
            Text.Append(tr("collection"));
        else if (Reason == "TV show, missing episode")
            Text.Append(tr("TV show, missing episode"));
        else
            Text.Append(Reason.c_str());  // To be safe if there are more options
    }

    if (!Pattern.empty())  // VDRAdmin
        Text.Append(cString::sprintf("\nVDRadmin-AM: %s: %s", tr("search pattern"), Pattern.c_str()));
}

int GetEpgsearchConflicts(void) {
    cPlugin *pEpgSearch = cPluginManager::GetPlugin("epgsearch");
    if (pEpgSearch) {
        Epgsearch_lastconflictinfo_v1_0 ServiceData {.nextConflict = 0, .relevantConflicts = 0, .totalConflicts = 0};
        pEpgSearch->Service("Epgsearch-lastconflictinfo-v1.0", &ServiceData);
        if (ServiceData.relevantConflicts > 0)
            return ServiceData.relevantConflicts;
    }  // pEpgSearch
    return 0;
}

int GetFrameAfterEdit(const cMarks *marks, int Frame, int LastFrame) {  // From SkinLCARSNG
    if (LastFrame < 0 || Frame < 0 || Frame > LastFrame) return -1;

    int EditedFrame {0};
    int p {0}, PrevPos {-1};
    bool InEdit {false};
    for (const cMark *mi = marks->First(); mi; mi = marks->Next(mi)) {
        p = mi->Position();
        if (InEdit) {
            EditedFrame += p - PrevPos;
            InEdit = false;
            if (Frame <= p) {
                EditedFrame -= p - Frame;
                return EditedFrame;
            }
        } else {
            if (Frame <= p)
                return EditedFrame;

            PrevPos = p;
            InEdit = true;
        }
    }
    if (InEdit) {
        EditedFrame += LastFrame - PrevPos;  // The last sequence had no actual "end" mark
        if (Frame < LastFrame)
            EditedFrame -= LastFrame - Frame;
    }
    return EditedFrame;
}

void GetCuttedLengthSize(const cRecording *Recording, cString &Text) {  // NOLINT
    #ifdef DEBUGFUNCSCALL
        dsyslog("flatPlus: cFlat::GetCuttedLenghtSize()");
    #endif

    cMarks Marks;
    bool HasMarks {false};
    const bool IsPesRecording = (Recording->IsPesRecording()) ? true : false;
    const double FramesPerSecond = Recording->FramesPerSecond();
    const char *RecordingFileName = Recording->FileName();
    cIndexFile *index {nullptr};
    // From skinElchiHD - Avoid triggering index generation for recordings with empty/missing index
    if (Recording->NumFrames() > 0) {
        HasMarks = Marks.Load(RecordingFileName, FramesPerSecond, IsPesRecording) && Marks.Count();
        index = new cIndexFile(RecordingFileName, false, IsPesRecording);
        // cIndexFile index(RecordingFileName, false, IsPesRecording);
    }

    bool FsErr {false};
    uint64_t FileSize[65535];
    FileSize[0] = 0;
    const uint16_t MaxFiles = IsPesRecording ? 999 : 65535;
    int i {0}, rc {0};
    struct stat FileBuf;
    cString FileName {""};
    do {
        ++i;
        if (IsPesRecording)
            FileName = cString::sprintf("%s/%03d.vdr", RecordingFileName, i);
        else
            FileName = cString::sprintf("%s/%05d.ts", RecordingFileName, i);
        rc = stat(*FileName, &FileBuf);
        if (rc == 0) {
            FileSize[i] = FileSize[i - 1] + FileBuf.st_size;
        } else if (ENOENT != errno) {
            esyslog("flatPlus: Error determining file size of \"%s\" %d (%s)", *FileName, errno, strerror(errno));
            FsErr = true;  // Remember failed status for later displaying an '!'
        }
    } while (i <= MaxFiles && !rc);

    int CuttedLength {0};
    uint64_t RecSizeCutted {0};
    if (HasMarks && index) {
        uint16_t FileNumber {0};
        off_t FileOffset {0};
        bool CutIn {true};
        int32_t CutInFrame {0}, position {0};
        uint64_t CutInOffset {0};
        cMark *Mark = Marks.First();
        while (Mark) {
            position = Mark->Position();
            index->Get(position, &FileNumber, &FileOffset);
            if (CutIn) {
                CutInFrame = position;
                CutIn = false;
                CutInOffset = FileSize[FileNumber - 1] + FileOffset;
            } else {
                CuttedLength += position - CutInFrame;
                CutIn = true;
                RecSizeCutted += FileSize[FileNumber - 1] + FileOffset - CutInOffset;
            }
            cMark *NextMark = Marks.Next(Mark);
            Mark = NextMark;
        }
        if (!CutIn) {
            CuttedLength += index->Last() - CutInFrame;
            index->Get(index->Last() - 1, &FileNumber, &FileOffset);
            RecSizeCutted += FileSize[FileNumber - 1] + FileOffset - CutInOffset;
        }
    }

    int LastIndex {0};
    if (index) {
        LastIndex = index->Last();
        Text.Append(
            cString::sprintf("%s: %s", tr("Length"), *IndexToHMSF(LastIndex, false, FramesPerSecond)));
        if (HasMarks)
            Text.Append(cString::sprintf(" (%s: %s)", tr("cutted"),
                                         *IndexToHMSF(CuttedLength, false, FramesPerSecond)));
        Text.Append("\n");
    }
    delete index;

    uint64_t RecSize{0};
    /* if (!FsErr) */ RecSize = FileSize[i - 1];  //? 0 when error opening file / Show partial size
    if (RecSize > MEGABYTE(1023))                 // Show a '!' when an error occurred detecting filesize
        Text.Append(cString::sprintf("%s: %s%.2f GB", tr("Size"), (FsErr) ? "!" : "",
                                     static_cast<float>(RecSize) / MEGABYTE(1024)));
    else
        Text.Append(cString::sprintf("%s: %s%lld MB", tr("Size"), (FsErr) ? "!" : "", RecSize / MEGABYTE(1)));

    if (HasMarks) {
        if (RecSize > MEGABYTE(1023))
            Text.Append(
                cString::sprintf(" (%s: %.2f GB)", tr("cutted"), static_cast<float>(RecSizeCutted) / MEGABYTE(1024)));
        else
            Text.Append(cString::sprintf(" (%s: %lld MB)", tr("cutted"), RecSizeCutted / MEGABYTE(1)));
    }
    Text.Append(cString::sprintf("\n%s: %d, %s: %d", trVDR("Priority"), Recording->Priority(), trVDR("Lifetime"),
                                 Recording->Lifetime()));

    // Add video format information (Format, Resolution, Framerate, …)
#if APIVERSNUM >= 20605
    const cRecordingInfo *RecInfo = Recording->Info();  // From skin ElchiHD
    if (RecInfo->FrameWidth() > 0 && RecInfo->FrameHeight() > 0) {
        Text.Append(cString::sprintf("\n%s: %s, %dx%d", tr("format"), (IsPesRecording) ? "PES" : "TS",
                                     RecInfo->FrameWidth(), RecInfo->FrameHeight()));
        if (FramesPerSecond > 0) {
            Text.Append(cString::sprintf("@%.2g", FramesPerSecond));
            if (RecInfo->ScanTypeChar() != '-')  // Do not show the '-' for unknown scan type
                Text.Append(cString::sprintf("%c", RecInfo->ScanTypeChar()));
        }
        if (RecInfo->AspectRatio() != arUnknown) Text.Append(cString::sprintf(" %s", RecInfo->AspectRatioText()));

        if (LastIndex)  //* Bitrate in new line
            Text.Append(cString::sprintf("\n%s: Ø %.2f MBit/s (Video + Audio)", tr("bit rate"),
                                         static_cast<float>(RecSize) / LastIndex * FramesPerSecond * 8 / MEGABYTE(1)));
    } else  // NOLINT
#endif
    {
        Text.Append(cString::sprintf("\n%s: %s", tr("format"), (IsPesRecording) ? "PES" : "TS"));

        if (LastIndex)  //* Bitrate at same line
            Text.Append(cString::sprintf(", %s: Ø %.2f MBit/s (Video + Audio)", tr("bit rate"),
                                         static_cast<float>(RecSize) / LastIndex * FramesPerSecond * 8 / MEGABYTE(1)));
    }
}

// Returns the string between start and end or an empty string if not found
std::string XmlSubstring(const std::string &source, const char *StrStart, const char *StrEnd) {
    const std::size_t start = source.find(StrStart);
    const std::size_t end = source.find(StrEnd);

    if (std::string::npos != start && std::string::npos != end)
        return (source.substr(start + strlen(StrStart), end - start - strlen(StrStart)));

    return std::string();  // Empty string
}

u_int32_t GetCharIndex(const char *Name, FT_ULong CharCode) {
    FT_Library library;
    FT_Face face;
    FT_UInt glyph_index {0};
    const cString FontFileName = cFont::GetFontFileName(Name);
    int rc = FT_Init_FreeType(&library);
    if (!rc) {
        rc = FT_New_Face(library, *FontFileName, 0, &face);
        if (!rc) {
            FT_Select_Charmap(face, FT_ENCODING_UNICODE);  // Ensure an unicode charater map is loaded
            rc = FT_Set_Char_Size(face, 8 * 64, 8 * 64, 0, 0);  // TODO: Is that needed?
            if (!rc) {
                glyph_index = FT_Get_Char_Index(face, CharCode);  // Glyph index 0 means 'undefined character code'
                // dsyslog("flatPlus: GetCharIndex() CharCode: 0x%lX (%ld), glyph_index: %d", CharCode, CharCode, glyph_index);
            } else
                esyslog("flatPlus: FreeType: error %d during FT_Set_Char_Size (font = %s)\n", rc, *FontFileName);
        } else
            esyslog("flatPlus: FreeType: load error %d (font = %s)", rc, *FontFileName);
    } else
        esyslog("flatPlus: FreeType: initialization error %d (font = %s)", rc, *FontFileName);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return glyph_index;
}

void JustifyLine(std::string &Line, cFont *Font, int LineMaxWidth) {  // NOLINT
    if (isempty(Line.c_str()))  // Check for empty line
        return;

    if (Font->Width("M") == Font->Width("i"))  // Check for fixed font
        return;

    uint LineSpaces {0};
    for (auto &ch : Line)  // Count spaces in 'Line'
        if (ch == ' ') ++LineSpaces;

    // Hair Space is a very small space:
    // https://de.wikipedia.org/wiki/Leerzeichen#Schriftzeichen_in_ASCII_und_andere_Kodierungen
    /* FT_ULong HairSpaceCode = 0x0000200A;  // HairSpace: U+200A
    FT_ULong ThinSpaceCode = 0x00002009;  // ThinSpace: U+2009
    if (GetCharIndex(Setup.FontOsd, HairSpaceCode) > 0) {
        FillChar = u8"\U0000200A";
    } else if (GetCharIndex(Setup.FontOsd, ThinSpaceCode) > 0) {
        FillChar = u8"\U00002009";
    } else {
        FillChar = " ";  // White space U+0020 (Decimal 32)
    } */
    //* Workaround for detecting 'HairSpace'
    const char *FillChar {nullptr};
    // Assume that 'tofu' char (Char not found) is bigger in size than and space
    const char *HairSpace = u8"\U0000200A", *Space = " ";
    if (Font->Width(Space) < Font->Width(HairSpace)) {  // Space ~ 5 pixel; HairSpace ~ 1 pixel; Tofu ~ 10 pixel
        FillChar = Space;
        // dsyslog("flatPlus: JustifyLine(): Using 'Space' (U+0020) as 'FillChar'");
    } else {
        FillChar = HairSpace;
        // dsyslog("flatPlus: JustifyLine(): Using 'HairSpace' (U+200A) as 'FillChar'");
    }
    const int FillCharWidth = Font->Width(FillChar);      // Width in pixel
    const std::size_t FillCharLength = strlen(FillChar);  // Length in chars

    const int LineWidth = Font->Width(Line.c_str());   // Width in Pixel
    if ((LineWidth + FillCharWidth) > LineMaxWidth) {  // Check if at least one 'FillChar' fits in to the line
        // dsyslog("flatPlus: JustifyLine() ---Line too long for extra space---");
        return;
    }

    if (LineSpaces == 0 || FillCharWidth == 0) {  // Avoid div/0 with lines without space
        // dsyslog("flatPlus: JustifyLine() Zero value found: Spaces: %d, FillCharWidth: %d", LineSpaces, FillCharWidth);
        return;
    }

    if (LineWidth > (LineMaxWidth * 0.8)) {  // Lines shorter than 80% looking bad when justified
        const int NeedFillChar = (LineMaxWidth - LineWidth) / FillCharWidth;  // How many 'FillChar' we need?
        int FillCharBlock = NeedFillChar / LineSpaces;                  // For inserting multiple 'FillChar'
        if (!FillCharBlock) ++FillCharBlock;                            // Set minimum to one 'FillChar'

        std::string FillChars {""};
        FillChars.reserve(16);
        for (int i {0}; i < FillCharBlock; ++i) {  // Create 'FillChars' block for inserting
            FillChars.append(FillChar);
        }
        const std::size_t FillCharsLength = FillChars.length();

        std::size_t LineLength = Line.length();
        Line.reserve(LineLength + (NeedFillChar * FillCharLength));
        int InsertedFillChar {0};
        /* dsyslog("flatPlus: JustifyLine() [Line: spaces %d, width %d, length %ld]\n"
                "[FillChar: needed %d, blocksize %d, remainder %d, width %d]\n"
                "[FillChars: length %ld]",
                LineSpaces, LineWidth, LineLength, NeedFillChar, FillCharBlock, NeedFillChar % LineSpaces,
                FillCharWidth, FillCharsLength); */

        //* Insert blocks at spaces
        std::size_t pos = Line.find(' ');
        while (pos != std::string::npos && ((InsertedFillChar + FillCharBlock) <= NeedFillChar)) {
            if (!(isspace(Line[pos - 1]))) {
                // dsyslog("flatPlus:  Insert block at %ld", pos);
                Line.insert(pos, FillChars);
                InsertedFillChar += FillCharBlock;
            }
            pos = Line.find(' ', pos + FillCharsLength + 1);  // Inserted chars plus one
        }
        // dsyslog("flatPlus: JustifyLine() InsertedFillChar after first loop (' '): %d", InsertedFillChar);

        pos = Line.find(".,?!;");  //* Insert blocks at (.,?!;)
        while (pos != std::string::npos && ((InsertedFillChar + FillCharBlock) <= NeedFillChar)) {
            if (pos < (LineLength - FillCharBlock - 1)) {
                // Check for repeating '.'
                if (Line[pos] != Line[pos + 1]) {  // Next char is different
                    // dsyslog("flatPlus:  Insert block at %ld", pos + 1);
                    Line.insert(pos + 1, FillChars);  // Insert after pos!
                    pos = Line.find(".,?!;", pos + FillCharsLength + 1);
                    InsertedFillChar += FillCharBlock;
                    LineLength = Line.length();
                } else {
                    // dsyslog("flatPlus:  Double '.' found");
                    ++pos;
                }
            } else {
                // dsyslog("flatPlus: No space for blocks left or end of line reached: %ld", pos);
                break;
            }
        }
        // dsyslog("flatPlus: JustifyLine() InsertedFillChar after second loop (.,?!;): %d", InsertedFillChar);

        //* Insert the remainder of 'NeedFillChar' left to right
        pos = Line.find_last_of(' ');
        while (pos != std::string::npos && (InsertedFillChar < NeedFillChar)) {
            if (!(isspace(Line[pos - 1]))) {
                // dsyslog("flatPlus:  Insert char at %ld", pos);
                Line.insert(pos, FillChar);
                ++InsertedFillChar;
            }
            pos = Line.find_last_of(' ', pos - FillCharLength);  // 'FillChar' can be more than one byte in length
        }
        // dsyslog("flatPlus: JustifyLine() InsertedFillChar after third loop (' '): %d", InsertedFillChar);
    } else {
        // dsyslog("flatPlus: JustifyLine() Line too short for justifying: LineWidth %d, LineMaxWidth * 0.8: %.0f",
        //        LineWidth, LineMaxWidth * 0.8);
        // return;
    }
}

std::string_view ltrim(std::string_view str) {
    const auto pos(str.find_first_not_of(" \t\n\r\f\v"));
    str.remove_prefix(std::min(pos, str.length()));
    return str;
}

std::string_view rtrim(std::string_view str) {
    const auto pos(str.find_last_not_of(" \t\n\r\f\v"));
    str.remove_suffix(std::min(str.length() - pos - 1, str.length()));
    return str;
}

std::string_view trim(std::string_view str) {
    str = ltrim(str);
    str = rtrim(str);
    return str;
}

// --- cTextFloatingWrapper --- // From skin ElchiHD
// Based on VDR's cTextWrapper
cTextFloatingWrapper::cTextFloatingWrapper(void) {}

cTextFloatingWrapper::~cTextFloatingWrapper() {
    free(m_Text);
}

void cTextFloatingWrapper::Set(const char *Text, const cFont *Font, int WidthLower, int UpperLines, int WidthUpper) {
    // uint32_t tick0 = GetMsTicks();  //! For testing
    // dsyslog("flatPlus: TextFloatingWrapper::Set() start. Textlength: %ld", strlen(Text));

    free(m_Text);
    m_Text = Text ? strdup(Text) : nullptr;
    if (!m_Text)
        return;
    m_Lines = 1;
    if (WidthUpper < 0 || WidthLower <= 0 || UpperLines < 0)
        return;

    char *Blank {nullptr}, *Delim {nullptr}, *s {nullptr};
    int cw {0}, l {0}, sl {0}, w {0};
    int Width = UpperLines > 0 ? WidthUpper : WidthLower;
    uint sym {0};
    stripspace(m_Text);  // Strips trailing newlines

    for (char *p = m_Text; *p;) {
        /* int */ sl = Utf8CharLen(p);
        /* uint */ sym = Utf8CharGet(p, sl);
        if (sym == '\n') {
            ++m_Lines;
            if (m_Lines > UpperLines)
                Width = WidthLower;
            w = 0;
            Blank = Delim = nullptr;
            p++;
            continue;
        } else if (sl == 1 && isspace(sym)) {
            Blank = p;
        }
        /* int */ cw = Font->Width(sym);
        if (w + cw > Width) {
            if (Blank) {
                *Blank = '\n';
                p = Blank;
                continue;
            } else if (w > 0) {  // There has to be at least one character before the newline.
                                 // Here's the ugly part, where we don't have any whitespace to
                                 // punch in a newline, so we need to make room for it:
                if (Delim)
                    p = Delim + 1;  // Let's fall back to the most recent delimiter

                /* char * */ s = MALLOC(char, strlen(m_Text) + 2);  // The additional '\n' plus the terminating '\0'
                /* int */ l = p - m_Text;
                strncpy(s, m_Text, l);  // Dest, Source, Size
                s[l] = '\n';            // Insert line break.
                strcpy(s + l + 1, p);   // Dest, Source
                free(m_Text);
                m_Text = s;
                p = m_Text + l;
                continue;
            }
        }
        w += cw;
        if (strchr("-.,:;!?_", *p)) {  //! Breaks '...'
            if (*p != *(p + 1)) {      // Next char is different, so use it for 'Delim'
                Delim = p;
                Blank = nullptr;
            } else {
                // dsyslog("flatPlus: TextFloatingWrapper::Set() skipping double delimiter char!");
            }
        }
        p += sl;
    }  // for char
    // uint32_t tick1 = GetMsTicks();
    // dsyslog("flatPlus: TextFloatingWrapper::Set() time: %d ms", tick1 - tick0);
}

const char *cTextFloatingWrapper::Text(void) {
    if (m_EoL) {
        *m_EoL = '\n';
        m_EoL = nullptr;
    }
    return m_Text;
}

const char *cTextFloatingWrapper::GetLine(int Line) {
    char *s {nullptr};
    if (Line < m_Lines) {
        if (m_EoL) {
            *m_EoL = '\n';
            if (Line == m_LastLine + 1)
                s = m_EoL + 1;
            m_EoL = nullptr;
        }
        if (!s) {
            s = m_Text;
            for (int i {0}; i < Line; ++i) {
                s = strchr(s, '\n');
                if (s)
                    s++;
                else
                    break;
            }
        }
        if (s) {
            if ((m_EoL = strchr(s, '\n')) != NULL)
                *m_EoL = 0;
        }
        m_LastLine = Line;
    }
    return s;
}
