/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./complexcontent.h"

cComplexContent::cComplexContent() {
    Contents.reserve(128);  // Set to at least 128 entry's
}

cComplexContent::cComplexContent(cOsd *osd, int ScrollSize) {
    m_Osd = osd;
    m_ScrollSize = ScrollSize;

    Contents.reserve(128);  // Set to at least 128 entry's
}

cComplexContent::~cComplexContent() {
}

void cComplexContent::Clear(void) {
    m_IsShown = false;
    Contents.clear();
    if (m_Osd) {  //! Check because Clear() is called before SetOsd()
        m_Osd->DestroyPixmap(Pixmap);
        Pixmap = nullptr;
        m_Osd->DestroyPixmap(PixmapImage);
        PixmapImage = nullptr;
    }
}

void cComplexContent::CreatePixmaps(bool FullFillBackground) {
    CalculateDrawPortHeight();
    m_FullFillBackground = FullFillBackground;

    // if (!m_Osd) return;

    m_Osd->DestroyPixmap(Pixmap);
    Pixmap = nullptr;
    m_Osd->DestroyPixmap(PixmapImage);
    PixmapImage = nullptr;

    cRect PositionDraw {0, 0, m_Position.Width(), m_DrawPortHeight};
    if (FullFillBackground && m_DrawPortHeight < m_Position.Height())
        PositionDraw.SetHeight(m_Position.Height());

    Pixmap = CreatePixmap(m_Osd, "Pixmap", 1, m_Position, PositionDraw);
    PixmapImage = CreatePixmap(m_Osd, "PixmapImage", 2, m_Position, PositionDraw);
    // dsyslog("flatPlus: ComplexContentPixmap left: %d top: %d width: %d height: %d",
    //         m_Position.Left(), m_Position.Top(), m_Position.Width(), m_Position.Height());
    // dsyslog("flatPlus: ComplexContentPixmap drawport left: %d top: %d width: %d height: %d", m_PositionDraw.Left(),
    //         m_PositionDraw.Top(), m_PositionDraw.Width(), m_PositionDraw.Height());

    if (Pixmap) {
        if (FullFillBackground) {
            PixmapFill(Pixmap, m_ColorBg);
        } else {
            Pixmap->DrawRectangle(cRect(0, 0, m_Position.Width(), ContentHeight(false)), m_ColorBg);
        }
    } else {  // Log values and return
        esyslog("flatPlus: CreatePixmaps() Failed to create pixmap left: %d top: %d width: %d height: %d",
                m_Position.Left(), m_Position.Top(), m_Position.Width(), m_Position.Height());
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
    return (m_DrawPortHeight > Height()) ? Height() : m_DrawPortHeight;
}

bool cComplexContent::Scrollable(int height) {
    CalculateDrawPortHeight();
    if (height == 0) height = m_Position.Height();

    const int total {ScrollTotal()};
    const int shown = ceil(height * 1.0 / m_ScrollSize);  // Narrowing conversion
    return (total > shown) ? true : false;
}

void cComplexContent::AddText(const char *Text, bool Multiline, cRect Position, tColor ColorFg, tColor ColorBg,
                              cFont *Font, int TextWidth, int TextHeight, int TextAlignment) {
    Contents.emplace_back(cSimpleContent());
    Contents.back().SetText(Text, Multiline, Position, ColorFg, ColorBg, Font, TextWidth, TextHeight, TextAlignment);
}

void cComplexContent::AddImage(cImage *image, cRect Position) {
    Contents.emplace_back(cSimpleContent());
    Contents.back().SetImage(image, Position);
}

void cComplexContent::AddImageWithFloatedText(cImage *image, int imageAlignment, const char *Text, cRect TextPos,
                                              tColor ColorFg, tColor ColorBg, cFont *Font, int TextWidth,
                                              int TextHeight, int TextAlignment) {
    const int TextWidthFull = (TextWidth > 0) ? TextWidth : m_Position.Width() - TextPos.Left();
    // const int TextWidthLeft = m_Position.Width() - image->Width() - 10 - TextPos.Left();
    const int TextWidthLeft {TextWidthFull - image->Width() - 10};
    const int FloatLines = ceil(image->Height() * 1.0 / m_ScrollSize);  // Narrowing conversion

    cTextFloatingWrapper WrapperFloat;  // Modified cTextWrapper lent from skin ElchiHD
    WrapperFloat.Set(Text, Font, TextWidthFull, FloatLines, TextWidthLeft);  //* Set() strips trailing newlines!
    const int Lines {WrapperFloat.Lines()};

    // dsyslog("flatPlus: AddImageWithFloatedText:\nFloatLines %d, Lines %d, TextWithLeft %d, TextWidthFull %d",
    //         FloatLines, Lines, TextWidthLeft, TextWidthFull);

    std::string Line {""};
    Line.reserve(128);
    cRect FloatedTextPos {TextPos};
    for (int i {0}; i < Lines; ++i) {  // Add text line by line
        FloatedTextPos.SetTop(TextPos.Top() + (i * m_ScrollSize));
        Line = WrapperFloat.GetLine(i);
        if (Config.MenuEventRecordingViewJustify == 1 && i < (Lines - 1))  // Last line is not justified
            JustifyLine(Line, Font, (i < FloatLines) ? TextWidthLeft : TextWidthFull);

        AddText(Line.c_str(), false, FloatedTextPos, ColorFg, ColorBg, Font, TextWidthFull, TextHeight, TextAlignment);
        // dsyslog("flatPlus: Adding floatline (%d): %s", i, WrapperFloat.GetLine(i));
    }

    const cRect ImagePos {TextPos.Left() + TextWidthLeft + 5, TextPos.Top(), image->Width(), image->Height()};
    AddImage(image, ImagePos);
}

void cComplexContent::AddRect(cRect Position, tColor ColorBg) {
    Contents.emplace_back(cSimpleContent());
    Contents.back().SetRect(Position, ColorBg);
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
    return m_Position.Height() * 1.0 / m_DrawPortHeight;
}

int cComplexContent::ScrollTotal(void) {
    return ceil(m_DrawPortHeight * 1.0 / m_ScrollSize);
}

int cComplexContent::ScrollShown(void) {
    // return ceil(m_Position.Height() * 1.0 / m_ScrollSize);
    return m_Position.Height() / m_ScrollSize;
}

int cComplexContent::ScrollOffset(void) {
    if (!Pixmap) return 0;

    int y {Pixmap->DrawPort().Point().Y() * -1};
    if (y + m_Position.Height() + m_ScrollSize > m_DrawPortHeight) {
        if (y == m_DrawPortHeight - m_Position.Height())
            y += m_ScrollSize;
        else
            y = m_DrawPortHeight - m_Position.Height() - 1;
    }

    return ScrollTotal() * (y * 1.0 / m_DrawPortHeight);  // offset = y * 1.0 / m_DrawPortHeight;
}

bool cComplexContent::Scroll(bool Up, bool Page) {
    if (!Pixmap || !PixmapImage) return false;

    const int AktHeight {Pixmap->DrawPort().Point().Y()};
    const int TotalHeight {Pixmap->DrawPort().Height()};
    const int ScreenHeight {Pixmap->ViewPort().Height()};
    const int LineHeight {m_ScrollSize};

    bool scrolled {false};
    if (Up) {
        if (Page) {
            int NewY {AktHeight + ScreenHeight};
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
            int NewY {AktHeight - ScreenHeight};
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
                int NewY {AktHeight - ScreenHeight};
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
