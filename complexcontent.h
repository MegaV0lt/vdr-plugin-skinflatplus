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
#include <string>
#include <vector>

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

    void DrawMultilineText(cPixmap *Pixmap) const {
        cTextFloatingWrapper Wrapper;
        Wrapper.Set(*m_Text, m_Font, m_Position.Width());
        std::string Line;
        Line.reserve(128);
        const int Lines {Wrapper.Lines()};
        const int FontHeight {m_Font->Height()};

        for (int i {0}; i < Lines; ++i) {
            Line = Wrapper.GetLine(i);
            if (Config.MenuEventRecordingViewJustify == 1 && i < (Lines - 1)) {
                JustifyLine(Line, m_Font, m_Position.Width());
            }
            Pixmap->DrawText(cPoint(m_Position.Left(), m_Position.Top() + (i * FontHeight)),
                            Line.c_str(), m_ColorFg, m_ColorBg, m_Font,
                            m_TextWidth, m_TextHeight, m_TextAlignment);
        }
    }

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

    void SetText(const char *Text, bool Multiline, const cRect &Position, tColor ColorFg, tColor ColorBg, cFont *Font,
                 int TextWidth = 0, int TextHeight = 0, int TextAlignment = taDefault) {
        m_ContentType = (Multiline) ? CT_TextMultiline : CT_Text;
        m_Position = Position;
        m_Text = Text;
        m_Font = Font;

        m_TextWidth = TextWidth; m_TextHeight = TextHeight; m_TextAlignment = TextAlignment;
        m_ColorFg = ColorFg; m_ColorBg = ColorBg;
    }

    void SetImage(cImage *image, const cRect &Position) {
        if (!image) return;
        m_ContentType = CT_Image;
        m_Position = Position;
        m_Image = image;
    }

    void SetRect(const cRect &Position, tColor ColorBg) {
        m_ContentType = CT_Rect;
        m_Position = Position;
        m_ColorBg = ColorBg;
    }

    int GetContentType() const { return m_ContentType; }
    int GetBottom() const {
        switch (m_ContentType) {
            case CT_Text:
                return m_Position.Top() + m_Font->Height();

            case CT_TextMultiline: {
                cTextFloatingWrapper Wrapper;
                Wrapper.Set(*m_Text, m_Font, m_Position.Width());
                return m_Position.Top() + (Wrapper.Lines() * m_Font->Height());
            }

            case CT_Image:
                return m_Position.Top() + m_Image->Height();

            case CT_Rect:
                return m_Position.Top() + m_Position.Height();

            case CT_None:
            default:
                return 0;
        }
    }
    void Draw(cPixmap *Pixmap) const {
        if (!Pixmap) return;
        // if (!m_Font || !m_Text) return;

        switch (m_ContentType) {
            case CT_Text:
                Pixmap->DrawText(m_Position.Point(), *m_Text, m_ColorFg, m_ColorBg, m_Font,
                               m_TextWidth, m_TextHeight, m_TextAlignment);
                return;

            case CT_TextMultiline:
                DrawMultilineText(Pixmap);
                return;

            case CT_Rect:
                Pixmap->DrawRectangle(m_Position, m_ColorBg);
                return;

            case CT_Image:
                Pixmap->DrawImage(m_Position.Point(), *m_Image);
                return;
        }
    }
};

class cComplexContent {
 public:
    cComplexContent();
    cComplexContent(cOsd *osd, int ScrollSize);
    ~cComplexContent();

    void SetOsd(cOsd *osd) { m_Osd = osd; }
    void SetPosition(const cRect &Position) { m_Position = Position; }
    void SetScrollSize(int ScrollSize) { m_ScrollSize = ScrollSize; }
    void SetBGColor(tColor ColorBg) { m_ColorBg = ColorBg; }
    void CreatePixmaps(bool FullFillBackground);

    void Clear();

    void AddText(const char *Text, bool Multiline, const cRect &Position, tColor ColorFg, tColor ColorBg, cFont *Font,
                 int TextWidth = 0, int TextHeight = 0, int TextAlignment = taDefault);
    void AddImage(cImage *image, const cRect &Position);
    void AddImageWithFloatedText(cImage *image, int imageAlignment, const char *Text, const cRect &TextPos,
                                 tColor ColorFg, tColor ColorBg, cFont *Font, int TextWidth = 0, int TextHeight = 0,
                                 int TextAlignment = taDefault);
    void AddRect(const cRect &Position, tColor ColorBg);
    bool Scrollable(int height = 0);
     int ScrollTotal() const;
     int ScrollOffset() const;
     int ScrollShown() const;
    bool Scroll(bool Up, bool Page);
    double ScrollbarSize() const;
    void SetScrollingActive(bool active) { m_IsScrollingActive = active; }

    int Height() const { return m_Position.Height(); }
    int ContentHeight(bool Full);

    int BottomContent() const;

    int Top() const { return m_Position.Top(); }
    void Draw();
    bool IsShown() const { return m_IsShown; }
    bool IsScrollingActive() const { return m_IsScrollingActive; }

 private:
    std::vector<cSimpleContent> Contents;

    cOsd *m_Osd {nullptr};
    cPixmap *Pixmap {nullptr}, *PixmapImage {nullptr};
    cRect m_Position {0, 0, 0, 0};

    tColor m_ColorBg {0};
    bool m_FullFillBackground {false};
    int m_DrawPortHeight {0};
    int m_ScrollSize {0};
    bool m_IsShown {false};
    bool m_IsScrollingActive {true};

    void CalculateDrawPortHeight();
};
