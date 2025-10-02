/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <atomic>
#include <ctime>
// #include <string>
#include <vector>

#include "./imageloader.h"
#include "./flat.h"
#include "./textscroller.h"
#include "./complexcontent.h"

enum eBorder {
    BorderMenuItem,
    BorderRecordJump,
    BorderMenuRecord,
    BorderMessage,
    BorderButton,
    BorderContent,
    BorderTVSPoster,
    BorderSetRecording,
    BorderMMWidget
};

struct sDecorBorder {
    int Left {0}, Top {0}, Width {0}, Height {0}, Size {0}, Type {0};
    tColor ColorFg {clrTransparent}, ColorBg {clrTransparent};
    int From {0};
};

// Font/image caches for session-level reuse (Weather Widget + others)
class FontImageWeatherCache {
 public:
    static const int kMaxDays {8};   // Source provides up to 8 days of weather data
    struct WeatherDayData {
        cString Icon {""};           // Weather icon
        cString TempMax {""};        // Max temperature
        cString TempMin {""};        // Min temperature
        cString Precipitation {""};  // Precipitation
        cString Summary {""};        // Summary
    } Days[kMaxDays];                // Array of weather data for multiple days

    cString Location {""};           // Location name
    cString Temp {""};               // Actual temperature for day 0 only
    cString TempTodaySign {""};      // Temperature sign for today
    time_t LastReadMTime {0};

    cFont* WeatherFont {nullptr};
    cFont* WeatherFontSml {nullptr};
    cFont* WeatherFontSign {nullptr};

    int FontHeight {0};
    int FontSmlHeight {0};
    int FontSignHeight {0};

    int FontAscender {0}, FontSignAscender {0};  // Ascender for font and sign

    int TempTodaySignWidth {0};

    bool valid {false};  // Indicates if the cache is valid

    FontImageWeatherCache() = default;  // Constructor
    void Clear() { valid = false; }
};  // class FontImageWeatherCache
static FontImageWeatherCache WeatherCache;

// Base class for rendering OSD elements in the flatPlus skin
// Provides methods for creating and managing OSD elements like top bar, buttons, messages, progress
class cFlatBaseRender {
 public:
    cImageLoader ImgLoader;

    cFlatBaseRender();
    ~cFlatBaseRender();

    void CreateFullOsd();
    void CreateOsd(int Left, int Top, int Width, int Height);

    void TopBarCreate();
    void TopBarSetTitle(const cString &Title, bool Clear = true);
    void TopBarSetTitleExtra(const cString &Extra1, const cString &Extra2);
    void TopBarSetMenuIcon(const cString &icon);
    void TopBarSetMenuIconRight(const cString &icon);
    void TopBarClearMenuIconRight();
    void TopBarSetMenuLogo(const cString &icon);
    void TopBarSetExtraIcon(const cString &icon);
    void TopBarUpdate();

    void ButtonsCreate();
    void ButtonsSet(const char *Red, const char *Green = nullptr, const char *Yellow = nullptr,
                    const char *Blue = nullptr);
    bool ButtonsDrawn() const;

    void MessageCreate();
    void MessageSet(eMessageType Type, const char *Text);
    void MessageSetExtraTime(const char *Text);
    void MessageClear();

    void ProgressBarDrawRaw(cPixmap *Pixmap, cPixmap *PixmapBg, const cRect &rec, const cRect &recBg, int Current,
                            int Total, tColor ColorFg, tColor ColorBarFg, tColor ColorBg, int Type,
                            bool SetBackground, bool IsSignal = false);
    void ProgressBarCreate(const cRect &Rect, int MarginHor, int MarginVer, tColor ColorFg, tColor ColorBarFg,
                            tColor ColorBg, int Type, bool SetBackground = false, bool IsSignal = false);
    void ProgressBarDrawBgColor() const;
    void ProgressBarDraw(int Current, int Total);
#if APIVERSNUM >= 30004
    /**
     * Draws marks on the progress bar with error indicators
     * @param Errors Error markers to display (added in API v3.0.4)
     */
    void ProgressBarDrawMarks(int Current, int Total, const cMarks *Marks, const cErrors *Errors, tColor Color,
                                tColor ColorCurrent);
#else
    /**
     * Draws marks on the progress bar (legacy version)
     * Note: Error indicators not supported in API versions before 3.0.4
     */
    void ProgressBarDrawMarks(int Current, int Total, const cMarks *Marks, tColor Color, tColor ColorCurrent);
#endif
    void ScrollbarDraw(cPixmap *Pixmap, int Left, int Top, int Height, int Total, int Offset,
                        int Shown, bool CanScrollUp, bool CanScrollDown);
    int ScrollBarWidth() const;

    void DecorBorderDraw(const sDecorBorder &ib, bool Store = true);
    void DecorBorderClear(const cRect &Rect, int Size);
    void DecorBorderClearAll() const;
    void DecorBorderRedrawAll();
    void DecorBorderClearByFrom(int From);

    cString ReadAndExtractData(const cString &FilePath) const;  // Read file and return its content as cString
    bool BatchReadWeatherData(FontImageWeatherCache &out, time_t &out_latest_time);  // Read weather data  // NOLINT

    void DrawWidgetWeather();
    void DrawTextWithShadow(cPixmap *pixmap, const cPoint &pos, const char *text, tColor TextColor,
                            tColor ShadowColor, const cFont *font, int ShadowSize = 3, int xOffset = 1,
                            int yOffset = 1);

 protected:
    cOsd *m_Osd {nullptr};
    int m_OsdLeft {0}, m_OsdTop {0}, m_OsdWidth {0}, m_OsdHeight {0};
    int m_MarginItem {0}, m_MarginItem2 {0}, m_MarginItem3 {0};  // Margins for items in the OSD
    int m_MarginEPGImage {20};                                   // Margin for EPG image
    int m_LineWidth {0}, m_LineMargin {0};                       // Line width and margin for lines in the OSD

    static constexpr int kIconMaxSize {999};   // Max icon width or height (999)

    // Standard fonts
    cFont *m_Font {nullptr};
    cFont *m_FontSml {nullptr};
    cFont *m_FontFixed {nullptr};

    cString m_FontName {""};
    cString m_FontSmlName {""};

    int m_FontHeight {0}, m_FontHeight2 {0};
    int m_FontSmlHeight {0};
    int m_FontFixedHeight {0};
    int m_FontAscender {0};  // Ascender for font

    cFont *m_FontBig {nullptr};      // Big font for channel name in displaychannel.c
    cFont *m_FontMedium {nullptr};   // Font in Size between m_Font and m_FontSml
    cFont *m_FontTempSml {nullptr};  // Font for main menu weather widget
    cFont *m_FontTiny {nullptr};     // Very small font for actor name and role

    cString m_FontTempSmlName {""};

    int m_FontBigHeight {0};
    int m_FontTempSmlHeight {0};
    int m_FontTinyHeight {0};

    // TopBar
    cPixmap *TopBarPixmap {nullptr};
    cPixmap *TopBarIconPixmap {nullptr};
    cPixmap *TopBarIconBgPixmap {nullptr};
    cFont *m_TopBarFont {nullptr}, *m_TopBarFontSml {nullptr}, *m_TopBarFontClock {nullptr};  // Based on Setup.FontOsd
    int m_TopBarFontHeight {0}, m_TopBarFontSmlHeight {0}, m_TopBarFontClockHeight {0};

    cString m_TopBarTitle {""};
    cString m_TopBarTitleExtra1 {""}, m_TopBarTitleExtra2 {""};
    cString m_TopBarExtraIcon {""};
    bool m_TopBarExtraIconSet {false};
    cString m_TopBarMenuIcon {""};
    bool m_TopBarMenuIconSet {false};
    cString m_TopBarMenuIconRight {""};
    bool m_TopBarMenuIconRightSet {false};
    cString m_TopBarMenuLogo {""};
    bool m_TopBarMenuLogoSet {false};

    bool m_TopBarUpdateTitle {false};
    time_t m_TopBarLastDate {0};
    int m_TopBarHeight {0};
    int m_VideoDiskUsageState {-1};

    // Progressbar
    cPixmap *ProgressBarMarkerPixmap {nullptr};  // Draw marker on top
    cPixmap *ProgressBarPixmap {nullptr};        // Draw errors here
    cPixmap *ProgressBarPixmapBg {nullptr};
    int m_ProgressBarHeight {0}, m_ProgressBarTop {0}, m_ProgressBarWidth {0};
    int m_ProgressBarMarginHor {0}, m_ProgressBarMarginVer {0};
    int m_ProgressType {0};
    bool m_ProgressBarSetBackground {false};
    bool m_ProgressBarIsSignal {false};
    tColor m_ProgressBarColorFg {0}, m_ProgressBarColorBarFg {0};
    tColor m_ProgressBarColorBarCurFg {0}, m_ProgressBarColorBg {0};
    tColor m_ProgressBarColorMark {0}, m_ProgressBarColorMarkCurrent {0};

    // Scrollbar
    int m_ScrollBarWidth {0};

    // Buttons red, green, yellow, blue
    cPixmap *ButtonsPixmap {nullptr};
    int m_ButtonsWidth {0}, m_ButtonsHeight {0}, m_ButtonsTop {0};
    int m_MarginButtonColor {10}, m_ButtonColorHeight {8};  // Margin and height for button color bar
    bool m_ButtonsDrawn {false};

    // Message
    cPixmap *MessagePixmap {nullptr}, *MessageIconPixmap {nullptr};
    int m_MessageHeight {0};
    cTextScrollers MessageScroller;
    int m_OSDMessageTime {0};  // Backup for Setup.OSDMessageTime

    cComplexContent WeatherWidget;

    cPixmap *DecorPixmap {nullptr};
    std::vector<sDecorBorder> Borders;  // For clearing specific borders (Clear only 'MenuItems' and not 'TopBar')

    void ProgressBarDrawMark(int PosMark, int PosMarkLast, int PosCurrent, bool Start, bool IsCurrent);
#if APIVERSNUM >= 30004
    void ProgressBarDrawError(int Pos, int SmallLine, bool IsCurrent);
#endif

    void DecorDrawGlowRectHor(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);
    void DecorDrawGlowRectVer(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);

    void DecorDrawGlowRectTL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);
    void DecorDrawGlowRectTR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);
    void DecorDrawGlowRectBL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);
    void DecorDrawGlowRectBR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);

    void DecorDrawGlowEllipseTL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                int type);
    void DecorDrawGlowEllipseTR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                int type);
    void DecorDrawGlowEllipseBL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                int type);
    void DecorDrawGlowEllipseBR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                int type);

    void SetMargins(int Width, int Height);
    void TopBarEnableDiskUsage();
    // tColor Multiply(tColor Color, uint8_t Alpha);
    tColor SetAlpha(tColor Color, double am);
};  // class cFlatBaseRender

// Recording Timer Count: Cache with event update or periodic refresh
static std::atomic<uint16_t> s_NumRecordings {0};

class RecTimerCounter {
 public:
    std::atomic<time_t> LastUpdate;  // Last time the count was updated
    static constexpr int kUpdateIntervalSec {2};
    RecTimerCounter() : LastUpdate(0) {}  // Constructor

    void UpdateIfNeeded() {
        time_t now {time(0)};
        if (now - LastUpdate.load() >= kUpdateIntervalSec) {
            uint16_t count {0};
            { LOCK_TIMERS_READ;  // Creates local const cTimers *Timers
                for (const cTimer* Timer=Timers->First(); Timer; Timer = Timers->Next(Timer)) {
                    if (Timer->HasFlags(tfRecording)) ++count;
                }
            }
            s_NumRecordings.store(count, std::memory_order_relaxed);
            LastUpdate = now;
        }
    }
};  // class RecTimerCounter
static RecTimerCounter RecCountCache;
