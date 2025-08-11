/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./fontcache.h"
#include "./textscroller.h"

void cTextScroll::SetText(const char *text, const cRect &position, tColor colorFg, tColor colorBg, cFont *font,
                          tColor colorExtraTextFg) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScroll::SetText()");
#endif
    // if (!m_Osd) return;

    m_Text = text;
    m_Font = font;
    m_Position = position;

    ColorFg = colorFg;
    ColorBg = colorBg;
    ColorExtraTextFg = colorExtraTextFg;

    m_CacheValid = false;  // Invalidate cache when text changes

    m_Osd->DestroyPixmap(Pixmap);
    const cRect DrawPort(0, 0, font->Width(text), position.Height());
    Pixmap = CreatePixmap(m_Osd, "Pixmap", m_Layer, position, DrawPort);
#ifdef DEBUGFUNCSCALL
    dsyslog("   Pixmap left %d, top %d, width %d, height %d", m_Position.Left(), m_Position.Top(), m_Position.Width(),
            m_Position.Height());
    dsyslog("   DrawPort left %d, top %d, width %d, height %d", DrawPort.Left(), DrawPort.Top(), DrawPort.Width(),
            DrawPort.Height());
#endif

    PixmapFill(Pixmap, colorBg);
    Draw();
}

void cTextScroll::UpdateViewPortWidth(int w) {
    // if (!Pixmap) return;  // Check in 'Draw()'. Try to reduce load

    cRect ViewPort {Pixmap->ViewPort()};
    ViewPort.SetWidth(ViewPort.Width() - w);
    Pixmap->SetViewPort(ViewPort);
    m_CacheValid = false;  // Force recalculation on next DoStep()
}

void cTextScroll::Reset() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScroll::Reset()");
#endif
    // if (!Pixmap) return;  // Check in 'Draw()'. Try to reduce load

    Pixmap->SetDrawPortPoint(cPoint(0, 0));
    m_WaitSteps = m_WAITSTEPS;
}

void cTextScroll::Draw() const {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScroll::Draw()");
#endif

    if (!Pixmap) return;

    const char *TildePos {strchr(m_Text, '~')};
    if (TildePos && ColorExtraTextFg) {
        cString first(m_Text, TildePos);
        cString second(TildePos + 1);
        first.CompactChars(' ');  // Remove extra spaces
        second.CompactChars(' ');  // Remove extra spaces

        const cString FontName {GetFontName(m_Font->FontName())};
        const int FontHeight {FontCache.GetFontHeight(FontName, m_Font->Size())};
        Pixmap->DrawText(cPoint(0, 0), *first, ColorFg, ColorBg, m_Font);
        const int l {m_Font->Width(*first) + FontCache.GetStringWidth(FontName, FontHeight, "~")};
        Pixmap->DrawText(cPoint(l, 0), *second, ColorExtraTextFg, ColorBg, m_Font);
    } else {
        Pixmap->DrawText(cPoint(0, 0), *m_Text, ColorFg, ColorBg, m_Font);
    }
}

void cTextScroll::DoStep() {
    // if (!Pixmap) return;  // Try to reduce load

    if (m_WaitSteps > 0) {  // Wait at the beginning for better read
        --m_WaitSteps;
        return;
    }

    if (m_ResetX) {  // Wait after return to the front
        m_ResetX = false;
        Pixmap->SetDrawPortPoint(cPoint(0, 0));
        m_WaitSteps = m_WAITSTEPS;
        return;
    }

    int DrawPortX {Pixmap->DrawPort().X() + (m_IsReserveStep ? m_PixelsPerStep : -m_PixelsPerStep)};

    // Cache MaxX calculation to avoid repeated computation
    if (!m_CacheValid) {
#ifdef DEBUGFUNCSCALL
        dsyslog("   DoStep() Update MaxX");
#endif
        m_CachedMaxX = -(Pixmap->DrawPort().Width() - Pixmap->ViewPort().Width());
        m_CacheValid = true;
    }
    int MaxX = m_CachedMaxX;

    if (m_ScrollType == 0) {
        if (DrawPortX <= MaxX) {
            DrawPortX += m_PixelsPerStep;
            m_ResetX = true;
            m_WaitSteps = m_WAITSTEPS;
        }
    } else if (m_ScrollType == 1) {
        if (DrawPortX <= MaxX) {
            m_IsReserveStep = true;
            m_WaitSteps = m_WAITSTEPS;
        } else if (DrawPortX > 0) {
            m_IsReserveStep = false;
            m_WaitSteps = m_WAITSTEPS;
        }
    }

    Pixmap->SetDrawPortPoint(cPoint(DrawPortX, 0));
}

cTextScrollers::cTextScrollers() {
    m_Layer = 2;
    Scrollers.reserve(8);
}

cTextScrollers::~cTextScrollers() {}

void cTextScrollers::Clear() {
#ifdef DEBUGFUNCSCALL
    if (!Scrollers.empty()) dsyslog("flatPlus: cTextScrollers::Clear() Size %ld", Scrollers.size());
#endif

    Cancel(-1);
    while (Active())
        cCondWait::SleepMs(10);

    // No need to iterate over the vector in this case
    for (auto &scroller : Scrollers) {
        delete scroller;
        scroller = nullptr;  // Avoid dangling pointer
    }

    Scrollers.clear();
}

void cTextScrollers::AddScroller(const char *text, const cRect &position, tColor colorFg, tColor colorBg, cFont *Font,
                                 tColor ColorExtraTextFg) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScrollers::AddScroller()");
#endif

    Cancel(-1);
    while (Active())
        cCondWait::SleepMs(10);

    if (m_ScrollDelay < 15) {
        isyslog("flatPlus: cTextScrollers::AddScroller() Scroll delay is less than 15. Please check your config!");
        m_ScrollDelay = 15;  // Set to minimum 15 ms
    }

    Scrollers.emplace_back(new cTextScroll(m_Osd, m_ScrollType, m_ScrollStep,
                                           static_cast<int>((WAITDELAY << 8) / m_ScrollDelay) >> 8,  // Fixed-point
                                           m_Layer));
    Scrollers.back()->SetText(text, position, colorFg, colorBg, Font, ColorExtraTextFg);

    StartScrolling();
}

void cTextScrollers::UpdateViewPortWidth(int w) {
    // Batch lock/unlock
    cPixmap::Lock();
    for (auto *scroller : Scrollers)
        scroller->UpdateViewPortWidth(w);
    cPixmap::Unlock();
}

void cTextScrollers::StartScrolling() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScrollers::StartScrolling()");
#endif

    if (!Running() && !Scrollers.empty()) Start();
}

void cTextScrollers::Action() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScrollers::Action()");
#endif

    // Wait 1 second so the osd is finished
    for (std::size_t i {0}; i < 10 && Running(); ++i) {
        cCondWait::SleepMs(100);
    }

    if (!Running()) return;

    cPixmap::Lock();
    for (auto *scroller : Scrollers)
        scroller->Reset();
    cPixmap::Unlock();

    while (Running()) {
        cCondWait::SleepMs(m_ScrollDelay);

        cPixmap::Lock();
        for (auto *scroller : Scrollers)
            scroller->DoStep();
        cPixmap::Unlock();

        if (Running()) m_Osd->Flush();
    }
}
