/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./complexcontent.h"

cComplexContent::cComplexContent() {
    Contents.reserve(128);  // Set to at least 128 entrys
}

cComplexContent::cComplexContent(cOsd *osd, int ScrollSize) {
    m_Osd = osd;
    m_ScrollSize = ScrollSize;

    Contents.reserve(128);  // Set to at least 128 entrys
}

cComplexContent::~cComplexContent() {
}

void cComplexContent::Clear(void) {
    m_IsShown = false;
    Contents.clear();
    if (m_Osd) {  //! Check because Clear() is called before SetOsd()
        m_Osd->DestroyPixmap(Pixmap);
        Pixmap = NULL;
        m_Osd->DestroyPixmap(PixmapImage);
        PixmapImage = NULL;
    }
}

void cComplexContent::CreatePixmaps(bool fullFillBackground) {
    CalculateDrawPortHeight();
    m_FullFillBackground = fullFillBackground;

    // if (!m_Osd) return;

    m_Osd->DestroyPixmap(Pixmap);
    Pixmap = NULL;
    m_Osd->DestroyPixmap(PixmapImage);
    PixmapImage = NULL;

    cRect PositionDraw;
    PositionDraw.SetLeft(0);
    PositionDraw.SetTop(0);
    PositionDraw.SetWidth(Position.Width());
    if (m_FullFillBackground && m_DrawPortHeight < Position.Height())
        PositionDraw.SetHeight(Position.Height());
    else
        PositionDraw.SetHeight(m_DrawPortHeight);

    Pixmap = CreatePixmap(m_Osd, "Pixmap", 1, Position, PositionDraw);
    PixmapImage = CreatePixmap(m_Osd, "PixmapImage", 2, Position, PositionDraw);
    // dsyslog("flatPlus: ComplexContentPixmap left: %d top: %d width: %d height: %d",
    //         Position.Left(), Position.Top(), Position.Width(), Position.Height());
    // dsyslog("flatPlus: ComplexContentPixmap drawport left: %d top: %d width: %d height: %d", PositionDraw.Left(),
    //         PositionDraw.Top(), PositionDraw.Width(), PositionDraw.Height());

    if (Pixmap) {
        if (m_FullFillBackground) {
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
    m_DrawPortHeight = 0;
    std::vector<cSimpleContent>::iterator it, end = Contents.end();
    for (it = Contents.begin(); it != end; ++it) {
        if ((*it).GetBottom() > m_DrawPortHeight)
            m_DrawPortHeight = (*it).GetBottom();
    }
    if (m_IsScrollingActive)
        m_DrawPortHeight = ScrollTotal() * m_ScrollSize;}

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
    if (m_DrawPortHeight > Height()) return Height();

    return m_DrawPortHeight;
}

bool cComplexContent::Scrollable(int height) {
    CalculateDrawPortHeight();
    if (height == 0) height = Position.Height();

    int total = ScrollTotal();
    int shown = ceil(height * 1.0f / m_ScrollSize);
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
    WrapperFloat.Set(text, font, TextWidthLeft);  //* cTextWrapper.Set() strips trailing newlines!
    int FloatLines = ceil(image->Height() * 1.0f / m_ScrollSize);
    int Lines = WrapperFloat.Lines();

    cRect FloatedTextPos;
    FloatedTextPos.SetLeft(textPos.Left());
    FloatedTextPos.SetTop(textPos.Top());
    FloatedTextPos.SetWidth(TextWidthLeft);
    FloatedTextPos.SetHeight(textPos.Height());

    if (Lines < FloatLines) {  // Text fits on the left side of the image
        AddText(text, true, FloatedTextPos, colorFg, colorBg, font, textWidth, textHeight, textAlignment);
    } else {
        std::string s("");
        s.reserve(128);
        int NumChars {0};
        for (int i {0}; i < FloatLines && i < Lines; ++i)
            NumChars += strlen(WrapperFloat.GetLine(i));  //! Line breaks are removed, so NumChars may be to small

        dsyslog("flatPlus: NumChars = %d, FloatLines = %d", NumChars, FloatLines);

        //* Try to readd stripped newlines ('\n') to NumChars
        s = text;  // Convert text to string
        for (size_t i {0}; i < s[NumChars]; ++i)
            if (s[i] == '\n') NumChars += 1;

        dsyslog("flatPlus: Added line breaks, NumChars is now %d", NumChars);

        //! To be removed
        // Detect end of last word  // TODO: Improve; Result is not accurate sometimes
        for (; text[NumChars] != ' ' && text[NumChars] != '\0' && text[NumChars] != '\r' && text[NumChars] != '\n';
             ++NumChars) {
        }
        dsyslog("flatPlus: NumChars after detection of end of last word: %d", NumChars);

        // Split the text to 'FloatedText' and 'SecondText'
        cString FloatedText = cString::sprintf("%s", s.substr(0, NumChars).c_str());  // From start to NumChars
        dsyslog("flatPlus: Text(0 to %d) = \n%s", NumChars, s.substr(0, NumChars).c_str());
        ++NumChars;
        cString SecondText = cString::sprintf("%s", s.substr(NumChars).c_str());  // From NumChars to the end
        dsyslog("flatPlus: Text(%d to %d) = \n%s...", NumChars, NumChars + 15,
                                                      s.substr(NumChars, NumChars + 15).c_str());

        cRect SecondTextPos;
        SecondTextPos.SetLeft(textPos.Left());
        SecondTextPos.SetTop(textPos.Top() + FloatLines * m_ScrollSize);
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
    m_IsShown = true;
    std::vector<cSimpleContent>::iterator it, end = Contents.end();
    for (it = Contents.begin(); it != end; ++it) {
        if ((*it).GetContentType() == CT_Image)
            (*it).Draw(PixmapImage);
        else
            (*it).Draw(Pixmap);
    }
}

double cComplexContent::ScrollbarSize(void) {
    return Position.Height() * 1.0f / m_DrawPortHeight;
}

int cComplexContent::ScrollTotal(void) {
    return ceil(m_DrawPortHeight * 1.0f / m_ScrollSize);
}

int cComplexContent::ScrollShown(void) {
    // return ceil(Position.Height() * 1.0 / m_ScrollSize);
    return Position.Height() / m_ScrollSize;
}

int cComplexContent::ScrollOffset(void) {
    if (!Pixmap) return 0;

    int y = Pixmap->DrawPort().Point().Y() * -1;
    if (y + Position.Height() + m_ScrollSize > m_DrawPortHeight) {
        if (y == m_DrawPortHeight - Position.Height())
            y += m_ScrollSize;
        else
            y = m_DrawPortHeight - Position.Height() - 1;
    }
    double offset = y * 1.0f / m_DrawPortHeight;
    return ScrollTotal() * offset;
}

bool cComplexContent::Scroll(bool Up, bool Page) {
    if (!Pixmap || !PixmapImage) return false;

    int AktHeight = Pixmap->DrawPort().Point().Y();
    int TotalHeight = Pixmap->DrawPort().Height();
    int ScreenHeight = Pixmap->ViewPort().Height();
    int LineHeight = m_ScrollSize;

    bool scrolled = false;
    if (Up) {
        if (Page) {
            int NewY = AktHeight + ScreenHeight;
            if (NewY > 0) NewY = 0;

            Pixmap->SetDrawPortPoint(cPoint(0, NewY));
            PixmapImage->SetDrawPortPoint(cPoint(0, NewY));
            scrolled = true;
        } else {
            if (AktHeight < 0) {
                if (AktHeight + LineHeight < 0) {
                    Pixmap->SetDrawPortPoint(cPoint(0, AktHeight + LineHeight));
                    PixmapImage->SetDrawPortPoint(cPoint(0, AktHeight + LineHeight));
                } else {
                    Pixmap->SetDrawPortPoint(cPoint(0, 0));
                    PixmapImage->SetDrawPortPoint(cPoint(0, 0));
                }
                scrolled = true;
            }
        }
    } else {  // Down
        if (Page) {
            int NewY = AktHeight - ScreenHeight;
            if ((-1) * NewY > TotalHeight - ScreenHeight)
                NewY = (-1) * (TotalHeight - ScreenHeight);
            Pixmap->SetDrawPortPoint(cPoint(0, NewY));
            PixmapImage->SetDrawPortPoint(cPoint(0, NewY));
            scrolled = true;
        } else {
            if (TotalHeight - ((-1) * AktHeight + LineHeight) > ScreenHeight) {
                Pixmap->SetDrawPortPoint(cPoint(0, AktHeight - LineHeight));
                PixmapImage->SetDrawPortPoint(cPoint(0, AktHeight - LineHeight));
            } else {
                int NewY = AktHeight - ScreenHeight;
                if ((-1) * NewY > TotalHeight - ScreenHeight)
                    NewY = (-1) * (TotalHeight - ScreenHeight);
                Pixmap->SetDrawPortPoint(cPoint(0, NewY));
                PixmapImage->SetDrawPortPoint(cPoint(0, NewY));
            }
            scrolled = true;
        }
    }

    return scrolled;
}
