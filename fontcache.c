/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include <vdr/tools.h>

#include <map>
#include <string>
#include <string_view>

#include "./fontcache.h"

static GlyphMetricsCache &glyphMetricsCache() {
    static GlyphMetricsCache s_cache;
    return s_cache;
}

cFontCache FontCache;  // Global font cache object

cFontCache::~cFontCache() { Clear(); }

void cFontCache::Create() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFontCache::Create()");
#endif

    Clear();  // Clear also creates the cache with zero-initialized FontData
}

void cFontCache::Clear() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFontCache::Clear() Clearing cache");
#endif

    for (auto &data : FontCache) {
        if (data.font) {
            delete data.font;
            data.font = nullptr;
        }
        data.name = "";
        data.FileName = "";
        data.size = 0;
        data.height = 0;
        data.ascender = 0;
        data.StringWidthCache.clear();
        data.GlyphSizeCache.clear();
    }
    m_InsertIndex = 0;
}

cFont* cFontCache::GetFont(const cString &Name, int Size) {
    std::string_view NameView {*Name};
    if (NameView.empty() || Size <= 0) {  // Invalid parameters
        esyslog("flatPlus: cFontCache::GetFont() Invalid parameters: Name=%s, Size=%d", *Name, Size);
        return cFont::CreateFont("DummyFont", 16);  // Return dummy font
    }

    std::string_view DataNameView {""};
    for (const auto &data : FontCache) {
        DataNameView = *data.name;
        if (DataNameView.empty()) break;  // End of cache, insert new font

        if (DataNameView == NameView && data.size == Size && data.font != nullptr) {
#ifdef DEBUGFUNCSCALL
            dsyslog("flatPlus: Found in FontCache: Name=%s, Size=%d", *Name, Size);
#endif
            return data.font;
        }
    }

    // Font not found in cache, insert it
    InsertFont(Name, Size);
    auto &lastFont = FontCache[m_InsertIndex > 0 ? m_InsertIndex - 1 : 0];
    return lastFont.font;
}

/**
 * @brief Get the font name for a given font file name from the cache
 * @param FileName The file name of the font
 * @return The font name if found in the cache, otherwise an empty string
 */
cString cFontCache::GetFontName(const char *FileName) const {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFontCache::GetFontName() '%s'", FileName);
#endif

    for (const auto &data : FontCache) {
        if (std::string_view {*data.FileName} == FileName) {
            return data.name;  // Return the font name
        }
    }
    esyslog("flatPlus: cFontCache::GetFontName() Font name not found for file: %s", FileName);
    // If the font name is not found in the cache, return an empty string
    return "";  // Font name not found in cache
}

int cFontCache::GetFontHeight(const cString &Name, int Size) const {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFontCache::GetFontHeight() Name=%s, Size=%d", *Name, Size);
#endif

    std::string_view NameView {*Name};
    for (const auto &data : FontCache) {
        if (std::string_view {*data.name} == NameView && data.size == Size) {
            return data.height;
        }
    }
    dsyslog("flatPlus: cFontCache::GetFontHeight() Font not found in cache: Name=%s, Size=%d", *Name, Size);
    return 0;  // Font not found in cache
}

void cFontCache::InsertFont(const cString& Name, int Size) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFontCache::InsertFont() Name=%s, Size=%d", *Name, Size);
#endif

    if (isempty(*Name) || Size <= 0) return;  // Invalid parameters
    if (FontCache[m_InsertIndex].font != nullptr) {  // If the slot is already used, delete it
        dsyslog("flatPlus: cFontCache::InsertFont() Replacing existing font at index %zu", m_InsertIndex);
        delete FontCache[m_InsertIndex].font;
        FontCache[m_InsertIndex].font = nullptr;
        FontCache[m_InsertIndex].name = "";
        FontCache[m_InsertIndex].FileName = "";
        FontCache[m_InsertIndex].size = 0;
        FontCache[m_InsertIndex].height = 0;
        FontCache[m_InsertIndex].StringWidthCache.clear();
        FontCache[m_InsertIndex].GlyphSizeCache.clear();
    }
    // CreateFont() retuns a dummy font on failure
    FontCache[m_InsertIndex].font = cFont::CreateFont(*Name, Size);
    FontCache[m_InsertIndex].name = Name;
    FontCache[m_InsertIndex].FileName = FontCache[m_InsertIndex].font->FontName();
    FontCache[m_InsertIndex].size = Size;
    FontCache[m_InsertIndex].height = FontCache[m_InsertIndex].font->Height();
    #ifdef DEBUGFUNCSCALL
        dsyslog("   Font '%s' inserted at index %zu", *FontCache[m_InsertIndex].name, m_InsertIndex);
        dsyslog("   Font file name: '%s'", *FontCache[m_InsertIndex].FileName);
        dsyslog("   Font size: %d, height: %d", FontCache[m_InsertIndex].size, FontCache[m_InsertIndex].height);
    #endif

    if (++m_InsertIndex >= kMaxFontCache) {
        isyslog("flatPlus: cFontCache::InsertFont() Cache overflow, increase kMaxFontCache (%zu)", kMaxFontCache);
        m_InsertIndex = 0;
    }
}

int cFontCache::GetStringWidth(const cString &Name, int Height, const cString &Text) const {
    std::string_view NameView {*Name};
    for (auto &data : FontCache) {
        if (std::string_view {*data.name} == NameView && data.height == Height) {
            if (data.font) {
                const std::string TextStr {*Text};
                auto it = data.StringWidthCache.find(TextStr);
                if (it != data.StringWidthCache.end()) {
                    return it->second;  // Return cached width
                } else {
                    const int width {data.font->Width(*Text)};
                    data.StringWidthCache[TextStr] = width;  // Cache the width
                    return width;
                }
            }
        }
    }

    dsyslog("flatPlus: cFontCache::GetStringWidth() Font not found or invalid");
    return 0;  // Font not found or invalid
}

int cFontCache::GetCacheCount() const {
    int count {0};
    for (const auto &data : FontCache) {
        if (isempty(*data.name))
            break;
        ++count;
    }
    return count;
}

int cFontCache::GetSize() const { return FontCache.size(); }

int cFontCache::GetFontAscender(const cString &FontName, int FontSize) {
    std::string_view FontNameView {*FontName};
    if (FontNameView.empty() || FontSize <= 0) {
        esyslog("flatPlus: cFontCache::GetFontAscender() Invalid parameters: FontName=%s, FontSize=%d", *FontName,
                FontSize);
        return FontSize;
    }
    for (auto& data : FontCache) {
        if (std::string_view {*data.name} == FontNameView && data.size == FontSize) {
            if (data.ascender != 0) {
                // Return cached ascender value
                return data.ascender;
            } else {
                // Calculate and cache ascender value
                GlyphMetricsCache &cache = glyphMetricsCache();
                auto face = cache.GetFace(*cFont::GetFontFileName(FontName));
                if (!face) {
                    esyslog("flatPlus: cFontCache::GetFontAscender() FreeType error: Can't find face (Font = %s)",
                            *cFont::GetFontFileName(FontName));
                    return FontSize;
                }

                int ascender {FontSize};  // Default fallback value
                if (face->num_fixed_sizes && face->available_sizes) {  // Fixed size
                    ascender = face->available_sizes->height;
                } else if (FT_Set_Char_Size(face, FontSize * 64, FontSize * 64, 0, 0) == 0) {
                    ascender = face->size->metrics.ascender / 64;
                } else {
                    esyslog(
                        "flatPlus: cFontCache::GetFontAscender() FreeType error during FT_Set_Char_Size (Font = %s)",
                        *cFont::GetFontFileName(FontName));
                }
                data.ascender = ascender;
                return ascender;
            }
        }
    }
    dsyslog("flatPlus: cFontCache::GetFontAscender() Font not found in cache: FontName=%s, FontSize=%d", *FontName,
            FontSize);
    return FontSize;  // Default fallback if font not found
}

/**
 * @brief Get the size of a glyph in a given font and font height.
 * @param[in] Name The name of the font to use.
 * @param[in] CharCode The character code of the glyph to query.
 * @param[in] FontHeight The height of the font in pixels.
 * @return The size of the glyph in pixels, rounded up to the nearest integer.
 * @note This function returns 0 if any error occurs during the execution of the function.
 */
int cFontCache::GetGlyphSize(const cString &Name, const FT_ULong CharCode, const int FontHeight) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: GetGlyphSize() Name=%s, CharCode=%lu, FontHeight=%d", *Name, CharCode, FontHeight);
#endif

    std::string_view NameView {*Name};
    for (auto &data : FontCache) {
        if (std::string_view {*data.name} == NameView && data.size == FontHeight) {
            auto it = data.GlyphSizeCache.find({*Name, CharCode, FontHeight});
            if (it != data.GlyphSizeCache.end()) {
#ifdef DEBUGFUNCSCALL
                dsyslog("   Cache hit: GlyphSize=%d, Name=%s, CharCode=%lu, FontHeight=%d", it->second, *Name, CharCode,
                        FontHeight);
#endif
                // Return cached size
                return it->second;
            }

            GlyphMetricsCache &cache = glyphMetricsCache();
            const cString FontFileName = cFont::GetFontFileName(*Name);
            if (isempty(*FontFileName)) {
                esyslog("flatPlus: GetGlyphSize() Error: Font file name is empty for font %s", *Name);
                return 0;
            }
            auto face = cache.GetFace(*FontFileName);
            if (face == nullptr) {
                esyslog("flatPlus: GetGlyphSize() Error: Can't find face (Font = %s)", *FontFileName);
                return 0;
            }
            if (FT_Set_Char_Size(face, FontHeight * 64, FontHeight * 64, 0, 0) != 0) {
                esyslog("flatPlus: GetGlyphSize() Error: Can't set char size (Font = %s)", *FontFileName);
                return 0;
            }
            const FT_GlyphSlot slot {face->glyph};
            if (FT_Load_Glyph(face, FT_Get_Char_Index(face, CharCode), FT_LOAD_DEFAULT) != 0) {
                esyslog("flatPlus: GetGlyphSize() Error: Can't load glyph (Font = %s)", *FontFileName);
                return 0;
            }
            const int GlyphSize = (slot->metrics.height + 63) / 64;  // Round up to nearest integer
#ifdef DEBUGFUNCSCALL
            dsyslog("   Calculated GlyphSize: %d", GlyphSize);
#endif

            data.GlyphSizeCache[{*Name, CharCode, FontHeight}] = GlyphSize;
            return GlyphSize;
        }
    }
    return 0;
}
