/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/font.h>

#include <array>
#include <map>
#include <string>

class cFontCache {
 private:
    struct FontData {
        cFont *font {nullptr};
        cString name {""};
        int size {0};
        int height {0};
        int ascender;  // Ascender for the font, used for vertical alignment
        mutable std::map<std::string, int> StringWidthCache;  // Cache for string widths
    };

    static constexpr std::size_t MaxFontCache {32};
    std::array<FontData, MaxFontCache> FontCache {};  // Zero-initialized
    std::size_t m_InsertIndex {0};

 public:
    cFontCache() = default;
    ~cFontCache();

    void Create();
    void Clear();
    cFont* GetFont(const cString &Name, int Size);
    int GetFontHeight(const cString &Name, int Size) const;
    void InsertFont(const cString &Name, int Size);
    int GetCacheCount() const;
    int GetFontAscender(const cString& FontName, int FontSize);
    int CalculateFontAscender(const cString& FontName, int FontSize) const;
    int GetStringWidth(const cString &Name, int Height, const cString &Text) const;
};

extern cFontCache FontCache;
