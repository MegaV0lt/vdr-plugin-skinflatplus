#pragma once

#include <list>
#include "flat.h"
#include <string.h>
#include <vdr/thread.h>

#define WAITDELAY 1000  // In ms

class cTextScroll {
 private:
    cRect Position;

    tColor ColorFg, ColorExtraTextFg, ColorBg;
    std::string Text;
    cFont *Font;
    cPixmap *Pixmap;
    cOsd *Osd;
    int Layer;
    int PixelsPerStep;
    int WAITSTEPS, waitSteps {0};
    bool isReserveStep;
    bool ResetX;
    int ScrollType;

 public:
    cTextScroll(cOsd *osd, int type, int pixels, int waitsteps, int layer) {
        Font = NULL;
        Pixmap = NULL;
        Osd = osd;
        Layer = layer;
        PixelsPerStep = pixels;
        ScrollType = type;
        isReserveStep = false;
        WAITSTEPS = waitsteps;
        ResetX = false;
    }
    cTextScroll(cOsd *osd, int type, int pixels, int waitsteps) {
        Font = NULL;
        Pixmap = NULL;
        Osd = osd;
        Layer = 2;
        PixelsPerStep = pixels;
        ScrollType = type;
        isReserveStep = false;
        WAITSTEPS = waitsteps;
        ResetX = false;
    }

    virtual ~cTextScroll() {  // Fix deleting object of polymorphic class type ‘cTextScroll’ which has
        if (Pixmap) {         // non-virtual destructor might cause undefined behavior [-Wdelete-non-virtual-dtor]
            Osd->DestroyPixmap(Pixmap);
            Pixmap = NULL;
        }
    }

    void UpdateViewPortWidth(int w);
    void Reset(void);

    void SetText(const char *text, cRect position, tColor colorFg, tColor colorBg,
                 cFont *font, tColor ColorExtraTextFg = 0);
    void DoStep(void);
    void Draw(void);
};

class cTextScrollers : public cThread {
 private:
    std::vector<cTextScroll *> Scrollers;

    cOsd *Osd;
    int scrollStep, scrollDelay;
    int scrollType;
    int Layer;
    virtual void Action(void);
    void StartScrolling(void);
 public:
    cTextScrollers();
    ~cTextScrollers();

    void Clear(void);
    void SetOsd(cOsd *osd) { Osd = osd;}
    void SetPixmapLayer(int layer) { Layer = layer; }
    void SetScrollStep(int step) { scrollStep = step; }
    void SetScrollDelay(int delay) { scrollDelay = delay; }
    void SetScrollType(int type) { scrollType = type; }
    void AddScroller(const char *text, cRect position, tColor colorFg, tColor colorBg,
                     cFont *font, tColor ColorExtraTextFg = 0);
    void UpdateViewPortWidth(int w);
    bool isActive(void) { return Active(); }
};
