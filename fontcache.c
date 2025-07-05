/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include <vdr/tools.h>

#include <string_view>

#include "./fontcache.h"

cFontCache FontCache;

cFontCache::~cFontCache() {
    Clear();
}

void cFontCache::Create() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFontCache::Create() Clearing cache");
#endif

    Clear();  // Clear the cache on creation
}

void cFontCache::Clear() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFontCache::Clear() Clearing cache");
#endif

    for (auto& data : FontCache) {
        if (data.font) {
            delete data.font;
            data.font = nullptr;
        }
        data.name = "";
        data.size = -1;
    }
    m_InsertIndex = 0;
}

cFont* cFontCache::GetFont(const cString& Name, int Size) {
    std::string_view NameView {*Name};  // Convert cString to std::string_view for easier handling
    if (NameView.empty() || Size <= 0) {  // Invalid parameters
        esyslog("flatPlus: FontCache - Invalid parameters: Name=%s, Size=%d", *Name, Size);
        return nullptr;
    }

    std::string_view DataNameView {};  // Use std::string_view for comparison
    for (const auto &data : FontCache) {
        DataNameView = *data.name;
        if (DataNameView.empty())
            break;
        if (DataNameView == NameView && data.size == Size && data.font != nullptr) {
#ifdef DEBUGFUNCSCALL
            dsyslog("flatPlus: Font found in FontCache: Name=%s, Size=%d", *Name, Size);
#endif
            return data.font;
        }
    }

    // Font not found in cache, insert it
    InsertFont(Name, Size);
    auto& lastFont = FontCache[m_InsertIndex > 0 ? m_InsertIndex - 1 : 0];
    return lastFont.font;
}

void cFontCache::InsertFont(const cString& Name, int Size) {
    if (FontCache[m_InsertIndex].font != nullptr) {  // If the font already exists, delete it
        dsyslog("flatPlus: FontCache - Replacing existing font at index %zu", m_InsertIndex);
        delete FontCache[m_InsertIndex].font;
        FontCache[m_InsertIndex].font = nullptr;
        FontCache[m_InsertIndex].name = "";
        FontCache[m_InsertIndex].size = -1;
    }

    FontCache[m_InsertIndex].font = cFont::CreateFont(*Name, Size);
    if (!FontCache[m_InsertIndex].font) {
        esyslog("flatPlus: FontCache - Failed to create font %s size %d", *Name, Size);
        return;
    }

    FontCache[m_InsertIndex].name = Name;
    FontCache[m_InsertIndex].size = Size;

    if (++m_InsertIndex >= MaxFontCache) {
        isyslog("flatPlus: FontCache overflow, increase MaxFontCache (%zu)", MaxFontCache);
        m_InsertIndex = 0;
    }
}

int cFontCache::GetCacheCount() const {
    int count{0};
    for (const auto& data : FontCache) {
        if (isempty(*data.name))
            break;
        ++count;
    }
    return count;
}
