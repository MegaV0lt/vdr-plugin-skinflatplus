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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <cstring>  // string.h
#include <random>

#include "./config.h"
#include "./imagecache.h"

extern class cFlatConfig Config;
extern class cImageCache ImgCache;

extern time_t m_RemoteTimersLastRefresh;

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

class cFlat : public cSkin {
 private:
        cFlatDisplayMenu *Display_Menu;  // Using _ to avoid nameconflict with DisplayMenu()
 public:
        cFlat(void);
        virtual const char *Description(void);
        virtual cSkinDisplayChannel *DisplayChannel(bool WithInfo);
        virtual cSkinDisplayMenu *DisplayMenu(void);
        virtual cSkinDisplayReplay *DisplayReplay(bool ModeOnly);
        virtual cSkinDisplayVolume *DisplayVolume(void);
        virtual cSkinDisplayTracks *DisplayTracks(const char *Title, int NumTracks, const char * const *Tracks);
        virtual cSkinDisplayMessage *DisplayMessage(void);
};

// Based on VDR's cTextWrapper
class cTextFloatingWrapper {
 private:
    char *m_Text {nullptr};
    char *m_EoL {nullptr};
    int m_Lines {0};
    int m_LastLine {-1};

 public:
    cTextFloatingWrapper(void);
    ~cTextFloatingWrapper();
    void Set(const char *Text, const cFont *Font, int WidthLower, int UpperLines = 0, int WidthUpper = 0);
    ///< Wraps the Text to make it fit into the area defined by the given Width
    ///< when displayed with the given Font.
    ///< Wrapping is done by inserting the necessary number of newline
    ///< characters into the string.
    const char *Text(void);
    ///< Returns the full wrapped text.
    int Lines(void) { return m_Lines; }
    ///< Returns the actual number of lines needed to display the full wrapped text.
    const char *GetLine(int Line);
    ///< Returns the given Line. The first line is numbered 0.
};

static inline uint32_t GetMsTicks(void) {
#ifdef CLOCK_MONOTONIC
    struct timespec tspec;

    clock_gettime(CLOCK_MONOTONIC, &tspec);
    return (tspec.tv_sec * 1000) + (tspec.tv_nsec / (1000 * 1000));
#else
    struct timeval tval;

    if (gettimeofday(&tval, NULL) < 0)
        return 0;
    return (tval.tv_sec * 1000) + (tval.tv_usec / 1000);
#endif
}

cPixmap *CreatePixmap(cOsd *osd, cString Name = "", int Layer = 0, const cRect &ViewPort = cRect::Null,
                      const cRect &DrawPort = cRect::Null);
inline void PixmapFill(cPixmap *Pixmap, tColor Color) {
    if (Pixmap) Pixmap->Fill(Color);
}
inline void PixmapSetAlpha(cPixmap *Pixmap, int Alpha) {
    if (Pixmap) Pixmap->SetAlpha(Alpha);  // 0-255 (0 = Full transparent)
}

cPlugin *GetScraperPlugin(void);
cString GetAspectIcon(int ScreenWidth, double ScreenAspect);
cString GetScreenResolutionIcon(int ScreenWidth, int ScreenHeight, double ScreenAspect);
cString GetFormatIcon(int ScreenWidth);
cString GetRecordingFormatIcon(const cRecording *Recording);
cString GetRecordingerrorIcon(int RecInfoErrors);
cString GetRecordingseenIcon(int FrameTotal, int FrameResume);

inline void LeftTrim(std::string &s, const char *t = " \t\n\r\f\v") {  // NOLINT
    s.erase(0, s.find_first_not_of(t));  // Trim from left
    // return s;  // Only inplace trimming
}

inline void RightTrim(std::string &s, const char *t = " \t\n\r\f\v") {  // NOLINT
    s.erase(s.find_last_not_of(t) + 1);  // Trim from right
    // return s;  // Only inplace trimming
}

inline void trim(std::string &s, const char *t = " \t\n\r\f\v") {  // NOLINT
    LeftTrim(s, t);  // Trim from left & right
    RightTrim(s, t);
    /* return */  // LeftTrim(RightTrim(s, t), t);
}

void SetMediaSize(cSize &MediaSize, const cSize &TVSSize);

void InsertComponents(const cComponents *Components, cString &Text, cString &Audio,        // NOLINT
                      cString &Subtitle, bool NewLine = false);                            // NOLINT
void InsertAuxInfos(const cRecordingInfo *RecInfo, cString &Text, bool InfoLine = false);  // NOLINT
int GetEpgsearchConflichts(void);
bool GetCuttedLengthMarks(const cRecording *Recording, cString &Text, cString &Cutted, bool AddText);  // NOLINT
std::string XmlSubstring(const std::string &source, const char* StrStart, const char* StrEnd);

