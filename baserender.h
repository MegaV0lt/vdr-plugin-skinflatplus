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
    int Left, Top, Width, Height, Size, Type;
    tColor ColorFg, ColorBg;
    int From;
};

template<class T> inline T MyMax(T a, T b) { return a >= b ? a : b; }

class cFlatBaseRender {
 protected:
        cOsd *osd;

        int g_OsdLeft, g_OsdTop, g_OsdWidth, g_OsdHeight;
        int g_MarginItem;

        // Standard fonts
        cFont *g_Font;
        cFont *g_FontSml;
        cFont *g_FontFixed;
        int g_FontHight;
        int g_FontSmlHight;
        int g_FontFixedHight;

        // TopBar
        cPixmap *TopBarPixmap;
        cPixmap *TopBarIconPixmap;
        cPixmap *TopBarIconBgPixmap;
        cFont *g_TopBarFont, *g_TopBarFontSml, *g_TopBarFontClock;
        int g_TopBarFontHeight, g_TopBarFontSmlHeight, g_TopBarFontClockHeight;

        cString g_TopBarTitle;
        cString g_TopBarTitleExtra1, g_TopBarTitleExtra2;
        cString g_TopBarExtraIcon;
        bool g_TopBarExtraIconSet;
        cString g_TopBarMenuIcon;
        bool g_TopBarMenuIconSet;
        cString g_TopBarMenuIconRight;
        bool g_TopBarMenuIconRightSet;
        cString g_TopBarMenuLogo;
        bool g_TopBarMenuLogoSet;

        bool g_TopBarUpdateTitle;
        cString g_TopBarLastDate;
        int g_TopBarHeight;
        int g_VideoDiskUsageState;

        // Progressbar
        cPixmap *ProgressBarPixmap;
        cPixmap *ProgressBarPixmapBg;
        int g_ProgressBarHeight, g_ProgressBarTop, g_ProgressBarWidth, g_ProgressBarMarginHor, g_ProgressBarMarginVer;
        int g_ProgressType;
        bool g_ProgressBarSetBackground;
        bool g_ProgressBarIsSignal;
        tColor g_ProgressBarColorFg, g_ProgressBarColorBarFg, g_ProgressBarColorBarCurFg, g_ProgressBarColorBg;
        tColor g_ProgressBarColorMark, g_ProgressBarColorMarkCurrent;

        // Scrollbar
        int g_ScrollBarWidth;

        // Buttons red, green, yellow, blue
        cPixmap *ButtonsPixmap;
        int g_ButtonsWidth, g_ButtonsHeight, g_ButtonsTop;
        int g_MarginButtonColor, g_ButtonColorHeight;
        bool g_ButtonsDrawn;

        // Message
        cPixmap *MessagePixmap, *MessageIconPixmap;
        int MessageWidth, MessageHeight;
        cTextScrollers MessageScroller;

        // Multiline content with scrollbar
        cPixmap *ContentPixmap;
        cPixmap *ContentEpgImagePixmap;
        int ContentLeft, ContentTop, ContentHeight, ContentWidth;
        int ContentDrawPortHeight;  // Complete high of text
        int ContentTextHeight;
        bool ContentHasScrollbar;
        bool ContentShown;
        int ContentFontType;
        int ContentEventType;
        int ContentEventHeight;
        int ContentEventPosterWidth, ContentEventPosterHeight;

        tColor ContentColorFg, ContentColorBg;
        cTextWrapper ContentWrapper;
        cTextWrapper ContentWrapperPoster;
        const cEvent *ContentEvent;

        cComplexContent WeatherWidget;

        cPixmap *DecorPixmap;
        std::list<sDecorBorder> Borders;  // For clear specific Borders (clear only MenuItems and not TopBar)

        /* void ContentDraw(void);  // Unused?
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
        void TopBarSetTitle(cString title);
        void TopBarSetTitleWithoutClear(cString title);
        void TopBarSetTitleExtra(cString Extra1, cString Extra2);
        void TopBarSetMenuIcon(cString icon);
        void TopBarSetMenuIconRight(cString icon);
        void TopBarClearMenuIconRight(void);
        void TopBarSetMenuLogo(cString icon);
        void TopBarSetExtraIcon(cString icon);
        void TopBarUpdate(void);

        void ButtonsCreate(void);
        void ButtonsSet(const char *Red, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL);
        bool ButtonsDrawn(void);

        void MessageCreate(void);
        void MessageSet(eMessageType Type, const char *Text);
        void MessageClear(void);

        void ProgressBarDrawRaw(cPixmap *Pixmap, cPixmap *PixmapBg, cRect rec, cRect recBg, int Current, int Total,
                                tColor ColorFg, tColor ColorBarFg, tColor ColorBg,
                                int Type, bool SetBackground, bool IsSignal = false);
        void ProgressBarCreate(int Left, int Top, int Width, int Height, int MarginHor, int MarginVer,
                               tColor ColorFg, tColor ColorBarFg, tColor ColorBg, int Type,
                               bool SetBackground = false, bool IsSignal = false);
        void ProgressBarDrawBgColor(void);
        void ProgressBarDraw(int Current, int Total);
        void ProgressBarDrawMarks(int Current, int Total, const cMarks *Marks, tColor Color, tColor ColorCurrent);

        void ScrollbarDraw(cPixmap *Pixmap, int Left, int Top, int Height, int Total, int Offset,
                           int Shown, bool CanScrollUp, bool CanScrollDown);
        int ScrollBarWidth(void);

        void DecorBorderDraw(int Left, int Top, int Width, int Height, int Size, int Type,
                             tColor ColorFg, tColor ColorBg, int From = 0, bool Store = true);
        void DecorBorderClear(int Left, int Top, int Width, int Height, int Size);
        void DecorBorderClearAll(void);
        void DecorBorderRedrawAll(void);
        void DecorBorderClearByFrom(int From);

        int GetFontAscender(const char *Name, int CharHeight, int CharWidth = 0);

        void DrawWidgetWeather(void);
};
