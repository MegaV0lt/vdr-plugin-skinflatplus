/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./textscroller.h"

void cTextScroll::SetText(const char *text, cRect position, tColor colorFg, tColor colorBg, cFont *font,
                          tColor colorExtraTextFg) {
    // if (!m_Osd) return;

    Text.reserve(strlen(text));  // Defined in 'textscroller.h'
    Text = text;
    Font = font;
    Position = position;

    ColorFg = colorFg; ColorBg = colorBg; ColorExtraTextFg = colorExtraTextFg;
    const cRect DrawPort(0, 0, font->Width(text), position.Height());

    m_Osd->DestroyPixmap(Pixmap);

    Pixmap = CreatePixmap(m_Osd, "Pixmap", Layer, position, DrawPort);
    // dsyslog("flatPlus: TextScrollerPixmap left: %d top: %d width: %d height: %d",
    //        Position.Left(), Position.Top(), Position.Width(), Position.Height());
    // dsyslog("flatPlus: TextScrollerPixmap DrawPort left: %d top: %d width: %d height: %d",
    //        DrawPort.Left(), DrawPort.Top(), DrawPort.Width(), DrawPort.Height());
    PixmapFill(Pixmap, colorBg);
    Draw();
}

void cTextScroll::UpdateViewPortWidth(int w) {
    if (!Pixmap) return;

    cRect ViewPort {Pixmap->ViewPort()};
    ViewPort.SetWidth(ViewPort.Width() - w);
    Pixmap->SetViewPort(ViewPort);
}

void cTextScroll::Reset() {
    if (!Pixmap) return;

    Pixmap->SetDrawPortPoint(cPoint(0, 0));
    WaitSteps = WAITSTEPS;
}

void cTextScroll::Draw() {
    if (!Pixmap) return;

    if (ColorExtraTextFg) {
        std::string_view tilde {Text};
        const std::size_t found {tilde.find('~')};  // Search for ~
        if (found != std::string::npos) {
            std::string_view sv1 {tilde.substr(0, found)};
            std::string_view sv2 {tilde.substr(found + 1)};  // Default end is npos
            const std::string first {rtrim(sv1)};   // Trim possible space at end
            const std::string second {ltrim(sv2)};  // Trim possible space at begin

            Pixmap->DrawText(cPoint(0, 0), first.c_str(), ColorFg, ColorBg, Font);
            const int l {Font->Width(first.c_str()) + Font->Width('X')};
            Pixmap->DrawText(cPoint(l, 0), second.c_str(), ColorExtraTextFg, ColorBg, Font);
        } else {  // ~ not found
            Pixmap->DrawText(cPoint(0, 0), Text.c_str(), ColorFg, ColorBg, Font);
        }
    } else {  // No extra color defined
        Pixmap->DrawText(cPoint(0, 0), Text.c_str(), ColorFg, ColorBg, Font);
    }
}

void cTextScroll::DoStep() {
    if (!Pixmap) return;

    if (WaitSteps > 0) {  // Wait at the beginning for better read
        --WaitSteps;
        return;
    }

    if (ResetX) {  // Wait after return to the front
        ResetX = false;
        Pixmap->SetDrawPortPoint(cPoint(0, 0));
        WaitSteps = WAITSTEPS;
        return;
    }

    int DrawPortX {Pixmap->DrawPort().X()};

    if (IsReserveStep)
        DrawPortX += PixelsPerStep;
    else
        DrawPortX -= PixelsPerStep;

    int maxX {Pixmap->DrawPort().Width() - Pixmap->ViewPort().Width()};
    maxX *= -1;

    if (ScrollType == 0) {
        if (DrawPortX <= maxX) {
            DrawPortX += PixelsPerStep;
            ResetX = true;
            WaitSteps = WAITSTEPS;
        }
    } else if (ScrollType == 1) {
        if (DrawPortX <= maxX) {
            IsReserveStep = true;
            WaitSteps = WAITSTEPS;
        } else if (DrawPortX > 0) {
            IsReserveStep = false;
            WaitSteps = WAITSTEPS;
        }
    }

    Pixmap->SetDrawPortPoint(cPoint(DrawPortX, 0));
}

cTextScrollers::cTextScrollers() {
    Layer = 2;
    Scrollers.reserve(16);
}

cTextScrollers::~cTextScrollers() {}

void cTextScrollers::Clear() {
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
    Cancel(-1);
    while (Active())
        cCondWait::SleepMs(10);
        //! For debug
            if (ScrollDelay == 0)
                esyslog("FlatPlus: cTextScrollers::AddScroller() ScrollDelay is 0!");

    Scrollers.emplace_back(new cTextScroll(m_Osd, ScrollType, ScrollStep,
        static_cast<int>(WAITDELAY * 1.0 / ScrollDelay), Layer));
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
    if (!Running() && Scrollers.size() > 0)
        Start();
}

void cTextScrollers::Action() {
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
        if (Running())
            cCondWait::SleepMs(ScrollDelay);

        std::vector<cTextScroll *>::iterator it, end = Scrollers.end();
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
