#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <random>

#include <vdr/plugin.h>
#include <vdr/skins.h>
#include <vdr/videodir.h>

#include "config.h"
#include "imagecache.h"

extern class cFlatConfig Config;
extern class cImageCache imgCache;
extern bool firstDisplay;

extern time_t remoteTimersLastRefresh;

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
        cFlatDisplayMenu *displayMenu;
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

char *substr(char *string, int start, int end);
char *GetFilenameWithoutext(char *fullfilename);

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
inline void PixmapFill(cPixmap *pixmap, tColor Color) {
    if (pixmap) pixmap->Fill(Color);
}
cPlugin *GetScraperPlugin(void);
cString GetAspectIcon(int screenWidth, double screenAspect);
cString GetScreenResolutionIcon(int screenWidth, int screenHeight, double screenAspect);
cString GetFormatIcon(int screenWidth);
cString GetRecordingerrorIcon(int recInfoErrors);
cString GetRecordingseenIcon(int frameTotal, int frameResume);

// Trim from left
inline void ltrim(std::string &s, const char *t = " \t\n\r\f\v") {
    s.erase(0, s.find_first_not_of(t));
    // return s;  // Only inplace trimming
}

// Trim from right
inline void rtrim(std::string &s, const char *t = " \t\n\r\f\v") {
    s.erase(s.find_last_not_of(t) + 1);
    // return s;  // Only inplace trimming
}

// Trim from left & right
inline void trim(std::string &s, const char *t = " \t\n\r\f\v") {
    /* return */ // ltrim(rtrim(s, t), t);
    ltrim(s, t);
    rtrim(s, t);
}

void GetComponents(const cComponents *Components, cString &Text, cString &Audio, cString &Subtitle, bool NewLine = false);
