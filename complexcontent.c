#include "complexcontent.h"

cComplexContent::cComplexContent() {
    Osd = NULL;
    Pixmap = NULL;
    PixmapImage = NULL;
    g_IsShown = false;
    g_IsScrollingActive = true;
    Contents.reserve(128);  // Set to at least 128 entrys
}

cComplexContent::cComplexContent(cOsd *osd, int scrollSize) {
    Osd = osd;
    g_ScrollSize = scrollSize;

    Pixmap = NULL;
    PixmapImage = NULL;
    g_IsShown = false;
    g_IsScrollingActive = true;
    Contents.reserve(128);  // Set to at least 128 entrys
}

cComplexContent::~cComplexContent() {
}

void cComplexContent::Clear(void) {
    g_IsShown = false;
    Contents.clear();
    if (Osd) {
        if (Pixmap) {
            Osd->DestroyPixmap(Pixmap);
            Pixmap = NULL;
        }
        if (PixmapImage) {
            Osd->DestroyPixmap(PixmapImage);
            PixmapImage = NULL;
        }
    }
}

void cComplexContent::CreatePixmaps(bool fullFillBackground) {
    CalculateDrawPortHeight();
    g_FullFillBackground = fullFillBackground;

    if (!Osd) return;

    if (Pixmap) {
        Osd->DestroyPixmap(Pixmap);
        Pixmap = NULL;
    }
    if (PixmapImage) {
        Osd->DestroyPixmap(PixmapImage);
        PixmapImage = NULL;
    }

    cRect PositionDraw;
    PositionDraw.SetLeft(0);
    PositionDraw.SetTop(0);
    PositionDraw.SetWidth(Position.Width());
    if (g_FullFillBackground && g_DrawPortHeight < Position.Height())
        PositionDraw.SetHeight(Position.Height());
    else
        PositionDraw.SetHeight(g_DrawPortHeight);

    Pixmap = CreatePixmap(Osd, "Pixmap", 1, Position, PositionDraw);
    PixmapImage = CreatePixmap(Osd, "PixmapImage", 2, Position, PositionDraw);
    // dsyslog("flatPlus: ComplexContentPixmap left: %d top: %d width: %d height: %d",
    //         Position.Left(), Position.Top(), Position.Width(), Position.Height());
    // dsyslog("flatPlus: ComplexContentPixmap drawport left: %d top: %d width: %d height: %d", PositionDraw.Left(),
    //         PositionDraw.Top(), PositionDraw.Width(), PositionDraw.Height());

    if (Pixmap) {  // Check for nullptr
        if (g_FullFillBackground) {
            PixmapFill(Pixmap, ColorBg);
        } else {
            Pixmap->DrawRectangle(cRect(0, 0, Position.Width(), ContentHeight(false)), ColorBg);
        }
    } else {  // Log values and return
        esyslog("flatPlus: Failed to create ComplexContentPixmap left: %d top: %d width: %d height: %d",
                Position.Left(), Position.Top(), Position.Width(), Position.Height());
        return;
    }
    PixmapFill(PixmapImage, clrTransparent);
}

void cComplexContent::CalculateDrawPortHeight(void) {
    g_DrawPortHeight = 0;
    std::vector<cSimpleContent>::iterator it, end = Contents.end();
    for (it = Contents.begin(); it != end; ++it) {
        if ((*it).GetBottom() > g_DrawPortHeight)
            g_DrawPortHeight = (*it).GetBottom();
    }
    if (g_IsScrollingActive)
        g_DrawPortHeight = ScrollTotal() * g_ScrollSize;
}

int cComplexContent::BottomContent(void) {
    int bottom {0};
    std::vector<cSimpleContent>::iterator it, end = Contents.end();
    for (it = Contents.begin(); it != end; ++it) {
        if ((*it).GetBottom() > bottom)
            bottom = (*it).GetBottom();
    }
    return bottom;
}

int cComplexContent::ContentHeight(bool Full) {
    if (Full) return Height();

    CalculateDrawPortHeight();
    if (g_DrawPortHeight > Height()) return Height();

    return g_DrawPortHeight;
}

bool cComplexContent::Scrollable(int height) {
    CalculateDrawPortHeight();

    if (height == 0) height = Position.Height();

    int total = ScrollTotal();
    int shown = ceil(height * 1.0f / g_ScrollSize);
    if (total > shown) return true;

    return false;
}

void cComplexContent::AddText(const char *text, bool multiline, cRect position, tColor colorFg, tColor colorBg,
                              cFont *font, int textWidth, int textHeight, int textAlignment) {
    Contents.emplace_back(cSimpleContent());
    Contents.back().SetText(text, multiline, position, colorFg, colorBg, font, textWidth, textHeight, textAlignment);
}

void cComplexContent::AddImage(cImage *image, cRect position) {
    Contents.emplace_back(cSimpleContent());
    Contents.back().SetImage(image, position);
}

void cComplexContent::AddImageWithFloatedText(cImage *image, int imageAlignment, const char *text, cRect textPos,
                                              tColor colorFg, tColor colorBg, cFont *font, int textWidth,
                                              int textHeight, int textAlignment) {
    int TextWidthLeft = Position.Width() - image->Width() - 10 - textPos.Left();

    cTextWrapper WrapperFloat;
    WrapperFloat.Set(text, font, TextWidthLeft);
    int FloatLines = ceil(image->Height() * 1.0f / g_ScrollSize);
    int Lines = WrapperFloat.Lines();

    cRect FloatedTextPos;
    FloatedTextPos.SetLeft(textPos.Left());
    FloatedTextPos.SetTop(textPos.Top());
    FloatedTextPos.SetWidth(TextWidthLeft);
    FloatedTextPos.SetHeight(textPos.Height());

    if (Lines < FloatLines) {  // Text fits on the left side of the image
        AddText(text, true, FloatedTextPos, colorFg, colorBg, font, textWidth, textHeight, textAlignment);
    } else {
        int NumChars {0};
        for (int i {0}; i < Lines && i < FloatLines; ++i) {
            NumChars += strlen(WrapperFloat.GetLine(i));
        }
        // Detect end of last word
        for (; text[NumChars] != ' ' && text[NumChars] != '\0' && text[NumChars] != '\r' && text[NumChars] != '\n';
             ++NumChars) {
        }
        std::string Text = text;
        cString FloatedText = cString::sprintf("%s", Text.substr(0, NumChars).c_str());  // From start to NumChars

        ++NumChars;
        cString SecondText = cString::sprintf("%s", Text.substr(NumChars).c_str());  // From NumChars to the end

        cRect SecondTextPos;
        SecondTextPos.SetLeft(textPos.Left());
        SecondTextPos.SetTop(textPos.Top() + FloatLines * g_ScrollSize);
        SecondTextPos.SetWidth(textPos.Width());
        SecondTextPos.SetHeight(textPos.Height());

        AddText(*FloatedText, true, FloatedTextPos, colorFg, colorBg, font, textWidth, textHeight, textAlignment);
        AddText(*SecondText, true, SecondTextPos, colorFg, colorBg, font, textWidth, textHeight, textAlignment);
    }

    cRect ImagePos;
    ImagePos.SetLeft(textPos.Left() + TextWidthLeft + 5);
    ImagePos.SetTop(textPos.Top());
    ImagePos.SetWidth(image->Width());
    ImagePos.SetHeight(image->Height());

    AddImage(image, ImagePos);
}

void cComplexContent::AddRect(cRect position, tColor colorBg) {
    Contents.emplace_back(cSimpleContent());
    Contents.back().SetRect(position, colorBg);
}

void cComplexContent::Draw() {
    g_IsShown = true;
    std::vector<cSimpleContent>::iterator it, end = Contents.end();
    for (it = Contents.begin(); it != end; ++it) {
        if ((*it).GetContentType() == CT_Image)
            (*it).Draw(PixmapImage);
        else
            (*it).Draw(Pixmap);
    }
}

double cComplexContent::ScrollbarSize(void) {
    return Position.Height() * 1.0f / g_DrawPortHeight;
}

int cComplexContent::ScrollTotal(void) {
    return ceil(g_DrawPortHeight * 1.0f / g_ScrollSize);
}

int cComplexContent::ScrollShown(void) {
    // return ceil(Position.Height() * 1.0 / g_ScrollSize);
    return Position.Height() / g_ScrollSize;
}

int cComplexContent::ScrollOffset(void) {
    int y = Pixmap->DrawPort().Point().Y() * -1;
    if (y + Position.Height() + g_ScrollSize > g_DrawPortHeight) {
        if (y == g_DrawPortHeight - Position.Height())
            y += g_ScrollSize;
        else
            y = g_DrawPortHeight - Position.Height() - 1;
    }
    double offset = y * 1.0f / g_DrawPortHeight;
    return ScrollTotal() * offset;
}

bool cComplexContent::Scroll(bool Up, bool Page) {
    int aktHeight = Pixmap->DrawPort().Point().Y();
    int totalHeight = Pixmap->DrawPort().Height();
    int screenHeight = Pixmap->ViewPort().Height();
    int lineHeight = g_ScrollSize;

    bool scrolled = false;
    if (Up) {
        if (Page) {
            int newY = aktHeight + screenHeight;
            if (newY > 0) newY = 0;

            Pixmap->SetDrawPortPoint(cPoint(0, newY));
            PixmapImage->SetDrawPortPoint(cPoint(0, newY));
            scrolled = true;
        } else {
            if (aktHeight < 0) {
                if (aktHeight + lineHeight < 0) {
                    Pixmap->SetDrawPortPoint(cPoint(0, aktHeight + lineHeight));
                    PixmapImage->SetDrawPortPoint(cPoint(0, aktHeight + lineHeight));
                } else {
                    Pixmap->SetDrawPortPoint(cPoint(0, 0));
                    PixmapImage->SetDrawPortPoint(cPoint(0, 0));
                }
                scrolled = true;
            }
        }
    } else {  // Down
        if (Page) {
            int newY = aktHeight - screenHeight;
            if ((-1) * newY > totalHeight - screenHeight)
                newY = (-1) * (totalHeight - screenHeight);
            Pixmap->SetDrawPortPoint(cPoint(0, newY));
            PixmapImage->SetDrawPortPoint(cPoint(0, newY));
            scrolled = true;
        } else {
            if (totalHeight - ((-1) * aktHeight + lineHeight) > screenHeight) {
                Pixmap->SetDrawPortPoint(cPoint(0, aktHeight - lineHeight));
                PixmapImage->SetDrawPortPoint(cPoint(0, aktHeight - lineHeight));
            } else {
                int newY = aktHeight - screenHeight;
                if ((-1) * newY > totalHeight - screenHeight)
                    newY = (-1) * (totalHeight - screenHeight);
                Pixmap->SetDrawPortPoint(cPoint(0, newY));
                PixmapImage->SetDrawPortPoint(cPoint(0, newY));
            }
            scrolled = true;
        }
    }

    return scrolled;
}
