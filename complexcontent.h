#pragma once

#include "imageloader.h"
#include <list>
#include "flat.h"
#include <string.h>

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
    int ContentType;
    cRect Position;

    int TextWidth, TextHeight, TextAlignment;
    tColor ColorFg, ColorBg;
    std::string Text;
    cImage *Image;
    cFont *Font;

 public:
    cSimpleContent(void) {
        Image = NULL;
        Font = NULL;

        ContentType = CT_None;  // Added to avoid compiler warning
        TextWidth = 0, TextHeight = 0, TextAlignment = 0;
        // tColor ColorFg, ColorBg;  // TODO ???
    }

    cSimpleContent(const cSimpleContent& rhs) {  // Added to avoid compiler warning
        ContentType = rhs.ContentType;
        Position = rhs.Position;
        TextWidth = rhs.TextWidth;
        TextHeight = rhs.TextHeight;
        TextAlignment = rhs.TextAlignment;
        ColorFg = rhs.ColorFg;
        ColorBg = rhs.ColorBg;
        Text = rhs.Text;
        Image = rhs.Image;
        Font = rhs.Font;
    }

    ~cSimpleContent() {
    }

    cSimpleContent& operator=(const cSimpleContent& other) {
        if (this != &other) {
            this->ContentType = other.ContentType;
            this->Position = other.Position;
            this->Text = other.Text;
            this->TextWidth = other.TextWidth;
            this->TextHeight = other.TextHeight;
            this->TextAlignment = other.TextAlignment;
            this->ColorFg = other.ColorFg;
            this->ColorBg = other.ColorBg;
            this->Image = other.Image;
            this->Font = other.Font;
        }
        return *this;
    }

    void SetText(const char *text, bool Multiline, cRect position, tColor colorFg, tColor colorBg, cFont *font,
                 int textWidth = 0, int textHeight = 0, int textAlignment = taDefault) {
        ContentType = CT_Text;
        Text = text;

        Font = font;
        Position = position;

        if (Multiline)
            ContentType = CT_TextMultiline;

        ColorFg = colorFg; ColorBg = colorBg;
        TextWidth = textWidth; TextHeight = textHeight; TextAlignment = textAlignment;
    }

    void SetImage(cImage *image, cRect position) {
        ContentType = CT_Image;
        Image = image;
        Position = position;
    }

    void SetRect(cRect position, tColor colorBg) {
        ContentType = CT_Rect;
        Position = position;
        ColorBg = colorBg;
    }

    int GetContentType(void) { return ContentType; }
    int GetBottom(void) {
        if (ContentType == CT_Text)
            return Position.Top() + Font->Height();

        if (ContentType == CT_TextMultiline) {
            cTextWrapper Wrapper;
            Wrapper.Set(Text.c_str(), Font, Position.Width());
            return Position.Top() + (Wrapper.Lines() * Font->Height());
        }

        if (ContentType == CT_Image)
            return Position.Top() + Image->Height();

        if (ContentType == CT_Rect)
            return Position.Top() + Position.Height();

        return 0;
    }

    void Draw(cPixmap *Pixmap) {
        if (!Pixmap) return;

        if (ContentType == CT_Text) {
            Pixmap->DrawText(cPoint(Position.Left(), Position.Top()), Text.c_str(), ColorFg, ColorBg, Font,
                             TextWidth, TextHeight, TextAlignment);
        } else if (ContentType == CT_TextMultiline) {
            cTextWrapper Wrapper;
            Wrapper.Set(Text.c_str(), Font, Position.Width());
            int Lines = Wrapper.Lines();
            for (int i {0}; i < Lines; ++i) {
                Pixmap->DrawText(cPoint(Position.Left(), Position.Top() + (i * Font->Height())), Wrapper.GetLine(i),
                                 ColorFg, ColorBg, Font, TextWidth, TextHeight, TextAlignment);
            }
        } else if (ContentType == CT_Rect) {
            Pixmap->DrawRectangle(Position, ColorBg);
        } else if (ContentType == CT_Image) {
            Pixmap->DrawImage(cPoint(Position.Left(), Position.Top()), *Image);
        }
    }
};

class cComplexContent {
 private:
    std::vector<cSimpleContent> Contents;

    cPixmap *Pixmap, *PixmapImage;
    cRect Position;

    tColor ColorBg;

    bool m_FullFillBackground;  // m_* for private variables
    int m_DrawPortHeight;
    int m_ScrollSize;
    bool m_IsShown;
    bool m_IsScrollingActive;

    cOsd *Osd;

    void CalculateDrawPortHeight(void);

 public:
    cComplexContent(void);
    cComplexContent(cOsd *osd, int ScrollSize);
    ~cComplexContent();

    void SetOsd(cOsd *osd) { Osd = osd; }
    void SetPosition(cRect position) { Position = position; }
    void SetScrollSize(int ScrollSize) { m_ScrollSize = ScrollSize; }
    void SetBGColor(tColor colorBg) { ColorBg = colorBg; }
    void CreatePixmaps(bool fullFillBackground);

    void Clear(void);

    void AddText(const char *text, bool multiline, cRect position, tColor colorFg, tColor colorBg, cFont *font,
                 int textWidth = 0, int textHeight = 0, int textAlignment = taDefault);
    void AddImage(cImage *image, cRect position);
    void AddImageWithFloatedText(cImage *image, int imageAlignment, const char *text, cRect textPos, tColor colorFg,
                                 tColor colorBg, cFont *font, int textWidth = 0, int textHeight = 0,
                                 int textAlignment = taDefault);
    void AddRect(cRect position, tColor colorBg);
    bool Scrollable(int height = 0);
     int ScrollTotal(void);
     int ScrollOffset(void);
     int ScrollShown(void);
    bool Scroll(bool Up, bool Page);
    double ScrollbarSize(void);
    void SetScrollingActive(bool active) { m_IsScrollingActive = active; }

    int Height(void) { return Position.Height(); }
    int ContentHeight(bool Full);

    int BottomContent(void);

    int Top(void) { return Position.Top(); }
    void Draw();
    bool IsShown(void) { return m_IsShown; }
    bool IsScrollingActive(void) { return m_IsScrollingActive; }
};
