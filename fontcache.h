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
#include <mutex>  // NOLINT std::mutex
#include <string>
#include <tuple>
#include <unordered_map>

// Expensive FreeType Library/Face Handling (cache faces)
class GlyphMetricsCache {
 public:
    GlyphMetricsCache() {
        FT_Init_FreeType(&library_);
    }
    ~GlyphMetricsCache() {
        for (auto &pair : faces_) {
            FT_Done_Face(pair.second);
        }
        FT_Done_FreeType(library_);
    }

    FT_Face GetFace(const std::string &FontFileName) {
        std::unique_lock lock(mutex_);
        auto it = faces_.find(FontFileName);
        if (it != faces_.end())
            return it->second;
        FT_Face face {nullptr};
        if (FT_New_Face(library_, FontFileName.c_str(), 0, &face)) {
            esyslog("flatPlus: GlyphMetricsCache: FT_New_Face failed for '%s'", FontFileName.c_str());
            return nullptr;
        }
        faces_[FontFileName] = face;
        return face;
    }

 private:
    FT_Library library_ {nullptr};
    std::unordered_map<std::string, FT_Face> faces_;
    std::mutex mutex_;
};  // class GlyphMetricsCache

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
        mutable std::map<std::tuple<std::string, FT_ULong, int>, int> GlyphSizeCache;  // Cache for glyph sizes
    };

    static constexpr std::size_t kMaxFontCache {32};
    std::array<FontData, kMaxFontCache> FontCache {};  // Zero-initialized
    std::size_t m_InsertIndex {0};

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
    int GetCacheCount() const;  // Number of cached FontData
    int GetSize() const;        // Total size of the FontCache
    int GetFontAscender(const cString& FontName, int FontSize);
    int GetGlyphSize(const cString &Name, const FT_ULong CharCode, const int FontHeight);
};

extern cFontCache FontCache;
