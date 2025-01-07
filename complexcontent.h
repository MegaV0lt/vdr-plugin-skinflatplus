/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <cstring>  // string.h
#include <list>

#include "./imageloader.h"
#include "./flat.h"

enum eContentType {
    CT_Text,
    CT_TextMultiline,
    CT_Image,
    CT_Rect,
    CT_None
};

enum eContentImageAlignment {
    CIP_Right
};

class cSimpleContent {
 private:
    int m_ContentType {CT_None};  // Added to avoid compiler warning
    cRect m_Position {0, 0, 0, 0};

    int m_TextWidth {0}, m_TextHeight {0}, m_TextAlignment {0};
    tColor m_ColorFg {0}, m_ColorBg {0};
    cString m_Text {""};
    cImage *m_Image {nullptr};
    cFont *m_Font {nullptr};

 public:
    cSimpleContent() {
    }

    cSimpleContent(const cSimpleContent& rhs) {  // Added to avoid compiler warning
        m_ContentType = rhs.m_ContentType;
        m_Position = rhs.m_Position;
        m_TextWidth = rhs.m_TextWidth;
        m_TextHeight = rhs.m_TextHeight;
        m_TextAlignment = rhs.m_TextAlignment;
        m_ColorFg = rhs.m_ColorFg;
        m_ColorBg = rhs.m_ColorBg;
        m_Text = rhs.m_Text;
        m_Image = rhs.m_Image;
        m_Font = rhs.m_Font;
    }

    ~cSimpleContent() {
    }

    cSimpleContent& operator=(const cSimpleContent& other) {
        if (this != &other) {
            this->m_ContentType = other.m_ContentType;
            this->m_Position = other.m_Position;
            this->m_TextWidth = other.m_TextWidth;
            this->m_TextHeight = other.m_TextHeight;
            this->m_TextAlignment = other.m_TextAlignment;
            this->m_ColorFg = other.m_ColorFg;
            this->m_ColorBg = other.m_ColorBg;
            this->m_Text = other.m_Text;
            this->m_Image = other.m_Image;
            this->m_Font = other.m_Font;
        }
        return *this;
    }

    void SetText(const char *Text, bool Multiline, cRect Position, tColor ColorFg, tColor ColorBg, cFont *Font,
                 int TextWidth = 0, int TextHeight = 0, int TextAlignment = taDefault) {
        m_ContentType = (Multiline) ? CT_TextMultiline : CT_Text;
        m_Position = Position;
        m_Text = Text;
        m_Font = Font;

        m_TextWidth = TextWidth; m_TextHeight = TextHeight; m_TextAlignment = TextAlignment;
        m_ColorFg = ColorFg; m_ColorBg = ColorBg;
    }

    void SetImage(cImage *image, cRect Position) {
        m_ContentType = CT_Image;
        m_Position = Position;
        m_Image = image;
    }

    void SetRect(cRect Position, tColor ColorBg) {
        m_ContentType = CT_Rect;
        m_Position = Position;
        m_ColorBg = ColorBg;
    }

    int GetContentType() { return m_ContentType; }

    int GetBottom() {
        if (m_ContentType == CT_Text)
            return m_Position.Top() + m_Font->Height();

        if (m_ContentType == CT_TextMultiline) {
            cTextFloatingWrapper Wrapper;  // Use modified wrapper
            Wrapper.Set(*m_Text, m_Font, m_Position.Width());
            return m_Position.Top() + (Wrapper.Lines() * m_Font->Height());
        }

        if (m_ContentType == CT_Image)
            return m_Position.Top() + m_Image->Height();

        if (m_ContentType == CT_Rect)
            return m_Position.Top() + m_Position.Height();

        return 0;
    }

    void Draw(cPixmap *Pixmap) {
        if (!Pixmap) return;

        if (m_ContentType == CT_Text) {
            Pixmap->DrawText(m_Position.Point(), *m_Text, m_ColorFg, m_ColorBg, m_Font, m_TextWidth,
                             m_TextHeight, m_TextAlignment);
        } else if (m_ContentType == CT_TextMultiline) {
            cTextFloatingWrapper Wrapper;  // Use modified wrapper
            Wrapper.Set(*m_Text, m_Font, m_Position.Width());
            std::string Line {""};
            Line.reserve(128);
            const int Lines {Wrapper.Lines()};
            const int FontHeight {m_Font->Height()};
            for (int i {0}; i < Lines; ++i) {  // Justify line by line
                Line = Wrapper.GetLine(i);
                if (Config.MenuEventRecordingViewJustify == 1 && i < (Lines - 1))  // Last line is not justified
                    JustifyLine(Line, m_Font, m_Position.Width());
                Pixmap->DrawText(cPoint(m_Position.Left(), m_Position.Top() + (i * FontHeight)), Line.c_str(),
                                 m_ColorFg, m_ColorBg, m_Font, m_TextWidth, m_TextHeight, m_TextAlignment);
            }
        } else if (m_ContentType == CT_Rect) {
            Pixmap->DrawRectangle(m_Position, m_ColorBg);
        } else if (m_ContentType == CT_Image) {
            Pixmap->DrawImage(m_Position.Point(), *m_Image);
        }
    }
};

class cComplexContent {
 public:
    cComplexContent();
    cComplexContent(cOsd *osd, int ScrollSize);
    ~cComplexContent();

    void SetOsd(cOsd *osd) { m_Osd = osd; }
    void SetPosition(cRect Position) { m_Position = Position; }
    void SetScrollSize(int ScrollSize) { m_ScrollSize = ScrollSize; }
    void SetBGColor(tColor ColorBg) { m_ColorBg = ColorBg; }
    void CreatePixmaps(bool FullFillBackground);

    void Clear();

    void AddText(const char *Text, bool Multiline, cRect Position, tColor ColorFg, tColor ColorBg, cFont *Font,
                 int TextWidth = 0, int TextHeight = 0, int TextAlignment = taDefault);
    void AddImage(cImage *image, cRect Position);
    void AddImageWithFloatedText(cImage *image, int imageAlignment, const char *Text, cRect TextPos, tColor ColorFg,
                                 tColor ColorBg, cFont *Font, int TextWidth = 0, int TextHeight = 0,
                                 int TextAlignment = taDefault);
    void AddRect(cRect Position, tColor ColorBg);
    bool Scrollable(int height = 0);
     int ScrollTotal();
     int ScrollOffset();
     int ScrollShown();
    bool Scroll(bool Up, bool Page);
    double ScrollbarSize();
    void SetScrollingActive(bool active) { m_IsScrollingActive = active; }

    int Height() { return m_Position.Height(); }
    int ContentHeight(bool Full);

    int BottomContent();

    int Top() { return m_Position.Top(); }
    void Draw();
    bool IsShown() { return m_IsShown; }
    bool IsScrollingActive() { return m_IsScrollingActive; }

 private:
    std::vector<cSimpleContent> Contents;

    cPixmap *Pixmap {nullptr}, *PixmapImage {nullptr};
    cRect m_Position {0, 0, 0, 0};

    tColor m_ColorBg {0};

    bool m_FullFillBackground {false};
    int m_DrawPortHeight {0};
    int m_ScrollSize {0};
    bool m_IsShown {false};
    bool m_IsScrollingActive {true};

    cOsd *m_Osd {nullptr};

    void CalculateDrawPortHeight();
};
