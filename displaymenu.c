/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displaymenu.h"

#include <vdr/device.h>
#include <vdr/epg.h>
#include <vdr/i18n.h>
#include <vdr/menu.h>
#include <vdr/plugin.h>
#include <vdr/recorder.h>
#include <vdr/recording.h>
#include <vdr/timers.h>
#include <vdr/videodir.h>

#include <dirent.h>
#include <fstream>
#include <functional>  // std::less
#include <locale>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "./services/epgsearch.h"
#include "./services/scraper2vdr.h"

#include "./flat.h"
#include "./fontcache.h"

#ifndef VDRLOGO
#define VDRLOGO "vdrlogo_default"
#endif

static int CompareTimers(const void *a, const void *b) { return (*(const cTimer **)a)->Compare(**(const cTimer **)b); }

cFlatDisplayMenu::cFlatDisplayMenu() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::cFlatDisplayMenu()");
    cTimeMs Timer;  // Set Timer
#endif

    CreateFullOsd();
    TopBarCreate();
    ButtonsCreate();
    MessageCreate();

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
    const cRect MenuPixmapViewPort {0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight};
    MenuPixmap = CreatePixmap(m_Osd, "MenuPixmap", 1, MenuPixmapViewPort);
    MenuIconsBgPixmap = CreatePixmap(m_Osd, "MenuIconsBgPixmap", 2, MenuPixmapViewPort);
    MenuIconsPixmap = CreatePixmap(m_Osd, "MenuIconsPixmap", 3, MenuPixmapViewPort);
    MenuIconsOvlPixmap = CreatePixmap(m_Osd, "MenuIconsOvlPixmap", 4, MenuPixmapViewPort);

    m_chLeft = Config.decorBorderMenuContentHeadSize;
    m_chTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentHeadSize;
    m_chWidth = m_MenuWidth - Config.decorBorderMenuContentHeadSize * 2;
    m_chHeight = m_FontHeight + m_FontSmlHeight * 2 + m_MarginItem2;
    const cRect ContentHeadPixmapViewPort {m_chLeft, m_chTop, m_chWidth, m_chHeight};
    ContentHeadPixmap = CreatePixmap(m_Osd, "ContentHeadPixmap", 1, ContentHeadPixmapViewPort);
    ContentHeadIconsPixmap = CreatePixmap(m_Osd, "ContentHeadIconsPixmap", 2, ContentHeadPixmapViewPort);

    ScrollbarPixmap = CreatePixmap(
        m_Osd, "ScrollbarPixmap", 2,
        cRect(0, m_ScrollBarTop, m_MenuWidth, m_ScrollBarHeight + m_ButtonsHeight + Config.decorBorderButtonSize * 2));

    PixmapClear(MenuPixmap);
    PixmapClear(MenuIconsPixmap);
    PixmapClear(MenuIconsBgPixmap);
    PixmapClear(MenuIconsOvlPixmap);
    PixmapClear(ContentHeadPixmap);
    PixmapClear(ContentHeadIconsPixmap);
    PixmapClear(ScrollbarPixmap);

    MenuItemScroller.SetOsd(m_Osd);
    MenuItemScroller.SetScrollStep(Config.ScrollerStep);
    MenuItemScroller.SetScrollDelay(Config.ScrollerDelay);
    MenuItemScroller.SetScrollType(Config.ScrollerType);

    static constexpr std::size_t kDefaultItemBorderSize {64};
    ItemsBorder.reserve(kDefaultItemBorderSize);
#ifdef DEBUGFUNCSCALL
    if (Timer.Elapsed() > 0) dsyslog("   Done in %ld ms", Timer.Elapsed());
#endif
}

cFlatDisplayMenu::~cFlatDisplayMenu() {
    MenuItemScroller.Clear();
    // if (m_Osd) {
    m_Osd->DestroyPixmap(MenuPixmap);
    m_Osd->DestroyPixmap(MenuIconsPixmap);
    m_Osd->DestroyPixmap(MenuIconsBgPixmap);
    m_Osd->DestroyPixmap(MenuIconsOvlPixmap);
    m_Osd->DestroyPixmap(ContentHeadPixmap);
    m_Osd->DestroyPixmap(ContentHeadIconsPixmap);
    m_Osd->DestroyPixmap(ScrollbarPixmap);
    // }
}

void cFlatDisplayMenu::SetMenuCategory(eMenuCategory MenuCategory) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetMenuCategory(%d)", MenuCategory);
#endif

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
        if (Config.MainMenuWidgetsEnable) DrawMainMenuWidgets();
        break;
    default: break;
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
    dsyslog("   Total: %d, Offset: %d, Shown: %d, Top: %d, Height: %d", Total, Offset, Shown, Top, Height);
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
            DrawScrollbar(ComplexContent.ScrollTotal(), ScrollOffset, ComplexContent.ScrollShown(),
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
    case mcChannel: ItemHeight = m_ItemChannelHeight; break;
    case mcTimer: ItemHeight = m_ItemTimerHeight; break;
    case mcSchedule:
    case mcScheduleNow:
    case mcScheduleNext: ItemHeight = m_ItemEventHeight; break;
    case mcRecording: ItemHeight = m_ItemRecordingHeight; break;
    default: break;
    }
    return m_ScrollBarHeight / ItemHeight;  // Truncation is wanted here to get only full items displayed
}

int cFlatDisplayMenu::ItemsHeight() {
    switch (m_MenuCategory) {
    case mcChannel: return MaxItems() * m_ItemChannelHeight - Config.MenuItemPadding;
    case mcTimer: return MaxItems() * m_ItemTimerHeight - Config.MenuItemPadding;
    case mcSchedule:
    case mcScheduleNow:
    case mcScheduleNext: return MaxItems() * m_ItemEventHeight - Config.MenuItemPadding;
    case mcRecording: return MaxItems() * m_ItemRecordingHeight - Config.MenuItemPadding;
    default: return MaxItems() * m_ItemHeight - Config.MenuItemPadding;
    }
}

void cFlatDisplayMenu::Clear() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::Clear()");
#endif

    MenuItemScroller.Clear();
    PixmapClear(MenuPixmap);
    PixmapClear(MenuIconsPixmap);
    PixmapClear(MenuIconsBgPixmap);
    PixmapClear(MenuIconsOvlPixmap);
    PixmapClear(ScrollbarPixmap);
    PixmapClear(ContentHeadPixmap);
    PixmapClear(ContentHeadIconsPixmap);
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
    m_EventInfoDrawn = m_RecordingInfoDrawn = false;
}

void cFlatDisplayMenu::SetTitle(const char *Title) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetTitle() '%s' m_MenuCategory: %d", Title, m_MenuCategory);
#endif

    m_LastTitle = Title;
    cString NewTitle = Title;

    switch (m_MenuCategory) {
    case mcMain: NewTitle = ""; break;
    case mcChannel:
        if (Config.MenuChannelShowCount) {
            uint16_t ChanCount {0};  // Number of channels (Max. 65535)
            {
                LOCK_CHANNELS_READ;                 // Creates local const cChannels *Channels
                ChanCount = Channels->MaxNumber();  // Number of channels
            }
            NewTitle = cString::sprintf("%s (%d)", Title, ChanCount);
        }  // Config.MenuChannelShowCount
        break;
    case mcTimer:
        if (Config.MenuTimerShowCount) {
            //* UpdateTimerCounts() is called in Flush()
            NewTitle = cString::sprintf("%s (%d/%d)", Title, m_LastTimerActiveCount, m_LastTimerCount);
        }
        break;
    case mcRecording:
        if (Config.MenuRecordingShowCount) {
            //* GetRecCounts() is called in SetItemRecording()
            NewTitle = cString::sprintf("%s %s", Title, *m_RecCounts);
        }
        break;
    default: break;
    }  // switch (m_MenuCategory)

    TopBarSetTitle(*NewTitle);  // Must be called before other TopBarSet*

    if (Config.TopBarMenuIconShow) TopBarSetMenuIcon(*GetMenuIconName());

    // Show disk usage in Top Bar if:
    // - in Recording or Timer menu and Config.DiskUsageShow > 0
    // - in any menu and Config.DiskUsageShow > 1
    // - always when Config.DiskUsageShow == 3 (Handled in TopBarCreate() and TopBarSetTitle())
    if (((m_MenuCategory == mcRecording || m_MenuCategory == mcTimer) && (Config.DiskUsageShow > 0)) ||
        ((m_MenuCategory > mcUndefined && (Config.DiskUsageShow > 1))))
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
    if (m_MenuCategory == mcMain && Config.MainMenuWidgetsEnable) m_MenuItemWidth *= Config.MainMenuItemScale;

    if (m_IsScrolling) m_MenuItemWidth -= m_WidthScrollBar;

    const auto [ColorFg, ColorBg, ColorExtraTextFg] =
        Current ? std::make_tuple(Theme.Color(clrItemCurrentFont), Theme.Color(clrItemCurrentBg),
                                  Theme.Color(clrMenuItemExtraTextCurrentFont))
        : Selectable
            ? std::make_tuple(Theme.Color(clrItemSelableFont), Theme.Color(clrItemSelableBg),
                              Theme.Color(clrMenuItemExtraTextFont))
            : std::make_tuple(Theme.Color(clrItemFont), Theme.Color(clrItemBg), Theme.Color(clrMenuItemExtraTextFont));

    const int y {Index * m_ItemHeight};
    if (y + m_ItemHeight > m_MenuItemLastHeight) m_MenuItemLastHeight = y + m_ItemHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, m_FontHeight), ColorBg);

    int XOff {0}, xt {0};
    const char *s {nullptr};
    for (std::size_t i {0}; i < MaxTabs; ++i) {
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
                        img = ImgLoader.GetIcon("text_timer_full_cur", m_FontHeight, m_FontHeight);
                    } else if (Selectable) {
                        img = ImgLoader.GetIcon("text_timer_full_sel", m_FontHeight, m_FontHeight);
                    } else {
                        img = ImgLoader.GetIcon("text_timer_full", m_FontHeight, m_FontHeight);
                    }
                    break;
                case '#':
                    if (Current) {
                        img = ImgLoader.GetIcon("timerRecording_cur", m_FontHeight, m_FontHeight);
                    } else if (Selectable) {
                        img = ImgLoader.GetIcon("timerRecording_sel", m_FontHeight, m_FontHeight);
                    } else {
                        img = ImgLoader.GetIcon("timerRecording", m_FontHeight, m_FontHeight);
                    }
                    break;
                case '!':
                    if (Current) {
                        img = ImgLoader.GetIcon("text_arrowturn_cur", m_FontHeight, m_FontHeight);
                    } else if (Selectable) {
                        img = ImgLoader.GetIcon("text_arrowturn_sel", m_FontHeight, m_FontHeight);
                    } else {
                        img = ImgLoader.GetIcon("text_arrowturn", m_FontHeight, m_FontHeight);
                    }
                    break;
                // case ' ':
                default: break;
                }
                // Draw the icon
                if (img) MenuIconsPixmap->DrawImage(cPoint(XOff, y + (m_FontHeight - img->Height()) / 2), *img);
            } else {  // Not EPGsearch timer menu
                if ((m_MenuCategory == mcMain || m_MenuCategory == mcSetup) && Config.MenuItemIconsShow) {
                    const int IconSize {m_FontHeight - m_MarginItem2};
                    const cString IconName = *GetIconName(*MainMenuText(s));
                    cImage *img {nullptr};
                    if (Current) {
                        const cString IconNameCur = cString::sprintf("%s_cur", *IconName);
                        img = ImgLoader.GetIcon(*IconNameCur, IconSize, IconSize);
                    }
                    if (!img) img = ImgLoader.GetIcon(*IconName, IconSize, IconSize);

                    if (!img) { img = ImgLoader.GetIcon("menuIcons/blank", IconSize, IconSize); }
                    // Draw the icon
                    if (img) {
                        MenuIconsPixmap->DrawImage(
                            cPoint(xt + Config.decorBorderMenuItemSize + m_MarginItem, y + m_MarginItem), *img);
                    }

                    const int AvailableTextWidth {m_MenuItemWidth - m_WidthScrollBar};
                    MenuPixmap->DrawText(cPoint(m_FontHeight + m_MarginItem2 + xt + Config.decorBorderMenuItemSize, y),
                                         s, ColorFg, ColorBg, m_Font, AvailableTextWidth - xt - IconSize);
                } else {
                    if (Config.MenuItemParseTilde) {
                        const char *TildePos {strchr(s, '~')};
                        if (TildePos) {
                            cString first(s, TildePos);
                            cString second(TildePos + 1);
                            first.CompactChars(' ');   // Remove extra spaces
                            second.CompactChars(' ');  // Remove extra spaces

                            MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), *first, ColorFg,
                                                 ColorBg, m_Font,
                                                 m_MenuItemWidth - xt - Config.decorBorderMenuItemSize);
                            const int l {m_Font->Width(*first) +
                                         FontCache.GetStringWidth(m_FontName, m_FontHeight, "~")};
                            MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize + l, y), *second,
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
            }  // Not EPGSearch searchtimer
        }  // if (s)
        if (!Tab(i + 1)) break;
    }  // for

    sDecorBorder ib {.Left = Config.decorBorderMenuItemSize,
                     .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 +
                            Config.decorBorderMenuItemSize + y,
                     .Width = m_MenuItemWidth,
                     .Height = m_FontHeight,
                     .Size = Config.decorBorderMenuItemSize,
                     .Type = Config.decorBorderMenuItemType};

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

    if (!m_IsScrolling) ItemBorderInsertUnique(ib);

    SetEditableWidth(m_MenuWidth - Tab(1));
}

bool cFlatDisplayMenu::SetItemChannel(const cChannel *Channel, int Index, bool Current, bool Selectable,
                                      bool WithProvider) {
    if (!MenuPixmap || !MenuIconsPixmap || !MenuIconsBgPixmap) return false;

    if (Config.MenuChannelView == 0 || !Channel) return false;

    if (Current) MenuItemScroller.Clear();

    // flatPlus short, flatPlus short + EPG
    const bool MenuChannelViewShort {(Config.MenuChannelView == 3 || Config.MenuChannelView == 4)};

    int Height {m_FontHeight};
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (MenuChannelViewShort) {  // flatPlus short, flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem + Config.decorProgressMenuItemSize;
        m_MenuItemWidth /= 2;
    }

    if (m_IsScrolling) m_MenuItemWidth -= m_WidthScrollBar;

    const auto [ColorFg, ColorBg] =
        Current      ? std::make_pair(Theme.Color(clrItemCurrentFont), Theme.Color(clrItemCurrentBg))
        : Selectable ? std::make_pair(Theme.Color(clrItemSelableFont), Theme.Color(clrItemSelableBg))
                     : std::make_pair(Theme.Color(clrItemFont), Theme.Color(clrItemBg));

    const int y {Index * m_ItemChannelHeight};
    if (y + m_ItemChannelHeight > m_MenuItemLastHeight) m_MenuItemLastHeight = y + m_ItemChannelHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);

    int Left {Config.decorBorderMenuItemSize + m_MarginItem};
    int Top {y};
    //* At least four digits because of different sort modes
    int Width = FontCache.GetStringWidth(m_FontName, m_FontHeight, "0000");
    const bool IsGroup {Channel->GroupSep()};
    cString Buffer {""};
    if (!IsGroup) {  // Show channel number for channels only
        const int ChannelNumber {Channel->Number()};
        if (ChannelNumber > 9999) Width = m_Font->Width(*Buffer);
        Buffer = itoa(ChannelNumber);
        MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, Width, m_FontHeight, taRight);
    }

    Left += Width + m_MarginItem;

    int ImageLeft {Left};
    int ImageTop {Top};
    int ImageHeight {m_FontHeight};
    int ImageBgWidth = ImageHeight * 1.34f;  // Narrowing conversion
    int ImageBgHeight {ImageHeight};

    cImage *img {nullptr};
    if (!IsGroup) {
        img = ImgLoader.GetIcon("logo_background", ImageBgWidth, ImageBgHeight);
        if (img) {
            ImageBgHeight = img->Height();
            ImageBgWidth = img->Width();
            ImageTop = Top + (m_FontHeight - ImageBgHeight) / 2;
            MenuIconsBgPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }
        // Load named logo only for channels
        img = ImgLoader.GetLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
    }

    if (!img) {
        const bool IsRadioChannel {((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false};
        if (IsRadioChannel) {
            if (Current) img = ImgLoader.GetIcon("radio_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img) img = ImgLoader.GetIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
        } else if (IsGroup) {
            img = ImgLoader.GetIcon("changroup", ImageBgWidth - 10, ImageBgHeight - 10);
        } else {
            if (Current) img = ImgLoader.GetIcon("tv_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img) img = ImgLoader.GetIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);
        }
    }
    if (img) {  // Draw the logo
        ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
        ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
        MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        Left += ImageBgWidth + m_MarginItem2;
    }

    // Event from channel
    const cEvent *Event {nullptr};
    cString EventTitle {""};
    double progress {0.0};
    {
        LOCK_SCHEDULES_READ;  // Creates local const cSchedules *Schedules
        const cSchedule *Schedule {Schedules->GetSchedule(Channel)};
        if (Schedule) {
            Event = Schedule->GetPresentEvent();
            if (Event) {
                // Calculate progress bar
                const time_t now {time(0)};
                const double duration = Event->Duration();
                progress = (duration > 0) ? std::clamp((now - Event->StartTime()) * 100.0 / duration, 0.0, 100.0) : 0.0;
                EventTitle = Event->Title();
            }
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
            // const int LineTop {Top + (m_FontHeight - 3) / 2};  //? Better calculation
            const int LineTop {Top + (m_FontHeight - m_LineWidth / 2) / 2};
            // dsyslog("flatPlus: SetItemChannel() LineTop: %d, m_LineWidth: %d, m_FontHeight: %d", LineTop,
            //        m_LineWidth, m_FontHeight);
            MenuPixmap->DrawRectangle(cRect(Left, LineTop, m_MenuItemWidth - Left, m_LineWidth), ColorFg);
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
    } else {  // flatPlus long with epg or flatPlus short (with epg)
        //? m_WidthScrollBar is already substracted. If not scrolling, we need to substract it
        Width = (m_MenuItemWidth + (m_IsScrolling ? m_WidthScrollBar : 0)) / 10 * 2;

        if (MenuChannelViewShort)  // flatPlus short, flatPlus short + EPG
            Width = m_MenuItemWidth - LeftName;

        if (IsGroup) {
            const int LineTop {Top + (m_FontHeight - m_LineWidth / 2) / 2};
            MenuPixmap->DrawRectangle(cRect(Left, LineTop, m_MenuItemWidth - Left, m_LineWidth), ColorFg);
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

            int PBTop {y + (m_ItemChannelHeight - Config.MenuItemPadding) / 2 -
                        Config.decorProgressMenuItemSize / 2 - Config.decorBorderMenuItemSize};
            int PBLeft {Left};
            int PBWidth {m_MenuItemWidth / 10};
            if (MenuChannelViewShort) {  // flatPlus short, flatPlus short + EPG
                PBTop = Top + m_FontHeight + m_FontSmlHeight;
                PBLeft = Left - Width - m_MarginItem;
                //? m_WidthScrollBar is already substarcted
                PBWidth =
                    m_MenuItemWidth - LeftName - m_MarginItem2 - Config.decorBorderMenuItemSize - m_WidthScrollBar;

                if (m_IsScrolling) PBWidth += m_WidthScrollBar;
            }

            //? m_WidthScrollBar is already substarcted
            Width = (m_MenuItemWidth + (m_IsScrolling ? m_WidthScrollBar : 0)) / 10;
            const cRect ProgressBarSize {PBLeft, PBTop, PBWidth, Config.decorProgressMenuItemSize};
            if (Current)
                ProgressBarDrawRaw(MenuPixmap, MenuPixmap, ProgressBarSize, ProgressBarSize, progress, 100,
                                    Config.decorProgressMenuItemCurFg, Config.decorProgressMenuItemCurBarFg,
                                    Config.decorProgressMenuItemCurBg, Config.decorProgressMenuItemType, false);
            else
                ProgressBarDrawRaw(MenuPixmap, MenuPixmap, ProgressBarSize, ProgressBarSize, progress, 100,
                                    Config.decorProgressMenuItemFg, Config.decorProgressMenuItemBarFg,
                                    Config.decorProgressMenuItemBg, Config.decorProgressMenuItemType, false);
            Left += Width + m_MarginItem;

            if (MenuChannelViewShort) {  // flatPlus short, flatPlus short + EPG
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

    sDecorBorder ib {.Left = Config.decorBorderMenuItemSize,
                     .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 +
                            Config.decorBorderMenuItemSize + y,
                     .Width = m_MenuItemWidth,
                     .Height = Height,
                     .Size = Config.decorBorderMenuItemSize,
                     .Type = Config.decorBorderMenuItemType};

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

    if (!m_IsScrolling) ItemBorderInsertUnique(ib);

    if (Config.MenuChannelView == 4 && Current) DrawItemExtraEvent(Event, "");

    return true;
}

void cFlatDisplayMenu::DrawItemExtraEvent(const cEvent *Event, const cString EmptyText) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawItemExtraEvent()");
#endif

    m_cLeft = m_MenuItemWidth + Config.decorBorderMenuItemSize * 2 + Config.decorBorderMenuContentSize + m_MarginItem;
    if (m_IsScrolling) m_cLeft += m_WidthScrollBar;

    m_cTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
    m_cWidth = m_MenuWidth - m_cLeft - Config.decorBorderMenuContentSize;
    m_cHeight =
        m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                       Config.decorBorderButtonSize * 2 + m_MarginItem3 + Config.decorBorderMenuContentSize * 2);

    bool IsEmpty {false};
    cString Text {""};
    if (Event) {
        // Description or ShortText if Description is empty
        if (isempty(Event->Description())) {
            if (!isempty(Event->ShortText())) Text.Append(Event->ShortText());
        } else {
            Text.Append(Event->Description());
        }

        if (Config.EpgAdditionalInfoShow) {
            if (Text[0] != '\0') Text.Append("\n");
            InsertGenreInfo(Event, Text);  // Add genre info

            if (Event->ParentalRating())  // FSK
                Text.Append(cString::sprintf("\n%s: %s", tr("FSK"), *Event->GetParentalRatingString()));

            const cComponents *Components {Event->Components()};
            if (Components) {
                cString Audio {""}, Subtitle {""};
                InsertComponents(Components, Text, Audio, Subtitle, true);  // Get info for audio/video and subtitle

                if (Audio[0] != '\0') Text.Append(cString::sprintf("\n%s: %s", tr("Audio"), *Audio));
                if (Subtitle[0] != '\0') Text.Append(cString::sprintf("\n%s: %s", tr("Subtitle"), *Subtitle));
            }  // if Components
        }  // EpgAdditionalInfoShow
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
        cImage *img {ImgLoader.GetIcon("timerInactiveBig", 256, 256)};
        if (img) {
            ComplexContent.AddImage(img, cRect(m_MarginItem, m_MarginItem, img->Width(), img->Height()));
            ComplexContent.AddText(
                *Text, true,
                cRect(m_MarginItem, m_MarginItem + img->Height(), m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
        }
    } else {
        cString MediaPath {""};
        int MediaWidth {0}, MediaHeight {m_cHeight - m_MarginItem2};
        int MediaType {0};  // 0 = None, 1 = Series, 2 = Movie
        if (Config.TVScraperEPGInfoShowPoster) {
            cSize Dummy {0, 0};
            MediaType = GetScraperMediaTypeSize(MediaPath, Dummy, Event);
            if (MediaType == 1)
                MediaWidth = m_cWidth - m_MarginItem2;
            else if (MediaType == 2)
                MediaWidth = m_cWidth / 2 - m_MarginItem3;
        }  // TVScraperEPGInfoShowPoster

        if (MediaPath[0] != '\0') {
            cImage *img {ImgLoader.GetFile(*MediaPath, MediaWidth, MediaHeight)};
            if (img && MediaType == 2) {  // Movie
                ComplexContent.AddImageWithFloatedText(
                    img, CIP_Right, *Text, m_MarginItem,
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
                    *Text, true, cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
            }
        } else {
            ComplexContent.AddText(
                *Text, true, cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
        }
    }
    ComplexContent.CreatePixmaps(Config.MenuContentFullSize);
    ComplexContent.Draw();

    DecorBorderClearByFrom(BorderContent);
    const sDecorBorder ib {m_cLeft,
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
    dsyslog("flatPlus: cFlatDisplayMenu::SetItemTimer() Index: %d", Index);
#endif

    if (!MenuPixmap || !MenuIconsPixmap || !MenuIconsOvlPixmap || !MenuIconsBgPixmap) return false;

    if (Config.MenuTimerView == 0 || !Timer) return false;

    if (Current) MenuItemScroller.Clear();

    int Height {m_FontHeight};
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuTimerView == 2 || Config.MenuTimerView == 3) {  // flatPlus short, flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem;
        m_MenuItemWidth /= 2;
    }

    if (m_IsScrolling) m_MenuItemWidth -= m_WidthScrollBar;

    auto [ColorFg, ColorBg, ColorExtraTextFg] =
        Current ? std::make_tuple(Theme.Color(clrItemCurrentFont), Theme.Color(clrItemCurrentBg),
                                  Theme.Color(clrMenuItemExtraTextCurrentFont))
        : Selectable
            ? std::make_tuple(Theme.Color(clrItemSelableFont), Theme.Color(clrItemSelableBg),
                              Theme.Color(clrMenuItemExtraTextFont))
            : std::make_tuple(Theme.Color(clrItemFont), Theme.Color(clrItemBg), Theme.Color(clrMenuItemExtraTextFont));

    const int y {Index * m_ItemTimerHeight};
    if (y + m_ItemTimerHeight > m_MenuItemLastHeight) m_MenuItemLastHeight = y + m_ItemTimerHeight;

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
    } else {  // Active timer
        IconName = "timerActive";
    }

    cImage *img {ImgLoader.GetIcon(*IconName, ImageHeight, ImageHeight)};
    if (img) {
        ImageTop = Top + (m_FontHeight - img->Height()) / 2;
        MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    }

    //? Make overlay configurable? (disable)
    if (Timer->Remote()) {  // Remote timer
        img = ImgLoader.GetIcon("timerRemote", ImageHeight, ImageHeight);
        if (img) {
            ImageTop = Top + (m_FontHeight - img->Height()) / 2;
            MenuIconsOvlPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }
    }
    Left += ImageHeight + m_MarginItem2;

    const cChannel *Channel {Timer->Channel()};
    const int ChannelNumber {Channel->Number()};
    cString Buffer = itoa(ChannelNumber);
    const int Width {(ChannelNumber < 1000) ? FontCache.GetStringWidth(m_FontName, m_FontHeight, "000")
                                            : m_Font->Width(*Buffer)};  // Minimal width for channel number

    MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, Width, m_FontHeight, taRight);
    Left += Width + m_MarginItem;

    ImageLeft = Left;
    int ImageBgWidth = ImageHeight * 1.34f;  // Narrowing conversion
    int ImageBgHeight {ImageHeight};
    img = ImgLoader.GetIcon("logo_background", ImageBgWidth, ImageBgHeight);
    if (img) {
        ImageBgWidth = img->Width();
        ImageBgHeight = img->Height();
        ImageTop = Top + (m_FontHeight - ImageBgHeight) / 2;
        MenuIconsBgPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    }

    img = ImgLoader.GetLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
    if (!img) {
        const bool IsRadioChannel {((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false};
        if (IsRadioChannel) {
            if (Current) img = ImgLoader.GetIcon("radio_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img) img = ImgLoader.GetIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
        } else {
            if (Current) img = ImgLoader.GetIcon("tv_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img) img = ImgLoader.GetIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);
        }
    }
    if (img) {  // Draw the logo
        ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
        ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
        MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    }
    Left += ImageBgWidth + m_MarginItem2;

    cString day {""}, name {""};
    if (Timer->WeekDays()) {
        day = Timer->PrintDay(0, Timer->WeekDays(), false);
    } else if (Timer->Day() - time(0) < 28 * SECSINDAY) {
        day = itoa(Timer->GetMDay(Timer->Day()));
        name = WeekDayName(Timer->Day());
    } else {
        const time_t Day {Timer->Day()};
        struct tm *tm_r = gmtime(&Day);  // NOLINT gmtime() is not thread-safe
        char buf[12];
        strftime(buf, sizeof(buf), "%Y%m%d", tm_r);
        day = buf;
    }
    const char *File {(Setup.FoldersInTimerMenu) ? nullptr : strrchr(Timer->File(), FOLDERDELIMCHAR)};
    if (File && strcmp(File + 1, TIMERMACRO_TITLE) && strcmp(File + 1, TIMERMACRO_EPISODE))
        ++File;
    else
        File = Timer->File();

    const div_t TimerStart {std::div(Timer->Start(), 100)};
    const div_t TimerStop {std::div(Timer->Stop(), 100)};
    if (Config.MenuTimerView == 1) {  // flatPlus long
        Buffer = cString::sprintf("%s%s%s.", *name, *name && **name ? " " : "", *day);
        MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                             m_MenuItemWidth - Left - m_MarginItem);
        Left += FontCache.GetStringWidth(m_FontName, m_FontHeight, "MMM 00.  ");
        Buffer =
            cString::sprintf("%02d:%02d - %02d:%02d", TimerStart.quot, TimerStart.rem, TimerStop.quot, TimerStop.rem);
        MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                             m_MenuItemWidth - Left - m_MarginItem);
        Left += FontCache.GetStringWidth(m_FontName, m_FontHeight, "00:00 - 00:00  ");

        if (Current && m_Font->Width(File) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
            MenuItemScroller.AddScroller(
                File, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight), ColorFg,
                clrTransparent, m_Font, ColorExtraTextFg);
        } else {
            if (Config.MenuItemParseTilde) {
                const char *TildePos {strchr(File, '~')};
                if (TildePos) {
                    cString first(File, TildePos);
                    cString second(TildePos + 1);
                    first.CompactChars(' ');   // Remove extra spaces
                    second.CompactChars(' ');  // Remove extra spaces

                    MenuPixmap->DrawText(cPoint(Left, Top), *first, ColorFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
                    const int l {m_Font->Width(*first) + FontCache.GetStringWidth(m_FontName, m_FontHeight, "~")};
                    MenuPixmap->DrawText(cPoint(Left + l, Top), *second, ColorExtraTextFg, ColorBg, m_Font,
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
    } else if (Config.MenuTimerView == 2 || Config.MenuTimerView == 3) {  // flatPlus short, flatPlus short + EPG
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
                    cString first(File, TildePos);
                    cString second(TildePos + 1);
                    first.CompactChars(' ');   // Remove extra spaces
                    second.CompactChars(' ');  // Remove extra spaces

                    MenuPixmap->DrawText(cPoint(Left, Top + m_FontHeight), *first, ColorFg, ColorBg, m_FontSml,
                                         m_MenuItemWidth - Left - m_MarginItem);
                    const int l {m_FontSml->Width(*first) +
                                 FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "~")};
                    MenuPixmap->DrawText(cPoint(Left + l, Top + m_FontHeight), *second, ColorExtraTextFg, ColorBg,
                                         m_FontSml, m_MenuItemWidth - Left - l - m_MarginItem);
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

    sDecorBorder ib {.Left = Config.decorBorderMenuItemSize,
                     .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 +
                            Config.decorBorderMenuItemSize + y,
                     .Width = m_MenuItemWidth,
                     .Height = Height,
                     .Size = Config.decorBorderMenuItemSize,
                     .Type = Config.decorBorderMenuItemType};

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

    if (!m_IsScrolling) ItemBorderInsertUnique(ib);

    if (Config.MenuTimerView == 3 && Current) {  // flatPlus short
        const cEvent *Event {Timer->Event()};
        DrawItemExtraEvent(Event, tr("timer not enabled"));
    }

    return true;
}

bool cFlatDisplayMenu::SetItemEvent(const cEvent *Event, int Index, bool Current, bool Selectable,
                                    const cChannel *Channel, bool WithDate, eTimerMatch TimerMatch, bool TimerActive) {
    if (!MenuPixmap || !MenuIconsBgPixmap || !MenuIconsPixmap) return false;

    if (Config.MenuEventView == 0) return false;

    if (Config.MenuEventViewAlwaysWithDate) WithDate = true;

    if (Current) MenuItemScroller.Clear();

    const bool MenuEventViewShort {(Config.MenuEventView == 2 || Config.MenuEventView == 3)};

    int Height {m_FontHeight};
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (MenuEventViewShort) {  // flatPlus short. flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem2 + Config.decorProgressMenuItemSize / 2;
        m_MenuItemWidth *= 0.6;
    }

    if (m_IsScrolling) m_MenuItemWidth -= m_WidthScrollBar;

    const auto [ColorFg, ColorBg, ColorExtraTextFg] =
        Current ? std::make_tuple(Theme.Color(clrItemCurrentFont), Theme.Color(clrItemCurrentBg),
                                  Theme.Color(clrMenuItemExtraTextCurrentFont))
        : Selectable
            ? std::make_tuple(Theme.Color(clrItemSelableFont), Theme.Color(clrItemSelableBg),
                              Theme.Color(clrMenuItemExtraTextFont))
            : std::make_tuple(Theme.Color(clrItemFont), Theme.Color(clrItemBg), Theme.Color(clrMenuItemExtraTextFont));

    const int y {Index * m_ItemEventHeight};
    if (y + m_ItemEventHeight > m_MenuItemLastHeight) m_MenuItemLastHeight = y + m_ItemEventHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);

    int Left {Config.decorBorderMenuItemSize + m_MarginItem};
    int LeftSecond {Left};
    int Top {y}, w {0};
    int ImageTop {Top};
    cImage *img {nullptr};
    if (Channel) {
        cString Buffer {""};
        w = FontCache.GetStringWidth(m_FontName, m_FontHeight, "0000");
        const bool IsGroup {Channel->GroupSep()};
        if (!IsGroup) {  // Show channel number for channels only
            const int ChannelNumber {Channel->Number()};
            if (ChannelNumber > 9999) w = m_Font->Width(*Buffer);  // Width for channel number in Event (EPGSearch)
            Buffer = itoa(ChannelNumber);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, w, m_FontHeight, taRight);
        }
        Left += w + m_MarginItem;

        cString ChannelName {Channel->Name()};
        if (Current) m_ItemEventLastChannelName = ChannelName;

        int ImageLeft {Left};
        int ImageBgWidth = m_FontHeight * 1.34f;  // Narrowing conversion
        int ImageBgHeight {m_FontHeight};
        if (!IsGroup) {
            img = ImgLoader.GetIcon("logo_background", ImageBgWidth, ImageBgHeight);
            if (img) {
                ImageBgWidth = img->Width();
                ImageBgHeight = img->Height();
                ImageTop = Top + (m_FontHeight - ImageBgHeight) / 2;
                MenuIconsBgPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
            }
            // Load named logo only for channels
            img = ImgLoader.GetLogo(*ChannelName, ImageBgWidth - 4, ImageBgHeight - 4);
        }

        if (!img) {
            const bool IsRadioChannel {((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false};
            if (IsRadioChannel) {
                if (Current) img = ImgLoader.GetIcon("radio_cur", ImageBgWidth - 10, ImageBgHeight - 10);
                if (!img) img = ImgLoader.GetIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
            } else if (IsGroup) {
                img = ImgLoader.GetIcon("changroup", ImageBgWidth - 10, ImageBgHeight - 10);
            } else {
                if (Current) img = ImgLoader.GetIcon("tv_cur", ImageBgWidth - 10, ImageBgHeight - 10);
                if (!img) img = ImgLoader.GetIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);
            }
        }
        if (img) {  // Draw the logo
            ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
            ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
            MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }

        Left += ImageBgWidth + m_MarginItem2;
        LeftSecond = Left;
        // m_WidthScrollBar is already substracted. If not scrolling, we need to substract it
        w = m_IsScrolling ? m_MenuItemWidth / 10 * 2 : (m_MenuItemWidth - m_WidthScrollBar) / 10 * 2;

        if (MenuEventViewShort) {  // flatPlus short, flatPlus short + EPG
            w = m_Font->Width(*ChannelName);
        } else {
            ChannelName = Channel->ShortName(true);
        }

        if (IsGroup) {  // Draw group name or channel name
            const int LineTop {Top + (m_FontHeight - m_LineWidth / 2) / 2};
            MenuPixmap->DrawRectangle(cRect(Left, LineTop, m_MenuItemWidth - Left, m_LineWidth), ColorFg);
            Left += w / 2;
            Buffer = cString::sprintf(" %s ", *ChannelName);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, 0, 0, taCenter);
        } else {
            MenuPixmap->DrawText(cPoint(Left, Top), *ChannelName, ColorFg, ColorBg, m_Font, w);
        }

        Left += w + m_MarginItem2;

        if (Event) {  // Draw progress of event
            // m_WidthScrollBar is already substracted. If not scrolling, we need to substract it
            int PBWidth {(m_IsScrolling) ? m_MenuItemWidth / 20 : (m_MenuItemWidth - m_WidthScrollBar) / 20};

            const time_t now {time(0)};
            const time_t EventStartTime {Event->StartTime()};
            static constexpr int kTwoMinutes {2 * 60};
            if ((now >= (EventStartTime - kTwoMinutes))) {
                const int total = Event->EndTime() - EventStartTime;  // Narrowing conversion
                if (total >= 0) {
                    // Calculate progress bar
                    const double duration = Event->Duration();
                    const double progress =
                        (duration > 0) ? std::clamp((now - EventStartTime) * 100.0 / duration, 0.0, 100.0) : 0.0;

                    int PBLeft {Left};
                    int PBTop {y + (m_ItemEventHeight - Config.MenuItemPadding) / 2 -
                               Config.decorProgressMenuItemSize / 2 - Config.decorBorderMenuItemSize};
                    int PBHeight {Config.decorProgressMenuItemSize};

                    if (MenuEventViewShort) {  // flatPlus short, flatPlus short + EPG
                        PBTop = y + m_FontHeight + m_FontSmlHeight + m_MarginItem;
                        PBWidth = m_MenuItemWidth - LeftSecond - m_WidthScrollBar - m_MarginItem2;
                        if (m_IsScrolling) PBWidth += m_WidthScrollBar;

                        PBLeft = LeftSecond;
                        PBHeight = Config.decorProgressMenuItemSize / 2;
                    }

                    if (!IsGroup) {  // Exclude epg2vdr group separator
                        const cRect ProgressBarSize {PBLeft, PBTop, PBWidth, PBHeight};
                        if (Current)
                            ProgressBarDrawRaw(MenuPixmap, MenuPixmap, ProgressBarSize, ProgressBarSize, progress, 100,
                                               Config.decorProgressMenuItemCurFg, Config.decorProgressMenuItemCurBarFg,
                                               Config.decorProgressMenuItemCurBg, Config.decorProgressMenuItemType,
                                               false);
                        else
                            ProgressBarDrawRaw(MenuPixmap, MenuPixmap, ProgressBarSize, ProgressBarSize, progress, 100,
                                               Config.decorProgressMenuItemFg, Config.decorProgressMenuItemBarFg,
                                               Config.decorProgressMenuItemBg, Config.decorProgressMenuItemType, false);
                    }
                }
            }
            Left += PBWidth + m_MarginItem;
        }  // if Event
    } else {
        TopBarSetMenuLogo(m_ItemEventLastChannelName);
    }  // if Channel

    int ImageHeight {m_FontHeight};
    if (Event && Selectable) {
        cString Buffer {""};
        if (WithDate) {  // Draw day and date (Mon 01.)
            const time_t Day {Event->StartTime()};
            struct tm *tm_r = gmtime(&Day);  // NOLINT gmtime() is not thread-safe
            char buf[3];
            snprintf(buf, sizeof(buf), "%2d", tm_r->tm_mday);

            Buffer = cString::sprintf("%s %s. ", *WeekDayName(Day), buf);
            if (MenuEventViewShort && Channel) {  // flatPlus short, flatPlus short + EPG
                w = FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "Mon 00. ");
                MenuPixmap->DrawText(cPoint(LeftSecond, Top + m_FontHeight), *Buffer, ColorFg, ColorBg, m_FontSml,
                                     w);
                LeftSecond += w + m_MarginItem;
            } else {
                w = FontCache.GetStringWidth(m_FontName, m_FontHeight, "Mon 00. ");
                MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, w, m_FontHeight,
                                     taRight);
            }
            Left += w + m_MarginItem;
        }
        // Draw time (00:00)
        Buffer = *Event->GetTimeString();  // 00:00
        if (MenuEventViewShort && Channel) {  // flatPlus short, flatPlus short + EPG
            Left = LeftSecond;
            Top += m_FontHeight;
            ImageHeight = m_FontSmlHeight;
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml);
            Left += FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "00:00") + m_MarginItem;
            /* } else if (MenuEventViewShort) {
                    MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font);
                    Left += m_Font->Width(*Buffer) + m_MarginItem; */
        } else {  //? Same as above
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font);
            Left += FontCache.GetStringWidth(m_FontName, m_FontHeight, "00:00") + m_MarginItem;
        }

        if (TimerActive) {  // Show timer icon only if timer is active
            img = nullptr;
            if (TimerMatch == tmFull) {
                if (Current) img = ImgLoader.GetIcon("timer_full_cur", ImageHeight, ImageHeight);
                if (!img) img = ImgLoader.GetIcon("timer_full", ImageHeight, ImageHeight);
            } else if (TimerMatch == tmPartial) {
                if (Current) img = ImgLoader.GetIcon("timer_partial_cur", ImageHeight, ImageHeight);
                if (!img) img = ImgLoader.GetIcon("timer_partial", ImageHeight, ImageHeight);
            }
            if (img) MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
        }  // TimerActive

        // Draw VPS icon (Overlay)
        if (Event->Vps() && (Event->Vps() - Event->StartTime())) {
            img = nullptr;
            if (Current) img = ImgLoader.GetIcon("vps_cur", ImageHeight, ImageHeight);
            if (!img) img = ImgLoader.GetIcon("vps", ImageHeight, ImageHeight);
            if (img) MenuIconsOvlPixmap->DrawImage(cPoint(Left, Top), *img);
        }  // VPS
        Left += ImageHeight + m_MarginItem;

        // Draw title and short text
        const cString Title = Event->Title();
        const cString ShortText = (Event->ShortText()) ? Event->ShortText() : "";
        if (MenuEventViewShort && Channel) {  // flatPlus short, flatPlus short + EPG
            if (Current) {
                if (ShortText[0] != '\0') {
                    Buffer = cString::sprintf("%s~%s", *Title, *ShortText);
                    if (m_FontSml->Width(*Buffer) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                        MenuItemScroller.AddScroller(
                            *Buffer,
                            cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontSmlHeight),
                            ColorFg, clrTransparent, m_FontSml, ColorExtraTextFg);
                    } else {
                        MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_FontSml,
                                             m_MenuItemWidth - Left - m_MarginItem);
                        Left +=
                            m_FontSml->Width(*Title) + FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "~");
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
                if (ShortText[0] != '\0') {
                    Left += m_FontSml->Width(*Title) + FontCache.GetStringWidth(m_FontName, m_FontHeight, "~");
                    MenuPixmap->DrawText(cPoint(Left, Top), *ShortText, ColorExtraTextFg, ColorBg, m_FontSml,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            }
        } else if (MenuEventViewShort) {  // flatPlus short, flatPlus short + EPG
            if (Current && m_Font->Width(*Title) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    *Title, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight), ColorFg,
                    clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
            if (ShortText[0] != '\0') {
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
                if (ShortText[0] != '\0') {
                    Buffer = cString::sprintf("%s~%s", *Title, *ShortText);
                    if (m_Font->Width(*Buffer) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                        MenuItemScroller.AddScroller(
                            *Buffer, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                            ColorFg, clrTransparent, m_Font, ColorExtraTextFg);
                    } else {
                        MenuPixmap->DrawText(cPoint(Left, Top), *Title, ColorFg, ColorBg, m_Font,
                                             m_MenuItemWidth - Left - m_MarginItem);
                        Left += m_Font->Width(*Title) + FontCache.GetStringWidth(m_FontName, m_FontHeight, "~");
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
                if (ShortText[0] != '\0') {
                    Left += m_Font->Width(*Title) + FontCache.GetStringWidth(m_FontName, m_FontHeight, "~");
                    MenuPixmap->DrawText(cPoint(Left, Top), *ShortText, ColorExtraTextFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            }
        }
    } else if (Event) {
        // dsyslog("flatPlus: SetItemEvent() Try to extract date from event title '%s'", Event->Title());
        if (Channel && Channel->GroupSep()) {  // Exclude epg2vdr group separator '-----#011 Nachrichten -----'
            // dsyslog("   Channel is group separator!");
        } else {  // Extract date from separator
            // EPGSearch program '--------------------         Fr. 21.02.2025 --------------------'
            std::string_view sep {Event->Title()};  // Event->Title should always set to something
            if (sep.length() > 16) {                // Date with day and search string ' -'
                const std::size_t found {sep.find(" -")};
                if (found != std::string_view::npos && found >= 14) {
                    // dsyslog("   Date string found at %ld", found);
                    const std::string date {sep.substr(found - 14, 14)};
                    const int LineTop {Top + (m_FontHeight - m_LineWidth / 2) / 2};
                    MenuPixmap->DrawRectangle(cRect(0, LineTop, m_MenuItemWidth, m_LineWidth), ColorFg);
                    const cString DateSpace = cString::sprintf(" %s ", date.c_str());
                    MenuPixmap->DrawText(cPoint(LeftSecond + m_MenuWidth / 10 * 2, Top), *DateSpace, ColorFg, ColorBg,
                                         m_Font, 0, 0, taCenter);
                } else {  // Date string not found
                    MenuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
                }
            } else {  // Event title too short
                MenuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
        }
    }  // Event && Selectable

    sDecorBorder ib {.Left = Config.decorBorderMenuItemSize,
                     .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 +
                            Config.decorBorderMenuItemSize + y,
                     .Width = m_MenuItemWidth,
                     .Height = Height,
                     .Size = Config.decorBorderMenuItemSize,
                     .Type = Config.decorBorderMenuItemType};

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

    if (!m_IsScrolling) ItemBorderInsertUnique(ib);

    if (Config.MenuEventView == 3 && Current) DrawItemExtraEvent(Event, "");

    return true;
}

/**
 * @brief Draws an icon representing the state of a recording.
 *
 * This function draws an icon at the specified position to indicate whether a recording is currently being recorded,
 * replayed, is new, or has been seen. The icon is chosen based on the state of the recording and whether it is the
 * current item.
 *
 * @param Recording Pointer to the recording object whose state is to be represented.
 * @param Left The x-coordinate where the icon should be drawn.
 * @param Top The y-coordinate where the icon should be drawn.
 * @param Current Boolean indicating whether the recording is the current item.
 */
void cFlatDisplayMenu::DrawRecordingStateIcon(const cRecording *Recording, int Left, int Top, bool Current) {
    cImage *img {nullptr};
    const int RecordingIsInUse {Recording->IsInUse()};
    if ((RecordingIsInUse & ruTimer) != 0) {  // The recording is currently written to by a timer
        if (Current) img = ImgLoader.GetIcon("timerRecording_cur", m_FontHeight, m_FontHeight);
        if (!img) img = ImgLoader.GetIcon("timerRecording", m_FontHeight, m_FontHeight);
    } else if ((RecordingIsInUse & ruReplay) != 0) {  // The recording is being replayed
        if (Current) img = ImgLoader.GetIcon("play", m_FontHeight, m_FontHeight);
        if (!img) img = ImgLoader.GetIcon("play_sel", m_FontHeight, m_FontHeight);
        // img = ImgLoader.GetIcon("recording_replay", m_FontHeight, m_FontHeight);
    } else if (Recording->IsNew()) {
        if (Current) img = ImgLoader.GetIcon("recording_new_cur", m_FontHeight, m_FontHeight);
        if (!img) img = ImgLoader.GetIcon("recording_new", m_FontHeight, m_FontHeight);
    } else {
        const cString IconName = *GetRecordingSeenIcon(Recording->NumFrames(), Recording->GetResume());
        if (Current) {
            const cString IconNameCur = cString::sprintf("%s_cur", *IconName);
            img = ImgLoader.GetIcon(*IconNameCur, m_FontHeight, m_FontHeight);
        }
        if (!img) img = ImgLoader.GetIcon(*IconName, m_FontHeight, m_FontHeight);
    }
    // Draw the icon for recording state
    if (img) MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
}

/**
 * @brief Draws an icon representing a recording format (SD, HD, UHD).
 *
 * This function draws an overlay icon at the specified position to indicate the recording format.
 *
 * @param Recording The recording whose format should be drawn.
 * @param Left The x-coordinate where the icon should be drawn.
 * @param Top The y-coordinate where the icon should be drawn.
 */
void cFlatDisplayMenu::DrawRecordingFormatIcon(const cRecording *Recording, int Left, int Top) {
    static constexpr float kIconFormatHeightRatio {1.0 / 3.0};
    const cString IconName = *GetRecordingFormatIcon(Recording);    // Show (SD), HD or UHD Logo
    const int ImageHeight = m_FontHeight * kIconFormatHeightRatio;  // 1/3 height. Narrowing conversion
    const cImage *img {ImgLoader.GetIcon(*IconName, kIconMaxSize, ImageHeight)};
    if (img) {
        const int ImageTop {Top + m_FontHeight - m_FontAscender};
        const int ImageLeft {Left + m_FontHeight - img->Width()};
        MenuIconsOvlPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    }
}

/**
 * @brief Draws an icon representing the error state of a recording.
 *
 * This function draws an overlay icon at the specified position to indicate the error state of a recording.
 * The icon is based on the number of errors associated with the recording, and is visually differentiated if
 * the recording is the current item.
 *
 * @param Recording Pointer to the recording object whose error state is to be represented.
 * @param Left The x-coordinate where the icon should be drawn.
 * @param Top The y-coordinate where the icon should be drawn.
 * @param Current Boolean indicating whether the recording is the current item.
 */
void cFlatDisplayMenu::DrawRecordingErrorIcon(const cRecording *Recording, int Left, int Top, bool Current) {
    const cString IconName = *GetRecordingErrorIcon(Recording->Info()->Errors());
    cImage *img {nullptr};
    if (Current) {
        const cString IconNameCur = cString::sprintf("%s_cur", *IconName);
        img = ImgLoader.GetIcon(*IconNameCur, m_FontHeight, m_FontHeight);
    }
    if (!img) img = ImgLoader.GetIcon(*IconName, m_FontHeight, m_FontHeight);
    if (img) MenuIconsOvlPixmap->DrawImage(cPoint(Left, Top), *img);
}

/**
 * @brief Draws a recording icon on the menu.
 *
 * This function draws a recording icon at the specified position on the menu.
 * If the icon is marked as current, it attempts to load a special version of the icon
 * with a "_cur" suffix. If the special version is not found, it falls back to the regular icon.
 *
 * @param IconName The name of the icon to be drawn.
 * @param Left The x-coordinate where the icon will be drawn. This value will be updated to the new position after
 * drawing the icon.
 * @param Top The y-coordinate where the icon will be drawn.
 * @param Current A boolean indicating whether the icon is the current one.
 */
void cFlatDisplayMenu::DrawRecordingIcon(const char *IconName, int &Left, int Top, bool Current) {
    cImage *img {nullptr};
    if (Current) {
        const cString IconNameCur = cString::sprintf("%s_cur", IconName);
        img = ImgLoader.GetIcon(*IconNameCur, m_FontHeight, m_FontHeight);
    }
    if (!img) img = ImgLoader.GetIcon(IconName, m_FontHeight, m_FontHeight);
    if (img) {
        MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
        Left += m_FontHeight + m_MarginItem;
    }
}

bool cFlatDisplayMenu::SetItemRecording(const cRecording *Recording, int Index, bool Current, bool Selectable,
                                        int Level, int Total, int New) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetItemRecording()");
    dsyslog("   Index: %d, Level: %d, Total: %d, New: %d", Index, Level, Total, New);
#endif

    if (!MenuPixmap || !MenuIconsPixmap || !MenuIconsOvlPixmap) return false;

    if (Config.MenuRecordingView == 0) return false;

    if (Index == 0)  //? Only update when Index = 0 (First item on the page)
        m_RecFolder = (Level > 0) ? *GetRecordingName(Recording, Level - 1, true) : "";

    if (Config.MenuRecordingShowCount && m_LastItemRecordingLevel != Level) {  // Only update when Level changes
#ifdef DEBUGFUNCSCALL
        dsyslog("   Level changed from %d to %d", m_LastItemRecordingLevel, Level);
#endif
        m_LastItemRecordingLevel = Level;
        m_RecCounts = *GetRecCounts();
        const cString NewTitle = cString::sprintf("%s %s", *m_LastTitle, *m_RecCounts);
        TopBarSetTitle(*NewTitle, false);  // Do not clear
    }

    if (Current) MenuItemScroller.Clear();

    int Height {m_FontHeight};
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuRecordingView == 2 || Config.MenuRecordingView == 3) {  // flatPlus short, flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem;
        m_MenuItemWidth /= 2;
    }

    if (m_IsScrolling) m_MenuItemWidth -= m_WidthScrollBar;

    const auto [ColorFg, ColorBg, ColorExtraTextFg] =
        Current ? std::make_tuple(Theme.Color(clrItemCurrentFont), Theme.Color(clrItemCurrentBg),
                                  Theme.Color(clrMenuItemExtraTextCurrentFont))
        : Selectable
            ? std::make_tuple(Theme.Color(clrItemSelableFont), Theme.Color(clrItemSelableBg),
                              Theme.Color(clrMenuItemExtraTextFont))
            : std::make_tuple(Theme.Color(clrItemFont), Theme.Color(clrItemBg), Theme.Color(clrMenuItemExtraTextFont));

    const int y {Index * m_ItemRecordingHeight};
    if (y + m_ItemRecordingHeight > m_MenuItemLastHeight) m_MenuItemLastHeight = y + m_ItemRecordingHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);
    static constexpr float kIconCutHeightRatio {2.0 / 3.0};
    const int ImgRecNewHeight {Config.MenuRecordingView == 1 ? m_FontHeight : m_FontSmlHeight};
    //* Preload for calculation of position
    cImage *ImgRecCut {nullptr}, *ImgRecNew {nullptr};
    if (Current) {
        ImgRecNew = ImgLoader.GetIcon("recording_new_cur", ImgRecNewHeight, ImgRecNewHeight);
        ImgRecCut = ImgLoader.GetIcon("recording_cutted_cur", m_FontHeight, m_FontHeight * kIconCutHeightRatio);
    }
    if (!ImgRecNew) ImgRecNew = ImgLoader.GetIcon("recording_new", ImgRecNewHeight, ImgRecNewHeight);
    if (!ImgRecCut) ImgRecCut = ImgLoader.GetIcon("recording_cutted", m_FontHeight, m_FontHeight * kIconCutHeightRatio);

    const int ImgRecNewWidth {(ImgRecNew) ? ImgRecNew->Width() : 0};
    const int ImgRecCutWidth {(ImgRecCut) ? ImgRecCut->Width() : 0};
    const int ImgRecCutHeight {(ImgRecCut) ? ImgRecCut->Height() : 0};

    int Left {Config.decorBorderMenuItemSize + m_MarginItem};
    int Top {y};
    const bool IsRecording {Total == 0};  // Recording or a folder
    cString RecName = *GetRecordingName(Recording, Level, IsRecording);
    if (Config.MenuItemRecordingClearPercent && IsRecording) {  // Remove leading percent sign(s) from RecName
        while (RecName[0] != '\0' && RecName[0] == '%')
            RecName = cString(*RecName + 1);
    }
#ifdef DEBUGFUNCSCALL
    dsyslog("   RecName for display '%s'", *RecName);
#endif

    cString Buffer {""};
    if (Config.MenuRecordingView == 1) {  // flatPlus long
        if (IsRecording) {                 // Recording
            DrawRecordingIcon("recording", Left, Top, Current);

            const div_t TimeHM {std::div((Recording->LengthInSeconds() + 30) / 60, 60)};
            const cString Length = cString::sprintf("%02d:%02d", TimeHM.quot, TimeHM.rem);
            Buffer = cString::sprintf("%s  %s  Σ%s ", *ShortDateString(Recording->Start()),
                                      *TimeString(Recording->Start()), *Length);

            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                                 m_MenuItemWidth - Left - m_MarginItem);
            Left += FontCache.GetStringWidth(m_FontName, m_FontHeight, "00.00.00  00:00  Σ00:00") + m_MarginItem;

            // Show if recording is still in progress (ruTimer), or played (ruReplay)
            DrawRecordingStateIcon(Recording, Left, Top, Current);
#if APIVERSNUM >= 20505
            if (Config.MenuItemRecordingShowRecordingErrors) DrawRecordingErrorIcon(Recording, Left, Top, Current);
#endif

            Left += ImgRecNewWidth + m_MarginItem;
            if (Recording->IsEdited() && ImgRecCut) {
                const int ImageTop {Top + m_FontHeight - ImgRecCutHeight};  // 2/3 image height
                MenuIconsPixmap->DrawImage(cPoint(Left, ImageTop), *ImgRecCut);
            }

            if (Config.MenuItemRecordingShowFormatIcons)
                DrawRecordingFormatIcon(Recording, Left, Top);  // Show (SD), HD or UHD Logo

            Left += ImgRecCutWidth + (ImgRecCutWidth / 2) + m_MarginItem;  // Ensures exact 100% increase

            if (Current && m_Font->Width(*RecName) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    *RecName, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                    ColorFg, clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), *RecName, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
        } else if (Total > 0) {  // Folder with recordings
            DrawRecordingIcon("folder", Left, Top, Current);

            const int DigitsWidth {FontCache.GetStringWidth(m_FontName, m_FontHeight, "0000")};
            const int DigitsMaxWidth {DigitsWidth + m_MarginItem};  // Use same width for recs and new recs
            Buffer = itoa(Total);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, DigitsMaxWidth, m_FontHeight,
                                 taLeft);
            Left += DigitsMaxWidth;

            if (ImgRecNew) MenuIconsPixmap->DrawImage(cPoint(Left, Top), *ImgRecNew);

            Left += ImgRecNewWidth + m_MarginItem;
            Buffer = itoa(New);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, DigitsMaxWidth, m_FontHeight);

            Left += DigitsMaxWidth;
            int LeftWidth {Config.decorBorderMenuItemSize + m_FontHeight + (DigitsWidth * 2) + ImgRecNewWidth +
                           m_MarginItem * 5};  // For folder with recordings

            if (Config.MenuItemRecordingShowFolderDate > 0) {
                const int ShortDateWidth {FontCache.GetStringWidth(m_FontName, m_FontHeight, "00.00.00")};
                LeftWidth += ShortDateWidth + m_FontHeight + m_MarginItem2;
                Buffer = *ShortDateString(GetLastRecTimeFromFolder(Recording, Level));
                MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorExtraTextFg, ColorBg, m_Font);
                Left += ShortDateWidth + m_MarginItem;
                if (IsRecordingOld(Recording, Level)) {
                    DrawRecordingIcon("recording_old", Left, Top, Current);
                    Left -= m_FontHeight + m_MarginItem;  //* Must be increased always
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
            DrawRecordingIcon("folder", Left, Top, Current);

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
    } else {                // flatPlus short
        if (IsRecording) {  // Recording
            DrawRecordingIcon("recording", Left, Top, Current);

            int ImagesWidth {ImgRecNewWidth + ImgRecCutWidth + m_MarginItem2 + m_WidthScrollBar};
            if (m_IsScrolling) ImagesWidth -= m_WidthScrollBar;

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
            Buffer = cString::sprintf("%s  %s  Σ%s ", *ShortDateString(Recording->Start()),
                                      *TimeString(Recording->Start()), *Length);

            Top += m_FontHeight;
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml,
                                 m_MenuItemWidth - Left - m_MarginItem);

            Left = m_MenuItemWidth - ImagesWidth;
            Top -= m_FontHeight;
            // Show if recording is still in progress (ruTimer), or played (ruReplay)
            DrawRecordingStateIcon(Recording, Left, Top, Current);
#if APIVERSNUM >= 20505
            if (Config.MenuItemRecordingShowRecordingErrors) DrawRecordingErrorIcon(Recording, Left, Top, Current);
#endif

            Left += ImgRecNewWidth + m_MarginItem;
            if (Recording->IsEdited() && ImgRecCut) {
                const int ImageTop {Top + m_FontHeight - ImgRecCutHeight};  // 2/3 image height
                MenuIconsPixmap->DrawImage(cPoint(Left, ImageTop), *ImgRecCut);
            }

            if (Config.MenuItemRecordingShowFormatIcons)
                DrawRecordingFormatIcon(Recording, Left, Top);  // Show (SD), HD or UHD Logo

            Left += ImgRecCutWidth + (ImgRecCutWidth / 2) + m_MarginItem;  // Ensures exact 100% increase

        } else if (Total > 0) {  // Folder with recordings
            DrawRecordingIcon("folder", Left, Top, Current);

            if (Current && m_Font->Width(*RecName) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    *RecName, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                    ColorFg, clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), *RecName, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }

            Top += m_FontHeight;
            const int DigitsMaxWidth {FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "0000") +
                                      m_MarginItem};  // Use same width for recs and new recs
            Buffer = itoa(Total);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml, DigitsMaxWidth,
                                 m_FontSmlHeight, taRight);
            Left += DigitsMaxWidth;

            if (ImgRecNew) MenuIconsPixmap->DrawImage(cPoint(Left, Top), *ImgRecNew);

            Left += ImgRecNewWidth + m_MarginItem;
            Buffer = itoa(New);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml, DigitsMaxWidth,
                                 m_FontSmlHeight);
            Left += DigitsMaxWidth;

            if (Config.MenuItemRecordingShowFolderDate > 0) {
                Buffer = *ShortDateString(GetLastRecTimeFromFolder(Recording, Level));
                MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorExtraTextFg, ColorBg, m_FontSml);
                if (IsRecordingOld(Recording, Level)) {
                    Left += FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "00.00.00");
                    DrawRecordingIcon("recording_old", Left, Top, Current);
                }
            }
        } else if (Total == -1) {  // Folder without recordings
            DrawRecordingIcon("folder", Left, Top, Current);

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

    sDecorBorder ib {.Left = Config.decorBorderMenuItemSize,
                     .Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 +
                            Config.decorBorderMenuItemSize + y,
                     .Width = m_MenuItemWidth,
                     .Height = Height,
                     .Size = Config.decorBorderMenuItemSize,
                     .Type = Config.decorBorderMenuItemType};

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

    if (!m_IsScrolling) ItemBorderInsertUnique(ib);

    if (Config.MenuRecordingView == 3 && Current) DrawItemExtraRecording(Recording, tr("no recording info"));

    return true;
}

/**
 * Draws FSK and genre icons on the content header.
 *
 * This function attempts to load and draw the FSK (Freiwillige Selbstkontrolle) icon specified
 * by the provided `Fsk` string, followed by genre icons listed in the `GenreIcons` vector.
 * If the specified FSK or genre icon cannot be found, a default 'unknown' icon is used instead.
 *
 * @param Fsk           A string specifying the FSK icon to be loaded and drawn.
 * @param GenreIcons    A vector of genre icon names to be loaded and drawn. The vector is sorted
 *                      and duplicates are removed before drawing.
 */
int cFlatDisplayMenu::DrawContentHeadFskGenre(const cString &Fsk, std::vector<std::string> &GenreIcons) {
    const int IconHeight = (m_chHeight - m_MarginItem2) * Config.EpgFskGenreIconSize * 100.0;  // Narrowing conversion
    const int HeadIconTop {m_chHeight - IconHeight - m_MarginItem};  // Position for fsk/genre image
    int HeadIconLeft {m_chWidth - IconHeight - m_MarginItem};

    cString IconName {""};
    cImage *img {nullptr};
    if (!isempty(*Fsk)) {
        IconName = cString::sprintf("EPGInfo/FSK/%s", *Fsk);
        img = ImgLoader.GetIcon(*IconName, IconHeight, IconHeight);
        if (!img) img = ImgLoader.GetIcon("EPGInfo/FSK/unknown", IconHeight, IconHeight);

        if (img) {  // Draw the FSK icon
            ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
            HeadIconLeft -= IconHeight + m_MarginItem;
        }
    }
    if (GenreIcons.size() > 1) {  // Only bother to sort/unique if really needed
        std::sort(GenreIcons.begin(), GenreIcons.end());
        GenreIcons.erase(unique(GenreIcons.begin(), GenreIcons.end()), GenreIcons.end());
    }
    bool IsUnknownDrawn {false};
    for (const auto &GenreIcon : GenreIcons) {
        IconName = cString::sprintf("EPGInfo/Genre/%s", GenreIcon.c_str());
        img = ImgLoader.GetIcon(*IconName, IconHeight, IconHeight);
        if (!img && !IsUnknownDrawn) {
            img = ImgLoader.GetIcon("EPGInfo/Genre/unknown", IconHeight, IconHeight);
            IsUnknownDrawn = true;
        }
        if (img) {  // Draw the genre icon
            ContentHeadIconsPixmap->DrawImage(cPoint(HeadIconLeft, HeadIconTop), *img);
            HeadIconLeft -= IconHeight + m_MarginItem;
        }
    }
    GenreIcons.clear();  // Clear the vector after use to free memory

    return HeadIconLeft;
}

/**
 * Add extra information to the ComplexContent.
 *
 * @param Title The title of the extra information block.
 * @param Text The text to display in the extra information block.
 * @param ComplexContent The ComplexContent to add the extra information to.
 * @param ContentTop The top position of the extra information block.
 * @param IsEvent true if this block is for an event, false if it is for a recording.
 */
void cFlatDisplayMenu::AddExtraInfo(const char *Title, const cString &Text, cComplexContent &ComplexContent,
                                    int &ContentTop, bool IsEvent) {
    const int kTitleLeftMargin {m_MarginItem * 10};
    const tColor ColorMenuBg {Theme.Color(IsEvent ? clrMenuEventBg : clrMenuRecBg)};
    const tColor ColorMenuFontTitle {Theme.Color(IsEvent ? clrMenuEventFontTitle : clrMenuRecFontTitle)};
    const tColor ColorTitleLine {Theme.Color(IsEvent ? clrMenuEventTitleLine : clrMenuRecTitleLine)};
    const tColor ColorMenuFontInfo {Theme.Color(IsEvent ? clrMenuEventFontInfo : clrMenuRecFontInfo)};
    ContentTop = ComplexContent.BottomContent() + m_FontHeight;
    ComplexContent.AddText(Title, false, cRect(kTitleLeftMargin, ContentTop, 0, 0), ColorMenuFontTitle, ColorMenuBg,
                           m_Font);
    ContentTop += m_FontHeight;
    ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, m_LineWidth), ColorTitleLine);
    ContentTop += m_LineMargin;
    if (Text[0] != '\0')
        ComplexContent.AddText(*Text, true,
                               cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                               ColorMenuFontInfo, ColorMenuBg, m_FontMedium);
}

void cFlatDisplayMenu::SetEvent(const cEvent *Event) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetEvent()");
#endif

    // All work is done in DrawEventInfo() to avoid 'invalid lock sequence' in VDR.
    // Here we just save the cEvent object for later use.
    m_Event = Event;
    m_ShowEvent = true;
    m_ShowRecording = false;
    m_ShowText = false;
}

void cFlatDisplayMenu::DrawEventInfo(const cEvent *Event) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawEventInfo()");
#endif

    if (!ContentHeadIconsPixmap || !ContentHeadPixmap || !Event) return;

#ifdef DEBUGEPGTIME
    cTimeMs Timer;  // Set Timer
#endif

    ItemBorderClear();
    PixmapClear(ContentHeadIconsPixmap);

    m_cLeft = Config.decorBorderMenuContentSize;
    m_cTop = m_chTop + m_MarginItem3 + m_FontHeight + m_FontSmlHeight * 2 + Config.decorBorderMenuContentSize +
             Config.decorBorderMenuContentHeadSize;
    m_cWidth = m_MenuWidth - Config.decorBorderMenuContentSize * 2;
    m_cHeight = m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                               Config.decorBorderButtonSize * 2 + m_MarginItem3 + m_chHeight +
                               Config.decorBorderMenuContentHeadSize * 2 + Config.decorBorderMenuContentSize * 2);

    if (!ButtonsDrawn()) m_cHeight += m_ButtonsHeight + Config.decorBorderButtonSize * 2;

    m_MenuItemWidth = m_cWidth;

    cString Fsk {""}, Text {""}, TextAdditional {""};
    std::vector<std::string> GenreIcons;
    GenreIcons.reserve(8);

    // Description or ShortText if Description is empty
    if (isempty(Event->Description())) {
        if (!isempty(Event->ShortText())) Text.Append(Event->ShortText());
    } else {
        Text.Append(Event->Description());
    }

    if (Config.EpgAdditionalInfoShow) {
        if (Text[0] != '\0') Text.Append("\n");
        // Genre
        InsertGenreInfo(Event, Text, GenreIcons);  // Add genre info

        // FSK
        if (Event->ParentalRating()) {
            Fsk = *Event->GetParentalRatingString();
            Text.Append(cString::sprintf("\n%s: %s", tr("FSK"), *Fsk));
        }

        const cComponents *Components {Event->Components()};
        if (Components) {
            cString Audio {""}, Subtitle {""};
            InsertComponents(Components, TextAdditional, Audio, Subtitle);  // Get info for audio/video and subtitle

            if (Audio[0] != '\0') {
                if (TextAdditional[0] != '\0') TextAdditional.Append("\n");
                TextAdditional.Append(cString::sprintf("%s: %s", tr("Audio"), *Audio));
            }
            if (Subtitle[0] != '\0') {
                if (TextAdditional[0] != '\0') TextAdditional.Append("\n");
                TextAdditional.Append(cString::sprintf("\n%s: %s", tr("Subtitle"), *Subtitle));
            }
        }  // if Components
    }  // EpgAdditionalInfoShow

    const int HeadIconLeft = DrawContentHeadFskGenre(Fsk, GenreIcons);

#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: DrawEventInfo() Infotext time @ %ld ms", Timer.Elapsed());
#endif

    cString Reruns {""};
    if (Config.EpgRerunsShow) {
        // Lent from nopacity
        cPlugin *pEpgSearchPlugin {cPluginSkinFlatPlus::GetEpgSearchPlugin()};
        cString SearchTerm {Event->Title()};  // Search term
        if (pEpgSearchPlugin && !isempty(Event->Title())) {
            Epgsearch_searchresults_v1_0 data {
                .query = const_cast<char *>(static_cast<const char *>(SearchTerm)),
                .mode = 0,               // Search mode (0=phrase, 1=and, 2=or, 3=regular expression)
                .channelNr = 0,          // Channel number to search in (0=any)
                .useTitle = true,        // Search in title
                .useSubTitle = false,    // Search in subtitle
                .useDescription = false  // Search in description
            };

            if (pEpgSearchPlugin->Service("Epgsearch-searchresults-v1.0", &data)) {
                cList<Epgsearch_searchresults_v1_0::cServiceSearchResult> *list = data.pResultList;
                if (list && (list->Count() > 1)) {
                    int i {0};
                    const tChannelID ChannelID {Event->ChannelID()};
                    const time_t StartTime {Event->StartTime()};
                    for (Epgsearch_searchresults_v1_0::cServiceSearchResult *r = list->First(); r && i < 5;
                         r = list->Next(r)) {
                        if ((ChannelID == r->event->ChannelID()) && (StartTime == r->event->StartTime())) continue;
                        ++i;
                        Reruns.Append(*DayDateTime(r->event->StartTime()));
                        {
                            LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
                            const cChannel *channel {Channels->GetByChannelID(r->event->ChannelID(), true, true)};
                            if (channel)
                                Reruns.Append(
                                    cString::sprintf(", %d - %s", channel->Number(), channel->ShortName(true)));
                        }
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
    dsyslog("flatPlus: DrawEventInfo() Reruns time @ %ld ms", Timer.Elapsed());
#endif

    std::vector<cString> ActorsPath, ActorsName, ActorsRole;
    cString MediaPath {""};
    cString MovieInfo {""}, SeriesInfo {""};
    cImage *img {nullptr};
    int ContentTop {0};
    int MediaWidth {0};
    int MediaHeight {m_cHeight - m_MarginItem2 - m_FontHeight - m_LineMargin};  // Leave space for title
    bool FirstRun {true};
    bool Scrollable {false};

    // First run setup
    m_cWidth -= m_WidthScrollBar;  // Assume that we need scrollbar most of the time

    do {  // Runs up to two times!
        ComplexContent.Clear();
        ComplexContent.SetOsd(m_Osd);
        ComplexContent.SetPosition(cRect(m_cLeft, m_cTop, m_cWidth, m_cHeight));
        ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));
        ComplexContent.SetScrollSize(m_FontHeight);
        ComplexContent.SetScrollingActive(true);

        // Call scraper plugin only at first run and reuse data at second run
        if (FirstRun && (Config.TVScraperEPGInfoShowPoster || Config.TVScraperEPGInfoShowActors)) {
            GetScraperMedia(MediaPath, SeriesInfo, MovieInfo, ActorsPath, ActorsName, ActorsRole, Event);
        }
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: DrawEventInfo() TVScraper time @ %ld ms", Timer.Elapsed());
#endif
        const int kTitleLeftMargin {m_MarginItem * 10};
        ContentTop = m_MarginItem;

        // Add description header if needed
        if ((Text[0] != '\0') || (MediaPath[0] != '\0')) {  // Insert description line
            ComplexContent.AddText(tr("Description"), false, cRect(kTitleLeftMargin, ContentTop, 0, 0),
                                   Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, m_LineWidth), Theme.Color(clrMenuEventTitleLine));
            ContentTop += m_LineMargin;
        }

        // Handle media content
        MediaWidth = m_cWidth / 2 - m_MarginItem2;
        if (MediaPath[0] != '\0') {
            img = ImgLoader.GetFile(*MediaPath, MediaWidth, MediaHeight);
            if (img) {  // Insert image with floating text
                ComplexContent.AddImageWithFloatedText(
                    img, CIP_Right, *Text, m_MarginItem,
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
            } else if (Text[0] != '\0') {  // No image; insert text
                ComplexContent.AddText(
                    *Text, true, cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
            }
        } else if (Text[0] != '\0') {  // No image; insert text
            ComplexContent.AddText(*Text, true,
                                   cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                                   Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
        }

        // Add movie information if available
        if (MovieInfo[0] != '\0') AddExtraInfo(tr("Movie information"), MovieInfo, ComplexContent, ContentTop, true);

        // Add series information if available
        if (SeriesInfo[0] != '\0') AddExtraInfo(tr("Series information"), SeriesInfo, ComplexContent, ContentTop, true);
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: DrawEventInfo() Epgtext time @ %ld ms", Timer.Elapsed());
#endif

        // Add actors if available
        const int NumActors = ActorsPath.size();  // Narrowing conversion
        if (Config.TVScraperEPGInfoShowActors && NumActors > 0)
            AddActors(ComplexContent, ActorsPath, ActorsName, ActorsRole, NumActors, true);
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: DrawEventInfo() Actor time @ %ld ms", Timer.Elapsed());
#endif

        // Add reruns information if available
        if (Reruns[0] != '\0') AddExtraInfo(tr("Reruns"), Reruns, ComplexContent, ContentTop, true);

        // Add video information if available
        if (TextAdditional[0] != '\0')
            AddExtraInfo(tr("Video information"), TextAdditional, ComplexContent, ContentTop, true);

        Scrollable = ComplexContent.Scrollable(m_cHeight - m_MarginItem2);

        // Determine if we need a second run
        if (Scrollable || !FirstRun) break;  // No need for another run

        // Prepare for second run
        FirstRun = false;
        m_cWidth += m_WidthScrollBar;  // For second run readd scrollbar width
    } while (true);  // Loop will break internally

    ComplexContent.CreatePixmaps(Config.MenuContentFullSize || Scrollable);
    ComplexContent.Draw();
#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: DrawEventInfo() Actor time @ %ld ms", Timer.Elapsed());
#endif

    PixmapClear(ContentHeadPixmap);
    ContentHeadPixmap->DrawRectangle(cRect(0, 0, m_MenuWidth, m_chHeight), Theme.Color(clrScrollbarBg));

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
            const int DotsWidth {FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "...")};
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

    const sDecorBorder ib {m_chLeft,
                           m_chTop,
                           m_chWidth,
                           m_chHeight,
                           Config.decorBorderMenuContentHeadSize,
                           Config.decorBorderMenuContentHeadType,
                           Config.decorBorderMenuContentHeadFg,
                           Config.decorBorderMenuContentHeadBg};
    DecorBorderDraw(ib);

    if (Scrollable) {
        const int ScrollOffset {ComplexContent.ScrollOffset()};
        DrawScrollbar(ComplexContent.ScrollTotal(), ScrollOffset, ComplexContent.ScrollShown(),
                      ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ScrollOffset > 0,
                      ScrollOffset + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);
    }

    const sDecorBorder ContentBorder {m_cLeft,
                                      m_cTop,
                                      m_cWidth,
                                      (Config.MenuContentFullSize || Scrollable) ? ComplexContent.ContentHeight(true)
                                                                                 : ComplexContent.ContentHeight(false),
                                      Config.decorBorderMenuContentSize,
                                      Config.decorBorderMenuContentType,
                                      Config.decorBorderMenuContentFg,
                                      Config.decorBorderMenuContentBg};
    DecorBorderDraw(ContentBorder);

    m_EventInfoDrawn = true;  // Set flag that event info is drawn
#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: DrawEventInfo() Total time: %ld ms", Timer.Elapsed());
#endif
}

void cFlatDisplayMenu::DrawItemExtraRecording(const cRecording *Recording, const cString EmptyText) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawItemExtraRecording()");
#endif

    m_cLeft = m_MenuItemWidth + Config.decorBorderMenuItemSize * 2 + Config.decorBorderMenuContentSize + m_MarginItem;
    if (m_IsScrolling) m_cLeft += m_WidthScrollBar;

    m_cTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
    m_cWidth = m_MenuWidth - m_cLeft - Config.decorBorderMenuContentSize;
    m_cHeight =
        m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                       Config.decorBorderButtonSize * 2 + m_MarginItem3 + Config.decorBorderMenuContentSize * 2);

    cString Text {""};
    if (Recording) {
        const cRecordingInfo *RecInfo {Recording->Info()};

        // Description or ShortText if Description is empty
        if (isempty(RecInfo->Description())) {
            if (!isempty(RecInfo->ShortText())) Text.Append(RecInfo->ShortText());
        } else {
            Text.Append(RecInfo->Description());
        }

        // Lent from skinelchi
        if (Config.RecordingAdditionalInfoShow) {
            if (Text[0] != '\0') Text.Append("\n\n");
            {
                LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
                const cChannel *channel {Channels->GetByChannelID(RecInfo->ChannelID())};
                if (channel)
                    Text.Append(
                        cString::sprintf("%s: %d - %s\n", trVDR("Channel"), channel->Number(), channel->Name()));
            }
            const cEvent *Event {RecInfo->GetEvent()};
            if (Event) {
                InsertGenreInfo(Event, Text);  // Add genre info

                if (Event->Contents(0)) Text.Append("\n");

                if (Event->ParentalRating())  // FSK
                    Text.Append(cString::sprintf("%s: %s\n", tr("FSK"), *Event->GetParentalRatingString()));
            }

            InsertCutLengthSize(Recording, Text);  // Process marks and insert text

#if APIVERSNUM >= 20505
            InsertTSErrors(RecInfo, Text);  // Add TS Error information
#endif

            const cComponents *Components {RecInfo->Components()};
            if (Components) {
                cString Audio {""}, Subtitle {""};
                InsertComponents(Components, Text, Audio, Subtitle, true);  // Get info for audio/video and subtitle

                if (Audio[0] != '\0') Text.Append(cString::sprintf("\n%s: %s", tr("Audio"), *Audio));
                if (Subtitle[0] != '\0') Text.Append(cString::sprintf("\n%s: %s", tr("Subtitle"), *Subtitle));
            }
            if (RecInfo->Aux()) InsertAuxInfos(RecInfo, Text, true);  // Insert aux infos with info line
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

    cString MediaPath {""};
    int MediaWidth {0}, MediaHeight {m_cHeight - m_MarginItem2};  // Use content height
    int MediaType {0};                                            // 0 = None, 1 = Series, 2 = Movie
    cImage *img {nullptr};
    if (Config.TVScraperRecInfoShowPoster) {
        cSize Dummy {0, 0};
        MediaType = GetScraperMediaTypeSize(MediaPath, Dummy, nullptr, Recording);
        if (MediaType == 1)
            MediaWidth = m_cWidth - m_MarginItem2;
        else if (MediaType == 2)
            MediaWidth = m_cWidth / 2 - m_MarginItem3;

        if (MediaPath[0] == '\0' && Config.TVScraperSearchLocalPosters) {  // Prio for tvscraper poster
            const cString RecPath = Recording->FileName();
            if (ImgLoader.SearchRecordingPoster(RecPath, MediaPath)) {
                img = ImgLoader.GetFile(*MediaPath, m_cWidth - m_MarginItem2, MediaHeight);
                if (img) {
                    const uint16_t Aspect = img->Width() / img->Height();  // Narrowing conversion
                    if (Aspect < 1) {                                      //* Poster (For example 680x1000 = 0.68)
                        MediaWidth = m_cWidth / 2 - m_MarginItem3;
                        MediaType = 2;
                    } else {  //* Portrait (For example 1920x1080 = 1.77); Banner (Usually 758x140 = 5.41)
                        MediaWidth = m_cWidth - m_MarginItem2;
                        MediaType = 1;
                    }
                }
            }
        }
    }  // TVScraperRecInfoShowPoster

    if (MediaPath[0] != '\0') {
        img = ImgLoader.GetFile(*MediaPath, MediaWidth, MediaHeight);
        if (img && MediaType == 2) {  // Movie
            ComplexContent.AddImageWithFloatedText(
                img, CIP_Right, *Text, m_MarginItem,
                cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
        } else if (img && MediaType == 1) {  // Series
            ComplexContent.AddImage(img, cRect(m_MarginItem, m_MarginItem, img->Width(), img->Height()));
            ComplexContent.AddText(
                *Text, true,
                cRect(m_MarginItem, m_MarginItem + img->Height(), m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
        } else {
            ComplexContent.AddText(
                *Text, true, cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
        }
    } else {
        ComplexContent.AddText(*Text, true,
                               cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                               Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
    }

    ComplexContent.CreatePixmaps(Config.MenuContentFullSize);
    ComplexContent.Draw();

    DecorBorderClearByFrom(BorderContent);
    const sDecorBorder ib {m_cLeft,
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

/**
 * Add actors' images and names to the recording info display
 *
 * The method takes the content object, the paths to the actors' images,
 * the actors' names, the actors' roles, and the number of actors as parameters.
 *
 * @param ComplexContent Complex content to add actors to.
 * @param ActorsPath Path of actor images.
 * @param ActorsName Vector of actor names.
 * @param ActorsRole Vector of actor roles.
 * @param NumActors Number of actors to add.
 * @param IsEvent Is this for an event or a recording?
 */
void cFlatDisplayMenu::AddActors(cComplexContent &ComplexContent, std::vector<cString> &ActorsPath,
                                 std::vector<cString> &ActorsName, std::vector<cString> &ActorsRole, int NumActors,
                                 bool IsEvent) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::AddActors()");
#endif

    // Setting for TVScraperEPGInfoShowActors and TVScraperRecInfoShowActors
    const int ShowMaxActors {Config.TVScraperShowMaxActors};
    if (ShowMaxActors == 0) return;  // Do not show actors
    if (ShowMaxActors > 0 && ShowMaxActors < NumActors)
        NumActors = ShowMaxActors;  // Limit to ShowMaxActors (-1 = Show all actors)

    int ContentTop {0};  // Calculated by 'AddExtraInfo()'
    AddExtraInfo(tr("Actors"), "", ComplexContent, ContentTop, IsEvent);

    const tColor ColorMenuBg {Theme.Color(IsEvent ? clrMenuEventBg : clrMenuRecBg)};
    const tColor ColorMenuFontInfo {Theme.Color(IsEvent ? clrMenuEventFontInfo : clrMenuRecFontInfo)};

    cImage *img {nullptr};
    cString Role {""};  // Actor role
    int Actor {0};
    const uint16_t ActorsPerLine {6};                                    // TODO: Config option?
    const int ActorWidth = m_cWidth / ActorsPerLine - m_MarginItem2;  // Narrowing conversion
    const int ActorMargin = ((m_cWidth - m_MarginItem2) - ActorWidth * ActorsPerLine) / (ActorsPerLine - 1);
    const uint16_t PicLines = NumActors / ActorsPerLine + (NumActors % ActorsPerLine != 0);  // Number of lines needed
    int ImgHeight {0}, MaxImgHeight {0};
    int x {m_MarginItem};
    int y {ContentTop};
#ifdef DEBUGFUNCSCALL
    int y2 {ContentTop};  //! y2 is for testing
    dsyslog("   ActorWidth/ActorMargin: %d/%d", ActorWidth, ActorMargin);
#endif
    if (NumActors > 48) isyslog("flatPlus: First display of %d actor images will probably be slow!", NumActors);

    for (std::size_t row {0}; row < PicLines; ++row) {
        for (std::size_t col {0}; col < ActorsPerLine; ++col) {
            if (Actor == NumActors) break;
            img = ImgLoader.GetFile(*ActorsPath[Actor], ActorWidth, kIconMaxSize);
            if (img) {
                ComplexContent.AddImage(img, cRect(x, y, 0, 0));
                ImgHeight = img->Height();
                ComplexContent.AddText(*ActorsName[Actor], false, cRect(x, y + ImgHeight + m_MarginItem, ActorWidth, 0),
                                       ColorMenuFontInfo, ColorMenuBg, m_FontTiny, ActorWidth, m_FontTinyHeight,
                                       taCenter);
                Role = cString::sprintf("\"%s\"", *ActorsRole[Actor]);
                ComplexContent.AddText(
                    *Role, false, cRect(x, y + ImgHeight + m_MarginItem + m_FontTinyHeight, ActorWidth, 0),
                    ColorMenuFontInfo, ColorMenuBg, m_FontTiny, ActorWidth, m_FontTinyHeight, taCenter);
#ifdef DEBUGFUNCSCALL
                if (ImgHeight > MaxImgHeight) { dsyslog("   Column %ld: MaxImgHeight changed to %d", col, ImgHeight); }
#endif
                MaxImgHeight = std::max(MaxImgHeight, ImgHeight);  // In case images have different size
            }
            x += ActorWidth + ActorMargin;
            ++Actor;
        }  // for col
        x = m_MarginItem;
        // y = ComplexContent.BottomContent() + m_FontHeight;
        y += MaxImgHeight + m_MarginItem + (m_FontTinyHeight * 2) + m_FontHeight;
#ifdef DEBUGFUNCSCALL
        // Alternate way to get y
        y2 += MaxImgHeight + m_MarginItem + (m_FontTinyHeight * 2) + m_FontHeight;
        dsyslog("   y/y2 BottomContent()/Calculation: %d/%d", ComplexContent.BottomContent() + m_FontHeight, y2);
#endif
        MaxImgHeight = 0;  // Reset value for next row
    }  // for row
}

/**
 * @brief Add the number of TS errors to the recording info display
 *
 * Add the number of TS errors to the recording info display.
 * The method takes the recording information object and the text to add the errors to as parameters.
 * If the recording information object is valid and the number of TS errors is greater than 0,
 * the method adds a line to the text with the number of TS errors.
 * The line is formatted as "TS errors: <number>".
 */
void cFlatDisplayMenu::InsertTSErrors(const cRecordingInfo *RecInfo, cString &Text) const {  // NOLINT
    // From SkinNopacity
    if (RecInfo && RecInfo->Errors() > 0) {
        std::ostringstream RecErrors {""};
        RecErrors.imbue(std::locale {""});  // Set to local locale
        RecErrors << RecInfo->Errors();
        Text.Append(cString::sprintf("\n%s: %s", tr("TS errors"), RecErrors.str().c_str()));
    }
}

void cFlatDisplayMenu::SetRecording(const cRecording *Recording) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::SetRecording()");
#endif

    // All the work is done in DrawRecordingInfo() to prevent 'invalid lock sequence' in VDR.
    // Here we only save the recording object for later use.
    m_Recording = Recording;
    m_ShowEvent = false;
    m_ShowRecording = true;
    m_ShowText = false;
}

void cFlatDisplayMenu::DrawRecordingInfo(const cRecording *Recording) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawRecordingInfo()");
    cTimeMs Timer;  // Set Timer
#endif

    if (!ContentHeadPixmap || !ContentHeadIconsPixmap || !Recording) return;

    ItemBorderClear();
    PixmapClear(ContentHeadIconsPixmap);

    m_cLeft = Config.decorBorderMenuContentSize;
    m_cTop = m_chTop + m_MarginItem3 + m_FontHeight + m_FontSmlHeight * 2 + Config.decorBorderMenuContentSize +
             Config.decorBorderMenuContentHeadSize;
    m_cWidth = m_MenuWidth - Config.decorBorderMenuContentSize * 2;
    m_cHeight = m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                               Config.decorBorderButtonSize * 2 + m_MarginItem3 + m_chHeight +
                               Config.decorBorderMenuContentHeadSize * 2 + Config.decorBorderMenuContentSize * 2);

    if (!ButtonsDrawn()) m_cHeight += m_ButtonsHeight + Config.decorBorderButtonSize * 2;

    m_MenuItemWidth = m_cWidth;

    cString RecAdditional {""}, Text {""}, TextAdditional {""};
    const cRecordingInfo *RecInfo {Recording->Info()};
    std::vector<std::string> GenreIcons;
    GenreIcons.reserve(8);

    // Description or ShortText if Description is empty
    if (isempty(RecInfo->Description())) {
        if (!isempty(RecInfo->ShortText())) Text.Append(RecInfo->ShortText());
    } else {
        Text.Append(RecInfo->Description());
    }

    cString Fsk {""};
    // Lent from skinelchi
    if (Config.RecordingAdditionalInfoShow) {
        if (Text[0] != '\0') Text.Append("\n");
        const cEvent *Event {RecInfo->GetEvent()};
        if (Event) {
            // Genre
            InsertGenreInfo(Event, Text, GenreIcons);  // Add genre info

            if (Event->Contents(0)) Text.Append("\n");
            // FSK
            if (Event->ParentalRating()) {
                Fsk = *Event->GetParentalRatingString();
                Text.Append(cString::sprintf("%s: %s\n", tr("FSK"), *Fsk));
            }
        }
#ifdef DEBUGFUNCSCALL
        dsyslog("   RecordingAdditionalInfoShow() GetByChannelID() @ %ld ms", Timer.Elapsed());
#endif

        {
            LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
            const cChannel *Channel {Channels->GetByChannelID(RecInfo->ChannelID())};
            if (Channel)
                RecAdditional.Append(
                    cString::sprintf("%s: %d - %s\n", trVDR("Channel"), Channel->Number(), Channel->Name()));
        }

#ifdef DEBUGFUNCSCALL
        dsyslog("   RecordingAdditionalInfoShow() GetByChannelID() done @ %ld ms", Timer.Elapsed());
#endif

        InsertCutLengthSize(Recording, RecAdditional);  // Process marks and insert text

#if APIVERSNUM >= 20505
        // Add TS Error information
        InsertTSErrors(RecInfo, RecAdditional);
#endif

        const cComponents *Components {RecInfo->Components()};
        if (Components) {
            cString Audio {""}, Subtitle {""};
            InsertComponents(Components, TextAdditional, Audio, Subtitle);  // Get info for audio/video and subtitle

            if (Audio[0] != '\0') {
                if (TextAdditional[0] != '\0') TextAdditional.Append("\n");
                TextAdditional.Append(cString::sprintf("%s: %s", tr("Audio"), *Audio));
            }
            if (Subtitle[0] != '\0') {
                if (TextAdditional[0] != '\0') TextAdditional.Append("\n");
                TextAdditional.Append(cString::sprintf("\n%s: %s", tr("Subtitle"), *Subtitle));
            }
        }
        if (RecInfo->Aux()) InsertAuxInfos(RecInfo, RecAdditional, false);  // Insert aux infos without info line
    }  // if Config.RecordingAdditionalInfoShow

    const int HeadIconLeft = DrawContentHeadFskGenre(Fsk, GenreIcons);

#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: DrawRecordingInfo() Infotext time @ %ld ms", Timer.Elapsed());
#endif

    std::vector<cString> ActorsPath, ActorsName, ActorsRole;
    cString MediaPath {""};
    cString MovieInfo {""}, SeriesInfo {""};
    cImage *img {nullptr};
    int ContentTop {0};
    int MediaWidth {0};
    int MediaHeight {m_cHeight - m_MarginItem2 - m_FontHeight - m_LineMargin};  // Leave space for title
    bool FirstRun {true};
    bool Scrollable {false};

    // First run setup
    m_cWidth -= m_WidthScrollBar;  // Assume that we need scrollbar most of the time

    do {  // Runs up to two times!
        ComplexContent.Clear();
        ComplexContent.SetOsd(m_Osd);
        ComplexContent.SetPosition(cRect(m_cLeft, m_cTop, m_cWidth, m_cHeight));
        ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));
        ComplexContent.SetScrollSize(m_FontHeight);
        ComplexContent.SetScrollingActive(true);

        // Call scraper plugin only at first run and reuse data at second run
        if (FirstRun && (Config.TVScraperRecInfoShowPoster || Config.TVScraperRecInfoShowActors)) {
            GetScraperMedia(MediaPath, SeriesInfo, MovieInfo, ActorsPath, ActorsName, ActorsRole, nullptr, Recording);
            if (MediaPath[0] == '\0' && Config.TVScraperSearchLocalPosters) {  // Prio for tvscraper poster
                const cString RecPath = Recording->FileName();
                ImgLoader.SearchRecordingPoster(RecPath, MediaPath);
            }
        }
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: DrawRecordingInfo() TVSscraper time @ %ld ms", Timer.Elapsed());
#endif
        const int kTitleLeftMargin {m_MarginItem * 10};
        MediaWidth = m_cWidth / 2 - m_MarginItem2;
        ContentTop = m_MarginItem;

        // Add description header if needed
        if ((Text[0] != '\0') || (MediaPath[0] != '\0')) {  // Insert description line
            ComplexContent.AddText(tr("Description"), false, cRect(kTitleLeftMargin, ContentTop, 0, 0),
                                   Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), m_Font);
            ContentTop += m_FontHeight;
            ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, m_LineWidth), Theme.Color(clrMenuRecTitleLine));
            ContentTop += m_LineMargin;
        }

        // Handle media content
        if (MediaPath[0] != '\0') {
            img = ImgLoader.GetFile(*MediaPath, MediaWidth, MediaHeight);
            //* Make portrait smaller than poster or banner to prevent wasting of space
            if (img) {
                const uint16_t Aspect = img->Width() / img->Height();  // Narrowing conversion
                if (Aspect > 1 && Aspect < 4) {                        //* Portrait (For example 1920x1080)
                    // dsyslog("flatPlus: SetRecording() Portrait image %dx%d (%d) found! Setting to 2/3 size.",
                    //         img->Width(), img->Height(), Aspect);
                    MediaHeight *= (2.0 / 3.0);                                    // Size * 0,666 = 2/3
                    img = ImgLoader.GetFile(*MediaPath, MediaWidth, MediaHeight);  // Reload portrait with new size
                }
            }
            if (img) {  // Insert image with floating text
                ComplexContent.AddImageWithFloatedText(
                    img, CIP_Right, *Text, m_MarginItem,
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
            } else if (Text[0] != '\0') {  // No image; insert text
                ComplexContent.AddText(
                    *Text, true, cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                    Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
            }
        } else if (Text[0] != '\0') {  // No image; insert text
            ComplexContent.AddText(*Text, true,
                                   cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem2, m_cHeight - m_MarginItem2),
                                   Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
        }

        // Add movie information if available
        if (MovieInfo[0] != '\0') AddExtraInfo(tr("Movie information"), MovieInfo, ComplexContent, ContentTop);

        // Add series information if available
        if (SeriesInfo[0] != '\0') AddExtraInfo(tr("Series information"), SeriesInfo, ComplexContent, ContentTop);
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: DrawRecordingInfo() Epgtext time @ %ld ms", Timer.Elapsed());
#endif

        // Add actors if available
        const int NumActors = ActorsPath.size();  // Narrowing conversion
        if (Config.TVScraperRecInfoShowActors && NumActors > 0)
            AddActors(ComplexContent, ActorsPath, ActorsName, ActorsRole, NumActors);
#ifdef DEBUGEPGTIME
        dsyslog("flatPlus: DrawRecordingInfo() Actor time @ %ld ms", Timer.Elapsed());
#endif

        // Add recording information if available
        if (RecAdditional[0] != '\0')
            AddExtraInfo(tr("Recording information"), RecAdditional, ComplexContent, ContentTop);

        // Add video information if available
        if (TextAdditional[0] != '\0')
            AddExtraInfo(tr("Video information"), TextAdditional, ComplexContent, ContentTop);

        Scrollable = ComplexContent.Scrollable(m_cHeight - m_MarginItem2);

        // Determine if we need a second run
        if (Scrollable || !FirstRun) break;  // No need for another run

        // Prepare for second run
        FirstRun = false;
        m_cWidth += m_WidthScrollBar;  // For second run readd scrollbar width
    } while (true);  // Loop will break internally

    ComplexContent.CreatePixmaps(Config.MenuContentFullSize || Scrollable);
    ComplexContent.Draw();
#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: DrawRecordingInfo() Complexcontent draw time @ %ld ms", Timer.Elapsed());
#endif

    PixmapClear(ContentHeadPixmap);
    ContentHeadPixmap->DrawRectangle(cRect(0, 0, m_MenuWidth, m_chHeight), Theme.Color(clrScrollbarBg));

    const cString StrTime =
        cString::sprintf("%s  %s  %s", *DateString(Recording->Start()), *TimeString(Recording->Start()),
                         (RecInfo->ChannelName()) ? RecInfo->ChannelName() : "");

    cString Title = RecInfo->Title();
    if (isempty(*Title)) Title = Recording->Name();

    const cString ShortText = (RecInfo->ShortText() ? RecInfo->ShortText() : " - ");  // No short text. Show ' - '
    const int ShortTextWidth {m_FontSml->Width(*ShortText)};
    int MaxWidth {HeadIconLeft - m_MarginItem};
    int left {m_MarginItem};

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors) MaxWidth -= m_FontSmlHeight;  // Substract width of imgRecErr
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
            const int DotsWidth {FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "...")};
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

        img = ImgLoader.GetIcon(*RecErrIcon, kIconMaxSize, m_FontSmlHeight);  // Small image
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

    const sDecorBorder RecordingBorder {m_cLeft,
                                        m_cTop,
                                        m_cWidth,
                                        (Config.MenuContentFullSize || Scrollable)
                                            ? ComplexContent.ContentHeight(true)
                                            : ComplexContent.ContentHeight(false),
                                        Config.decorBorderMenuContentSize,
                                        Config.decorBorderMenuContentType,
                                        Config.decorBorderMenuContentFg,
                                        Config.decorBorderMenuContentBg,
                                        BorderMenuRecord};
    DecorBorderDraw(RecordingBorder, false);

    m_RecordingInfoDrawn = true;  // Recording info is drawn
#ifdef DEBUGEPGTIME
    dsyslog("flatPlus: DrawRecordingInfo() Total time: %ld ms", Timer.Elapsed());
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
    PixmapClear(ContentHeadPixmap);

    const int Left {Config.decorBorderMenuContentSize};  // Config.decorBorderMenuContentSize
    const int Top {m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Left};
    int Width {m_MenuWidth - Left * 2};
    int Height {m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                               Config.decorBorderButtonSize * 2 + Left * 2 + m_MarginItem)};

    if (!ButtonsDrawn()) Height += m_ButtonsHeight + Config.decorBorderButtonSize * 2;

    ComplexContent.Clear();

    m_MenuItemWidth = Width;

    // First assume text is with scrollbar
    ComplexContentAddText(Text, FixedFont, Width - m_WidthScrollBar, Height);

    const bool Scrollable {ComplexContent.Scrollable(Height - m_MarginItem2)};
    if (!Scrollable) {
        ComplexContent.Clear();
        ComplexContentAddText(Text, FixedFont, Width, Height);
    }

    ComplexContent.SetOsd(m_Osd);
    ComplexContent.SetPosition(cRect(Left, Top, Width, Height));
    ComplexContent.SetBGColor(Theme.Color(clrMenuTextBg));
    ComplexContent.SetScrollingActive(true);
    ComplexContent.CreatePixmaps(Config.MenuContentFullSize || Scrollable);
    ComplexContent.Draw();

    if (Scrollable) {
        const int ScrollOffset {ComplexContent.ScrollOffset()};
        DrawScrollbar(ComplexContent.ScrollTotal(), ScrollOffset, ComplexContent.ScrollShown(),
                      ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ScrollOffset > 0,
                      ScrollOffset + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);
    }

    const sDecorBorder ib {.Left = Left,
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

void cFlatDisplayMenu::SetMenuSortMode(eMenuSortMode MenuSortMode) {
    // Do not set sort icon if mode is unknown
    static constexpr const char *SortIcons[] {"", "SortNumber", "SortName", "SortDate", "SortProvider"};
    if (MenuSortMode > msmUnknown && MenuSortMode <= msmProvider) TopBarSetMenuIconRight(SortIcons[MenuSortMode]);
}

void cFlatDisplayMenu::Flush() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::Flush()");
#endif

    if (Config.MenuFullOsd && !m_MenuFullOsdIsDrawn && MenuPixmap) {
        MenuPixmap->DrawRectangle(cRect(0, m_MenuItemLastHeight - Config.decorBorderMenuItemSize,
                                        m_MenuItemWidth + Config.decorBorderMenuItemSize * 2,
                                        MenuPixmap->ViewPort().Height() - m_MenuItemLastHeight + m_MarginItem),
                                  Theme.Color(clrItemSelableBg));
        // MenuPixmap->DrawRectangle(cRect(0, MenuPixmap->ViewPort().Height() - 5, m_MenuItemWidth +
        //                           Config.decorBorderMenuItemSize * 2, 5), Theme.Color(clrItemSelableBg));
        m_MenuFullOsdIsDrawn = true;
    }

    if (m_MenuCategory == mcEvent && !m_EventInfoDrawn) DrawEventInfo(m_Event);  // Draw event info

    if (m_MenuCategory == mcRecordingInfo && !m_RecordingInfoDrawn)
        DrawRecordingInfo(m_Recording);  // Draw recording info

    if (m_MenuCategory == mcTimer && Config.MenuTimerShowCount) {
        uint16_t TimerActiveCount {0}, TimerCount {0};
        UpdateTimerCounts(TimerActiveCount, TimerCount);

        if (m_LastTimerActiveCount != TimerActiveCount || m_LastTimerCount != TimerCount) {
            m_LastTimerActiveCount = TimerActiveCount;
            m_LastTimerCount = TimerCount;
            const cString NewTitle = cString::sprintf("%s (%d/%d)", *m_LastTitle, TimerActiveCount, TimerCount);
            TopBarSetTitle(*NewTitle, false);  // Do not clear
        }
    }

    if (cVideoDiskUsage::HasChanged(m_VideoDiskUsageState)) TopBarEnableDiskUsage();  // Keep 'DiskUsage' up to date

    TopBarUpdate();
    m_Osd->Flush();
}

// Insert a new sDecorBorder into ItemsBorder, or update the existing one if Left and Top already exist.
void cFlatDisplayMenu::ItemBorderInsertUnique(const sDecorBorder &ib) {
    for (auto &item : ItemsBorder) {
        if (item.Left == ib.Left && item.Top == ib.Top) {
            item = ib;
            return;
        }
    }

    ItemsBorder.emplace_back(ib);
}

void cFlatDisplayMenu::ItemBorderDrawAllWithScrollbar() {
    for (auto &Border : ItemsBorder) {
        Border.Width -= m_WidthScrollBar;
        DecorBorderDraw(Border);
        Border.Width += m_WidthScrollBar;  // Restore original width
    }
}

void cFlatDisplayMenu::ItemBorderDrawAllWithoutScrollbar() {
    for (auto &Border : ItemsBorder) {
        Border.Width += m_WidthScrollBar;
        DecorBorderDraw(Border);
        Border.Width -= m_WidthScrollBar;  // Restore original width
    }
}

void cFlatDisplayMenu::ItemBorderClear() { ItemsBorder.clear(); }

/**
 * Remove up to 4 leading digits from a menu text.
 *
 * This method is used to remove leading numbers from a menu text.
 *
 * @param Text the menu text to be processed
 * @return the processed menu text
 */
cString cFlatDisplayMenu::MainMenuText(const cString &Text) const {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::MainMenuText() '%s'", *Text);
#endif
    std::string_view text {skipspace(*Text)};
    bool found {false};
    const std::size_t TextLength {text.length()};
    std::size_t i {0};  // 'i' used also after loop
    for (; i < TextLength; ++i) {
        if (isdigit(text.at(i)) && i < 5)  // Up to 4 digits expected
            found = true;
        else
            break;
    }

    return found ? skipspace(text.substr(i).data()) : text.data();
}

cString cFlatDisplayMenu::GetIconName(const cString &element) const {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::GetIconName() '%s'", *element);
#endif

    static constexpr const char *items[] {"Schedule", "Channels",      "Timers",  "Recordings", "Setup", "Commands",
                                          "OSD",      "EPG",           "DVB",     "LNB",        "CAM",   "Recording",
                                          "Replay",   "Miscellaneous", "Plugins", "Restart"};

    // Static cache to store the names of main menu entries
    static std::unordered_map<std::string, cString> cache;

    std::string_view ElementView {*element}, sv;

    // Check if the element matches any name in the cache
    auto it = cache.find(ElementView.data());
    if (it != cache.end()) return *it->second;  // Return cached icon name including path

    cache.reserve(32);  // Reserve space for 32 entries to avoid rehashing
    //* Check for standard menu entries
    for (const auto &item : items) {
        sv = trVDR(item);  // Translate item to current language
        if (ElementView == sv) {
            cache.emplace(ElementView, cString::sprintf("menuIcons/%s", item));  // Store menu name in cache
            return cString::sprintf("menuIcons/%s", item);
        }
    }

    //* Iterate through all plugins and check for main menu entries
    const char *MainMenuEntry {""};
    for (std::size_t i {0};; ++i) {
        cPlugin *p {cPluginManager::GetPlugin(i)};
#ifdef DEBUGFUNCSCALL
        dsyslog("   Plugin %ld: %p", i, p);
        if (p != nullptr) dsyslog("     Plugin name '%s', menu entry '%s'", p->Name(), p->MainMenuEntry());
#endif
        if (p != nullptr) {                      // Plugin found
            MainMenuEntry = p->MainMenuEntry();  // Get main menu entry of plugin
            if (!isempty(MainMenuEntry)) {       // Plugin has a main menu entry
                if (ElementView == MainMenuEntry) {
#ifdef DEBUGFUNCSCALL
                    dsyslog("     Adding plugin '%s' to cache (#%ld)", p->Name(), cache.size() + 1);
#endif
                    cache.emplace(MainMenuEntry, cString::sprintf("pluginIcons/%s", p->Name()));  // Store in cache
                    return cString::sprintf("pluginIcons/%s", p->Name());                         // Return icon name
                }
            }
        } else {
            break;  // Plugin not found
        }
    }  // for plugins

    //* Check for special main menu entries "stop recording", "stop replay"
    sv = skipspace(trVDR(" Stop recording "));
    if (ElementView == sv) {
        cache.emplace(ElementView, "menuIcons/StopRecording");  // Store in cache
        return "menuIcons/StopRecording";
    }
    sv = skipspace(trVDR(" Stop replaying"));
    if (ElementView == sv) {
        cache.emplace(ElementView, "menuIcons/StopReplay");  // Store in cache
        return "menuIcons/StopReplay";
    }

    //* Nothing found, return a generic icon
    cache.emplace(ElementView, cString::sprintf("extraIcons/%s", *element));  // Store in cache
    return cString::sprintf("extraIcons/%s", *element);
}

cString cFlatDisplayMenu::GetMenuIconName() const {
    static constexpr struct {
        eMenuCategory category;
        const char *icon;
    } MenuIcons[] = {{mcMain, "menuIcons/" VDRLOGO},
                     {mcSchedule, "menuIcons/Schedule"},
                     {mcScheduleNow, "menuIcons/Schedule"},
                     {mcScheduleNext, "menuIcons/Schedule"},
                     {mcChannel, "menuIcons/Channels"},
                     {mcTimer, "menuIcons/Timers"},
                     {mcRecording, "menuIcons/Recordings"},
                     {mcSetup, "menuIcons/Setup"},
                     {mcCommand, "menuIcons/Commands"},
                     {mcEvent, "extraIcons/Info"},
                     {mcRecordingInfo, "extraIcons/PlayInfo"}};
    // Use structured binding
    for (const auto &[category, icon] : MenuIcons) {
        if (category == m_MenuCategory) return cString(icon);
    }

    return "";
}

/**
 * @brief Retrieves the name of a recording at a specified folder level.
 *
 * This function extracts and returns the name segment of a recording based on the specified
 * folder level. It traverses the recording's name string, which is delimited by a specific
 * character, to locate the appropriate segment.
 *
 * @param Recording A pointer to the cRecording object whose name is to be retrieved.
 * @param Level The folder level at which to retrieve the name segment.
 * @param IsFolder A boolean indicating if the recording is a folder.
 * @return The name segment of the recording as a cString.
 */
cString cFlatDisplayMenu::GetRecordingName(const cRecording *Recording, int Level, bool IsFolder) const {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::GetRecordingName() Level %d", Level);
#endif

    if (!Recording) return "";

    std::string_view RecName {Recording->Name()};
    std::string_view RecNamePart {""};
    std::size_t start {0}, end {0};
    for (int i {0}; i <= Level; ++i) {
        end = RecName.find(FOLDERDELIMCHAR, start);
        if (end == std::string_view::npos) {
            if (i == Level) RecNamePart = RecName.substr(start);
            break;
        }
        if (i == Level) {
            RecNamePart = RecName.substr(start, end - start);
            break;
        }
        start = end + 1;
    }
#ifdef DEBUGFUNCSCALL
    dsyslog("   RecNamePart: '%s'", *cString(std::string(RecNamePart).c_str()));
#endif

    return cString(std::string(RecNamePart).c_str());
}

cString cFlatDisplayMenu::GetRecCounts() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::GetRecCounts() m_RecFolder, m_LastItemRecordingLevel: '%s', %d", *m_RecFolder,
            m_LastItemRecordingLevel);
    cTimeMs Timer;  // Set timer
#endif

    uint16_t RecCount {0}, RecNewCount {0};
    m_LastRecFolder = m_RecFolder;
    if (!isempty(*m_RecFolder) && m_LastItemRecordingLevel > 0) {
        const int RecordingLevel {m_LastItemRecordingLevel - 1};  // Folder where the recording is stored in
        cString RecFolder2 {""};
        std::string_view sv1 {*m_RecFolder}, sv2;  // For efficient comparison
        LOCK_RECORDINGS_READ;                      // Creates local const cRecordings *Recordings
        for (const cRecording *Rec {Recordings->First()}; Rec; Rec = Recordings->Next(Rec)) {
            RecFolder2 = *GetRecordingName(Rec, RecordingLevel, true);
            sv2 = *RecFolder2;
            if (sv1 == sv2) {  // Compare recording folder with current folder
                ++RecCount;
                if (Rec->IsNew()) ++RecNewCount;
            }
        }  // for
    } else {                   // All recordings
        LOCK_RECORDINGS_READ;  // Creates local const cRecordings *Recordings
        for (const cRecording *Rec {Recordings->First()}; Rec; Rec = Recordings->Next(Rec)) {
            ++RecCount;
            if (Rec->IsNew()) ++RecNewCount;
        }
    }
    cString RecCounts {""};
    if (Config.ShortRecordingCount) {                            // Hidden option. 0 = disable, 1 = enable
        if (RecNewCount == 0)                                    // No new recordings
            RecCounts = cString::sprintf("(%d)", RecCount);      // (56)
        else if (RecNewCount == RecCount)                        // Only new recordings
            RecCounts = cString::sprintf("(%d*)", RecNewCount);  // (35*)
        else
            RecCounts = cString::sprintf("(%d*/%d)", RecNewCount, RecCount);  // (35*/56)
    } else {
        RecCounts = cString::sprintf("(%d*/%d)", RecNewCount, RecCount);  // (35*/56)
    }  // Config.ShortRecordingCount */
#ifdef DEBUGFUNCSCALL
    dsyslog("   RecCounts '%s', time: %ld ms", *RecCounts, Timer.Elapsed());
#endif

    return RecCounts;
}

void cFlatDisplayMenu::UpdateTimerCounts(uint16_t &TimerActiveCount, uint16_t &TimerCount) const {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::UpdateTimerCounts()");
    cTimeMs Timer;  // Set Timer
#endif

    TimerActiveCount = 0;
    TimerCount = 0;
    LOCK_TIMERS_READ;  // Creates local const cTimers *Timers
    for (const cTimer *Timer {Timers->First()}; Timer; Timer = Timers->Next(Timer)) {
        ++TimerCount;
        if (Timer->HasFlags(tfActive)) ++TimerActiveCount;
    }
#ifdef DEBUGFUNCSCALL
    dsyslog("   TimerActiveCount: %d, TimerCount: %d, time: %ld ms", TimerActiveCount, TimerCount, Timer.Elapsed());
#endif
}

bool cFlatDisplayMenu::IsRecordingOld(const cRecording *Recording, int Level) const {
    int16_t value {-1};
    if (Config.MenuItemRecordingUseOldFile) {
        const cString RecFolder {*GetRecordingName(Recording, Level, true)};
        value = Config.GetRecordingOldValue(*RecFolder);
    }
    if (value < 0) value = Config.MenuItemRecordingDefaultOldDays;
    if (value < 0) return false;

    const time_t LastRecTimeFromFolder {GetLastRecTimeFromFolder(Recording, Level)};
    const time_t now {time(0)};
    const double days = (now - LastRecTimeFromFolder) * (1.0 / SECSINDAY);
    return days > value;
}

const char *cFlatDisplayMenu::GetGenreIcon(uchar genre) const {
    static constexpr const char *icons[][16] {
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

    static constexpr uchar BaseGenres[] {
        ecgMovieDrama,         ecgNewsCurrentAffairs, ecgShow,        ecgSports,
        ecgChildrenYouth,      ecgMusicBalletDance,   ecgArtsCulture, ecgSocialPoliticalEconomics,
        ecgEducationalScience, ecgLeisureHobbies,     ecgSpecial};

    const std::size_t GenreNums {sizeof(BaseGenres) / sizeof(BaseGenres[0])};
    for (std::size_t i {0}; i < GenreNums; ++i) {
        if ((genre & 0xF0) == BaseGenres[i]) {
            // Return the first icon for the genre if field is empty
            return (isempty(icons[i][genre & 0x0F])) ? icons[i][0] : icons[i][genre & 0x0F];
        }
    }

    isyslog("flatPlus: Genre not found: %x", genre);
    return "";
}

void cFlatDisplayMenu::InsertGenreInfo(const cEvent *Event, cString &Text) const {
    bool FirstContent {true};
    cString ContentString {""};
    for (std::size_t i {0}; Event->Contents(i); ++i) {
        ContentString = Event->ContentToString(Event->Contents(i));
        if (ContentString[0] != '\0') {  // Skip empty (user defined) content
            if (!FirstContent)
                Text.Append(", ");
            else
                Text.Append(cString::sprintf("\n%s: ", tr("Genre")));

            Text.Append(ContentString);
            FirstContent = false;
        }
    }
}

void cFlatDisplayMenu::InsertGenreInfo(const cEvent *Event, cString &Text, std::vector<std::string> &GenreIcons) const {
    bool FirstContent {true};
    cString ContentString {""};
    for (std::size_t i {0}; Event->Contents(i); ++i) {
        ContentString = Event->ContentToString(Event->Contents(i));
        if (ContentString[0] != '\0') {  // Skip empty (user defined) content
            if (!FirstContent)
                Text.Append(", ");
            else
                Text.Append(cString::sprintf("\n%s: ", tr("Genre")));

            Text.Append(ContentString);
            FirstContent = false;
            GenreIcons.emplace_back(GetGenreIcon(Event->Contents(i)));
        }
    }
}

/**
 * Returns the start time of the newest or oldest recording in the folder.
 * The folder is determined by the Level parameter.
 * If Config.MenuItemRecordingShowFolderDate is 0, the start time of the given Recording is returned.
 * If Config.MenuItemRecordingShowFolderDate is 1, the newest recording is returned.
 * If Config.MenuItemRecordingShowFolderDate is 2, the oldest recording is returned.
 */
time_t cFlatDisplayMenu::GetLastRecTimeFromFolder(const cRecording *Recording, int Level) const {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::GetLastRecTimeFromFolder() Level: %d", Level);
    cTimeMs Timer;  // Set Timer
#endif

    const time_t RecStart {Recording->Start()};
    if (Config.MenuItemRecordingShowFolderDate == 0) return RecStart;  // None (default)

    const cString RecFolder {*GetRecordingName(Recording, Level, true)};
    if (isempty(*RecFolder)) return RecStart;  // No folder

    std::string_view sv1 {*RecFolder}, sv2;
    cString RecFolder2 {""};
    const time_t now {time(0)};
    time_t RecNewest {0}, RecOldest {now};
    LOCK_RECORDINGS_READ;  // Creates local const cRecordings *Recordings
    for (const cRecording *Rec {Recordings->First()}; Rec; Rec = Recordings->Next(Rec)) {
        RecFolder2 = *GetRecordingName(Rec, Level, true);
        sv2 = *RecFolder2;
        if (sv1 == sv2) {  // Recordings must be in the same folder
            RecNewest = std::max(RecNewest, Rec->Start());
            RecOldest = std::min(RecOldest, Rec->Start());
        }
    }

    if (RecNewest == 0 && RecOldest == now) return RecStart;  // No recordings in folder

#ifdef DEBUGFUNCSCALL
    dsyslog("   Newest: %s, Oldest: %s, time: %ld ms", *ShortDateString(RecNewest), *ShortDateString(RecOldest),
            Timer.Elapsed());
#endif

    return (Config.MenuItemRecordingShowFolderDate == 1) ? RecNewest : RecOldest;
}

int cFlatDisplayMenu::GetTextAreaWidth() const { return m_MenuWidth - m_MarginItem2; }

const cFont *cFlatDisplayMenu::GetTextAreaFont(bool FixedFont) const { return (FixedFont) ? m_FontFixed : m_Font; }

bool cFlatDisplayMenu::CheckProgressBar(const char *text) const {
    const std::size_t TextLength {strlen(text)};
    if (text[0] == '[' && TextLength > 5 && ((text[1] == '|') || (text[1] == ' ')) && text[TextLength - 1] == ']')
        return true;

    return false;
}

void cFlatDisplayMenu::DrawProgressBarFromText(const cRect &rec, const cRect &recBg, const char *bar, tColor ColorFg,
                                               tColor ColorBarFg, tColor ColorBg) {
    const char *p {bar + 1};
    uint16_t now {0}, total {0};
    while (*p != ']') {
        if (*p == '|') ++now;
        ++total;
        ++p;
    }
    if (total > 0) {
        ProgressBarDrawRaw(MenuPixmap, MenuPixmap, rec, recBg, now, total, ColorFg, ColorBarFg, ColorBg,
                           Config.decorProgressMenuItemType, true);
    }
}

/* Widgets */
void cFlatDisplayMenu::DrawMainMenuWidgets() {
    if (!MenuPixmap) return;

    const int MenuPixmapViewPortHeight {MenuPixmap->ViewPort().Height()};
    const int wLeft = (m_OsdWidth * Config.MainMenuItemScale) + m_MarginItem + Config.decorBorderMenuContentSize +
                      Config.decorBorderMenuItemSize;  // Narrowing conversion
    const int wTop {m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 +
                    Config.decorBorderMenuContentSize};

    const int wWidth {m_OsdWidth - wLeft - Config.decorBorderMenuContentSize};
    const int wHeight {MenuPixmapViewPortHeight - m_MarginItem2};

    ContentWidget.Clear();
    ContentWidget.SetOsd(m_Osd);
    ContentWidget.SetPosition(cRect(wLeft, wTop, wWidth, wHeight));
    ContentWidget.SetBGColor(Theme.Color(clrMenuRecBg));
    ContentWidget.SetScrollingActive(false);

    std::vector<std::pair<int, const char *>> widgets;
    widgets.reserve(10);  // Set to at least 10 entry's

    struct WidgetConfig {
        int &ShowFlag;
        int &position;
        const char *name;
    };

    const WidgetConfig WidgetConfigs[] {
        {Config.MainMenuWidgetDVBDevicesShow, Config.MainMenuWidgetDVBDevicesPosition, "dvb_devices"},
        {Config.MainMenuWidgetActiveTimerShow, Config.MainMenuWidgetActiveTimerPosition, "active_timer"},
        {Config.MainMenuWidgetLastRecShow, Config.MainMenuWidgetLastRecPosition, "last_recordings"},
        {Config.MainMenuWidgetSystemInfoShow, Config.MainMenuWidgetSystemInfoPosition, "system_information"},
        {Config.MainMenuWidgetSystemUpdatesShow, Config.MainMenuWidgetSystemUpdatesPosition, "system_updates"},
        {Config.MainMenuWidgetTemperaturesShow, Config.MainMenuWidgetTemperaturesPosition, "temperatures"},
        {Config.MainMenuWidgetTimerConflictsShow, Config.MainMenuWidgetTimerConflictsPosition, "timer_conflicts"},
        {Config.MainMenuWidgetCommandShow, Config.MainMenuWidgetCommandPosition, "custom_command"},
        {Config.MainMenuWidgetWeatherShow, Config.MainMenuWidgetWeatherPosition, "weather"}};

    // Populates the widgets vector with enabled widgets based on configuration
    // Iterates through predefined widget configurations and adds widgets to the vector
    // if their corresponding show flag is enabled, preserving their configured position
    for (const auto &cfg : WidgetConfigs) {
        if (cfg.ShowFlag) { widgets.emplace_back(std::make_pair(cfg.position, cfg.name)); }
    }

    // Define a type for the widget drawing functions
    using WidgetDrawFunction = std::function<int(int, int, int, int)>;

    // Create a map that associates widget names with their drawing functions
    // Works as long all strings are different. Else we must use std::string
    std::unordered_map<const char *, WidgetDrawFunction> WidgetDrawers = {
        {"dvb_devices",
         [this](int left, int width, int top, int MenuPixmapViewPortHeight) {
             return DrawMainMenuWidgetDVBDevices(left, width, top, MenuPixmapViewPortHeight);
         }},
        {"active_timer",
         [this](int left, int width, int top, int MenuPixmapViewPortHeight) {
             return DrawMainMenuWidgetActiveTimers(left, width, top, MenuPixmapViewPortHeight);
         }},
        {"last_recordings",
         [this](int left, int width, int top, int MenuPixmapViewPortHeight) {
             return DrawMainMenuWidgetLastRecordings(left, width, top, MenuPixmapViewPortHeight);
         }},
        {"system_information",
         [this](int left, int width, int top, int MenuPixmapViewPortHeight) {
             return DrawMainMenuWidgetSystemInformation(left, width, top, MenuPixmapViewPortHeight);
         }},
        {"system_updates",
         [this](int left, int width, int top, int MenuPixmapViewPortHeight) {
             return DrawMainMenuWidgetSystemUpdates(left, width, top, MenuPixmapViewPortHeight);
         }},
        {"temperatures",
         [this](int left, int width, int top, int MenuPixmapViewPortHeight) {
             return DrawMainMenuWidgetTemperatures(left, width, top, MenuPixmapViewPortHeight);
         }},
        {"timer_conflicts",
         [this](int left, int width, int top, int MenuPixmapViewPortHeight) {
             return DrawMainMenuWidgetTimerConflicts(left, width, top, MenuPixmapViewPortHeight);
         }},
        {"custom_command",
         [this](int left, int width, int top, int MenuPixmapViewPortHeight) {
             return DrawMainMenuWidgetCommand(left, width, top, MenuPixmapViewPortHeight);
         }},
        {"weather", [this](int left, int width, int top, int MenuPixmapViewPortHeight) {
             return DrawMainMenuWidgetWeather(left, width, top, MenuPixmapViewPortHeight);
         }}};

    // Sort the widgets based on their positions
    std::sort(widgets.begin(), widgets.end(), PairCompareIntString);

    int AddHeight {0}, ContentTop {0};
    // Process widgets
    for (const auto &PairWidget : widgets) {
        std::string_view widget = PairWidget.second;

        // Look up the widget drawer function
        auto drawerIt = WidgetDrawers.find(widget.data());
        if (drawerIt != WidgetDrawers.end()) {
            // Call the drawer function and update ContentTop
            AddHeight = drawerIt->second(wLeft, wWidth, ContentTop, MenuPixmapViewPortHeight);
            if (AddHeight > 0) ContentTop = AddHeight + m_MarginItem;
        } else {
            // Handle unknown widget type
            esyslog("flatPlus: DrawMainMenuWidget() Unknown widget type: %s", widget.data());
        }
    }  // for widgets

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

/**
 * Add a header line to the main menu widget content, with the given icon and title.
 * @param Icon The name of the icon to use, or nullptr for no icon.
 * @param Title The title text to display.
 * @param ContentTop The current top position of the content.
 * @param wWidth The width of the content widget.
 * @return The new top position of the content.
 */
int cFlatDisplayMenu::AddWidgetHeader(const char *Icon, const char *Title, int ContentTop, int wWidth) {
    cImage *img {ImgLoader.GetIcon(Icon, m_FontHeight, m_FontHeight - m_MarginItem2)};
    if (img) ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(Title, false, cRect(m_MarginItem2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, m_LineWidth), Theme.Color(clrMenuEventTitleLine));
    ContentTop += m_LineMargin;

    return ContentTop;
}

int cFlatDisplayMenu::DrawMainMenuWidgetDVBDevices(int wLeft, int wWidth, int ContentTop,
                                                   int MenuPixmapViewPortHeight) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawMainMenuWidgetDVBDevices()");
#endif

    if (ContentTop + m_FontHeight + m_LineMargin + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;  // Not enough space to display anything meaningful

    ContentTop = AddWidgetHeader("widgets/dvb_devices", tr("DVB Devices"), ContentTop, wWidth);

    // Get number of devices
    const int NumDevices {cDevice::NumDevices()};

    // Check device which currently displays live tv
    int DeviceLiveTV {-1};
    cDevice *PrimaryDevice {cDevice::PrimaryDevice()};
    if (PrimaryDevice) {
        if (!PrimaryDevice->Replaying() || PrimaryDevice->Transferring())
            DeviceLiveTV = cDevice::ActualDevice()->DeviceNumber();
        else
            DeviceLiveTV = PrimaryDevice->DeviceNumber();
    }

    // Find all recording devices and set RecDevices[] to true for these devices
    bool RecDevices[NumDevices] {false};  // Array initialised to false
    {
        LOCK_TIMERS_READ;  // Creates local const cTimers *Timers
        for (const cTimer *Timer {Timers->First()}; Timer; Timer = Timers->Next(Timer)) {
            if (Timer->HasFlags(tfRecording)) {
                if (cRecordControl *RecordControl = cRecordControls::GetRecordControl(Timer)) {
                    const cDevice *RecDevice {RecordControl->Device()};
                    // Before setting the RecDevices array element, add bounds check
                    if (RecDevice && RecDevice->DeviceNumber() < NumDevices) {
                        RecDevices[RecDevice->DeviceNumber()] = true;
                    } else if (RecDevice) {
                        esyslog("flatPlus: DrawMainMenuWidgetDVBDevices() Device number %d out of bounds (max: %d)",
                                RecDevice->DeviceNumber(), NumDevices - 1);
                    }
                }
            }
        }  // for cTimer
    }

    // Now display all devices
    bool FoundTuner {false};
    int ActualNumDevices {0};
    cString ChannelName {""}, str {""}, StrDevice {""};
    for (int16_t i {0}; i < NumDevices; ++i) {
        if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
            break;  // Not enough space to display anything meaningful

        const cDevice *device {cDevice::GetDevice(i)};
        if (device && device->NumProvidedSystems())
            FoundTuner = true;
        else
            continue;  // No tuner

        ++ActualNumDevices;
        StrDevice = "";  // Reset string

        const cChannel *channel {device->GetCurrentlyTunedTransponder()};
        if (channel && channel->Number() > 0)
            ChannelName = channel->Name();
        else
            ChannelName = tr("Unknown");

        if (i == DeviceLiveTV) {
            StrDevice.Append(cString::sprintf("%s (%s)", tr("LiveTV"), *ChannelName));
        } else if (RecDevices[i]) {
            StrDevice.Append(cString::sprintf("%s (%s)", tr("recording"), *ChannelName));
        } else {
            if (channel) {
                ChannelName = channel->Name();
                // Check if the first character is null terminator (empty string)
                if (**ChannelName == '\0') {  // Double dereference to get the first character
                    if (Config.MainMenuWidgetDVBDevicesDiscardNotUsed) continue;
                    StrDevice.Append(tr("not used"));
                } else {
                    if (Config.MainMenuWidgetDVBDevicesDiscardUnknown) continue;
                    StrDevice.Append(cString::sprintf("%s (%s)", tr("Unknown"), *ChannelName));
                }
            } else {
                if (Config.MainMenuWidgetDVBDevicesDiscardNotUsed) continue;
                StrDevice.Append(tr("not used"));
            }
        }

        if (Config.MainMenuWidgetDVBDevicesNativeNumbering)
            str = itoa(i);  // Display Tuners 0..3
        else
            str = itoa(i + 1);  // Display Tuners 1..4

        int left {m_MarginItem};
        const int FontSmlWidthDigit {FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "0")};
        if (NumDevices <= 9) {
            ContentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);

            left += FontSmlWidthDigit * 2;
        } else {
            ContentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  FontSmlWidthDigit * 2, m_FontSmlHeight, taRight);

            left += FontSmlWidthDigit * 3;
        }
        str = *(device->DeviceType());  // Something like 'DVB-S2'. Longest in dvbdevice.c is 'ATSCMH'
        const int FontSmlWidthDevice {FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "ATSCMH")};
        ContentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              FontSmlWidthDevice, m_FontSmlHeight, taLeft);

        left += FontSmlWidthDevice + m_MarginItem;
        ContentWidget.AddText(*StrDevice, false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);

        ContentTop += m_FontSmlHeight;
    }  // for NumDevices

    if (FoundTuner == false) {
        dsyslog("flatPlus: DrawMainMenuWidgetDVBDevices() No DVB devices found");
        ContentWidget.AddText(tr("No DVB devices found"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
        // ContentTop += m_FontSmlHeight;
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetActiveTimers(int wLeft, int wWidth, int ContentTop,
                                                     int MenuPixmapViewPortHeight) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: DrawMainMenuWidgetActiveTimers()");
#endif

    if (ContentTop + m_FontHeight + m_LineMargin + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;  // Not enough space to display anything meaningful

    // Look for timers
    cVector<const cTimer *> TimerRec;
    cVector<const cTimer *> TimerActive;
    cVector<const cTimer *> TimerRemoteRec;
    cVector<const cTimer *> TimerRemoteActive;

    int AllTimers {0};  // All local and remote timers

    {
        LOCK_TIMERS_READ;  // Creates local const cTimers *Timers
        for (const cTimer *Timer {Timers->First()}; Timer; Timer = Timers->Next(Timer)) {
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
            if (AllTimers >= Config.MainMenuWidgetActiveTimerMaxCount) break;
        }
    }

    if (AllTimers == 0 && Config.MainMenuWidgetActiveTimerHideEmpty) return -1;

    TimerRec.Sort(CompareTimers);
    TimerActive.Sort(CompareTimers);
    TimerRemoteRec.Sort(CompareTimers);
    TimerRemoteActive.Sort(CompareTimers);

    ContentTop = AddWidgetHeader("widgets/active_timers", tr("Timer"), ContentTop, wWidth);

    int Left {m_MarginItem};
    int Width {wWidth - m_MarginItem2};
    if (AllTimers == 0) {
        ContentWidget.AddText(tr("no active/recording timer"), false, cRect(Left, ContentTop, Width, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, Width);
    } else {
        int count {-1}, RemoteCount {-1};
        cString StrTimer {""};
        cImage *img {nullptr};
        // First recording timer
        if (Config.MainMenuWidgetActiveTimerShowRecording) {
            const int TimerRecSize {TimerRec.Size()};
            for (int16_t i {0}; i < TimerRecSize; ++i) {
                ++count;
                if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight) break;

                if (count >= Config.MainMenuWidgetActiveTimerMaxCount) break;

                StrTimer = "";  // Reset string
                if ((Config.MainMenuWidgetActiveTimerShowRemoteActive ||
                     Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
                    (TimerRemoteRec.Size() > 0 || TimerRemoteActive.Size() > 0)) {
                    img = ImgLoader.GetIcon("widgets/home", m_FontSmlHeight, m_FontSmlHeight);
                    if (img) {
                        ContentWidget.AddImage(img, cRect(Left, ContentTop, Width, m_FontSmlHeight));
                        Left += m_FontSmlHeight + m_MarginItem;
                        Width -= m_FontSmlHeight - m_MarginItem;
                    } else {
                        StrTimer = "L: ";  // Local (Fallback)
                    }
                }

                const cChannel *Channel {(TimerRec[i])->Channel()};
                StrTimer.Append(cString::sprintf("%s - ", Channel ? Channel->Name() : tr("Unknown")));
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
            for (int16_t i {0}; i < TimerActiveSize; ++i) {
                ++count;
                if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight) break;

                if (count >= Config.MainMenuWidgetActiveTimerMaxCount) break;

                StrTimer = "";  // Reset string
                if ((Config.MainMenuWidgetActiveTimerShowRemoteActive ||
                     Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
                    (TimerRemoteRec.Size() > 0 || TimerRemoteActive.Size() > 0)) {
                    img = ImgLoader.GetIcon("widgets/home", m_FontSmlHeight, m_FontSmlHeight);
                    if (img) {
                        ContentWidget.AddImage(img, cRect(Left, ContentTop, Width, m_FontSmlHeight));
                        Left += m_FontSmlHeight + m_MarginItem;
                        Width -= m_FontSmlHeight - m_MarginItem;
                    } else {
                        StrTimer = "L: ";  // Local (Fallback)
                    }
                }

                const cChannel *Channel {(TimerActive[i])->Channel()};
                StrTimer.Append(cString::sprintf("%s - ", Channel ? Channel->Name() : tr("Unknown")));
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
            for (int16_t i {0}; i < TimerRemoteRecSize; ++i) {
                ++RemoteCount;
                if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight) break;

                if (count + RemoteCount >= Config.MainMenuWidgetActiveTimerMaxCount) break;

                StrTimer = "";  // Reset string
                img = ImgLoader.GetIcon("widgets/remotetimer", m_FontSmlHeight, m_FontSmlHeight);
                if (img) {
                    ContentWidget.AddImage(img, cRect(Left, ContentTop, Width, m_FontSmlHeight));
                    Left += m_FontSmlHeight + m_MarginItem;
                    Width -= m_FontSmlHeight - m_MarginItem;
                } else {
                    StrTimer = "R: ";  // Remote (Fallback)
                }

                const cChannel *Channel {(TimerRemoteRec[i])->Channel()};
                // const cEvent *Event {Timer->Event()};
                StrTimer.Append(cString::sprintf("%s - ", Channel ? Channel->Name() : tr("Unknown")));
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
            for (int16_t i {0}; i < TimerRemoteActiveSize; ++i) {
                ++RemoteCount;
                if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight) break;

                if (count + RemoteCount >= Config.MainMenuWidgetActiveTimerMaxCount) break;

                StrTimer = "";  // Reset string
                img = ImgLoader.GetIcon("widgets/remotetimer", m_FontSmlHeight, m_FontSmlHeight);
                if (img) {
                    ContentWidget.AddImage(img, cRect(Left, ContentTop, Width, m_FontSmlHeight));
                    Left += m_FontSmlHeight + m_MarginItem;
                    Width -= m_FontSmlHeight - m_MarginItem;
                } else {
                    StrTimer = "R: ";  // Remote (Fallback)
                }

                const cChannel *Channel {(TimerRemoteActive[i])->Channel()};
                StrTimer.Append(cString::sprintf("%s - ", Channel ? Channel->Name() : tr("Unknown")));
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

/**
 * Draws the Last Recordings widget in the main menu
 * @param wLeft Left position of the widget
 * @param wWidth Width of the widget
 * @param ContentTop Top position for the content
 * @param MenuPixmapViewPortHeight Height of the menu pixmap view port
 * @return The content height if successful, -1 if there's not enough space to display
 */
int cFlatDisplayMenu::DrawMainMenuWidgetLastRecordings(int wLeft, int wWidth, int ContentTop,
                                                       int MenuPixmapViewPortHeight) {
#ifdef DEBUGFUNCSCALL
    dsyslog("DrawMainMenuWidgetLastRecordings(%d, %d, %d)", wLeft, wWidth, ContentTop);
#endif

    if (ContentTop + m_FontHeight + m_LineMargin + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;  // Not enough space to display anything meaningful

    // Get all Recordings including start time and build string for displaying
    std::vector<std::pair<time_t, cString>> Recs;
    Recs.reserve(512);  // Set to at least 512 entry's
    time_t RecStart {0};
    cString DateTime {""}, Length {""}, StrRec {""};
    div_t TimeHM {0, 0};
    {
        LOCK_RECORDINGS_READ;  // Creates local const cRecordings *Recordings
        for (const cRecording *Rec {Recordings->First()}; Rec; Rec = Recordings->Next(Rec)) {
            RecStart = Rec->Start();
            TimeHM = std::div((Rec->LengthInSeconds() + 30) / 60, 60);
            Length = cString::sprintf("%02d:%02d", TimeHM.quot, TimeHM.rem);
            DateTime = cString::sprintf("%s %s Σ%s", *ShortDateString(RecStart), *TimeString(RecStart), *Length);
            StrRec = cString::sprintf("%s - %s", *DateTime, Rec->Name());
            Recs.emplace_back(std::make_pair(RecStart, StrRec));
        }
    }

    ContentTop = AddWidgetHeader("widgets/last_recordings", tr("Last Recordings"), ContentTop, wWidth);
    if (Recs.size() == 0) {
        ContentWidget.AddText(
            tr("no recordings found"), false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem2);
        ContentTop += m_FontSmlHeight;
        return ContentWidget.ContentHeight(false);
    }

    // Sort by RecStart and add entrys to ContentWidget
    // Use 'partial_sort()' to limit the number of entries to Config.MainMenuWidgetLastRecMaxCount
    const std::size_t LastRecMaxCount = Config.MainMenuWidgetLastRecMaxCount;
    std::partial_sort(Recs.begin(), Recs.begin() + LastRecMaxCount, Recs.end(), PairCompareTimeString);
    for (std::size_t i {0}; i < LastRecMaxCount && i < Recs.size(); ++i) {
        if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight)
            break;  // Exit the loop if we've run out of display space

        const auto &PairRec = Recs[i];

        ContentWidget.AddText(
            *PairRec.second, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem2);
        ContentTop += m_FontSmlHeight;
    }
    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetTimerConflicts(int wLeft, int wWidth, int ContentTop,
                                                       int MenuPixmapViewPortHeight) {
#ifdef DEBUGFUNCSCALL
    dsyslog("DrawMainMenuWidgetTimerConflicts(%d, %d, %d)", wLeft, wWidth, ContentTop);
#endif

    if (ContentTop + m_FontHeight + m_LineMargin+ m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;  // Not enough space to display anything meaningful

    const int NumConflicts {GetEpgsearchConflicts()};  // Get conflicts from plugin Epgsearch
    if (NumConflicts == 0 && Config.MainMenuWidgetTimerConflictsHideEmpty) return -1;  // No conflicts and hide empty

    ContentTop = AddWidgetHeader("widgets/timer_conflicts", tr("Timer Conflicts"), ContentTop, wWidth);
    if (NumConflicts == 0) {
        ContentWidget.AddText(
            tr("no timer conflicts"), false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem2);
    } else {
        cString str = cString::sprintf("%s: %d", tr("timer conflicts"), NumConflicts);
        ContentWidget.AddText(*str, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetSystemInformation(int wLeft, int wWidth, int ContentTop,
                                                          int MenuPixmapViewPortHeight) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: DrawMainMenuWidgetSystemInfomation() ContentTop: %d", ContentTop);
#endif

    if (ContentTop + m_FontHeight + m_LineMargin + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;  // Not enough space to display anything meaningful

    static const cString ExecFile = cString::sprintf("\"%s/system_information/system_information\"", WIDGETFOLDER);
    [[maybe_unused]] int r {system(*ExecFile)};  // Prevent warning for unused variable

    static const cString ConfigsPath = cString::sprintf("%s/system_information/", WIDGETOUTPUTPATH);

    cReadDir d(*ConfigsPath);
    if (d.Ok() == false) {
        // Handle error: unable to read directory
        dsyslog("flatPlus: DrawMainMenuWidgetSystemInfomation() Unable to read directory: %s", *ConfigsPath);
        return -1;
    }
    std::vector<cString> files;
    files.reserve(32);  // Set to at least 32 entry's

    struct dirent *entry;
    cString num {""};
    std::string_view FileName {""};
    std::size_t found {std::string_view::npos};
    while ((entry = d.Next()) != nullptr) {
        FileName = entry->d_name;
        found = FileName.find('_');
        if (found != std::string_view::npos) {               // File name contains '_'
            num = cString(FileName.data()).Truncate(found);  // Truncate the string to the number part
            if (atoi(*num) > 0)                              // Number is greater than zero
                files.emplace_back(FileName.data());         // Store the file name
        }
    }

    // dsyslog("flatPlus: DrawMainMenuWidgetSystemInfomation() Found %ld files", files.size());
    std::sort(files.begin(), files.end(), CompareNumStrings);  // Sort the files by number

    ContentTop = AddWidgetHeader("widgets/system_information", tr("System Information"), ContentTop, wWidth);

    cString Buffer {""};
    if (files.size() == 0) {
        Buffer = cString::sprintf("%s - %s", tr("no information available please check the script"), *ExecFile);
        ContentWidget.AddText(*Buffer, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
    } else {
        // Items are stored in an array, in which case they need to be marked differently
        // Using the trNOOP() macro, the actual translation is then done when such a text is used
        // https://github.com/vdr-projects/vdr/wiki/The-VDR-Plugin-System#internationalization
        const struct ItemData {
            const char *key;
            const char *label;
        } items[] {{"sys_version", trNOOP("System Version")},
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
                   {"security_updates", trNOOP("Security Updates")}};

        cString item {""}, ItemContent {""}, ItemFilename {""};
        std::string_view FileNameView {""};  // Use std::string_view for better performance
        int Column {1};
        int ContentLeft {m_MarginItem};
        for (const cString &FileName : files) {
            if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight) break;

            FileNameView = *FileName;  // Convert cString to std::string_view
            found = FileNameView.find('_');
            item = FileNameView.substr(found + 1).data();  // Extract the item name
            ItemFilename = cString::sprintf("%s/system_information/%s", WIDGETOUTPUTPATH, FileNameView.data());

            ItemContent = ReadAndExtractData(ItemFilename);
            if (isempty(*ItemContent)) continue;

            for (const auto &data : items) {
                if (strcmp(*item, data.key) == 0) {
                    Buffer = cString::sprintf("%s: %s", tr(data.label), *ItemContent);
                    ContentWidget.AddText(*Buffer, false,
                                          cRect(ContentLeft, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                                          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                          wWidth - m_MarginItem2);
                    // Items 'sys_version' and 'kernel_version' are printed on one line
                    if (Column == 1 && !(strcmp(*item, items[0].key) == 0 || strcmp(*item, items[1].key) == 0)) {
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
        }
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetSystemUpdates(int wLeft, int wWidth, int ContentTop,
                                                      int MenuPixmapViewPortHeight) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawMainMenuWidgetSystemUpdates()");
#endif
    if (ContentTop + m_FontHeight + m_LineMargin + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;  // Not enough space to display anything meaningful

    cString Content = *ReadAndExtractData(cString::sprintf("%s/system_updatestatus/updates", WIDGETOUTPUTPATH));
    const int updates {(Content[0] == '\0') ? -1 : atoi(*Content)};

    Content = *ReadAndExtractData(cString::sprintf("%s/system_updatestatus/security_updates", WIDGETOUTPUTPATH));
    const int SecurityUpdates {(Content[0] == '\0') ? -1 : atoi(*Content)};

    if (updates == 0 && SecurityUpdates == 0 && Config.MainMenuWidgetSystemUpdatesHideIfZero)
        return -1;  // Nothing to display

    ContentTop = AddWidgetHeader("widgets/system_updates", tr("System Updates"), ContentTop, wWidth);
    if (updates == -1 || SecurityUpdates == -1) {
        ContentWidget.AddText(tr("Updatestatus not available please check the widget"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
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

int cFlatDisplayMenu::DrawMainMenuWidgetTemperatures(int wLeft, int wWidth, int ContentTop,
                                                     int MenuPixmapViewPortHeight) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMenu::DrawMainMenuWidgetTemperatures()");
#endif

    if (ContentTop + m_FontHeight + m_LineMargin + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;  // Not enough space to display anything meaningful

    ContentTop = AddWidgetHeader("widgets/temperatures", tr("Temperatures"), ContentTop, wWidth);

    static const cString ExecFile =
        cString::sprintf("cd \"%s/temperatures\"; \"%s/temperatures/temperatures\"", WIDGETFOLDER, WIDGETFOLDER);
    [[maybe_unused]] int r {system(*ExecFile)};  // Prevent warning for unused variable

    int CountTemps {0};

    const cString TempCPU = *ReadAndExtractData(cString::sprintf("%s/temperatures/cpu", WIDGETOUTPUTPATH));
    if (TempCPU[0] != '\0') ++CountTemps;

    const cString TempCase = *ReadAndExtractData(cString::sprintf("%s/temperatures/pccase", WIDGETOUTPUTPATH));
    if (TempCase[0] != '\0') ++CountTemps;

    const cString TempMB = *ReadAndExtractData(cString::sprintf("%s/temperatures/motherboard", WIDGETOUTPUTPATH));
    if (TempMB[0] != '\0') ++CountTemps;

    const cString TempGPU = *ReadAndExtractData(cString::sprintf("%s/temperatures/gpu", WIDGETOUTPUTPATH));
    if (TempGPU[0] != '\0') ++CountTemps;

    if (CountTemps == 0) {
        ContentWidget.AddText(tr("Temperatures not available please check the widget"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem2);
    } else {
        const int AddLeft {wWidth / CountTemps};
        int Left {m_MarginItem};
        cString str {""};
        if (TempCPU[0] != '\0') {
            str = cString::sprintf("%s: %s", tr("CPU"), *TempCPU);
            ContentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  wWidth - m_MarginItem2);
            Left += AddLeft;
        }
        if (TempCase[0] != '\0') {
            str = cString::sprintf("%s: %s", tr("PC-Case"), *TempCase);
            ContentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth / 3 - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  wWidth - m_MarginItem2);
            Left += AddLeft;
        }
        if (TempMB[0] != '\0') {
            str = cString::sprintf("%s: %s", tr("MB"), *TempMB);
            ContentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth / 3 * 2 - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  wWidth - m_MarginItem2);
            Left += AddLeft;
        }
        if (TempGPU[0] != '\0') {
            str = cString::sprintf("%s: %s", tr("GPU"), *TempGPU);
            ContentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth / 3 * 2 - m_MarginItem2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  wWidth - m_MarginItem2);
        }
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetCommand(int wLeft, int wWidth, int ContentTop, int MenuPixmapViewPortHeight) {
    if (ContentTop + m_FontHeight + m_LineMargin + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;  // Not enough space to display anything meaningful

    static const cString ExecFile = cString::sprintf("\"%s/command_output/command\"", WIDGETFOLDER);
    [[maybe_unused]] int r {system(*ExecFile)};  // Prevent warning for unused variable

    cString Title = *ReadAndExtractData(cString::sprintf("%s/command_output/title", WIDGETOUTPUTPATH));
    if (Title[0] == '\0') Title = tr("no title available");

    ContentTop = AddWidgetHeader("widgets/command_output", *Title, ContentTop, wWidth);

    std::string Output {""};
    Output.reserve(32);
    static const cString ItemFilename = cString::sprintf("%s/command_output/output", WIDGETOUTPUTPATH);
    std::ifstream file(*ItemFilename);
    if (file.is_open()) {
        for (; std::getline(file, Output);) {
            if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight) break;
            ContentWidget.AddText(
                Output.c_str(), false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem2);
            ContentTop += m_FontSmlHeight;
        }
        file.close();
    } else {
        ContentWidget.AddText(
            tr("no output available"), false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem2, m_FontSmlHeight),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem2);
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetWeather(int wLeft, int wWidth, int ContentTop, int MenuPixmapViewPortHeight) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatBaseRender::DrawMainMenuWidgetWeather()");
#endif

    if (ContentTop + m_FontHeight + m_LineMargin + m_FontSmlHeight > MenuPixmapViewPortHeight)
        return -1;  // Not enough space to display anything meaningful

    time_t NewestFiletime {0};  // Last read time
    if (!WeatherCache.valid || !BatchReadWeatherData(WeatherCache, NewestFiletime) ||
        (WeatherCache.LastReadMTime != NewestFiletime)) {
#ifdef DEBUGFUNCSCALL
        dsyslog("   Need to refresh/calc cache");
#endif
        // Need to refresh/calc
        if (!BatchReadWeatherData(WeatherCache, NewestFiletime)) return -1;  // Error reading weather data
#ifdef DEBUGFUNCSCALL
        dsyslog("   Data read");
#endif
        WeatherCache.LastReadMTime = NewestFiletime;
        WeatherCache.valid = true;
    }

    const auto &wd = WeatherCache;  // Weather data reference

    const cString TempToday = cString::sprintf("%s %s", *wd.Temp, *wd.TempTodaySign);  // Read temperature
    const cString Title = cString::sprintf("%s - %s %s", tr("Weather"), *wd.Location, *TempToday);
    ContentTop = AddWidgetHeader("widgets/weather", *Title, ContentTop, wWidth);

    int left {m_MarginItem};
    cString WeatherIcon {""};
    cImage *ImgUmbrella {ImgLoader.GetIcon("widgets/umbrella", m_FontHeight, m_FontHeight - m_MarginItem2)};
    cImage *img {nullptr};
    const int Middle {(m_FontHeight - m_FontTempSmlHeight) / 2};  // Vertical center
    const int TempSmlWidth {FontCache.GetStringWidth(m_FontTempSmlName, m_FontTempSmlHeight, "-00,0°C")};  // Width
    const int16_t MainMenuWidgetWeatherDays =
        std::min(Config.MainMenuWidgetWeatherDays, wd.kMaxDays);                           // Narrowing conversion
    if (Config.MainMenuWidgetWeatherType == 0) {                                           // Short
        const int TempBarWidth {FontCache.GetStringWidth(m_FontName, m_FontHeight, "|")};  // Width of the char '|'
        // Width to fit the temperature string in
        const int TempMaxStringWidth {FontCache.GetStringWidth(m_FontTempSmlName, m_FontTempSmlHeight, "MMMM")};
        for (int16_t i {0}; i < MainMenuWidgetWeatherDays; ++i) {
            if (left + m_FontHeight2 + TempSmlWidth + TempMaxStringWidth + m_MarginItem * 6 > wWidth) break;

            if (isempty(*wd.Days[i].Icon) || isempty(*wd.Days[i].TempMax) || isempty(*wd.Days[i].TempMin) ||
                isempty(*wd.Days[i].Precipitation))  // Check if data is available
                continue;

            if (i > 0) {
                ContentWidget.AddText("|", false, cRect(left, ContentTop, 0, 0), Theme.Color(clrMenuEventFontInfo),
                                      Theme.Color(clrMenuEventBg), m_Font);
                left += TempBarWidth + m_MarginItem2;
            }

            WeatherIcon = cString::sprintf("widgets/%s", *wd.Days[i].Icon);
            img = ImgLoader.GetIcon(*WeatherIcon, m_FontHeight, m_FontHeight - m_MarginItem2);
            if (img) {
                ContentWidget.AddImage(img, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight + m_MarginItem;
            }
            const int wtemp {
                std::max(m_FontTempSml->Width(*wd.Days[i].TempMax), m_FontTempSml->Width(*wd.Days[i].TempMin))};
            ContentWidget.AddText(*wd.Days[i].TempMax, false, cRect(left, ContentTop, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml, wtemp,
                                  m_FontTempSmlHeight, taRight);
            ContentWidget.AddText(*wd.Days[i].TempMin, false, cRect(left, ContentTop + m_FontTempSmlHeight, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml, wtemp,
                                  m_FontTempSmlHeight, taRight);

            left += wtemp + m_MarginItem;

            if (ImgUmbrella) {
                ContentWidget.AddImage(ImgUmbrella, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight - m_MarginItem;
            }
            ContentWidget.AddText(*wd.Days[i].Precipitation, false, cRect(left, ContentTop + Middle, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml);
            left += m_FontTempSml->Width(*wd.Days[i].Precipitation) + m_MarginItem2;
        }  // for Config.MainMenuWidgetWeatherDays
    } else {  // Long
        const time_t now {time(0)};
        time_t t2 {0};
        // Space between temperature and precipitation
        const int TempSmlSpaceWidth {FontCache.GetStringWidth(m_FontTempSmlName, m_FontTempSmlHeight, " ")};
        // Max. width of precipitation string
        const int TempSmlPrecWidth {FontCache.GetStringWidth(m_FontTempSmlName, m_FontTempSmlHeight, "100%")};
        const int DayStringWidth {FontCache.GetStringWidth(m_FontName, m_FontHeight, "Mon")};  // Mon, Mo.
        for (int16_t i {0}; i < MainMenuWidgetWeatherDays; ++i) {
            if (ContentTop + m_MarginItem > MenuPixmapViewPortHeight) break;

            if (isempty(*wd.Days[i].Icon) || isempty(*wd.Days[i].TempMax) || isempty(*wd.Days[i].TempMin) ||
                isempty(*wd.Days[i].Summary) || isempty(*wd.Days[i].Precipitation))  // Check if data is available
                continue;

            t2 = now + (i * SECSINDAY);  // Calculate time for the current day
            left = m_MarginItem;
            ContentWidget.AddText(
                *WeekDayName(t2), false, cRect(left, ContentTop, wWidth - m_MarginItem2, m_FontHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font, wWidth - m_MarginItem2);
            left += DayStringWidth + m_MarginItem;

            WeatherIcon = cString::sprintf("widgets/%s", *wd.Days[i].Icon);
            img = ImgLoader.GetIcon(*WeatherIcon, m_FontHeight, m_FontHeight - m_MarginItem2);
            if (img) {
                ContentWidget.AddImage(img, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight + m_MarginItem;
            }
            ContentWidget.AddText(*wd.Days[i].TempMax, false, cRect(left, ContentTop, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml,
                                  TempSmlWidth, m_FontTempSmlHeight, taRight);
            ContentWidget.AddText(*wd.Days[i].TempMin, false, cRect(left, ContentTop + m_FontTempSmlHeight, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml,
                                  TempSmlWidth, m_FontTempSmlHeight, taRight);

            left += TempSmlWidth + TempSmlSpaceWidth + m_MarginItem;

            if (ImgUmbrella) {
                ContentWidget.AddImage(ImgUmbrella, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight - m_MarginItem;
            }
            ContentWidget.AddText(*wd.Days[i].Precipitation, false, cRect(left, ContentTop + Middle, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml,
                                  TempSmlPrecWidth, m_FontTempSmlHeight, taRight);
            left += TempSmlPrecWidth + TempSmlSpaceWidth + m_MarginItem;

            ContentWidget.AddText(
                *wd.Days[i].Summary, false, cRect(left, ContentTop + Middle, wWidth - left, m_FontHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml, wWidth - left);

            ContentTop += m_FontHeight;
        }  // for Config.MainMenuWidgetWeatherDays
    }

    return ContentWidget.ContentHeight(false);
#ifdef DEBUGFUNCSCALL
    dsyslog("DrawMainMenuWidgetWeather() Done");
#endif
}

void cFlatDisplayMenu::PreLoadImages() {
    // Menu icons
    const int ImageHeight {m_FontHeight};
    const cString Path = cString::sprintf("%s/%s/menuIcons", *Config.IconPath, Setup.OSDTheme);
    std::string File {""};
    File.reserve(256);
    cString FileName {""};
    cReadDir d(*Path);
    if (d.Ok() == false) {
        // Handle error
        esyslog("flatPlus: PreLoadImages() Unable to read directory: %s", *Path);
        return;
    }

    struct dirent *e;
    while ((e = d.Next()) != nullptr) {
        File = e->d_name;
        if (File.find("vdrlogo") == 0)  // Skip vdrlogo* files
            continue;
        FileName = cString::sprintf("menuIcons/%s", File.substr(0, File.find_last_of(".")).c_str());
        ImgLoader.GetIcon(*FileName, ImageHeight - m_MarginItem2, ImageHeight - m_MarginItem2);
    }

    if (Config.TopBarMenuIconShow)
        ImgLoader.GetIcon(cString::sprintf("menuIcons/%s", VDRLOGO), kIconMaxSize, m_TopBarHeight - m_MarginItem2);

    ImgLoader.GetIcon("menuIcons/blank", ImageHeight - m_MarginItem2, ImageHeight - m_MarginItem2);

    // Channel icons
    int ImageBgHeight {ImageHeight};
    int ImageBgWidth = ImageHeight * 1.34f;  // Narrowing conversion
    cImage *img {ImgLoader.GetIcon("logo_background", ImageBgWidth, ImageBgHeight)};
    if (img) {
        ImageBgHeight = img->Height();
        ImageBgWidth = img->Width();
    }

    //* Same as in 'displaychannel.c' 'PreLoadImages()', but different logo size!
    uint16_t i {0};
    LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
    for (const cChannel *Channel {Channels->First()}; Channel && i < kLogoPreCache; Channel = Channels->Next(Channel)) {
        if (!Channel->GroupSep()) {  // Don't cache named channel group logo
            img = ImgLoader.GetLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
            if (img) ++i;
        }
    }  // for channel

    ImgLoader.GetIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
    ImgLoader.GetIcon("changroup", ImageBgWidth - 10, ImageBgHeight - 10);
    ImgLoader.GetIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);

    // Plugin icons
    for (std::size_t i {0};; ++i) {
        cString PluginName {""};
        cPlugin *p {cPluginManager::GetPlugin(i)};
        if (p) {
            if (!isempty(p->MainMenuEntry())) {  // Plugin has a main menu entry
                PluginName = cString::sprintf("pluginIcons/%s", p->Name());
                ImgLoader.GetIcon(*PluginName, ImageHeight - m_MarginItem2, ImageHeight - m_MarginItem2);
            }
        } else {
            break;
        }
    }

    // Top bar icons
    ImgLoader.GetIcon("radio", kIconMaxSize, m_TopBarHeight - m_MarginItem2);
    ImgLoader.GetIcon("changroup", kIconMaxSize, m_TopBarHeight - m_MarginItem2);
    ImgLoader.GetIcon("tv", kIconMaxSize, m_TopBarHeight - m_MarginItem2);

    ImgLoader.GetIcon("timerInactive", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("timerRecording", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("timerActive", ImageHeight, ImageHeight);

    ImgLoader.GetIcon("timer_full", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("timer_full_cur", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("timer_partial", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("vps", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("vps_cur", ImageHeight, ImageHeight);

    ImgLoader.GetIcon("sd", ImageHeight, ImageHeight / 3);
    ImgLoader.GetIcon("hd", ImageHeight, ImageHeight / 3);
    ImgLoader.GetIcon("uhd", ImageHeight, ImageHeight / 3);

    ImgLoader.GetIcon("folder", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("recording", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("recording_cutted", ImageHeight, ImageHeight * (2.0 / 3.0));
    ImgLoader.GetIcon("recording_cutted_cur", ImageHeight, ImageHeight * (2.0 / 3.0));
    ImgLoader.GetIcon("recording_new", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("recording_new_cur", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("recording_old", ImageHeight, ImageHeight);
    ImgLoader.GetIcon("recording_old_cur", ImageHeight, ImageHeight);

    ImgLoader.GetIcon("recording_new", m_FontSmlHeight, m_FontSmlHeight);
    ImgLoader.GetIcon("recording_new_cur", m_FontSmlHeight, m_FontSmlHeight);
    ImgLoader.GetIcon("recording_old", m_FontSmlHeight, m_FontSmlHeight);
    ImgLoader.GetIcon("recording_old_cur", m_FontSmlHeight, m_FontSmlHeight);

    // Widget icons
    if (Config.MainMenuWidgetDVBDevicesShow) {
        ImgLoader.GetIcon("widgets/dvb_devices", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetActiveTimerShow) {
        ImgLoader.GetIcon("widgets/active_timers", ImageHeight, ImageHeight - m_MarginItem2);
        ImgLoader.GetIcon("widgets/home", m_FontSmlHeight, m_FontSmlHeight);
        ImgLoader.GetIcon("widgets/remotetimer", m_FontSmlHeight, m_FontSmlHeight);
    }
    if (Config.MainMenuWidgetLastRecShow) {
        ImgLoader.GetIcon("widgets/last_recordings", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetTimerConflictsShow) {
        ImgLoader.GetIcon("widgets/timer_conflicts", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetSystemInfoShow) {
        ImgLoader.GetIcon("widgets/system_information", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetSystemUpdatesShow) {
        ImgLoader.GetIcon("widgets/system_updates", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetTemperaturesShow) {
        ImgLoader.GetIcon("widgets/temperatures", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetCommandShow) {
        ImgLoader.GetIcon("widgets/command_output", ImageHeight, ImageHeight - m_MarginItem2);
    }
    if (Config.MainMenuWidgetWeatherShow) {
        ImgLoader.GetIcon("widgets/weather", ImageHeight, ImageHeight - m_MarginItem2);
        ImgLoader.GetIcon("widgets/umbrella", ImageHeight, ImageHeight - m_MarginItem2);
    }
}
