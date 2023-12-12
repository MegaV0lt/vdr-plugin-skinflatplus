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

        int m_OsdLeft, m_OsdTop, m_OsdWidth, m_OsdHeight;
        int m_MarginItem;

        // Standard fonts
        cFont *m_Font;
        cFont *m_FontSml;
        cFont *m_FontFixed;
        int m_FontHight;
        int m_FontSmlHight;
        int m_FontFixedHight;

        // TopBar
        cPixmap *TopBarPixmap;
        cPixmap *TopBarIconPixmap;
        cPixmap *TopBarIconBgPixmap;
        cFont *m_TopBarFont, *m_TopBarFontSml, *m_TopBarFontClock;
        int m_TopBarFontHeight, m_TopBarFontSmlHeight, m_TopBarFontClockHeight;

        cString m_TopBarTitle;
        cString m_TopBarTitleExtra1, m_TopBarTitleExtra2;
        cString m_TopBarExtraIcon;
        bool m_TopBarExtraIconSet;
        cString m_TopBarMenuIcon;
        bool m_TopBarMenuIconSet;
        cString m_TopBarMenuIconRight;
        bool m_TopBarMenuIconRightSet;
        cString m_TopBarMenuLogo;
        bool m_TopBarMenuLogoSet;

        bool m_TopBarUpdateTitle;
        cString m_TopBarLastDate;
        int m_TopBarHeight;
        int m_VideoDiskUsageState;

        // Progressbar
        cPixmap *ProgressBarPixmap;
        cPixmap *ProgressBarPixmapBg;
        int m_ProgressBarHeight, m_ProgressBarTop, m_ProgressBarWidth, m_ProgressBarMarginHor, m_ProgressBarMarginVer;
        int m_ProgressType;
        bool m_ProgressBarSetBackground;
        bool m_ProgressBarIsSignal;
        tColor m_ProgressBarColorFg, m_ProgressBarColorBarFg, m_ProgressBarColorBarCurFg, m_ProgressBarColorBg;
        tColor m_ProgressBarColorMark, m_ProgressBarColorMarkCurrent;

        // Scrollbar
        int m_ScrollBarWidth;

        // Buttons red, green, yellow, blue
        cPixmap *ButtonsPixmap;
        int m_ButtonsWidth, m_ButtonsHeight, m_ButtonsTop;
        int m_MarginButtonColor, m_ButtonColorHeight;
        bool m_ButtonsDrawn;

        // Message
        cPixmap *MessagePixmap, *MessageIconPixmap;
        int m_MessageWidth, m_MessageHeight;  // TODO: m_MessageWidth unused?
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
