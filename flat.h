/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/plugin.h>
#include <vdr/skins.h>
#include <vdr/videodir.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// #include <stdint.h>

#include <memory>   // For 'unique_ptr<T>()' ...
#include <cstring>  // string.h
#include <cstdint>  // stdint.h
#include <string>
#include <string_view>
#include <random>
#include <filesystem>  // C++17

// Includes and defines for assert()
#include <iostream>
// uncomment to disable assert()
// #define NDEBUG
#include <cassert>

// Use (void) to silence unused warnings.
#define assertm(exp, msg) assert(((void)msg, exp))
// Includes and defines for assert()

#include "./services/scraper2vdr.h"

#include "./config.h"
#include "./imagecache.h"

extern class cFlatConfig Config;
extern class cImageCache ImgCache;

// extern time_t m_RemoteTimersLastRefresh;

class cFlatDisplayMenu;
extern cTheme Theme;

// BUTTONS
#define CLR_BUTTONRED                       0xFFBB0000
#define CLR_BUTTONGREEN                     0xFF00BB00
#define CLR_BUTTONYELLOW                    0xFFBBBB00
#define CLR_BUTTONBLUE                      0xFF0000BB

// MESSAGES
#define CLR_MESSAGESTATUS                   0xFF0000FF
#define CLR_MESSAGEINFO                     0xFF009900
#define CLR_MESSAGEWARNING                  0xFFBBBB00
#define CLR_MESSAGEERROR                    0xFFBB0000

// TopBar
THEME_CLR(Theme, clrTopBarBg,               0xF0202020);
THEME_CLR(Theme, clrTopBarFont,             0xFFEEEEEE);
THEME_CLR(Theme, clrTopBarTimeFont,         0xFFEEEEEE);
THEME_CLR(Theme, clrTopBarDateFont,         0xFF909090);
THEME_CLR(Theme, clrTopBarBorderFg,         0xF0202020);
THEME_CLR(Theme, clrTopBarBorderBg,         0xF0202020);

THEME_CLR(Theme, clrTopBarRecordingActiveFg,  0xFF880000);
THEME_CLR(Theme, clrTopBarRecordingActiveBg,  0xF0202020);

THEME_CLR(Theme, clrTopBarConflictLowFg,      0xFFBBBB00);
THEME_CLR(Theme, clrTopBarConflictLowBg,      0xF0202020);
THEME_CLR(Theme, clrTopBarConflictHighFg,     0xFF880000);
THEME_CLR(Theme, clrTopBarConflictHighBg,    0xF0202020);

// Buttons
THEME_CLR(Theme, clrButtonBg,               0xF0202020);
THEME_CLR(Theme, clrButtonFont,             0xFFEEEEEE);
THEME_CLR(Theme, clrButtonRed,              CLR_BUTTONRED);
THEME_CLR(Theme, clrButtonGreen,            CLR_BUTTONGREEN);
THEME_CLR(Theme, clrButtonYellow,           CLR_BUTTONYELLOW);
THEME_CLR(Theme, clrButtonBlue,             CLR_BUTTONBLUE);

THEME_CLR(Theme, clrButtonBorderFg,        0xF0202020);
THEME_CLR(Theme, clrButtonBorderBg,        0xF0202020);

// Messages
THEME_CLR(Theme, clrMessageBg,              0xF0202020);
THEME_CLR(Theme, clrMessageFont,            0xFFEEEEEE);

THEME_CLR(Theme, clrMessageStatus,          CLR_MESSAGESTATUS);
THEME_CLR(Theme, clrMessageInfo,            CLR_MESSAGEINFO);
THEME_CLR(Theme, clrMessageWarning,         CLR_MESSAGEWARNING);
THEME_CLR(Theme, clrMessageError,           CLR_MESSAGEERROR);

THEME_CLR(Theme, clrMessageBorderFg,        0xF0202020);
THEME_CLR(Theme, clrMessageBorderBg,        0xF0202020);

// Channel
THEME_CLR(Theme, clrChannelBg,              0xF0202020);
THEME_CLR(Theme, clrChannelFontTitle,       0xFF03A9F4);
THEME_CLR(Theme, clrChannelFontEpg,         0xFFEEEEEE);
THEME_CLR(Theme, clrChannelFontEpgFollow,   0xFF909090);
THEME_CLR(Theme, clrChannelProgressFg,      0xFF03A9F4);
THEME_CLR(Theme, clrChannelProgressBarFg,   0xFF03A9F4);
THEME_CLR(Theme, clrChannelProgressBg,      0xF0202020);

THEME_CLR(Theme, clrChannelBorderFg,        0xF003A9F4);
THEME_CLR(Theme, clrChannelBorderBg,        0xF003A9F4);

THEME_CLR(Theme, clrChannelEPGBorderFg,        0xF003A9F4);
THEME_CLR(Theme, clrChannelEPGBorderBg,        0xF003A9F4);

THEME_CLR(Theme, clrChannelSignalFont,            0xFF909090);
THEME_CLR(Theme, clrChannelSignalProgressFg,      0xFF909090);
THEME_CLR(Theme, clrChannelSignalProgressBarFg,   0xFF909090);
THEME_CLR(Theme, clrChannelSignalProgressBg,      0xF0202020);

THEME_CLR(Theme, clrChannelRecordingPresentFg,    0xFFAA0000);
THEME_CLR(Theme, clrChannelRecordingPresentBg,    0xF0202020);
THEME_CLR(Theme, clrChannelRecordingFollowFg,     0xFF909090);
THEME_CLR(Theme, clrChannelRecordingFollowBg,     0xF0202020);

// Menu
THEME_CLR(Theme, clrItemBg,                 0xF0909090);
THEME_CLR(Theme, clrItemFont,               0xFFEEEEEE);
THEME_CLR(Theme, clrItemCurrentBg,          0xF003A9F4);
THEME_CLR(Theme, clrItemCurrentFont,        0xFFEEEEEE);
THEME_CLR(Theme, clrItemSelableBg,          0xF0202020);
THEME_CLR(Theme, clrItemSelableFont,        0xFFEEEEEE);
THEME_CLR(Theme, clrScrollbarFg,            0xFF03A9F4);
THEME_CLR(Theme, clrScrollbarBarFg,         0xFF03A9F4);
THEME_CLR(Theme, clrScrollbarBg,            0xF0202020);
// Menu Event
THEME_CLR(Theme, clrMenuEventBg,            0xF0202020);
THEME_CLR(Theme, clrMenuEventFontTitle,     0xFF03A9F4);
THEME_CLR(Theme, clrMenuEventTitleLine,     0xFF03A9F4);
THEME_CLR(Theme, clrMenuEventFontInfo,      0xFFEEEEEE);
// Menu Recording
THEME_CLR(Theme, clrMenuRecBg,              0xF0202020);
THEME_CLR(Theme, clrMenuRecFontTitle,       0xFF03A9F4);
THEME_CLR(Theme, clrMenuRecTitleLine,       0xFF03A9F4);
THEME_CLR(Theme, clrMenuRecFontInfo,        0xFFEEEEEE);
// Menu Text (Multiline)
THEME_CLR(Theme, clrMenuTextBg,             0xF0202020);
THEME_CLR(Theme, clrMenuTextFont,           0xFFEEEEEE);
THEME_CLR(Theme, clrMenuTextFixedFont,      0xFFEEEEEE);

THEME_CLR(Theme, clrMenuContentHeadBorderFg,        0xF003A9F4);
THEME_CLR(Theme, clrMenuContentHeadBorderBg,        0xF003A9F4);

THEME_CLR(Theme, clrMenuContentBorderFg,        0xF003A9F4);
THEME_CLR(Theme, clrMenuContentBorderBg,        0xF003A9F4);

// Menu Items
THEME_CLR(Theme, clrMenuItemProgressFg,      0xFFEEEEEE);
THEME_CLR(Theme, clrMenuItemProgressBarFg,   0xFFEEEEEE);
THEME_CLR(Theme, clrMenuItemProgressBg,      0xF0202020);
THEME_CLR(Theme, clrMenuItemCurProgressFg,      0xFFEEEEEE);
THEME_CLR(Theme, clrMenuItemCurProgressBarFg,   0xFFEEEEEE);
THEME_CLR(Theme, clrMenuItemCurProgressBg,      0xF003A9F4);

THEME_CLR(Theme, clrMenuItemBorderFg,      0xF0909090);
THEME_CLR(Theme, clrMenuItemBorderBg,      0xF0909090);
THEME_CLR(Theme, clrMenuItemSelableBorderFg,      0xF0202020);
THEME_CLR(Theme, clrMenuItemSelableBorderBg,      0xF0202020);
THEME_CLR(Theme, clrMenuItemCurrentBorderFg,      0xF003A9F4);
THEME_CLR(Theme, clrMenuItemCurrentBorderBg,      0xF003A9F4);

// Menu Timer Item
THEME_CLR(Theme, clrMenuTimerItemDisabledFont,       0xFF909090);
THEME_CLR(Theme, clrMenuTimerItemRecordingFont,      0xFFEEEEEE);

// For Tilde, Timer Extra, Program Short Text
THEME_CLR(Theme, clrMenuItemExtraTextFont,    0xFF909090);
THEME_CLR(Theme, clrMenuItemExtraTextCurrentFont,    0xFF909090);

// Replay
THEME_CLR(Theme, clrReplayBg,               0xF0202020);
THEME_CLR(Theme, clrReplayFont,             0xFFEEEEEE);
THEME_CLR(Theme, clrReplayFontSpeed,        0xFF03A9F4);
THEME_CLR(Theme, clrReplayProgressFg,       0xFFEEEEEE);
THEME_CLR(Theme, clrReplayProgressBarFg,    0xFFEEEEEE);
THEME_CLR(Theme, clrReplayProgressBarCurFg, 0xFF03A9F4);
THEME_CLR(Theme, clrReplayProgressBg,       0xF0202020);
THEME_CLR(Theme, clrReplayMarkFg,           0xFFEEEEEE);
THEME_CLR(Theme, clrReplayMarkCurrentFg,    0xFF03A9F4);
THEME_CLR(Theme, clrReplayErrorMark,            clrRed);  // Alternative 0xFFBB0000

THEME_CLR(Theme, clrReplayBorderFg,         0xF0202020);
THEME_CLR(Theme, clrReplayBorderBg,         0xF0202020);

// Tracks
THEME_CLR(Theme, clrTrackItemBg,            0xF0909090);
THEME_CLR(Theme, clrTrackItemFont,          0xFFEEEEEE);
THEME_CLR(Theme, clrTrackItemSelableBg,     0xF0202020);
THEME_CLR(Theme, clrTrackItemSelableFont,   0xFFEEEEEE);
THEME_CLR(Theme, clrTrackItemCurrentBg,     0xF003A9F4);
THEME_CLR(Theme, clrTrackItemCurrentFont,   0xFFEEEEEE);

THEME_CLR(Theme, clrTrackItemBorderFg,      0xF0909090);
THEME_CLR(Theme, clrTrackItemBorderBg,      0xF0909090);
THEME_CLR(Theme, clrTrackItemSelableBorderFg,      0xF0202020);
THEME_CLR(Theme, clrTrackItemSelableBorderBg,      0xF0202020);
THEME_CLR(Theme, clrTrackItemCurrentBorderFg,      0xF003A9F4);
THEME_CLR(Theme, clrTrackItemCurrentBorderBg,      0xF003A9F4);

// Volume
THEME_CLR(Theme, clrVolumeBg,               0xF0202020);
THEME_CLR(Theme, clrVolumeFont,             0xFFEEEEEE);
THEME_CLR(Theme, clrVolumeProgressFg,       0xFF03A9F4);
THEME_CLR(Theme, clrVolumeProgressBarFg,    0xFF03A9F4);
THEME_CLR(Theme, clrVolumeProgressBg,       0xF0202020);

THEME_CLR(Theme, clrVolumeBorderFg,         0xF0202020);
THEME_CLR(Theme, clrVolumeBorderBg,         0xF0202020);

// SeenIconNames for GetRecordingSeenIcon()
static const cString SeenIconNames[] {"recording_seen_0", "recording_seen_1", "recording_seen_2", "recording_seen_3",
                                      "recording_seen_4", "recording_seen_5", "recording_seen_6", "recording_seen_7",
                                      "recording_seen_8", "recording_seen_9", "recording_seen_10"};

class cFlat : public cSkin {
 public:
    cFlat();
    const char *Description() override;
    cSkinDisplayChannel *DisplayChannel(bool WithInfo) override;
    cSkinDisplayMenu *DisplayMenu() override;
    cSkinDisplayReplay *DisplayReplay(bool ModeOnly) override;
    cSkinDisplayVolume *DisplayVolume() override;
    cSkinDisplayTracks *DisplayTracks(const char *Title, int NumTracks, const char * const *Tracks) override;
    cSkinDisplayMessage *DisplayMessage() override;

 private:
    cFlatDisplayMenu *Display_Menu;  // Using _ to avoid name conflict with DisplayMenu()
};

// Based on VDR's cTextWrapper
class cTextFloatingWrapper {
 public:
    cTextFloatingWrapper();
    ~cTextFloatingWrapper();
    ///< Wraps the Text to make it fit into the area defined by the given Width when displayed with the given Font.
    ///< Wrapping is done by inserting the necessary number of newline characters into the string.
    ///< When 'UpperLines' and 'WidthUpper' are set the 'UpperLines' are wrapped to fit in 'WidthUpper'.
    void Set(const char *Text, const cFont *Font, int WidthLower, int UpperLines = 0, int WidthUpper = 0);
    ///< Returns the full wrapped text.
    const char *Text();
    ///< Returns the actual number of lines needed to display the full wrapped text.
    int Lines() const { return m_Lines; }
    ///< Returns the given Line. The first line is numbered 0.
    const char *GetLine(int Line);

 private:
    char *m_Text {nullptr};
    char *m_EoL {nullptr};
    int m_Lines {0};
    int m_LastLine {-1};
};

cPixmap *CreatePixmap(cOsd *osd, const cString Name, int Layer = 0, const cRect &ViewPort = cRect::Null,
                      const cRect &DrawPort = cRect::Null);
inline void PixmapFill(cPixmap *Pixmap, tColor Color) {
    if (Pixmap) Pixmap->Fill(Color);
}
inline void PixmapClear(cPixmap *Pixmap) {
    if (Pixmap) Pixmap->Clear();
}
/**
 * Sets the alpha value of the given `cPixmap` object.
 *
 * @param Pixmap - A pointer to the `cPixmap` object.
 * @param Alpha - The alpha value to be set (0-255, where 0 represents full transparency).
 */
inline void PixmapSetAlpha(cPixmap *Pixmap, int Alpha) {
    if (Pixmap) Pixmap->SetAlpha(Alpha);  // 0-255 (0 = Full transparent)
}

void JustifyLine(std::string &Line, const cFont *Font, const int LineMaxWidth);  // NOLINT
uint32_t GetGlyphSize(const char *Name, const FT_ULong CharCode, const int FontHeight = 8);

static cPlugin *GetScraperPlugin();
void GetScraperMedia(cString &MediaPath, cString &SeriesInfo, cString &MovieInfo,         // NOLINT
    std::vector<cString> &ActorsPath, std::vector<cString> &ActorsName,  // NOLINT
    std::vector<cString> &ActorsRole, const cEvent *Event = nullptr,     // NOLINT
    const cRecording *Recording = nullptr);                              // NOLINT
int GetScraperMediaTypeSize(cString &MediaPath, cSize &MediaSize, const cEvent *Event = nullptr, const cRecording *Recording = nullptr);  // NOLINT

void InsertSeriesInfos(const cSeries &Series, cString &SeriesInfo);  // NOLINT
void InsertMovieInfos(const cMovie &Movie, cString &MovieInfo);      // NOLINT


cString GetAspectIcon(int ScreenWidth, double ScreenAspect);
cString GetScreenResolutionIcon(int ScreenWidth, int ScreenHeight);
cString GetFormatIcon(int ScreenWidth);
cString GetRecordingFormatIcon(const cRecording *Recording);
cString GetCurrentAudioIcon();
cString GetRecordingErrorIcon(int RecInfoErrors);
cString GetRecordingSeenIcon(int FrameTotal, int FrameResume);

std::string_view ltrim(std::string_view str);
std::string_view rtrim(std::string_view str);
std::string_view trim(std::string_view str);

void SetMediaSize(cSize &MediaSize, const cSize &ContentSize);  // NOLINT
void InsertComponents(const cComponents *Components, cString &Text, cString &Audio,        // NOLINT
                      cString &Subtitle, bool NewLine = false);                            // NOLINT
void InsertAuxInfos(const cRecordingInfo *RecInfo, cString &Text, bool InfoLine = false);  // NOLINT
int GetEpgsearchConflicts();
int GetFrameAfterEdit(const cMarks *marks = nullptr, int Frame = 0, int LastFrame = 0);
void InsertCutLengthSize(const cRecording *Recording, cString &Text);  // NOLINT
std::string XmlSubstring(const std::string &source, const char* StrStart, const char* StrEnd);
