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
#include <string>

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

// Accessor:
GlyphMetricsCache &glyphMetricsCache();
