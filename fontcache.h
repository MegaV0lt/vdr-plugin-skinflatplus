/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/font.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <array>
#include <map>
#include <string>

class cFontCache {
 private:
    struct FontData {
        cFont *font {nullptr};
        cString name {""};      // Name of the font as used in cFont::CreateFont()
        cString FileName {""};  // Name of the font file
        int size {0};           // Size of the font (% of osd height)
        int height {0};         // Height of the font in pixels
        int ascender {0};       // Ascender for the font, used for vertical alignment
        mutable std::map<std::string, int> StringWidthCache;  // Cache for string widths
    };

    static constexpr std::size_t kMaxFontCache {32};
    std::array<FontData, kMaxFontCache> FontCache {};  // Zero-initialized
    std::size_t m_InsertIndex {0};

    int CalculateFontAscender(const cString& FontName, int FontSize) const;

 public:
    cFontCache() = default;
    ~cFontCache();

    void Create();
    void Clear();
    cFont* GetFont(const cString &Name, int Size);
    cString GetFontName(const char *FileName) const;
    int GetFontHeight(const cString &Name, int Size) const;
    void InsertFont(const cString &Name, int Size);
    int GetStringWidth(const cString &Name, int Height, const cString &Text) const;
    int GetCacheCount() const;
    int GetSize() const;
    int GetFontAscender(const cString& FontName, int FontSize);
    uint32_t GetGlyphSize(const cString &Name, const FT_ULong CharCode, const int FontHeight);
};

extern cFontCache FontCache;
