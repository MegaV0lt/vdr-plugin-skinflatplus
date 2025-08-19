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
#include "./glyphmetricscache.h"

cFontCache FontCache;  // Global font cache object

cFontCache::~cFontCache() { Clear(); }

void cFontCache::Create() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFontCache::Create() Clearing cache");
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
        data.FontFileName = "";
        data.size = 0;
        data.height = 0;
        data.ascender = 0;
        data.StringWidthCache.clear();
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
 * @param FontFileName The file name of the font
 * @return The font name if found in the cache, otherwise an empty string
 */
cString cFontCache::GetFontName(const char *FontFileName) {
    for (const auto &data : FontCache) {
        if (std::string_view {*data.FontFileName} == std::string_view {FontFileName}) {
            return data.FontFileName;
        }
    }
    esyslog("flatPlus: cFontCache::GetFontName() Font name not found in cache: %s", FontFileName);
    // If the font name is not found in the cache, return an empty string
    return "";  // Font name not found in cache
}

int cFontCache::GetFontHeight(const cString &Name, int Size) const {
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
        FontCache[m_InsertIndex].FontFileName = "";
        FontCache[m_InsertIndex].size = 0;
        FontCache[m_InsertIndex].height = 0;
        FontCache[m_InsertIndex].StringWidthCache.clear();
    }
    // CreateFont() retuns a dummy font on failure
    FontCache[m_InsertIndex].font = cFont::CreateFont(*Name, Size);
    FontCache[m_InsertIndex].name = Name;
    FontCache[m_InsertIndex].FontFileName = FontCache[m_InsertIndex].font->FontName();
#ifdef DEBUGFUNCSCALL
    dsyslog("   Font (%s) inserted at index %zu", *FontCache[m_InsertIndex].FontFileName, m_InsertIndex);
#endif

    FontCache[m_InsertIndex].size = Size;
    FontCache[m_InsertIndex].height = FontCache[m_InsertIndex].font->Height();

    if (++m_InsertIndex >= kMaxFontCache) {
        isyslog("flatPlus: cFontCache::InsertFont() Cache overflow, increase kMaxFontCache (%zu)", kMaxFontCache);
        m_InsertIndex = 0;
    }
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

int cFontCache::GetFontAscender(const cString& FontName, int FontSize) {
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
                data.ascender = CalculateFontAscender(FontName, FontSize);
                return data.ascender;
            }
        }
    }
    dsyslog("flatPlus: cFontCache::GetFontAscender() Font not found in cache: FontName=%s, FontSize=%d", *FontName,
            FontSize);
    return FontSize;  // Default fallback if font not found
}

int cFontCache::CalculateFontAscender(const cString& FontName, int FontSize) const {
    GlyphMetricsCache& cache = glyphMetricsCache();
    FT_Face face = cache.GetFace(*cFont::GetFontFileName(FontName));
    if (!face) {
        esyslog("flatPlus: cFontCache::GetFontAscender() FreeType error: Can't find face (Font = %s)",
                *cFont::GetFontFileName(FontName));
        return FontSize;
    }

    int ascender = FontSize;
    if (face->num_fixed_sizes && face->available_sizes) {  // Fixed size
        ascender = face->available_sizes->height;
    } else if (FT_Set_Char_Size(face, FontSize * 64, FontSize * 64, 0, 0) == 0) {
        ascender = face->size->metrics.ascender / 64;
    } else {
        esyslog("flatPlus: cFontCache::GetFontAscender() FreeType error during FT_Set_Char_Size (Font = %s)",
                *cFont::GetFontFileName(FontName));
    }

    return ascender;
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
