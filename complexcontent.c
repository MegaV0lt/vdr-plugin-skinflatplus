#include "complexcontent.h"

cComplexContent::cComplexContent() {
    Osd = NULL;
    Pixmap = NULL;
    PixmapImage = NULL;
    isShown = false;
    isScrollingActive = true;
    Contents.reserve(128);  // Set to at least 128 entrys
}

cComplexContent::cComplexContent(cOsd *osd, int scrollSize) {
    Osd = osd;
    ScrollSize = scrollSize;

    Pixmap = NULL;
    PixmapImage = NULL;
    isShown = false;
    isScrollingActive = true;
    Contents.reserve(128);  // Set to at least 128 entrys
}

cComplexContent::~cComplexContent() {
}

void cComplexContent::Clear(void) {
    isShown = false;
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
    FullFillBackground = fullFillBackground;

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
    if (FullFillBackground && DrawPortHeight < Position.Height())
        PositionDraw.SetHeight(Position.Height());
    else
        PositionDraw.SetHeight(DrawPortHeight);

    Pixmap = CreatePixmap(Osd, "Pixmap", 1, Position, PositionDraw);
    PixmapImage = CreatePixmap(Osd, "PixmapImage", 2, Position, PositionDraw);
    // dsyslog("flatPlus: ComplexContentPixmap left: %d top: %d width: %d height: %d",
    //         Position.Left(), Position.Top(), Position.Width(), Position.Height());
    // dsyslog("flatPlus: ComplexContentPixmap drawport left: %d top: %d width: %d height: %d", PositionDraw.Left(),
    //         PositionDraw.Top(), PositionDraw.Width(), PositionDraw.Height());

    if (Pixmap) {  // Check for nullptr
        if (FullFillBackground) {
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
    DrawPortHeight = 0;
    std::vector<cSimpleContent>::iterator it, end = Contents.end();
    for (it = Contents.begin(); it != end; ++it) {
        if ((*it).GetBottom() > DrawPortHeight)
            DrawPortHeight = (*it).GetBottom();
    }
    if (isScrollingActive)
        DrawPortHeight = ScrollTotal() * ScrollSize;
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
    if (DrawPortHeight > Height()) return Height();

    return DrawPortHeight;
}

bool cComplexContent::Scrollable(int height) {
    CalculateDrawPortHeight();

    if (height == 0) height = Position.Height();

    int total = ScrollTotal();
    int shown = ceil(height * 1.0f / ScrollSize);
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
    int FloatLines = ceil(image->Height() * 1.0f / ScrollSize);
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
        char *FloatedText = new char[NumChars + 1];
        memset(FloatedText, '\0', NumChars + 1);
        strncpy(FloatedText, text, NumChars);

        ++NumChars;
        char *SecondText = new char[strlen(text) - NumChars + 2];
        memset(SecondText, '\0', strlen(text) - NumChars + 2);
        strncpy(SecondText, text + NumChars, strlen(text) - NumChars);

        cRect SecondTextPos;
        SecondTextPos.SetLeft(textPos.Left());
        SecondTextPos.SetTop(textPos.Top() + FloatLines * ScrollSize);
        SecondTextPos.SetWidth(textPos.Width());
        SecondTextPos.SetHeight(textPos.Height());

        AddText(FloatedText, true, FloatedTextPos, colorFg, colorBg, font, textWidth, textHeight, textAlignment);
        AddText(SecondText, true, SecondTextPos, colorFg, colorBg, font, textWidth, textHeight, textAlignment);

        delete[] FloatedText;
        delete[] SecondText;
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
    isShown = true;
    std::vector<cSimpleContent>::iterator it, end = Contents.end();
    for (it = Contents.begin(); it != end; ++it) {
        if ((*it).GetContentType() == CT_Image)
            (*it).Draw(PixmapImage);
        else
            (*it).Draw(Pixmap);
    }
}

double cComplexContent::ScrollbarSize(void) {
    return Position.Height() * 1.0f / DrawPortHeight;
}

int cComplexContent::ScrollTotal(void) {
    return ceil(DrawPortHeight * 1.0f / ScrollSize);
}

int cComplexContent::ScrollShown(void) {
    // return ceil(Position.Height() * 1.0 / ScrollSize);
    return Position.Height() / ScrollSize;
}

int cComplexContent::ScrollOffset(void) {
    int y = Pixmap->DrawPort().Point().Y() * -1;
    if (y + Position.Height() + ScrollSize > DrawPortHeight) {
        if (y == DrawPortHeight - Position.Height())
            y += ScrollSize;
        else
            y = DrawPortHeight - Position.Height() - 1;
    }
    double offset = y * 1.0f / DrawPortHeight;
    return ScrollTotal() * offset;
}

bool cComplexContent::Scroll(bool Up, bool Page) {
    int aktHeight = Pixmap->DrawPort().Point().Y();
    int totalHeight = Pixmap->DrawPort().Height();
    int screenHeight = Pixmap->ViewPort().Height();
    int lineHeight = ScrollSize;

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
    } else {
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
