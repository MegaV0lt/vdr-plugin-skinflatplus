#include "textscroller.h"

void cTextScroll::SetText(const char *text, cRect position, tColor colorFg, tColor colorBg, cFont *font,
                          tColor colorExtraTextFg) {
    if (!Osd) return;

    Text = text;
    Font = font;
    Position = position;

    ColorFg = colorFg; ColorBg = colorBg; ColorExtraTextFg = colorExtraTextFg;
    cRect drawPort(0, 0, font->Width(Text.c_str()), Position.Height());

    if (Osd && Pixmap)
        Osd->DestroyPixmap(Pixmap);

    Pixmap = CreatePixmap(Osd, "Pixmap", Layer, Position, drawPort);
    // dsyslog("flatPlus: TextScrollerPixmap left: %d top: %d width: %d height: %d",
    //        Position.Left(), Position.Top(), Position.Width(), Position.Height());
    // dsyslog("flatPlus: TextScrollerPixmap drawPort left: %d top: %d width: %d height: %d",
    //        drawPort.Left(), drawPort.Top(), drawPort.Width(), drawPort.Height());
    PixmapFill(Pixmap, colorBg);
    Draw();
}

void cTextScroll::UpdateViewPortWidth(int w) {
    cRect viewPort = Pixmap->ViewPort();
    viewPort.SetWidth(viewPort.Width() - w);
    Pixmap->SetViewPort(viewPort);
}

void cTextScroll::Reset(void) {
    if (!Pixmap) return;

    Pixmap->SetDrawPortPoint(cPoint(0, 0));
    waitSteps = WAITSTEPS;
}

void cTextScroll::Draw(void) {
    if (!Pixmap) return;

    if (ColorExtraTextFg) {
        std::string tilde = Text;
        size_t found = tilde.find('~');  // Search for ~
        if (found != std::string::npos) {
            std::string first = tilde.substr(0, found);
            std::string second = tilde.substr(found + 1, tilde.length());
            rtrim(first);   // Trim possible space on right side
            ltrim(second);  // Trim possible space at begin

            // Pixmap->DrawText(cPoint(0, 0), first.c_str(), ColorFg, ColorBg, Font);
            Pixmap->DrawText(cPoint(0, 0), *(first.c_str()), ColorFg, ColorBg, Font);
            int l = Font->Width(first.c_str()) + Font->Width('X');
            Pixmap->DrawText(cPoint(l, 0), second.c_str(), ColorExtraTextFg, ColorBg, Font);
        } else  // ~ not found
            Pixmap->DrawText(cPoint(0, 0), Text.c_str(), ColorFg, ColorBg, Font);
    } else {  // No extracolor defined
        Pixmap->DrawText(cPoint(0, 0), Text.c_str(), ColorFg, ColorBg, Font);
    }
}

void cTextScroll::DoStep(void) {
    if (!Pixmap) return;

    // Wait at the beginning for better read
    if (waitSteps > 0) {
        --waitSteps;
        return;
    }
    // Wait after return to the front
    if (ResetX) {
        ResetX = false;
        Pixmap->SetDrawPortPoint(cPoint(0, 0));
        waitSteps = WAITSTEPS;
        return;
    }

    int drawPortX = Pixmap->DrawPort().X();

    if (isReserveStep)
        drawPortX += PixelsPerStep;
    else
        drawPortX -= PixelsPerStep;

    int maxX = Pixmap->DrawPort().Width() - Pixmap->ViewPort().Width();
    maxX *= -1;

    if (ScrollType == 0) {
        if (drawPortX <= maxX) {
            drawPortX += PixelsPerStep;
            ResetX = true;
            waitSteps = WAITSTEPS;
        }
    } else if (ScrollType == 1) {
        if (drawPortX <= maxX) {
            isReserveStep = true;
            waitSteps = WAITSTEPS;
        } else if (drawPortX > 0) {
            isReserveStep = false;
            waitSteps = WAITSTEPS;
        }
    }

    Pixmap->SetDrawPortPoint(cPoint(drawPortX, 0));
}

cTextScrollers::cTextScrollers() {
    Layer = 2;
}

cTextScrollers::~cTextScrollers() {
}

void cTextScrollers::Clear(void) {
    Cancel(-1);
    while (Active())
        cCondWait::SleepMs(10);

    std::vector<cTextScroll *>::iterator it, end = Scrollers.end();
    for (it = Scrollers.begin(); it != end; ++it) {
        delete *it;
    }

    Scrollers.clear();
}

void cTextScrollers::AddScroller(const char *text, cRect position, tColor colorFg, tColor colorBg, cFont *font,
                                 tColor ColorExtraTextFg) {
    Cancel(-1);
    while (Active())
        cCondWait::SleepMs(10);

    Scrollers.push_back(new cTextScroll(Osd, scrollType, scrollStep,
        static_cast<int>(WAITDELAY * 1.0 / scrollDelay), Layer));
    Scrollers.back()->SetText(text, position, colorFg, colorBg, font, ColorExtraTextFg);

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

void cTextScrollers::StartScrolling(void) {
    if (!Running() && Scrollers.size() > 0) {
        Start();
    }
}

void cTextScrollers::Action(void) {
    // Wait 1 second so the osd is finished
    for (int i {0}; i < 100 && Running(); ++i) {
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
            cCondWait::SleepMs(scrollDelay);

        std::vector<cTextScroll *>::iterator it, end = Scrollers.end();
        for (it = Scrollers.begin(); it != end; ++it) {
            if (!Running()) return;

            cPixmap::Lock();
            (*it)->DoStep();
            cPixmap::Unlock();
        }

        if (Running())
            Osd->Flush();
    }
}
