/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displaymenu.h"

#include <fstream>
#include <future>  // NOLINT
#include <iostream>
#include <utility>
#include <sstream>
#include <locale>

#include "./services/epgsearch.h"
#include "./services/scraper2vdr.h"

#include "./flat.h"
// #include "./locale"

#ifndef VDRLOGO
#define VDRLOGO "vdrlogo_default"
#endif

static int CompareTimers(const void *a, const void *b) {
    return (*(const cTimer **)a)->Compare(**(const cTimer **)b);
}

cFlatDisplayMenu::cFlatDisplayMenu() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::cFlatDisplayMenu()");
#endif

    CreateFullOsd();
    TopBarCreate();
    ButtonsCreate();
    MessageCreate();

    // m_FontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);  // Top of capital letter

    m_RecFolder.reserve(256);
    m_LastRecFolder.reserve(256);

    m_VideoDiskUsageState = -1;

    m_ItemHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
    m_ItemChannelHeight = m_ItemHeight;
    m_ItemTimerHeight = m_ItemHeight;

    m_WidthScrollBar = ScrollBarWidth() + m_MarginItem;
    m_ScrollBarHeight = m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_MarginItem3 +
                                       m_ButtonsHeight + Config.decorBorderButtonSize * 2);
    m_ScrollBarTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2;

    m_MenuWidth = m_OsdWidth;
    m_MenuTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize;
    MenuPixmap = CreatePixmap(m_Osd, "MenuPixmap", 1, cRect(0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight));
    MenuIconsBgPixmap = CreatePixmap(m_Osd, "MenuIconsBgPixmap", 2,
                                     cRect(0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight));
    MenuIconsPixmap = CreatePixmap(m_Osd, "MenuIconsPixmap", 3, cRect(0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight));
    MenuIconsOvlPixmap =
        CreatePixmap(m_Osd, "MenuIconsOvlPixmap", 4, cRect(0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight));

    m_chLeft = Config.decorBorderMenuContentHeadSize;
    m_chTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentHeadSize;
    m_chWidth = m_MenuWidth - Config.decorBorderMenuContentHeadSize * 2;
    m_chHeight = m_FontHeight + m_FontSmlHeight * 2 + m_MarginItem2;
    ContentHeadPixmap = CreatePixmap(m_Osd, "ContentHeadPixmap", 1, cRect(m_chLeft, m_chTop, m_chWidth, m_chHeight));
    ContentHeadIconsPixmap =
        CreatePixmap(m_Osd, "ContentHeadIconsPixmap", 2, cRect(m_chLeft, m_chTop, m_chWidth, m_chHeight));

    ScrollbarPixmap = CreatePixmap(
        m_Osd, "ScrollbarPixmap", 2,
        cRect(0, m_ScrollBarTop, m_MenuWidth, m_ScrollBarHeight + m_ButtonsHeight + Config.decorBorderButtonSize * 2));

    PixmapFill(MenuPixmap, clrTransparent);
    PixmapFill(MenuIconsPixmap, clrTransparent);
    PixmapFill(MenuIconsBgPixmap, clrTransparent);
    PixmapFill(MenuIconsOvlPixmap, clrTransparent);
    PixmapFill(ContentHeadIconsPixmap, clrTransparent);
    PixmapFill(ScrollbarPixmap, clrTransparent);

    MenuItemScroller.SetOsd(m_Osd);
    MenuItemScroller.SetScrollStep(Config.ScrollerStep);
    MenuItemScroller.SetScrollDelay(Config.ScrollerDelay);
    MenuItemScroller.SetScrollType(Config.ScrollerType);

    ItemsBorder.reserve(64);

    // Call to get values for 'DiskUsage' and have it outside of SetItem()
    cVideoDiskUsage::HasChanged(m_VideoDiskUsageState);
}

cFlatDisplayMenu::~cFlatDisplayMenu() {
    MenuItemScroller.Clear();
    if (m_FontTempSml)  // Created in 'DrawMainMenuWidgetWeather()'
        delete m_FontTempSml;

    if (m_FontTiny)  // Created in 'AddActors()'
        delete m_FontTiny;

    // if (m_Osd) {
        m_Osd->DestroyPixmap(MenuPixmap);
        m_Osd->DestroyPixmap(MenuIconsPixmap);
        m_Osd->DestroyPixmap(MenuIconsBgPixmap);
        m_Osd->DestroyPixmap(MenuIconsOvlPixmap);
        m_Osd->DestroyPixmap(ScrollbarPixmap);
        m_Osd->DestroyPixmap(ContentHeadPixmap);
        m_Osd->DestroyPixmap(ContentHeadIconsPixmap);
    // }
}

void cFlatDisplayMenu::SetMenuCategory(eMenuCategory MenuCategory) {
    MenuItemScroller.Clear();
    ItemBorderClear();
    m_IsScrolling = false;
    m_ShowRecording = m_ShowEvent = m_ShowText = false;

    m_MenuCategory = MenuCategory;
    switch (MenuCategory) {
    case mcChannel:
        if (Config.MenuChannelView <= 2)  // 0 = VDR default, 1 = flatPlus long, 2 = flatPlus long + EPG
            m_ItemChannelHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
        else  // 3 = flatPlus short, 4 = flatPlus short + EPG
            m_ItemChannelHeight = m_FontHeight + m_FontSmlHeight + m_MarginItem + Config.decorProgressMenuItemSize +
                                  Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
        break;
    case mcTimer:
        if (Config.MenuTimerView <= 1)  // 0 = VDR default, 1 = flatPlus long
            m_ItemTimerHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
        else  // 2 = flatPlus short, 3 = flatPlus short + EPG
            m_ItemTimerHeight = m_FontHeight + m_FontSmlHeight + m_MarginItem + Config.MenuItemPadding +
                                Config.decorBorderMenuItemSize * 2;
        break;
    case mcSchedule:
    case mcScheduleNow:
    case mcScheduleNext:
        if (Config.MenuEventView <= 1)  // 0 = VDR default, 1 = flatPlus long
            m_ItemEventHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
        else  // 2 = flatPlus short, 3 = flatPlus short + EPG
            m_ItemEventHeight = m_FontHeight + m_FontSmlHeight + m_MarginItem2 + Config.MenuItemPadding +
                                Config.decorBorderMenuItemSize * 2 + Config.decorProgressMenuItemSize / 2;
        break;
    case mcRecording:
        if (Config.MenuRecordingView <= 1)  // 0 = VDR default, 1 = flatPlus long
            m_ItemRecordingHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
        else  // 2 = flatPlus short, 3 = flatPlus short + EPG
            m_ItemRecordingHeight = m_FontHeight + m_FontSmlHeight + m_MarginItem + Config.MenuItemPadding +
                                    Config.decorBorderMenuItemSize * 2;
        break;
    case mcMain:
        if (Config.MainMenuWidgetsEnable)
            DrawMainMenuWidgets();
        break;
    default:
        break;
    }
}

void cFlatDisplayMenu::SetScrollbar(int Total, int Offset) {
    const int ItemsMax {MaxItems()};  // Cache value
    DrawScrollbar(Total, Offset, ItemsMax, 0, ItemsHeight(), Offset > 0, Offset + ItemsMax < Total);
}

void cFlatDisplayMenu::DrawScrollbar(int Total, int Offset, int Shown, int Top, int Height, bool CanScrollUp,
                                     bool CanScrollDown, bool IsContent) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawScrollbar()");
    dsyslog("   Total: %d Offset: %d Shown: %d Top: %d Height: %d", Total, Offset, Shown, Top, Height);
#endif

    if (!MenuPixmap) return;

    if (Total > 0 && Total > Shown) {
        if (!m_IsScrolling && !(m_ShowEvent || m_ShowRecording || m_ShowText)) {
            m_IsScrolling = true;
            DecorBorderClearByFrom(BorderMenuItem);
            ItemBorderDrawAllWithScrollbar();
            ItemBorderClear();

            MenuItemScroller.UpdateViewPortWidth(m_WidthScrollBar);

            if (IsContent)
                MenuPixmap->DrawRectangle(cRect(m_MenuItemWidth - m_WidthScrollBar + Config.decorBorderMenuContentSize,
                                                0, m_WidthScrollBar + m_MarginItem, m_ScrollBarHeight),
                                          clrTransparent);
            else
                MenuPixmap->DrawRectangle(cRect(m_MenuItemWidth - m_WidthScrollBar + Config.decorBorderMenuItemSize, 0,
                                                m_WidthScrollBar + m_MarginItem, m_ScrollBarHeight),
                                          clrTransparent);
        }
    } else if (!(m_ShowEvent || m_ShowRecording || m_ShowText)) {
        m_IsScrolling = false;
    }

    if (IsContent)
        ScrollbarDraw(ScrollbarPixmap,
                      m_MenuItemWidth - m_WidthScrollBar + Config.decorBorderMenuContentSize * 2 + m_MarginItem, Top,
                      Height, Total, Offset, Shown, CanScrollUp, CanScrollDown);
    else
        ScrollbarDraw(ScrollbarPixmap,
                      m_MenuItemWidth - m_WidthScrollBar + Config.decorBorderMenuItemSize * 2 + m_MarginItem, Top,
                      Height, Total, Offset, Shown, CanScrollUp, CanScrollDown);
}

void cFlatDisplayMenu::Scroll(bool Up, bool Page) {
    // Is the menu scrolling or content?
    if (ComplexContent.IsShown() && ComplexContent.IsScrollingActive() && ComplexContent.Scrollable()) {
        const bool IsScrolled {ComplexContent.Scroll(Up, Page)};
        if (IsScrolled) {
            const int ScrollOffset {ComplexContent.ScrollOffset()};  // Cache value
            DrawScrollbar(
                ComplexContent.ScrollTotal(), ScrollOffset, ComplexContent.ScrollShown(),
                ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ScrollOffset > 0,
                ScrollOffset + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);
        }
    } else {
        cSkinDisplayMenu::Scroll(Up, Page);
    }
}

int cFlatDisplayMenu::MaxItems() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::MaxItems()");
#endif
    int ItemHeight {m_ItemHeight};
    switch (m_MenuCategory) {
    case mcChannel:
        ItemHeight = m_ItemChannelHeight;
        break;
    case mcTimer:
        ItemHeight = m_ItemTimerHeight;
        break;
    case mcSchedule:
    case mcScheduleNow:
    case mcScheduleNext:
        ItemHeight = m_ItemEventHeight;
        break;
    case mcRecording:
        ItemHeight = m_ItemRecordingHeight;
        break;
    default:
        break;
    }
    return m_ScrollBarHeight / ItemHeight;  //? Avoid DIV/0
}

int cFlatDisplayMenu::ItemsHeight() {
    switch (m_MenuCategory) {
    case mcChannel:
        return MaxItems() * m_ItemChannelHeight - Config.MenuItemPadding;
    case mcTimer:
        return MaxItems() * m_ItemTimerHeight - Config.MenuItemPadding;
    case mcSchedule:
    case mcScheduleNow:
    case mcScheduleNext:
        return MaxItems() * m_ItemEventHeight - Config.MenuItemPadding;
    case mcRecording:
        return MaxItems() * m_ItemRecordingHeight - Config.MenuItemPadding;
    default:
        return MaxItems() * m_ItemHeight - Config.MenuItemPadding;
    }
}

void cFlatDisplayMenu::Clear() {
    MenuItemScroller.Clear();
    PixmapFill(MenuPixmap, clrTransparent);
    PixmapFill(MenuIconsPixmap, clrTransparent);
    PixmapFill(MenuIconsBgPixmap, clrTransparent);
    PixmapFill(MenuIconsOvlPixmap, clrTransparent);
    PixmapFill(ScrollbarPixmap, clrTransparent);
    PixmapFill(ContentHeadPixmap, clrTransparent);
    PixmapFill(ContentHeadIconsPixmap, clrTransparent);
    DecorBorderClearByFrom(BorderMenuItem);
    DecorBorderClearByFrom(BorderContent);
    DecorBorderClearByFrom(BorderMMWidget);
    DecorBorderClearAll();
    m_IsScrolling = false;

    m_MenuItemLastHeight = 0;
    m_MenuFullOsdIsDrawn = false;

    ComplexContent.Clear();
    ContentWidget.Clear();
    TopBarClearMenuIconRight();

    m_ShowRecording = m_ShowEvent = m_ShowText = false;
}

void cFlatDisplayMenu::SetTitle(const char *Title) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetTitle() '%s' m_MenuCategory %d", Title, m_MenuCategory);
#endif

    m_LastTitle = Title;

    cString IconName{""}, NewTitle = Title;
    switch (m_MenuCategory) {
    case mcMain:
        IconName = cString::sprintf("menuIcons/%s", VDRLOGO);
        NewTitle = "";
        break;
    case mcSchedule:
    case mcScheduleNow:
    case mcScheduleNext: IconName = "menuIcons/Schedule"; break;
    case mcChannel:
        IconName = "menuIcons/Channels";
        if (Config.MenuChannelShowCount) {
            uint ChanCount {0};
            LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
            for (const cChannel *Channel = Channels->First(); Channel; Channel = Channels->Next(Channel)) {
                if (!Channel->GroupSep()) ++ChanCount;
            }
            NewTitle = cString::sprintf("%s (%d)", Title, ChanCount);
        }  // Config.MenuChannelShowCount
        break;
    case mcTimer:
        IconName = "menuIcons/Timers";
        if (Config.MenuTimerShowCount) {
            GetTimerCounts(m_LastTimerActiveCount, m_LastTimerCount);  // Update timer counts
            NewTitle = cString::sprintf("%s (%d/%d)", Title, m_LastTimerActiveCount, m_LastTimerCount);
        }
        break;
    case mcRecording:
        IconName = "menuIcons/Recordings";
        if (Config.MenuRecordingShowCount) { NewTitle = cString::sprintf("%s %s", Title, *GetRecCounts()); }
        break;
    case mcSetup: IconName = "menuIcons/Setup"; break;
    case mcCommand: IconName = "menuIcons/Commands"; break;
    case mcEvent: IconName = "extraIcons/Info"; break;
    case mcRecordingInfo: IconName = "extraIcons/PlayInfo"; break;
    default: break;
    }  // switch (m_MenuCategory)

    TopBarSetTitle(*NewTitle);  // Must be called before other TopBarSet*

    if (Config.TopBarMenuIconShow)
        TopBarSetMenuIcon(*IconName);

    // Show disk usage in Top Bar if:
    // - in Recording or Timer menu and Config.DiskUsageShow > 0
    // - in any menu and Config.DiskUsageShow > 1
    // - always when Config.DiskUsageShow == 3 (Handled in TopBarCreate() and TopBarSetTitle())
    if (((m_MenuCategory == mcRecording || m_MenuCategory == mcTimer) && (Config.DiskUsageShow > 0)) ||
        ((m_MenuCategory && (Config.DiskUsageShow > 1))))
        TopBarEnableDiskUsage();
}

void cFlatDisplayMenu::SetButtons(const char *Red, const char *Green, const char *Yellow, const char *Blue) {
    ButtonsSet(Red, Green, Yellow, Blue);
}

void cFlatDisplayMenu::SetMessage(eMessageType Type, const char *Text) {
    (Text) ? MessageSet(Type, Text) : MessageClear();
}

void cFlatDisplayMenu::SetItem(const char *Text, int Index, bool Current, bool Selectable) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetItem() '%s'", Text);
#endif
    if (!MenuPixmap || !MenuIconsPixmap) return;

    m_ShowEvent = false;
    m_ShowRecording = false;
    m_ShowText = false;

    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (m_MenuCategory == mcMain && Config.MainMenuWidgetsEnable)
        m_MenuItemWidth *= Config.MainMenuItemScale;

    if (m_IsScrolling)
        m_MenuItemWidth -= m_WidthScrollBar;

    tColor ColorFg = Theme.Color(clrItemFont);
    tColor ColorBg = Theme.Color(clrItemBg);
    tColor ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextFont);
    if (Current) {
        ColorFg = Theme.Color(clrItemCurrentFont);
        ColorBg = Theme.Color(clrItemCurrentBg);
        ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextCurrentFont);
    } else if (Selectable) {
        ColorFg = Theme.Color(clrItemSelableFont);
        ColorBg = Theme.Color(clrItemSelableBg);
    }

    const int y {Index * m_ItemHeight};
    if (y + m_ItemHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, m_FontHeight), ColorBg);

    const int AvailableTextWidth {m_MenuItemWidth - m_WidthScrollBar};
    int XOff {0}, xt {0};
    const char *s {nullptr};
    for (uint i {0}; i < MaxTabs; ++i) {
        s = GetTabbedText(Text, i);
        if (s) {
            // From skinelchi
            xt = Tab(i);
            XOff = xt + Config.decorBorderMenuItemSize;

            // Check for timer info symbols: " !#>" (EPGSearch search timer)
            if (i == 0 && strlen(s) == 1 && strchr(" !#>", s[0])) {
                cImage *img {nullptr};
                switch (s[0]) {
                case '>':
                    if (Current) {
                        img = ImgLoader.LoadIcon("text_timer_full_cur", m_FontHeight, m_FontHeight);
                    } else if (Selectable) {
                        img = ImgLoader.LoadIcon("text_timer_full_sel", m_FontHeight, m_FontHeight);
                    } else {
                        img = ImgLoader.LoadIcon("text_timer_full", m_FontHeight, m_FontHeight);
                    }
                    if (img)
                        MenuIconsPixmap->DrawImage(cPoint(XOff, y + (m_FontHeight - img->Height()) / 2),
                                                   *img);
                    break;
                case '#':
                    if (Current) {
                        img = ImgLoader.LoadIcon("timerRecording_cur", m_FontHeight, m_FontHeight);
                    } else if (Selectable) {
                        img = ImgLoader.LoadIcon("timerRecording_sel", m_FontHeight, m_FontHeight);
                    } else {
                        img = ImgLoader.LoadIcon("timerRecording", m_FontHeight, m_FontHeight);
                    }
                    if (img)
                        MenuIconsPixmap->DrawImage(cPoint(XOff, y + (m_FontHeight - img->Height()) / 2),
                                                   *img);
                    break;
                case '!':
                    if (Current) {
                        img = ImgLoader.LoadIcon("text_arrowturn_cur", m_FontHeight, m_FontHeight);
                    } else if (Selectable) {
                        img = ImgLoader.LoadIcon("text_arrowturn_sel", m_FontHeight, m_FontHeight);
                    } else {
                        img = ImgLoader.LoadIcon("text_arrowturn", m_FontHeight, m_FontHeight);
                    }
                    if (img)
                        MenuIconsPixmap->DrawImage(cPoint(XOff, y + (m_FontHeight - img->Height()) / 2),
                                                   *img);
                    break;
                // case ' ':
                default:
                    break;
                }
            } else {  // Not EPGsearch timer menu
                if ((m_MenuCategory == mcMain || m_MenuCategory == mcSetup) && Config.MenuItemIconsShow) {
                    const cString IconName = *GetIconName(*MainMenuText(s));
                    cImage *img {nullptr};
                    if (Current) {
                        const cString IconNameCur = cString::sprintf("%s_cur", *IconName);
                        img = ImgLoader.LoadIcon(*IconNameCur, m_FontHeight - m_MarginItem2,
                                                 m_FontHeight - m_MarginItem2);
                    }
                    if (!img)
                        img = ImgLoader.LoadIcon(*IconName, m_FontHeight - m_MarginItem2,
                                                 m_FontHeight - m_MarginItem2);

                    if (img) {
                        MenuIconsPixmap->DrawImage(
                            cPoint(xt + Config.decorBorderMenuItemSize + m_MarginItem, y + m_MarginItem), *img);
                    } else {
                        img = ImgLoader.LoadIcon("menuIcons/blank", m_FontHeight - m_MarginItem2,
                                                 m_FontHeight - m_MarginItem2);
                        if (img) {
                            MenuIconsPixmap->DrawImage(
                                cPoint(xt + Config.decorBorderMenuItemSize + m_MarginItem, y + m_MarginItem), *img);
                        }
                    }
                    MenuPixmap->DrawText(
                        cPoint(m_FontHeight + m_MarginItem2 + xt + Config.decorBorderMenuItemSize, y), s, ColorFg,
                        ColorBg, m_Font, AvailableTextWidth - xt - m_MarginItem2 - m_FontHeight);
                } else {
                    if (Config.MenuItemParseTilde) {
                        const char *TildePos {strchr(s, '~')};

                        if (TildePos) {
                            std::string_view sv1 {s, static_cast<size_t>(TildePos - s)};
                            std::string_view sv2 {TildePos + 1};

                            const std::string first {rtrim(sv1)};   // Trim possible space at end
                            const std::string second {ltrim(sv2)};  // Trim possible space at begin

                            MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), first.c_str(), ColorFg,
                                                 ColorBg, m_Font,
                                                 m_MenuItemWidth - xt - Config.decorBorderMenuItemSize);
                            const int l {m_Font->Width(first.c_str()) + m_Font->Width('X')};
                            MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize + l, y), second.c_str(),
                                                 ColorExtraTextFg, ColorBg, m_Font,
                                                 m_MenuItemWidth - xt - Config.decorBorderMenuItemSize - l);
                        } else {  // ~ not found
                            MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), s, ColorFg, ColorBg,
                                                 m_Font, m_MenuItemWidth - xt - Config.decorBorderMenuItemSize);
                        }
                    } else {  // MenuItemParseTilde disabled
                        MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), s, ColorFg, ColorBg,
                                             m_Font, m_MenuItemWidth - xt - Config.decorBorderMenuItemSize);
                    }
                }
            }  // Not EPGsearch searchtimer
        }      // if (s)
        if (!Tab(i + 1))
            break;
    }  // for

    sDecorBorder ib {
        .Left = Config.decorBorderMenuItemSize,
        .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y,
        .Width = m_MenuItemWidth,
        .Height = m_FontHeight,
        .Size = Config.decorBorderMenuItemSize,
        .Type = Config.decorBorderMenuItemType
    };

    if (Current) {
        ib.ColorFg = Config.decorBorderMenuItemCurFg;
        ib.ColorBg = Config.decorBorderMenuItemCurBg;
    } else if (Selectable) {
        ib.ColorFg = Config.decorBorderMenuItemSelFg;
        ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
        ib.ColorFg = Config.decorBorderMenuItemFg;
        ib.ColorBg = Config.decorBorderMenuItemBg;
    }

    ib.From = BorderMenuItem;
    DecorBorderDraw(ib);

    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    SetEditableWidth(m_MenuWidth - Tab(1));
}

cString cFlatDisplayMenu::MainMenuText(const cString &Text) {
    const std::string text {skipspace(*Text)};
    bool found {false};
    const std::size_t TextLength {text.length()};
    uint i {0};  // 'i' used also after loop
    char s;
    for (; i < TextLength; ++i) {
        s = text.at(i);
        if (isdigit(s) && i < 5)  // Up to 4 digits expected
            found = true;
        else
            break;
    }

    return found ? skipspace(text.substr(i).c_str()) : text.c_str();
}

cString cFlatDisplayMenu::GetIconName(const std::string &element) {
    // Check for standard menu entries
    std::string s {""};
    s.reserve(32);  // Space for translated menu entry
    for (uint i {0}; i < 16; ++i) {  // 16 menu entry's in vdr
        s = trVDR(*items[i]);
        if (s == element) {
            return cString::sprintf("menuIcons/%s", *items[i]);
        }
    }

    // Check for special main menu entries "stop recording", "stop replay"
    const std::string StopRecording {skipspace(trVDR(" Stop recording "))};
    const std::string StopReplay {skipspace(trVDR(" Stop replaying"))};
    if (element.compare(0, StopRecording.size(), StopRecording) == 0)
        return "menuIcons/StopRecording";
    if (element.compare(0, StopReplay.size(), StopReplay) == 0)
        return "menuIcons/StopReplay";

    // Check for plugins
    const char *MainMenuEntry {nullptr};
    std::string PlugMainEntry {""};
    PlugMainEntry.reserve(32);  // Space for menu entry
    for (uint i {0};; ++i) {
        cPlugin *p = cPluginManager::GetPlugin(i);
        if (p) {
            MainMenuEntry = p->MainMenuEntry();
            if (MainMenuEntry) {
                PlugMainEntry = MainMenuEntry;
                if (element.compare(0, PlugMainEntry.size(), PlugMainEntry) == 0) {
                    return cString::sprintf("pluginIcons/%s", p->Name());
                }
            }
        } else {
            break;  // Plugin not found
        }
    }

    return cString::sprintf("extraIcons/%s", element.c_str());
}

bool cFlatDisplayMenu::CheckProgressBar(const char *text) {
    const std::size_t TextLength {strlen(text)};
    if (text[0] == '[' && TextLength > 5 && ((text[1] == '|') || (text[1] == ' ')) && text[TextLength - 1] == ']')
        return true;

    return false;
}

void cFlatDisplayMenu::DrawProgressBarFromText(cRect rec, cRect recBg, const char *bar, tColor ColorFg,
                                               tColor ColorBarFg, tColor ColorBg) {
    const char *p {bar + 1};
    uint now {0}, total {0};
    while (*p != ']') {
        if (*p == '|')
            ++now;
        ++total;
        ++p;
    }
    if (total > 0) {
        ProgressBarDrawRaw(MenuPixmap, MenuPixmap, rec, recBg, now, total, ColorFg, ColorBarFg, ColorBg,
                           Config.decorProgressMenuItemType, true);
    }
}

bool cFlatDisplayMenu::SetItemChannel(const cChannel *Channel, int Index, bool Current, bool Selectable,
                                      bool WithProvider) {
    if (!MenuPixmap || !MenuIconsPixmap || !MenuIconsBgPixmap)
        return false;

    if (Config.MenuChannelView == 0 || !Channel)
        return false;

    if (Current)
        MenuItemScroller.Clear();

    int Height {m_FontHeight};
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4) {  // flatPlus short, flatPlus short +EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem + Config.decorProgressMenuItemSize;
        m_MenuItemWidth /= 2;
    }

    if (m_IsScrolling)
        m_MenuItemWidth -= m_WidthScrollBar;

    tColor ColorFg = Theme.Color(clrItemFont);
    tColor ColorBg = Theme.Color(clrItemBg);
    if (Current) {
        ColorFg = Theme.Color(clrItemCurrentFont);
        ColorBg = Theme.Color(clrItemCurrentBg);
    } else if (Selectable) {
        ColorFg = Theme.Color(clrItemSelableFont);
        ColorBg = Theme.Color(clrItemSelableBg);
    }

    const int y {Index * m_ItemChannelHeight};
    if (y + m_ItemChannelHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemChannelHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);

    int Width {0};
    int Left {Config.decorBorderMenuItemSize + m_MarginItem};
    int Top {y};
    bool DrawProgress {true};
    const bool IsGroup {Channel->GroupSep()};  // Also used later

    /* Disabled because invalid lock sequence
    LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
    cString ws = cString::sprintf("%d", Channels->MaxNumber());
    int w = m_Font->Width(ws); */  // Try to fix invalid lock sequence (Only with scraper2vdr - Program)

    cString Buffer {""};
    if (IsGroup) {
        DrawProgress = false;
    } else {
        Buffer = cString::sprintf("%d", Channel->Number());
        Width = m_Font->Width(*Buffer);
    }

    //* At least four digits width in channel list because of different sort modes
    Width = std::max(m_Font->Width("9999"), Width);

    MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, Width, m_FontHeight, taRight);
    Left += Width + m_MarginItem;

    int ImageLeft {Left};
    int ImageTop {Top};
    int ImageHeight {m_FontHeight};
    int ImageBgWidth = ImageHeight * 1.34f;  // Narrowing conversion
    int ImageBgHeight {ImageHeight};

    cImage *img {nullptr};
    if (!IsGroup) {
        img = ImgLoader.LoadIcon("logo_background", ImageBgWidth, ImageBgHeight);
        if (img) {
            ImageBgHeight = img->Height();
            ImageBgWidth = img->Width();
            ImageTop = Top + (m_FontHeight - ImageBgHeight) / 2;
            MenuIconsBgPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }
        // Load named logo only for channels
        img = ImgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
    }

    if (img) {
        ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
        ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;

        MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        Left += ImageBgWidth + m_MarginItem2;
    } else {
        const bool IsRadioChannel {((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false};
        if (IsRadioChannel) {
            if (Current)
                img = ImgLoader.LoadIcon("radio_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img)
                img = ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
            if (img) {
                ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                Left += ImageBgWidth + m_MarginItem2;
            }
        } else if (IsGroup) {
            img = ImgLoader.LoadIcon("changroup", ImageBgWidth - 10, ImageBgHeight - 10);
            if (img) {
                ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                Left += ImageBgWidth + m_MarginItem2;
            }
        } else {
            if (Current)
                img = ImgLoader.LoadIcon("tv_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img)
                img = ImgLoader.LoadIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);
            if (img) {
                ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                Left += ImageBgWidth + m_MarginItem2;
            }
        }
    }

    // Event from channel
    const cEvent *Event {nullptr};
    cString EventTitle {""};
    double progress {0.0};
    LOCK_SCHEDULES_READ;
    const cSchedule *Schedule = Schedules->GetSchedule(Channel);
    if (Schedule) {
        Event = Schedule->GetPresentEvent();
        if (Event) {
            // Calculate progress bar
            const time_t now {time(0)};
            const double duration = Event->Duration();
            progress = (duration > 0) ? std::min(100.0, std::max(0.0, (now - Event->StartTime()) * 100.0 / duration)) : 0.0;

            EventTitle = Event->Title();
        }
    }

    if (WithProvider)
        Buffer = cString::sprintf("%s - %s", Channel->Provider(), Channel->Name());
    else
        Buffer = Channel->Name();

    const int LeftName {Left};
    if (Config.MenuChannelView == 1) {  // flatPlus long
        Width = m_MenuItemWidth - LeftName;
        if (IsGroup) {
            const int LineTop {Top + (m_FontHeight - 3) / 2};
            MenuPixmap->DrawRectangle(cRect(Left, LineTop, m_MenuItemWidth - Left, 3), ColorFg);
            const cString GroupName = cString::sprintf(" %s ", *Buffer);
            MenuPixmap->DrawText(cPoint(Left + (m_MenuItemWidth / 10 * 2), Top), *GroupName, ColorFg, ColorBg, m_Font,
                                 0, 0, taCenter);
        } else {
            if (Current && m_Font->Width(*Buffer) > (Width) && Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(*Buffer, cRect(LeftName, Top + m_MenuTop, Width, m_FontHeight), ColorFg,
                                             clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(LeftName, Top), *Buffer, ColorFg, ColorBg, m_Font, Width);
            }
        }
    } else {  // flatPlus short
        Width = (m_MenuItemWidth + (m_IsScrolling ? m_WidthScrollBar : 0)) / 10 * 2;

        if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4)  // flatPlus short, flatPlus short + EPG
            Width = m_MenuItemWidth - LeftName;

        if (IsGroup) {
            const int LineTop {Top + (m_FontHeight - 3) / 2};
            MenuPixmap->DrawRectangle(cRect(Left, LineTop, m_MenuItemWidth - Left, 3), ColorFg);
            const cString GroupName = cString::sprintf(" %s ", *Buffer);
            MenuPixmap->DrawText(cPoint(Left + (m_MenuItemWidth / 10 * 2), Top), *GroupName, ColorFg, ColorBg, m_Font,
                                 0, 0, taCenter);
        } else {
            if (Current && m_Font->Width(*Buffer) > (Width) && Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(*Buffer, cRect(LeftName, Top + m_MenuTop, Width, m_FontHeight), ColorFg,
                                             clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(LeftName, Top), *Buffer, ColorFg, ColorBg, m_Font, Width);
            }

            Left += Width + m_MarginItem;

            if (DrawProgress) {
                int PBTop {y + (m_ItemChannelHeight - Config.MenuItemPadding) / 2 -
                           Config.decorProgressMenuItemSize / 2 - Config.decorBorderMenuItemSize};
                int PBLeft {Left};
                int PBWidth {m_MenuItemWidth / 10};
                if (Config.MenuChannelView == 3 ||
                    Config.MenuChannelView == 4) {  // flatPlus short, flatPlus short + EPG
                    PBTop = Top + m_FontHeight + m_FontSmlHeight;
                    PBLeft = Left - Width - m_MarginItem;
                    PBWidth = m_MenuItemWidth - LeftName - m_MarginItem2 - Config.decorBorderMenuItemSize -
                              m_WidthScrollBar;

                    if (m_IsScrolling)
                        PBWidth += m_WidthScrollBar;
                }

                Width = (m_MenuItemWidth + (m_IsScrolling ? m_WidthScrollBar : 0)) / 10;
                if (Current)
                    ProgressBarDrawRaw(MenuPixmap, MenuPixmap,
                                       cRect(PBLeft, PBTop, PBWidth, Config.decorProgressMenuItemSize),
                                       cRect(PBLeft, PBTop, PBWidth, Config.decorProgressMenuItemSize), progress, 100,
                                       Config.decorProgressMenuItemCurFg, Config.decorProgressMenuItemCurBarFg,
                                       Config.decorProgressMenuItemCurBg, Config.decorProgressMenuItemType, false);
                else
                    ProgressBarDrawRaw(MenuPixmap, MenuPixmap,
                                       cRect(PBLeft, PBTop, PBWidth, Config.decorProgressMenuItemSize),
                                       cRect(PBLeft, PBTop, PBWidth, Config.decorProgressMenuItemSize), progress, 100,
                                       Config.decorProgressMenuItemFg, Config.decorProgressMenuItemBarFg,
                                       Config.decorProgressMenuItemBg, Config.decorProgressMenuItemType, false);
                Left += Width + m_MarginItem;
            }

            if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4) {  // flatPlus short, flatPlus short + EPG
                Left = LeftName;
                Top += m_FontHeight;

                if (Current && m_FontSml->Width(*EventTitle) > (m_MenuItemWidth - Left - m_MarginItem) &&
                    Config.ScrollerEnable) {
                    MenuItemScroller.AddScroller(
                        *EventTitle,
                        cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontSmlHeight), ColorFg,
                        clrTransparent, m_FontSml);
                } else {
                    MenuPixmap->DrawText(cPoint(Left, Top), *EventTitle, ColorFg, ColorBg, m_FontSml,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            } else {
                if (Current && m_Font->Width(*EventTitle) > (m_MenuItemWidth - Left - m_MarginItem) &&
                    Config.ScrollerEnable) {
                    MenuItemScroller.AddScroller(
                        *EventTitle, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                        ColorFg, clrTransparent, m_Font);
                } else {
                    MenuPixmap->DrawText(cPoint(Left, Top), *EventTitle, ColorFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            }
        }
    }

    sDecorBorder ib {
        .Left = Config.decorBorderMenuItemSize,
        .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y,
        .Width = m_MenuItemWidth,
        .Height = Height,
        .Size = Config.decorBorderMenuItemSize,
        .Type = Config.decorBorderMenuItemType
    };

    if (Current) {
        ib.ColorFg = Config.decorBorderMenuItemCurFg;
        ib.ColorBg = Config.decorBorderMenuItemCurBg;
    } else if (Selectable) {
        ib.ColorFg = Config.decorBorderMenuItemSelFg;
        ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
        ib.ColorFg = Config.decorBorderMenuItemFg;
        ib.ColorBg = Config.decorBorderMenuItemBg;
    }

    ib.From = BorderMenuItem;
    DecorBorderDraw(ib);

    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    if (Config.MenuChannelView == 4 && Current)
        DrawItemExtraEvent(Event, "");

    return true;
}

void cFlatDisplayMenu::DrawItemExtraEvent(const cEvent *Event, const cString EmptyText) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawItemExtraEvent()");
#endif

    m_cLeft = m_MenuItemWidth + Config.decorBorderMenuItemSize * 2 + Config.decorBorderMenuContentSize + m_MarginItem;
    if (m_IsScrolling)
        m_cLeft += m_WidthScrollBar;

    m_cTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
    m_cWidth = m_MenuWidth - m_cLeft - Config.decorBorderMenuContentSize;
    m_cHeight =
        m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                       Config.decorBorderButtonSize * 2 + m_MarginItem3 + Config.decorBorderMenuContentSize * 2);

    bool IsEmpty {false};
    cString Text {""};
    if (Event) {
        // Description
        if (!isempty(Event->Description()))
            Text.Append(Event->Description());

        if (Config.EpgAdditionalInfoShow) {
            Text.Append("\n");
            // Genre
            InsertGenreInfo(Event, Text);  // Add genre info

            // FSK
            if (Event->ParentalRating())
                Text.Append(cString::sprintf("\n%s: %s", tr("FSK"), *Event->GetParentalRatingString()));

            const cComponents *Components = Event->Components();
            if (Components) {
                cString Audio {""}, Subtitle {""};
                InsertComponents(Components, Text, Audio, Subtitle, true);  // Get info for audio/video and subtitle

                if (!isempty(*Audio))
                    Text.Append(cString::sprintf("\n%s: %s", tr("Audio"), *Audio));
                if (!isempty(*Subtitle))
                    Text.Append(cString::sprintf("\n%s: %s", tr("Subtitle"), *Subtitle));
            }  // if Components
        }      // EpgAdditionalInfoShow
    } else {
        Text.Append(*EmptyText);
        IsEmpty = true;
    }

    ComplexContent.Clear();
    ComplexContent.SetScrollSize(m_FontSmlHeight);
    ComplexContent.SetScrollingActive(false);
    ComplexContent.SetOsd(m_Osd);
    ComplexContent.SetPosition(cRect(m_cLeft, m_cTop, m_cWidth, m_cHeight));
    ComplexContent.SetBGColor(Theme.Color(clrMenuEventBg));

    if (IsEmpty) {
        cImage *img {ImgLoader.LoadIcon("timerInactiveBig", 256, 256)};
        if (img) {
            ComplexContent.AddImage(img, cRect(m_MarginItem, m_MarginItem, img->Width(), img->Height()));
            ComplexContent.AddText(*Text, true,
                                   cRect(m_MarginItem, m_MarginItem + img->Height(), m_cWidth - m_MarginItem2,
                                         m_cHeight - m_MarginItem2),
                                   Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
        }
    } else {
        cString MediaPath {""};
        int MediaWidth {0}, MediaHeight {m_cHeight - m_MarginItem2};
        int MediaType {0};

        static cPlugin *pScraper = GetScraperPlugin();
        if (Config.TVScraperEPGInfoShowPoster && pScraper) {
            ScraperGetPosterBannerV2 call;
            call.event = Event;
            if (pScraper->Service("GetPosterBannerV2", &call)) {
                if ((call.type == tSeries) && call.banner.path.size() > 0) {
                    MediaWidth = m_cWidth - m_MarginItem2;
                    MediaPath = call.banner.path.c_str();
                    MediaType = 1;
                } else if (call.type == tMovie && call.poster.path.size() > 0) {
                    MediaWidth = m_cWidth / 2 - m_MarginItem3;
                    MediaPath = call.poster.path.c_str();
                    MediaType = 2;
                }
            }
        }

        if (!isempty(*MediaPath)) {
            cImage *img {ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight)};
            if (img && MediaType == 2) {  // Movie
                ComplexContent.AddImageWithFloatedText(
                    img, CIP_Right, *Text,
                    cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
            } else if (img && MediaType == 1) {  // Series
                ComplexContent.AddImage(img, cRect(m_MarginItem, m_MarginItem, img->Width(), img->Height()));
                ComplexContent.AddText(*Text, true,
                                       cRect(m_MarginItem, m_MarginItem + img->Height(), m_cWidth - m_MarginItem2,
                                             m_cHeight - m_MarginItem2),
                                       Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
            } else {
                ComplexContent.AddText(
                    *Text, true,
                    cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
            }
        } else {
            ComplexContent.AddText(
                *Text, true,
                cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
        }
    }
    ComplexContent.CreatePixmaps(Config.MenuContentFullSize);
    ComplexContent.Draw();

    DecorBorderClearByFrom(BorderContent);
    sDecorBorder ib{m_cLeft,
                    m_cTop,
                    m_cWidth,
                    (Config.MenuContentFullSize) ? ComplexContent.ContentHeight(true)
                                                 : ComplexContent.ContentHeight(false),
                    Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType,
                    Config.decorBorderMenuContentFg,
                    Config.decorBorderMenuContentBg,
                    BorderContent};

    DecorBorderDraw(ib);
}

bool cFlatDisplayMenu::SetItemTimer(const cTimer *Timer, int Index, bool Current, bool Selectable) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetItemTimer()");
    dsyslog("   Index %d", Index);
#endif

    if (!MenuPixmap || !MenuIconsPixmap || !MenuIconsOvlPixmap || !MenuIconsBgPixmap)
        return false;

    if (Config.MenuTimerView == 0 || !Timer)
        return false;

    if (Current)
        MenuItemScroller.Clear();

    int Height {m_FontHeight};
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuTimerView == 2 || Config.MenuTimerView == 3) {  // flatPlus short, flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem;
        m_MenuItemWidth /= 2;
    }

    if (m_IsScrolling)
        m_MenuItemWidth -= m_WidthScrollBar;

    tColor ColorFg = Theme.Color(clrItemFont);
    tColor ColorBg = Theme.Color(clrItemBg);
    tColor ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextFont);
    if (Current) {
        ColorFg = Theme.Color(clrItemCurrentFont);
        ColorBg = Theme.Color(clrItemCurrentBg);
        ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextCurrentFont);
    } else if (Selectable) {
        ColorFg = Theme.Color(clrItemSelableFont);
        ColorBg = Theme.Color(clrItemSelableBg);
    }

    const int y {Index * m_ItemTimerHeight};
    if (y + m_ItemTimerHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemTimerHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);
    int Left {Config.decorBorderMenuItemSize + m_MarginItem};
    const int Top {y};

    int ImageLeft {Left};
    int ImageTop {Top};
    const int ImageHeight {m_FontHeight};

    cString IconName {""};
    if (!(Timer->HasFlags(tfActive))) {  // Inactive timer
        IconName = (Current) ? "timerInactive_cur" : "timerInactive";
        ColorFg = Theme.Color(clrMenuTimerItemDisabledFont);
    } else if (Timer->Recording()) {  // Active timer and recording
        IconName = "timerRecording";
        ColorFg = Theme.Color(clrMenuTimerItemRecordingFont);
    } else if (Timer->FirstDay()) {  // Active timer 'FirstDay'
        IconName = "text_arrowturn";
        // ColorFg = Theme.Color(clrMenuTimerItemRecordingFont);
    } else  // Active timer
        IconName = "timerActive";

    cImage *img {ImgLoader.LoadIcon(*IconName, ImageHeight, ImageHeight)};
    if (img) {
        ImageTop = Top + (m_FontHeight - img->Height()) / 2;
        MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    }

    //? Make overlay configurable? (disable)
    if (Timer->Remote()) {  // Remote timer
        img = ImgLoader.LoadIcon("timerRemote", ImageHeight, ImageHeight);
        if (img) {
            ImageTop = Top + (m_FontHeight - img->Height()) / 2;
            MenuIconsOvlPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }
    }
    Left += ImageHeight + m_MarginItem2;

    /* Disabled because invalid lock sequence
    LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
    cString ws = cString::sprintf("%d", Channels->MaxNumber());
    int w = m_Font->Width(ws); */

    const cChannel *Channel = Timer->Channel();
    cString Buffer = cString::sprintf("%d", Channel->Number());
    const int Width {std::max(m_Font->Width("999"), m_Font->Width(*Buffer))};  // Minimal width for channel number

    MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, Width, m_FontHeight, taRight);
    Left += Width + m_MarginItem;

    ImageLeft = Left;
    int ImageBgWidth = ImageHeight * 1.34f;  // Narrowing conversion
    int ImageBgHeight {ImageHeight};
    img = ImgLoader.LoadIcon("logo_background", ImageBgWidth, ImageBgHeight);
    if (img) {
        ImageBgWidth = img->Width();
        ImageBgHeight = img->Height();
        ImageTop = Top + (m_FontHeight - ImageBgHeight) / 2;
        MenuIconsBgPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    }

    img = ImgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
    if (img) {
        ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
        ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
        MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    } else {
        const bool IsRadioChannel {((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false};
        if (IsRadioChannel) {
            if (Current)
                img = ImgLoader.LoadIcon("radio_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img)
                img = ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
            if (img) {
                ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
            }
        } else {
            if (Current)
                img = ImgLoader.LoadIcon("tv_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img)
                img = ImgLoader.LoadIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);
            if (img) {
                ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
            }
        }
    }
    Left += ImageBgWidth + m_MarginItem2;

    cString day {""}, name {""};
    if (Timer->WeekDays()) {
        day = Timer->PrintDay(0, Timer->WeekDays(), false);
    } else if (Timer->Day() - time(0) < 28 * SECSINDAY) {
        day = itoa(Timer->GetMDay(Timer->Day()));
        name = WeekDayName(Timer->Day());
    } else {
        struct tm tm_r;
        time_t Day {Timer->Day()};
        localtime_r(&Day, &tm_r);
        char Buffer[16];
        strftime(Buffer, sizeof(Buffer), "%Y%m%d", &tm_r);
        day = Buffer;
    }
    const char *File {(Setup.FoldersInTimerMenu) ? nullptr : strrchr(Timer->File(), FOLDERDELIMCHAR)};
    if (File && strcmp(File + 1, TIMERMACRO_TITLE) && strcmp(File + 1, TIMERMACRO_EPISODE))
        ++File;
    else
        File = Timer->File();

    div_t TimerStart {std::div(Timer->Start(), 100)};
    div_t TimerStop {std::div(Timer->Stop(), 100)};
    if (Config.MenuTimerView == 1) {  // flatPlus long
        Buffer = cString::sprintf("%s%s%s.", *name, *name && **name ? " " : "", *day);
        MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                             m_MenuItemWidth - Left - m_MarginItem);
        Left += m_Font->Width("XXX 99.  ");
        Buffer =
            cString::sprintf("%02d:%02d - %02d:%02d", TimerStart.quot, TimerStart.rem, TimerStop.quot, TimerStop.rem);
        MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                             m_MenuItemWidth - Left - m_MarginItem);
        Left += m_Font->Width("99:99 - 99:99  ");

        if (Current && m_Font->Width(File) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
            MenuItemScroller.AddScroller(
                File, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight), ColorFg,
                clrTransparent, m_Font, ColorExtraTextFg);
        } else {
            if (Config.MenuItemParseTilde) {
                const char *TildePos {strchr(File, '~')};

                if (TildePos) {
                    std::string_view sv1 {File, static_cast<size_t>(TildePos - File)};
                    std::string_view sv2 {TildePos + 1};
                    const std::string first {rtrim(sv1)};   // Trim possible space at end
                    const std::string second {ltrim(sv2)};  // Trim possible space at begin

                    MenuPixmap->DrawText(cPoint(Left, Top), first.c_str(), ColorFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
                    const int l {m_Font->Width(first.c_str()) + m_Font->Width('X')};
                    MenuPixmap->DrawText(cPoint(Left + l, Top), second.c_str(), ColorExtraTextFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - l - m_MarginItem);
                } else {  // ~ not found
                    MenuPixmap->DrawText(cPoint(Left, Top), File, ColorFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            } else {  // MenuItemParseTilde disabled
                MenuPixmap->DrawText(cPoint(Left, Top), File, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
        }
    } else if (Config.MenuTimerView == 2 || Config.MenuTimerView == 3) {  // flatPlus long + EPG, flatPlus short
        Buffer = cString::sprintf("%s%s%s.  %02d:%02d - %02d:%02d", *name, *name && **name ? " " : "", *day,
                                  TimerStart.quot, TimerStart.rem, TimerStop.quot, TimerStop.rem);
        MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                             m_MenuItemWidth - Left - m_MarginItem);

        if (Current && m_FontSml->Width(File) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
            MenuItemScroller.AddScroller(File,
                                         cRect(Left, Top + m_FontHeight + m_MenuTop,  // TODO: Mismatch when scrolling
                                               m_MenuItemWidth - Left - m_MarginItem - m_WidthScrollBar,
                                               m_FontSmlHeight),
                                         ColorFg, clrTransparent, m_FontSml, ColorExtraTextFg);
        } else {
            if (Config.MenuItemParseTilde) {
                const char *TildePos {strchr(File, '~')};

                if (TildePos) {
                    std::string_view sv1 {File, static_cast<size_t>(TildePos - File)};
                    std::string_view sv2 {TildePos + 1};

                    const std::string first {rtrim(sv1)};   // Trim possible space at end
                    const std::string second {ltrim(sv2)};  // Trim possible space at begin

                    MenuPixmap->DrawText(cPoint(Left, Top + m_FontHeight), first.c_str(), ColorFg, ColorBg, m_FontSml,
                                         m_MenuItemWidth - Left - m_MarginItem);
                    const int l {m_FontSml->Width(first.c_str()) + m_FontSml->Width('X')};
                    MenuPixmap->DrawText(cPoint(Left + l, Top + m_FontHeight), second.c_str(), ColorExtraTextFg,
                                         ColorBg, m_FontSml, m_MenuItemWidth - Left - l - m_MarginItem);
                } else {  // ~ not found
                    MenuPixmap->DrawText(cPoint(Left, Top + m_FontHeight), File, ColorFg, ColorBg, m_FontSml,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            } else {  // MenuItemParseTilde disabled
                MenuPixmap->DrawText(cPoint(Left, Top + m_FontHeight), File, ColorFg, ColorBg, m_FontSml,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
        }
    }

    sDecorBorder ib {
        .Left = Config.decorBorderMenuItemSize,
        .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y,
        .Width = m_MenuItemWidth,
        .Height = Height,
        .Size = Config.decorBorderMenuItemSize,
        .Type = Config.decorBorderMenuItemType
    };

    if (Current) {
        ib.ColorFg = Config.decorBorderMenuItemCurFg;
        ib.ColorBg = Config.decorBorderMenuItemCurBg;
    } else if (Selectable) {
        ib.ColorFg = Config.decorBorderMenuItemSelFg;
        ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
        ib.ColorFg = Config.decorBorderMenuItemFg;
        ib.ColorBg = Config.decorBorderMenuItemBg;
    }

    ib.From = BorderMenuItem;
    DecorBorderDraw(ib);

    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    if (Config.MenuTimerView == 3 && Current)  {  // flatPlus short
        const cEvent *Event = Timer->Event();
        DrawItemExtraEvent(Event, tr("timer not enabled"));
    }

    return true;
}

bool cFlatDisplayMenu::SetItemEvent(const cEvent *Event, int Index, bool Current, bool Selectable,
                                    const cChannel *Channel, bool WithDate, eTimerMatch TimerMatch, bool TimerActive) {
    if (!MenuPixmap || !MenuIconsBgPixmap || !MenuIconsPixmap)
        return false;

    if (Config.MenuEventView == 0)
        return false;

    if (Config.MenuEventViewAlwaysWithDate)
        WithDate = true;

    if (Current)
        MenuItemScroller.Clear();

    int Height {m_FontHeight};
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuEventView == 2 || Config.MenuEventView == 3) {  // flatPlus short. flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem2 + Config.decorProgressMenuItemSize / 2;
        m_MenuItemWidth *= 0.6;
    }

    if (m_IsScrolling)
        m_MenuItemWidth -= m_WidthScrollBar;

    tColor ColorFg = Theme.Color(clrItemFont);
    tColor ColorBg = Theme.Color(clrItemBg);
    tColor ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextFont);
    if (Current) {
        ColorFg = Theme.Color(clrItemCurrentFont);
        ColorBg = Theme.Color(clrItemCurrentBg);
        ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextCurrentFont);
    } else if (Selectable) {
        ColorFg = Theme.Color(clrItemSelableFont);
        ColorBg = Theme.Color(clrItemSelableBg);
    }

    const int y {Index * m_ItemEventHeight};
    if (y + m_ItemEventHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemEventHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);

    int Left {Config.decorBorderMenuItemSize + m_MarginItem};
    int LeftSecond {Left};
    int Top {y}, w {0};
    int ImageTop {Top};
    cString Buffer {""};
    cImage *img {nullptr};
    if (Channel) {
        if (Current)
            m_ItemEventLastChannelName = Channel->Name();

        /* Disabled because invalid lock sequence
        LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
        cString ws = cString::sprintf("%d", Channels->MaxNumber());
        int w = m_Font->Width(ws); */

        w = m_Font->Width("9999");
        const bool IsGroup {Channel->GroupSep()};
        if (!IsGroup) {
            Buffer = cString::sprintf("%d", Channel->Number());
            w = std::max(w, m_Font->Width(*Buffer));  // Minimal width for channel number in Event (epgSearch)

            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, w, m_FontHeight, taRight);
        }
        Left += w + m_MarginItem;

        int ImageLeft {Left};
        int ImageBgWidth = m_FontHeight * 1.34f;  // Narrowing conversion
        int ImageBgHeight {m_FontHeight};
        if (!IsGroup) {
            img = ImgLoader.LoadIcon("logo_background", ImageBgWidth, ImageBgHeight);
            if (img) {
                ImageBgWidth = img->Width();
                ImageBgHeight = img->Height();
                ImageTop = Top + (m_FontHeight - ImageBgHeight) / 2;
                MenuIconsBgPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
            }
            // Load named logo only for channels
            img = ImgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
        }
        if (img) {
            ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
            ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
            MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        } else {
            const bool IsRadioChannel {((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false};

            if (IsRadioChannel) {
                if (Current)
                    img = ImgLoader.LoadIcon("radio_cur", ImageBgWidth - 10, ImageBgHeight - 10);
                if (!img)
                    img = ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
                if (img) {
                    ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                    ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                    MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                }
            } else if (IsGroup) {
                img = ImgLoader.LoadIcon("changroup", ImageBgWidth - 10, ImageBgHeight - 10);
                if (img) {
                    ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                    ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                    MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                }
            } else {
                if (Current)
                    img = ImgLoader.LoadIcon("tv_cur", ImageBgWidth - 10, ImageBgHeight - 10);
                if (!img)
                    img = ImgLoader.LoadIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);
                if (img) {
                    ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                    ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                    MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                }
            }
        }
        Left += ImageBgWidth + m_MarginItem2;
        LeftSecond = Left;

        w = m_IsScrolling ? m_MenuItemWidth / 10 * 2 : (m_MenuItemWidth - m_WidthScrollBar) / 10 * 2;

        cString ChannelName {""};
        if (Config.MenuEventView == 2 || Config.MenuEventView == 3) {  // flatPlus short, flatPlus short + EPG
            ChannelName = Channel->Name();
            w = m_Font->Width(*ChannelName);
        } else {
            ChannelName = Channel->ShortName(true);
        }

        if (IsGroup) {
            const int LineTop {Top + (m_FontHeight - 3) / 2};
            MenuPixmap->DrawRectangle(cRect(Left, LineTop, m_MenuItemWidth - Left, 3), ColorFg);
            Left += w / 2;
            const cString GroupName = cString::sprintf(" %s ", *ChannelName);
            MenuPixmap->DrawText(cPoint(Left, Top), *GroupName, ColorFg, ColorBg, m_Font, 0, 0, taCenter);
        } else {
            MenuPixmap->DrawText(cPoint(Left, Top), *ChannelName, ColorFg, ColorBg, m_Font, w);
        }

        Left += w + m_MarginItem2;

        if (Event) {  // Draw progress of event
            int PBWidth {(m_IsScrolling) ? m_MenuItemWidth / 20 : (m_MenuItemWidth - m_WidthScrollBar) / 20};

            const time_t now {time(0)};
            if ((now >= (Event->StartTime() - 2 * 60))) {
                const int total = Event->EndTime() - Event->StartTime();  // Narrowing conversion
                if (total >= 0) {
                    // Calculate progress bar
                    const double duration = Event->Duration();
                    const double progress =
                        (duration > 0) ? std::min(100.0, std::max(0.0, (now - Event->StartTime()) * 100.0 / duration))
                                       : 0.0;

                    int PBLeft {Left};
                    int PBTop {y + (m_ItemEventHeight - Config.MenuItemPadding) / 2 -
                               Config.decorProgressMenuItemSize / 2 - Config.decorBorderMenuItemSize};
                    int PBHeight {Config.decorProgressMenuItemSize};

                    if ((Config.MenuEventView == 2 || Config.MenuEventView == 3)) {
                        // flatPlus short, flatPlus short + EPG
                        PBTop = y + m_FontHeight + m_FontSmlHeight + m_MarginItem;
                        PBWidth = m_MenuItemWidth - LeftSecond - m_WidthScrollBar - m_MarginItem2;
                        if (m_IsScrolling)
                            PBWidth += m_WidthScrollBar;

                        PBLeft = LeftSecond;
                        PBHeight = Config.decorProgressMenuItemSize / 2;
                    }

                    if (!IsGroup) {  // Exclude epg2vdr group separator
                        if (Current)
                            ProgressBarDrawRaw(MenuPixmap, MenuPixmap, cRect(PBLeft, PBTop, PBWidth, PBHeight),
                                               cRect(PBLeft, PBTop, PBWidth, PBHeight), progress, 100,
                                               Config.decorProgressMenuItemCurFg, Config.decorProgressMenuItemCurBarFg,
                                               Config.decorProgressMenuItemCurBg, Config.decorProgressMenuItemType,
                                               false);
                        else
                            ProgressBarDrawRaw(MenuPixmap, MenuPixmap, cRect(PBLeft, PBTop, PBWidth, PBHeight),
                                               cRect(PBLeft, PBTop, PBWidth, PBHeight), progress, 100,
                                               Config.decorProgressMenuItemFg, Config.decorProgressMenuItemBarFg,
                                               Config.decorProgressMenuItemBg, Config.decorProgressMenuItemType, false);
                    }
                }
            }
            Left += PBWidth + m_MarginItem2;
        }  // if Event
    } else {
        TopBarSetMenuLogo(m_ItemEventLastChannelName);
    }  // if Channel

    if (WithDate && Event && Selectable) {
        struct tm tm_r;
        time_t Day {Event->StartTime()};
        localtime_r(&Day, &tm_r);
        char buf[8];
        strftime(buf, sizeof(buf), "%2d", &tm_r);

        const cString DateString = cString::sprintf("%s %s. ", *WeekDayName(Event->StartTime()), buf);
        if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Channel) {
            // flatPlus short, flatPlus short + EPG
            w = m_FontSml->Width("XXX 99. ") + m_MarginItem;
            MenuPixmap->DrawText(cPoint(LeftSecond, Top + m_FontHeight), *DateString, ColorFg, ColorBg, m_FontSml, w);
            LeftSecond += w + m_MarginItem;
        } else {
            w = m_Font->Width("XXX 99. ") + m_MarginItem;
            MenuPixmap->DrawText(cPoint(Left, Top), *DateString, ColorFg, ColorBg, m_Font, w, m_FontHeight, taRight);
        }

        Left += w + m_MarginItem;
    }

    int ImageHeight {m_FontHeight};
    if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Channel && Event && Selectable) {
        // flatPlus short, flatPlus short + EPG
        Left = LeftSecond;
        Top += m_FontHeight;
        ImageHeight = m_FontSmlHeight;
        MenuPixmap->DrawText(cPoint(Left, Top), *Event->GetTimeString(), ColorFg, ColorBg, m_FontSml);
        Left += m_FontSml->Width(*Event->GetTimeString()) + m_MarginItem;
    /* } else if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Event && Selectable) {
            MenuPixmap->DrawText(cPoint(Left, Top), *Event->GetTimeString(), ColorFg, ColorBg, m_Font);
            Left += m_Font->Width(*Event->GetTimeString()) + m_MarginItem; */
    } else if (Event && Selectable) {  //? Same as above
        MenuPixmap->DrawText(cPoint(Left, Top), *Event->GetTimeString(), ColorFg, ColorBg, m_Font);
        Left += m_Font->Width(*Event->GetTimeString()) + m_MarginItem;
    }

    if (TimerActive) {  // Show timer icon only if timer is active
        if (TimerMatch == tmFull) {
            img = nullptr;
            if (Current)
                img = ImgLoader.LoadIcon("timer_full_cur", ImageHeight, ImageHeight);
            if (!img)
                img = ImgLoader.LoadIcon("timer_full", ImageHeight, ImageHeight);
            if (img) {
                ImageTop = Top;
                MenuIconsPixmap->DrawImage(cPoint(Left, ImageTop), *img);
            }
        } else if (TimerMatch == tmPartial) {
            img = nullptr;
            if (Current)
                img = ImgLoader.LoadIcon("timer_partial_cur", ImageHeight, ImageHeight);
            if (!img)
                img = ImgLoader.LoadIcon("timer_partial", ImageHeight, ImageHeight);
            if (img) {
                ImageTop = Top;
                MenuIconsPixmap->DrawImage(cPoint(Left, ImageTop), *img);
            }
        }
    }  // TimerActive

    Left += ImageHeight + m_MarginItem;
    if (Event && Selectable) {
        if (Event->Vps() && (Event->Vps() - Event->StartTime())) {
            img = nullptr;
            if (Current)
                img = ImgLoader.LoadIcon("vps_cur", ImageHeight, ImageHeight);
            if (!img)
                img = ImgLoader.LoadIcon("vps", ImageHeight, ImageHeight);
            if (img) {
                ImageTop = Top;
                MenuIconsPixmap->DrawImage(cPoint(Left, ImageTop), *img);
            }
        }
        Left += ImageHeight + m_MarginItem;

        const cString Title = Event->Title();
        const cString ShortText = (Event->ShortText()) ? Event->ShortText() : "";
        if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Channel) {
            // flatPlus short, flatPlus short + EPG
            if (Current) {
                if (!isempty(*ShortText)) {
                    Buffer = cString::sprintf("%s~%s", *Title, *ShortText);
                    if (m_FontSml->Width(*Buffer) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                        MenuItemScroller.AddScroller(
                            *Buffer,
                            cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontSmlHeight),
                            ColorFg, clrTransparent, m_FontSml, ColorExtraTextFg);
                    } else {
                        MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_FontSml,
                                             m_MenuItemWidth - Left - m_MarginItem);
                        Left += m_FontSml->Width(*Title) + m_FontSml->Width('~');
                        MenuPixmap->DrawText(cPoint(Left, Top), *ShortText, ColorExtraTextFg, ColorBg, m_FontSml,
                                             m_MenuItemWidth - Left - m_MarginItem);
                    }
                } else {
                    if (m_FontSml->Width(*Title) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                        MenuItemScroller.AddScroller(
                            *Title,
                            cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontSmlHeight),
                            ColorFg, clrTransparent, m_FontSml, ColorExtraTextFg);
                    } else {
                        MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_FontSml,
                                             m_MenuItemWidth - Left - m_MarginItem);
                    }
                }
            } else {  // Not current
                MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_FontSml,
                                     m_MenuItemWidth - Left - m_MarginItem);
                if (!isempty(*ShortText)) {
                    Left += m_FontSml->Width(*Title) + m_Font->Width('~');
                    MenuPixmap->DrawText(cPoint(Left, Top), *ShortText, ColorExtraTextFg, ColorBg, m_FontSml,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            }
        } else if ((Config.MenuEventView == 2 || Config.MenuEventView == 3)) {
            if (Current && m_Font->Width(*Title) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    *Title, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight), ColorFg,
                    clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
            if (!isempty(*ShortText)) {
                Top += m_FontHeight;
                if (Current && m_FontSml->Width(*ShortText) > (m_MenuItemWidth - Left - m_MarginItem) &&
                    Config.ScrollerEnable) {
                    MenuItemScroller.AddScroller(
                        *ShortText, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                        ColorExtraTextFg, clrTransparent, m_FontSml);
                } else {
                    MenuPixmap->DrawText(cPoint(Left, Top), *ShortText, ColorExtraTextFg, ColorBg, m_FontSml,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            }
        } else {
            if (Current) {
                if (!isempty(*ShortText)) {
                    Buffer = cString::sprintf("%s~%s", *Title, *ShortText);
                    if (m_Font->Width(*Buffer) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                        MenuItemScroller.AddScroller(
                            *Buffer, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                            ColorFg, clrTransparent, m_Font, ColorExtraTextFg);
                    } else {
                        MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_Font,
                                             m_MenuItemWidth - Left - m_MarginItem);
                        Left += m_Font->Width(*Title) + m_Font->Width('~');
                        MenuPixmap->DrawText(cPoint(Left, Top), *ShortText, ColorExtraTextFg, ColorBg, m_Font,
                                             m_MenuItemWidth - Left - m_MarginItem);
                    }
                } else {
                    if (m_Font->Width(*Title) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                        MenuItemScroller.AddScroller(
                            *Title, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                            ColorFg, clrTransparent, m_Font, ColorExtraTextFg);
                    } else {
                        MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_Font,
                                             m_MenuItemWidth - Left - m_MarginItem);
                    }
                }
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
                if (!isempty(*ShortText)) {
                    Left += m_Font->Width(*Title) + m_Font->Width('~');
                    MenuPixmap->DrawText(cPoint(Left, Top), *ShortText, ColorExtraTextFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            }
        }
    } else if (Event) {
        dsyslog("flatPlus: SetItemEvent() Try to extract date from event title\n   Title is '%s'", Event->Title());
        if (Channel && Channel->GroupSep()) {  // Exclude epg2vdr group separator '-----#011 Nachrichten -----'
            dsyslog("flatPlus: SetItemEvent() Channel is group separator!");
        } else {
            // Extract date from separator
            // epgsearch program '----------------------------------------         Fr. 21.02.2025 ----------------------------------------'  //NOLINT
            std::string_view sep {Event->Title()};  // Event->Title should always set to something
            if (sep.length() > 12) {
                const std::size_t found {sep.find(" -")};
                if (found >= 10) {
                    dsyslog("flatPlus: SetItemEvent() Date string found at %ld", found);
                    const std::string date {sep.substr(found - 10, 10)};
                    const int LineTop {Top + (m_FontHeight - 3) / 2};
                    MenuPixmap->DrawRectangle(cRect(0, LineTop, m_MenuItemWidth, 3), ColorFg);
                    const cString DateSpace = cString::sprintf(" %s ", date.c_str());
                    MenuPixmap->DrawText(cPoint(LeftSecond + m_MenuWidth / 10 * 2, Top), *DateSpace, ColorFg, ColorBg,
                                         m_Font, 0, 0, taCenter);
                } else {
                    dsyslog("flatPlus: SetItemEvent() Date string not found!");
                    MenuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            } else {
                dsyslog("flatPlus: SetItemEvent() Event title too short!");
                MenuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
        }
    }

    sDecorBorder ib {
        .Left = Config.decorBorderMenuItemSize,
        .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y,
        .Width = m_MenuItemWidth,
        .Height = Height,
        .Size = Config.decorBorderMenuItemSize,
        .Type = Config.decorBorderMenuItemType
    };

    if (Current) {
        ib.ColorFg = Config.decorBorderMenuItemCurFg;
        ib.ColorBg = Config.decorBorderMenuItemCurBg;
    } else if (Selectable) {
        ib.ColorFg = Config.decorBorderMenuItemSelFg;
        ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
        ib.ColorFg = Config.decorBorderMenuItemFg;
        ib.ColorBg = Config.decorBorderMenuItemBg;
    }

    ib.From = BorderMenuItem;
    DecorBorderDraw(ib);

    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    if (Config.MenuEventView == 3 && Current)
        DrawItemExtraEvent(Event, "");

    return true;
}

bool cFlatDisplayMenu::SetItemRecording(const cRecording *Recording, int Index, bool Current, bool Selectable,
                                        int Level, int Total, int New) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetItemRecording()");
    dsyslog("   Index %d, Level %d, Total %d, New %d", Index, Level, Total, New);
#endif

    if (!MenuPixmap || !MenuIconsPixmap || !MenuIconsOvlPixmap)
        return false;

    if (Config.MenuRecordingView == 0)
        return false;

    m_RecFolder = (Level > 0) ? *GetRecordingName(Recording, Level - 1, true) : "";
    m_LastItemRecordingLevel = Level;

    if (Config.MenuRecordingShowCount) {
        const cString NewTitle = cString::sprintf("%s %s", *m_LastTitle, *GetRecCounts());
        TopBarSetTitle(*NewTitle, false);  // Do not clear
    }

    if (Current)
        MenuItemScroller.Clear();

    int Height {m_FontHeight};
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuRecordingView == 2 || Config.MenuRecordingView == 3) {  // flatPlus short, flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem;
        m_MenuItemWidth /= 2;
    }

    if (m_IsScrolling)
        m_MenuItemWidth -= m_WidthScrollBar;

    tColor ColorFg = Theme.Color(clrItemFont);
    tColor ColorBg = Theme.Color(clrItemBg);
    tColor ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextFont);
    if (Current) {
        ColorFg = Theme.Color(clrItemCurrentFont);
        ColorBg = Theme.Color(clrItemCurrentBg);
        ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextCurrentFont);
    } else if (Selectable) {
        ColorFg = Theme.Color(clrItemSelableFont);
        ColorBg = Theme.Color(clrItemSelableBg);
    }

    const int y {Index * m_ItemRecordingHeight};
    if (y + m_ItemRecordingHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemRecordingHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);
    //* Preload for calculation of position
    cImage *ImgRecCut {nullptr}, *ImgRecNew {nullptr}, *ImgRecNewSml {nullptr};
    if (Current) {
        ImgRecNew = ImgLoader.LoadIcon("recording_new_cur", m_FontHeight, m_FontHeight);
        ImgRecNewSml = ImgLoader.LoadIcon("recording_new_cur", m_FontSmlHeight, m_FontSmlHeight);
        ImgRecCut = ImgLoader.LoadIcon("recording_cutted_cur", m_FontHeight, m_FontHeight * (2.0 / 3.0));
    }
    if (!ImgRecNew)
        ImgRecNew = ImgLoader.LoadIcon("recording_new", m_FontHeight, m_FontHeight);
    if (!ImgRecNewSml)
        ImgRecNewSml = ImgLoader.LoadIcon("recording_new", m_FontSmlHeight, m_FontSmlHeight);
    if (!ImgRecCut)
        ImgRecCut = ImgLoader.LoadIcon("recording_cutted", m_FontHeight, m_FontHeight * (2.0 / 3.0));

    const int ImgRecNewWidth {(ImgRecNew) ? ImgRecNew->Width() : 0};
    const int ImgRecNewSmlWidth {(ImgRecNewSml) ? ImgRecNewSml->Width() : 0};
    const int ImgRecCutWidth {(ImgRecCut) ? ImgRecCut->Width() : 0};
    const int ImgRecCutHeight {(ImgRecCut) ? ImgRecCut->Height() : 0};

    int Left {Config.decorBorderMenuItemSize + m_MarginItem};
    int Top {y};
    cString Buffer {""}, IconName {""};
    const cString RecName = *GetRecordingName(Recording, Level, Total == 0);
    cImage *img {nullptr};
    if (Config.MenuRecordingView == 1) {  // flatPlus long
        if (Total == 0) {  // Recording
            if (Current)
                img = ImgLoader.LoadIcon("recording_cur", m_FontHeight, m_FontHeight);
            if (!img)
                img = ImgLoader.LoadIcon("recording", m_FontHeight, m_FontHeight);
            if (img) {
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                Left += m_FontHeight + m_MarginItem;
            }

            const div_t TimeHM {std::div((Recording->LengthInSeconds() + 30) / 60, 60)};
            const cString Length = cString::sprintf("%02d:%02d", TimeHM.quot, TimeHM.rem);
            Buffer = cString::sprintf("%s  %s   %s ", *ShortDateString(Recording->Start()),
                                      *TimeString(Recording->Start()), *Length);

            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                                 m_MenuItemWidth - Left - m_MarginItem);

            Left += m_Font->Width(*Buffer);

            // Show if recording is still in progress (ruTimer), or played (ruReplay)
            const int RecordingIsInUse {Recording->IsInUse()};
            if ((RecordingIsInUse & ruTimer) != 0) {  // The recording is currently written to by a timer
                img = nullptr;
                if (Current)
                    img = ImgLoader.LoadIcon("timerRecording_cur", m_FontHeight, m_FontHeight);
                if (!img)
                    img = ImgLoader.LoadIcon("timerRecording", m_FontHeight, m_FontHeight);
                if (img)
                    MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
            } else if ((RecordingIsInUse & ruReplay) != 0) {  // The recording is being replayed
                img = nullptr;
                if (Current)
                    img = ImgLoader.LoadIcon("play", m_FontHeight, m_FontHeight);
                if (!img)
                    img = ImgLoader.LoadIcon("play_sel", m_FontHeight, m_FontHeight);
                    // img = ImgLoader.LoadIcon("recording_replay", m_FontHeight, m_FontHeight);
                if (img)
                    MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
            } else if (Recording->IsNew()) {
                if (ImgRecNew)
                    MenuIconsPixmap->DrawImage(cPoint(Left, Top), *ImgRecNew);
            } else /* if (!RecordingIsInUse) */ {
                IconName = *GetRecordingSeenIcon(Recording->NumFrames(), Recording->GetResume());

                img = nullptr;
                if (Current) {
                    const cString IconNameCur = cString::sprintf("%s_cur", *IconName);
                    img = ImgLoader.LoadIcon(*IconNameCur, m_FontHeight, m_FontHeight);
                }
                if (!img)
                    img = ImgLoader.LoadIcon(*IconName, m_FontHeight, m_FontHeight);
                if (img)
                    MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
            }
#if APIVERSNUM >= 20505
            if (Config.MenuItemRecordingShowRecordingErrors) {
                IconName = *GetRecordingErrorIcon(Recording->Info()->Errors());

                img = nullptr;
                if (Current) {
                    const cString IconNameCur = cString::sprintf("%s_cur", *IconName);
                    img = ImgLoader.LoadIcon(*IconNameCur, m_FontHeight, m_FontHeight);
                }
                if (!img)
                    img = ImgLoader.LoadIcon(*IconName, m_FontHeight, m_FontHeight);
                if (img)
                    MenuIconsOvlPixmap->DrawImage(cPoint(Left, Top), *img);
            }  // MenuItemRecordingShowRecordingErrors
#endif

            Left += ImgRecNewWidth + m_MarginItem;
            if (Recording->IsEdited() && ImgRecCut) {
                const int ImageTop {Top + m_FontHeight - ImgRecCutHeight};  // 2/3 image height
                MenuIconsPixmap->DrawImage(cPoint(Left, ImageTop), *ImgRecCut);
            }
            if (Config.MenuItemRecordingShowFormatIcons) {
                IconName = *GetRecordingFormatIcon(Recording);  // Show (SD), HD or UHD Logo
                if (!isempty(*IconName)) {
                    const int ImageHeight = m_FontHeight * (1.0 / 3.0);  // 1/3 image height. Narrowing conversion
                    img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, ImageHeight);
                        if (img) {
                            const int ImageTop {Top + m_FontHeight - m_FontAscender};
                            const int ImageLeft {Left + m_FontHeight - img->Width()};
                            MenuIconsOvlPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                        }
                }
            }

            Left += (ImgRecCutWidth * 1.5f) + m_MarginItem;  // 0.666 * 1.5 = 0.999

            if (Current && m_Font->Width(*RecName) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    *RecName, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                    ColorFg, clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), *RecName, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
        } else if (Total > 0) {  // Folder with recordings
            img = nullptr;
            if (Current)
                img = ImgLoader.LoadIcon("folder_cur", m_FontHeight, m_FontHeight);
            if (!img)
                img = ImgLoader.LoadIcon("folder", m_FontHeight, m_FontHeight);
            if (img) {
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                Left += img->Width() + m_MarginItem;
            }

            const int DigitsMaxWidth {m_Font->Width("9999") + m_MarginItem};  // Use same width for recs and new recs
            Buffer = cString::sprintf("%d", Total);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, DigitsMaxWidth, m_FontHeight,
                                 taLeft);
            Left += DigitsMaxWidth;  // m_Font->Width("9999  ");

            if (ImgRecNew)
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *ImgRecNew);

            Left += ImgRecNewWidth + m_MarginItem;
            Buffer = cString::sprintf("%d", New);
            // MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
            //                     m_MenuItemWidth - Left - m_MarginItem);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                                 DigitsMaxWidth, m_FontHeight);

            Left += DigitsMaxWidth;  // m_Font->Width(" 9999 ");
            int LeftWidth {Config.decorBorderMenuItemSize + m_FontHeight + (m_Font->Width("9999") * 2) +
                           ImgRecNewWidth + m_MarginItem * 5};  // For folder with recordings

            if (Config.MenuItemRecordingShowFolderDate > 0) {
                LeftWidth += m_Font->Width("(99.99.99)") + m_FontHeight + m_MarginItem2;
                Buffer = cString::sprintf("(%s)", *ShortDateString(GetLastRecTimeFromFolder(Recording, Level)));
                // MenuPixmap->DrawText(
                //    cPoint(LeftWidth - m_Font->Width(*Buffer) - m_FontHeight2 - m_MarginItem2, Top), *Buffer,
                //    ColorExtraTextFg, ColorBg, m_Font);
                MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorExtraTextFg, ColorBg, m_Font);
                Left += m_Font->Width(*Buffer) + m_MarginItem;
                if (IsRecordingOld(Recording, Level)) {
                    // Left = LeftWidth - m_FontHeight2 - m_MarginItem2;
                    // img = nullptr;
                    if (Current)
                        img = ImgLoader.LoadIcon("recording_old_cur", m_FontHeight, m_FontHeight);
                    else
                        img = ImgLoader.LoadIcon("recording_old", m_FontHeight, m_FontHeight);
                    if (img) {
                        MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                    }
                }
                Left += m_FontHeight + m_MarginItem;  // Increase 'Left' even if no image is drawn
            }

            if (Current && m_Font->Width(*RecName) > (m_MenuItemWidth - LeftWidth - m_MarginItem) &&
                Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    *RecName,
                    cRect(LeftWidth, Top + m_MenuTop, m_MenuItemWidth - LeftWidth - m_MarginItem, m_FontHeight),
                    ColorFg, clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(LeftWidth, Top), *RecName, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - LeftWidth - m_MarginItem);
            }
            // LeftWidth += m_Font->Width(*RecName) + m_MarginItem2;  //* Unused from here
        } else if (Total == -1) {  // Folder without recordings
            img = nullptr;
            if (Current)
                img = ImgLoader.LoadIcon("folder_cur", m_FontHeight, m_FontHeight);
            if (!img)
                img = ImgLoader.LoadIcon("folder", m_FontHeight, m_FontHeight);
            if (img) {
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                Left += img->Width() + m_MarginItem;
            }

            if (Current && m_Font->Width(Recording->FileName()) > (m_MenuItemWidth - Left - m_MarginItem) &&
                Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    Recording->FileName(),
                    cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight), ColorFg,
                    clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), Recording->FileName(), ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
        }
    } else {               // flatPlus short
        if (Total == 0) {  // Recording
            img = nullptr;
            if (Current)
                img = ImgLoader.LoadIcon("recording_cur", m_FontHeight, m_FontHeight);
            if (!img)
                img = ImgLoader.LoadIcon("recording", m_FontHeight, m_FontHeight);
            if (img) {
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                Left += m_FontHeight + m_MarginItem;
            }

            int ImagesWidth {ImgRecNewWidth + ImgRecCutWidth + m_MarginItem2 + m_WidthScrollBar};
            if (m_IsScrolling)
                ImagesWidth -= m_WidthScrollBar;

            if (Current && m_Font->Width(*RecName) > (m_MenuItemWidth - Left - m_MarginItem - ImagesWidth) &&
                Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    *RecName,
                    cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem - ImagesWidth, m_FontHeight),
                    ColorFg, clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), *RecName, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem - ImagesWidth);
            }

            const div_t TimeHM {std::div((Recording->LengthInSeconds() + 30) / 60, 60)};
            const cString Length = cString::sprintf("%02d:%02d", TimeHM.quot, TimeHM.rem);
            Buffer = cString::sprintf("%s  %s   %s ", *ShortDateString(Recording->Start()),
                                      *TimeString(Recording->Start()), *Length);

            Top += m_FontHeight;
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml,
                                 m_MenuItemWidth - Left - m_MarginItem);

            Left = m_MenuItemWidth - ImagesWidth;
            Top -= m_FontHeight;
            // Show if recording is still in progress (ruTimer), or played (ruReplay)
            const int RecordingIsInUse {Recording->IsInUse()};
            if ((RecordingIsInUse & ruTimer) != 0) {  // The recording is currently written to by a timer
                img = nullptr;
                if (Current)
                    img = ImgLoader.LoadIcon("timerRecording_cur", m_FontHeight, m_FontHeight);
                if (!img)
                    img = ImgLoader.LoadIcon("timerRecording", m_FontHeight, m_FontHeight);
                if (img)
                    MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
            } else if ((RecordingIsInUse & ruReplay) != 0) {  // The recording is being replayed
                img = nullptr;
                if (Current)
                    img = ImgLoader.LoadIcon("play", m_FontHeight, m_FontHeight);
                if (!img)
                    img = ImgLoader.LoadIcon("play_sel", m_FontHeight, m_FontHeight);
                if (img)
                    MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
            } else if (Recording->IsNew()) {
                if (ImgRecNew)
                    MenuIconsPixmap->DrawImage(cPoint(Left, Top), *ImgRecNew);
            } else {
                IconName = *GetRecordingSeenIcon(Recording->NumFrames(), Recording->GetResume());

                img = nullptr;
                if (Current) {
                    cString IconNameCur = cString::sprintf("%s_cur", *IconName);
                    img = ImgLoader.LoadIcon(*IconNameCur, m_FontHeight, m_FontHeight);
                }
                if (!img)
                    img = ImgLoader.LoadIcon(*IconName, m_FontHeight, m_FontHeight);
                if (img)
                    MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
            }
#if APIVERSNUM >= 20505
            if (Config.MenuItemRecordingShowRecordingErrors) {
                IconName = *GetRecordingErrorIcon(Recording->Info()->Errors());

                img = nullptr;
                if (Current) {
                    const cString IconNameCur = cString::sprintf("%s_cur", *IconName);
                    img = ImgLoader.LoadIcon(*IconNameCur, m_FontHeight, m_FontHeight);
                }
                if (!img)
                    img = ImgLoader.LoadIcon(*IconName, m_FontHeight, m_FontHeight);
                if (img)
                    MenuIconsOvlPixmap->DrawImage(cPoint(Left, Top), *img);
            }  // MenuItemRecordingShowRecordingErrors
#endif

            Left += ImgRecNew->Width() + m_MarginItem;
            if (Recording->IsEdited() && ImgRecCut) {
                const int ImageTop {Top + m_FontHeight - ImgRecCut->Height()};  // 2/3 image height
                MenuIconsPixmap->DrawImage(cPoint(Left, ImageTop), *ImgRecCut);
            }
            if (Config.MenuItemRecordingShowFormatIcons) {
                IconName = *GetRecordingFormatIcon(Recording);  // Show (SD), HD or UHD Logo
                if (!isempty(*IconName)) {
                    const int ImageHeight = m_FontHeight * (1.0 / 3.0);  // 1/3 image height. Narrowing conversion
                    img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, ImageHeight);
                    if (img) {
                        const int ImageTop {Top + m_FontHeight - m_FontAscender};
                        const int ImageLeft {Left + m_FontHeight - img->Width()};
                        MenuIconsOvlPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                    }
                }
            }

            Left += (ImgRecCutWidth * 1.5f) + m_MarginItem;  // 0.666 * 1.5 = 0.999

        } else if (Total > 0) {  // Folder with recordings
            img = nullptr;
            if (Current)
                img = ImgLoader.LoadIcon("folder_cur", m_FontHeight, m_FontHeight);
            if (!img)
                img = ImgLoader.LoadIcon("folder", m_FontHeight, m_FontHeight);
            if (img) {
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                Left += img->Width() + m_MarginItem;
            }

            if (Current && m_Font->Width(*RecName) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    *RecName, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                    ColorFg, clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), *RecName, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }

            Top += m_FontHeight;
            const int DigitsMaxWidth {m_FontSml->Width("9999") + m_MarginItem};  // Use same width for recs and new recs
            Buffer = cString::sprintf("%d", Total);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml, DigitsMaxWidth,
                                 m_FontSmlHeight, taRight);
            Left += DigitsMaxWidth;

            if (ImgRecNewSml)
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *ImgRecNewSml);

            Left += ImgRecNewSmlWidth + m_MarginItem;
            Buffer = cString::sprintf("%d", New);
            // MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml,
            //                     m_MenuItemWidth - Left - m_MarginItem);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml,
                                 DigitsMaxWidth, m_FontSmlHeight);
            Left += DigitsMaxWidth;

            if (Config.MenuItemRecordingShowFolderDate > 0) {
                Buffer = cString::sprintf("(%s)", *ShortDateString(GetLastRecTimeFromFolder(Recording, Level)));
                MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorExtraTextFg, ColorBg, m_FontSml);
                if (IsRecordingOld(Recording, Level)) {
                    Left += m_FontSml->Width(*Buffer);
                    // img = nullptr;
                    if (Current)
                        img = ImgLoader.LoadIcon("recording_old_cur", m_FontSmlHeight, m_FontSmlHeight);
                    else
                        img = ImgLoader.LoadIcon("recording_old", m_FontSmlHeight, m_FontSmlHeight);
                    if (img)
                        MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                }
            }
        } else if (Total == -1) {  // Folder without recordings
            img = nullptr;
            if (Current)
                img = ImgLoader.LoadIcon("folder_cur", m_FontHeight, m_FontHeight);
            if (!img)
                img = ImgLoader.LoadIcon("folder", m_FontHeight, m_FontHeight);
            if (img) {
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                Left += img->Width() + m_MarginItem;
            }
            if (Current && m_Font->Width(Recording->FileName()) > (m_MenuItemWidth - Left - m_MarginItem) &&
                Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    Recording->FileName(),
                    cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight), ColorFg,
                    clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), Recording->FileName(), ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
        }
    }

    sDecorBorder ib {
        .Left = Config.decorBorderMenuItemSize,
        .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y,
        .Width = m_MenuItemWidth,
        .Height = Height,
        .Size = Config.decorBorderMenuItemSize,
        .Type = Config.decorBorderMenuItemType
    };

    if (Current) {
        ib.ColorFg = Config.decorBorderMenuItemCurFg;
        ib.ColorBg = Config.decorBorderMenuItemCurBg;
    } else if (Selectable) {
        ib.ColorFg = Config.decorBorderMenuItemSelFg;
        ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
        ib.ColorFg = Config.decorBorderMenuItemFg;
        ib.ColorBg = Config.decorBorderMenuItemBg;
    }

    ib.From = BorderMenuItem;
    DecorBorderDraw(ib);

    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    if (Config.MenuRecordingView == 3 && Current)
        DrawItemExtraRecording(Recording, tr("no recording info"));

    return true;
}

void cFlatDisplayMenu::SetEvent(const cEvent *Event) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetEvent()");
#endif

    if (!ContentHeadIconsPixmap || !ContentHeadPixmap || !Event ) return;

#ifdef DEBUGEPGTIME
    cTimeMs Timer;  // Set Timer
#endif

    m_ShowEvent = true;
    m_ShowRecording = false;
    m_ShowText = false;
    ItemBorderClear();

    m_cLeft = Config.decorBorderMenuContentSize;
    m_cTop = m_chTop + m_MarginItem3 + m_FontHeight + m_FontSmlHeight * 2 + Config.decorBorderMenuContentSize +
             Config.decorBorderMenuContentHeadSize;
    m_cWidth = m_MenuWidth - Config.decorBorderMenuContentSize * 2;
    m_cHeight = m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                               Config.decorBorderButtonSize * 2 + m_MarginItem3 + m_chHeight +
                               Config.decorBorderMenuContentHeadSize * 2 + Config.decorBorderMenuContentSize * 2);

    if (!ButtonsDrawn())
        m_cHeight += m_ButtonsHeight + Config.decorBorderButtonSize * 2;

    m_MenuItemWidth = m_cWidth;

    PixmapFill(ContentHeadIconsPixmap, clrTransparent);

    cString Fsk {""}, Text {""}, TextAdditional {""};
    std::vector<std::string> GenreIcons;
    GenreIcons.reserve(8);

    // Description
    if (!isempty(Event->Description()))
        Text.Append(Event->Description());

    if (Config.EpgAdditionalInfoShow) {
        Text.Append("\n");
        // Genre
        InsertGenreInfo(Event, Text, GenreIcons);  // Add genre info

        // FSK
        if (Event->ParentalRating()) {
            Fsk = *Event->GetParentalRatingString();
            Text.Append(cString::sprintf("\n%s: %s", tr("FSK"), *Fsk));
        }

        const cComponents *Components = Event->Components();
        if (Components) {
            cString Audio {""}, Subtitle {""};
            InsertComponents(Components, TextAdditional, Audio, Subtitle);  // Get info for audio/video and subtitle

            if (!isempty(*Audio)) {
                if (!isempty(*TextAdditional))
                    TextAdditional.Append("\n");
                TextAdditional.Append(cString::sprintf("%s: %s", tr("Audio"), *Audio));
            }
            if (!isempty(*Subtitle)) {
                if (!isempty(*TextAdditional))
                    TextAdditional.Append("\n");
                TextAdditional.Append(cString::sprintf("\n%s: %s", tr("Subtitle"), *Subtitle));
            }
        }  // if Components
    }  // EpgAdditionalInfoShow

    const int IconHeight = (m_chHeight - m_MarginItem2) * Config.EpgFskGenreIconSize * 100.0;  // Narrowing conversion
    int HeadIconLeft {m_chWidth - IconHeight - m_MarginItem};
    int HeadIconTop {m_chHeight - IconHeight - m_MarginItem};  // Position for fsk/genre image
    cString IconName {""};
    cImage *img {nullptr};
    if (strlen(*Fsk) > 0) {
        IconName = cString::sprintf("EPGInfo/FSK/%s", *Fsk);
        img = ImgLoader.LoadIcon(*IconName, IconHeight, IconHeight);
        if (img) {
            ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
            HeadIconLeft -= IconHeight + m_MarginItem;
        } else {
            isyslog("flatPlus: FSK icon not found: %s", *IconName);
            img = ImgLoader.LoadIcon("EPGInfo/FSK/unknown", IconHeight, IconHeight);
            if (img) {
                ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
                HeadIconLeft -= IconHeight + m_MarginItem;
            }
        }
    }
    bool IsUnknownDrawn {false};
    std::sort(GenreIcons.begin(), GenreIcons.end());
    GenreIcons.erase(unique(GenreIcons.begin(), GenreIcons.end()), GenreIcons.end());
    while (!GenreIcons.empty()) {
        IconName = cString::sprintf("EPGInfo/Genre/%s", GenreIcons.back().c_str());
        img = ImgLoader.LoadIcon(*IconName, IconHeight, IconHeight);
        if (img) {
            ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
            HeadIconLeft -= IconHeight + m_MarginItem;
        } else {
            isyslog("flatPlus: Genre icon not found: %s", *IconName);
            if (!IsUnknownDrawn) {
                img = ImgLoader.LoadIcon("EPGInfo/Genre/unknown", IconHeight, IconHeight);
                if (img) {
                    ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
                    HeadIconLeft -= IconHeight + m_MarginItem;
                    IsUnknownDrawn = true;
                }
            }
        }
        GenreIcons.pop_back();
    }

#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: SetEvent info-text time @ %ld ms", Timer.Elapsed());
#endif

    cString Reruns {""};
    if (Config.EpgRerunsShow) {
        // Lent from nopacity
        cPlugin *pEpgSearchPlugin = cPluginManager::GetPlugin("epgsearch");
        if (pEpgSearchPlugin && !isempty(Event->Title())) {
            // const std::string StrQuery = Event->Title();
            Epgsearch_searchresults_v1_0 data {
                // .query = (char *)StrQuery.c_str(),  // Search term
                .query = const_cast<char *>(Event->Title()),  // Search term //? Is this save?
                .mode = 0,                          // Search mode (0=phrase, 1=and, 2=or, 3=regular expression)
                .channelNr = 0,                     // Channel number to search in (0=any)
                .useTitle = true,                   // Search in title
                .useSubTitle = false,               // Search in subtitle
                .useDescription = false             // Search in description
            };

            if (pEpgSearchPlugin->Service("Epgsearch-searchresults-v1.0", &data)) {
                cList<Epgsearch_searchresults_v1_0::cServiceSearchResult> *list = data.pResultList;
                if (list && (list->Count() > 1)) {
                    int i {0};
                    for (Epgsearch_searchresults_v1_0::cServiceSearchResult *r = list->First(); r && i < 5;
                         r = list->Next(r)) {
                        if ((Event->ChannelID() == r->event->ChannelID()) &&
                            (Event->StartTime() == r->event->StartTime()))
                            continue;
                        ++i;
                        Reruns.Append(*DayDateTime(r->event->StartTime()));
                        LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
                        const cChannel *channel = Channels->GetByChannelID(r->event->ChannelID(), true, true);
                        if (channel)
                            Reruns.Append(cString::sprintf(", %d - %s", channel->Number(), channel->ShortName(true)));

                        Reruns.Append(cString::sprintf(":  %s", r->event->Title()));
                        // if (!isempty(r->event->ShortText()))
                        //    Reruns.Append(cString::sprintf("~%s", r->event->ShortText()));
                        Reruns.Append("\n");
                    }  // for
                    delete list;
                }  // if list
            }
        }
    }  // Config.EpgRerunsShow
#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: SetEvent reruns time @ %ld ms", Timer.Elapsed());
#endif

    std::vector<cString> ActorsPath, ActorsName, ActorsRole;

    cString MediaPath {""};
    cString MovieInfo {""}, SeriesInfo {""};

    int ContentTop {0};
    int MediaWidth {0}, MediaHeight {m_cHeight - m_MarginItem2 - m_FontHeight - 6};
    bool FirstRun {true}, SecondRun {false};
    bool Scrollable {false};

    do {  // Runs up to two times!
        if (FirstRun)
            m_cWidth -= m_WidthScrollBar;  // Assume that we need scrollbar most of the time
        else
            m_cWidth += m_WidthScrollBar;  // For second run readd scrollbar width

        ComplexContent.Clear();
        ComplexContent.SetOsd(m_Osd);
        ComplexContent.SetPosition(cRect(m_cLeft, m_cTop, m_cWidth, m_cHeight));
        ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));
        ComplexContent.SetScrollSize(m_FontHeight);
        ComplexContent.SetScrollingActive(true);

        MediaWidth = m_cWidth / 2 - m_MarginItem2;
        if (FirstRun) {  // Call scraper plugin only at first run and reuse data at second run
            static cPlugin *pScraper = GetScraperPlugin();
            if ((Config.TVScraperEPGInfoShowPoster || Config.TVScraperEPGInfoShowActors) && pScraper) {
                ScraperGetEventType call;
                call.event = Event;
                int seriesId {0}, episodeId {0}, movieId {0};

                if (pScraper->Service("GetEventType", &call)) {
                    seriesId = call.seriesId;
                    episodeId = call.episodeId;
                    movieId = call.movieId;
                }
                if (call.type == tSeries) {
                    cSeries series;
                    series.seriesId = seriesId;
                    series.episodeId = episodeId;
                    if (pScraper->Service("GetSeries", &series)) {
                        if (series.banners.size() > 1) {  // Use random banner
                            // Gets 'entropy' from device that generates random numbers itself
                            // to seed a mersenne twister (pseudo) random generator
                            std::mt19937 generator(std::random_device {}());

                            // Make sure all numbers have an equal chance.
                            // Range is inclusive (so we need -1 for vector index)
                            std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                            const std::size_t number {distribution(generator)};

                            MediaPath = series.banners[number].path.c_str();
                            dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                                    static_cast<int>(number + 1), *MediaPath,
                                    static_cast<int>(series.banners.size()));  // Log result
                        } else if (series.banners.size() == 1) {               // Just one banner
                            MediaPath = series.banners[0].path.c_str();
                        }
                        if (Config.TVScraperEPGInfoShowActors) {
                            const int ActorsSize = series.actors.size();
                            ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                            ActorsName.reserve(ActorsSize);
                            ActorsRole.reserve(ActorsSize);
                            for (int i {0}; i < ActorsSize; ++i) {
                                if (std::filesystem::exists(series.actors[i].actorThumb.path)) {
                                    ActorsPath.emplace_back(series.actors[i].actorThumb.path.c_str());
                                    ActorsName.emplace_back(series.actors[i].name.c_str());
                                    ActorsRole.emplace_back(series.actors[i].role.c_str());
                                }
                            }
                        }
                        InsertSeriesInfos(series, SeriesInfo);  // Add series infos
                    }
                } else if (call.type == tMovie) {
                    cMovie movie;
                    movie.movieId = movieId;
                    if (pScraper->Service("GetMovie", &movie)) {
                        MediaPath = movie.poster.path.c_str();
                        if (Config.TVScraperEPGInfoShowActors) {
                            const int ActorsSize = movie.actors.size();
                            ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                            ActorsName.reserve(ActorsSize);
                            ActorsRole.reserve(ActorsSize);
                            for (int i {0}; i < ActorsSize; ++i) {
                                if (std::filesystem::exists(movie.actors[i].actorThumb.path)) {
                                    ActorsPath.emplace_back(movie.actors[i].actorThumb.path.c_str());
                                    ActorsName.emplace_back(movie.actors[i].name.c_str());
                                    ActorsRole.emplace_back(movie.actors[i].role.c_str());
                                }
                            }
                        }
                        InsertMovieInfos(movie, MovieInfo);  // Add movie infos
                    }
                }
            }  // Scraper plugin
        }  // FirstRun
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: SetEvent tvscraper time @ %ld ms", Timer.Elapsed());
#endif
        ContentTop = m_MarginItem;
        if (!isempty(*Text) || !isempty(*MediaPath)) {  // Insert description line
            ComplexContent.AddText(tr("Description"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuEventTitleLine));
            ContentTop += 6;
        }
        if (!isempty(*MediaPath)) {
            img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);
            if (img) {  // Insert image with floating text
                ComplexContent.AddImageWithFloatedText(
                    img, CIP_Right, *Text,
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
            } else if (!isempty(*Text)) {  // No image; insert text
                ComplexContent.AddText(
                    *Text, true,
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
            }
        } else if (!isempty(*Text)) {  // No image; insert text
            ComplexContent.AddText(
                *Text, true, cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
        }

        if (!isempty(*MovieInfo)) {
            ContentTop = ComplexContent.BottomContent() + m_FontHeight;
            ComplexContent.AddText(tr("Movie information"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuEventTitleLine));
            ContentTop += 6;
            ComplexContent.AddText(
                *MovieInfo, true,
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
        }

        if (!isempty(*SeriesInfo)) {
            ContentTop = ComplexContent.BottomContent() + m_FontHeight;
            ComplexContent.AddText(tr("Series information"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuEventTitleLine));
            ContentTop += 6;
            ComplexContent.AddText(
                *SeriesInfo, true,
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
        }
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: SetEvent epg-text time @ %ld ms", Timer.Elapsed());
#endif

        const int NumActors = ActorsPath.size();  // Narrowing conversion
        if (Config.TVScraperEPGInfoShowActors && NumActors > 0) {
            //* Add actors to complexcontent for later displaying
            AddActors(ComplexContent, ActorsPath, ActorsName, ActorsRole, NumActors);
        }
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: SetEvent actor time @ %ld ms", Timer.Elapsed());
#endif

        if (!isempty(*Reruns)) {
            ContentTop = ComplexContent.BottomContent() + m_FontHeight;
            ComplexContent.AddText(tr("Reruns"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuEventTitleLine));
            ContentTop += 6;
            ComplexContent.AddText(
                *Reruns, true,
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
        }

        if (!isempty(*TextAdditional)) {
            ContentTop = ComplexContent.BottomContent() + m_FontHeight;
            ComplexContent.AddText(tr("Video information"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuEventTitleLine));
            ContentTop += 6;
            ComplexContent.AddText(
                *TextAdditional, true,
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
        }
        Scrollable = ComplexContent.Scrollable(m_cHeight - m_MarginItem2);
        if (Scrollable || SecondRun)
            break;             // No need for another run (Scrolling content or second run)
        if (FirstRun) {        // Second run because not scrolling content. Should be cheap to rerun
            SecondRun = true;  // Only runs when minimal contents fits in area of description
            FirstRun = false;
        }
    } while (FirstRun || SecondRun);

    if (Config.MenuContentFullSize || Scrollable)
        ComplexContent.CreatePixmaps(true);
    else
        ComplexContent.CreatePixmaps(false);

    ComplexContent.Draw();
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: SetRecording actor time @ %ld ms", Timer.Elapsed());
#endif

    PixmapFill(ContentHeadPixmap, clrTransparent);
    ContentHeadPixmap->DrawRectangle(cRect(0, 0, m_MenuWidth, m_FontHeight + m_FontSmlHeight * 2 + m_MarginItem2),
                                     Theme.Color(clrScrollbarBg));
    const cString StrTime =
        cString::sprintf("%s  %s - %s", *Event->GetDateString(), *Event->GetTimeString(), *Event->GetEndTimeString());

    const cString Title = Event->Title();
    const cString ShortText = (Event->ShortText()) ? Event->ShortText() : "";  // No short text. Show empty string
    const int ShortTextWidth {m_FontSml->Width(*ShortText)};
    const int MaxWidth {HeadIconLeft - m_MarginItem};
    int left {m_MarginItem};

    ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem), *StrTime, Theme.Color(clrMenuEventFontInfo),
                                Theme.Color(clrMenuEventBg), m_FontSml, m_MenuWidth - m_MarginItem2);
    ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight), *Title,
                                Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font,
                                m_MenuWidth - m_MarginItem2);
    // Add scroller to long short text
    if (ShortTextWidth > MaxWidth) {  // Short text too long
        if (Config.ScrollerEnable) {
            MenuItemScroller.AddScroller(*ShortText,
                                         cRect(m_chLeft + left, m_chTop + m_MarginItem + m_FontSmlHeight + m_FontHeight,
                                               MaxWidth, m_FontSmlHeight),
                                         Theme.Color(clrMenuEventFontInfo), clrTransparent, m_FontSml);
        } else {  // Add ... if info ist too long
            dsyslog("flatPlus: Short text too long! (%d) Setting MaxWidth to %d", ShortTextWidth, MaxWidth);
            const int DotsWidth {m_FontSml->Width("...")};
            ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight + m_FontHeight), *ShortText,
                                        Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                        MaxWidth - DotsWidth);
            left += (MaxWidth - DotsWidth);
            ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight + m_FontHeight), "...",
                                        Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                        DotsWidth);
        }
    } else {  // Short text fits into maxwidth
        ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight + m_FontHeight), *ShortText,
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                    ShortTextWidth);
    }

    const sDecorBorder ib {m_chLeft, m_chTop, m_chWidth, m_chHeight, Config.decorBorderMenuContentHeadSize,
                           Config.decorBorderMenuContentHeadType, Config.decorBorderMenuContentHeadFg,
                           Config.decorBorderMenuContentHeadBg};
    DecorBorderDraw(ib);

    if (Scrollable) {
        const int ScrollOffset {ComplexContent.ScrollOffset()};
        DrawScrollbar(ComplexContent.ScrollTotal(), ScrollOffset, ComplexContent.ScrollShown(),
                      ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ScrollOffset > 0,
                      ScrollOffset + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);
    }

    sDecorBorder ibContent{m_cLeft,
                           m_cTop,
                           m_cWidth,
                           (Config.MenuContentFullSize || Scrollable) ? ComplexContent.ContentHeight(true)
                                                                      : ComplexContent.ContentHeight(false),
                           Config.decorBorderMenuContentSize,
                           Config.decorBorderMenuContentType,
                           Config.decorBorderMenuContentFg,
                           Config.decorBorderMenuContentBg};

    DecorBorderDraw(ibContent);

#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: SetEvent total time: %ld ms", Timer.Elapsed());
#endif
}

void cFlatDisplayMenu::DrawItemExtraRecording(const cRecording *Recording, const cString EmptyText) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawItemExtraRecording()");
#endif

    m_cLeft = m_MenuItemWidth + Config.decorBorderMenuItemSize * 2 + Config.decorBorderMenuContentSize + m_MarginItem;
    if (m_IsScrolling)
        m_cLeft += m_WidthScrollBar;

    m_cTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
    m_cWidth = m_MenuWidth - m_cLeft - Config.decorBorderMenuContentSize;
    m_cHeight =
        m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                       Config.decorBorderButtonSize * 2 + m_MarginItem3 + Config.decorBorderMenuContentSize * 2);

    cString Text {""};
    if (Recording) {
        const cRecordingInfo *RecInfo = Recording->Info();

        if (!isempty(RecInfo->Description()))
            Text.Append(cString::sprintf("%s\n\n", RecInfo->Description()));

        // Lent from skinelchi
        if (Config.RecordingAdditionalInfoShow) {
            LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
            const cChannel *channel = Channels->GetByChannelID(RecInfo->ChannelID());
            if (channel)
                Text.Append(cString::sprintf("%s: %d - %s\n", trVDR("Channel"), channel->Number(), channel->Name()));

            const cEvent *Event = RecInfo->GetEvent();
            if (Event) {
                // Genre
                InsertGenreInfo(Event, Text);  // Add genre info

                if (Event->Contents(0))
                    Text.Append("\n");
                // FSK
                if (Event->ParentalRating())
                    Text.Append(cString::sprintf("%s: %s\n", tr("FSK"), *Event->GetParentalRatingString()));
            }

            InsertCuttedLengthSize(Recording, Text);  // Process marks and insert text

            // From SkinNopacity
#if APIVERSNUM >= 20505
            if (RecInfo && RecInfo->Errors() > 0) {
                std::ostringstream RecErrors {""};
                RecErrors.imbue(std::locale(""));  // Set to local locale
                RecErrors << RecInfo->Errors();
                Text.Append(cString::sprintf("%s: %s\n", tr("TS errors"), RecErrors.str().c_str()));
            }
#endif

            const cComponents *Components = RecInfo->Components();
            if (Components) {
                cString Audio {""}, Subtitle {""};
                InsertComponents(Components, Text, Audio, Subtitle, true);  // Get info for audio/video and subtitle

                if (!isempty(*Audio))
                    Text.Append(cString::sprintf("\n%s: %s", tr("Audio"), *Audio));
                if (!isempty(*Subtitle))
                    Text.Append(cString::sprintf("\n%s: %s", tr("Subtitle"), *Subtitle));
            }
            if (RecInfo->Aux())
                InsertAuxInfos(RecInfo, Text, true);  // Insert aux infos with info line
        }  // if Config.RecordingAdditionalInfoShow
    } else {
        Text.Append(*EmptyText);
    }

    ComplexContent.Clear();
    ComplexContent.SetScrollSize(m_FontSmlHeight);
    ComplexContent.SetScrollingActive(false);
    ComplexContent.SetOsd(m_Osd);
    ComplexContent.SetPosition(cRect(m_cLeft, m_cTop, m_cWidth, m_cHeight));
    ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));

    cString MediaPath {""};          // \/ Better use content hight
    int MediaWidth {0}, MediaHeight {m_cHeight - m_MarginItem2};
    int MediaType {0};

    static cPlugin *pScraper = GetScraperPlugin();
    if (Config.TVScraperRecInfoShowPoster && pScraper) {
        ScraperGetEventType call;
        call.recording = Recording;
        int seriesId {0}, episodeId {0}, movieId {0};

        if (pScraper->Service("GetEventType", &call)) {
            seriesId = call.seriesId;
            episodeId = call.episodeId;
            movieId = call.movieId;
        }
        if (call.type == tSeries) {
            cSeries series;
            series.seriesId = seriesId;
            series.episodeId = episodeId;
            if (pScraper->Service("GetSeries", &series)) {
                if (series.banners.size() > 1) {  // Use random banner
                    // Gets 'entropy' from device that generates random numbers itself
                    // to seed a mersenne twister (pseudo) random generator
                    std::mt19937 generator(std::random_device {}());

                    // Make sure all numbers have an equal chance.
                    // Range is inclusive (so we need -1 for vector index)
                    std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                    const std::size_t number = distribution(generator);

                    MediaPath = series.banners[number].path.c_str();
                    dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                            static_cast<int>(number + 1), *MediaPath,
                            static_cast<int>(series.banners.size()));  // Log result
                } else if (series.banners.size() == 1) {               // Just one banner
                    MediaPath = series.banners[0].path.c_str();
                }
                MediaWidth = m_cWidth - m_MarginItem2;
                MediaType = 1;
            }
        } else if (call.type == tMovie) {
            cMovie movie;
            movie.movieId = movieId;
            if (pScraper->Service("GetMovie", &movie)) {
                MediaPath = movie.poster.path.c_str();
                MediaWidth = m_cWidth / 2 - m_MarginItem3;
                MediaType = 2;
            }
        }
    }  // TVScraperRecInfoShowPoster

    cImage *img {nullptr};
    if (isempty(*MediaPath)) {  // Prio for tvscraper poster
        const cString RecPath = Recording->FileName();
        if (ImgLoader.SearchRecordingPoster(RecPath, MediaPath)) {
            img = ImgLoader.LoadFile(*MediaPath, m_cWidth - m_MarginItem2, MediaHeight);
            if (img) {
                const uint Aspect = img->Width() / img->Height();  // Narrowing conversion
                if (Aspect < 1) {  //* Poster (For example 680x1000 = 0.68)
                    MediaWidth = m_cWidth / 2 - m_MarginItem3;
                    MediaType = 2;
                } else {           //* Portrait (For example 1920x1080 = 1.77); Banner (Usually 758x140 = 5.41)
                    MediaWidth = m_cWidth - m_MarginItem2;
                    MediaType = 1;
                }
            }
        }
    }

    if (!isempty(*MediaPath)) {
        img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);
        if (img && MediaType == 2) {  // Movie
            ComplexContent.AddImageWithFloatedText(
                img, CIP_Right, *Text,
                cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
        } else if (img && MediaType == 1) {  // Series
            ComplexContent.AddImage(img, cRect(m_MarginItem, m_MarginItem, img->Width(), img->Height()));
            ComplexContent.AddText(*Text, true,
                                   cRect(m_MarginItem, m_MarginItem + img->Height(), m_cWidth - m_MarginItem2,
                                         m_cHeight - m_MarginItem2),
                                   Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
        } else {
            ComplexContent.AddText(
                *Text, true,
                cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
        }
    } else {
        ComplexContent.AddText(
            *Text, true, cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
            Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
    }

    ComplexContent.CreatePixmaps(Config.MenuContentFullSize);
    ComplexContent.Draw();

    DecorBorderClearByFrom(BorderContent);
    sDecorBorder ib{m_cLeft,
                    m_cTop,
                    m_cWidth,
                    (Config.MenuContentFullSize) ? ComplexContent.ContentHeight(true)
                                                 : ComplexContent.ContentHeight(false),
                    Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType,
                    Config.decorBorderMenuContentFg,
                    Config.decorBorderMenuContentBg,
                    BorderContent};

    DecorBorderDraw(ib);
}

void cFlatDisplayMenu::AddActors(cComplexContent &ComplexContent, std::vector<cString> &ActorsPath,
                                 std::vector<cString> &ActorsName, std::vector<cString> &ActorsRole,
                                 int NumActors) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::AddActors()");
#endif

    // Setting for TVScraperEPGInfoShowActors and TVScraperRecInfoShowActors
    const int ShowMaxActors {Config.TVScraperShowMaxActors};
    if (ShowMaxActors == 0) return;  // Do not show actors
    if (ShowMaxActors > 0 && ShowMaxActors < NumActors)
        NumActors = ShowMaxActors;   // Limit to ShowMaxActors (-1 = Show all actors)

    int ContentTop {ComplexContent.BottomContent() + m_FontHeight};
    ComplexContent.AddText(tr("Actors"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                           Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), m_Font);
    ContentTop += m_FontHeight;
    ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuRecTitleLine));
    ContentTop += 6;

    // Smaller font for actors name and role
    m_FontTiny = cFont::CreateFont(Setup.FontSml, Setup.FontSmlSize * 0.8);  // 80% of small font size
    const int FontTinyHeight {m_FontTiny->Height()};

    cImage *img {nullptr};
    cString Role {""};  // Actor role
    int Actor {0};
    const int ActorsPerLine {6};  // TODO: Config option?
    const int ActorWidth {m_cWidth / ActorsPerLine - m_MarginItem * 4};
    const int ActorMargin {((m_cWidth - m_MarginItem2) - ActorWidth * ActorsPerLine) / (ActorsPerLine - 1)};
    const uint PicsPerLine = (m_cWidth - m_MarginItem2) / ActorWidth;  // Narrowing conversion
    const uint PicLines {NumActors / PicsPerLine + (NumActors % PicsPerLine != 0)};
    int ImgHeight {0}, MaxImgHeight {0};
    int x {m_MarginItem};
    int y {ContentTop};
#ifdef DEBUGFUNCSCALL
    int y2 {ContentTop};  //! y2 is for testing
    dsyslog("   ActorsPerLine/PicsPerLine: %d/%d", ActorsPerLine, PicsPerLine);
    dsyslog("   ActorWidth/ActorMargin: %d/%d", ActorWidth, ActorMargin);
#endif
    if (NumActors > 50)
        dsyslog("flatPlus: Found %d actor images! First display will probably be slow.", NumActors);

    for (uint row {0}; row < PicLines; ++row) {
        for (uint col {0}; col < PicsPerLine; ++col) {
            if (Actor == NumActors)
                break;
            img = ImgLoader.LoadFile(*ActorsPath[Actor], ActorWidth, ICON_HEIGHT_UNLIMITED);
            if (img) {
                ComplexContent.AddImage(img, cRect(x, y, 0, 0));
                ImgHeight = img->Height();
                ComplexContent.AddText(*ActorsName[Actor], false, cRect(x, y + ImgHeight + m_MarginItem, ActorWidth, 0),
                                       Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontTiny,
                                       ActorWidth, FontTinyHeight, taCenter);
                Role = cString::sprintf("\"%s\"", *ActorsRole[Actor]);
                ComplexContent.AddText(
                    *Role, false, cRect(x, y + ImgHeight + m_MarginItem + FontTinyHeight, ActorWidth, 0),
                    Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontTiny, ActorWidth,
                    FontTinyHeight, taCenter);
#ifdef DEBUGFUNCSCALL
                if (ImgHeight > MaxImgHeight) {
                    dsyslog("   Column %d: MaxImgHeight changed to %d", col, ImgHeight);
                }
#endif
                MaxImgHeight = std::max(MaxImgHeight, ImgHeight);  // In case images have different size
            }
            x += ActorWidth + ActorMargin;
            ++Actor;
        }  // for col
        x = m_MarginItem;
        // y = ComplexContent.BottomContent() + m_FontHeight;
        y += MaxImgHeight + m_MarginItem + (FontTinyHeight * 2) + m_FontHeight;
#ifdef DEBUGFUNCSCALL
        // Alternate way to get y, but what if different image size?
        y2 += MaxImgHeight + m_MarginItem + (FontTinyHeight * 2) + m_FontHeight;
        dsyslog("   y/y2 BottomContent()/Calculation: %d/%d", ComplexContent.BottomContent() + m_FontHeight, y2);
#endif
        MaxImgHeight = 0;  // Reset value for next row
    }  // for row
}

void cFlatDisplayMenu::SetRecording(const cRecording *Recording) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetRecording()");
    cTimeMs Timer;  // Set Timer
#endif

    if (!ContentHeadPixmap || !ContentHeadIconsPixmap || !Recording) return;

    m_ShowEvent = false;
    m_ShowRecording = true;
    m_ShowText = false;
    ItemBorderClear();

    PixmapFill(ContentHeadIconsPixmap, clrTransparent);

    m_cLeft = Config.decorBorderMenuContentSize;
    m_cTop = m_chTop + m_MarginItem3 + m_FontHeight + m_FontSmlHeight * 2 + Config.decorBorderMenuContentSize +
             Config.decorBorderMenuContentHeadSize;
    m_cWidth = m_MenuWidth - Config.decorBorderMenuContentSize * 2;
    m_cHeight = m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                               Config.decorBorderButtonSize * 2 + m_MarginItem3 + m_chHeight +
                               Config.decorBorderMenuContentHeadSize * 2 + Config.decorBorderMenuContentSize * 2);

    if (!ButtonsDrawn())
        m_cHeight += m_ButtonsHeight + Config.decorBorderButtonSize * 2;

    m_MenuItemWidth = m_cWidth;

    cString RecAdditional {""}, Text {""}, TextAdditional {""};
    // Text.imbue(std::locale {""});
    // TextAdditional.imbue(std::locale {""});
    // RecAdditional.imbue(std::locale {""});

    const cRecordingInfo *RecInfo = Recording->Info();
    std::vector<std::string> GenreIcons;
    GenreIcons.reserve(8);

    if (!isempty(RecInfo->Description()))
        Text.Append(cString::sprintf("%s\n", RecInfo->Description()));

    cString Fsk {""};
    // Lent from skinelchi
    if (Config.RecordingAdditionalInfoShow) {
#ifdef DEBUGFUNCSCALL
        dsyslog("   RecordingAdditionalInfoShow() GetByChannelID() @ %ld ms", Timer.Elapsed());
#endif
        // Explanation for the following lines:
        // The call to `Channels->GetByChannelID(RecInfo->ChannelID())` is a potential source of
        // 'Invalid lock sequence' errors. This is because the `LOCK_CHANNELS_READ` call can cause a
        // lock on the `Channels` object to be taken, but the `cChannels` object is already locked when
        // the `cRecording` object is created. To avoid this, we use `std::async` to execute the code
        // that gets the channel information in a separate thread. This allows the lock on the `Channels`
        // object to be taken without causing an 'Invalid lock sequence' error.
        //
        // The `ChannelFuture.get()` call is used to wait for the asynchronous operation to complete.
        // This ensures that the `RecAdditional` string is properly updated with the channel information
        // before the `Text` string is updated with the `RecAdditional` string.
        auto ChannelFuture = std::async(
            [&RecAdditional](tChannelID channelId) {
                LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
                const cChannel *Channel = Channels->GetByChannelID(channelId);
                if (Channel)
                    RecAdditional.Append(
                        cString::sprintf("%s: %d - %s\n", trVDR("Channel"), Channel->Number(), Channel->Name()));
            },
            RecInfo->ChannelID());
        ChannelFuture.get();

#ifdef DEBUGFUNCSCALL
        dsyslog("   RecordingAdditionalInfoShow() GetByChannelID() done @ %ld ms", Timer.Elapsed());
#endif

        const cEvent *Event = RecInfo->GetEvent();
        if (Event) {
            // Genre
            InsertGenreInfo(Event, Text, GenreIcons);  // Add genre info

            if (Event->Contents(0))
                Text.Append("\n");
            // FSK
            if (Event->ParentalRating()) {
                Fsk = *Event->GetParentalRatingString();
                Text.Append(cString::sprintf("%s: %s\n", tr("FSK"), *Fsk));
            }
        }

        InsertCuttedLengthSize(Recording, RecAdditional);  // Process marks and insert text

        // From SkinNopacity
#if APIVERSNUM >= 20505
        if (RecInfo && RecInfo->Errors() > 0) {
            std::ostringstream RecErrors {""};
            RecErrors.imbue(std::locale {""});  // Set to local locale
            RecErrors << RecInfo->Errors();
            RecAdditional.Append(cString::sprintf("\n%s: %s", tr("TS errors"), RecErrors.str().c_str()));
        }
#endif

        const cComponents *Components = RecInfo->Components();
        if (Components) {
            cString Audio {""}, Subtitle {""};
            InsertComponents(Components, TextAdditional, Audio, Subtitle);  // Get info for audio/video and subtitle

            if (!isempty(*Audio)) {
                if (!isempty(*TextAdditional))
                    TextAdditional.Append("\n");
                TextAdditional.Append(cString::sprintf("%s: %s", tr("Audio"), *Audio));
            }
            if (!isempty(*Subtitle)) {
                if (!isempty(*TextAdditional))
                    TextAdditional.Append("\n");
                TextAdditional.Append(cString::sprintf("\n%s: %s", tr("Subtitle"), *Subtitle));
            }
        }
        if (RecInfo->Aux())
            InsertAuxInfos(RecInfo, RecAdditional, false);  // Insert aux infos without info line
    }  // if Config.RecordingAdditionalInfoShow

    const int IconHeight = (m_chHeight - m_MarginItem2) * Config.EpgFskGenreIconSize * 100.0;  // Narrowing conversion
    int HeadIconLeft {m_chWidth - IconHeight - m_MarginItem};
    int HeadIconTop {m_chHeight - IconHeight - m_MarginItem};  // Position for fsk/genre image
    cString IconName {""};
    cImage *img {nullptr};
    if (strlen(*Fsk) > 0) {
        IconName = cString::sprintf("EPGInfo/FSK/%s", *Fsk);
        img = ImgLoader.LoadIcon(*IconName, IconHeight, IconHeight);
        if (img) {
            ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
            HeadIconLeft -= IconHeight + m_MarginItem;
        } else {
            isyslog("flatPlus: FSK icon not found: %s", *IconName);
            img = ImgLoader.LoadIcon("EPGInfo/FSK/unknown", IconHeight, IconHeight);
            if (img) {
                ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
                HeadIconLeft -= IconHeight + m_MarginItem;
            }
        }
    }
    bool IsUnknownDrawn {false};
    std::sort(GenreIcons.begin(), GenreIcons.end());
    GenreIcons.erase(unique(GenreIcons.begin(), GenreIcons.end()), GenreIcons.end());
    while (!GenreIcons.empty()) {
        IconName = cString::sprintf("EPGInfo/Genre/%s", GenreIcons.back().c_str());
        img = ImgLoader.LoadIcon(*IconName, IconHeight, IconHeight);
        if (img) {
            ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
            HeadIconLeft -= IconHeight + m_MarginItem;
        } else {
            isyslog("flatPlus: Genre icon not found: %s", *IconName);
            if (!IsUnknownDrawn) {
                img = ImgLoader.LoadIcon("EPGInfo/Genre/unknown", IconHeight, IconHeight);
                if (img) {
                    ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
                    HeadIconLeft -= IconHeight + m_MarginItem;
                    IsUnknownDrawn = true;
                }
            }
        }
        GenreIcons.pop_back();
    }

#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: SetRecording info-text time @ %ld ms", Timer.Elapsed());
#endif

    std::vector<cString> ActorsPath, ActorsName, ActorsRole;

    cString MediaPath {""};
    cString MovieInfo {""}, SeriesInfo {""};

    int ContentTop {0};
    int MediaWidth {0}, MediaHeight {m_cHeight - m_MarginItem2 - m_FontHeight - 6};
    bool FirstRun {true}, SecondRun {false};
    bool Scrollable {false};

    do {  // Runs up to two times!
        if (FirstRun)
            m_cWidth -= m_WidthScrollBar;  // Assume that we need scrollbar most of the time
        else
            m_cWidth += m_WidthScrollBar;  // For second run readd scrollbar width

        ComplexContent.Clear();
        ComplexContent.SetOsd(m_Osd);
        ComplexContent.SetPosition(cRect(m_cLeft, m_cTop, m_cWidth, m_cHeight));
        ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));
        ComplexContent.SetScrollSize(m_FontHeight);
        ComplexContent.SetScrollingActive(true);

        MediaWidth = m_cWidth / 2 - m_MarginItem2;
        if (FirstRun) {  // Call scraper plugin only at first run and reuse data at second run
            static cPlugin *pScraper = GetScraperPlugin();
            if ((Config.TVScraperRecInfoShowPoster || Config.TVScraperRecInfoShowActors) && pScraper) {
                ScraperGetEventType call;
                call.recording = Recording;
                int seriesId {0}, episodeId {0}, movieId {0};

                if (pScraper->Service("GetEventType", &call)) {
                    seriesId = call.seriesId;
                    episodeId = call.episodeId;
                    movieId = call.movieId;
                }
                if (call.type == tSeries) {
                    cSeries series;
                    series.seriesId = seriesId;
                    series.episodeId = episodeId;
                    if (pScraper->Service("GetSeries", &series)) {
                        if (series.banners.size() > 1) {  // Use random banner
                            // Gets 'entropy' from device that generates random numbers itself
                            // to seed a mersenne twister (pseudo) random generator
                            std::mt19937 generator(std::random_device {}());

                            // Make sure all numbers have an equal chance.
                            // Range is inclusive (so we need -1 for vector index)
                            std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                            const std::size_t number {distribution(generator)};

                            MediaPath = series.banners[number].path.c_str();
                            dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                                    static_cast<int>(number + 1), *MediaPath,
                                    static_cast<int>(series.banners.size()));  // Log result
                        } else if (series.banners.size() == 1) {               // Just one banner
                            MediaPath = series.banners[0].path.c_str();
                        }
                        if (Config.TVScraperRecInfoShowActors) {
                            const int ActorsSize = series.actors.size();
                            ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                            ActorsName.reserve(ActorsSize);
                            ActorsRole.reserve(ActorsSize);
                            for (int i {0}; i < ActorsSize; ++i) {
                                if (std::filesystem::exists(series.actors[i].actorThumb.path)) {
                                    ActorsPath.emplace_back(series.actors[i].actorThumb.path.c_str());
                                    ActorsName.emplace_back(series.actors[i].name.c_str());
                                    ActorsRole.emplace_back(series.actors[i].role.c_str());
                                }
                            }
                        }
                        InsertSeriesInfos(series, SeriesInfo);  // Add series infos
                    }
                } else if (call.type == tMovie) {
                    cMovie movie;
                    movie.movieId = movieId;
                    if (pScraper->Service("GetMovie", &movie)) {
                        MediaPath = movie.poster.path.c_str();
                        if (Config.TVScraperRecInfoShowActors) {
                            const int ActorsSize = movie.actors.size();
                            ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                            ActorsName.reserve(ActorsSize);
                            ActorsRole.reserve(ActorsSize);
                            for (int i {0}; i < ActorsSize; ++i) {
                                if (std::filesystem::exists(movie.actors[i].actorThumb.path)) {
                                    ActorsPath.emplace_back(movie.actors[i].actorThumb.path.c_str());
                                    ActorsName.emplace_back(movie.actors[i].name.c_str());
                                    ActorsRole.emplace_back(movie.actors[i].role.c_str());
                                }
                            }
                        }
                        InsertMovieInfos(movie, MovieInfo);  // Add movie infos
                    }
                }
            }  // Scraper plugin

            if (isempty(*MediaPath)) {  // Prio for tvscraper poster
                const cString RecPath = Recording->FileName();
                ImgLoader.SearchRecordingPoster(RecPath, MediaPath);
            }
        }  // FirstRun

#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: SetRecording tvscraper time @ %ld ms", Timer.Elapsed());
#endif

        ContentTop = m_MarginItem;
        if (!isempty(*Text) || !isempty(*MediaPath)) {  // Insert description line
            ComplexContent.AddText(tr("Description"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuRecTitleLine));
            ContentTop += 6;
        }
        if (!isempty(*MediaPath)) {
            img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);

            //* Make portrait smaller than poster or banner to prevent wasting of space
            if (img) {
                const uint Aspect = img->Width() / img->Height();  // Narrowing conversion
                if (Aspect > 1 && Aspect < 4) {  //* Portrait (For example 1920x1080)
                    // dsyslog("flatPlus: SetRecording() Portrait image %dx%d (%d) found! Setting to 2/3 size.",
                    //         img->Width(), img->Height(), Aspect);
                    MediaHeight *= (2.0 / 3.0);  // Size * 0,666 = 2/3
                    img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);  // Reload portrait with new size
                }
            }
            if (img) {  // Insert image with floating text
                ComplexContent.AddImageWithFloatedText(
                    img, CIP_Right, *Text,
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
            } else if (!isempty(*Text)) {  // No image; insert text
                ComplexContent.AddText(
                    *Text, true,
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
            }
        } else if (!isempty(*Text)) {  // No image; insert text
            ComplexContent.AddText(
                *Text, true, cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
        }

        if (!isempty(*MovieInfo)) {
            ContentTop = ComplexContent.BottomContent() + m_FontHeight;
            ComplexContent.AddText(tr("Movie information"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuRecTitleLine));
            ContentTop += 6;
            ComplexContent.AddText(
                *MovieInfo, true,
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
        }

        if (!isempty(*SeriesInfo)) {
            ContentTop = ComplexContent.BottomContent() + m_FontHeight;
            ComplexContent.AddText(tr("Series information"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuRecTitleLine));
            ContentTop += 6;
            ComplexContent.AddText(
                *SeriesInfo, true,
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
        }
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: SetRecording epg-text time @ %ld ms", Timer.Elapsed());
#endif

        const int NumActors = ActorsPath.size();  // Narrowing conversion
        if (Config.TVScraperRecInfoShowActors && NumActors > 0) {
            //* Add actors to complexcontent for later displaying
            AddActors(ComplexContent, ActorsPath, ActorsName, ActorsRole, NumActors);
        }
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: SetRecording actor time @ %ld ms", Timer.Elapsed());
#endif

        if (!isempty(*RecAdditional)) {
            ContentTop = ComplexContent.BottomContent() + m_FontHeight;
            ComplexContent.AddText(tr("Recording information"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuRecTitleLine));
            ContentTop += 6;
            ComplexContent.AddText(
                *RecAdditional, true,
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
        }

        if (!isempty(*TextAdditional)) {
            ContentTop = ComplexContent.BottomContent() + m_FontHeight;
            ComplexContent.AddText(tr("Video information"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                                   Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuRecTitleLine));
            ContentTop += 6;
            ComplexContent.AddText(
                *TextAdditional, true,
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
        }

        Scrollable = ComplexContent.Scrollable(m_cHeight - m_MarginItem2);
        if (Scrollable || SecondRun)
            break;             // No need for another run (Scrolling content or second run)
        if (FirstRun) {        // Second run because not scrolling content. Should be cheap to rerun
            SecondRun = true;  // Only runs when minimal contents fits in area of description
            FirstRun = false;
        }
    } while (FirstRun || SecondRun);

    if (Config.MenuContentFullSize || Scrollable)
        ComplexContent.CreatePixmaps(true);
    else
        ComplexContent.CreatePixmaps(false);

    ComplexContent.Draw();
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: SetRecording complexcontent draw time @ %ld ms", Timer.Elapsed());
#endif

    PixmapFill(ContentHeadPixmap, clrTransparent);
    ContentHeadPixmap->DrawRectangle(cRect(0, 0, m_MenuWidth, m_FontHeight + m_FontSmlHeight * 2 + m_MarginItem2),
                                     Theme.Color(clrScrollbarBg));

    const cString StrTime =
        cString::sprintf("%s  %s  %s", *DateString(Recording->Start()), *TimeString(Recording->Start()),
                         (RecInfo->ChannelName()) ? RecInfo->ChannelName() : "");

    cString Title = RecInfo->Title();
    if (isempty(*Title))
        Title = Recording->Name();

    const cString ShortText = (RecInfo->ShortText() ? RecInfo->ShortText() : " - ");  // No short text. Show ' - '
    const int ShortTextWidth {m_FontSml->Width(*ShortText)};
    int MaxWidth {HeadIconLeft - m_MarginItem};  // Reduce redundant calculations
    int left {m_MarginItem};

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors)
        MaxWidth -= m_FontSmlHeight;  // Substract width of imgRecErr
#endif

    ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem), *StrTime, Theme.Color(clrMenuRecFontInfo),
                                Theme.Color(clrMenuRecBg), m_FontSml, m_MenuWidth - m_MarginItem2);
    ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight), *Title, Theme.Color(clrMenuRecFontTitle),
                                Theme.Color(clrMenuRecBg), m_Font, m_MenuWidth - m_MarginItem2);
    // Add scroller to long short text
    if (ShortTextWidth > MaxWidth) {  // Short text too long
        if (Config.ScrollerEnable) {
            MenuItemScroller.AddScroller(*ShortText,
                                         cRect(m_chLeft + left, m_chTop + m_MarginItem + m_FontSmlHeight + m_FontHeight,
                                               MaxWidth, m_FontSmlHeight),
                                         Theme.Color(clrMenuRecFontInfo), clrTransparent, m_FontSml);
            left += MaxWidth;
        } else {  // Add ... if info ist too long
            dsyslog("flatPlus: Short text too long! (%d) Setting MaxWidth to %d", ShortTextWidth, MaxWidth);
            const int DotsWidth {m_FontSml->Width("...")};
            ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight + m_FontHeight), *ShortText,
                                        Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml,
                                        MaxWidth - DotsWidth);
            left += (MaxWidth - DotsWidth);
            ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight + m_FontHeight), "...",
                                        Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml,
                                        DotsWidth);
            left += DotsWidth;
        }
    } else {  // Short text fits into maxwidth
        ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight + m_FontHeight), *ShortText,
                                    Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml,
                                    ShortTextWidth);
        left += ShortTextWidth;
    }

#if APIVERSNUM >= 20505
    if (Config.MenuItemRecordingShowRecordingErrors) {  // TODO: Separate config option?
        const cString RecErrIcon = cString::sprintf("%s_replay", *GetRecordingErrorIcon(RecInfo->Errors()));

        img = ImgLoader.LoadIcon(*RecErrIcon, ICON_WIDTH_UNLIMITED, m_FontSmlHeight);  // Small image
        if (img) {
            left += m_MarginItem;
            const int ImageTop {m_MarginItem + m_FontSmlHeight + m_FontHeight};
            ContentHeadIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
        }
    }  // MenuItemRecordingShowRecordingErrors
#endif

    const sDecorBorder ib {m_chLeft,
                           m_chTop,
                           m_chWidth,
                           m_chHeight,
                           Config.decorBorderMenuContentHeadSize,
                           Config.decorBorderMenuContentHeadType,
                           Config.decorBorderMenuContentHeadFg,
                           Config.decorBorderMenuContentHeadBg,
                           BorderSetRecording};
    DecorBorderDraw(ib, false);

    if (Scrollable) {
        const int ScrollOffset {ComplexContent.ScrollOffset()};
        DrawScrollbar(ComplexContent.ScrollTotal(), ScrollOffset, ComplexContent.ScrollShown(),
                      ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ScrollOffset > 0,
                      ScrollOffset + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);
    }

    RecordingBorder = {
        m_cLeft,
        m_cTop,
        m_cWidth,
        ComplexContent.Height(),
        Config.decorBorderMenuContentSize,
        Config.decorBorderMenuContentType,
        Config.decorBorderMenuContentFg,
        Config.decorBorderMenuContentBg,
        BorderMenuRecord
    };

    sDecorBorder ibRecording = RecordingBorder;
    ibRecording.Height = (Config.MenuContentFullSize || Scrollable) ? ComplexContent.ContentHeight(true)
                                                                    : ComplexContent.ContentHeight(false);
    DecorBorderDraw(ibRecording, false);

#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: SetRecording total time: %ld ms", Timer.Elapsed());
#endif
}

void cFlatDisplayMenu::SetText(const char *Text, bool FixedFont) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetText()");
#endif

    if (!Text) return;

    m_ShowEvent = false;
    m_ShowRecording = false;
    m_ShowText = true;
    ItemBorderClear();

    PixmapFill(ContentHeadPixmap, clrTransparent);

    const int Left {Config.decorBorderMenuContentSize};  // Config.decorBorderMenuContentSize
    const int Top {m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Left};
    int Width {m_MenuWidth - Left * 2};
    int Height {m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                Config.decorBorderButtonSize * 2 + Left * 2 + m_MarginItem)};

    if (!ButtonsDrawn())
        Height += m_ButtonsHeight + Config.decorBorderButtonSize * 2;

    ComplexContent.Clear();

    m_MenuItemWidth = Width;

    Width -= m_WidthScrollBar;  // First assume text is with scrollbar and redraw if not
    if (FixedFont) {
        ComplexContent.AddText(Text, true,
                               cRect(m_MarginItem, m_MarginItem, Width - m_MarginItem2, Height - m_MarginItem2),
                               Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), m_FontFixed);
        ComplexContent.SetScrollSize(m_FontFixedHeight);
    } else {
        ComplexContent.AddText(Text, true,
                               cRect(m_MarginItem, m_MarginItem, Width - m_MarginItem2, Height - m_MarginItem2),
                               Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), m_Font);
        ComplexContent.SetScrollSize(m_FontHeight);
    }

    const bool Scrollable {ComplexContent.Scrollable(Height - m_MarginItem2)};
    if (!Scrollable) {
        Width += m_WidthScrollBar;  // Readd scrollbar width
        ComplexContent.Clear();
        if (FixedFont) {
            ComplexContent.AddText(
                Text, true, cRect(m_MarginItem, m_MarginItem, Width - m_MarginItem2, Height - m_MarginItem2),
                Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), m_FontFixed);
            ComplexContent.SetScrollSize(m_FontFixedHeight);
        } else {
            ComplexContent.AddText(
                Text, true, cRect(m_MarginItem, m_MarginItem, Width - m_MarginItem2, Height - m_MarginItem2),
                Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), m_Font);
            ComplexContent.SetScrollSize(m_FontHeight);
        }
    }

    ComplexContent.SetOsd(m_Osd);
    ComplexContent.SetPosition(cRect(Left, Top, Width, Height));
    ComplexContent.SetBGColor(Theme.Color(clrMenuTextBg));
    ComplexContent.SetScrollingActive(true);

    if (Config.MenuContentFullSize || Scrollable)
        ComplexContent.CreatePixmaps(true);
    else
        ComplexContent.CreatePixmaps(false);

    ComplexContent.Draw();

    if (Scrollable) {
        const int ScrollOffset {ComplexContent.ScrollOffset()};
        DrawScrollbar(ComplexContent.ScrollTotal(), ScrollOffset, ComplexContent.ScrollShown(),
                      ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ScrollOffset > 0,
                      ScrollOffset + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(),
                      true);
    }

    sDecorBorder ib {.Left = Left,
                     .Top = Top,
                     .Width = Width,
                     .Height = (Config.MenuContentFullSize || Scrollable) ? ComplexContent.ContentHeight(true)
                                                                          : ComplexContent.ContentHeight(false),
                     .Size = Left,
                     .Type = Config.decorBorderMenuContentType,
                     .ColorFg = Config.decorBorderMenuContentFg,
                     .ColorBg = Config.decorBorderMenuContentBg};

    DecorBorderDraw(ib);
}

int cFlatDisplayMenu::GetTextAreaWidth() const {
    return m_MenuWidth - m_MarginItem2;
}

const cFont *cFlatDisplayMenu::GetTextAreaFont(bool FixedFont) const {
    return (FixedFont) ? m_FontFixed : m_Font;
}

void cFlatDisplayMenu::SetMenuSortMode(eMenuSortMode MenuSortMode) {
    // Do not set sort icon if mode is unknown
    const char* SortIcons[] {"SortNumber", "SortName", "SortDate", "SortProvider"};
    if (MenuSortMode > msmUnknown && MenuSortMode <= msmProvider) {
        TopBarSetMenuIconRight(SortIcons[MenuSortMode - 1]);
    }
}

void cFlatDisplayMenu::Flush() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::Flush()");
#endif

    if (!MenuPixmap) return;

    if (Config.MenuFullOsd && !m_MenuFullOsdIsDrawn) {
        MenuPixmap->DrawRectangle(cRect(0, m_MenuItemLastHeight - Config.decorBorderMenuItemSize,
                                        m_MenuItemWidth + Config.decorBorderMenuItemSize * 2,
                                        MenuPixmap->ViewPort().Height() - m_MenuItemLastHeight + m_MarginItem),
                                  Theme.Color(clrItemSelableBg));
        // MenuPixmap->DrawRectangle(cRect(0, MenuPixmap->ViewPort().Height() - 5, m_MenuItemWidth +
        //                           Config.decorBorderMenuItemSize * 2, 5), Theme.Color(clrItemSelableBg));
        m_MenuFullOsdIsDrawn = true;
    }

    if (m_MenuCategory == mcTimer && Config.MenuTimerShowCount) {
        uint TimerActiveCount {0}, TimerCount {0};
        GetTimerCounts(TimerActiveCount, TimerCount);

        if (m_LastTimerActiveCount != TimerActiveCount || m_LastTimerCount != TimerCount) {
            m_LastTimerActiveCount = TimerActiveCount;
            m_LastTimerCount = TimerCount;
            const cString NewTitle = cString::sprintf("%s (%d/%d)", *m_LastTitle, TimerActiveCount, TimerCount);
            TopBarSetTitle(*NewTitle, false);  // Do not clear
        }
    }

    if (cVideoDiskUsage::HasChanged(m_VideoDiskUsageState))
        TopBarEnableDiskUsage();  // Keep 'DiskUsage' up to date

    TopBarUpdate();
    m_Osd->Flush();
}

void cFlatDisplayMenu::ItemBorderInsertUnique(const sDecorBorder &ib) {
    std::vector<sDecorBorder>::iterator it, end {ItemsBorder.end()};
    for (it = ItemsBorder.begin(); it != end; ++it) {
        if ((*it).Left == ib.Left && (*it).Top == ib.Top) {
            (*it) = ib;
            return;
        }
    }

    ItemsBorder.emplace_back(ib);
}

void cFlatDisplayMenu::ItemBorderDrawAllWithScrollbar() {
    for (auto &Border : ItemsBorder) {
        Border.Width -= m_WidthScrollBar;
        DecorBorderDraw(Border);
        // Border.Width += m_WidthScrollBar;  // Restore original width
    }
}

void cFlatDisplayMenu::ItemBorderDrawAllWithoutScrollbar() {
    for (auto &Border : ItemsBorder) {
        Border.Width += m_WidthScrollBar;
        DecorBorderDraw(Border);
        // Border.Width -= m_WidthScrollBar;  // Restore original width
    }
}

void cFlatDisplayMenu::ItemBorderClear() {
    ItemsBorder.clear();
}

bool cFlatDisplayMenu::IsRecordingOld(const cRecording *Recording, int Level) {
    const std::string RecFolder {*GetRecordingName(Recording, Level, true)};

    int value {Config.GetRecordingOldValue(RecFolder)};
    if (value < 0) value = Config.MenuItemRecordingDefaultOldDays;
    if (value < 0) return false;

    const time_t LastRecTimeFromFolder {GetLastRecTimeFromFolder(Recording, Level)};
    const time_t now {time(0)};

    const double days = difftime(now, LastRecTimeFromFolder) / (60 * 60 * 24);
    return days > value;
}

time_t cFlatDisplayMenu::GetLastRecTimeFromFolder(const cRecording *Recording, int Level) {
    time_t RecStart {Recording->Start()};

    if (Config.MenuItemRecordingShowFolderDate == 0) return RecStart;  // None (default)

    const std::string RecFolder {*GetRecordingName(Recording, Level, true)};
    std::string RecFolder2 {""};
    RecFolder2.reserve(256);

    LOCK_RECORDINGS_READ;
    for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
        RecFolder2 = *GetRecordingName(Rec, Level, true);
        if (RecFolder == RecFolder2) {  // Recordings must be in the same folder
            if (Config.MenuItemRecordingShowFolderDate == 1) {  // Newest
                RecStart = std::max(RecStart, Rec->Start());
            } else if (Config.MenuItemRecordingShowFolderDate == 2) {  // Oldest
                RecStart = std::min(RecStart, Rec->Start());
            }
        }
    }

    return RecStart;
}

cString cFlatDisplayMenu::GetRecordingName(const cRecording *Recording, int Level, bool IsFolder) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::GetRecordingName() Level %d", Level);
#endif

    if (!Recording) return "";

    std::string_view RecName {Recording->Name()};
    std::string RecNamePart {""};
    RecNamePart.reserve(64);

    std::size_t start {0}, end {0};
    for (int i {0}; i <= Level; ++i) {
        end = RecName.find(FOLDERDELIMCHAR, start);
        if (end == std::string::npos) {
            if (i == Level) RecNamePart = RecName.substr(start);
            break;
        }
        if (i == Level) {
            RecNamePart = RecName.substr(start, end - start);
            break;
        }
        start = end + 1;
    }

    if (Config.MenuItemRecordingClearPercent && IsFolder && !RecNamePart.empty() && RecNamePart[0] == '%') {
        RecNamePart.erase(0, 1);
    }
#ifdef DEBUGFUNCSCALL
    dsyslog("   RecNamePart '%s'", RecNamePart.c_str());
#endif

    return RecNamePart.c_str();
}

cString cFlatDisplayMenu::GetRecCounts() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::GetRecCounts() m_RecFolder, m_LastItemRecordingLevel: '%s', %d",
            m_RecFolder.c_str(), m_LastItemRecordingLevel);
#endif

    uint RecCount {0}, RecNewCount {0};
    m_LastRecFolder = m_RecFolder;
    if (m_RecFolder != "" && m_LastItemRecordingLevel > 0) {
        std::string RecFolder2 {""};
        RecFolder2.reserve(256);
        LOCK_RECORDINGS_READ;
        for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
            RecFolder2 = *GetRecordingName(Rec, m_LastItemRecordingLevel - 1, true);
            if (m_RecFolder == RecFolder2) {
                ++RecCount;
                if (Rec->IsNew()) ++RecNewCount;
            }
        }  // for
    } else {  // All recordings
        LOCK_RECORDINGS_READ;
        for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
            ++RecCount;
            if (Rec->IsNew()) ++RecNewCount;
        }
    }
    cString RecCounts {""};
    if (Config.ShortRecordingCount) {  // Hidden option. 0 = disable, 1 = enable
        if (RecNewCount == 0)          // No new recordings
            RecCounts = cString::sprintf("(%d)", RecCount);      // (56)
        else if (RecNewCount == RecCount)                        // Only new recordings
            RecCounts = cString::sprintf("(%d*)", RecNewCount);  // (35*)
        else
            RecCounts = cString::sprintf("(%d*/%d)", RecNewCount, RecCount);  // (35*/56)
    } else {
        RecCounts = cString::sprintf("(%d*/%d)", RecNewCount, RecCount);      // (35*/56)
    }  // Config.ShortRecordingCount */

    return RecCounts;
}

void cFlatDisplayMenu::GetTimerCounts(uint &TimerActiveCount, uint &TimerCount) {
    TimerActiveCount = 0;
    TimerCount = 0;
    LOCK_TIMERS_READ;  // Creates local const cTimers *Timers
    for (const cTimer *Timer = Timers->First(); Timer; Timer = Timers->Next(Timer)) {
        ++TimerCount;
        if (Timer->HasFlags(tfActive)) ++TimerActiveCount;
    }
}

void cFlatDisplayMenu::InsertGenreInfo(const cEvent *Event, cString &Text) {
    bool FirstContent {true};
    for (int i {0}; Event->Contents(i); ++i) {
        if (!isempty(Event->ContentToString(Event->Contents(i)))) {  // Skip empty (user defined) content
            if (!FirstContent)
                Text.Append(", ");
            else
                Text.Append(cString::sprintf("\n%s: ", tr("Genre")));

            Text.Append(Event->ContentToString(Event->Contents(i)));
            FirstContent = false;
        }
    }
}

void cFlatDisplayMenu::InsertGenreInfo(const cEvent *Event, cString &Text, std::vector<std::string> &GenreIcons) {
    bool FirstContent {true};
    for (int i {0}; Event->Contents(i); ++i) {
        if (!isempty(Event->ContentToString(Event->Contents(i)))) {  // Skip empty (user defined) content
            if (!FirstContent)
                Text.Append(", ");
            else
                Text.Append(cString::sprintf("\n%s: ", tr("Genre")));

            Text.Append(Event->ContentToString(Event->Contents(i)));
            FirstContent = false;
            GenreIcons.emplace_back(GetGenreIcon(Event->Contents(i)));
        }
    }
}

void cFlatDisplayMenu::InsertSeriesInfos(const cSeries &Series, cString &SeriesInfo) {  // NOLINT
    if (Series.name.length() > 0) SeriesInfo.Append(cString::sprintf("%s%s\n", tr("name: "), Series.name.c_str()));
    if (Series.firstAired.length() > 0)
        SeriesInfo.Append(cString::sprintf("%s%s\n", tr("first aired: "), Series.firstAired.c_str()));
    if (Series.network.length() > 0)
        SeriesInfo.Append(cString::sprintf("%s%s\n", tr("network: "), Series.network.c_str()));
    if (Series.genre.length() > 0) SeriesInfo.Append(cString::sprintf("%s%s\n", tr("genre: "), Series.genre.c_str()));
    if (Series.rating > 0) SeriesInfo.Append(cString::sprintf("%s%.1f\n", tr("rating: "), Series.rating));  // TheTVDB
    if (Series.status.length() > 0)
        SeriesInfo.Append(cString::sprintf("%s%s\n", tr("status: "), Series.status.c_str()));
    if (Series.episode.season > 0)
        SeriesInfo.Append(cString::sprintf("%s%d\n", tr("season number: "), Series.episode.season));
    if (Series.episode.number > 0)
        SeriesInfo.Append(cString::sprintf("%s%d\n", tr("episode number: "), Series.episode.number));
}

void cFlatDisplayMenu::InsertMovieInfos(const cMovie &Movie, cString &MovieInfo) {  // NOLINT
    if (Movie.title.length() > 0) MovieInfo.Append(cString::sprintf("%s%s\n", tr("title: "), Movie.title.c_str()));
    if (Movie.originalTitle.length() > 0)
        MovieInfo.Append(cString::sprintf("%s%s\n", tr("original title: "), Movie.originalTitle.c_str()));
    if (Movie.collectionName.length() > 0)
        MovieInfo.Append(cString::sprintf("%s%s\n", tr("collection name: "), Movie.collectionName.c_str()));
    if (Movie.genres.length() > 0) MovieInfo.Append(cString::sprintf("%s%s\n", tr("genre: "), Movie.genres.c_str()));
    if (Movie.releaseDate.length() > 0)
        MovieInfo.Append(cString::sprintf("%s%s\n", tr("release date: "), Movie.releaseDate.c_str()));
    // TheMovieDB
    if (Movie.popularity > 0) MovieInfo.Append(cString::sprintf("%s%.1f\n", tr("popularity: "), Movie.popularity));
    if (Movie.voteAverage > 0)
        MovieInfo.Append(
            cString::sprintf("%s%.0f%%\n", tr("vote average: "), Movie.voteAverage * 10));  // 10 Points = 100%
}

const char *cFlatDisplayMenu::GetGenreIcon(uchar genre) {
    static const char *icons[][16] {
        // MovieDrama
        {"Movie_Drama", "Detective_Thriller", "Adventure_Western_War", "Science Fiction_Fantasy_Horror", "Comedy",
         "Soap_Melodrama_Folkloric", "Romance", "Serious_Classical_Religious_Historical Movie_Drama",
         "Adult Movie_Drama"},
        // NewsCurrentAffairs
        {"News_Current Affairs", "News_Weather Report", "News Magazine", "Documentary", "Discussion_Interview_Debate"},
        // Show
        {"Show_Game Show", "Game Show_Quiz_Contest", "Variety Show", "Talk Show"},
        // Sports
        {"Sports", "Special Event", "Sport Magazine", "Football_Soccer", "Tennis_Squash", "Team Sports", "Athletics",
         "Motor Sport", "Water Sport", "Winter Sports", "Equestrian", "Martial Sports"},
        // ChildrenYouth
        {"Childrens_Youth Programme", "Pre-school Childrens Programme", "Entertainment Programme for 6 to 14",
         "Entertainment Programme for 10 to 16", "Informational_Educational_School Programme", "Cartoons_Puppets"},
        // MusicBalletDance
        {"Music_Ballet_Dance", "Rock_Pop", "Serious_Classical Music", "Folk_Traditional Music", "Jazz", "Musical_Opera",
         "Ballet"},
        // ArtsCulture
        {"Arts_Culture", "Performing Arts", "Fine Arts", "Religion", "Popular Culture_Traditional Arts", "Literature",
         "Film_Cinema", "Experimental Film_Video", "Broadcasting_Press", "New Media", "Arts_Culture Magazine",
         "Fashion"},
        // SocialPoliticalEconomics
        {"Social_Political_Economics", "Magazine_Report_Documentary", "Economics_Social Advisory", "Remarkable People"},
        // EducationalScience
        {"Education_Science_Factual", "Nature_Animals_Environment", "Technology_Natural Sciences",
         "Medicine_Physiology_Psychology", "Foreign Countries_Expeditions", "Social_Spiritual Sciences",
         "Further Education", "Languages"},
        // LeisureHobbies
        {"Leisure_Hobbies", "Tourism_Travel", "Handicraft", "Motoring", "Fitness_Health", "Cooking",
         "Advertisement_Shopping", "Gardening"},
        // Special
        {"Original Language", "Black & White", "Unpublished", "Live Broadcast"}};

    static const uchar BaseGenres[] {
        ecgMovieDrama, ecgNewsCurrentAffairs, ecgShow, ecgSports, ecgChildrenYouth,
        ecgMusicBalletDance, ecgArtsCulture, ecgSocialPoliticalEconomics,
        ecgEducationalScience, ecgLeisureHobbies, ecgSpecial
    };

    const size_t GenreNums {sizeof(BaseGenres) / sizeof(BaseGenres[0])};
    for (size_t i {0}; i < GenreNums; ++i) {
        if ((genre & 0xF0) == BaseGenres[i]) {
            // Return the first icon for the genre if field is empty
            return (isempty(icons[i][genre & 0x0F])) ? icons[i][0] : icons[i][genre & 0x0F];
        }
    }

    isyslog("flatPlus: Genre not found: %x", genre);
    return "";
}

/* Widgets */
void cFlatDisplayMenu::DrawMainMenuWidgets() {
    if (!MenuPixmap) return;

    const int wLeft = m_OsdWidth * Config.MainMenuItemScale + m_MarginItem + Config.decorBorderMenuContentSize +
                      Config.decorBorderMenuItemSize;  // Narrowing conversion
    const int wTop {m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 +
                    Config.decorBorderMenuContentSize};

    const int wWidth {m_OsdWidth - wLeft - Config.decorBorderMenuContentSize};
    const int wHeight {MenuPixmap->ViewPort().Height() - m_MarginItem2};

    ContentWidget.Clear();
    ContentWidget.SetOsd(m_Osd);
    ContentWidget.SetPosition(cRect(wLeft, wTop, wWidth, wHeight));
    ContentWidget.SetBGColor(Theme.Color(clrMenuRecBg));
    ContentWidget.SetScrollingActive(false);

    std::vector<std::pair<int, std::string>> widgets;
    widgets.reserve(10);  // Set to at least 10 entry's

    if (Config.MainMenuWidgetDVBDevicesShow)
        widgets.emplace_back(std::make_pair(Config.MainMenuWidgetDVBDevicesPosition, "dvb_devices"));
    if (Config.MainMenuWidgetActiveTimerShow)
        widgets.emplace_back(std::make_pair(Config.MainMenuWidgetActiveTimerPosition, "active_timer"));
    if (Config.MainMenuWidgetLastRecShow)
        widgets.emplace_back(std::make_pair(Config.MainMenuWidgetDVBDevicesPosition, "last_recordings"));
    if (Config.MainMenuWidgetSystemInfoShow)
        widgets.emplace_back(std::make_pair(Config.MainMenuWidgetSystemInfoPosition, "system_information"));
    if (Config.MainMenuWidgetSystemUpdatesShow)
        widgets.emplace_back(std::make_pair(Config.MainMenuWidgetSystemUpdatesPosition, "system_updates"));
    if (Config.MainMenuWidgetTemperaturesShow)
        widgets.emplace_back(std::make_pair(Config.MainMenuWidgetTemperaturesPosition, "temperatures"));
    if (Config.MainMenuWidgetTimerConflictsShow)
        widgets.emplace_back(std::make_pair(Config.MainMenuWidgetTimerConflictsPosition, "timer_conflicts"));
    if (Config.MainMenuWidgetCommandShow)
        widgets.emplace_back(std::make_pair(Config.MainMenuWidgetCommandPosition, "custom_command"));
    if (Config.MainMenuWidgetWeatherShow)
        widgets.emplace_back(std::make_pair(Config.MainMenuWidgetWeatherPosition, "weather"));

    std::sort(widgets.begin(), widgets.end(), PairCompareIntString);
    std::pair<int, std::string> PairWidget {};
    std::string widget {""};
    widget.reserve(19);  // Size of 'system_information'
    int AddHeight {0};
    int ContentTop {0};
    while (!widgets.empty()) {
        PairWidget = widgets.back();
        widgets.pop_back();
        widget = PairWidget.second;

        if (widget.compare("dvb_devices") == 0) {
            AddHeight = DrawMainMenuWidgetDVBDevices(wLeft, wWidth, ContentTop);
            if (AddHeight > 0)
                ContentTop = AddHeight + m_MarginItem;
        } else if (widget.compare("active_timer") == 0) {
            AddHeight = DrawMainMenuWidgetActiveTimers(wLeft, wWidth, ContentTop);
            if (AddHeight > 0)
                ContentTop = AddHeight + m_MarginItem;
        } else if (widget.compare("last_recordings") == 0) {
            AddHeight = DrawMainMenuWidgetLastRecordings(wLeft, wWidth, ContentTop);
            if (AddHeight > 0)
                ContentTop = AddHeight + m_MarginItem;
        } else if (widget.compare("system_information") == 0) {
            AddHeight = DrawMainMenuWidgetSystemInformation(wLeft, wWidth, ContentTop);
            if (AddHeight > 0)
                ContentTop = AddHeight + m_MarginItem;
        } else if (widget.compare("system_updates") == 0) {
            AddHeight = DrawMainMenuWidgetSystemUpdates(wLeft, wWidth, ContentTop);
            if (AddHeight > 0)
                ContentTop = AddHeight + m_MarginItem;
        } else if (widget.compare("temperatures") == 0) {
            AddHeight = DrawMainMenuWidgetTemperatures(wLeft, wWidth, ContentTop);
            if (AddHeight > 0)
                ContentTop = AddHeight + m_MarginItem;
        } else if (widget.compare("timer_conflicts") == 0) {
            AddHeight = DrawMainMenuWidgetTimerConflicts(wLeft, wWidth, ContentTop);
            if (AddHeight > 0)
                ContentTop = AddHeight + m_MarginItem;
        } else if (widget.compare("custom_command") == 0) {
            AddHeight = DrawMainMenuWidgetCommand(wLeft, wWidth, ContentTop);
            if (AddHeight > 0)
                ContentTop = AddHeight + m_MarginItem;
        } else if (widget.compare("weather") == 0) {
            AddHeight = DrawMainMenuWidgetWeather(wLeft, wWidth, ContentTop);
            if (AddHeight > 0)
                ContentTop = AddHeight + m_MarginItem;
        }
    }
    ContentWidget.CreatePixmaps(false);
    ContentWidget.Draw();

    const sDecorBorder ib {wLeft,
                           wTop,
                           wWidth,
                           ContentWidget.ContentHeight(false),
                           Config.decorBorderMenuContentSize,
                           Config.decorBorderMenuContentType,
                           Config.decorBorderMenuContentFg,
                           Config.decorBorderMenuContentBg,
                           BorderMMWidget};
    DecorBorderDraw(ib);
}

int cFlatDisplayMenu::DrawMainMenuWidgetDVBDevices(int wLeft, int wWidth, int ContentTop) {
    const int MenuPixmapViewPortHeight {MenuPixmap->ViewPort().Height()};
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;

    cImage *img {ImgLoader.LoadIcon("widgets/dvb_devices", m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("DVB Devices"), false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    // Check device which currently displays live tv
    int DeviceLiveTV {-1};
    cDevice *PrimaryDevice = cDevice::PrimaryDevice();
    if (PrimaryDevice) {
        if (!PrimaryDevice->Replaying() || PrimaryDevice->Transferring())
            DeviceLiveTV = cDevice::ActualDevice()->DeviceNumber();
        else
            DeviceLiveTV = PrimaryDevice->DeviceNumber();
    }

    // Check currently recording devices
    const int NumDevices {cDevice::NumDevices()};
    bool RecDevices[NumDevices] {false};  // Array initialised to false

    LOCK_TIMERS_READ;  // Creates local const cTimers *Timers
    for (const cTimer *Timer = Timers->First(); Timer; Timer = Timers->Next(Timer)) {
        if (Timer->HasFlags(tfRecording)) {
            if (cRecordControl *RecordControl = cRecordControls::GetRecordControl(Timer)) {
                const cDevice *RecDevice = RecordControl->Device();
                if (RecDevice) RecDevices[RecDevice->DeviceNumber()] = true;
            }
        }
    }  // for cTimer

    int ActualNumDevices {0};
    cString ChannelName {""}, str {""}, StrDevice {""};
    for (int i {0}; i < NumDevices; ++i) {
        if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
            continue;

        const cDevice *device = cDevice::GetDevice(i);
        if (!device || !device->NumProvidedSystems())
            continue;

        ++ActualNumDevices;
        StrDevice = "";  // Reset string

        const cChannel *channel = device->GetCurrentlyTunedTransponder();
        if (i == DeviceLiveTV) {
            StrDevice.Append(cString::sprintf("%s (", tr("LiveTV")));
            if (channel && channel->Number() > 0)
                ChannelName = channel->Name();
            else
                ChannelName = tr("Unknown");
            StrDevice.Append(cString::sprintf("%s)", *ChannelName));
        } else if (RecDevices[i]) {
            StrDevice.Append(cString::sprintf("%s (", tr("recording")));
            if (channel && channel->Number() > 0)
                ChannelName = channel->Name();
            else
                ChannelName = tr("Unknown");
            StrDevice.Append(cString::sprintf("%s)", *ChannelName));
        } else {
            if (channel) {
                ChannelName = channel->Name();
                if (!strcmp(*ChannelName, "")) {
                    if (Config.MainMenuWidgetDVBDevicesDiscardNotUsed)
                        continue;
                    StrDevice.Append(tr("not used"));
                } else {
                    if (Config.MainMenuWidgetDVBDevicesDiscardUnknown)
                        continue;
                    StrDevice.Append(cString::sprintf("%s (%s)", tr("Unknown"), *ChannelName));
                }
            } else {
                if (Config.MainMenuWidgetDVBDevicesDiscardNotUsed)
                    continue;
                StrDevice.Append(tr("not used"));
            }
        }

        if (Config.MainMenuWidgetDVBDevicesNativeNumbering)
            str = cString::sprintf("%d", i);  // Display Tuners 0..3
        else
            str = cString::sprintf("%d", i + 1);  // Display Tuners 1..4

        int left {m_MarginItem};
        const int FontSmlWidthX {m_FontSml->Width('X')};  // Width of one X
        if (NumDevices <= 9) {
            ContentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);

            left += FontSmlWidthX * 2;
        } else {
            ContentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  FontSmlWidthX * 2, m_FontSmlHeight, taRight);

            left += FontSmlWidthX * 3;
        }
        str = *(device->DeviceType());
        ContentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              FontSmlWidthX * 7, m_FontSmlHeight, taLeft);

        left += FontSmlWidthX * 8;
        ContentWidget.AddText(*StrDevice, false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);

        ContentTop += m_FontSmlHeight;
    }  // for NumDevices

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetActiveTimers(int wLeft, int wWidth, int ContentTop) {
    const int MenuPixmapViewPortHeight {MenuPixmap->ViewPort().Height()};
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;

    cImage *img {ImgLoader.LoadIcon("widgets/active_timers", m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("Timer"), false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    // Look for timers
    cVector<const cTimer *> TimerRec;
    cVector<const cTimer *> TimerActive;
    cVector<const cTimer *> TimerRemoteRec;
    cVector<const cTimer *> TimerRemoteActive;

    int AllTimers {0};  // All timers and remote timers

    LOCK_TIMERS_READ;  // Creates local const cTimers *Timers
    for (const cTimer *Timer = Timers->First(); Timer; Timer = Timers->Next(Timer)) {
        if (Timer->HasFlags(tfActive) && !Timer->HasFlags(tfRecording)) {
            if (Timer->Remote()) {
                if (Config.MainMenuWidgetActiveTimerShowRemoteActive) {
                    TimerRemoteActive.Append(Timer);
                    AllTimers += 1;
                }
            } else {  // Local
                if (Config.MainMenuWidgetActiveTimerShowActive) {
                    TimerActive.Append(Timer);
                    AllTimers += 1;
                }
            }
        }

        if (Timer->HasFlags(tfRecording)) {
            if (Timer->Remote()) {
                if (Config.MainMenuWidgetActiveTimerShowRemoteRecording) {
                    TimerRemoteRec.Append(Timer);
                    AllTimers += 1;
                }
            } else {  // Local
                if (Config.MainMenuWidgetActiveTimerShowRecording) {
                    TimerRec.Append(Timer);
                    AllTimers += 1;
                }
            }
        }

        if (AllTimers >= Config.MainMenuWidgetActiveTimerMaxCount)
            break;
    }

    if (AllTimers == 0 && Config.MainMenuWidgetActiveTimerHideEmpty)
        return 0;

    TimerRec.Sort(CompareTimers);
    TimerActive.Sort(CompareTimers);
    TimerRemoteRec.Sort(CompareTimers);
    TimerRemoteActive.Sort(CompareTimers);

    int Left {m_MarginItem};
    int Width {wWidth - m_MarginItem2};
    if (AllTimers == 0) {
        ContentWidget.AddText(tr("no active/recording timer"), false, cRect(Left, ContentTop, Width, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, Width);
    } else {
        int count {-1}, RemoteCount {-1};
        cString StrTimer {""};
        // First recording timer
        if (Config.MainMenuWidgetActiveTimerShowRecording) {
            const int TimerRecSize {TimerRec.Size()};
            for (int i {0}; i < TimerRecSize; ++i) {
                ++count;
                if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
                    break;

                if (count >= Config.MainMenuWidgetActiveTimerMaxCount)
                    break;

                StrTimer = "";  // Reset string
                if ((Config.MainMenuWidgetActiveTimerShowRemoteActive ||
                     Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
                    (TimerRemoteRec.Size() > 0 || TimerRemoteActive.Size() > 0)) {
                    img = ImgLoader.LoadIcon("widgets/home", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
                    if (img) {
                        ContentWidget.AddImage(img, cRect(Left, ContentTop, Width, m_FontSmlHeight));
                        Left += m_FontSmlHeight + m_MarginItem;
                        Width -= m_FontSmlHeight - m_MarginItem;
                    }  //? Fallback? // else StrTimer.Append("L: ");
                }

                const cChannel *Channel = (TimerRec[i])->Channel();
                // const cEvent *Event = Timer->Event();
                if (Channel)
                    StrTimer.Append(cString::sprintf("%s - ", Channel->Name()));
                else
                    StrTimer.Append(cString::sprintf("%s - ", tr("Unknown")));

                StrTimer.Append((TimerRec[i])->File());

                ContentWidget.AddText(*StrTimer, false, cRect(Left, ContentTop, Width, m_FontSmlHeight),
                                      Theme.Color(clrTopBarRecordingActiveFg), Theme.Color(clrMenuEventBg), m_FontSml,
                                      Width);

                ContentTop += m_FontSmlHeight;
                Left = m_MarginItem;
                Width = wWidth - m_MarginItem2;
            }
        }  // Config.MainMenuWidgetActiveTimerShowRecording

        if (Config.MainMenuWidgetActiveTimerShowActive) {
            const int TimerActiveSize {TimerActive.Size()};
            for (int i {0}; i < TimerActiveSize; ++i) {
                ++count;
                if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
                    break;

                if (count >= Config.MainMenuWidgetActiveTimerMaxCount)
                    break;

                StrTimer = "";  // Reset string
                if ((Config.MainMenuWidgetActiveTimerShowRemoteActive ||
                     Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
                    (TimerRemoteRec.Size() > 0 || TimerRemoteActive.Size() > 0)) {
                    img = ImgLoader.LoadIcon("widgets/home", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
                    if (img) {
                        ContentWidget.AddImage(img, cRect(Left, ContentTop, Width, m_FontSmlHeight));
                        Left += m_FontSmlHeight + m_MarginItem;
                        Width -= m_FontSmlHeight - m_MarginItem;
                    }  // StrTimer.Append("L: ");
                }

                const cChannel *Channel = (TimerActive[i])->Channel();
                // const cEvent *Event = Timer->Event();
                if (Channel)
                    StrTimer.Append(cString::sprintf("%s - ", Channel->Name()));
                else
                    StrTimer.Append(cString::sprintf("%s - ", tr("Unknown")));
                StrTimer.Append((TimerActive[i])->File());

                ContentWidget.AddText(*StrTimer, false, cRect(Left, ContentTop, Width, m_FontSmlHeight),
                                      Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, Width);

                ContentTop += m_FontSmlHeight;
                Left = m_MarginItem;
                Width = wWidth - m_MarginItem2;
            }
        }  // Config.MainMenuWidgetActiveTimerShowActive

        if (Config.MainMenuWidgetActiveTimerShowRemoteRecording) {
            const int TimerRemoteRecSize {TimerRemoteRec.Size()};
            for (int i {0}; i < TimerRemoteRecSize; ++i) {
                ++RemoteCount;
                if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
                    break;

                if (count + RemoteCount >= Config.MainMenuWidgetActiveTimerMaxCount)
                    break;

                StrTimer = "";  // Reset string
                img = ImgLoader.LoadIcon("widgets/remotetimer", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
                if (img) {
                    ContentWidget.AddImage(img, cRect(Left, ContentTop, Width, m_FontSmlHeight));
                    Left += m_FontSmlHeight + m_MarginItem;
                    Width -= m_FontSmlHeight - m_MarginItem;
                }  // StrTimer = cString::sprintf("R: ");

                const cChannel *Channel = (TimerRemoteRec[i])->Channel();
                // const cEvent *Event = Timer->Event();
                if (Channel)
                    StrTimer.Append(cString::sprintf("%s - ", Channel->Name()));
                else
                    StrTimer.Append(cString::sprintf("%s - ", tr("Unknown")));

                StrTimer.Append((TimerRemoteRec[i])->File());

                ContentWidget.AddText(*StrTimer, false, cRect(Left, ContentTop, Width, m_FontSmlHeight),
                                      Theme.Color(clrTopBarRecordingActiveFg), Theme.Color(clrMenuEventBg), m_FontSml,
                                      Width);

                ContentTop += m_FontSmlHeight;
                Left = m_MarginItem;
                Width = wWidth - m_MarginItem2;
            }
        }  // Config.MainMenuWidgetActiveTimerShowRemoteRecording

        if (Config.MainMenuWidgetActiveTimerShowRemoteActive) {
            const int TimerRemoteActiveSize {TimerRemoteActive.Size()};
            for (int i {0}; i < TimerRemoteActiveSize; ++i) {
                ++RemoteCount;
                if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
                    break;

                if (count + RemoteCount >= Config.MainMenuWidgetActiveTimerMaxCount)
                    break;

                StrTimer = "";  // Reset string
                img = ImgLoader.LoadIcon("widgets/remotetimer", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
                if (img) {
                    ContentWidget.AddImage(img, cRect(Left, ContentTop, Width, m_FontSmlHeight));
                    Left += m_FontSmlHeight + m_MarginItem;
                    Width -= m_FontSmlHeight - m_MarginItem;
                }  // StrTimer = cString::sprintf("R: ");

                const cChannel *Channel = (TimerRemoteActive[i])->Channel();
                // const cEvent *Event = Timer->Event();
                if (Channel)
                    StrTimer.Append(cString::sprintf("%s - ", Channel->Name()));
                else
                    StrTimer.Append(cString::sprintf("%s - ", tr("Unknown")));

                StrTimer.Append((TimerRemoteActive[i])->File());

                ContentWidget.AddText(*StrTimer, false, cRect(Left, ContentTop, Width, m_FontSmlHeight),
                                      Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, Width);

                ContentTop += m_FontSmlHeight;
                Left = m_MarginItem;
                Width = wWidth - m_MarginItem2;
            }
        }  // Config.MainMenuWidgetActiveTimerShowRemoteActive
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetLastRecordings(int wLeft, int wWidth, int ContentTop) {
    const int MenuPixmapViewPortHeight {MenuPixmap->ViewPort().Height()};
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;

    cImage *img {ImgLoader.LoadIcon("widgets/last_recordings", m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("Last Recordings"), false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    std::vector<std::pair<time_t, cString>> Recs;
    Recs.reserve(512);  // Set to at least 512 entry's
    time_t RecStart {0};
    cString DateTime {""}, Length {""}, StrRec {""};
    div_t TimeHM {0, 0};
    LOCK_RECORDINGS_READ;
    for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
        RecStart = Rec->Start();
        TimeHM = std::div((Rec->LengthInSeconds() + 30) / 60, 60);
        Length = cString::sprintf("%02d:%02d", TimeHM.quot, TimeHM.rem);
        DateTime = cString::sprintf("%s  %s  %s", *ShortDateString(RecStart), *TimeString(RecStart), *Length);

        StrRec = cString::sprintf("%s - %s", *DateTime, Rec->Name());
        Recs.emplace_back(std::make_pair(RecStart, StrRec));
    }

    // Sort by RecStart
    std::sort(Recs.begin(), Recs.end(), PairCompareTimeString);
    std::pair<time_t, cString> PairRec {};
    int index {0};
    cString Rec {""};
    while (!Recs.empty() && index < Config.MainMenuWidgetLastRecMaxCount) {
        if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
            continue;

        PairRec = Recs.back();
        Recs.pop_back();
        Rec = PairRec.second;

        ContentWidget.AddText(*Rec, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
        ContentTop += m_FontSmlHeight;
        ++index;
    }
    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetTimerConflicts(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img {ImgLoader.LoadIcon("widgets/timer_conflicts", m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("Timer Conflicts"), false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    const int NumConflicts {GetEpgsearchConflicts()};  // Get conflicts from plugin Epgsearch
    if (NumConflicts == 0 && Config.MainMenuWidgetTimerConflictsHideEmpty)
        return 0;

    if (NumConflicts == 0) {
        ContentWidget.AddText(tr("no timer conflicts"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
    } else {
        cString str = cString::sprintf("%s: %d", tr("timer conflicts"), NumConflicts);
        ContentWidget.AddText(*str, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetSystemInformation(int wLeft, int wWidth, int ContentTop) {
    const int MenuPixmapViewPortHeight {MenuPixmap->ViewPort().Height()};
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;

    cImage *img {ImgLoader.LoadIcon("widgets/system_information", m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("System Information"), false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    const cString ExecFile = cString::sprintf("\"%s/system_information/system_information\"", WIDGETFOLDER);
    [[maybe_unused]] int r {system(*ExecFile)};  // Prevent warning for unused variable

    const cString ConfigsPath = cString::sprintf("%s/system_information/", WIDGETOUTPUTPATH);
    std::vector<std::string> files;
    files.reserve(32);  // Set to at least 32 entry's

    cReadDir d(*ConfigsPath);
    struct dirent *e;
    std::string FileName {""}, num {""};
    FileName.reserve(128);
    num.reserve(16);
    std::size_t found {0};
    while ((e = d.Next()) != nullptr) {
        FileName = e->d_name;
        found = FileName.find('_');
        if (found != std::string::npos) {  // File name contains '_'
            num = FileName.substr(0, found);
            if (atoi(num.c_str()) > 0)  // Number is greater than zero
                files.emplace_back(FileName);
        }
    }
    std::sort(files.begin(), files.end(), StringCompare);

    cString str {""};
    if (files.size() == 0) {
        str = cString::sprintf("%s - %s", tr("no information available please check the script"), *ExecFile);
        ContentWidget.AddText(*str, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
    } else {
        std::string item {""}, ItemContent {""}, ItemFilename {""};
        item.reserve(17);
        ItemContent.reserve(16);
        ItemFilename.reserve(128);
        int Column {1};
        int ContentLeft {m_MarginItem};

        // Items are stored in an array, in which case they need to be marked differently
        // Using the trNOOP() macro, the actual translation is then done when such a text is used
        // https://github.com/vdr-projects/vdr/wiki/The-VDR-Plugin-System#internationalization
        const struct ItemData {
            const char *key;
            const char *label;
        } items[] {
            {"sys_version", trNOOP("System Version")},
            {"kernel_version", trNOOP("Kernel Version")},
            {"uptime", trNOOP("Uptime")},
            {"load", trNOOP("Load")},
            {"processes", trNOOP("Processes")},
            {"mem_usage", trNOOP("Memory Usage")},
            {"swap_usage", trNOOP("Swap Usage")},
            {"root_usage", trNOOP("Root Usage")},
            {"video_usage", trNOOP("Video Usage")},
            {"vdr_cpu_usage", trNOOP("VDR CPU Usage")},
            {"vdr_mem_usage", trNOOP("VDR MEM Usage")},
            {"cpu", trNOOP("Temp CPU")},
            {"gpu", trNOOP("Temp GPU")},
            {"pccase", trNOOP("Temp PC-Case")},
            {"motherboard", trNOOP("Temp MB")},
            {"updates", trNOOP("Updates")},
            {"security_updates", trNOOP("Security Updates")}
        };

        for (const auto &FileName : files) {
            if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight) break;

            found = FileName.find('_');
            item = FileName.substr(found + 1);
            ItemFilename = cString::sprintf("%s/system_information/%s", WIDGETOUTPUTPATH, FileName.c_str());

            std::ifstream file(ItemFilename.c_str());
            if (!file.is_open()) continue;

            std::getline(file, ItemContent);

            for (const auto &data : items) {
                if (item.compare(data.key) == 0) {
                    str = cString::sprintf("%s: %s", tr(data.label), ItemContent.c_str());
                    ContentWidget.AddText(*str, false,
                                          cRect(ContentLeft, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                                          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                          wWidth - m_MarginItem2);
                    // Items 'sys_version' and 'kernel_version' are printed on one line
                    if (Column == 1 && !(item.compare(items[0].key) == 0 || item.compare(items[1].key) == 0)) {
                        Column = 2;
                        ContentLeft = wWidth / 2;
                    } else {
                        Column = 1;
                        ContentLeft = m_MarginItem;
                        ContentTop += m_FontSmlHeight;
                    }
                    break;
                }
            }
            file.close();
        }
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetSystemUpdates(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img {ImgLoader.LoadIcon("widgets/system_updates", m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("System Updates"), false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    cString Content = *ReadAndExtractData(cString::sprintf("%s/system_updatestatus/updates", WIDGETOUTPUTPATH));
    const int updates = (isempty(*Content) ? -1 : atoi(*Content));

    Content = *ReadAndExtractData(cString::sprintf("%s/system_updatestatus/security_updates", WIDGETOUTPUTPATH));
    const int SecurityUpdates = (isempty(*Content) ? -1 : atoi(*Content));

    if (updates == -1 || SecurityUpdates == -1) {
        ContentWidget.AddText(tr("Updatestatus not available please check the widget"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
    } else if (updates == 0 && SecurityUpdates == 0 && Config.MainMenuWidgetSystemUpdatesHideIfZero) {
        return 0;
    } else {
        cString str = cString::sprintf("%s: %d", tr("Updates"), updates);
        ContentWidget.AddText(*str, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
        str = cString::sprintf("%s: %d", tr("Security Updates"), SecurityUpdates);
        ContentWidget.AddText(
            *str, false, cRect(wWidth / 2 + m_MarginItem, ContentTop, wWidth / 2 - m_MarginItem2, m_FontSmlHeight),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem2);
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetTemperatures(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img {ImgLoader.LoadIcon("widgets/temperatures", m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("Temperatures"), false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    const cString ExecFile =
        cString::sprintf("cd \"%s/temperatures\"; \"%s/temperatures/temperatures\"", WIDGETFOLDER, WIDGETFOLDER);
    [[maybe_unused]] int r {system(*ExecFile)};  // Prevent warning for unused variable

    int CountTemps {0};

    const cString TempCPU = *ReadAndExtractData(cString::sprintf("%s/temperatures/cpu", WIDGETOUTPUTPATH));
    if (!isempty(*TempCPU)) ++CountTemps;

    const cString TempCase = *ReadAndExtractData(cString::sprintf("%s/temperatures/pccase", WIDGETOUTPUTPATH));
    if (!isempty(*TempCase)) ++CountTemps;

    const cString TempMB = *ReadAndExtractData(cString::sprintf("%s/temperatures/motherboard", WIDGETOUTPUTPATH));
    if (!isempty(*TempMB)) ++CountTemps;

    const cString TempGPU = *ReadAndExtractData(cString::sprintf("%s/temperatures/gpu", WIDGETOUTPUTPATH));
    if (!isempty(*TempGPU)) ++CountTemps;

    if (CountTemps == 0) {
        ContentWidget.AddText(tr("Temperatures not available please check the widget"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
    } else {
        const int AddLeft {wWidth / CountTemps};
        int Left {m_MarginItem};
        cString str {""};
        if (!isempty(*TempCPU)) {
            str = cString::sprintf("%s: %s", tr("CPU"), *TempCPU);
            ContentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  wWidth - m_MarginItem2);
            Left += AddLeft;
        }
        if (!isempty(*TempCase)) {
            str = cString::sprintf("%s: %s", tr("PC-Case"), *TempCase);
            ContentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth / 3 - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  wWidth - m_MarginItem2);
            Left += AddLeft;
        }
        if (!isempty(*TempMB)) {
            str = cString::sprintf("%s: %s", tr("MB"), *TempMB);
            ContentWidget.AddText(
                *str, false, cRect(Left, ContentTop, wWidth / 3 * 2 - m_MarginItem2, m_FontSmlHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem2);
            Left += AddLeft;
        }
        if (!isempty(*TempGPU)) {
            str = cString::sprintf("%s: %s", tr("GPU"), *TempGPU);
            ContentWidget.AddText(
                *str, false, cRect(Left, ContentTop, wWidth / 3 * 2 - m_MarginItem2, m_FontSmlHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem2);
        }
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetCommand(int wLeft, int wWidth, int ContentTop) {
    const int MenuPixmapViewPortHeight {MenuPixmap->ViewPort().Height()};
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;

    const cString ExecFile = cString::sprintf("\"%s/command_output/command\"", WIDGETFOLDER);
    [[maybe_unused]] int r {system(*ExecFile)};  // Prevent warning for unused variable

    cString Title = *ReadAndExtractData(cString::sprintf("%s/command_output/title", WIDGETOUTPUTPATH));
    if (isempty(*Title)) Title = tr("no title available");

    cImage *img {ImgLoader.LoadIcon("widgets/command_output", m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(*Title, false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    std::string Output {""};
    Output.reserve(32);
    const cString ItemFilename = cString::sprintf("%s/command_output/output", WIDGETOUTPUTPATH);
    std::ifstream file(*ItemFilename);
    if (file.is_open()) {
        for (; std::getline(file, Output);) {
            if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
                break;
            ContentWidget.AddText(
                Output.c_str(), false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem2);
            ContentTop += m_FontSmlHeight;
        }
        file.close();
    } else {
        ContentWidget.AddText(tr("no output available"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetWeather(int wLeft, int wWidth, int ContentTop) {
    const int MenuPixmapViewPortHeight {MenuPixmap->ViewPort().Height()};
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;

    // Read location
    cString Location = *ReadAndExtractData(cString::sprintf("%s/weather/weather.location", WIDGETOUTPUTPATH));
    if (isempty(*Location))
        Location = tr("Unknown");

    // Read temperature
    const cString TempToday = *ReadAndExtractData(cString::sprintf("%s/weather/weather.0.temp", WIDGETOUTPUTPATH));

    //* Declared in 'baserender.h'
    // Deleted in '~cFlatDisplayMenu', because of segfault when deleted here or in 'DrawMainMenuWidgets'
    m_FontTempSml = cFont::CreateFont(Setup.FontOsd, Setup.FontOsdSize * (1.0 / 2.0));
    const int FontTempSmlHeight {m_FontTempSml->Height()};

    cImage *img {ImgLoader.LoadIcon("widgets/weather", m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    const cString Title = cString::sprintf("%s - %s %s", tr("Weather"), *Location, *TempToday);
    ContentWidget.AddText(*Title, false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    int left {m_MarginItem};
    cString Icon {""}, Summary {""}, TempMax {""}, TempMin {""};
    cString DayName {""}, PrecString(""), StrWeekDayName {""}, WeatherIcon {""};
    time_t t {time(0)};
    time_t t2 {t};
    struct tm tm_r;
    localtime_r(&t, &tm_r);
    for (int index {0}; index < Config.MainMenuWidgetWeatherDays; ++index) {
        // Read icon
        Icon = *ReadAndExtractData(cString::sprintf("%s/weather/weather.%d.icon", WIDGETOUTPUTPATH, index));

        // Read summary
        Summary = *ReadAndExtractData(cString::sprintf("%s/weather/weather.%d.summary", WIDGETOUTPUTPATH, index));

        // Read max temperature
        TempMax = *ReadAndExtractData(cString::sprintf("%s/weather/weather.%d.tempMax", WIDGETOUTPUTPATH, index));

        // Read min temperature
        TempMin = *ReadAndExtractData(cString::sprintf("%s/weather/weather.%d.tempMin", WIDGETOUTPUTPATH, index));

        // Read precipitation
        PrecString =
            *FormatPrecipitation(cString::sprintf("%s/weather/weather.%d.precipitation", WIDGETOUTPUTPATH, index));

        if (isempty(*Icon) || isempty(*Summary) || isempty(*TempMax) || isempty(*TempMin) || (isempty(*PrecString)))
            continue;

        tm_r.tm_mday += index;
        /* time_t */ t2 = mktime(&tm_r);

        if (Config.MainMenuWidgetWeatherType == 0) {  // Short
            if (left + m_FontHeight2 + m_FontTempSml->Width("-99,9°C") + m_FontTempSml->Width("XXXX") +
                    m_MarginItem * 6 >
                wWidth)
                break;
            if (index > 0) {
                ContentWidget.AddText("|", false, cRect(left, ContentTop, 0, 0), Theme.Color(clrMenuEventFontInfo),
                                      Theme.Color(clrMenuEventBg), m_Font);
                left += m_Font->Width('|') + m_MarginItem2;
            }

            WeatherIcon = cString::sprintf("widgets/%s", *Icon);
            img = ImgLoader.LoadIcon(*WeatherIcon, m_FontHeight, m_FontHeight - m_MarginItem2);
            if (img) {
                ContentWidget.AddImage(img, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight + m_MarginItem;
            }
            const int wtemp {std::max(m_FontTempSml->Width(*TempMax), m_FontTempSml->Width(*TempMin))};
            ContentWidget.AddText(*TempMax, false, cRect(left, ContentTop, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml, wtemp,
                                  FontTempSmlHeight, taRight);
            ContentWidget.AddText(*TempMin, false, cRect(left, ContentTop + FontTempSmlHeight, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml, wtemp,
                                  FontTempSmlHeight, taRight);

            left += wtemp + m_MarginItem;

            img = ImgLoader.LoadIcon("widgets/umbrella", m_FontHeight, m_FontHeight - m_MarginItem2);
            if (img) {
                ContentWidget.AddImage(img, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight - m_MarginItem;
            }
            ContentWidget.AddText(*PrecString, false,
                                  cRect(left, ContentTop + (m_FontHeight / 2 - FontTempSmlHeight / 2), 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml);
            left += m_FontTempSml->Width(*PrecString) + m_MarginItem2;
        } else {  // Long
            if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
                break;

            left = m_MarginItem;
            StrWeekDayName = WeekDayName(t2);
            DayName = cString::sprintf("%s ", *StrWeekDayName);
            ContentWidget.AddText(*DayName, false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font,
                                  wWidth - m_MarginItem2);
            left += m_Font->Width("XXXX") + m_MarginItem;

            WeatherIcon = cString::sprintf("widgets/%s", *Icon);
            img = ImgLoader.LoadIcon(*WeatherIcon, m_FontHeight, m_FontHeight - m_MarginItem2);
            if (img) {
                ContentWidget.AddImage(img, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight + m_MarginItem;
            }
            ContentWidget.AddText(*TempMax, false, cRect(left, ContentTop, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml,
                                  m_FontTempSml->Width("-99,9°C"), FontTempSmlHeight, taRight);
            ContentWidget.AddText(*TempMin, false, cRect(left, ContentTop + FontTempSmlHeight, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml,
                                  m_FontTempSml->Width("-99,9°C"), FontTempSmlHeight, taRight);

            left += m_FontTempSml->Width("-99,9°C ") + m_MarginItem;

            img = ImgLoader.LoadIcon("widgets/umbrella", m_FontHeight, m_FontHeight - m_MarginItem2);
            if (img) {
                ContentWidget.AddImage(img, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight - m_MarginItem;
            }
            ContentWidget.AddText(*PrecString, false,
                                  cRect(left, ContentTop + (m_FontHeight / 2 - FontTempSmlHeight / 2), 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml,
                                  m_FontTempSml->Width("100%"), FontTempSmlHeight, taRight);
            left += m_FontTempSml->Width("100% ") + m_MarginItem;

            ContentWidget.AddText(
                *Summary, false,
                cRect(left, ContentTop + (m_FontHeight / 2 - FontTempSmlHeight / 2), wWidth - left, m_FontHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml, wWidth - left);

            ContentTop += m_FontHeight;
        }
    }  // for Config.MainMenuWidgetWeatherDays
    return ContentWidget.ContentHeight(false);
}

void cFlatDisplayMenu::PreLoadImages() {
    // Menu icons
    const int ImageHeight {m_FontHeight};
    const cString Path = cString::sprintf("%s/%s/menuIcons", *Config.IconPath, Setup.OSDTheme);
    std::string File {""};
    File.reserve(256);
    cString FileName {""};
    cReadDir d(*Path);
    struct dirent *e;
    while ((e = d.Next()) != nullptr) {
        File = e->d_name;
        if (File.find("vdrlogo") == 0)  // Skip vdrlogo* files
            continue;
        FileName = cString::sprintf("menuIcons/%s", File.substr(0, File.find_last_of(".")).c_str());
        ImgLoader.LoadIcon(*FileName, ImageHeight - m_MarginItem2, ImageHeight - m_MarginItem2);
    }

    if (Config.TopBarMenuIconShow)
        ImgLoader.LoadIcon(cString::sprintf("menuIcons/%s", VDRLOGO), ICON_WIDTH_UNLIMITED,
                           m_TopBarHeight - m_MarginItem2);

    ImgLoader.LoadIcon("menuIcons/blank", ImageHeight - m_MarginItem2, ImageHeight - m_MarginItem2);

    int ImageBgHeight {ImageHeight};
    int ImageBgWidth = ImageHeight * 1.34f;  // Narrowing conversion
    cImage *img {ImgLoader.LoadIcon("logo_background", ImageBgWidth, ImageBgHeight)};
    if (img) {
        ImageBgHeight = img->Height();
        ImageBgWidth = img->Width();
    }

    ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
    ImgLoader.LoadIcon("changroup", ImageBgWidth - 10, ImageBgHeight - 10);
    ImgLoader.LoadIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);
    ImgLoader.LoadIcon("timerInactive", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("timerRecording", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("timerActive", ImageHeight, ImageHeight);

    //* Same as in 'displaychannel.c' 'PreLoadImages()', but different logo size!
    int index {0};
    LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
    for (const cChannel *Channel = Channels->First(); Channel && index < LogoPreCache;
        Channel = Channels->Next(Channel)) {
        if (!Channel->GroupSep()) {  // Don't cache named channel group logo
            img = ImgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
            if (img)
                ++index;
        }
    }  // for channel

    ImgLoader.LoadIcon("radio", ICON_WIDTH_UNLIMITED, m_TopBarHeight - m_MarginItem2);
    ImgLoader.LoadIcon("changroup", ICON_WIDTH_UNLIMITED, m_TopBarHeight - m_MarginItem2);
    ImgLoader.LoadIcon("tv", ICON_WIDTH_UNLIMITED, m_TopBarHeight - m_MarginItem2);

    ImgLoader.LoadIcon("timer_full", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("timer_full_cur", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("timer_partial", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("vps", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("vps_cur", ImageHeight, ImageHeight);

    ImgLoader.LoadIcon("sd", ImageHeight, ImageHeight * (1.0 / 3.0));
    ImgLoader.LoadIcon("hd", ImageHeight, ImageHeight * (1.0 / 3.0));
    ImgLoader.LoadIcon("uhd", ImageHeight, ImageHeight * (1.0 / 3.0));

    ImgLoader.LoadIcon("folder", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("recording", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("recording_cutted", ImageHeight, ImageHeight * (2.0 / 3.0));
    ImgLoader.LoadIcon("recording_cutted_cur", ImageHeight, ImageHeight * (2.0 / 3.0));
    ImgLoader.LoadIcon("recording_new", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("recording_new_cur", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("recording_old", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("recording_old_cur", ImageHeight, ImageHeight);

    ImgLoader.LoadIcon("recording_new", m_FontSmlHeight, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_new_cur", m_FontSmlHeight, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_old", m_FontSmlHeight, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_old_cur", m_FontSmlHeight, m_FontSmlHeight);

    if (Config.MainMenuWidgetDVBDevicesShow) {
        ImgLoader.LoadIcon("widgets/dvb_devices", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetActiveTimerShow) {
        ImgLoader.LoadIcon("widgets/active_timers", ImageHeight, ImageHeight - m_MarginItem2);
        ImgLoader.LoadIcon("widgets/home", m_FontSmlHeight, m_FontSmlHeight);
        ImgLoader.LoadIcon("widgets/remotetimer", m_FontSmlHeight, m_FontSmlHeight);
    }
    if (Config.MainMenuWidgetLastRecShow) {
        ImgLoader.LoadIcon("widgets/last_recordings", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetTimerConflictsShow) {
        ImgLoader.LoadIcon("widgets/timer_conflicts", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetSystemInfoShow) {
        ImgLoader.LoadIcon("widgets/system_information", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetSystemUpdatesShow) {
        ImgLoader.LoadIcon("widgets/system_updates", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetTemperaturesShow) {
        ImgLoader.LoadIcon("widgets/temperatures", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetCommandShow) {
        ImgLoader.LoadIcon("widgets/command_output", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetWeatherShow) {
        ImgLoader.LoadIcon("widgets/weather", ImageHeight, ImageHeight - m_MarginItem2);
        ImgLoader.LoadIcon("widgets/umbrella", ImageHeight, ImageHeight - m_MarginItem2);
    }
}
