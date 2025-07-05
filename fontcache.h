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

class cFontCache {
 private:
    struct FontData {
        cFont *font {nullptr};
        cString name {""};  // Empty string initialized
        int size {-1};
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
    void InsertFont(const cString &Name, int Size);
    int GetCacheCount() const;
};

extern cFontCache FontCache;
