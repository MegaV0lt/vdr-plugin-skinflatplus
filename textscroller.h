/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <cstring>  // string.h
#include <vdr/thread.h>
#include <list>
#include "./flat.h"

#define WAITDELAY 1000  // In ms

class cTextScroll {
 private:
    cRect Position {0, 0, 0, 0};

    tColor ColorFg {0}, ColorExtraTextFg {0}, ColorBg {0};
    std::string Text{""};
    cFont *Font {nullptr};
    cPixmap *Pixmap {nullptr};
    cOsd *m_Osd {nullptr};
    int Layer {0};
    int PixelsPerStep {0};
    int WAITSTEPS {0}, WaitSteps {0};
    bool IsReserveStep = false;
    bool ResetX = false;
    int ScrollType {0};

 public:
    cTextScroll(cOsd *osd, int type, int pixels, int waitsteps, int layer) {
        // Font = NULL;  // TODO: Is that needed?
        // Pixmap = NULL;
        m_Osd = osd;
        Layer = layer;
        PixelsPerStep = pixels;
        ScrollType = type;
        // IsReserveStep = false;
        WAITSTEPS = waitsteps;
        // ResetX = false;
    }
    cTextScroll(cOsd *osd, int type, int pixels, int waitsteps) {
        // Font = NULL;
        // Pixmap = NULL;
        m_Osd = osd;
        Layer = 2;
        PixelsPerStep = pixels;
        ScrollType = type;
        // IsReserveStep = false;
        WAITSTEPS = waitsteps;
        // ResetX = false;
    }

    virtual ~cTextScroll() { // Fix deleting object of polymorphic class type ‘cTextScroll’ which has
                             // non-virtual destructor might cause undefined behavior [-Wdelete-non-virtual-dtor]
        m_Osd->DestroyPixmap(Pixmap);
        Pixmap = nullptr;
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

    cOsd *m_Osd {nullptr};
    int ScrollStep {0}, ScrollDelay {0};
    int ScrollType {0};
    int Layer {0};
    virtual void Action(void);
    void StartScrolling(void);
 public:
    cTextScrollers();
    ~cTextScrollers();

    void Clear(void);
    void SetOsd(cOsd *osd) { m_Osd = osd;}
    void SetPixmapLayer(int layer) { Layer = layer; }
    void SetScrollStep(int step) { ScrollStep = step; }
    void SetScrollDelay(int delay) { ScrollDelay = delay; }
    void SetScrollType(int type) { ScrollType = type; }
    void AddScroller(const char *text, cRect position, tColor colorFg, tColor colorBg,
                     cFont *font, tColor ColorExtraTextFg = 0);
    void UpdateViewPortWidth(int w);
    bool isActive(void) { return Active(); }
};
