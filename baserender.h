/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <list>

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

class cFlatBaseRender {
 protected:
        cOsd *m_Osd {nullptr};

        int m_OsdLeft {0}, m_OsdTop {0}, m_OsdWidth {0}, m_OsdHeight {0};
        const int m_MarginItem {5}, m_MarginItem2 {10};

        // Standard fonts
        cFont *m_Font {nullptr};
        cFont *m_FontSml {nullptr};
        cFont *m_FontFixed {nullptr};
        int m_FontHeight {0};
        int m_FontHeight2 {0};
        int m_FontSmlHeight {0};
        int m_FontFixedHeight {0};

        // Font for main menu weather widget
        cFont *m_FontTempSml {nullptr};

        // Very small font for actor name and role
        cFont *m_FontTiny {nullptr};

        // TopBar
        cPixmap *TopBarPixmap {nullptr};
        cPixmap *TopBarIconPixmap {nullptr};
        cPixmap *TopBarIconBgPixmap {nullptr};
        cFont *m_TopBarFont {nullptr}, *m_TopBarFontSml {nullptr}, *m_TopBarFontClock {nullptr};
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
        cString m_TopBarLastDate {""};
        int m_TopBarHeight {0};
        int m_VideoDiskUsageState {-1};

        // Progressbar
        cPixmap *ProgressBarPixmap {nullptr};
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
        int m_MarginButtonColor {0}, m_ButtonColorHeight {0};
        bool m_ButtonsDrawn {false};

        // Message
        cPixmap *MessagePixmap {nullptr}, *MessageIconPixmap {nullptr};
        int m_MessageHeight {0};
        cTextScrollers MessageScroller;
        int m_OSDMessageTime {0};  // Backup for Setup.OSDMessageTime

        // Multiline content with scrollbar
        // cPixmap *ContentPixmap;  //* Content* is unused
        // cPixmap *ContentEpgImagePixmap;
        // int ContentLeft, ContentTop, ContentHeight, ContentWidth;
        // int ContentDrawPortHeight;  // Complete high of text
        // int ContentTextHeight;
        // bool ContentHasScrollbar;
        // bool ContentShown;
        // int ContentFontType;
        // int ContentEventType;
        // int ContentEventHeight;
        // int ContentEventPosterWidth, ContentEventPosterHeight;

        // tColor ContentColorFg, ContentColorBg;
        // cTextWrapper ContentWrapper;
        // cTextWrapper ContentWrapperPoster;
        // const cEvent *ContentEvent;

        cComplexContent WeatherWidget;

        cPixmap *DecorPixmap {nullptr};
        std::vector<sDecorBorder> Borders;  // For clearing specific borders (Clear only 'MenuItems' and not 'TopBar')

        /* void ContentDraw(void);  // Unused
        void ContentEventDraw(void); */
        double ScrollbarSize(void);

        void ProgressBarDrawMark(int PosMark, int PosMarkLast, int PosCurrent, bool Start, bool IsCurrent);
        int ProgressBarMarkPos(int P, int Total);

        void DecorDrawGlowRectHor(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);
        void DecorDrawGlowRectVer(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);

        void DecorDrawGlowRectTL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);
        void DecorDrawGlowRectTR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);
        void DecorDrawGlowRectBL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);
        void DecorDrawGlowRectBR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg);

        void DecorDrawGlowEllipseTL(cPixmap *pixmap, int Left, int Top, int Width, int Height,
                                    tColor ColorBg, int type);
        void DecorDrawGlowEllipseTR(cPixmap *pixmap, int Left, int Top, int Width, int Height,
                                    tColor ColorBg, int type);
        void DecorDrawGlowEllipseBL(cPixmap *pixmap, int Left, int Top, int Width, int Height,
                                    tColor ColorBg, int type);
        void DecorDrawGlowEllipseBR(cPixmap *pixmap, int Left, int Top, int Width, int Height,
                                    tColor ColorBg, int type);

        void TopBarEnableDiskUsage(void);
        // tColor Multiply(tColor Color, uint8_t Alpha);
        tColor SetAlpha(tColor Color, double am);

 public:
        cImageLoader ImgLoader;

        cFlatBaseRender(void);
        ~cFlatBaseRender(void);

        void CreateFullOsd(void);
        void CreateOsd(int Left, int Top, int Width, int Height);

        void TopBarCreate(void);
        void TopBarSetTitle(const cString &Title, bool Clear = true);
        void TopBarSetTitleExtra(const cString Extra1, const cString Extra2);
        void TopBarSetMenuIcon(const cString icon);
        void TopBarSetMenuIconRight(const cString icon);
        void TopBarClearMenuIconRight(void);
        void TopBarSetMenuLogo(const cString icon);
        void TopBarSetExtraIcon(const cString icon);
        void TopBarUpdate(void);

        void ButtonsCreate(void);
        void ButtonsSet(const char *Red, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL);
        bool ButtonsDrawn(void);

        void MessageCreate(void);
        void MessageSet(eMessageType Type, const char *Text);
        void MessageSetExtraTime(const char *Text);
        void MessageClear(void);

        void ProgressBarDrawRaw(cPixmap *Pixmap, cPixmap *PixmapBg, cRect rec, cRect recBg, int Current, int Total,
                                tColor ColorFg, tColor ColorBarFg, tColor ColorBg,
                                int Type, bool SetBackground, bool IsSignal = false);
        void ProgressBarCreate(cRect Rect, int MarginHor, int MarginVer, tColor ColorFg, tColor ColorBarFg,
                               tColor ColorBg, int Type, bool SetBackground = false, bool IsSignal = false);
        void ProgressBarDrawBgColor(void);
        void ProgressBarDraw(int Current, int Total);
        void ProgressBarDrawMarks(int Current, int Total, const cMarks *Marks, tColor Color, tColor ColorCurrent);

        void ScrollbarDraw(cPixmap *Pixmap, int Left, int Top, int Height, int Total, int Offset,
                           int Shown, bool CanScrollUp, bool CanScrollDown);
        int ScrollBarWidth(void);

        void DecorBorderDraw(int Left, int Top, int Width, int Height, int Size, int Type,
                             tColor ColorFg, tColor ColorBg, int From = 0, bool Store = true);
        void DecorBorderClear(cRect Rect, int Size);
        void DecorBorderClearAll(void);
        void DecorBorderRedrawAll(void);
        void DecorBorderClearByFrom(int From);

        int GetFontAscender(const char *Name, int CharHeight, int CharWidth = 0);

        void DrawWidgetWeather(void);
};
