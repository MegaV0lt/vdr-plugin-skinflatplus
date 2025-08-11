/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./flat.h"

#include <vdr/device.h>
#include <vdr/font.h>
#include <vdr/osd.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>  // For glyph metrics
#include <sys/stat.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <random>

#include "./displaychannel.h"
#include "./displaymenu.h"
#include "./displaymessage.h"
#include "./displayreplay.h"
#include "./displaytracks.h"
#include "./displayvolume.h"
#include "./fontcache.h"
#include "./glyphmetricscache.h"

#include "./services/epgsearch.h"

/* Possible values of the stream content descriptor according to ETSI EN 300 468 */
enum stream_content {
    sc_reserved        = 0x00,
    sc_video_MPEG2     = 0x01,  // MPEG2 video
    sc_audio_MP2       = 0x02,  // MPEG1 Layer 2 audio
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
// time_t m_RemoteTimersLastRefresh {0};

cFlat::cFlat() : cSkin("flatPlus", &::Theme) { Display_Menu = nullptr; }

const char *cFlat::Description() { return "flatPlus"; }

cSkinDisplayChannel *cFlat::DisplayChannel(bool WithInfo) { return new cFlatDisplayChannel(WithInfo); }

cSkinDisplayMenu *cFlat::DisplayMenu() {
    cFlatDisplayMenu *menu = new cFlatDisplayMenu;
    Display_Menu = menu;
    m_MenuActive = true;
    return menu;
}

cSkinDisplayReplay *cFlat::DisplayReplay(bool ModeOnly) { return new cFlatDisplayReplay(ModeOnly); }

cSkinDisplayVolume *cFlat::DisplayVolume() { return new cFlatDisplayVolume; }

cSkinDisplayTracks *cFlat::DisplayTracks(const char *Title, int NumTracks, const char *const *Tracks) {
    return new cFlatDisplayTracks(Title, NumTracks, Tracks);
}

cSkinDisplayMessage *cFlat::DisplayMessage() { return new cFlatDisplayMessage; }

cPixmap *CreatePixmap(cOsd *osd, const cString Name, int Layer, const cRect &ViewPort, const cRect &DrawPort) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: CreatePixmap('%s', %d, left %d, top %d, size %dx%d)", *Name, Layer,
            ViewPort.Left(), ViewPort.Top(), ViewPort.Width(), ViewPort.Height());
    if (DrawPort.Width() > 0 && DrawPort.Height() > 0) {
        dsyslog("   DrawPort: left %d, top %d, size %dx%d", DrawPort.Left(), DrawPort.Top(), DrawPort.Width(),
                DrawPort.Height());
    }
    cTimeMs Timer;  // Start Timer
#endif
    /* if (!osd) {
        esyslog("flatPlus: No osd! Could not create pixmap '%s' with size %ix%i", *Name, DrawPort.Width(),
                DrawPort.Height());
        return nullptr;
    } */

    if (cPixmap *pixmap {osd->CreatePixmap(Layer, ViewPort, DrawPort)}) {
#ifdef DEBUGFUNCSCALL
        if (Timer.Elapsed() > 0) dsyslog("   CreatePixmap() Time: %ld ms", Timer.Elapsed());
#endif
        return pixmap;
    }  // Everything runs according to the plan

    esyslog("flatPlus: Could not create pixmap '%s' of size %ix%i", *Name, DrawPort.Width(), DrawPort.Height());
    const cSize MaxPixmapSize {osd->MaxPixmapSize()};
    const int width {std::min(DrawPort.Width(), MaxPixmapSize.Width())};
    const int height {std::min(DrawPort.Height(), MaxPixmapSize.Height())};

    cRect NewDrawPort {DrawPort};
    NewDrawPort.SetSize(width, height);
    if (cPixmap *pixmap {osd->CreatePixmap(Layer, ViewPort, NewDrawPort)}) {
        isyslog("flatPlus: Created pixmap '%s' with reduced size %ix%i", *Name, width, height);
        return pixmap;
    }

    esyslog("flatPlus: Could not create pixmap '%s' with reduced size %ix%i", *Name, width, height);
    return nullptr;
}

// Optimized EpgSearch Plugin Lookup
bool cPluginSkinFlatPlus::s_bEpgSearchPluginChecked = false;
cPlugin *cPluginSkinFlatPlus::s_pEpgSearchPlugin = nullptr;
// Optimized Scraper Plugin Lookup
bool cPluginSkinFlatPlus::s_bScraperPluginChecked = false;
cPlugin *cPluginSkinFlatPlus::s_pScraperPlugin = nullptr;

// Get MediaPath, Series/Movie info and add actors if wanted
void GetScraperMedia(cString &MediaPath, cString &SeriesInfo, cString &MovieInfo,                              // NOLINT
                     std::vector<cString> &ActorsPath,                                                         // NOLINT
                     std::vector<cString> &ActorsName, std::vector<cString> &ActorsRole, const cEvent *Event,  // NOLINT
                     const cRecording *Recording) {
    static cPlugin *pScraper {cPluginSkinFlatPlus::GetScraperPlugin()};
    if (pScraper) {
        ScraperGetEventType call;
        if (Event)
            call.event = Event;
        else if (Recording)
            call.recording = Recording;
        else
            return;  // Check if both are unset

        int seriesId {0}, episodeId {0}, movieId {0};
        if (pScraper->Service("GetEventType", &call)) {
            seriesId = call.seriesId;
            episodeId = call.episodeId;
            movieId = call.movieId;
        }
        if (call.type == tSeries) {
            cSeries series;
            series.seriesId = seriesId;
            series.episodeId = episodeId;
            if (pScraper->Service("GetSeries", &series)) {
                if (series.banners.size() > 1) {  // Use random banner
                    // Gets 'entropy' from device that generates random numbers itself
                    // to seed a mersenne twister (pseudo) random generator
                    std::mt19937 generator(std::random_device {}());

                    // Make sure all numbers have an equal chance.
                    // Range is inclusive (so we need -1 for vector index)
                    std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                    const std::size_t number {distribution(generator)};
                    MediaPath = series.banners[number].path.c_str();
                    dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                            static_cast<int>(number + 1), *MediaPath,
                            static_cast<int>(series.banners.size()));  // Log result
                } else if (series.banners.size() == 1) {               // Just one banner
                    MediaPath = series.banners[0].path.c_str();
                }
                if ((Event && Config.TVScraperEPGInfoShowActors) || (Recording && Config.TVScraperRecInfoShowActors)) {
                    const std::size_t ActorsSize {series.actors.size()};
                    ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                    ActorsName.reserve(ActorsSize);
                    ActorsRole.reserve(ActorsSize);
                    for (std::size_t i {0}; i < ActorsSize; ++i) {
                        if (LastModifiedTime(series.actors[i].actorThumb.path.c_str())) {
                            ActorsPath.emplace_back(series.actors[i].actorThumb.path.c_str());
                            ActorsName.emplace_back(series.actors[i].name.c_str());
                            ActorsRole.emplace_back(series.actors[i].role.c_str());
                        }
                    }
                }
                InsertSeriesInfos(series, SeriesInfo);  // Add series infos
            }
        } else if (call.type == tMovie) {
            cMovie movie;
            movie.movieId = movieId;
            if (pScraper->Service("GetMovie", &movie)) {
                MediaPath = movie.poster.path.c_str();
                if ((Event && Config.TVScraperEPGInfoShowActors) || (Recording && Config.TVScraperRecInfoShowActors)) {
                    const std::size_t ActorsSize {movie.actors.size()};
                    ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                    ActorsName.reserve(ActorsSize);
                    ActorsRole.reserve(ActorsSize);
                    for (std::size_t i {0}; i < ActorsSize; ++i) {
                        if (LastModifiedTime(movie.actors[i].actorThumb.path.c_str())) {
                            ActorsPath.emplace_back(movie.actors[i].actorThumb.path.c_str());
                            ActorsName.emplace_back(movie.actors[i].name.c_str());
                            ActorsRole.emplace_back(movie.actors[i].role.c_str());
                        }
                    }
                }
                InsertMovieInfos(movie, MovieInfo);  // Add movie infos
            }
        }
    }  // Scraper plugin
}

// Get MediaPath, MediaSize and return MediaType
int GetScraperMediaTypeSize(cString &MediaPath, cSize &MediaSize, const cEvent *Event,  // NOLINT
                            const cRecording *Recording) {
    static cPlugin *pScraper {cPluginSkinFlatPlus::GetScraperPlugin()};
    if (pScraper) {
        ScraperGetEventType call;
        if (Event)
            call.event = Event;
        else if (Recording)
            call.recording = Recording;
        else
            return 0;  // Check if both are unset

        int seriesId {0}, episodeId {0}, movieId {0};
        if (pScraper->Service("GetEventType", &call)) {
            seriesId = call.seriesId;
            episodeId = call.episodeId;
            movieId = call.movieId;
        }
        if (call.type == tSeries) {
            cSeries series;
            series.seriesId = seriesId;
            series.episodeId = episodeId;
            if (pScraper->Service("GetSeries", &series)) {
                if (series.banners.size() > 1) {  // Use random banner
                    // Gets 'entropy' from device that generates random numbers itself
                    // to seed a mersenne twister (pseudo) random generator
                    std::mt19937 generator(std::random_device {}());

                    // Make sure all numbers have an equal chance.
                    // Range is inclusive (so we need -1 for vector index)
                    std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                    const std::size_t number {distribution(generator)};
                    MediaPath = series.banners[number].path.c_str();
                    MediaSize.Set(series.banners[number].width, series.banners[number].height);
                    dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                            static_cast<int>(number + 1), *MediaPath,
                            static_cast<int>(series.banners.size()));  // Log result
                } else if (series.banners.size() == 1) {               // Just one banner
                    MediaPath = series.banners[0].path.c_str();
                    MediaSize.Set(series.banners[0].width, series.banners[0].height);
                }
                return 1;  // MediaType = 1;
            }
        } else if (call.type == tMovie) {
            cMovie movie;
            movie.movieId = movieId;
            if (pScraper->Service("GetMovie", &movie)) {
                MediaPath = movie.poster.path.c_str();
                MediaSize.Set(movie.poster.width, movie.poster.height);
                return 2;  // MediaType = 2;
            }
        }
    }
    return 0;  // tNone
}

void InsertSeriesInfos(const cSeries &Series, cString &SeriesInfo) {  // NOLINT
    if (Series.name.length() > 0) SeriesInfo.Append(cString::sprintf("%s%s\n", tr("name: "), Series.name.c_str()));
    if (Series.firstAired.length() > 0)
        SeriesInfo.Append(cString::sprintf("%s%s\n", tr("first aired: "), Series.firstAired.c_str()));
    if (Series.network.length() > 0)
        SeriesInfo.Append(cString::sprintf("%s%s\n", tr("network: "), Series.network.c_str()));
    if (Series.genre.length() > 0) SeriesInfo.Append(cString::sprintf("%s%s\n", tr("genre: "), Series.genre.c_str()));
    if (Series.rating > 0) SeriesInfo.Append(cString::sprintf("%s%.1f\n", tr("rating: "), Series.rating));  // TheTVDB
    if (Series.status.length() > 0)
        SeriesInfo.Append(cString::sprintf("%s%s\n", tr("status: "), Series.status.c_str()));
    if (Series.episode.season > 0)
        SeriesInfo.Append(cString::sprintf("%s%d\n", tr("season number: "), Series.episode.season));
    if (Series.episode.number > 0)
        SeriesInfo.Append(cString::sprintf("%s%d\n", tr("episode number: "), Series.episode.number));
}

void InsertMovieInfos(const cMovie &Movie, cString &MovieInfo) {  // NOLINT
    if (Movie.title.length() > 0) MovieInfo.Append(cString::sprintf("%s%s\n", tr("title: "), Movie.title.c_str()));
    if (Movie.originalTitle.length() > 0)
        MovieInfo.Append(cString::sprintf("%s%s\n", tr("original title: "), Movie.originalTitle.c_str()));
    if (Movie.collectionName.length() > 0)
        MovieInfo.Append(cString::sprintf("%s%s\n", tr("collection name: "), Movie.collectionName.c_str()));
    if (Movie.genres.length() > 0) MovieInfo.Append(cString::sprintf("%s%s\n", tr("genre: "), Movie.genres.c_str()));
    if (Movie.releaseDate.length() > 0)
        MovieInfo.Append(cString::sprintf("%s%s\n", tr("release date: "), Movie.releaseDate.c_str()));
    // TheMovieDB
    if (Movie.popularity > 0) MovieInfo.Append(cString::sprintf("%s%.1f\n", tr("popularity: "), Movie.popularity));
    if (Movie.voteAverage > 0)
        MovieInfo.Append(
            cString::sprintf("%s%.0f%%\n", tr("vote average: "), Movie.voteAverage * 10));  // 10 Points = 100%
}

cString GetAspectIcon(int ScreenWidth, double ScreenAspect) {
    if (Config.ChannelSimpleAspectFormat && ScreenWidth > 720) return (ScreenWidth > 1920) ? "uhd" : "hd";  // UHD or HD

    static constexpr double ScreenAspects[] {16.0 / 9.0, 20.0 / 11.0, 15.0 / 11.0, 4.0 / 3.0, 2.21};
    static const cString ScreenAspectNames[] {"169", "169w", "169w", "43", "221"};
    const uint16_t ScreenAspectNums {sizeof(ScreenAspects) / sizeof(ScreenAspects[0])};
    for (std::size_t i {0}; i < ScreenAspectNums; ++i) {
        if (std::abs(ScreenAspect - ScreenAspects[i]) < 0.0001)  // Compare double with epsilon tolerance
            return ScreenAspectNames[i];
    }

    dsyslog("flatPlus: Unknown screen aspect (%.5f)", ScreenAspect);
    return "unknown_asp";
}

cString GetScreenResolutionIcon(int ScreenWidth, int ScreenHeight) {
    /* Resolutions
    7680×4320 (UHD-2 / 8K)        3840×2160 (UHD-1 / 4K)
    2560x1440 (QHD) //? Is that used somewhere on sat/cable?
    1920x1080 (HD1080 Full HDTV)  1440x1080 (HD1080 DV)
    1280x720 (HD720)              960x720 (HD720 DV)
    720x576 (PAL)                 704x576 (PAL)
    544x576 (PAL)                 528x576 (PAL)
    480x576 (PAL SVCD)            352x576 (PAL CVD) */

    static const cString ResNames[] {"7680x4320", "3840x2160", "2560x1440", "1920x1080", "1440x1080",
                                     "1280x720",  "960x720",   "720x576",   "704x576",   "544x576",
                                     "528x576",   "480x576",   "352x576"};
    static constexpr int16_t ResWidths[] {7680, 3840, 2560, 1920, 1440, 1280, 960, 720, 704, 544, 528, 480, 352};
    const uint16_t ResNums {sizeof(ResNames) / sizeof(ResNames[0])};
    for (std::size_t i {0}; i < ResNums; ++i) {
        if (ScreenWidth == ResWidths[i]) return ResNames[i];
    }

    dsyslog("flatPlus: Unknown screen resolution: %dx%d", ScreenWidth, ScreenHeight);
    return "unknown_res";
}

cString GetFormatIcon(int ScreenWidth) {
    return (ScreenWidth > 1920) ? "uhd" : (ScreenWidth > 720) ? "hd" : "sd";  // 720 and below is considered sd
}

cString GetRecordingFormatIcon(const cRecording *Recording) {
#if APIVERSNUM >= 20605
    const uint16_t FrameWidth {Recording->Info()->FrameWidth()};
    if (FrameWidth > 1920) return "uhd";  // TODO: Separate images
    if (FrameWidth > 720) return "hd";
    if (FrameWidth > 0) return "sd";  // 720 and below is considered sd
#endif
    // Find radio and H.264/H.265 streams.
    //! Detection FAILED for RTL, SAT1 etc. They do not send a video component :-(
    if (const auto *Components {Recording->Info()->Components()}) {
        for (int16_t i {0}, n = Components->NumComponents(); i < n; ++i) {  // TODO: Use for_each
            switch (Components->Component(i)->stream) {
            case sc_video_MPEG2: return "sd";
            case sc_video_H264_AVC: return "hd";
            case sc_video_H265_HEVC: return "uhd";
            default: break;
            }
        }
    }
    return "";
}

cString GetCurrentAudioIcon() {
    const eTrackType CurrentAudioTrack {cDevice::PrimaryDevice()->GetCurrentAudioTrack()};
    return (IS_AUDIO_TRACK(CurrentAudioTrack))   ? "audio_stereo"
           : (IS_DOLBY_TRACK(CurrentAudioTrack)) ? "audio_dolby"
                                                 : "";  // No audio?
}

cString GetRecordingErrorIcon(int RecInfoErrors) {
    return (RecInfoErrors == 0)                                                     ? "recording_ok"
           : (RecInfoErrors < 0)                                                    ? "recording_untested"
           : (RecInfoErrors < Config.MenuItemRecordingShowRecordingErrorsThreshold) ? "recording_warning"
                                                                                    : "recording_error";
}

cString GetRecordingSeenIcon(int FrameTotal, int FrameResume) {
    static const cString SeenIconNames[] {"recording_seen_0", "recording_seen_1", "recording_seen_2",
                                          "recording_seen_3", "recording_seen_4", "recording_seen_5",
                                          "recording_seen_6", "recording_seen_7", "recording_seen_8",
                                          "recording_seen_9", "recording_seen_10"};

    if (FrameTotal == 0) {  // Avoid DIV/0
        esyslog("flatPlus: Error in GetRecordingSeenIcon() FrameTotal is 0! FrameResume: %d", FrameResume);
        return SeenIconNames[0];  // 0 = 0%
    }

    const double FrameSeen {static_cast<double>(FrameResume) / FrameTotal};
    const double SeenThreshold {Config.MenuItemRecordingSeenThreshold * 100.0};
    // dsyslog("flatPlus: Config.MenuItemRecordingSeenThreshold: %.2f", SeenThreshold);
    if (FrameSeen >= SeenThreshold) return SeenIconNames[10];  // 10 = 100%

    const int idx {std::min(static_cast<int>(FrameSeen * 10.0 + 0.5), 10)};  // 0..10 rounded
    return SeenIconNames[idx];
}

/**
 * Adjusts the size of a media object based on its aspect ratio and the
 * constraints provided by the content size. This function ensures that
 * the media's aspect ratio is preserved while fitting within the
 * specified dimensions. It categorizes media as poster, portrait, or
 * banner based on its aspect ratio and adjusts its size accordingly.
 *
 * @param ContentSize The size constraints within which the media should fit.
 * @param MediaSize A reference to the size of the media to be adjusted.
 *
 * - Posters are adjusted to a maximum height of 70% of the content height.
 * - Portraits are adjusted to a maximum width of 1/3 of the content width.
 * - Banners are adjusted to maintain a target ratio of 758 width at 1920.
 */
void SetMediaSize(const cSize &ContentSize, cSize &MediaSize) {  // NOLINT
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: SetMediaSize() MediaSize %dx%d, ContentSize %dx%d", MediaSize.Width(), MediaSize.Height(),
            ContentSize.Width(), ContentSize.Height());
#endif

    if (MediaSize.Height() == 0) {  // Avoid DIV/0
        esyslog("flatPlus: Error in SetMediaSize() MediaSize.Height() is 0!");
        return;
    }

    static constexpr int kPosterAspectThreshold {1};              // Smaller than 1 = Poster
    static constexpr int kBannerAspectThreshold {4};              // Smaller than 4 = Portrait, bigger than 4 = Banner
    static constexpr double kPosterHeightRatio {0.7};             // Max 70% of pixmap height
    static constexpr double kPortraitWidthRatio {1.0 / 3.0};      // Max 1/3 of pixmap width
    static constexpr double kBannerTargetRatio {758.0 / 1920.0};  // To get 758 width @ 1920

    //* Set to default size
    const uint16_t Aspect = MediaSize.Width() / MediaSize.Height();  // Aspect ratio as integer. Narrowing conversion
    //* Aspect of image is preserved in cImageLoader::LoadFile()
    if (Aspect < kPosterAspectThreshold) {  //* Poster (For example 680x1000 = 0.68)
        MediaSize.SetHeight(static_cast<int>(ContentSize.Height() * kPosterHeightRatio));
    } else if (Aspect < kBannerAspectThreshold) {  //* Portrait (For example 1920x1080 = 1.77)
        MediaSize.SetWidth(static_cast<int>(ContentSize.Width() * kPortraitWidthRatio));
    } else {  //* Banner (Usually 758x140 = 5.41)
        MediaSize.SetWidth(static_cast<int>(ContentSize.Width() * kBannerTargetRatio));
    }
#ifdef DEBUGFUNCSCALL
    dsyslog("   New MediaSize max. %dx%d", MediaSize.Width(), MediaSize.Height());
#endif
}

void InsertComponents(const cComponents *Components, cString &Text, cString &Audio, cString &Subtitle,  // NOLINT
                      bool NewLine) {
    cString AudioType {""};
    for (int16_t i {0}; i < Components->NumComponents(); ++i) {
        const tComponent *p {Components->Component(i)};
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
            if (Audio[0] != '\0') Audio.Append(", ");
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
            if (Subtitle[0] != '\0') Subtitle.Append(", ");
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

    const std::string AuxInfo {RecInfo->Aux()};  // Cache aux info
    std::string Buffer {XmlSubstring(AuxInfo, "<epgsearch>", "</epgsearch>")};
    std::string Channel {""}, Searchtimer {""};
    if (!Buffer.empty()) {
        Channel.reserve(32);
        Searchtimer.reserve(32);
        Channel = XmlSubstring(Buffer, "<channel>", "</channel>");
        Searchtimer = XmlSubstring(Buffer, "<searchtimer>", "</searchtimer>");
        if (Searchtimer.empty()) Searchtimer = XmlSubstring(Buffer, "<Search timer>", "</Search timer>");
    }

    Buffer = XmlSubstring(AuxInfo, "<tvscraper>", "</tvscraper>");
    std::string Causedby {""}, Reason {""};
    if (!Buffer.empty()) {
        Causedby.reserve(32);
        Reason.reserve(32);
        Causedby = XmlSubstring(Buffer, "<causedBy>", "</causedBy>");
        Reason = XmlSubstring(Buffer, "<reason>", "</reason>");
    }

    Buffer = XmlSubstring(AuxInfo, "<vdradmin-am>", "</vdradmin-am>");
    std::string Pattern {""};
    if (!Buffer.empty()) {
        Pattern.reserve(32);
        Pattern = XmlSubstring(Buffer, "<pattern>", "</pattern>");
    }

    if (InfoLine) {
        if ((!Channel.empty() && !Searchtimer.empty()) || (!Causedby.empty() && !Reason.empty()) || !Pattern.empty())
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

int GetEpgsearchConflicts() {
    cPlugin *pEpgSearchPlugin {cPluginSkinFlatPlus::GetEpgSearchPlugin()};
    if (pEpgSearchPlugin) {
        Epgsearch_lastconflictinfo_v1_0 ServiceData {.nextConflict = 0, .relevantConflicts = 0, .totalConflicts = 0};
        pEpgSearchPlugin->Service("Epgsearch-lastconflictinfo-v1.0", &ServiceData);
        if (ServiceData.relevantConflicts > 0) return ServiceData.relevantConflicts;
    }  // pEpgSearch
    return 0;
}

int GetFrameAfterEdit(const cMarks *marks, int Frame, int LastFrame) {  // From SkinLCARSNG
    if (LastFrame < 0 || Frame < 0 || Frame > LastFrame) return -1;

    int EditedFrame {0};
    int p {0}, PrevPos {-1};
    bool InEdit {false};
    for (const cMark *mi {marks->First()}; mi; mi = marks->Next(mi)) {
        p = mi->Position();
        if (InEdit) {
            EditedFrame += p - PrevPos;
            InEdit = false;
            if (Frame <= p) {
                EditedFrame -= p - Frame;
                return EditedFrame;
            }
        } else {
            if (Frame <= p) return EditedFrame;

            PrevPos = p;
            InEdit = true;
        }
    }
    if (InEdit) {
        EditedFrame += LastFrame - PrevPos;  // The last sequence had no actual "end" mark
        if (Frame < LastFrame) EditedFrame -= LastFrame - Frame;
    }
    return EditedFrame;
}

void InsertCutLengthSize(const cRecording *Recording, cString &Text) {  // NOLINT
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlat::InsertCutLengthSize()");
#endif

    cMarks Marks;
    bool HasMarks {false};
    const char *RecordingFileName {Recording->FileName()};
    const bool IsPesRecording {Recording->IsPesRecording()};
    const double FramesPerSecond {Recording->FramesPerSecond()};
    std::unique_ptr<cIndexFile> index;  // Automatically deleted; no need for 'new'
    int LastIndex {0};
    uint16_t MaxFileNum {0};

    if (FramesPerSecond == 0.0) {  // Avoid DIV/0
        esyslog("flatPlus: Error in InsertCutLengthSize() FramesPerSecond is 0.0!");
        return;
    }

    // From skinElchiHD - Avoid triggering index generation for recordings with empty/missing index
    if (Recording->NumFrames() > 0) {
        HasMarks = Marks.Load(RecordingFileName, FramesPerSecond, IsPesRecording) && Marks.Count();
        index = std::make_unique<cIndexFile>(RecordingFileName, false, IsPesRecording);  // Assign unique ptr object
        if (index) {
            off_t dummy;
            LastIndex = index->Last();
            index->Get(LastIndex, &MaxFileNum, &dummy);
        }
    }

    if (MaxFileNum == 0) {
        esyslog("flatPlus: Error in InsertCutLengthSize() MaxFileNum is 0!");
        return;
    }

    bool FsErr {false};
    std::vector<uint64_t> FileSize(MaxFileNum + 1, 0);
    int rc {0};
    struct stat FileBuf;
    cString FileName {""};
    for (uint16_t i {1}; i <= MaxFileNum && !rc; ++i) {
        FileName = IsPesRecording ? cString::sprintf("%s/%03d.vdr", RecordingFileName, i)
                                  : cString::sprintf("%s/%05d.ts", RecordingFileName, i);

        rc = stat(*FileName, &FileBuf);
        if (rc == 0) {
            FileSize[i] = FileSize[i - 1] + FileBuf.st_size;
        } else if (ENOENT != errno) {
            esyslog("flatPlus: Error determining file size of '%s' %d (%s)", *FileName, errno, strerror(errno));
            FsErr = true;  // Remember failed status for later displaying an '!'
        }
    }

    int CutLength {0};
    uint64_t RecSizeCut {0};
    if (HasMarks && index) {
        uint16_t FileNumber {0};
        off_t FileOffset {0};
        bool CutIn {true};
        int32_t CutInFrame {0}, position {0};
        uint64_t CutInOffset {0};
        for (cMark *Mark {Marks.First()}; Mark; Mark = Marks.Next(Mark)) {
            position = Mark->Position();
            index->Get(position, &FileNumber, &FileOffset);
            if (CutIn) {
                CutInFrame = position;
                CutIn = false;
                CutInOffset = FileSize[FileNumber - 1] + FileOffset;
            } else {
                CutLength += position - CutInFrame;
                CutIn = true;
                RecSizeCut += FileSize[FileNumber - 1] + FileOffset - CutInOffset;
            }
        }
        if (!CutIn) {
            CutLength += index->Last() - CutInFrame;
            index->Get(index->Last() - 1, &FileNumber, &FileOffset);
            RecSizeCut += FileSize[FileNumber - 1] + FileOffset - CutInOffset;
        }
    }

    if (index && LastIndex) {  // Do not show zero value
        Text.Append(cString::sprintf("%s: %s", tr("Length"), *IndexToHMSF(LastIndex, false, FramesPerSecond)));
        if (HasMarks && CutLength > 0)  // Do not show zero value
            Text.Append(cString::sprintf(" (%s: %s)", tr("cutted"), *IndexToHMSF(CutLength, false, FramesPerSecond)));
        Text.Append("\n");
    }

    const uint64_t RecSize {FileSize[MaxFileNum]};  // Size of the recording in bytes
    if (RecSize > MEGABYTE(1023))                   // Show a '!' when an error occurred detecting filesize
        Text.Append(cString::sprintf("%s: %s%.2f GB", tr("Size"), (FsErr) ? "!" : "",
                                     static_cast<float>(RecSize) / MEGABYTE(1024)));
    else
        Text.Append(cString::sprintf("%s: %s%lld MB", tr("Size"), (FsErr) ? "!" : "", RecSize / MEGABYTE(1)));

    if (HasMarks && (RecSizeCut)) {  // Do not show zero value
        if (RecSizeCut > MEGABYTE(1023))
            Text.Append(
                cString::sprintf(" (%s: %.2f GB)", tr("cutted"), static_cast<float>(RecSizeCut) / MEGABYTE(1024)));
        else
            Text.Append(cString::sprintf(" (%s: %lld MB)", tr("cutted"), RecSizeCut / MEGABYTE(1)));
    }
    Text.Append(cString::sprintf("\n%s: %d, %s: %d", trVDR("Priority"), Recording->Priority(), trVDR("Lifetime"),
                                 Recording->Lifetime()));

    // Add video format information (Format, Resolution, Framerate, …)
#if APIVERSNUM >= 20605
    const cRecordingInfo *RecInfo {Recording->Info()};  // From skin ElchiHD
    if (RecInfo->FrameWidth() > 0 && RecInfo->FrameHeight() > 0) {
        Text.Append(cString::sprintf("\n%s: %s, %dx%d", tr("format"), (IsPesRecording) ? "PES" : "TS",
                                     RecInfo->FrameWidth(), RecInfo->FrameHeight()));
        Text.Append(cString::sprintf("@%.2g", FramesPerSecond));
        if (RecInfo->ScanTypeChar() != '-')  // Do not show the '-' for unknown scan type
            Text.Append(cString::sprintf("%c", RecInfo->ScanTypeChar()));
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
    auto StartPos {source.find(StrStart)};
    if (StartPos == std::string::npos) return {};

    StartPos += strlen(StrStart);
    const auto EndPos {source.find(StrEnd, StartPos)};
    if (EndPos == std::string::npos) return {};

    return source.substr(StartPos, EndPos - StartPos);
}

// Optimized GetGlyphSize with caching
GlyphMetricsCache &glyphMetricsCache() {
    static GlyphMetricsCache s_cache;
    return s_cache;
}
/**
 * @brief Get the size of a glyph in a given font and font height.
 * @param[in] Name The name of the font to use.
 * @param[in] CharCode The character code of the glyph to query.
 * @param[in] FontHeight The height of the font in pixels.
 * @return The size of the glyph in pixels, rounded up to the nearest integer.
 * @note This function returns 0 if any error occurs during the execution of the function.
 */
uint32_t GetGlyphSize(const char *Name, const FT_ULong CharCode, const int FontHeight) {
    GlyphMetricsCache &cache = glyphMetricsCache();

    FT_Face face = cache.GetFace(*cFont::GetFontFileName(Name));
    if (face == nullptr) {
        esyslog("flatPlus: GetGlyphSize() Error: Can't find face (Font = %s)", *cFont::GetFontFileName(Name));
        return 0;
    }
    if (FT_Set_Char_Size(face, FontHeight * 64, FontHeight * 64, 0, 0) != 0) {
        esyslog("flatPlus: GetGlyphSize() Error: Can't set char size (Font = %s)", *cFont::GetFontFileName(Name));
        return 0;
    }
    FT_GlyphSlot slot = face->glyph;
    if (FT_Load_Glyph(face, FT_Get_Char_Index(face, CharCode), FT_LOAD_DEFAULT) != 0) {
        esyslog("flatPlus: GetGlyphSize() Error: Can't load glyph (Font = %s)", *cFont::GetFontFileName(Name));
        return 0;
    }
    return (slot->metrics.height + 63) / 64;
}

/**
 * @brief Get the name of a font from its file name.
 * @param[in] FontFileName The file name of the font to query.
 * @return The name of the font, including style name if applicable.
 * @note This function caches the FreeType face for the given font file and returns the face->family_name
 *       and face->style_name (if applicable) as a cString.
 */
cString GetFontName(const char *FontFileName) {
    GlyphMetricsCache &cache = glyphMetricsCache();

    if (FontFileName == nullptr) {
        esyslog("flatPlus: GetFontName() Error: FontFileName is nullptr");
        return "";
    }
    FT_Face face = cache.GetFace(FontFileName);
    if (face == nullptr) {
        esyslog("flatPlus: GetFontName() Error: Can't find face (Font = %s)", FontFileName);
        return "";
    }
    if (face->family_name == nullptr) {
        esyslog("flatPlus: GetFontName() Error: face->family_name is nullptr (Font = %s)", FontFileName);
        return "";
    }
    cString FontName {face->family_name};  // Start with the family name of the font
    if (face->style_name != nullptr) {
        FontName.Append(cString::sprintf(":%s", face->style_name));  // Add style name if it exists
    } else {
        FontName.Append(":Regular");  // Default to "Regular" if no style name exists
    }
    return FontName;
}

/**
 * @brief Justifies a line of text by inserting fill characters to fit a specified maximum width.
 *
 * This function adjusts the spacing within a line of text to ensure that it fits within the specified
 * maximum width. The line is justified by inserting fill characters, typically spaces or hair spaces,
 * between existing spaces or punctuation marks in the text. The function does not justify lines with
 * fixed-width fonts or lines that cannot accommodate additional fill characters.
 *
 * @param Line The line of text to be justified, passed by reference and modified in place.
 * @param Font The font used to calculate the width of characters and spaces. Must not be null.
 * @param LineMaxWidth The maximum width in pixels that the justified line should fit within.
 *
 * @note The function returns immediately if the font is null, the line is empty, or the maximum
 * width is non-positive. Additionally, no justification is performed if a fixed-width font is detected.
 */
void JustifyLine(std::string &Line, const cFont *Font, const int LineMaxWidth) {  // NOLINT
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: JustifyLine() '%s'", Line.c_str());
    cTimeMs Timer;  // Set Timer
#endif
    if (Line.empty() || LineMaxWidth <= 0)  // Check for empty line or invalid LineMaxWidth
        return;

    if (!Font) {
        esyslog("flatPlus: JustifyLine() called with null Font");
        return;
    }

    // Get the font name (Ubuntu:Regular)
    const cString FontName {*GetFontName(Font->FontName())};
    // dsyslog("   FontName: '%s'", *FontName);

    // Use cached font metrics to determine if the font is fixed-width
    const int FontHeight {FontCache.GetFontHeight(FontName, Font->Size())};
    if (FontCache.GetStringWidth(FontName, FontHeight, "M") ==
        FontCache.GetStringWidth(FontName, FontHeight, "i"))
        return;  // Font is fixed-width, no justification needed
#ifdef DEBUGFUNCSCALL
    dsyslog("   FontHeight %d, Width 'M' %d, Width 'i' %d",
            FontHeight,
            FontCache.GetStringWidth(FontName, FontHeight, "M"),
            FontCache.GetStringWidth(FontName, FontHeight, "i"));
#endif

    // Count spaces in line
    const int LineSpaces = std::count_if(Line.begin(), Line.end(), [](char c) { return c == ' '; });

    // Hair Space is a very small space:
    // https://de.wikipedia.org/wiki/Leerzeichen#Schriftzeichen_in_ASCII_und_andere_Kodierungen
    // HairSpace: U+200A, ThinSpace: U+2009

    //* Detect 'HairSpace'
    // Assume that 'tofu' char (Char not found) is bigger in size than space
    // Space ~ 5 pixel; HairSpace ~ 1 pixel; Tofu ~ 10 pixel
    const char *FillChar {FontCache.GetStringWidth(FontName, FontHeight, " ") <
                          FontCache.GetStringWidth(FontName, FontHeight, u8"\U0000200A")
                              ? " "
                              : u8"\U0000200A"};          // Use hair space if it is smaller than space
    const int16_t FillCharWidth = FontCache.GetStringWidth(FontName, FontHeight, FillChar);  // Width in pixel
    const std::size_t FillCharLength {strlen(FillChar)};  // Length in chars

    const int16_t LineWidth = Font->Width(Line.c_str());  // Width in Pixel
    if ((LineWidth + FillCharWidth) > LineMaxWidth)       // Check if at least one 'FillChar' fits in to the line
        return;

    if (LineSpaces == 0 || FillCharWidth == 0) {  // Avoid DIV/0 with lines without space
        // dsyslog("flatPlus: JustifyLine() Zero value found: Spaces: %d, FillCharWidth: %d", LineSpaces,
        //          FillCharWidth);
        return;
    }

    static constexpr float kLineWidthThreshold {0.8f};       // Line width threshold for justifying
    static const char *kPunctuationChars {".,?!;"};          // Punctuation characters for justifying
    if (LineWidth > (LineMaxWidth * kLineWidthThreshold)) {  // Lines shorter than 80% looking bad when justified
        const int16_t NeedFillChar = (LineMaxWidth - LineWidth) / FillCharWidth;  // How many 'FillChar' we need?
        const int16_t FillCharBlock = std::max(NeedFillChar / LineSpaces, 1);     // For inserting multiple 'FillChar'
        std::string FillChars {""};
        FillChars.reserve(FillCharBlock);
        for (int16_t i {0}; i < FillCharBlock; ++i) {  // Create 'FillChars' block for inserting
            FillChars.append(FillChar);
        }

        const std::size_t FillCharsLength {FillChars.length()};
        std::size_t LineLength {Line.length()};
        Line.reserve(LineLength + (NeedFillChar * FillCharLength));
        int16_t InsertedFillChar {0};
#ifdef DEBUGFUNCSCALL
        dsyslog("   [Line: spaces %d, width %d, length %ld]\n"
                "   [FillChar: needed %d, blocksize %d, remainder %d, width %d]\n"
                "   [FillChars: length %ld]",
                LineSpaces, LineWidth, LineLength, NeedFillChar, FillCharBlock, NeedFillChar % LineSpaces,
                FillCharWidth, FillCharsLength);
#endif
        //* Insert blocks at (space)
        std::size_t pos {0};  // Position also used in following loops
        for (pos = Line.find(' ');
             pos != std::string::npos && pos > 0 && (InsertedFillChar + FillCharBlock <= NeedFillChar);
             pos = Line.find(' ', pos + FillCharsLength + 1)) {
            if (!isspace(Line[pos - 1])) {
                // dsyslog("flatPlus:  Insert block at %ld", pos);
                Line.insert(pos, FillChars);
                InsertedFillChar += FillCharBlock;
                LineLength += FillCharsLength;  // Just add the length we inserted
            }
        }
#ifdef DEBUGFUNCSCALL
        dsyslog("   InsertedFillChar after first loop (space): %d", InsertedFillChar);
#endif

        //* Insert blocks at (.,?!;)
        pos = 0;  // Reset pos before the second loop
        for (pos = Line.find_first_of(kPunctuationChars);
             pos != std::string::npos && pos > 0 && ((InsertedFillChar + FillCharBlock) <= NeedFillChar);
             pos = Line.find_first_of(kPunctuationChars, pos + FillCharsLength + 1)) {
            if (pos < (LineLength - FillCharBlock - 1) && Line[pos] != Line[pos + 1]) {  // Next char is different
                // dsyslog("flatPlus:  Insert block at %ld", pos + 1);
                Line.insert(pos + 1, FillChars);
                InsertedFillChar += FillCharBlock;
                LineLength += FillCharsLength;  // Just add the length we inserted
            }
        }
#ifdef DEBUGFUNCSCALL
        dsyslog("   InsertedFillChar after second loop (.,?!;): %d", InsertedFillChar);
#endif

        //* Insert the remainder of 'NeedFillChar' from right to left
        pos = LineLength;  // Reset pos to the end of the string before the third loop
        std::size_t PrevPos {std::string::npos};
        while ((pos = Line.find_last_of(' ', pos - FillCharLength)) != std::string::npos &&
               pos < PrevPos &&                      // Ensure position is decreasing (potential infinite loop)
               (InsertedFillChar < NeedFillChar) &&  // Check if we still need to insert fill characters
               pos != LineLength - 1) {              // Do not insert at last position of line
            PrevPos = pos;
            if (pos > 0 && !(isspace(Line[pos - 1]))) {
                // dsyslog("flatPlus:  Insert char at %ld", pos);
                Line.insert(pos, FillChar);
                ++InsertedFillChar;
            }
        }
#ifdef DEBUGFUNCSCALL
        if (InsertedFillChar < NeedFillChar)
            dsyslog("   FillChar not inserted!: %d", NeedFillChar - InsertedFillChar);
        else
            dsyslog("   InsertedFillChar after third loop (space): %d", InsertedFillChar);
#endif
    } else {
        // dsyslog("flatPlus: JustifyLine() Line too short for justifying: LineWidth %d, LineMaxWidth * 0.8: %.0f",
        //        LineWidth, LineMaxWidth * 0.8);
    }
#ifdef DEBUGFUNCSCALL
    if (Timer.Elapsed() > 0) dsyslog("   Time: %ld ms", Timer.Elapsed());
#endif
}

std::string_view ltrim(std::string_view str) {
    const auto pos(str.find_first_not_of(" \t\n\r\f\v"));
    if (pos == std::string_view::npos) {
        // String contains only whitespace, return empty string_view
        return std::string_view {};
    }
    str.remove_prefix(std::min(pos, str.length()));
    return str;
}

std::string_view rtrim(std::string_view str) {
    const auto pos(str.find_last_not_of(" \t\n\r\f\v"));
    if (pos == std::string_view::npos) {
        // String contains only whitespace, return empty string_view
        return std::string_view {};
    }
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
cTextFloatingWrapper::cTextFloatingWrapper() {}

cTextFloatingWrapper::~cTextFloatingWrapper() {
    if (m_Text) {
        free(m_Text);
        m_Text = nullptr;
    }
}

void cTextFloatingWrapper::Set(const char *Text, const cFont *Font, int WidthLower, int UpperLines, int WidthUpper) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextFloatingWrapper::Set() Text length: %ld", strlen(Text));
    cTimeMs Timer;  // Start timer
#endif

    if (!Text || WidthUpper < 0 || WidthLower <= 0 || UpperLines < 0) return;

    free(m_Text);      // Free previous text if any
    m_Text = nullptr;  // Reset pointer to avoid dangling pointer
    m_Lines = 0;       // Reset line count

    // Estimate needed size of buffer including space for '\n' and '\0'
    const std::size_t TextLen {strlen(Text)};
    if (TextLen == 0) return;  // Avoid processing empty text
    // Estimate number of lines. More conservative size estimation
    const std::size_t EstimatedLines = (TextLen / 10) + UpperLines + 10;  // Add safety margin
    std::size_t Capacity {TextLen + EstimatedLines + 2};
#ifdef DEBUGFUNCSCALL
    dsyslog("   TextLen: %ld, EstimatedLines: %ld, Capacity: %ld", TextLen, EstimatedLines, Capacity);
#endif

    // Allocate buffer
    m_Text = static_cast<char *>(malloc(Capacity));
    if (!m_Text) return;

    // Copy text to buffer. Use memcpy instead of strncpy to avoid potential buffer overflow
    memcpy(m_Text, Text, TextLen);
    m_Text[TextLen] = '\0';

    m_Lines = 1;
    static const char *const kDelimiterChars {"-.,:;!?_~"};
    std::size_t CurLength {TextLen};  // Current length of the text
    char *Blank {nullptr}, *Delim {nullptr}, *NewText {nullptr};
    int16_t cw {0}, l {0}, sl {0}, w {0};
    int16_t Width = (UpperLines > 0) ? WidthUpper : WidthLower;
    uint32_t sym {0};
    stripspace(m_Text);  // Strips trailing newlines
    for (char *p {m_Text}; *p;) {
        /* int */ sl = Utf8CharLen(p);
        /* uint32_t */ sym = Utf8CharGet(p, sl);
        if (sym == '\n') {
            if (++m_Lines > UpperLines) Width = WidthLower;
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
            } else if (w > 0) {            // There has to be at least one character before the newline.
                                           // Here's the ugly part, where we don't have any whitespace to
                                           // punch in a newline, so we need to make room for it:
                if (Delim) p = Delim + 1;  // Let's fall back to the most recent delimiter

                l = p - m_Text;  // Calculate offset of current position
                // Instead of realloc per line, only grow buffer in powers of two
                if (CurLength + 2 > Capacity) {
                    Capacity = std::max(Capacity * 2, CurLength + 8);
                    NewText = static_cast<char *>(realloc(m_Text, Capacity));
                    if (!NewText) {
                        esyslog("flatPlus: cTextFloatingWrapper::Set() realloc failed");
                        return;
                    }
                    m_Text = NewText;
                    p = m_Text + l;
                }
                memmove(m_Text + l + 1, p, CurLength - l + 1);
                m_Text[l] = '\n';
                ++CurLength;
                continue;
            }
        }
        w += cw;
        if (strchr(kDelimiterChars, *p)) {
            if (*p != *(p + 1)) {  // Avoid breaks between repeated delimiters (like in "...")
                Delim = p;
                Blank = nullptr;
#ifdef DEBUGFUNCSCALL
            } else {
                dsyslog("   Skipping repeating delimiter char '%c'", *p);
#endif
            }
        }
        p += sl;
    }  // for char
#ifdef DEBUGFUNCSCALL
    if (Timer.Elapsed() > 0) dsyslog("   Time: %ld ms", Timer.Elapsed());
#endif
}

const char *cTextFloatingWrapper::Text() {
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
            if (Line == m_LastLine + 1) s = m_EoL + 1;
            m_EoL = nullptr;
        }
        if (!s) {
            s = m_Text;
            for (int16_t i {0}; i < Line; ++i) {
                s = strchr(s, '\n');
                if (s)
                    s++;
                else
                    break;
            }
        }
        if (s) {
            if ((m_EoL = strchr(s, '\n')) != nullptr) *m_EoL = 0;
        }
        m_LastLine = Line;
    }
    return s;
}
