/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./complexcontent.h"

#include <vdr/tools.h>

#include <algorithm>
#include <numeric>

cComplexContent::cComplexContent() {
    Contents.reserve(128);  // Set to at least 128 entry's
}

cComplexContent::cComplexContent(cOsd *osd, int ScrollSize) {
    m_Osd = osd;
    m_ScrollSize = ScrollSize;
    Contents.reserve(128);
}

cComplexContent::~cComplexContent() {
}

void cComplexContent::Clear() {
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

    if (Pixmap) {
        if (FullFillBackground) {
            PixmapFill(Pixmap, m_ColorBg);
        } else {
            const int HeightContent {ContentHeight(false)};
            if (HeightContent > 0)  // Only draw background if content height is > 0
                Pixmap->DrawRectangle(cRect(0, 0, m_Position.Width(), HeightContent), m_ColorBg);
            else
                PixmapClear(Pixmap);
        }
    } else {  // Log values and return
        esyslog(
            "flatPlus: cComplexContent::CreatePixmaps() Failed to create pixmap left: %d top: %d width: %d height: %d",
            m_Position.Left(), m_Position.Top(), m_Position.Width(), m_Position.Height());
        return;
    }
    PixmapClear(PixmapImage);
}

void cComplexContent::CalculateDrawPortHeight() {
    m_DrawPortHeight = BottomContent();

    // m_DrawPortHeight has to be set for 'ScrollTotal()' to work
    if (m_IsScrollingActive && m_ScrollSize > 0) {
        m_DrawPortHeight = ScrollTotal() * m_ScrollSize;
    }
}
int cComplexContent::BottomContent() const {
    // Using std::accumulate algorithm instead of manual loop
    return std::accumulate(Contents.begin(), Contents.end(), 0,
    [](int max, const auto &content) {
        return std::max(max, content.GetBottom());
    });
}

int cComplexContent::ContentHeight(bool Full) {
    if (Full) return Height();

    CalculateDrawPortHeight();
    return std::min(m_DrawPortHeight, Height());
}

bool cComplexContent::Scrollable(int height) {
    CalculateDrawPortHeight();
    if (height == 0) height = m_Position.Height();

    if (m_ScrollSize == 0) {  // Avoid DIV/0
        esyslog("flatPlus: Error in cComplexContent::Scrollable() m_ScrollSize is 0!");
        return false;
    }

    const int total {ScrollTotal()};
    // const int shown = ceil(height * 1.0 / m_ScrollSize);  // Narrowing conversion
    const int shown {(height + m_ScrollSize - 1) / m_ScrollSize};  // Avoid floating-point and use integer division
    return total > shown;
}

void cComplexContent::AddText(const char *Text, bool Multiline, const cRect &Position, tColor ColorFg, tColor ColorBg,
                              cFont *Font, int TextWidth, int TextHeight, int TextAlignment) {
    // Use emplace_back to construct a cSimpleContent in-place and then set its properties.
    Contents.emplace_back();
    Contents.back().SetText(Text, Multiline, Position, ColorFg, ColorBg, Font, TextWidth, TextHeight, TextAlignment);
}

void cComplexContent::AddImage(cImage *image, const cRect &Position) {
    Contents.emplace_back();
    Contents.back().SetImage(image, Position);
}

void cComplexContent::AddImageWithFloatedText(cImage *image, int imageAlignment, const char *Text, const cRect &TextPos,
                                              tColor ColorFg, tColor ColorBg, cFont *Font, int TextWidth,
                                              int TextHeight, int TextAlignment) {
    const int TextWidthFull {(TextWidth > 0) ? TextWidth : m_Position.Width() - TextPos.Left()};
    // const int TextWidthLeft = m_Position.Width() - image->Width() - 10 - TextPos.Left();
    const int TextWidthLeft {TextWidthFull - image->Width() - 10};
    if (m_ScrollSize == 0) {  // Avoid DIV/0
        esyslog("flatPlus: Error in cComplexContent::AddImageWithFloatedText() m_ScrollSize is 0!");
        return;
    }

    // const int FloatLines = ceil(image->Height() * 1.0 / m_ScrollSize);  // Narrowing conversion
    const int FloatLines {(image->Height() + m_ScrollSize - 1) / m_ScrollSize};  // Use integer division

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
        // Last line is not justified
        if (Config.MenuEventRecordingViewJustify == 1 && i < (Lines - 1))
            JustifyLine(Line, Font, (i < FloatLines) ? TextWidthLeft : TextWidthFull);

        AddText(Line.c_str(), false, FloatedTextPos, ColorFg, ColorBg, Font, TextWidthFull, TextHeight, TextAlignment);
    }

    const cRect ImagePos {TextPos.Left() + TextWidthLeft + 5, TextPos.Top(), image->Width(), image->Height()};
    AddImage(image, ImagePos);
}

void cComplexContent::AddRect(const cRect &Position, tColor ColorBg) {
    Contents.emplace_back();
    Contents.back().SetRect(Position, ColorBg);
}

void cComplexContent::Draw() {
    m_IsShown = true;
    if (Contents.empty()) {
        esyslog("flatPlus: Error in cComplexContent::Draw() Contents is empty!");
        return;
    }

    for (auto &content : Contents) {
        if (content.GetContentType() == CT_Image)
            content.Draw(PixmapImage);
        else
            content.Draw(Pixmap);
    }
}

double cComplexContent::ScrollbarSize() const {
    if (m_DrawPortHeight == 0) {  // Avoid DIV/0}
        esyslog("flatPlus: Error in cComplexContent::ScrollbarSize() m_DrawPortHeight is 0!");
        return 0;
    }

    return static_cast<double>(m_Position.Height()) / m_DrawPortHeight;
}

int cComplexContent::ScrollTotal() const {
    if (m_ScrollSize == 0) {  // Avoid DIV/0}
        esyslog("flatPlus: Error in cComplexContent::ScrollTotal() m_ScrollSize is 0!");
        return 0;
    }

    // return ceil(m_DrawPortHeight * 1.0 / m_ScrollSize);
    return (m_DrawPortHeight + m_ScrollSize - 1) / m_ScrollSize;
}

int cComplexContent::ScrollShown() const {
    if (m_ScrollSize == 0) {  // Avoid DIV/0
        esyslog("flatPlus: Error in cComplexContent::ScrollShown() m_ScrollSize is 0!");
        return 0;
    }

    // return ceil(m_Position.Height() * 1.0 / m_ScrollSize);
    return m_Position.Height() / m_ScrollSize;
}

int cComplexContent::ScrollOffset() const {
    if (!Pixmap) return 0;

    int y {Pixmap->DrawPort().Point().Y() * -1};
    const int PositionHeight {m_Position.Height()};
    if (y + PositionHeight + m_ScrollSize > m_DrawPortHeight) {
        if (y == m_DrawPortHeight - PositionHeight)
            y += m_ScrollSize;
        else
            y = m_DrawPortHeight - PositionHeight - 1;
    }

    if (m_DrawPortHeight == 0) {  // Avoid DIV/0
        esyslog("flatPlus: Error in cComplexContent::ScrollOffset() m_DrawPortHeight is 0!");
        return 0;
    }

    // return ScrollTotal() * (static_cast<double>(y) / m_DrawPortHeight);  // offset = y * 1.0 / m_DrawPortHeight;
    return (y * ScrollTotal()) / m_DrawPortHeight;
}


/**
 * Scrolls the content of this cComplexContent by one line or one page, depending
 * on the given parameters.
 *
 * @param Up true to scroll up, false to scroll down
 * @param Page true to scroll by one page, false to scroll by one line
 * @return true if the content was scrolled, false otherwise
 */
bool cComplexContent::Scroll(bool Up, bool Page) {
    if (!Pixmap || !PixmapImage) return false;

    const int CurrentHeight {Pixmap->DrawPort().Point().Y()};
    const int TotalHeight {Pixmap->DrawPort().Height()};
    const int ScreenHeight {Pixmap->ViewPort().Height()};
    const int LineHeight {m_ScrollSize};

    int NewY {CurrentHeight};

    if (Up) {
        if (Page) {  // Page up
            NewY = std::min(CurrentHeight + ScreenHeight, 0);
        } else if (CurrentHeight < 0) {  // Line up
            NewY = std::min(CurrentHeight + LineHeight, 0);
        } else {  // No scrolling needed
            return false;
        }
    } else {
        const int MaxScroll = -(TotalHeight - ScreenHeight);
        if (Page) {  // Page down
            NewY = std::max(CurrentHeight - ScreenHeight, MaxScroll);
        } else {  // Line down
            NewY = (TotalHeight - (-CurrentHeight + LineHeight) > ScreenHeight)
                   ? CurrentHeight - LineHeight
                   : MaxScroll;
        }
    }

    // Only update when the new Y position is different from the current one
    // This prevents unnecessary updates when the position is already correct
    if (NewY != CurrentHeight) {
        const cPoint NewPoint(0, NewY);
        Pixmap->SetDrawPortPoint(NewPoint);
        PixmapImage->SetDrawPortPoint(NewPoint);
        return true;
    }

    return false;
}
