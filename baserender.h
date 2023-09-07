#pragma once

#include "imageloader.h"
#include <list>
#include "flat.h"
#include "textscroller.h"
#include "complexcontent.h"

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

template<class T> inline T myMax(T a, T b) { return a >= b ? a : b; }

class cFlatBaseRender {
 protected:
        cOsd *osd;

        int osdLeft, osdTop, osdWidth, osdHeight;
        int marginItem;

        // Standard fonts
        cFont *font;
        cFont *fontSml;
        cFont *fontFixed;
        int fontHeight;
        int fontSmlHeight;
        int fontFixedHeight;

        // TopBar
        cPixmap *topBarPixmap;
        cPixmap *topBarIconPixmap;
        cPixmap *topBarIconBGPixmap;
        cFont *topBarFont, *topBarFontSml, *topBarFontClock;
        int topBarFontHeight, topBarFontSmlHeight, topBarFontClockHeight;

        cString topBarTitle;
        cString tobBarTitleExtra1, tobBarTitleExtra2;
        cString topBarExtraIcon;
        bool topBarExtraIconSet;
        cString topBarMenuIcon;
        bool topBarMenuIconSet;
        cString topBarMenuIconRight;
        bool topBarMenuIconRightSet;
        cString topBarMenuLogo;
        bool topBarMenuLogoSet;

        bool topBarUpdateTitle;
        cString topBarLastDate;
        int topBarHeight;
        int VideoDiskUsageState;

        // Progressbar
        cPixmap *progressBarPixmap;
        cPixmap *progressBarPixmapBg;
        int progressBarHeight, progressBarTop, progressBarWidth, progressBarMarginHor, progressBarMarginVer;
        int ProgressType;
        bool progressBarSetBackground;
        bool progressBarIsSignal;
        tColor progressBarColorFg, progressBarColorBarFg, progressBarColorBarCurFg, progressBarColorBg;
        tColor progressBarColorMark, progressBarColorMarkCurrent;

        // Scrollbar
        int scrollBarWidth;

        // Buttons rot, grün, gelb, blau
        cPixmap *buttonsPixmap;
        int buttonsWidth, buttonsHeight, buttonsTop;
        int marginButtonColor, buttonColorHeight;
        bool buttonsDrawn;

        // Nachricht
        cPixmap *messagePixmap, *messageIconPixmap;
        int messageWidth, messageHeight;
        cTextScrollers messageScroller;

        // Mehrzeiliger Content mit Scrollbalken
        cPixmap *contentPixmap;
        cPixmap *contentEpgImagePixmap;
        int contentLeft, contentTop, contentHeight, contentWidth;
        int contentDrawPortHeight;  // gesamthöhe des Textes
        int contentTextHeight;
        bool contentHasScrollbar;
        bool contentShown;
        int contentFontType;
        int contentEventType;
        int contentEventHeight;
        int contentEventPosterWidth, contentEventPosterHeight;

        tColor contentColorFg, contentColorBg;
        cTextWrapper contentWrapper;
        cTextWrapper contentWrapperPoster;
        const cEvent *contentEvent;

        cComplexContent weatherWidget;

        cPixmap *decorPixmap;
        std::list<sDecorBorder> Borders;  // for clear specific Borders (clear only MenuItems and not TopBar)

        void contentDraw(void);
        void contentEventDraw(void);
        double ScrollbarSize(void);

        void ProgressBarDrawMark(int posMark, int posMarkLast, int posCurrent, bool Start, bool isCurrent);
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
        cImageLoader imgLoader;

        cFlatBaseRender(void);
        ~cFlatBaseRender(void);

        void CreateFullOsd(void);
        void CreateOsd(int Left, int Top, int Width, int Height);

        void TopBarCreate(void);
        void TopBarSetTitle(cString title);
        void TopBarSetTitleWithoutClear(cString title);
        void TopBarSetTitleExtra(cString extra1, cString extra2);
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
                                int Type, bool SetBackground, bool isSignal = false);
        void ProgressBarCreate(int Left, int Top, int Width, int Height, int MarginHor, int MarginVer,
                               tColor ColorFg, tColor ColorBarFg, tColor ColorBg, int Type,
                               bool SetBackground = false, bool isSignal = false);
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
