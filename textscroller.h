/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/thread.h>

#include <cstring>  // string.h
#include <list>

#include "./flat.h"

#define WAITDELAY 1000  // In ms

class cTextScroll {
 public:
    cTextScroll(cOsd *osd, int type, int pixels, int waitsteps, int layer = 2) {
        m_Osd = osd;
        m_ScrollType = type;
        m_PixelsPerStep = pixels;
        m_WAITSTEPS = waitsteps;
        m_Layer = layer;
    }

    ~cTextScroll() {
        m_Osd->DestroyPixmap(Pixmap);
        Pixmap = nullptr;
    }

    void UpdateViewPortWidth(int w);
    void Reset();

    void SetText(const char *text, cRect position, tColor colorFg, tColor colorBg,
                 cFont *font, tColor ColorExtraTextFg = 0);
    void DoStep();
    void Draw() const;

 private:
    cRect m_Position {0, 0, 0, 0};

    tColor ColorFg {0}, ColorExtraTextFg {0}, ColorBg {0};
    cString m_Text {""};
    cFont *m_Font {nullptr};
    cPixmap *Pixmap {nullptr};
    cOsd *m_Osd {nullptr};
    int m_Layer {0};
    int m_PixelsPerStep {0};
    int m_WAITSTEPS {0}, m_WaitSteps {0};
    bool m_IsReserveStep {false};
    bool m_ResetX {false};
    int m_ScrollType {0};
};

class cTextScrollers : public cThread {
 public:
    cTextScrollers();
    ~cTextScrollers();

    void Clear();
    void SetOsd(cOsd *osd) { m_Osd = osd;}
    void SetPixmapLayer(int layer) { m_Layer = layer; }
    void SetScrollStep(int step) { m_ScrollStep = step; }
    void SetScrollDelay(int delay) { m_ScrollDelay = delay; }
    void SetScrollType(int type) { m_ScrollType = type; }
    void AddScroller(const char *text, cRect position, tColor colorFg, tColor colorBg,
                     cFont *font, tColor ColorExtraTextFg = 0);
    void UpdateViewPortWidth(int w);
    bool isActive() { return Active(); }

 private:
    std::vector<cTextScroll *> Scrollers;  // NOLINT

    cOsd *m_Osd {nullptr};
    int m_ScrollStep {0}, m_ScrollDelay {0};
    int m_ScrollType {0};
    int m_Layer {0};
    virtual void Action();
    void StartScrolling();
};
