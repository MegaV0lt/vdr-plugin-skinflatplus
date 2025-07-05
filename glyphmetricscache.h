/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <unordered_map>
#include <mutex>  // NOLINT std::mutex

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
    FT_Face GetFace(const cString& FontName) {
        std::unique_lock lock(mutex_);
        auto it = faces_.find(*FontName);
        if (it != faces_.end())
            return it->second;
        FT_Face face {nullptr};
        if (FT_New_Face(library_, *FontName, 0, &face))
            return nullptr;
        faces_[FontName] = face;
        return face;
    }

 private:
    FT_Library library_ {nullptr};
    std::unordered_map<cString, FT_Face> faces_;
    std::mutex mutex_;
};  // class GlyphMetricsCache

// Zugriffsfunktion:
GlyphMetricsCache &glyphMetricsCache();
