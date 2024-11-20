/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./textscroller.h"

/*
cTextScrollers::AddScroller()
cTextScroll::SetText()
cTextScroll::Draw()
cTextScrollers::StartScrolling()
cTextScrollers::Action()
*/

void cTextScroll::SetText(const char *text, cRect position, tColor colorFg, tColor colorBg, cFont *font,
                          tColor colorExtraTextFg) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScroll::SetText()");
#endif
    // if (!m_Osd) return;

    m_Text.reserve(strlen(text));  // Defined in 'textscroller.h'
    m_Text = text;
    m_Font = font;
    m_Position = position;

    ColorFg = colorFg; ColorBg = colorBg; ColorExtraTextFg = colorExtraTextFg;
    const cRect DrawPort(0, 0, font->Width(text), position.Height());

    m_Osd->DestroyPixmap(Pixmap);

    Pixmap = CreatePixmap(m_Osd, "Pixmap", m_Layer, position, DrawPort);
#ifdef DEBUGFUNCSCALL
    dsyslog("   Pixmap left: %d top: %d width: %d height: %d", m_Position.Left(), m_Position.Top(), m_Position.Width(),
            m_Position.Height());
    dsyslog("   DrawPort left: %d top: %d width: %d height: %d", DrawPort.Left(), DrawPort.Top(), DrawPort.Width(),
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
}

void cTextScroll::Reset() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScroll::Reset()");
#endif
    // if (!Pixmap) return;  // Check in 'Draw()'. Try to reduce load

    Pixmap->SetDrawPortPoint(cPoint(0, 0));
    m_WaitSteps = m_WAITSTEPS;
}

void cTextScroll::Draw() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScroll::Draw()");
#endif

    if (!Pixmap) return;

    if (ColorExtraTextFg) {
        std::string_view tilde {m_Text};
        const std::size_t found {tilde.find('~')};  // Search for ~
        if (found != std::string::npos) {
            std::string_view sv1 {tilde.substr(0, found)};
            std::string_view sv2 {tilde.substr(found + 1)};  // Default end is npos
            const std::string first {rtrim(sv1)};   // Trim possible space at end
            const std::string second {ltrim(sv2)};  // Trim possible space at begin

            Pixmap->DrawText(cPoint(0, 0), first.c_str(), ColorFg, ColorBg, m_Font);
            const int l {m_Font->Width(first.c_str()) + m_Font->Width('X')};
            Pixmap->DrawText(cPoint(l, 0), second.c_str(), ColorExtraTextFg, ColorBg, m_Font);
        } else {  // ~ not found
            Pixmap->DrawText(cPoint(0, 0), m_Text.c_str(), ColorFg, ColorBg, m_Font);
        }
    } else {  // No extra color defined
        Pixmap->DrawText(cPoint(0, 0), m_Text.c_str(), ColorFg, ColorBg, m_Font);
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

    int DrawPortX {Pixmap->DrawPort().X()};

    if (m_IsReserveStep)
        DrawPortX += m_PixelsPerStep;
    else
        DrawPortX -= m_PixelsPerStep;

    int maxX {Pixmap->DrawPort().Width() - Pixmap->ViewPort().Width()};
    maxX *= -1;

    if (m_ScrollType == 0) {
        if (DrawPortX <= maxX) {
            DrawPortX += m_PixelsPerStep;
            m_ResetX = true;
            m_WaitSteps = m_WAITSTEPS;
        }
    } else if (m_ScrollType == 1) {
        if (DrawPortX <= maxX) {
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
    Scrollers.reserve(16);
}

cTextScrollers::~cTextScrollers() {}

void cTextScrollers::Clear() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScrollers::Clear()");
#endif

    Cancel(-1);
    while (Active())
        cCondWait::SleepMs(10);

    std::vector<cTextScroll *>::iterator it, end = Scrollers.end();
    for (it = Scrollers.begin(); it != end; ++it) {
        delete *it;
    }

    Scrollers.clear();
}

void cTextScrollers::AddScroller(const char *text, cRect position, tColor colorFg, tColor colorBg, cFont *m_Font,
                                 tColor ColorExtraTextFg) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScrollers::AddScroller()");
#endif

    Cancel(-1);
    while (Active())
        cCondWait::SleepMs(10);

    if (m_ScrollDelay == 0) {  // Avoid DIV/0
        esyslog("FlatPlus: cTextScrollers::AddScroller() ScrollDelay is 0!");
        return;
    }

    Scrollers.emplace_back(new cTextScroll(m_Osd, m_ScrollType, m_ScrollStep,
        static_cast<int>(WAITDELAY * 1.0 / m_ScrollDelay), m_Layer));
    Scrollers.back()->SetText(text, position, colorFg, colorBg, m_Font, ColorExtraTextFg);

    StartScrolling();
}

void cTextScrollers::UpdateViewPortWidth(int w) {
    std::vector<cTextScroll *>::iterator it, end = Scrollers.end();
    for (it = Scrollers.begin(); it != end; ++it) {
        cPixmap::Lock();
        (*it)->UpdateViewPortWidth(w);
        cPixmap::Unlock();
    }
}

void cTextScrollers::StartScrolling() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScrollers::StartScrolling()");
#endif

    if (!Running() && Scrollers.size() > 0)
        Start();
}

void cTextScrollers::Action() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cTextScrollers::Action()");
#endif

    // Wait 1 second so the osd is finished
    for (uint i {0}; i < 100 && Running(); ++i) {
        cCondWait::SleepMs(10);
    }

    if (!Running()) return;

    std::vector<cTextScroll *>::iterator it, end = Scrollers.end();
    for (it = Scrollers.begin(); it != end; ++it) {
        if (!Running()) return;

        cPixmap::Lock();
        (*it)->Reset();
        cPixmap::Unlock();
    }

    while (Running()) {
        // if (Running())  //? Check needed here?
            cCondWait::SleepMs(m_ScrollDelay);

        // std::vector<cTextScroll *>::iterator it, end = Scrollers.end();  // Reuse iterator above
        for (it = Scrollers.begin(); it != end; ++it) {
            if (!Running()) return;

            cPixmap::Lock();
            (*it)->DoStep();
            cPixmap::Unlock();
        }

        if (Running())
            m_Osd->Flush();
    }
}
