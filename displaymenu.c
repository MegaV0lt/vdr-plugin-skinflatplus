/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displaymenu.h"
#include <fstream>
#include <iostream>
#include <utility>

#include <future>

#include <sstream>
#include <locale>

#include "./services/epgsearch.h"
#include "./services/remotetimers.h"
#include "./services/scraper2vdr.h"

#ifndef VDRLOGO
#define VDRLOGO "vdrlogo_default"
#endif

#include "./flat.h"
#include "./locale"

static int CompareTimers(const void *a, const void *b) {
    return (*(const cTimer **)a)->Compare(**(const cTimer **)b);
}

cFlatDisplayMenu::cFlatDisplayMenu(void) {
    CreateFullOsd();
    TopBarCreate();
    ButtonsCreate();
    MessageCreate();

    m_FontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);  // Top of capital letter

    m_VideoDiskUsageState = -1;

    m_ItemHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
    m_ItemChannelHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
    m_ItemTimerHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;

    m_ScrollBarWidth = ScrollBarWidth() + m_MarginItem;
    m_ScrollBarHeight = m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_MarginItem * 3 +
                                       m_ButtonsHeight + Config.decorBorderButtonSize * 2);
    m_ScrollBarTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2;

    m_MenuWidth = m_OsdWidth;
    m_MenuTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize;
    MenuPixmap = CreatePixmap(m_Osd, "MenuPixmap", 1, cRect(0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight));
    // dsyslog("flatPlus: MenuPixmap left: %d top: %d width: %d height: %d", 0, m_MenuTop, m_MenuWidth,
    // m_ScrollBarHeight);
    MenuIconsBgPixmap = CreatePixmap(m_Osd, "MenuIconsBgPixmap", 2,
                                     cRect(0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight));
    // dsyslog("flatPlus: MenuIconsBgPixmap left: %d top: %d width: %d height: %d", 0,
    //         m_MenuTop, m_MenuWidth, m_ScrollBarHeight);
    MenuIconsPixmap = CreatePixmap(m_Osd, "MenuIconsPixmap", 3, cRect(0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight));
    // dsyslog("flatPlus: MenuIconsPixmap left: %d top: %d width: %d height: %d",
    //         0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight);
    MenuIconsOvlPixmap =
        CreatePixmap(m_Osd, "MenuIconsOvlPixmap", 4, cRect(0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight));
    // dsyslog("flatPlus: MenuIconsOvlPixmap left: %d top: %d width: %d height: %d",
    //         0, m_MenuTop, m_MenuWidth, m_ScrollBarHeight);

    m_chLeft = Config.decorBorderMenuContentHeadSize;
    m_chTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentHeadSize;
    m_chWidth = m_MenuWidth - Config.decorBorderMenuContentHeadSize * 2;
    m_chHeight = m_FontHeight + m_FontSmlHeight * 2 + m_MarginItem * 2;
    ContentHeadPixmap = CreatePixmap(m_Osd, "ContentHeadPixmap", 1, cRect(m_chLeft, m_chTop, m_chWidth, m_chHeight));
    // dsyslog("flatPlus: ContentHeadPixmap left: %d top: %d width: %d height: %d", m_chLeft, m_chTop, m_chWidth,
    // m_chHeight);
    ContentHeadIconsPixmap =
        CreatePixmap(m_Osd, "ContentHeadIconsPixmap", 2, cRect(m_chLeft, m_chTop, m_chWidth, m_chHeight));

    ScrollbarPixmap = CreatePixmap(
        m_Osd, "ScrollbarPixmap", 2,
        cRect(0, m_ScrollBarTop, m_MenuWidth, m_ScrollBarHeight + m_ButtonsHeight + Config.decorBorderButtonSize * 2));
    // dsyslog("flatPlus: ScrollbarPixmap left: %d top: %d width: %d height: %d", 0, m_ScrollBarTop, m_MenuWidth,
    //         m_ScrollBarHeight + m_ButtonsHeight + Config.decorBorderButtonSize * 2);

    PixmapFill(MenuPixmap, clrTransparent);
    PixmapFill(MenuIconsPixmap, clrTransparent);
    PixmapFill(MenuIconsBgPixmap, clrTransparent);
    PixmapFill(MenuIconsOvlPixmap, clrTransparent);
    PixmapFill(ScrollbarPixmap, clrTransparent);
    PixmapFill(ContentHeadIconsPixmap, clrTransparent);

    m_MenuCategory = mcUndefined;

    MenuItemScroller.SetOsd(m_Osd);
    MenuItemScroller.SetScrollStep(Config.ScrollerStep);
    MenuItemScroller.SetScrollDelay(Config.ScrollerDelay);
    MenuItemScroller.SetScrollType(Config.ScrollerType);

    // Call to get values for 'DiskUsage' and have it outside of SetItem()
    cVideoDiskUsage::HasChanged(m_VideoDiskUsageState);
}

cFlatDisplayMenu::~cFlatDisplayMenu() {
    MenuItemScroller.Clear();
    if (m_FontTempSml)  // Created in 'DrawMainMenuWidgetWeather()'
        delete m_FontTempSml;

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
    if (m_MenuCategory == mcChannel) {
        if (Config.MenuChannelView == 0 || Config.MenuChannelView == 1 || Config.MenuChannelView == 2)
            m_ItemChannelHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
        else if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4)
            m_ItemChannelHeight = m_FontHeight + m_FontSmlHeight + m_MarginItem + Config.decorProgressMenuItemSize +
                                  Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
    } else if (m_MenuCategory == mcTimer) {
        if (Config.MenuTimerView == 0 || Config.MenuTimerView == 1)
            m_ItemTimerHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
        else if (Config.MenuTimerView == 2 || Config.MenuTimerView == 3)
            m_ItemTimerHeight = m_FontHeight + m_FontSmlHeight + m_MarginItem + Config.MenuItemPadding +
                                Config.decorBorderMenuItemSize * 2;
    } else if (m_MenuCategory == mcSchedule || m_MenuCategory == mcScheduleNow || m_MenuCategory == mcScheduleNext) {
        if (Config.MenuEventView == 0 || Config.MenuEventView == 1)
            m_ItemEventHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
        else if (Config.MenuEventView == 2 || Config.MenuEventView == 3)
            m_ItemEventHeight = m_FontHeight + m_FontSmlHeight + m_MarginItem * 2 + Config.MenuItemPadding +
                                Config.decorBorderMenuItemSize * 2 + Config.decorProgressMenuItemSize / 2;
    } else if (m_MenuCategory == mcRecording) {
        if (Config.MenuRecordingView == 0 || Config.MenuRecordingView == 1)
            m_ItemRecordingHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
        else if (Config.MenuRecordingView == 2 || Config.MenuRecordingView == 3)
            m_ItemRecordingHeight = m_FontHeight + m_FontSmlHeight + m_MarginItem + Config.MenuItemPadding +
                                    Config.decorBorderMenuItemSize * 2;
    } else if (m_MenuCategory == mcMain && Config.MainMenuWidgetsEnable) {
        DrawMainMenuWidgets();
    }
}

void cFlatDisplayMenu::SetScrollbar(int Total, int Offset) {
    DrawScrollbar(Total, Offset, MaxItems(), 0, ItemsHeight(), Offset > 0, Offset + MaxItems() < Total);
}

void cFlatDisplayMenu::DrawScrollbar(int Total, int Offset, int Shown, int Top, int Height, bool CanScrollUp,
                                     bool CanScrollDown, bool IsContent) {
    // dsyslog("flatPlus: Total: %d Offset: %d Shown: %d Top: %d Height: %d", Total, Offset, Shown, Top, Height);
    if (!MenuPixmap) return;

    if (Total > 0 && Total > Shown) {
        if (m_IsScrolling == false && m_ShowEvent == false && m_ShowRecording == false && m_ShowText == false) {
            m_IsScrolling = true;
            DecorBorderClearByFrom(BorderMenuItem);
            ItemBorderDrawAllWithScrollbar();
            ItemBorderClear();

            MenuItemScroller.UpdateViewPortWidth(m_ScrollBarWidth);

            if (IsContent)
                MenuPixmap->DrawRectangle(cRect(m_MenuItemWidth - m_ScrollBarWidth + Config.decorBorderMenuContentSize,
                                                0, m_ScrollBarWidth + m_MarginItem, m_ScrollBarHeight),
                                          clrTransparent);
            else
                MenuPixmap->DrawRectangle(cRect(m_MenuItemWidth - m_ScrollBarWidth + Config.decorBorderMenuItemSize, 0,
                                                m_ScrollBarWidth + m_MarginItem, m_ScrollBarHeight),
                                          clrTransparent);
        }
    } else if (m_ShowEvent == false && m_ShowRecording == false && m_ShowText == false) {
        m_IsScrolling = false;
    }

    if (IsContent)
        ScrollbarDraw(ScrollbarPixmap,
                      m_MenuItemWidth - m_ScrollBarWidth + Config.decorBorderMenuContentSize * 2 + m_MarginItem, Top,
                      Height, Total, Offset, Shown, CanScrollUp, CanScrollDown);
    else
        ScrollbarDraw(ScrollbarPixmap,
                      m_MenuItemWidth - m_ScrollBarWidth + Config.decorBorderMenuItemSize * 2 + m_MarginItem, Top,
                      Height, Total, Offset, Shown, CanScrollUp, CanScrollDown);
}

void cFlatDisplayMenu::Scroll(bool Up, bool Page) {
    // Is the menu scrolling or content?
    if (ComplexContent.IsShown() && ComplexContent.IsScrollingActive() && ComplexContent.Scrollable()) {
        bool IsScrolled = ComplexContent.Scroll(Up, Page);
        if (IsScrolled) {
            DrawScrollbar(
                ComplexContent.ScrollTotal(), ComplexContent.ScrollOffset(), ComplexContent.ScrollShown(),
                ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ComplexContent.ScrollOffset() > 0,
                ComplexContent.ScrollOffset() + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);
        }
    } else {
        cSkinDisplayMenu::Scroll(Up, Page);
    }
}

int cFlatDisplayMenu::MaxItems(void) {
    if (m_MenuCategory == mcChannel)
        return m_ScrollBarHeight / m_ItemChannelHeight;
    if (m_MenuCategory == mcTimer)
        return m_ScrollBarHeight / m_ItemTimerHeight;
    if (m_MenuCategory == mcSchedule || m_MenuCategory == mcScheduleNow || m_MenuCategory == mcScheduleNext)
        return m_ScrollBarHeight / m_ItemEventHeight;
    if (m_MenuCategory == mcRecording)
        return m_ScrollBarHeight / m_ItemRecordingHeight;

    return m_ScrollBarHeight / m_ItemHeight;
}

int cFlatDisplayMenu::ItemsHeight(void) {
    if (m_MenuCategory == mcChannel)
        return MaxItems() * m_ItemChannelHeight - Config.MenuItemPadding;
    if (m_MenuCategory == mcTimer)
        return MaxItems() * m_ItemTimerHeight - Config.MenuItemPadding;
    if (m_MenuCategory == mcSchedule || m_MenuCategory == mcScheduleNow || m_MenuCategory == mcScheduleNext)
        return MaxItems() * m_ItemEventHeight - Config.MenuItemPadding;
    if (m_MenuCategory == mcRecording)
        return MaxItems() * m_ItemRecordingHeight - Config.MenuItemPadding;

    return MaxItems() * m_ItemHeight - Config.MenuItemPadding;
}

void cFlatDisplayMenu::Clear(void) {
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
    TopBarSetTitle(Title);
    m_LastTitle = Title;

    if (Config.TopBarMenuIconShow) {
        cString icon("");
        switch (m_MenuCategory) {
        case mcMain:
            TopBarSetTitle("");
            icon = cString::sprintf("menuIcons/%s", VDRLOGO);
            break;
        case mcSchedule:
        case mcScheduleNow:
        case mcScheduleNext:
            icon = "menuIcons/Schedule"; break;
        case mcChannel:
            icon = "menuIcons/Channels";
            if (Config.MenuChannelShowCount) {
                int ChanCount{0};
                LOCK_CHANNELS_READ;
                for (const cChannel *Channel = Channels->First(); Channel; Channel = Channels->Next(Channel)) {
                    if (!Channel->GroupSep())
                        ++ChanCount;
                }  // for
                cString NewTitle = cString::sprintf("%s (%d)", Title, ChanCount);
                TopBarSetTitle(*NewTitle);
            }  // Config.MenuChannelShowCount
            break;
        case mcTimer:
            icon = "menuIcons/Timers";
            if (Config.MenuTimerShowCount) {
                int TimerCount{0}, TimerActiveCount{0};
                LOCK_TIMERS_READ;
                for (const cTimer *Timer = Timers->First(); Timer; Timer = Timers->Next(Timer)) {
                    ++TimerCount;
                    if (Timer->HasFlags(tfActive))
                        ++TimerActiveCount;
                }  // for
                m_LastTimerCount = TimerCount;
                m_LastTimerActiveCount = TimerActiveCount;
                cString NewTitle = cString::sprintf("%s (%d/%d)", Title, TimerActiveCount, TimerCount);
                TopBarSetTitle(*NewTitle);
            }  // Config.MenuTimerShowCount
            break;
        case mcRecording:
            if (Config.MenuRecordingShowCount) {
                int RecCount {0}, RecNewCount {0};
                m_LastRecFolder = m_RecFolder;
                if (m_RecFolder != "" && m_LastItemRecordingLevel > 0) {
                    std::string RecFolder2("");
                    LOCK_RECORDINGS_READ;
                    for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
                        RecFolder2 = GetRecordingName(Rec, m_LastItemRecordingLevel - 1, true).c_str();
                        if (m_RecFolder == RecFolder2) {
                            ++RecCount;
                            if (Rec->IsNew())
                                ++RecNewCount;
                        }
                    }  // for
                } else {
                    LOCK_RECORDINGS_READ;
                    for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
                        ++RecCount;
                        if (Rec->IsNew())
                            ++RecNewCount;
                    }
                }
                cString NewTitle = cString::sprintf("%s (%d*/%d)", Title, RecNewCount, RecCount);  // Display (35*/56)
                if (Config.ShortRecordingCount) {
                    if (RecNewCount == 0)  // No new recordings
                        NewTitle = cString::sprintf("%s (%d)", Title, RecCount);
                    else if (RecNewCount == RecCount)  // Only new recordings
                        NewTitle = cString::sprintf("%s (%d*)", Title, RecNewCount);
                }
                TopBarSetTitle(*NewTitle);
            }  // Config.MenuRecordingShowCount
            /*
            if(RecordingsSortMode == rsmName)
                TopBarSetMenuIconRight("menuIcons/RecsSortName");
            else if(RecordingsSortMode == rsmTime)
                TopBarSetMenuIconRight("menuIcons/RecsSortDate");
            */
            icon = "menuIcons/Recordings"; break;
        case mcSetup:
            icon = "menuIcons/Setup"; break;
        case mcCommand:
            icon = "menuIcons/Commands"; break;
        case mcEvent:
            icon = "extraIcons/Info"; break;
        case mcRecordingInfo:
            icon = "extraIcons/PlayInfo"; break;
        default:
            break;
        }
        TopBarSetMenuIcon(*icon);

        if ((m_MenuCategory == mcRecording || m_MenuCategory == mcTimer) && Config.DiskUsageShow == 1 ||
            Config.DiskUsageShow == 2 || Config.DiskUsageShow == 3) {
            TopBarEnableDiskUsage();
        }
    }
}

void cFlatDisplayMenu::SetButtons(const char *Red, const char *Green, const char *Yellow, const char *Blue) {
    ButtonsSet(Red, Green, Yellow, Blue);
}

void cFlatDisplayMenu::SetMessage(eMessageType Type, const char *Text) {
    (Text) ? MessageSet(Type, Text) : MessageClear();
}

void cFlatDisplayMenu::SetItem(const char *Text, int Index, bool Current, bool Selectable) {
    if (!MenuPixmap || !MenuIconsPixmap) return;

    m_ShowEvent = false;
    m_ShowRecording = false;
    m_ShowText = false;

    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (m_MenuCategory == mcMain && Config.MainMenuWidgetsEnable)
        m_MenuItemWidth *= Config.MainMenuItemScale;

    if (m_IsScrolling)
        m_MenuItemWidth -= m_ScrollBarWidth;

    tColor ColorFg = Theme.Color(clrItemFont);
    tColor ColorBg = Theme.Color(clrItemBg);
    tColor ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextFont);
    if (Current) {
        ColorFg = Theme.Color(clrItemCurrentFont);
        ColorBg = Theme.Color(clrItemCurrentBg);
        ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextCurrentFont);

        IconTimerFull = ImgLoader.LoadIcon("text_timer_full_cur", m_FontHeight, m_FontHeight);
        IconArrowTurn = ImgLoader.LoadIcon("text_arrowturn_cur", m_FontHeight, m_FontHeight);
        IconRec = ImgLoader.LoadIcon("timerRecording_cur", m_FontHeight, m_FontHeight);
    } else if (Selectable) {
        ColorFg = Theme.Color(clrItemSelableFont);
        ColorBg = Theme.Color(clrItemSelableBg);

        IconTimerFull = ImgLoader.LoadIcon("text_timer_full_sel", m_FontHeight, m_FontHeight);
        IconArrowTurn = ImgLoader.LoadIcon("text_arrowturn_sel", m_FontHeight, m_FontHeight);
        IconRec = ImgLoader.LoadIcon("timerRecording_sel", m_FontHeight, m_FontHeight);
    } else {
        IconTimerFull = ImgLoader.LoadIcon("text_timer_full", m_FontHeight, m_FontHeight);
        IconArrowTurn = ImgLoader.LoadIcon("text_arrowturn", m_FontHeight, m_FontHeight);
        IconRec = ImgLoader.LoadIcon("timerRecording", m_FontHeight, m_FontHeight);
    }

    int y = Index * m_ItemHeight;
    if (y + m_ItemHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, m_FontHeight), ColorBg);

    int AvailableTextWidth = m_MenuItemWidth - m_ScrollBarWidth;
    int XOff {0}, xt {0};
    const char *s {nullptr};
    for (int i {0}; i < MaxTabs; ++i) {
        s = GetTabbedText(Text, i);
        if (s) {
            // from skinelchi
            xt = Tab(i);
            XOff = xt + Config.decorBorderMenuItemSize;

            // Check for timer info symbols: " !#>" (EPGSearch searchtimer)
            if (i == 0 && strlen(s) == 1 && strchr(" !#>", s[0])) {
                switch (s[0]) {
                case '>':
                    if (IconTimerFull)
                        MenuIconsPixmap->DrawImage(cPoint(XOff, y + (m_FontHeight - IconTimerFull->Height()) / 2),
                                                   *IconTimerFull);
                    break;
                case '#':
                    if (IconRec)
                        MenuIconsPixmap->DrawImage(cPoint(XOff, y + (m_FontHeight - IconRec->Height()) / 2), *IconRec);
                    break;
                case '!':
                    if (IconArrowTurn)
                        MenuIconsPixmap->DrawImage(cPoint(XOff, y + (m_FontHeight - IconArrowTurn->Height()) / 2),
                                                   *IconArrowTurn);
                    break;
                // case ' ':
                default:
                    break;
                }
            } else {  // Not EPGsearch timermenu
                if ((m_MenuCategory == mcMain || m_MenuCategory == mcSetup) && Config.MenuItemIconsShow) {
                    cImageLoader ImgLoader;
                    cString IconName = GetIconName(MainMenuText(s));
                    cImage *img {nullptr};
                    if (Current) {
                        cString IconNameCur = cString::sprintf("%s_cur", *IconName);
                        img = ImgLoader.LoadIcon(*IconNameCur, m_FontHeight - m_MarginItem * 2,
                                                 m_FontHeight - m_MarginItem * 2);
                    }
                    if (!img)
                        img = ImgLoader.LoadIcon(*IconName, m_FontHeight - m_MarginItem * 2,
                                                 m_FontHeight - m_MarginItem * 2);

                    if (img) {
                        MenuIconsPixmap->DrawImage(
                            cPoint(xt + Config.decorBorderMenuItemSize + m_MarginItem, y + m_MarginItem), *img);
                    } else {
                        img = ImgLoader.LoadIcon("menuIcons/blank", m_FontHeight - m_MarginItem * 2,
                                                 m_FontHeight - m_MarginItem * 2);
                        if (img) {
                            MenuIconsPixmap->DrawImage(
                                cPoint(xt + Config.decorBorderMenuItemSize + m_MarginItem, y + m_MarginItem), *img);
                        }
                    }
                    MenuPixmap->DrawText(
                        cPoint(m_FontHeight + m_MarginItem * 2 + xt + Config.decorBorderMenuItemSize, y), s, ColorFg,
                        ColorBg, m_Font, AvailableTextWidth - xt - m_MarginItem * 2 - m_FontHeight);
                } else {
                    if (Config.MenuItemParseTilde) {
                        std::string tilde = s;
                        std::size_t found = tilde.find('~');  // Search for ~
                        if (found != std::string::npos) {
                            std::string first = tilde.substr(0, found);
                            std::string second = tilde.substr(found + 1);  // Default end is npos
                            RightTrim(first);                              // Trim possible space on right side
                            LeftTrim(second);                              // Trim possible space at begin

                            MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), first.c_str(), ColorFg,
                                                 ColorBg, m_Font,
                                                 m_MenuItemWidth - xt - Config.decorBorderMenuItemSize);
                            int l = m_Font->Width(first.c_str()) + m_Font->Width('X');
                            MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize + l, y), second.c_str(),
                                                 ColorExtraTextFg, ColorBg, m_Font,
                                                 m_MenuItemWidth - xt - Config.decorBorderMenuItemSize - l);
                        } else  // ~ not found
                            MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), s, ColorFg, ColorBg,
                                                 m_Font, m_MenuItemWidth - xt - Config.decorBorderMenuItemSize);
                    } else  // MenuItemParseTilde disabled
                        MenuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), s, ColorFg, ColorBg,
                                             m_Font, m_MenuItemWidth - xt - Config.decorBorderMenuItemSize);
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

    DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);

    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    SetEditableWidth(m_MenuWidth - Tab(1));
}

std::string cFlatDisplayMenu::MainMenuText(std::string Text) {
    std::string text = skipspace(Text.c_str());
    std::string MenuEntry("") /*, MenuNumber("")*/;
    MenuEntry.reserve(13);  // Length of 'Miscellaneous'
    bool found = false, DoBreak = false;
    char s;
    std::size_t i {0}, TextLength = text.length();
    for (; i < TextLength; ++i) {
        s = text.at(i);
        if (i == 0) {  // If text directly starts with nonnumeric, break
            if (!(s >= '0' && s <= '9')) break;
        }
        if (found) {
            if (!(s >= '0' && s <= '9')) DoBreak = true;
        }
        if (s >= '0' && s <= '9') found = true;
        if (DoBreak || i > 4) break;
    }
    if (found) {
        // MenuNumber = skipspace(text.substr(0, i).c_str());  // Unused?
        MenuEntry = skipspace(text.substr(i).c_str());
    } else {
        // MenuNumber = "";
        MenuEntry = text.c_str();
    }
    return MenuEntry;
}

cString cFlatDisplayMenu::GetIconName(std::string element) {
    // Check for standard menu entries
    std::string s("");
    s.reserve(16);  // Space for translated menu entry
    for (int i {0}; i < 16; ++i) {  // 16 menu entry's in vdr
        s = trVDR(items[i].c_str());
        if (s == element) {
            return cString::sprintf("menuIcons/%s", items[i].c_str());
        }
    }
    // Check for special main menu entries "stop recording", "stop replay"
    std::string StopRecording = skipspace(trVDR(" Stop recording "));
    std::string StopReplay = skipspace(trVDR(" Stop replaying"));
    try {
        if (element.substr(0, StopRecording.size()) == StopRecording)
            return "menuIcons/StopRecording";
        if (element.substr(0, StopReplay.size()) == StopReplay)
            return "menuIcons/StopReplay";
    } catch (...) {
    }
    // Check for plugins
    const char *MaimMenuEntry {nullptr};
    std::string PlugMainEntry("");
    PlugMainEntry.reserve(16);  // Space for menu entry
    for (int i {0};; ++i) {
        cPlugin *p = cPluginManager::GetPlugin(i);
        if (p) {
            MaimMenuEntry = p->MainMenuEntry();
            if (MaimMenuEntry) {
                PlugMainEntry = MaimMenuEntry;
                try {
                    if (element.substr(0, PlugMainEntry.size()) == PlugMainEntry) {
                        return cString::sprintf("pluginIcons/%s", p->Name());
                    }
                } catch (...) {
                }
            }
        } else
            break;
    }
    return cString::sprintf("extraIcons/%s", element.c_str());
}

bool cFlatDisplayMenu::CheckProgressBar(const char *text) {
    if (strlen(text) > 5 && text[0] == '[' && ((text[1] == '|')
        || (text[1] == ' ')) && ((text[2] == '|')
        || (text[2] == ' ')) && text[strlen(text) - 1] == ']')
        return true;
    return false;
}

void cFlatDisplayMenu::DrawProgressBarFromText(cRect rec, cRect recBg, const char *bar, tColor ColorFg,
                                               tColor ColorBarFg, tColor ColorBg) {
    const char *p = bar + 1;
    bool IsProgressbar = true;
    int now {0}, total {0};
    for (; *p != ']'; ++p) {
        if (*p == ' ' || *p == '|') {
            ++total;
            if (*p == '|')
                ++now;
        } else {
            IsProgressbar = false;
            break;
        }
    }
    if (IsProgressbar) {
        double progress = now * 1.0f / total;
        ProgressBarDrawRaw(MenuPixmap, MenuPixmap, rec, recBg, progress * total, total, ColorFg, ColorBarFg, ColorBg,
                           Config.decorProgressMenuItemType, true);
    }
}

bool cFlatDisplayMenu::SetItemChannel(const cChannel *Channel, int Index, bool Current, bool Selectable,
                                      bool WithProvider) {
    if (!MenuPixmap || !MenuIconsPixmap || !MenuIconsBgPixmap) return false;

    if (Config.MenuChannelView == 0 || !Channel)
        return false;

    if (Current)
        MenuItemScroller.Clear();

    int Height = m_FontHeight;
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4) {  // flatPlus short, flatPlus short +EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem + Config.decorProgressMenuItemSize;
        m_MenuItemWidth *= 0.5;
    }

    if (m_IsScrolling)
        m_MenuItemWidth -= m_ScrollBarWidth;

    tColor ColorFg = Theme.Color(clrItemFont);
    tColor ColorBg = Theme.Color(clrItemBg);
    if (Current) {
        ColorFg = Theme.Color(clrItemCurrentFont);
        ColorBg = Theme.Color(clrItemCurrentBg);
    } else if (Selectable) {
        ColorFg = Theme.Color(clrItemSelableFont);
        ColorBg = Theme.Color(clrItemSelableBg);
    }

    int y = Index * m_ItemChannelHeight;
    if (y + m_ItemChannelHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemChannelHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);

    int Width {0}, LeftName {0};
    int Left = Config.decorBorderMenuItemSize + m_MarginItem;
    int Top = y;
    bool DrawProgress = true;

    m_IsGroup = Channel->GroupSep();  // Also used later
    if (m_IsGroup)
        DrawProgress = false;

    /* Disabled because invalid lock sequence
    LOCK_CHANNELS_READ;
    cString ws = cString::sprintf("%d", Channels->MaxNumber());
    int w = m_Font->Width(ws); */  // Try to fix invalid lock sequence (Only with scraper2vdr - Program)
    int w = m_Font->Width("9999");  // At least four digits in channel list because of different sort modes
    cString Buffer("");
    if (!m_IsGroup) {
        Buffer = cString::sprintf("%d", Channel->Number());
        Width = m_Font->Width(*Buffer);
    }

    if (Width < w)
        Width = w;  // Minimal width for channel number

    MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, Width, m_FontHeight, taRight);
    Left += Width + m_MarginItem;

    int ImageHeight = m_FontHeight;
    int ImageLeft = Left;
    int ImageTop = Top;
    int ImageBgHeight = ImageHeight;
    int ImageBgWidth = ImageHeight * 1.34;

    cImage *img {nullptr};
    if (!m_IsGroup) {
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
        Left += ImageBgWidth + m_MarginItem * 2;
    } else {
        bool IsRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;

        if (IsRadioChannel) {
            if (Current)
                img = ImgLoader.LoadIcon("radio_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img)
                img = ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);

            if (img) {
                ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                Left += ImageBgWidth + m_MarginItem * 2;
            }
        } else if (m_IsGroup) {
            img = ImgLoader.LoadIcon("changroup", ImageBgWidth - 10, ImageBgHeight - 10);
            if (img) {
                ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                Left += ImageBgWidth + m_MarginItem * 2;
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
                Left += ImageBgWidth + m_MarginItem * 2;
            }
        }
    }

    LeftName = Left;

    // Event from channel
    const cEvent *Event {nullptr};
    LOCK_SCHEDULES_READ;
    const cSchedule *Schedule = Schedules->GetSchedule(Channel);
    float progress {0.0};
    cString EventTitle("");
    if (Schedule) {
        Event = Schedule->GetPresentEvent();
        if (Event) {
            // Calculate progress bar
            progress = roundf((time(NULL) * 1.0f - Event->StartTime()) / Event->Duration() * 100.0f);
            if (progress < 0) progress = 0.0;
            else if (progress > 100) progress = 100;

            EventTitle = Event->Title();
        }
    }

    if (WithProvider)
        Buffer = cString::sprintf("%s - %s", Channel->Provider(), Channel->Name());
    else
        Buffer = cString::sprintf("%s", Channel->Name());

    if (Config.MenuChannelView == 1) {  // flatPlus long
        Width = m_MenuItemWidth - LeftName;
        if (m_IsGroup) {
            int LineTop = Top + (m_FontHeight - 3) / 2;
            MenuPixmap->DrawRectangle(cRect(Left, LineTop, m_MenuItemWidth - Left, 3), ColorFg);
            cString GroupName = cString::sprintf(" %s ", *Buffer);
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
        Width = m_MenuItemWidth / 10 * 2;
        if (m_IsScrolling)
            Width = (m_MenuItemWidth + m_ScrollBarWidth) / 10 * 2;

        if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4)  // flatPlus short, flatPlus short + EPG
            Width = m_MenuItemWidth - LeftName;

        if (m_IsGroup) {
            int LineTop = Top + (m_FontHeight - 3) / 2;
            MenuPixmap->DrawRectangle(cRect(Left, LineTop, m_MenuItemWidth - Left, 3), ColorFg);
            cString GroupName = cString::sprintf(" %s ", *Buffer);
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
                int PBTop = y + (m_ItemChannelHeight - Config.MenuItemPadding) / 2 -
                            Config.decorProgressMenuItemSize / 2 - Config.decorBorderMenuItemSize;
                int PBLeft = Left;
                int PBWidth = m_MenuItemWidth / 10;
                if (Config.MenuChannelView == 3 ||
                    Config.MenuChannelView == 4) {  // flatPlus short, flatPlus short + EPG
                    PBTop = Top + m_FontHeight + m_FontSmlHeight;
                    PBLeft = Left - Width - m_MarginItem;
                    PBWidth = m_MenuItemWidth - LeftName - m_MarginItem * 2 - Config.decorBorderMenuItemSize -
                              m_ScrollBarWidth;

                    if (m_IsScrolling)
                        PBWidth += m_ScrollBarWidth;
                }

                Width = m_MenuItemWidth / 10;
                if (m_IsScrolling)
                    Width = (m_MenuItemWidth + m_ScrollBarWidth) / 10;
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

    DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);

    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    if (Config.MenuChannelView == 4 && Current)
        DrawItemExtraEvent(Event, "");

    return true;
}

void cFlatDisplayMenu::DrawItemExtraEvent(const cEvent *Event, cString EmptyText) {
    m_cLeft = m_MenuItemWidth + Config.decorBorderMenuItemSize * 2 + Config.decorBorderMenuContentSize + m_MarginItem;
    if (m_IsScrolling)
        m_cLeft += m_ScrollBarWidth;

    m_cTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
    m_cWidth = m_MenuWidth - m_cLeft - Config.decorBorderMenuContentSize;
    m_cHeight =
        m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                       Config.decorBorderButtonSize * 2 + m_MarginItem * 3 + Config.decorBorderMenuContentSize * 2);

    bool IsEmpty = false;
    cString Text("");
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
                cString Audio(""), Subtitle("");
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
        cImage *img = ImgLoader.LoadIcon("timerInactiveBig", 256, 256);
        if (img) {
            ComplexContent.AddImage(img, cRect(m_MarginItem, m_MarginItem, img->Width(), img->Height()));
            ComplexContent.AddText(*Text, true,
                                   cRect(m_MarginItem, m_MarginItem + img->Height(), m_cWidth - m_MarginItem * 2,
                                         m_cHeight - m_MarginItem * 2),
                                   Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
        }
    } else {
        cString MediaPath("");
        int MediaWidth {0}, MediaHeight {0};
        int MediaType {0};

        static cPlugin *pScraper = GetScraperPlugin();
        if (Config.TVScraperEPGInfoShowPoster && pScraper) {
            ScraperGetPosterBannerV2 call;
            call.event = Event;
            call.recording = NULL;
            if (pScraper->Service("GetPosterBannerV2", &call)) {
                if ((call.type == tSeries) && call.banner.path.size() > 0) {
                    MediaWidth = m_cWidth - m_MarginItem * 2;
                    MediaHeight = 999;
                    MediaPath = call.banner.path.c_str();
                    MediaType = 1;
                } else if (call.type == tMovie && call.poster.path.size() > 0) {
                    MediaWidth = m_cWidth / 2 - m_MarginItem * 3;
                    MediaHeight = 999;
                    MediaPath = call.poster.path.c_str();
                    MediaType = 2;
                }
            }
        }

        if (!isempty(*MediaPath)) {
            cImage *img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);
            if (img && MediaType == 2) {
                ComplexContent.AddImageWithFloatedText(
                    img, CIP_Right, *Text,
                    cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
            } else if (img && MediaType == 1) {
                ComplexContent.AddImage(img, cRect(m_MarginItem, m_MarginItem, img->Width(), img->Height()));
                ComplexContent.AddText(*Text, true,
                                       cRect(m_MarginItem, m_MarginItem + img->Height(), m_cWidth - m_MarginItem * 2,
                                             m_cHeight - m_MarginItem * 2),
                                       Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
            } else {
                ComplexContent.AddText(
                    *Text, true,
                    cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
            }
        } else {
            ComplexContent.AddText(
                *Text, true,
                cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);
        }
    }
    ComplexContent.CreatePixmaps(Config.MenuContentFullSize);
    ComplexContent.Draw();

    DecorBorderClearByFrom(BorderContent);
    if (Config.MenuContentFullSize)
        DecorBorderDraw(m_cLeft, m_cTop, m_cWidth, ComplexContent.ContentHeight(true),
                        Config.decorBorderMenuContentSize, Config.decorBorderMenuContentType,
                        Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg, BorderContent);
    else
        DecorBorderDraw(m_cLeft, m_cTop, m_cWidth, ComplexContent.ContentHeight(false),
                        Config.decorBorderMenuContentSize, Config.decorBorderMenuContentType,
                        Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg, BorderContent);
}

bool cFlatDisplayMenu::SetItemTimer(const cTimer *Timer, int Index, bool Current, bool Selectable) {
    if (!MenuPixmap || !MenuIconsPixmap || !MenuIconsOvlPixmap || !MenuIconsBgPixmap) return false;

    if (Config.MenuTimerView == 0 || !Timer)
        return false;

    const cChannel *Channel = Timer->Channel();
    const cEvent *Event = Timer->Event();

    if (Current)
        MenuItemScroller.Clear();

    int Height = m_FontHeight;
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuTimerView == 2 || Config.MenuTimerView == 3) {  // flatPlus short, flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem;
        m_MenuItemWidth *= 0.5;
    }

    if (m_IsScrolling)
        m_MenuItemWidth -= m_ScrollBarWidth;

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

    int y = Index * m_ItemTimerHeight;
    if (y + m_ItemTimerHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemTimerHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);
    int Left = Config.decorBorderMenuItemSize + m_MarginItem;
    int Top = y;

    int ImageHeight = m_FontHeight;
    int ImageLeft = Left;
    int ImageTop = Top;

    cString IconName("");
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

    cImage *img = ImgLoader.LoadIcon(*IconName, ImageHeight, ImageHeight);
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
    Left += ImageHeight + m_MarginItem * 2;

    /* Disabled because invalid lock sequence
    LOCK_CHANNELS_READ;
    cString ws = cString::sprintf("%d", Channels->MaxNumber());
    int w = m_Font->Width(ws); */
    int w = m_Font->Width("999");  // Try to fix invalid lock sequence (Only with scraper2vdr - Program)
    cString Buffer = cString::sprintf("%d", Channel->Number());
    int Width = m_Font->Width(*Buffer);
    if (Width < w)
        Width = w;  // Minimal width for channel number

    MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, Width, m_FontHeight, taRight);
    Left += Width + m_MarginItem;

    ImageLeft = Left;
    int ImageBgHeight = ImageHeight;
    int ImageBgWidth = ImageHeight * 1.34;
    img = ImgLoader.LoadIcon("logo_background", ImageBgWidth, ImageBgHeight);
    if (img) {
        ImageBgHeight = img->Height();
        ImageBgWidth = img->Width();
        ImageTop = Top + (m_FontHeight - ImageBgHeight) / 2;
        MenuIconsBgPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    }

    img = ImgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
    if (img) {
        ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
        ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
        MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    } else {
        bool IsRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;

        if (IsRadioChannel) {
            if (Current)
                img = ImgLoader.LoadIcon("radio_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img)
                img = ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);

            if (img) {
                ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
            }
            /* } else if (Channel->GroupSep()) {  //? Is GroupSep() in SetItemTimer() possible/needed?
                img = ImgLoader.LoadIcon("changroup", ImageBgWidth - 10, ImageBgHeight - 10);
                if (img) {
                    ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                    ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                    MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                } */
        } else {
            if (Current)
                img = ImgLoader.LoadIcon("tv_cur", ImageBgWidth - 10, ImageBgHeight - 10);
            if (!img)
                img = ImgLoader.LoadIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);

            if (img) {
                ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
            }
        }
    }
    Left += ImageBgWidth + m_MarginItem * 2;

    cString day(""), name("");
    if (Timer->WeekDays()) {
        day = Timer->PrintDay(0, Timer->WeekDays(), false);
    } else if (Timer->Day() - time(NULL) < 28 * SECSINDAY) {
        day = itoa(Timer->GetMDay(Timer->Day()));
        name = WeekDayName(Timer->Day());
    } else {
        struct tm tm_r;
        time_t Day = Timer->Day();
        localtime_r(&Day, &tm_r);
        char Buffer[16];
        strftime(Buffer, sizeof(Buffer), "%Y%m%d", &tm_r);
        day = Buffer;
    }
    const char *File = Setup.FoldersInTimerMenu ? NULL : strrchr(Timer->File(), FOLDERDELIMCHAR);
    if (File && strcmp(File + 1, TIMERMACRO_TITLE) && strcmp(File + 1, TIMERMACRO_EPISODE))
        ++File;
    else
        File = Timer->File();

    if (Config.MenuTimerView == 1) {  // flatPlus long
        Buffer = cString::sprintf("%s%s%s.", *name, *name && **name ? " " : "", *day);
        MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                             m_MenuItemWidth - Left - m_MarginItem);
        Left += m_Font->Width("XXX 99.  ");
        Buffer = cString::sprintf("%02d:%02d - %02d:%02d", Timer->Start() / 100, Timer->Start() % 100,
                                  Timer->Stop() / 100, Timer->Stop() % 100);
        MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                             m_MenuItemWidth - Left - m_MarginItem);
        Left += m_Font->Width("99:99 - 99:99  ");

        if (Current && m_Font->Width(File) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
            MenuItemScroller.AddScroller(
                File, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight), ColorFg,
                clrTransparent, m_Font, ColorExtraTextFg);
        } else {
            if (Config.MenuItemParseTilde) {
                std::string tilde = File;
                std::size_t found = tilde.find('~');  // Search for ~
                if (found != std::string::npos) {
                    std::string first = tilde.substr(0, found);
                    std::string second = tilde.substr(found + 1);  // Default end is npos
                    RightTrim(first);                              // Trim possible space on right side
                    LeftTrim(second);                              // Trim possible space at begin

                    MenuPixmap->DrawText(cPoint(Left, Top), first.c_str(), ColorFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
                    int l = m_Font->Width(first.c_str()) + m_Font->Width('X');
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
                                  Timer->Start() / 100, Timer->Start() % 100, Timer->Stop() / 100, Timer->Stop() % 100);
        MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                             m_MenuItemWidth - Left - m_MarginItem);

        if (Current && m_FontSml->Width(File) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
            MenuItemScroller.AddScroller(File,
                                         cRect(Left, Top + m_FontHeight + m_MenuTop,  // TODO: Mismatch when scrolling
                                               m_MenuItemWidth - Left - m_MarginItem - m_ScrollBarWidth,
                                               m_FontSmlHeight),
                                         ColorFg, clrTransparent, m_FontSml, ColorExtraTextFg);
        } else {
            if (Config.MenuItemParseTilde) {
                std::string tilde = File;
                std::size_t found = tilde.find('~');  // Search for ~
                if (found != std::string::npos) {
                    std::string first = tilde.substr(0, found);
                    std::string second = tilde.substr(found + 1);  // Default end is npos
                    RightTrim(first);                              // Trim possible space on right side
                    LeftTrim(second);                              // Trim possible space at begin

                    MenuPixmap->DrawText(cPoint(Left, Top + m_FontHeight), first.c_str(), ColorFg, ColorBg, m_FontSml,
                                         m_MenuItemWidth - Left - m_MarginItem);
                    int l = m_FontSml->Width(first.c_str()) + m_FontSml->Width('X');
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

    DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);

    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    if (Config.MenuTimerView == 3 && Current)  // flatPlus short
        DrawItemExtraEvent(Event, tr("timer not enabled"));

    return true;
}

bool cFlatDisplayMenu::SetItemEvent(const cEvent *Event, int Index, bool Current, bool Selectable,
                                    const cChannel *Channel, bool WithDate, eTimerMatch TimerMatch, bool TimerActive) {
    if (!MenuPixmap || !MenuIconsBgPixmap || !MenuIconsPixmap) return false;

    if (Config.MenuEventView == 0)
        return false;

    if (Config.MenuEventViewAlwaysWithDate)
        WithDate = true;

    if (Current)
        MenuItemScroller.Clear();

    int Height = m_FontHeight;
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuEventView == 2 || Config.MenuEventView == 3) {  // flatPlus short. flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem * 2 + Config.decorProgressMenuItemSize / 2;
        m_MenuItemWidth *= 0.6;
    }

    if (m_IsScrolling)
        m_MenuItemWidth -= m_ScrollBarWidth;

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

    int y = Index * m_ItemEventHeight;
    if (y + m_ItemEventHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemEventHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);

    int Left = Config.decorBorderMenuItemSize + m_MarginItem;
    int LeftSecond = Left;
    int Top = y, w {0};
    int ImageTop = Top;

    if (!Channel)
        TopBarSetMenuLogo(m_ItemEventLastChannelName);

    cString Buffer("");
    cImage *img {nullptr};
    if (Channel) {
        if (Current)
            m_ItemEventLastChannelName = Channel->Name();

        /* Disabled because invalid lock sequence
            LOCK_CHANNELS_READ;
            cString ws = cString::sprintf("%d", Channels->MaxNumber());
            int w = m_Font->Width(ws); */
        w = m_Font->Width("9999");  // Try to fix invalid lock sequence (Only with scraper2vdr - Program)
        m_IsGroup = Channel->GroupSep();
        if (!m_IsGroup) {
            Buffer = cString::sprintf("%d", Channel->Number());
            int Width = m_Font->Width(*Buffer);  // w is used here for calculation of width
            if (Width > w)
                w = Width;  // Minimal width for channel number in Event (epgSearch)

            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, w, m_FontHeight, taRight);
        }
        Left += w + m_MarginItem;

        int ImageLeft = Left;
        int ImageBgHeight = m_FontHeight;
        int ImageBgWidth = m_FontHeight * 1.34;
        if (!m_IsGroup) {
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
        } else {
            bool IsRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;

            if (IsRadioChannel) {
                if (Current)
                    img = ImgLoader.LoadIcon("radio_cur", ImageBgWidth - 10, ImageBgHeight - 10);
                if (!img)
                    img = ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);

                if (img) {
                    ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                    ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                    MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                }
            } else if (m_IsGroup) {
                img = ImgLoader.LoadIcon("changroup", ImageBgWidth - 10, ImageBgHeight - 10);
                if (img) {
                    ImageTop = Top + (ImageBgHeight - img->Height()) / 2;
                    ImageLeft = Left + (ImageBgWidth - img->Width()) / 2;
                    MenuIconsPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
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
                }
            }
        }
        Left += ImageBgWidth + m_MarginItem * 2;
        LeftSecond = Left;

        w = m_MenuItemWidth / 10 * 2;
        if (!m_IsScrolling)
            w = (m_MenuItemWidth - m_ScrollBarWidth) / 10 * 2;

        cString ChannelName("");
        if (Config.MenuEventView == 2 || Config.MenuEventView == 3) {  // flatPlus short, flatPlus short + EPG
            ChannelName = Channel->Name();
            w = m_Font->Width(*ChannelName);
        } else
            ChannelName = Channel->ShortName(true);

        if (m_IsGroup) {
            int LineTop = Top + (m_FontHeight - 3) / 2;
            MenuPixmap->DrawRectangle(cRect(Left, LineTop, m_MenuItemWidth - Left, 3), ColorFg);
            Left += w / 2;
            cString GroupName = cString::sprintf(" %s ", *ChannelName);
            MenuPixmap->DrawText(cPoint(Left, Top), *GroupName, ColorFg, ColorBg, m_Font, 0, 0, taCenter);
        } else
            MenuPixmap->DrawText(cPoint(Left, Top), *ChannelName, ColorFg, ColorBg, m_Font, w);

        Left += w + m_MarginItem * 2;

        if (Event) {
            int PBWidth = m_MenuItemWidth / 20;
            if (!m_IsScrolling)
                PBWidth = (m_MenuItemWidth - m_ScrollBarWidth) / 20;

            time_t now = time(NULL);
            if ((now >= (Event->StartTime() - 2 * 60))) {
                int total = Event->EndTime() - Event->StartTime();
                if (total >= 0) {
                    // Calculate progress bar
                    double progress = roundf((now * 1.0f - Event->StartTime()) / Event->Duration() * 100.0f);
                    if (progress < 0) progress = 0.0;
                    else if (progress > 100) progress = 100;

                    int PBTop = y + (m_ItemEventHeight - Config.MenuItemPadding) / 2 -
                                Config.decorProgressMenuItemSize / 2 - Config.decorBorderMenuItemSize;
                    int PBLeft = Left;
                    int PBHeight = Config.decorProgressMenuItemSize;

                    if ((Config.MenuEventView == 2 || Config.MenuEventView == 3)) {
                        // flatPlus short, flatPlus short + EPG
                        PBTop = y + m_FontHeight + m_FontSmlHeight + m_MarginItem;
                        PBWidth = m_MenuItemWidth - LeftSecond - m_ScrollBarWidth - m_MarginItem * 2;
                        if (m_IsScrolling)
                            PBWidth += m_ScrollBarWidth;

                        PBLeft = LeftSecond;
                        PBHeight = Config.decorProgressMenuItemSize / 2;
                    }

                    if (Current)
                        ProgressBarDrawRaw(MenuPixmap, MenuPixmap, cRect(PBLeft, PBTop, PBWidth, PBHeight),
                                           cRect(PBLeft, PBTop, PBWidth, PBHeight), progress, 100,
                                           Config.decorProgressMenuItemCurFg, Config.decorProgressMenuItemCurBarFg,
                                           Config.decorProgressMenuItemCurBg, Config.decorProgressMenuItemType, false);
                    else
                        ProgressBarDrawRaw(MenuPixmap, MenuPixmap, cRect(PBLeft, PBTop, PBWidth, PBHeight),
                                           cRect(PBLeft, PBTop, PBWidth, PBHeight), progress, 100,
                                           Config.decorProgressMenuItemFg, Config.decorProgressMenuItemBarFg,
                                           Config.decorProgressMenuItemBg, Config.decorProgressMenuItemType, false);
                }
            }
            Left += PBWidth + m_MarginItem * 2;
        }
    }  // if channel

    if (WithDate && Event && Selectable) {
        struct tm tm_r;
        time_t Day = Event->StartTime();
        localtime_r(&Day, &tm_r);
        char buf[8];
        strftime(buf, sizeof(buf), "%2d", &tm_r);

        cString DateString = cString::sprintf("%s %s. ", *WeekDayName((time_t)Event->StartTime()), buf);
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

    int ImageHeight = m_FontHeight;
    if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Channel && Event && Selectable) {
        // flatPlus short, flatPlus short + EPG
        Top += m_FontHeight;
        Left = LeftSecond;
        ImageHeight = m_FontSmlHeight;
        MenuPixmap->DrawText(cPoint(Left, Top), Event->GetTimeString(), ColorFg, ColorBg, m_FontSml);
        Left += m_FontSml->Width(Event->GetTimeString()) + m_MarginItem;
    /* } else if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Event && Selectable) {
            MenuPixmap->DrawText(cPoint(Left, Top), Event->GetTimeString(), ColorFg, ColorBg, m_Font);
            Left += m_Font->Width(Event->GetTimeString()) + m_MarginItem; */
    } else if (Event && Selectable) {  //? Same as above
        MenuPixmap->DrawText(cPoint(Left, Top), Event->GetTimeString(), ColorFg, ColorBg, m_Font);
        Left += m_Font->Width(Event->GetTimeString()) + m_MarginItem;
    }

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

        cString Title = cString::sprintf("%s", Event->Title());
        cString ShortText = Event->ShortText() ? cString::sprintf("%s", Event->ShortText()) : "";
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
        try {
            // Extract date from separator
            std::string sep = Event->Title();
            if (sep.size() > 12) {
                std::size_t found = sep.find(" -");
                if (found >= 10) {
                    std::string date = sep.substr(found - 10, 10);
                    int LineTop = Top + (m_FontHeight - 3) / 2;
                    MenuPixmap->DrawRectangle(cRect(0, LineTop, m_MenuItemWidth, 3), ColorFg);
                    cString DateSpace = cString::sprintf(" %s ", date.c_str());
                    MenuPixmap->DrawText(cPoint(LeftSecond + m_MenuWidth / 10 * 2, Top), *DateSpace, ColorFg, ColorBg,
                                         m_Font, 0, 0, taCenter);
                } else
                    MenuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, m_Font,
                                         m_MenuItemWidth - Left - m_MarginItem);
            } else
                MenuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
        } catch (...) {
            MenuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, m_Font,
                                 m_MenuItemWidth - Left - m_MarginItem);
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

    DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);
    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    if (Config.MenuEventView == 3 && Current)
        DrawItemExtraEvent(Event, "");

    return true;
}

bool cFlatDisplayMenu::SetItemRecording(const cRecording *Recording, int Index, bool Current, bool Selectable,
                                        int Level, int Total, int New) {
    if (!MenuPixmap || !MenuIconsPixmap || !MenuIconsOvlPixmap) return false;

    if (Config.MenuRecordingView == 0)
        return false;

    if (Level > 0)
        m_RecFolder = GetRecordingName(Recording, Level - 1, true).c_str();
    else
        m_RecFolder = "";

    m_LastItemRecordingLevel = Level;

    if (m_LastRecFolder != m_RecFolder) {
        int RecCount {0}, RecNewCount {0};
        m_LastRecFolder = m_RecFolder;
        if (m_RecFolder != "" && m_LastItemRecordingLevel > 0) {
            std::string RecFolder2("");
            LOCK_RECORDINGS_READ;
            for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
                RecFolder2 = GetRecordingName(Rec, m_LastItemRecordingLevel - 1, true).c_str();
                if (m_RecFolder == RecFolder2) {
                    ++RecCount;
                    if (Rec->IsNew())
                        ++RecNewCount;
                }
            }  // for
        } else {
            LOCK_RECORDINGS_READ;
            for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
                ++RecCount;
                if (Rec->IsNew())
                    ++RecNewCount;
            }  // for
        }
        cString NewTitle = cString::sprintf("%s (%d*/%d)", *m_LastTitle, RecNewCount, RecCount);
        TopBarSetTitleWithoutClear(*NewTitle);
    }

    if (Current)
        MenuItemScroller.Clear();

    int Height = m_FontHeight;
    m_MenuItemWidth = m_MenuWidth - Config.decorBorderMenuItemSize * 2;
    if (Config.MenuRecordingView == 2 || Config.MenuRecordingView == 3) {  // flatPlus short, flatPlus short + EPG
        Height = m_FontHeight + m_FontSmlHeight + m_MarginItem;
        m_MenuItemWidth *= 0.5;
    }

    if (m_IsScrolling)
        m_MenuItemWidth -= m_ScrollBarWidth;

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

    int y = Index * m_ItemRecordingHeight;
    if (y + m_ItemRecordingHeight > m_MenuItemLastHeight)
        m_MenuItemLastHeight = y + m_ItemRecordingHeight;

    MenuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, m_MenuItemWidth, Height), ColorBg);
    //* Preload for calculation of position
    cImage *ImgRecCut {nullptr}, *ImgRecNew {nullptr}, *ImgRecNewSml {nullptr};
    if (Current) {
        ImgRecNew = ImgLoader.LoadIcon("recording_new_cur", m_FontHeight, m_FontHeight);
        ImgRecNewSml = ImgLoader.LoadIcon("recording_new_cur", m_FontSmlHeight, m_FontSmlHeight);
        ImgRecCut = ImgLoader.LoadIcon("recording_cutted_cur", m_FontHeight, m_FontHeight * (1.0 / 3.0 * 2.0));
    }
    if (!ImgRecNew)
        ImgRecNew = ImgLoader.LoadIcon("recording_new", m_FontHeight, m_FontHeight);
    if (!ImgRecNewSml)
        ImgRecNewSml = ImgLoader.LoadIcon("recording_new", m_FontSmlHeight, m_FontSmlHeight);
    if (!ImgRecCut)
        ImgRecCut = ImgLoader.LoadIcon("recording_cutted", m_FontHeight, m_FontHeight *  (1.0 / 3.0 * 2.0));

    int Left = Config.decorBorderMenuItemSize + m_MarginItem;
    int Top = y;

    cString Buffer(""), IconName("");
    cString RecName = GetRecordingName(Recording, Level, Total == 0).c_str();
    cImage *img {nullptr};
    if (Config.MenuRecordingView == 1) {  // flatPlus long
        int LeftWidth = Left + m_FontHeight + ImgRecNew->Width() + ImgRecCut->Width() + m_MarginItem * 3 +
                        m_Font->Width("99.99.99  99:99   99:99 ");

        if (Total == 0) {
            if (Current)
                img = ImgLoader.LoadIcon("recording_cur", m_FontHeight, m_FontHeight);
            if (!img)
                img = ImgLoader.LoadIcon("recording", m_FontHeight, m_FontHeight);
            if (img) {
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                Left += m_FontHeight + m_MarginItem;
            }

            int Minutes = (Recording->LengthInSeconds() + 30) / 60;
            cString Length = cString::sprintf("%02d:%02d", Minutes / 60, Minutes % 60);
            Buffer = cString::sprintf("%s  %s   %s ", *ShortDateString(Recording->Start()),
                                      *TimeString(Recording->Start()), *Length);

            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                                 m_MenuItemWidth - Left - m_MarginItem);

            Left += m_Font->Width(*Buffer);

            // Show if recording is still in progress (ruTimer), or played (ruReplay)
            int RecordingIsInUse = Recording->IsInUse();
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
                IconName = GetRecordingseenIcon(Recording->NumFrames(), Recording->GetResume());

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
                const cRecordingInfo *RecInfo = Recording->Info();
                IconName = GetRecordingerrorIcon(RecInfo->Errors());

                img = nullptr;
                if (Current) {
                    cString IconNameCur = cString::sprintf("%s_cur", *IconName);
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
                int ImageTop = Top + m_FontHeight - ImgRecCut->Height();  // ~ 2/3 height
                MenuIconsPixmap->DrawImage(cPoint(Left, ImageTop), *ImgRecCut);
            }
            if (Config.MenuItemRecordingShowFormatIcons) {
                IconName = GetRecordingFormatIcon(Recording);  // Show (SD), HD or UHD Logo
                if (!isempty(*IconName)) {
                    int ImageHeight = m_FontHeight * (1.0 / 3.0);  // 1/3 image height
                    img = ImgLoader.LoadIcon(*IconName, 999, ImageHeight);
                        if (img) {
                            // int FontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);
                            int ImageTop = Top + m_FontHeight - m_FontAscender;
                            int ImageLeft = Left + m_FontHeight - img->Width();
                            MenuIconsOvlPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                        }
                }
            }

            Left += (ImgRecCut->Width() * 1.5f) + m_MarginItem;  // 0.666 * 1.5 = 0.999

            if (Current && m_Font->Width(*RecName) > (m_MenuItemWidth - Left - m_MarginItem) && Config.ScrollerEnable) {
                MenuItemScroller.AddScroller(
                    *RecName, cRect(Left, Top + m_MenuTop, m_MenuItemWidth - Left - m_MarginItem, m_FontHeight),
                    ColorFg, clrTransparent, m_Font);
            } else {
                MenuPixmap->DrawText(cPoint(Left, Top), *RecName, ColorFg, ColorBg, m_Font,
                                     m_MenuItemWidth - Left - m_MarginItem);
            }
        } else if (Total > 0) {  // Folder
            img = nullptr;
            if (Current)
                img = ImgLoader.LoadIcon("folder_cur", m_FontHeight, m_FontHeight);
            if (!img)
                img = ImgLoader.LoadIcon("folder", m_FontHeight, m_FontHeight);
            if (img) {
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                Left += img->Width() + m_MarginItem;
            }

            Buffer = cString::sprintf("%d  ", Total);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font, m_Font->Width("  999"),
                                 m_FontHeight, taLeft);
            Left += m_Font->Width("  999 ");

            if (ImgRecNew)
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *ImgRecNew);

            Left += ImgRecNew->Width() + m_MarginItem;
            Buffer = cString::sprintf("%d", New);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_Font,
                                 m_MenuItemWidth - Left - m_MarginItem);
            Left += m_Font->Width(" 999 ");
            if (Config.MenuItemRecordingShowFolderDate != 0) {
                Buffer = cString::sprintf("(%s) ", *ShortDateString(GetLastRecTimeFromFolder(Recording, Level)));
                MenuPixmap->DrawText(
                    cPoint(LeftWidth - m_Font->Width(Buffer) - m_FontHeight * 2 - m_MarginItem * 2, Top), *Buffer,
                    ColorExtraTextFg, ColorBg, m_Font);
                if (IsRecordingOld(Recording, Level)) {
                    Left = LeftWidth - m_FontHeight * 2 - m_MarginItem * 2;
                    img = nullptr;
                    if (Current)
                        img = ImgLoader.LoadIcon("recording_old_cur", m_FontHeight, m_FontHeight);
                    else
                        img = ImgLoader.LoadIcon("recording_old", m_FontHeight, m_FontHeight);
                    if (img) {
                        MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                        Left += img->Width() + m_MarginItem;
                    }
                }
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
            LeftWidth += m_Font->Width(*RecName) + m_MarginItem * 2;
        } else if (Total == -1) {
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

            int ImagesWidth = ImgRecNew->Width() + ImgRecCut->Width() + m_MarginItem * 2 + m_ScrollBarWidth;
            if (m_IsScrolling)
                ImagesWidth -= m_ScrollBarWidth;

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

            int Minutes = (Recording->LengthInSeconds() + 30) / 60;
            cString Length = cString::sprintf("%02d:%02d", Minutes / 60, Minutes % 60);
            Buffer = cString::sprintf("%s  %s   %s ", *ShortDateString(Recording->Start()),
                                      *TimeString(Recording->Start()), *Length);

            Top += m_FontHeight;
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml,
                                 m_MenuItemWidth - Left - m_MarginItem);

            Top -= m_FontHeight;
            Left = m_MenuItemWidth - ImagesWidth;
            // Show if recording is still in progress (ruTimer), or played (ruReplay)
            int RecordingIsInUse = Recording->IsInUse();
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
            } else {
                IconName = GetRecordingseenIcon(Recording->NumFrames(), Recording->GetResume());

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
                const cRecordingInfo *RecInfo = Recording->Info();
                IconName = GetRecordingerrorIcon(RecInfo->Errors());

                img = nullptr;
                if (Current) {
                    cString IconNameCur = cString::sprintf("%s_cur", *IconName);
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
                int ImageTop = Top + m_FontHeight - ImgRecCut->Height();  // ~ 2/3 height
                MenuIconsPixmap->DrawImage(cPoint(Left, ImageTop), *ImgRecCut);
            }
            if (Config.MenuItemRecordingShowFormatIcons) {
                IconName = GetRecordingFormatIcon(Recording);  // Show (SD), HD or UHD Logo
                if (!isempty(*IconName)) {
                    int ImageHeight = m_FontHeight * (1.0 / 3.0);  // 1/3 image height
                    img = ImgLoader.LoadIcon(*IconName, 999, ImageHeight);
                        if (img) {
                            // int FontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);
                            int ImageTop = Top + m_FontHeight - m_FontAscender;
                            int ImageLeft = Left + m_FontHeight - img->Width();
                            MenuIconsOvlPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
                        }
                }
            }

            Left += (ImgRecCut->Width() * 1.5f) + m_MarginItem;  // 0.666 * 1.5 = 0.999

        } else if (Total > 0) {
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
            Buffer = cString::sprintf("  %d", Total);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml, m_FontSml->Width("  9999"),
                                 m_FontSmlHeight, taRight);
            Left += m_FontSml->Width("  9999 ");

            if (ImgRecNewSml)
                MenuIconsPixmap->DrawImage(cPoint(Left, Top), *ImgRecNewSml);

            Left += ImgRecNewSml->Width() + m_MarginItem;
            Buffer = cString::sprintf("%d", New);
            MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorFg, ColorBg, m_FontSml,
                                 m_MenuItemWidth - Left - m_MarginItem);
            Left += m_FontSml->Width(" 999 ");

            if (Config.MenuItemRecordingShowFolderDate != 0) {
                Buffer = cString::sprintf("  (%s) ", *ShortDateString(GetLastRecTimeFromFolder(Recording, Level)));
                MenuPixmap->DrawText(cPoint(Left, Top), *Buffer, ColorExtraTextFg, ColorBg, m_FontSml);
                if (IsRecordingOld(Recording, Level)) {
                    Left += m_FontSml->Width(*Buffer);
                    img = nullptr;
                    if (Current)
                        img = ImgLoader.LoadIcon("recording_old_cur", m_FontSmlHeight, m_FontSmlHeight);
                    else
                        img = ImgLoader.LoadIcon("recording_old", m_FontSmlHeight, m_FontSmlHeight);
                    if (img)
                        MenuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
                }
            }
        } else if (Total == -1) {
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

    DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);

    if (!m_IsScrolling)
        ItemBorderInsertUnique(ib);

    if (Config.MenuRecordingView == 3 && Current)
        DrawItemExtraRecording(Recording, tr("no recording info"));

    return true;
}

void cFlatDisplayMenu::SetEvent(const cEvent *Event) {
    if (!ContentHeadIconsPixmap || !ContentHeadPixmap) return;
    if (!Event) return;

#ifdef DEBUGEPGTIME
    uint32_t tick0 = GetMsTicks();
#endif

    m_ShowEvent = true;
    m_ShowRecording = false;
    m_ShowText = false;
    ItemBorderClear();

    m_cLeft = Config.decorBorderMenuContentSize;
    m_cTop = m_chTop + m_MarginItem * 3 + m_FontHeight + m_FontSmlHeight * 2 + Config.decorBorderMenuContentSize +
             Config.decorBorderMenuContentHeadSize;
    m_cWidth = m_MenuWidth - Config.decorBorderMenuContentSize * 2;
    m_cHeight = m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                               Config.decorBorderButtonSize * 2 + m_MarginItem * 3 + m_chHeight +
                               Config.decorBorderMenuContentHeadSize * 2 + Config.decorBorderMenuContentSize * 2);

    if (!ButtonsDrawn())
        m_cHeight += m_ButtonsHeight + Config.decorBorderButtonSize * 2;

    m_MenuItemWidth = m_cWidth;

    PixmapFill(ContentHeadIconsPixmap, clrTransparent);

    cString Text(""), TextAdditional("");
    std::string Fsk("");
    std::list<std::string> GenreIcons;

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
            Text.Append(cString::sprintf("\n%s: %s", tr("FSK"), Fsk.c_str()));
        }

        const cComponents *Components = Event->Components();
        if (Components) {
            cString Audio(""), Subtitle("");
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

    double IconHeight = (m_chHeight - (2 * m_MarginItem)) * Config.EpgFskGenreIconSize * 100.0f;
    int HeadIconTop = m_chHeight - IconHeight - m_MarginItem;  // Position for fsk/genre image
    int HeadIconLeft = m_chWidth - IconHeight - m_MarginItem;
    cString IconName("");
    cImage *img {nullptr};
    if (Fsk.length() > 0) {
        IconName = cString::sprintf("EPGInfo/FSK/%s", Fsk.c_str());
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
    bool IsUnknownDrawn = false;
    while (!GenreIcons.empty()) {
        GenreIcons.sort();
        GenreIcons.unique();
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
    uint32_t tick1 = GetMsTicks();
    dsyslog("flatPlus: SetEvent info-text time: %d ms", tick1 - tick0);
#endif

    cString Reruns("");
    if (Config.EpgRerunsShow) {
        // Lent from nopacity
        cPlugin *pEpgSearchPlugin = cPluginManager::GetPlugin("epgsearch");
        if (pEpgSearchPlugin && !isempty(Event->Title())) {
            std::string StrQuery = Event->Title();
            Epgsearch_searchresults_v1_0 data {
                .query = (char *)StrQuery.c_str(),  // Search term
                .mode = 0,                          // Search mode (0=phrase, 1=and, 2=or, 3=regular expression)
                .channelNr = 0,                     // Channel number to search in (0=any)
                .useTitle = true,                   // Search in title
                .useSubTitle = false,               // Search in subtitle
                .useDescription = false             // Search in description
            };
            // data.query = reinterpret_cast<char *>(StrQuery.c_str());
            // error: ‘reinterpret_cast’ from type ‘const char*’ to type ‘char*’ casts away qualifiers

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
                        LOCK_CHANNELS_READ;
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
    uint32_t tick2 = GetMsTicks();
    dsyslog("flatPlus: SetEvent reruns time: %d ms", tick2 - tick1);
#endif

    std::vector<cString> ActorsPath;
    std::vector<cString> ActorsName;
    std::vector<cString> ActorsRole;

    cString MediaPath("");
    cString MovieInfo(""), SeriesInfo("");

    int ActorsSize {0}, NumActors {0};
    int ContentTop {0};
    int MediaWidth {0}, MediaHeight {0};
    bool FirstRun = true, SecondRun = false;
    bool Scrollable = false;

    do {  // Runs up to two times!
        if (FirstRun)
            m_cWidth -= m_ScrollBarWidth;  // Assume that we need scrollbar most of the time
        else
            m_cWidth += m_ScrollBarWidth;  // For second run readd scrollbar width

        ComplexContent.Clear();
        ComplexContent.SetOsd(m_Osd);
        ComplexContent.SetPosition(cRect(m_cLeft, m_cTop, m_cWidth, m_cHeight));
        ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));
        ComplexContent.SetScrollSize(m_FontHeight);
        ComplexContent.SetScrollingActive(true);

#ifdef DEBUGEPGTIME
        uint32_t tick3 = GetMsTicks();
#endif
        MediaWidth = m_cWidth / 2 - m_MarginItem * 2;
        MediaHeight = m_cHeight - m_MarginItem * 2 - m_FontHeight - 6;
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
                        if (series.banners.size() > 0) {  // Use random banner
                            // Gets 'entropy' from device that generates random numbers itself
                            // to seed a mersenne twister (pseudo) random generator
                            std::mt19937 generator(std::random_device {}());

                            // Make sure all numbers have an equal chance.
                            // Range is inclusive (so we need -1 for vector index)
                            std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                            std::size_t number = distribution(generator);

                            MediaPath = series.banners[number].path.c_str();
                            if (series.banners.size() > 1)
                                dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                                        static_cast<int>(number + 1), *MediaPath,
                                        static_cast<int>(series.banners.size()));  // Log result
                        }
                        if (Config.TVScraperEPGInfoShowActors) {
                            ActorsSize = series.actors.size();
                            ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                            ActorsName.reserve(ActorsSize);
                            ActorsRole.reserve(ActorsSize);
                            for (int i {0}; i < ActorsSize; ++i) {
                                if (ImgLoader.FileExits(series.actors[i].actorThumb.path)) {
                                    ActorsPath.emplace_back(series.actors[i].actorThumb.path.c_str());
                                    ActorsName.emplace_back(series.actors[i].name.c_str());
                                    ActorsRole.emplace_back(series.actors[i].role.c_str());
                                }
                            }
                        }
                        if (series.name.length() > 0)
                            SeriesInfo.Append(cString::sprintf("%s%s\n", tr("name: "), series.name.c_str()));
                        if (series.firstAired.length() > 0)
                            SeriesInfo.Append(
                                cString::sprintf("%s%s\n", tr("first aired: "), series.firstAired.c_str()));
                        if (series.network.length() > 0)
                            SeriesInfo.Append(cString::sprintf("%s%s\n", tr("network: "), series.network.c_str()));
                        if (series.genre.length() > 0)
                            SeriesInfo.Append(cString::sprintf("%s%s\n", tr("genre: "), series.genre.c_str()));
                        if (series.rating > 0)
                            SeriesInfo.Append(cString::sprintf("%s%.1f\n", tr("rating: "), series.rating));
                        if (series.status.length() > 0)
                            SeriesInfo.Append(cString::sprintf("%s%s\n", tr("status: "), series.status.c_str()));
                        if (series.episode.season > 0)
                            SeriesInfo.Append(cString::sprintf("%s%d\n", tr("season number: "), series.episode.season));
                        if (series.episode.number > 0)
                            SeriesInfo.Append(
                                cString::sprintf("%s%d\n", tr("episode number: "), series.episode.number));
                    }
                } else if (call.type == tMovie) {
                    cMovie movie;
                    movie.movieId = movieId;
                    if (pScraper->Service("GetMovie", &movie)) {
                        MediaPath = movie.poster.path.c_str();
                        if (Config.TVScraperEPGInfoShowActors) {
                            ActorsSize = movie.actors.size();
                            ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                            ActorsName.reserve(ActorsSize);
                            ActorsRole.reserve(ActorsSize);
                            for (int i {0}; i < ActorsSize; ++i) {
                                if (ImgLoader.FileExits(movie.actors[i].actorThumb.path)) {
                                    ActorsPath.emplace_back(movie.actors[i].actorThumb.path.c_str());
                                    ActorsName.emplace_back(movie.actors[i].name.c_str());
                                    ActorsRole.emplace_back(movie.actors[i].role.c_str());
                                }
                            }
                        }
                        if (movie.title.length() > 0)
                            MovieInfo.Append(cString::sprintf("%s%s\n", tr("title: "), movie.title.c_str()));
                        if (movie.originalTitle.length() > 0)
                            MovieInfo.Append(
                                cString::sprintf("%s%s\n", tr("original title: "), movie.originalTitle.c_str()));
                        if (movie.collectionName.length() > 0)
                            MovieInfo.Append(
                                cString::sprintf("%s%s\n", tr("collection name: "), movie.collectionName.c_str()));
                        if (movie.genres.length() > 0)
                            MovieInfo.Append(cString::sprintf("%s%s\n", tr("genre: "), movie.genres.c_str()));
                        if (movie.releaseDate.length() > 0)
                            MovieInfo.Append(
                                cString::sprintf("%s%s\n", tr("release date: "), movie.releaseDate.c_str()));
                        if (movie.popularity > 0)
                            MovieInfo.Append(cString::sprintf("%s%.1f\n", tr("popularity: "), movie.popularity));
                        if (movie.voteAverage > 0)
                            MovieInfo.Append(cString::sprintf("%s%.1f\n", tr("vote average: "), movie.voteAverage));
                    }
                }
            }  // Scraper plugin
        }  // FirstRun
#ifdef DEBUGEPGTIME
        uint32_t tick4 = GetMsTicks();
        dsyslog("flatPlus: SetEvent tvscraper time: %d ms", tick4 - tick3);
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
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
            } else if (!isempty(*Text)) {  // No image; insert text
                ComplexContent.AddText(
                    *Text, true,
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
            }
        } else if (!isempty(*Text)) {  // No image; insert text
            ComplexContent.AddText(
                *Text, true, cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
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
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
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
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
        }
#ifdef DEBUGEPGTIME
        uint32_t tick5 = GetMsTicks();
        dsyslog("flatPlus: SetEvent epg-text time: %d ms", tick5 - tick4);
#endif

        NumActors = ActorsPath.size();
        if (Config.TVScraperEPGInfoShowActors && NumActors > 0) {
            //* Add actors to complexcontent for later displaying
            AddActors(ComplexContent, ActorsPath, ActorsName, ActorsRole, NumActors);
        }
#ifdef DEBUGEPGTIME
        uint32_t tick6 = GetMsTicks();
        dsyslog("flatPlus: SetEvent actor time: %d ms", tick6 - tick5);
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
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
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
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font);
        }
        Scrollable = ComplexContent.Scrollable(m_cHeight - m_MarginItem * 2);
        if (Scrollable || SecondRun)
            break;             // No need for another run (Scrolling content or second run)
        if (FirstRun) {        // Second run because not scrolling content. Should be cheap to rerun
            SecondRun = true;  // Only runs when minimal contents fits in area of description
            FirstRun = false;
            // dsyslog("flatPlus: --- SetEvent second run with no scrollbars ---");
        }
    } while (FirstRun || SecondRun);

    if (Config.MenuContentFullSize || Scrollable)
        ComplexContent.CreatePixmaps(true);
    else
        ComplexContent.CreatePixmaps(false);
#ifdef DEBUGEPGTIME
        uint32_t tick7 = GetMsTicks();
#endif

    ComplexContent.Draw();
#ifdef DEBUGEPGTIME
        uint32_t tick8 = GetMsTicks();
        dsyslog("flatPlus: SetRecording actor time: %d ms", tick8 - tick7);
#endif

    PixmapFill(ContentHeadPixmap, clrTransparent);
    ContentHeadPixmap->DrawRectangle(cRect(0, 0, m_MenuWidth, m_FontHeight + m_FontSmlHeight * 2 + m_MarginItem * 2),
                                     Theme.Color(clrScrollbarBg));

    cString date = Event->GetDateString();
    cString StartTime = Event->GetTimeString();
    cString EndTime = Event->GetEndTimeString();

    cString StrTime = cString::sprintf("%s  %s - %s", *date, *StartTime, *EndTime);

    cString title = Event->Title();
    cString ShortText = Event->ShortText();
    int ShortTextWidth = m_FontSml->Width(*ShortText);                         // Width of short text
    int MaxWidth = m_MenuWidth - m_MarginItem - (m_MenuWidth - HeadIconLeft);  // headIconLeft includes right margin
    int left = m_MarginItem;

    ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem), *StrTime, Theme.Color(clrMenuEventFontInfo),
                                Theme.Color(clrMenuEventBg), m_FontSml, m_MenuWidth - m_MarginItem * 2);
    ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight), *title,
                                Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font,
                                m_MenuWidth - m_MarginItem * 2);
    // Add scroller to long short text
    if (ShortTextWidth > MaxWidth) {  // Short text too long
        if (Config.ScrollerEnable) {
            MenuItemScroller.AddScroller(*ShortText,
                                         cRect(m_chLeft + left, m_chTop + m_MarginItem + m_FontSmlHeight + m_FontHeight,
                                               MaxWidth, m_FontSmlHeight),
                                         Theme.Color(clrMenuEventFontInfo), clrTransparent, m_FontSml);
        } else {  // Add ... if info ist too long
            dsyslog("flatPlus: Short text too long! (%d) Setting MaxWidth to %d", ShortTextWidth, MaxWidth);
            int DotsWidth = m_FontSml->Width("...");
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

    DecorBorderDraw(m_chLeft, m_chTop, m_chWidth, m_chHeight, Config.decorBorderMenuContentHeadSize,
                    Config.decorBorderMenuContentHeadType, Config.decorBorderMenuContentHeadFg,
                    Config.decorBorderMenuContentHeadBg);

    if (Scrollable)
        DrawScrollbar(ComplexContent.ScrollTotal(), ComplexContent.ScrollOffset(), ComplexContent.ScrollShown(),
                      ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ComplexContent.ScrollOffset() > 0,
                      ComplexContent.ScrollOffset() + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(),
                      true);

    if (Config.MenuContentFullSize || Scrollable)
        DecorBorderDraw(m_cLeft, m_cTop, m_cWidth, ComplexContent.ContentHeight(true),
                        Config.decorBorderMenuContentSize, Config.decorBorderMenuContentType,
                        Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg);
    else
        DecorBorderDraw(m_cLeft, m_cTop, m_cWidth, ComplexContent.ContentHeight(false),
                        Config.decorBorderMenuContentSize, Config.decorBorderMenuContentType,
                        Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg);

#ifdef DEBUGEPGTIME
    uint32_t tick9 = GetMsTicks();
    dsyslog("flatPlus: SetEvent total time: %d ms", tick9 - tick0);
#endif
}

void cFlatDisplayMenu::DrawItemExtraRecording(const cRecording *Recording, cString EmptyText) {
    m_cLeft = m_MenuItemWidth + Config.decorBorderMenuItemSize * 2 + Config.decorBorderMenuContentSize + m_MarginItem;
    if (m_IsScrolling)
        m_cLeft += m_ScrollBarWidth;

    m_cTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
    m_cWidth = m_MenuWidth - m_cLeft - Config.decorBorderMenuContentSize;
    m_cHeight =
        m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                       Config.decorBorderButtonSize * 2 + m_MarginItem * 3 + Config.decorBorderMenuContentSize * 2);

    cString Dummy(""), Text("");
    if (Recording) {
        const cRecordingInfo *RecInfo = Recording->Info();
        // Text.imbue(std::locale(""));

        if (!isempty(RecInfo->Description()))
            Text.Append(cString::sprintf("%s\n\n", RecInfo->Description()));

        // Lent from skinelchi
        if (Config.RecordingAdditionalInfoShow) {
            LOCK_CHANNELS_READ;
            const cChannel *channel = Channels->GetByChannelID(((cRecordingInfo *)RecInfo)->ChannelID());
            // const cChannel *channel =
            //    Channels->GetByChannelID((reinterpret_cast<cRecordingInfo *>(RecInfo))->ChannelID());
            // error: ‘reinterpret_cast’ from type ‘const cRecordingInfo*’ to type ‘cRecordingInfo*’
            //         casts away qualifiers
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

            GetCuttedLengthMarks(Recording, Text, Dummy, true);  // Process marks and insert text

            // From SkinNopacity
#if APIVERSNUM >= 20505
            if (RecInfo && RecInfo->Errors() >= 1) {
                std::ostringstream RecErrors("");
                RecErrors.imbue(std::locale(""));  // Set to local locale
                RecErrors << RecInfo->Errors();
                Text.Append(cString::sprintf("%s: %s\n", tr("TS errors"), RecErrors.str().c_str()));
            }
#endif

            const cComponents *Components = RecInfo->Components();
            if (Components) {
                cString Audio(""), Subtitle("");
                InsertComponents(Components, Text, Audio, Subtitle, true);  // Get info for audio/video and subtitle

                if (!isempty(*Audio))
                    Text.Append(cString::sprintf("\n%s: %s", tr("Audio"), *Audio));
                if (!isempty(*Subtitle))
                    Text.Append(cString::sprintf("\n%s: %s", tr("Subtitle"), *Subtitle));
            }
            if (RecInfo->Aux())
                InsertAuxInfos(RecInfo, Text, true);  // Insert aux infos with info line
        }  // if Config.RecordingAdditionalInfoShow
    } else
        Text.Append(*EmptyText);

    ComplexContent.Clear();
    ComplexContent.SetScrollSize(m_FontSmlHeight);
    ComplexContent.SetScrollingActive(false);
    ComplexContent.SetOsd(m_Osd);
    ComplexContent.SetPosition(cRect(m_cLeft, m_cTop, m_cWidth, m_cHeight));
    ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));

    cString MediaPath("");
    int MediaWidth {0}, MediaHeight {999};
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
                if (series.banners.size() > 0) {  // Use random banner
                    // Gets 'entropy' from device that generates random numbers itself
                    // to seed a mersenne twister (pseudo) random generator
                    std::mt19937 generator(std::random_device {}());

                    // Make sure all numbers have an equal chance.
                    // Range is inclusive (so we need -1 for vector index)
                    std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                    std::size_t number = distribution(generator);

                    MediaPath = series.banners[number].path.c_str();
                    if (series.banners.size() > 1)
                        dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                                static_cast<int>(number + 1), *MediaPath,
                                static_cast<int>(series.banners.size()));  // Log result
                }
                MediaWidth = m_cWidth - m_MarginItem * 2;
                MediaType = 1;
            }
        } else if (call.type == tMovie) {
            cMovie movie;
            movie.movieId = movieId;
            if (pScraper->Service("GetMovie", &movie)) {
                MediaPath = movie.poster.path.c_str();
                MediaWidth = m_cWidth / 2 - m_MarginItem * 3;
                MediaType = 2;
            }
        }
    }  // TVScraperRecInfoShowPoster

    if (isempty(*MediaPath)) {  // Prio for tvscraper poster
        cString RecPath = cString::sprintf("%s", Recording->FileName());
        cString RecImage("");
        if (ImgLoader.SearchRecordingPoster(*RecPath, RecImage)) {
            MediaWidth = m_cWidth / 2 - m_MarginItem * 3;
            MediaType = 2;
            MediaPath = RecImage;
        }
    }

    if (!isempty(*MediaPath)) {
        cImage *img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);
        if (img && MediaType == 2) {  // Movie
            ComplexContent.AddImageWithFloatedText(
                img, CIP_Right, *Text,
                cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
        } else if (img && MediaType == 1) {  // Series
            ComplexContent.AddImage(img, cRect(m_MarginItem, m_MarginItem, img->Width(), img->Height()));
            ComplexContent.AddText(*Text, true,
                                   cRect(m_MarginItem, m_MarginItem + img->Height(), m_cWidth - m_MarginItem * 2,
                                         m_cHeight - m_MarginItem * 2),
                                   Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
        } else {
            ComplexContent.AddText(
                *Text, true,
                cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
        }
    } else {
        ComplexContent.AddText(
            *Text, true, cRect(m_MarginItem, m_MarginItem, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
            Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml);
    }

    ComplexContent.CreatePixmaps(Config.MenuContentFullSize);
    ComplexContent.Draw();

    DecorBorderClearByFrom(BorderContent);
    if (Config.MenuContentFullSize)
        DecorBorderDraw(m_cLeft, m_cTop, m_cWidth, ComplexContent.ContentHeight(true),
                        Config.decorBorderMenuContentSize, Config.decorBorderMenuContentType,
                        Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg, BorderContent);
    else
        DecorBorderDraw(m_cLeft, m_cTop, m_cWidth, ComplexContent.ContentHeight(false),
                        Config.decorBorderMenuContentSize, Config.decorBorderMenuContentType,
                        Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg, BorderContent);
}

void cFlatDisplayMenu::AddActors(cComplexContent &ComplexContent, std::vector<cString> &ActorsPath,
                                 std::vector<cString> &ActorsName, std::vector<cString> &ActorsRole,
                                 int NumActors) {
    // TVScraperEPGInfoShowActors, TVScraperRecInfoShowActors
    int ShowMaxActors = Config.TVScraperShowMaxActors;  // Global setting for epg- and rec info
    if (ShowMaxActors == 0) return;  // Do not show actors
    if ( /* ShowMaxActors != -1 && */ ShowMaxActors > 0 && ShowMaxActors < NumActors)
        NumActors = ShowMaxActors;  // Limit to ShowMaxActors (-1 = Show all actors)

    int ContentTop = ComplexContent.BottomContent() + m_FontHeight;
    ComplexContent.AddText(tr("Actors"), false, cRect(m_MarginItem * 10, ContentTop, 0, 0),
                           Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), m_Font);
    ContentTop += m_FontHeight;
    ComplexContent.AddRect(cRect(0, ContentTop, m_cWidth, 3), Theme.Color(clrMenuRecTitleLine));
    ContentTop += 6;

    cImage *img {nullptr};
    cString Name(""), Path(""), Role("");  // Actor name, path and role
    int Actor{0}, ActorsPerLine{6};  // TODO: Config option?
    int ActorWidth = m_cWidth / ActorsPerLine - m_MarginItem * 4;
    int ActorMargin = ((m_cWidth - m_MarginItem * 2) - ActorWidth * ActorsPerLine) / (ActorsPerLine - 1);
    int PicsPerLine = (m_cWidth - m_MarginItem * 2) / ActorWidth;
    int PicLines = NumActors / PicsPerLine + (NumActors % PicsPerLine != 0);
    int x = m_MarginItem;
    int y = ContentTop;
    if (NumActors > 50)
        dsyslog("flatPlus: Showing %d actor images! First display will probably be slow.", NumActors);

    for (int row {0}; row < PicLines; ++row) {
        for (int col {0}; col < PicsPerLine; ++col) {
            if (Actor == NumActors)
                break;
            Path = ActorsPath[Actor];
            img = ImgLoader.LoadFile(*Path, ActorWidth, 999);
            if (img) {
                ComplexContent.AddImage(img, cRect(x, y, 0, 0));
                Name = ActorsName[Actor];
                Role = cString::sprintf("\"%s\"", *ActorsRole[Actor]);
                ComplexContent.AddText(*Name, false, cRect(x, y + img->Height() + m_MarginItem, ActorWidth, 0),
                                       Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml,
                                       ActorWidth, m_FontSmlHeight, taCenter);
                ComplexContent.AddText(
                    *Role, false, cRect(x, y + img->Height() + m_MarginItem + m_FontSmlHeight, ActorWidth, 0),
                    Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_FontSml, ActorWidth,
                    m_FontSmlHeight, taCenter);
            }
            x += ActorWidth + ActorMargin;
            ++Actor;
        }  // for col
        x = m_MarginItem;
        y = ComplexContent.BottomContent() + m_FontHeight;
    }  // for row
}

void cFlatDisplayMenu::SetRecording(const cRecording *Recording) {
    if (!ContentHeadPixmap || !ContentHeadIconsPixmap) return;
    if (!Recording) return;

#ifdef DEBUGEPGTIME
    uint32_t tick0 = GetMsTicks();
#endif

    m_ShowEvent = false;
    m_ShowRecording = true;
    m_ShowText = false;
    ItemBorderClear();

    PixmapFill(ContentHeadIconsPixmap, clrTransparent);

    m_cLeft = Config.decorBorderMenuContentSize;
    m_cTop = m_chTop + m_MarginItem * 3 + m_FontHeight + m_FontSmlHeight * 2 + Config.decorBorderMenuContentSize +
             Config.decorBorderMenuContentHeadSize;
    m_cWidth = m_MenuWidth - Config.decorBorderMenuContentSize * 2;
    m_cHeight = m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                               Config.decorBorderButtonSize * 2 + m_MarginItem * 3 + m_chHeight +
                               Config.decorBorderMenuContentHeadSize * 2 + Config.decorBorderMenuContentSize * 2);

    if (!ButtonsDrawn())
        m_cHeight += m_ButtonsHeight + Config.decorBorderButtonSize * 2;

    m_MenuItemWidth = m_cWidth;

    cString Dummy(""), RecAdditional(""), Text(""), TextAdditional("");
    // Text.imbue(std::locale(""));
    // TextAdditional.imbue(std::locale(""));
    // RecAdditional.imbue(std::locale(""));

    const cRecordingInfo *RecInfo = Recording->Info();
    std::list<std::string> GenreIcons;
    std::string Fsk("");
    if (!isempty(RecInfo->Description()))
        Text.Append(cString::sprintf("%s", RecInfo->Description()));
        // Text.Append(cString::sprintf("%s\n\n", RecInfo->Description()));  //! Why two line breaks?

    // Lent from skinelchi
    if (Config.RecordingAdditionalInfoShow) {
        auto channelFuture = std::async(
            [&RecAdditional](tChannelID channelId) {
                LOCK_CHANNELS_READ;
                const cChannel *channel = Channels->GetByChannelID(channelId);
                if (channel)
                    RecAdditional.Append(
                        cString::sprintf("%s: %d - %s\n", trVDR("Channel"), channel->Number(), channel->Name()));
            },
            RecInfo->ChannelID());
        channelFuture.get();
        const cEvent *Event = RecInfo->GetEvent();
        if (Event) {
            // Genre
            InsertGenreInfo(Event, Text, GenreIcons);  // Add genre info

            if (Event->Contents(0))
                Text.Append("\n");
            // FSK
            if (Event->ParentalRating()) {
                Fsk = *Event->GetParentalRatingString();
                Text.Append(cString::sprintf("%s: %s\n", tr("FSK"), Fsk.c_str()));
            }
        }

        GetCuttedLengthMarks(Recording, RecAdditional, Dummy, true);  // Process marks and insert text

        // From SkinNopacity
#if APIVERSNUM >= 20505
        if (RecInfo && RecInfo->Errors() >= 1) {
            std::ostringstream RecErrors("");
            RecErrors.imbue(std::locale(""));  // Set to local locale
            RecErrors << RecInfo->Errors();
            RecAdditional.Append(cString::sprintf("\n%s: %s", tr("TS errors"), RecErrors.str().c_str()));
        }
#endif

        const cComponents *Components = RecInfo->Components();
        if (Components) {
            cString Audio(""), Subtitle("");
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

    double IconHeight = (m_chHeight - (2 * m_MarginItem)) * Config.EpgFskGenreIconSize * 100.0f;
    int HeadIconTop = m_chHeight - IconHeight - m_MarginItem;  // Position for fsk/genre image
    int HeadIconLeft = m_chWidth - IconHeight - m_MarginItem;
    cString IconName("");
    cImage *img {nullptr};
    if (Fsk.length() > 0) {
        IconName = cString::sprintf("EPGInfo/FSK/%s", Fsk.c_str());
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
    bool IsUnknownDrawn = false;
    while (!GenreIcons.empty()) {
        GenreIcons.sort();
        GenreIcons.unique();
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
    uint32_t tick1 = GetMsTicks();
    dsyslog("flatPlus: SetRecording info-text time: %d ms", tick1 - tick0);
#endif

    std::vector<cString> ActorsPath;
    std::vector<cString> ActorsName;
    std::vector<cString> ActorsRole;

    cString MediaPath("");
    cString MovieInfo(""), SeriesInfo("");

    int ActorsSize {0}, NumActors {0};
    int ContentTop {0};
    int MediaWidth {0}, MediaHeight {0};
    bool FirstRun = true, SecondRun = false;
    bool Scrollable = false;

    do {  // Runs up to two times!
        if (FirstRun)
            m_cWidth -= m_ScrollBarWidth;  // Assume that we need scrollbar most of the time
        else
            m_cWidth += m_ScrollBarWidth;  // For second run readd scrollbar width

        ComplexContent.Clear();
        ComplexContent.SetOsd(m_Osd);
        ComplexContent.SetPosition(cRect(m_cLeft, m_cTop, m_cWidth, m_cHeight));
        ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));
        ComplexContent.SetScrollSize(m_FontHeight);
        ComplexContent.SetScrollingActive(true);

#ifdef DEBUGEPGTIME
        uint32_t tick2 = GetMsTicks();
#endif
        MediaWidth = m_cWidth / 2 - m_MarginItem * 2;
        MediaHeight = m_cHeight - m_MarginItem * 2 - m_FontHeight - 6;
        if (FirstRun) {  // Call scraper plugin only at first run and reuse data at second run
            static cPlugin *pScraper = GetScraperPlugin();
            if ((Config.TVScraperRecInfoShowPoster || Config.TVScraperRecInfoShowActors) && pScraper) {
                ScraperGetEventType call;
                call.recording = Recording;
                int seriesId{0}, episodeId{0}, movieId{0};

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
                        if (series.banners.size() > 0) {  // Use random banner
                            // Gets 'entropy' from device that generates random numbers itself
                            // to seed a mersenne twister (pseudo) random generator
                            std::mt19937 generator(std::random_device {}());

                            // Make sure all numbers have an equal chance.
                            // Range is inclusive (so we need -1 for vector index)
                            std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                            std::size_t number = distribution(generator);

                            MediaPath = series.banners[number].path.c_str();
                            if (series.banners.size() > 1)
                                dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                                        static_cast<int>(number + 1), *MediaPath,
                                        static_cast<int>(series.banners.size()));  // Log result
                        }
                        if (Config.TVScraperRecInfoShowActors) {
                            ActorsSize = series.actors.size();
                            ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                            ActorsName.reserve(ActorsSize);
                            ActorsRole.reserve(ActorsSize);
                            for (int i {0}; i < ActorsSize; ++i) {
                                if (ImgLoader.FileExits(series.actors[i].actorThumb.path)) {
                                    ActorsPath.emplace_back(series.actors[i].actorThumb.path.c_str());
                                    ActorsName.emplace_back(series.actors[i].name.c_str());
                                    ActorsRole.emplace_back(series.actors[i].role.c_str());
                                }
                            }
                        }
                        if (series.name.length() > 0)
                            SeriesInfo.Append(cString::sprintf("%s%s\n", tr("name: "), series.name.c_str()));
                        if (series.firstAired.length() > 0)
                            SeriesInfo.Append(
                                cString::sprintf("%s%s\n", tr("first aired: "), series.firstAired.c_str()));
                        if (series.network.length() > 0)
                            SeriesInfo.Append(cString::sprintf("%s%s\n", tr("network: "), series.network.c_str()));
                        if (series.genre.length() > 0)
                            SeriesInfo.Append(cString::sprintf("%s%s\n", tr("genre: "), series.genre.c_str()));
                        if (series.rating > 0)
                            SeriesInfo.Append(cString::sprintf("%s%.1f\n", tr("rating: "), series.rating));
                        if (series.status.length() > 0)
                            SeriesInfo.Append(cString::sprintf("%s%s\n", tr("status: "), series.status.c_str()));
                        if (series.episode.season > 0)
                            SeriesInfo.Append(cString::sprintf("%s%d\n", tr("season number: "), series.episode.season));
                        if (series.episode.number > 0)
                            SeriesInfo.Append(
                                cString::sprintf("%s%d\n", tr("episode number: "), series.episode.number));
                    }
                } else if (call.type == tMovie) {
                    cMovie movie;
                    movie.movieId = movieId;
                    if (pScraper->Service("GetMovie", &movie)) {
                        MediaPath = movie.poster.path.c_str();
                        if (Config.TVScraperRecInfoShowActors) {
                            ActorsSize = movie.actors.size();
                            ActorsPath.reserve(ActorsSize);  // Set capacity to size of actors
                            ActorsName.reserve(ActorsSize);
                            ActorsRole.reserve(ActorsSize);
                            for (int i {0}; i < ActorsSize; ++i) {
                                if (ImgLoader.FileExits(movie.actors[i].actorThumb.path)) {
                                    ActorsPath.emplace_back(movie.actors[i].actorThumb.path.c_str());
                                    ActorsName.emplace_back(movie.actors[i].name.c_str());
                                    ActorsRole.emplace_back(movie.actors[i].role.c_str());
                                }
                            }
                        }
                        if (movie.title.length() > 0)
                            MovieInfo.Append(cString::sprintf("%s%s\n", tr("title: "), movie.title.c_str()));
                        if (movie.originalTitle.length() > 0)
                            MovieInfo.Append(
                                cString::sprintf("%s%s\n", tr("original title: "), movie.originalTitle.c_str()));
                        if (movie.collectionName.length() > 0)
                            MovieInfo.Append(
                                cString::sprintf("%s%s\n", tr("collection name: "), movie.collectionName.c_str()));
                        if (movie.genres.length() > 0)
                            MovieInfo.Append(cString::sprintf("%s%s\n", tr("genre: "), movie.genres.c_str()));
                        if (movie.releaseDate.length() > 0)
                            MovieInfo.Append(
                                cString::sprintf("%s%s\n", tr("release date: "), movie.releaseDate.c_str()));
                        if (movie.popularity > 0)
                            MovieInfo.Append(cString::sprintf("%s%.1f\n", tr("popularity: "), movie.popularity));
                        if (movie.voteAverage > 0)
                            MovieInfo.Append(cString::sprintf("%s%.1f\n", tr("vote average: "), movie.voteAverage));
                    }
                }
            }  // Scraper plugin

            if (isempty(*MediaPath)) {  // Prio for tvscraper poster
                cString RecPath = cString::sprintf("%s", Recording->FileName());
                cString RecImage("");
                if (ImgLoader.SearchRecordingPoster(*RecPath, RecImage))
                    MediaPath = RecImage;
            }
        }  // FirstRun

#ifdef DEBUGEPGTIME
        uint32_t tick3 = GetMsTicks();
        dsyslog("flatPlus: SetRecording tvscraper time: %d ms", tick3 - tick2);
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
                int Aspect = img->Width() / img->Height();  // 50% x 100% of content size
                if (Aspect >= 1 && Aspect < 4) {  //* Portrait (For example 1920x1080)
                    // dsyslog("flatPlus: SetRecording Portrait image %dx%d (%d) found! Setting to 2/3 size.",
                    //         img->Width(), img->Height(), Aspect);
                    MediaHeight *= (1.0 / 3.0 * 2);  // Size * 0,666 = 2/3
                    img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);  // Reload portrait with new size
                }
            }
            if (img) {  // Insert image with floating text
                ComplexContent.AddImageWithFloatedText(
                    img, CIP_Right, *Text,
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                    Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
            } else if (!isempty(*Text)) {  // No image; insert text
                ComplexContent.AddText(
                    *Text, true,
                    cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                    Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
            }
        } else if (!isempty(*Text)) {  // No image; insert text
            ComplexContent.AddText(
                *Text, true, cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
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
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
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
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
        }
#ifdef DEBUGEPGTIME
        uint32_t tick4 = GetMsTicks();
        dsyslog("flatPlus: SetRecording epg-text time: %d ms", tick4 - tick3);
#endif

        NumActors = ActorsPath.size();
        if (Config.TVScraperRecInfoShowActors && NumActors > 0) {
            //* Add actors to complexcontent for later displaying
            AddActors(ComplexContent, ActorsPath, ActorsName, ActorsRole, NumActors);
        }
#ifdef DEBUGEPGTIME
        uint32_t tick5 = GetMsTicks();
        dsyslog("flatPlus: SetRecording actor time: %d ms", tick5 - tick4);
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
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
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
                cRect(m_MarginItem, ContentTop, m_cWidth - m_MarginItem * 2, m_cHeight - m_MarginItem * 2),
                Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), m_Font);
        }

        Scrollable = ComplexContent.Scrollable(m_cHeight - m_MarginItem * 2);
        if (Scrollable || SecondRun)
            break;             // No need for another run (Scrolling content or second run)
        if (FirstRun) {        // Second run because not scrolling content. Should be cheap to rerun
            SecondRun = true;  // Only runs when minimal contents fits in area of description
            FirstRun = false;
            // dsyslog("flatPlus: --- SetRecording second run with no scrollbar ---");
        }
    } while (FirstRun || SecondRun);

    if (Config.MenuContentFullSize || Scrollable)
        ComplexContent.CreatePixmaps(true);
    else
        ComplexContent.CreatePixmaps(false);
#ifdef DEBUGEPGTIME
        uint32_t tick6 = GetMsTicks();
#endif

    ComplexContent.Draw();
#ifdef DEBUGEPGTIME
        uint32_t tick7 = GetMsTicks();
        dsyslog("flatPlus: SetRecording complexcontent draw time: %d ms", tick7 - tick6);
#endif

    PixmapFill(ContentHeadPixmap, clrTransparent);
    ContentHeadPixmap->DrawRectangle(cRect(0, 0, m_MenuWidth, m_FontHeight + m_FontSmlHeight * 2 + m_MarginItem * 2),
                                     Theme.Color(clrScrollbarBg));

    cString StrTime = cString::sprintf("%s  %s  %s", *DateString(Recording->Start()), *TimeString(Recording->Start()),
                                       RecInfo->ChannelName() ? RecInfo->ChannelName() : "");

    cString title = RecInfo->Title();
    if (isempty(*title))
        title = Recording->Name();

    cString ShortText = RecInfo->ShortText();
    int ShortTextWidth = m_FontSml->Width(*ShortText);                         // Width of short text
    int MaxWidth = m_MenuWidth - m_MarginItem - (m_MenuWidth - HeadIconLeft);  // HeadIconLeft includes right margin
    int left = m_MarginItem;

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors)
        MaxWidth -= m_FontSmlHeight;  // Substract width of imgRecErr
#endif

    ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem), *StrTime, Theme.Color(clrMenuRecFontInfo),
                                Theme.Color(clrMenuRecBg), m_FontSml, m_MenuWidth - m_MarginItem * 2);
    ContentHeadPixmap->DrawText(cPoint(left, m_MarginItem + m_FontSmlHeight), *title, Theme.Color(clrMenuRecFontTitle),
                                Theme.Color(clrMenuRecBg), m_Font, m_MenuWidth - m_MarginItem * 2);
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
            int DotsWidth = m_FontSml->Width("...");
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
        cString ErrIcon = GetRecordingerrorIcon(RecInfo->Errors());
        cString RecErrIcon = cString::sprintf("%s_replay", *ErrIcon);

        img = ImgLoader.LoadIcon(*RecErrIcon, 999, m_FontSmlHeight);  // Small image
        if (img) {
            left += m_MarginItem;
            int ImageTop = m_MarginItem + m_FontSmlHeight + m_FontHeight + (m_FontSmlHeight - img->Height()) / 2;
            ContentHeadIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
        }
    }  // MenuItemRecordingShowRecordingErrors
#endif

    DecorBorderDraw(m_chLeft, m_chTop, m_chWidth, m_chHeight, Config.decorBorderMenuContentHeadSize,
                    Config.decorBorderMenuContentHeadType, Config.decorBorderMenuContentHeadFg,
                    Config.decorBorderMenuContentHeadBg, BorderSetRecording, false);

    if (Scrollable)
        DrawScrollbar(ComplexContent.ScrollTotal(), ComplexContent.ScrollOffset(), ComplexContent.ScrollShown(),
                      ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ComplexContent.ScrollOffset() > 0,
                      ComplexContent.ScrollOffset() + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(),
                      true);

    RecordingBorder.Left = m_cLeft;
    RecordingBorder.Top = m_cTop;
    RecordingBorder.Width = m_cWidth;
    RecordingBorder.Height = ComplexContent.Height();
    RecordingBorder.Size = Config.decorBorderMenuContentSize;
    RecordingBorder.Type = Config.decorBorderMenuContentType;
    RecordingBorder.ColorFg = Config.decorBorderMenuContentFg;
    RecordingBorder.ColorBg = Config.decorBorderMenuContentBg;
    RecordingBorder.From = BorderMenuRecord;

    if (Config.MenuContentFullSize || Scrollable)
        DecorBorderDraw(RecordingBorder.Left, RecordingBorder.Top, RecordingBorder.Width,
                        ComplexContent.ContentHeight(true), RecordingBorder.Size, RecordingBorder.Type,
                        RecordingBorder.ColorFg, RecordingBorder.ColorBg, RecordingBorder.From, false);
    else
        DecorBorderDraw(RecordingBorder.Left, RecordingBorder.Top, RecordingBorder.Width,
                        ComplexContent.ContentHeight(false), RecordingBorder.Size, RecordingBorder.Type,
                        RecordingBorder.ColorFg, RecordingBorder.ColorBg, RecordingBorder.From, false);

#ifdef DEBUGEPGTIME
    uint32_t tick8 = GetMsTicks();
    dsyslog("flatPlus: SetRecording total time: %d ms", tick8 - tick0);
#endif
}

void cFlatDisplayMenu::SetText(const char *Text, bool FixedFont) {
    if (!Text) return;

    m_ShowEvent = false;
    m_ShowRecording = false;
    m_ShowText = true;
    ItemBorderClear();

    PixmapFill(ContentHeadPixmap, clrTransparent);

    int Left = Config.decorBorderMenuContentSize;
    int Top = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
    int Width = m_MenuWidth - Config.decorBorderMenuContentSize * 2;
    int Height =
        m_OsdHeight - (m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_ButtonsHeight +
                       Config.decorBorderButtonSize * 2 + Config.decorBorderMenuContentSize * 2 + m_MarginItem);

    if (!ButtonsDrawn())
        Height += m_ButtonsHeight + Config.decorBorderButtonSize * 2;

    ComplexContent.Clear();

    m_MenuItemWidth = Width;

    Width -= m_ScrollBarWidth;  // First assume text is with scrollbar and redraw if not
    if (FixedFont) {
        ComplexContent.AddText(Text, true,
                               cRect(m_MarginItem, m_MarginItem, Width - m_MarginItem * 2, Height - m_MarginItem * 2),
                               Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), m_FontFixed);
        ComplexContent.SetScrollSize(m_FontFixedHeight);
    } else {
        ComplexContent.AddText(Text, true,
                               cRect(m_MarginItem, m_MarginItem, Width - m_MarginItem * 2, Height - m_MarginItem * 2),
                               Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), m_Font);
        ComplexContent.SetScrollSize(m_FontHeight);
    }

    bool Scrollable = ComplexContent.Scrollable(Height - m_MarginItem * 2);
    if (!Scrollable) {
        Width += m_ScrollBarWidth;  // Readd scrollbar width
        ComplexContent.Clear();
        if (FixedFont) {
            ComplexContent.AddText(
                Text, true, cRect(m_MarginItem, m_MarginItem, Width - m_MarginItem * 2, Height - m_MarginItem * 2),
                Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), m_FontFixed);
            ComplexContent.SetScrollSize(m_FontFixedHeight);
        } else {
            ComplexContent.AddText(
                Text, true, cRect(m_MarginItem, m_MarginItem, Width - m_MarginItem * 2, Height - m_MarginItem * 2),
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

    if (Scrollable)
        DrawScrollbar(ComplexContent.ScrollTotal(), ComplexContent.ScrollOffset(), ComplexContent.ScrollShown(),
                      ComplexContent.Top() - m_ScrollBarTop, ComplexContent.Height(), ComplexContent.ScrollOffset() > 0,
                      ComplexContent.ScrollOffset() + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(),
                      true);

    if (Config.MenuContentFullSize || Scrollable)
        DecorBorderDraw(Left, Top, Width, ComplexContent.ContentHeight(true), Config.decorBorderMenuContentSize,
                        Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg,
                        Config.decorBorderMenuContentBg);
    else
        DecorBorderDraw(Left, Top, Width, ComplexContent.ContentHeight(false), Config.decorBorderMenuContentSize,
                        Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg,
                        Config.decorBorderMenuContentBg);
}

int cFlatDisplayMenu::GetTextAreaWidth(void) const {
    return m_MenuWidth - (m_MarginItem * 2);
}

const cFont *cFlatDisplayMenu::GetTextAreaFont(bool FixedFont) const {
    const cFont *font = FixedFont ? m_FontFixed : m_Font;
    return font;
}

void cFlatDisplayMenu::SetMenuSortMode(eMenuSortMode MenuSortMode) {
    cString SortIcon("SortUnknown");
    switch (MenuSortMode) {
    case msmTime: SortIcon = "SortDate"; break;
    case msmName: SortIcon = "SortName"; break;
    case msmNumber: SortIcon = "SortNumber"; break;
    case msmProvider: SortIcon = "SortProvider"; break;
    case msmUnknown: return;  // Do not set search icon if it is unknown
    default: return;
    }

    TopBarSetMenuIconRight(*SortIcon);
}

void cFlatDisplayMenu::Flush(void) {
    if (!MenuPixmap) return;
    TopBarUpdate();

    if (Config.MenuFullOsd && !m_MenuFullOsdIsDrawn) {
        MenuPixmap->DrawRectangle(cRect(0, m_MenuItemLastHeight - Config.decorBorderMenuItemSize,
                                        m_MenuItemWidth + Config.decorBorderMenuItemSize * 2,
                                        MenuPixmap->ViewPort().Height() - m_MenuItemLastHeight + m_MarginItem),
                                  Theme.Color(clrItemSelableBg));
        // MenuPixmap->DrawRectangle(cRect(0, MenuPixmap->ViewPort().Height() - 5, m_MenuItemWidth +
        //                           Config.decorBorderMenuItemSize * 2, 5), Theme.Color(clrItemSelableBg));
        m_MenuFullOsdIsDrawn = true;
    }

    if (Config.MenuTimerShowCount && m_MenuCategory == mcTimer) {
        int TimerCount {0}, TimerActiveCount {0};
        LOCK_TIMERS_READ;
        for (const cTimer *Timer = Timers->First(); Timer; Timer = Timers->Next(Timer)) {
            ++TimerCount;
            if (Timer->HasFlags(tfActive))
                ++TimerActiveCount;
        }
        if (m_LastTimerCount != TimerCount || m_LastTimerActiveCount != TimerActiveCount) {
            m_LastTimerCount = TimerCount;
            m_LastTimerActiveCount = TimerActiveCount;
            cString NewTitle = cString::sprintf("%s (%d/%d)", *m_LastTitle, TimerActiveCount, TimerCount);
            TopBarSetTitleWithoutClear(*NewTitle);
        }
    }
    if (cVideoDiskUsage::HasChanged(m_VideoDiskUsageState))
        TopBarEnableDiskUsage();  // Keep 'DiskUsage' up to date

    m_Osd->Flush();
}

void cFlatDisplayMenu::ItemBorderInsertUnique(sDecorBorder ib) {
    std::list<sDecorBorder>::iterator it, end = ItemsBorder.end();
    for (it = ItemsBorder.begin(); it != end; ++it) {
        if ((*it).Left == ib.Left && (*it).Top == ib.Top) {
            (*it).Left = ib.Left;
            (*it).Top = ib.Top;
            (*it).Width = ib.Width;
            (*it).Height = ib.Height;
            (*it).Size = ib.Size;
            (*it).Type = ib.Type;
            (*it).ColorFg = ib.ColorFg;
            (*it).ColorBg = ib.ColorBg;
            (*it).From = ib.From;
            return;
        }
    }

    ItemsBorder.emplace_back(ib);
}

void cFlatDisplayMenu::ItemBorderDrawAllWithScrollbar(void) {
    std::list<sDecorBorder>::iterator it, end = ItemsBorder.end();
    for (it = ItemsBorder.begin(); it != end; ++it) {
        DecorBorderDraw((*it).Left, (*it).Top, (*it).Width - m_ScrollBarWidth, (*it).Height, (*it).Size, (*it).Type,
                        (*it).ColorFg, (*it).ColorBg, BorderMenuItem);
    }
}

void cFlatDisplayMenu::ItemBorderDrawAllWithoutScrollbar(void) {
    std::list<sDecorBorder>::iterator it, end = ItemsBorder.end();
    for (it = ItemsBorder.begin(); it != end; ++it) {
        DecorBorderDraw((*it).Left, (*it).Top, (*it).Width + m_ScrollBarWidth, (*it).Height, (*it).Size, (*it).Type,
                        (*it).ColorFg, (*it).ColorBg, BorderMenuItem);
    }
}

void cFlatDisplayMenu::ItemBorderClear(void) {
    ItemsBorder.clear();
}

bool cFlatDisplayMenu::IsRecordingOld(const cRecording *Recording, int Level) {
    /*std::string*/ m_RecFolder = GetRecordingName(Recording, Level, true).c_str();

    int value = Config.GetRecordingOldValue(m_RecFolder);
    if (value < 0) value = Config.MenuItemRecordingDefaultOldDays;
    if (value < 0) return false;

    int LastRecTimeFromFolder = GetLastRecTimeFromFolder(Recording, Level);
    time_t now = time(NULL);

    int DiffSecs = now - LastRecTimeFromFolder;
    int days = DiffSecs / (60 * 60 * 24);
    // dsyslog("flatPlus: m_RecFolder: %s LastRecTimeFromFolder: %d time: %d value: %d diff: %d days: %d",
    //          m_RecFolder.c_str(), LastRecTimeFromFolder, now, value, DiffSecs, days);
    if (days > value) return true;

    return false;
}

time_t cFlatDisplayMenu::GetLastRecTimeFromFolder(const cRecording *Recording, int Level) {
    /*std::string*/ m_RecFolder = GetRecordingName(Recording, Level, true).c_str();
    std::string RecFolder2("");
    time_t RecStart = Recording->Start();
    time_t RecStart2 {0};

    LOCK_RECORDINGS_READ;
    for (const cRecording *rec = Recordings->First(); rec; rec = Recordings->Next(rec)) {
        RecFolder2 = GetRecordingName(rec, Level, true).c_str();
        if (m_RecFolder == RecFolder2) {  // Recordings must be in the same folder
            RecStart2 = rec->Start();
            if (Config.MenuItemRecordingShowFolderDate == 1) {  // Newest
                if (RecStart2 > RecStart)
                    RecStart = RecStart2;
            } else if (Config.MenuItemRecordingShowFolderDate == 2)  // Oldest
                if (RecStart2 < RecStart)
                    RecStart = RecStart2;
        }
    }

    return RecStart;
}

std::string cFlatDisplayMenu::GetRecordingName(const cRecording *Recording, int Level, bool IsFolder) {
    if (!Recording) return "";

    std::string RecNamePart("");
    std::string RecName = Recording->Name();
    try {
        std::vector<std::string> tokens;
        tokens.reserve(6);  // Set to at least 6 entry's
        std::istringstream f(RecName.c_str());
        std::string s("");
        while (std::getline(f, s, FOLDERDELIMCHAR)) {
            tokens.emplace_back(s);
        }
        RecNamePart = tokens.at(Level);
    } catch (...) {
        RecNamePart = RecName.c_str();
    }

    if (Config.MenuItemRecordingClearPercent && IsFolder) {
        if (RecNamePart[0] == '%')
            RecNamePart.erase(0, 1);
    }

    return RecNamePart;
}

void cFlatDisplayMenu::InsertGenreInfo(const cEvent *Event, cString &Text) {
    bool FirstContent = true;
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
void cFlatDisplayMenu::InsertGenreInfo(const cEvent *Event, cString &Text, std::list<std::string> &GenreIcons) {
    bool FirstContent = true;
    for (int i{0}; Event->Contents(i); ++i) {
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

const char *cFlatDisplayMenu::GetGenreIcon(uchar genre) {
    switch (genre & 0xF0) {
    case ecgMovieDrama:
        switch (genre & 0x0F) {
        case 0x00: return "Movie_Drama";
        case 0x01: return "Detective_Thriller";
        case 0x02: return "Adventure_Western_War";
        case 0x03: return "Science Fiction_Fantasy_Horror";
        case 0x04: return "Comedy";
        case 0x05: return "Soap_Melodrama_Folkloric";
        case 0x06: return "Romance";
        case 0x07: return "Serious_Classical_Religious_Historical Movie_Drama";
        case 0x08: return "Adult Movie_Drama";
        default: return "Movie_Drama";
        }
        break;
    case ecgNewsCurrentAffairs:
        switch (genre & 0x0F) {
        case 0x00: return "News_Current Affairs";
        case 0x01: return "News_Weather Report";
        case 0x02: return "News Magazine";
        case 0x03: return "Documentary";
        case 0x04: return "Discussion_Interview_Debate";
        default: return "News_Current Affairs";
        }
        break;
    case ecgShow:
        switch (genre & 0x0F) {
        case 0x00: return "Show_Game Show";
        case 0x01: return "Game Show_Quiz_Contest";
        case 0x02: return "Variety Show";
        case 0x03: return "Talk Show";
        default: return "Show_Game Show";
        }
        break;
    case ecgSports:
        switch (genre & 0x0F) {
        case 0x00: return "Sports";
        case 0x01: return "Special Event";
        case 0x02: return "Sport Magazine";
        case 0x03: return "Football_Soccer";
        case 0x04: return "Tennis_Squash";
        case 0x05: return "Team Sports";
        case 0x06: return "Athletics";
        case 0x07: return "Motor Sport";
        case 0x08: return "Water Sport";
        case 0x09: return "Winter Sports";
        case 0x0A: return "Equestrian";
        case 0x0B: return "Martial Sports";
        default: return "Sports";
        }
        break;
    case ecgChildrenYouth:
        switch (genre & 0x0F) {
        case 0x00: return "Childrens_Youth Programme";
        case 0x01: return "Pre-school Childrens Programme";
        case 0x02: return "Entertainment Programme for 6 to 14";
        case 0x03: return "Entertainment Programme for 10 to 16";
        case 0x04: return "Informational_Educational_School Programme";
        case 0x05: return "Cartoons_Puppets";
        default: return "Childrens_Youth Programme";
        }
        break;
    case ecgMusicBalletDance:
        switch (genre & 0x0F) {
        case 0x00: return "Music_Ballet_Dance";
        case 0x01: return "Rock_Pop";
        case 0x02: return "Serious_Classical Music";
        case 0x03: return "Folk_Traditional Music";
        case 0x04: return "Jazz";
        case 0x05: return "Musical_Opera";
        case 0x06: return "Ballet";
        default: return "Music_Ballet_Dance";
        }
        break;
    case ecgArtsCulture:
        switch (genre & 0x0F) {
        case 0x00: return "Arts_Culture";
        case 0x01: return "Performing Arts";
        case 0x02: return "Fine Arts";
        case 0x03: return "Religion";
        case 0x04: return "Popular Culture_Traditional Arts";
        case 0x05: return "Literature";
        case 0x06: return "Film_Cinema";
        case 0x07: return "Experimental Film_Video";
        case 0x08: return "Broadcasting_Press";
        case 0x09: return "New Media";
        case 0x0A: return "Arts_Culture Magazine";
        case 0x0B: return "Fashion";
        default: return "Arts_Culture";
        }
        break;
    case ecgSocialPoliticalEconomics:
        switch (genre & 0x0F) {
        case 0x00: return "Social_Political_Economics";
        case 0x01: return "Magazine_Report_Documentary";
        case 0x02: return "Economics_Social Advisory";
        case 0x03: return "Remarkable People";
        default: return "Social_Political_Economics";
        }
        break;
    case ecgEducationalScience:
        switch (genre & 0x0F) {
        case 0x00: return "Education_Science_Factual";
        case 0x01: return "Nature_Animals_Environment";
        case 0x02: return "Technology_Natural Sciences";
        case 0x03: return "Medicine_Physiology_Psychology";
        case 0x04: return "Foreign Countries_Expeditions";
        case 0x05: return "Social_Spiritual Sciences";
        case 0x06: return "Further Education";
        case 0x07: return "Languages";
        default: return "Education_Science_Factual";
        }
        break;
    case ecgLeisureHobbies:
        switch (genre & 0x0F) {
        case 0x00: return "Leisure_Hobbies";
        case 0x01: return "Tourism_Travel";
        case 0x02: return "Handicraft";
        case 0x03: return "Motoring";
        case 0x04: return "Fitness_Health";
        case 0x05: return "Cooking";
        case 0x06: return "Advertisement_Shopping";
        case 0x07: return "Gardening";
        default: return "Leisure_Hobbies";
        }
        break;
    case ecgSpecial:
        switch (genre & 0x0F) {
        case 0x00: return "Original Language";
        case 0x01: return "Black & White";
        case 0x02: return "Unpublished";
        case 0x03: return "Live Broadcast";
        default: return "Original Language";
        }
        break;
    default:
        isyslog("flatPlus: Genre not found: %x", genre);
    }
    return "";
}

/* Widgets */
void cFlatDisplayMenu::DrawMainMenuWidgets(void) {
    if (!MenuPixmap) return;

    int wLeft = m_OsdWidth * Config.MainMenuItemScale + m_MarginItem + Config.decorBorderMenuContentSize +
                Config.decorBorderMenuItemSize;
    int wTop = m_TopBarHeight + m_MarginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;

    int wWidth = m_OsdWidth - wLeft - Config.decorBorderMenuContentSize;
    int wHeight = MenuPixmap->ViewPort().Height() - m_MarginItem * 2;
    int ContentTop {0};

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

    std::sort(widgets.begin(), widgets.end(), pairCompareIntString);
    std::pair<int, std::string> PairWidget {};
    std::string widget("");
    int AddHeight {0};
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

    DecorBorderDraw(wLeft, wTop, wWidth, ContentWidget.ContentHeight(false), Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg,
                    BorderMMWidget);
}

int cFlatDisplayMenu::DrawMainMenuWidgetDVBDevices(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img = ImgLoader.LoadIcon("widgets/dvb_devices", m_FontHeight, m_FontHeight - m_MarginItem * 2);
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("DVB Devices"), false, cRect(m_MarginItem * 2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    // Check device which currently displays live tv
    int DeviceLiveTV = -1;
    cDevice *PrimaryDevice = cDevice::PrimaryDevice();
    if (PrimaryDevice) {
        if (!PrimaryDevice->Replaying() || PrimaryDevice->Transferring())
            DeviceLiveTV = cDevice::ActualDevice()->DeviceNumber();
        else
            DeviceLiveTV = PrimaryDevice->DeviceNumber();
    }

    // Check currently recording devices
    const int NumDevices = cDevice::NumDevices();
    bool RecDevices[NumDevices] {false};  // Array initialised to false

    LOCK_TIMERS_READ;
    for (const cTimer *timer = Timers->First(); timer; timer = Timers->Next(timer)) {
        if (!timer->Recording())
            continue;

        if (cRecordControl *RecordControl = cRecordControls::GetRecordControl(timer)) {
            const cDevice *RecDevice = RecordControl->Device();
            if (RecDevice)
                RecDevices[RecDevice->DeviceNumber()] = true;
        }
    }  // for cTimer
    int ActualNumDevices {0};
    cString ChannelName(""), str(""), StrDevice("");
    for (int i {0}; i < NumDevices; ++i) {
        if (ContentTop + m_MarginItem > MenuPixmap->ViewPort().Height())
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

        int left = m_MarginItem;
        int FontSmlWidthX = m_FontSml->Width('X');  // Width of one X
        if (NumDevices <= 9) {
            ContentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);

            left += FontSmlWidthX * 2;
        } else {
            ContentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  FontSmlWidthX * 2, m_FontSmlHeight, taRight);

            left += FontSmlWidthX * 3;
        }
        str = *(device->DeviceType());
        ContentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              FontSmlWidthX * 7, m_FontSmlHeight, taLeft);

        left += FontSmlWidthX * 8;
        ContentWidget.AddText(*StrDevice, false, cRect(left, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml);

        ContentTop += m_FontSmlHeight;
    }  // for NumDevices

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetActiveTimers(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img = ImgLoader.LoadIcon("widgets/active_timers", m_FontHeight, m_FontHeight - m_MarginItem * 2);
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("Timer"), false, cRect(m_MarginItem * 2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    // Check if remotetimers plugin is available
    static cPlugin *pRemoteTimers = cPluginManager::GetPlugin("remotetimers");
    time_t now = time(NULL);
    if ((Config.MainMenuWidgetActiveTimerShowRemoteActive || Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
        pRemoteTimers && (now - m_RemoteTimersLastRefresh) > Config.MainMenuWidgetActiveTimerShowRemoteRefreshTime) {
        m_RemoteTimersLastRefresh = now;
        cString ErrorMsg("");
        pRemoteTimers->Service("RemoteTimers::RefreshTimers-v1.0", &ErrorMsg);
    }

    // Look for timers
    cVector<const cTimer *> TimerRec;
    cVector<const cTimer *> TimerActive;
    cVector<const cTimer *> TimerRemoteRec;
    cVector<const cTimer *> TimerRemoteActive;

    LOCK_TIMERS_READ;
    for (const cTimer *ti = Timers->First(); ti; ti = Timers->Next(ti)) {
        if (ti->HasFlags(tfRecording) && Config.MainMenuWidgetActiveTimerShowRecording)
            TimerRec.Append(ti);

        if (ti->HasFlags(tfActive) && !ti->HasFlags(tfRecording) && Config.MainMenuWidgetActiveTimerShowActive)
            TimerActive.Append(ti);

        if (TimerRec.Size() + TimerActive.Size() >= Config.MainMenuWidgetActiveTimerMaxCount)
            break;
    }

    LOCK_SCHEDULES_READ;
    if ((Config.MainMenuWidgetActiveTimerShowRemoteActive || Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
        pRemoteTimers && TimerRec.Size() + TimerActive.Size() < Config.MainMenuWidgetActiveTimerMaxCount) {
        cTimer *RemoteTimer {nullptr};
        while (pRemoteTimers->Service("RemoteTimers::ForEach-v1.0", &RemoteTimer) && RemoteTimer != NULL) {
            RemoteTimer->SetEventFromSchedule(Schedules);  // Make sure the event is current
            if (RemoteTimer->HasFlags(tfRecording) && Config.MainMenuWidgetActiveTimerShowRemoteRecording)
                TimerRemoteRec.Append(RemoteTimer);
            if (RemoteTimer->HasFlags(tfActive) && !RemoteTimer->HasFlags(tfRecording) &&
                Config.MainMenuWidgetActiveTimerShowRemoteActive)
                TimerRemoteActive.Append(RemoteTimer);
        }
    }
    TimerRec.Sort(CompareTimers);
    TimerActive.Sort(CompareTimers);
    TimerRemoteRec.Sort(CompareTimers);
    TimerRemoteActive.Sort(CompareTimers);

    if ((TimerRec.Size() == 0 && TimerActive.Size() == 0 && TimerRemoteRec.Size() == 0 &&
         TimerRemoteActive.Size() == 0) &&
        Config.MainMenuWidgetActiveTimerHideEmpty)
        return 0;
    else if (TimerRec.Size() == 0 && TimerActive.Size() == 0 && TimerRemoteRec.Size() == 0 &&
             TimerRemoteActive.Size() == 0) {
        ContentWidget.AddText(tr("no active/recording timer"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem * 2);
    } else {
        int count = -1, RemoteCount = -1;
        // First recording timer
        if (Config.MainMenuWidgetActiveTimerShowRecording) {
            cString StrTimer("");
            int TimerRecSize = TimerRec.Size();
            for (int i {0}; i < TimerRecSize; ++i) {
                ++count;
                if (ContentTop + m_MarginItem > MenuPixmap->ViewPort().Height())
                    break;

                if (count >= Config.MainMenuWidgetActiveTimerMaxCount)
                    break;

                const cChannel *Channel = (TimerRec[i])->Channel();
                // const cEvent *Event = Timer->Event();
                StrTimer = "";  // Reset string
                if ((Config.MainMenuWidgetActiveTimerShowRemoteActive ||
                     Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
                    pRemoteTimers && (TimerRemoteRec.Size() > 0 || TimerRemoteActive.Size() > 0))
                    StrTimer.Append("L");
                StrTimer.Append(cString::sprintf("%d: ", count + 1));
                if (Channel)
                    StrTimer.Append(cString::sprintf("%s - ", Channel->Name()));
                else
                    StrTimer.Append(cString::sprintf("%s - ", tr("Unknown")));

                StrTimer.Append((TimerRec[i])->File());

                ContentWidget.AddText(*StrTimer, false,
                                      cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                      Theme.Color(clrTopBarRecordingActiveFg), Theme.Color(clrMenuEventBg), m_FontSml,
                                      wWidth - m_MarginItem * 2);

                ContentTop += m_FontSmlHeight;
            }
        }
        if (Config.MainMenuWidgetActiveTimerShowActive) {
            cString StrTimer("");
            int TimerActiveSize = TimerActive.Size();
            for (int i {0}; i < TimerActiveSize; ++i) {
                ++count;
                if (ContentTop + m_MarginItem > MenuPixmap->ViewPort().Height())
                    break;

                if (count >= Config.MainMenuWidgetActiveTimerMaxCount)
                    break;

                const cChannel *Channel = (TimerActive[i])->Channel();
                // const cEvent *Event = Timer->Event();
                StrTimer = "";  // Reset string
                if ((Config.MainMenuWidgetActiveTimerShowRemoteActive ||
                     Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
                    pRemoteTimers && (TimerRemoteRec.Size() > 0 || TimerRemoteActive.Size() > 0))
                    StrTimer.Append("L");
                StrTimer.Append(cString::sprintf("%d: ", count + 1));
                if (Channel)
                    StrTimer.Append(cString::sprintf("%s - ", Channel->Name()));
                else
                    StrTimer.Append(cString::sprintf("%s - ", tr("Unknown")));
                StrTimer.Append((TimerActive[i])->File());

                ContentWidget.AddText(*StrTimer, false,
                                      cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                      Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                      wWidth - m_MarginItem * 2);

                ContentTop += m_FontSmlHeight;
            }
        }
        if (Config.MainMenuWidgetActiveTimerShowRemoteRecording) {
            cString StrTimer("");
            int TimerRemoteRecSize = TimerRemoteRec.Size();
            for (int i {0}; i < TimerRemoteRecSize; ++i) {
                ++RemoteCount;
                if (ContentTop + m_MarginItem > MenuPixmap->ViewPort().Height())
                    break;

                if (count + RemoteCount >= Config.MainMenuWidgetActiveTimerMaxCount)
                    break;

                const cChannel *Channel = (TimerRemoteRec[i])->Channel();
                // const cEvent *Event = Timer->Event();
                StrTimer = "";  // Reset string
                StrTimer.Append(cString::sprintf("R%d: ", RemoteCount + 1));
                if (Channel)
                    StrTimer.Append(cString::sprintf("%s - ", Channel->Name()));
                else
                    StrTimer.Append(cString::sprintf("%s - ", tr("Unknown")));

                StrTimer.Append((TimerRemoteRec[i])->File());

                ContentWidget.AddText(*StrTimer, false,
                                      cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                      Theme.Color(clrTopBarRecordingActiveFg), Theme.Color(clrMenuEventBg), m_FontSml,
                                      wWidth - m_MarginItem * 2);

                ContentTop += m_FontSmlHeight;
            }
        }
        if (Config.MainMenuWidgetActiveTimerShowRemoteActive) {
            cString StrTimer("");
            int TimerRemoteActiveSize = TimerRemoteActive.Size();
            for (int i {0}; i < TimerRemoteActiveSize; ++i) {
                ++RemoteCount;
                if (ContentTop + m_MarginItem > MenuPixmap->ViewPort().Height())
                    break;

                if (count + RemoteCount >= Config.MainMenuWidgetActiveTimerMaxCount)
                    break;

                const cChannel *Channel = (TimerRemoteActive[i])->Channel();
                // const cEvent *Event = Timer->Event();
                StrTimer = "";  // Reset string
                StrTimer.Append(cString::sprintf("R%d: ", RemoteCount + 1));
                if (Channel)
                    StrTimer.Append(cString::sprintf("%s - ", Channel->Name()));
                else
                    StrTimer.Append(cString::sprintf("%s - ", tr("Unknown")));

                StrTimer.Append((TimerRemoteActive[i])->File());

                ContentWidget.AddText(*StrTimer, false,
                                      cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                      Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                      wWidth - m_MarginItem * 2);

                ContentTop += m_FontSmlHeight;
            }
        }
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetLastRecordings(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img = ImgLoader.LoadIcon("widgets/last_recordings", m_FontHeight, m_FontHeight - m_MarginItem * 2);
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("Last Recordings"), false, cRect(m_MarginItem * 2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    std::vector<std::pair<time_t, std::string>> Recs;
    Recs.reserve(512);  // Set to at least 512 entry's
    time_t RecStart {0};
    int Minutes {0};
    cString DateTime(""), Length("");
    std::string StrRec("");
    LOCK_RECORDINGS_READ;
    for (const cRecording *rec = Recordings->First(); rec; rec = Recordings->Next(rec)) {
        RecStart = rec->Start();

        Minutes = (rec->LengthInSeconds() + 30) / 60;
        Length = cString::sprintf("%02d:%02d", Minutes / 60, Minutes % 60);
        DateTime = cString::sprintf("%s  %s  %s", *ShortDateString(RecStart), *TimeString(RecStart), *Length);

        StrRec = *(cString::sprintf("%s - %s", *DateTime, rec->Name()));
        Recs.emplace_back(std::make_pair(RecStart, StrRec));
    }
    // Sort by RecStart
    std::sort(Recs.begin(), Recs.end(), PairCompareTimeStringDesc);
    std::pair<time_t, std::string> PairRec {};
    int index {0};
    cString Rec("");
    while (!Recs.empty() && index < Config.MainMenuWidgetLastRecMaxCount) {
        if (ContentTop + m_MarginItem > MenuPixmap->ViewPort().Height())
            continue;

        PairRec = Recs.back();
        Recs.pop_back();
        Rec = PairRec.second.c_str();

        ContentWidget.AddText(*Rec, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem * 2);
        ContentTop += m_FontSmlHeight;
        ++index;
    }
    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetTimerConflicts(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img = ImgLoader.LoadIcon("widgets/timer_conflicts", m_FontHeight, m_FontHeight - m_MarginItem * 2);
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("Timer Conflicts"), false, cRect(m_MarginItem * 2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    int NumConflicts = GetEpgsearchConflicts();  // Get conflicts from plugin Epgsearch
    if (NumConflicts == 0 && Config.MainMenuWidgetTimerConflictsHideEmpty) {
        return 0;
    } else if (NumConflicts == 0) {
        ContentWidget.AddText(tr("no timer conflicts"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem * 2);
    } else {
        cString str = cString::sprintf("%s: %d", tr("timer conflicts"), NumConflicts);
        ContentWidget.AddText(*str, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem * 2);
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetSystemInformation(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img = ImgLoader.LoadIcon("widgets/system_information", m_FontHeight, m_FontHeight - m_MarginItem * 2);
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("System Information"), false, cRect(m_MarginItem * 2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    cString ExecFile = cString::sprintf("\"%s/system_information/system_information\"", WIDGETFOLDER);
    int r = system(*ExecFile);
    r += 0;  // Prevent warning for unused variable

    cString ConfigsPath = cString::sprintf("%s/system_information/", WIDGETOUTPUTPATH);

    std::vector<std::string> files;
    files.reserve(64);  // Set to at least 64 entry's

    cReadDir d(*ConfigsPath);
    struct dirent *e;
    std::string FileName(""), num("");
    std::size_t found {0};
    while ((e = d.Next()) != NULL) {
        FileName = e->d_name;
        found = FileName.find('_');
        if (found != std::string::npos) {
            num = FileName.substr(0, found);
            if (atoi(num.c_str()) > 0)
                files.emplace_back(e->d_name);
        }
    }
    cString str("");
    int Column {1};
    int ContentLeft = m_MarginItem;
    std::sort(files.begin(), files.end(), StringCompare);
    std::size_t FilesSize = files.size();
    if (FilesSize == 0) {
        str = cString::sprintf("%s - %s", tr("no information available please check the script"), *ExecFile);
        ContentWidget.AddText(*str, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem * 2);
    } else {
        std::string item(""), ItemContent("");
        cString ItemFilename("");
        for (unsigned i = 0; i < FilesSize; ++i) {
            // Check for height
            if (ContentTop + m_MarginItem > MenuPixmap->ViewPort().Height())
                break;

            FileName = files[i];
            found = FileName.find('_');
            if (found != std::string::npos) {
                num = FileName.substr(0, found);
                if (atoi(num.c_str()) > 0) {
                    item = FileName.substr(found + 1, FileName.length() - found);
                    ItemFilename = cString::sprintf("%s/system_information/%s", WIDGETOUTPUTPATH, FileName.c_str());
                    std::ifstream file(*ItemFilename, std::ifstream::in);
                    if (file.is_open()) {
                        std::getline(file, ItemContent);

                        if (!strcmp(item.c_str(), "sys_version")) {
                            if (Column == 2) {
                                Column = 1;
                                ContentTop += m_FontSmlHeight;
                                ContentLeft = m_MarginItem;
                            }
                            str = cString::sprintf("%s: %s", tr("System Version"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false,
                                cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            ContentTop += m_FontSmlHeight;
                        } else if (!item.compare("kernel_version")) {
                            if (Column == 2) {
                                Column = 1;
                                ContentTop += m_FontSmlHeight;
                                ContentLeft = m_MarginItem;
                            }
                            str = cString::sprintf("%s: %s", tr("Kernel Version"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false,
                                cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            ContentTop += m_FontSmlHeight;
                        } else if (!item.compare("uptime")) {
                            str = cString::sprintf("%s: %s", tr("Uptime"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("load")) {
                            str = cString::sprintf("%s: %s", tr("Load"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("processes")) {
                            str = cString::sprintf("%s: %s", tr("Processes"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("mem_usage")) {
                            str = cString::sprintf("%s: %s", tr("Memory Usage"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("swap_usage")) {
                            str = cString::sprintf("%s: %s", tr("Swap Usage"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("root_usage")) {
                            str = cString::sprintf("%s: %s", tr("Root Usage"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("video_usage")) {
                            str = cString::sprintf("%s: %s", tr("Video Usage"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("vdr_cpu_usage")) {
                            str = cString::sprintf("%s: %s", tr("VDR CPU Usage"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("vdr_mem_usage")) {
                            str = cString::sprintf("%s: %s", tr("VDR MEM Usage"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("cpu")) {
                            str = cString::sprintf("%s: %s", tr("Temp CPU"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("gpu")) {
                            str = cString::sprintf("%s: %s", tr("Temp GPU"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("pccase")) {
                            str = cString::sprintf("%s: %s", tr("Temp PC-Case"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("motherboard")) {
                            str = cString::sprintf("%s: %s", tr("Temp MB"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("updates")) {
                            str = cString::sprintf("%s: %s", tr("Updates"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        } else if (!item.compare("security_updates")) {
                            str = cString::sprintf("%s: %s", tr("Security Updates"), ItemContent.c_str());
                            ContentWidget.AddText(
                                *str, false, cRect(ContentLeft, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                wWidth - m_MarginItem * 2);
                            if (Column == 1) {
                                Column = 2;
                                ContentLeft = wWidth / 2;
                            } else {
                                Column = 1;
                                ContentLeft = m_MarginItem;
                                ContentTop += m_FontSmlHeight;
                            }
                        }
                        file.close();
                    }
                }
            }
        }
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetSystemUpdates(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img = ImgLoader.LoadIcon("widgets/system_updates", m_FontHeight, m_FontHeight - m_MarginItem * 2);
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("System Updates"), false, cRect(m_MarginItem * 2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    int updates {0}, SecurityUpdates {0};
    cString ItemFilename = cString::sprintf("%s/system_updatestatus/updates", WIDGETOUTPUTPATH);
    std::ifstream file(*ItemFilename, std::ifstream::in);
    if (file.is_open()) {
        std::string cont("");
        std::getline(file, cont);
        updates = atoi(cont.c_str());
        file.close();
    } else {
        updates = -1;
    }

    ItemFilename = cString::sprintf("%s/system_updatestatus/security_updates", WIDGETOUTPUTPATH);
    file.open(*ItemFilename, std::ifstream::in);
    if (file.is_open()) {
        std::string cont("");
        std::getline(file, cont);
        SecurityUpdates = atoi(cont.c_str());
        file.close();
    } else {
        SecurityUpdates = -1;
    }

    if (updates == -1 || SecurityUpdates == -1) {
        ContentWidget.AddText(tr("Updatestatus not available please check the widget"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem * 2);
    } else if (updates == 0 && SecurityUpdates == 0 && Config.MainMenuWidgetSystemUpdatesHideIfZero) {
        return 0;
    } else {
        cString str = cString::sprintf("%s: %d", tr("Updates"), updates);
        ContentWidget.AddText(*str, false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem * 2);
        str = cString::sprintf("%s: %d", tr("Security Updates"), SecurityUpdates);
        ContentWidget.AddText(
            *str, false, cRect(wWidth / 2 + m_MarginItem, ContentTop, wWidth / 2 - m_MarginItem * 2, m_FontSmlHeight),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem * 2);
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetTemperatures(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cImage *img = ImgLoader.LoadIcon("widgets/temperatures", m_FontHeight, m_FontHeight - m_MarginItem * 2);
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(tr("Temperatures"), false, cRect(m_MarginItem * 2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    cString ExecFile =
        cString::sprintf("cd \"%s/temperatures\"; \"%s/temperatures/temperatures\"", WIDGETFOLDER, WIDGETFOLDER);
    int r = system(*ExecFile);
    r += 0;  // Prevent warning for unused variable

    int CountTemps {0};

    std::string TempCPU(""), TempCase(""), TempMB(""), TempGPU("");
    cString ItemFilename = cString::sprintf("%s/temperatures/cpu", WIDGETOUTPUTPATH);
    std::ifstream file(*ItemFilename, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, TempCPU);
        file.close();
        ++CountTemps;
    } else {
        TempCPU = "-1";
    }

    ItemFilename = cString::sprintf("%s/temperatures/pccase", WIDGETOUTPUTPATH);
    file.open(*ItemFilename, std::ifstream::in);
    if (file.is_open()) {
        // std::string cont("");
        std::getline(file, TempCase);
        file.close();
        ++CountTemps;
    } else {
        TempCase = "-1";
    }

    ItemFilename = cString::sprintf("%s/temperatures/motherboard", WIDGETOUTPUTPATH);
    file.open(*ItemFilename, std::ifstream::in);
    if (file.is_open()) {
        // std::string cont("");
        std::getline(file, TempMB);
        file.close();
        ++CountTemps;
    } else {
        TempMB = "-1";
    }
    ItemFilename = cString::sprintf("%s/temperatures/gpu", WIDGETOUTPUTPATH);
    file.open(*ItemFilename, std::ifstream::in);
    if (file.is_open()) {
        // std::string cont("");
        std::getline(file, TempGPU);
        file.close();
        ++CountTemps;
    } else {
        TempGPU = "-1";
    }

    if (!strcmp(TempCPU.c_str(), "-1") && !strcmp(TempCase.c_str(), "-1") && !strcmp(TempMB.c_str(), "-1") &&
        !strcmp(TempGPU.c_str(), "-1")) {
        ContentWidget.AddText(tr("Temperatures not available please check the widget"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem * 2);
    } else {
        int Left = m_MarginItem;
        int AddLeft = wWidth / (CountTemps);
        cString str("");
        if (strcmp(TempCPU.c_str(), "-1")) {
            str = cString::sprintf("%s: %s", tr("CPU"), TempCPU.c_str());
            ContentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  wWidth - m_MarginItem * 2);
            Left += AddLeft;
        }
        if (strcmp(TempCase.c_str(), "-1")) {
            str = cString::sprintf("%s: %s", tr("PC-Case"), TempCase.c_str());
            ContentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth / 3 - m_MarginItem * 2, m_FontSmlHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                                  wWidth - m_MarginItem * 2);
            Left += AddLeft;
        }
        if (strcmp(TempMB.c_str(), "-1")) {
            str = cString::sprintf("%s: %s", tr("MB"), TempMB.c_str());
            ContentWidget.AddText(
                *str, false, cRect(Left, ContentTop, wWidth / 3 * 2 - m_MarginItem * 2, m_FontSmlHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem * 2);
            Left += AddLeft;
        }
        if (strcmp(TempGPU.c_str(), "-1")) {
            str = cString::sprintf("%s: %s", tr("GPU"), TempGPU.c_str());
            ContentWidget.AddText(
                *str, false, cRect(Left, ContentTop, wWidth / 3 * 2 - m_MarginItem * 2, m_FontSmlHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem * 2);
        }
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetCommand(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    cString ExecFile = cString::sprintf("\"%s/command_output/command\"", WIDGETFOLDER);
    int r = system(*ExecFile);
    r += 0;  // Prevent warning for unused variable

    std::string Title("");
    cString ItemFilename = cString::sprintf("%s/command_output/title", WIDGETOUTPUTPATH);
    std::ifstream file(*ItemFilename, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, Title);
        file.close();
    } else {
        Title = tr("no title available");
    }

    cImage *img = ImgLoader.LoadIcon("widgets/command_output", m_FontHeight, m_FontHeight - m_MarginItem * 2);
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    ContentWidget.AddText(Title.c_str(), false, cRect(m_MarginItem * 2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    std::string Output("");
    ItemFilename = cString::sprintf("%s/command_output/output", WIDGETOUTPUTPATH);
    file.open(*ItemFilename, std::ifstream::in);
    if (file.is_open()) {
        for (; std::getline(file, Output);) {
            if (ContentTop + m_MarginItem > MenuPixmap->ViewPort().Height())
                break;
            ContentWidget.AddText(
                Output.c_str(), false, cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml, wWidth - m_MarginItem * 2);
            ContentTop += m_FontSmlHeight;
        }
        file.close();
    } else {
        ContentWidget.AddText(tr("no output available"), false,
                              cRect(m_MarginItem, ContentTop, wWidth - m_MarginItem * 2, m_FontSmlHeight),
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontSml,
                              wWidth - m_MarginItem * 2);
    }

    return ContentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetWeather(int wLeft, int wWidth, int ContentTop) {
    if (ContentTop + m_FontHeight + 6 + m_FontSmlHeight > MenuPixmap->ViewPort().Height())
        return -1;

    std::string Location("");
    cString FileName = cString::sprintf("%s/weather/weather.location", WIDGETOUTPUTPATH);
    std::ifstream file(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, Location);
        file.close();
    } else {
        Location = tr("Unknown");
    }

    std::string TempToday("");
    FileName = cString::sprintf("%s/weather/weather.0.temp", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, TempToday);
        file.close();
    }
    //* Declared in 'baserender.h'
    // Deleted in '~cFlatDisplayMenu', because of segfault when deleted here or in 'DrawMainMenuWidgets'
    m_FontTempSml = cFont::CreateFont(Setup.FontOsd, Setup.FontOsdSize * (1.0 / 2.0));

    cImage *img = ImgLoader.LoadIcon("widgets/weather", m_FontHeight, m_FontHeight - m_MarginItem * 2);
    if (img)
        ContentWidget.AddImage(img, cRect(m_MarginItem, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));

    cString Title = cString::sprintf("%s - %s %s", tr("Weather"), Location.c_str(), TempToday.c_str());
    ContentWidget.AddText(*Title, false, cRect(m_MarginItem * 2 + m_FontHeight, ContentTop, 0, 0),
                          Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), m_Font);
    ContentTop += m_FontHeight;
    ContentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
    ContentTop += 6;

    int left = m_MarginItem;
    std::string icon(""), summary(""), TempMax(""), TempMin(""), prec("");
    cString DayName(""), PrecString("0%"), StrWeekDayName(""), WeatherIcon("");
    double p {0.0};
    time_t t = time(NULL), t2 = time(NULL);
    struct tm tm_r;
    localtime_r(&t, &tm_r);
    for (int index {0}; index < Config.MainMenuWidgetWeatherDays; ++index) {
        FileName = cString::sprintf("%s/weather/weather.%d.icon", WIDGETOUTPUTPATH, index);
        file.open(*FileName, std::ifstream::in);
        if (file.is_open()) {
            std::getline(file, icon);
            file.close();
        } else
            continue;

        FileName = cString::sprintf("%s/weather/weather.%d.summary", WIDGETOUTPUTPATH, index);
        file.open(*FileName, std::ifstream::in);
        if (file.is_open()) {
            std::getline(file, summary);
            file.close();
        } else
            continue;

        FileName = cString::sprintf("%s/weather/weather.%d.tempMax", WIDGETOUTPUTPATH, index);
        file.open(*FileName, std::ifstream::in);
        if (file.is_open()) {
            std::getline(file, TempMax);
            file.close();
        } else
            continue;

        FileName = cString::sprintf("%s/weather/weather.%d.tempMin", WIDGETOUTPUTPATH, index);
        file.open(*FileName, std::ifstream::in);
        if (file.is_open()) {
            std::getline(file, TempMin);
            file.close();
        } else
            continue;

        FileName = cString::sprintf("%s/weather/weather.%d.precipitation", WIDGETOUTPUTPATH, index);
        file.open(*FileName, std::ifstream::in);
        if (file.is_open()) {
            std::getline(file, prec);
            file.close();
            std::istringstream istr(prec);
            istr.imbue(std::locale("C"));
            istr >> p;
            p = p * 100.0;
            p = RoundUp(p, 10);
            PrecString = cString::sprintf("%.0f%%", p);
        } else
            continue;

        tm_r.tm_mday += index;
        /* time_t */ t2 = mktime(&tm_r);

        int FontTempSmlHeight = m_FontTempSml->Height();
        if (Config.MainMenuWidgetWeatherType == 0) {  // Short
            if (left + m_FontHeight * 2 + m_FontTempSml->Width("-99,9°C") + m_FontTempSml->Width("XXXX") +
                    m_MarginItem * 6 >
                wWidth)
                break;
            if (index > 0) {
                ContentWidget.AddText("|", false, cRect(left, ContentTop, 0, 0), Theme.Color(clrMenuEventFontInfo),
                                      Theme.Color(clrMenuEventBg), m_Font);
                left += m_Font->Width('|') + m_MarginItem * 2;
            }

            WeatherIcon = cString::sprintf("widgets/%s", icon.c_str());
            img = ImgLoader.LoadIcon(*WeatherIcon, m_FontHeight, m_FontHeight - m_MarginItem * 2);
            if (img) {
                ContentWidget.AddImage(img, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight + m_MarginItem;
            }
            int wtemp = MyMax(m_FontTempSml->Width(TempMax.c_str()), m_FontTempSml->Width(TempMin.c_str()));
            ContentWidget.AddText(TempMax.c_str(), false, cRect(left, ContentTop, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml, wtemp,
                                  FontTempSmlHeight, taRight);
            ContentWidget.AddText(TempMin.c_str(), false, cRect(left, ContentTop + FontTempSmlHeight, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml, wtemp,
                                  FontTempSmlHeight, taRight);

            left += wtemp + m_MarginItem;

            img = ImgLoader.LoadIcon("widgets/umbrella", m_FontHeight, m_FontHeight - m_MarginItem * 2);
            if (img) {
                ContentWidget.AddImage(img, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight - m_MarginItem;
            }
            ContentWidget.AddText(*PrecString, false,
                                  cRect(left, ContentTop + (m_FontHeight / 2 - FontTempSmlHeight / 2), 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml);
            left += m_FontTempSml->Width(*PrecString) + m_MarginItem * 2;
        } else {  // Long
            if (ContentTop + m_MarginItem > MenuPixmap->ViewPort().Height())
                break;

            left = m_MarginItem;
            StrWeekDayName = WeekDayName(t2);
            DayName = cString::sprintf("%s ", *StrWeekDayName);
            ContentWidget.AddText(*DayName, false, cRect(left, ContentTop, wWidth - m_MarginItem * 2, m_FontHeight),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_Font,
                                  wWidth - m_MarginItem * 2);
            left += m_Font->Width("XXXX") + m_MarginItem;

            WeatherIcon = cString::sprintf("widgets/%s", icon.c_str());
            img = ImgLoader.LoadIcon(*WeatherIcon, m_FontHeight, m_FontHeight - m_MarginItem * 2);
            if (img) {
                ContentWidget.AddImage(img, cRect(left, ContentTop + m_MarginItem, m_FontHeight, m_FontHeight));
                left += m_FontHeight + m_MarginItem;
            }
            ContentWidget.AddText(TempMax.c_str(), false, cRect(left, ContentTop, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml,
                                  m_FontTempSml->Width("-99,9°C"), FontTempSmlHeight, taRight);
            ContentWidget.AddText(TempMin.c_str(), false, cRect(left, ContentTop + FontTempSmlHeight, 0, 0),
                                  Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml,
                                  m_FontTempSml->Width("-99,9°C"), FontTempSmlHeight, taRight);

            left += m_FontTempSml->Width("-99,9°C ") + m_MarginItem;

            img = ImgLoader.LoadIcon("widgets/umbrella", m_FontHeight, m_FontHeight - m_MarginItem * 2);
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
                summary.c_str(), false,
                cRect(left, ContentTop + (m_FontHeight / 2 - FontTempSmlHeight / 2), wWidth - left, m_FontHeight),
                Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), m_FontTempSml, wWidth - left);

            ContentTop += m_FontHeight;
        }
    }  // for Config.MainMenuWidgetWeatherDays
    return ContentWidget.ContentHeight(false);
}

void cFlatDisplayMenu::PreLoadImages(void) {
    // Menu icons
    cString Path = cString::sprintf("%s%s/menuIcons", *Config.IconPath, Setup.OSDTheme);
    std::string File("");
    cString FileName("");
    cReadDir d(*Path);
    struct dirent *e;
    while ((e = d.Next()) != NULL) {
        File = e->d_name;
        FileName = cString::sprintf("menuIcons/%s", File.substr(0, File.find_last_of(".")).c_str());
        ImgLoader.LoadIcon(*FileName, m_FontHeight - m_MarginItem * 2, m_FontHeight - m_MarginItem * 2);
    }

    ImgLoader.LoadIcon("menuIcons/blank", m_FontHeight - m_MarginItem * 2, m_FontHeight - m_MarginItem * 2);

    int ImageHeight = m_FontHeight;
    int ImageBgHeight = ImageHeight;
    int ImageBgWidth = ImageHeight * 1.34;
    cImage *img = ImgLoader.LoadIcon("logo_background", ImageBgWidth, ImageBgHeight);
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

    //* Double code? Already in 'displaychannel.c' 'PreLoadImages()'
    /* int index {0};
    LOCK_CHANNELS_READ;
    for (const cChannel *Channel = Channels->First(); Channel && index < LOGO_PRE_CACHE;
         Channel = Channels->Next(Channel)) {
        img = ImgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
        if (img)
            ++index;
    } */

    ImgLoader.LoadIcon("radio", 999, m_TopBarHeight - m_MarginItem * 2);
    ImgLoader.LoadIcon("changroup", 999, m_TopBarHeight - m_MarginItem * 2);
    ImgLoader.LoadIcon("tv", 999, m_TopBarHeight - m_MarginItem * 2);

    ImgLoader.LoadIcon("timer_full", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("timer_full_cur", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("timer_partial", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("vps", ImageHeight, ImageHeight);
    ImgLoader.LoadIcon("vps_cur", ImageHeight, ImageHeight);

    ImgLoader.LoadIcon("sd", m_FontHeight, m_FontHeight * (1.0 / 3.0));
    ImgLoader.LoadIcon("hd", m_FontHeight, m_FontHeight * (1.0 / 3.0));
    ImgLoader.LoadIcon("uhd", m_FontHeight, m_FontHeight * (1.0 / 3.0));

    ImgLoader.LoadIcon("recording_new", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("recording_new", m_FontSmlHeight, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_new_cur", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("recording_new_cur", m_FontSmlHeight, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_cutted", m_FontHeight, m_FontHeight * (1.0 / 3.0 * 2.0));
    ImgLoader.LoadIcon("recording_cutted_cur", m_FontHeight, m_FontHeight * (1.0 / 3.0 * 2.0));
    ImgLoader.LoadIcon("recording", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("folder", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("recording_old", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("recording_old_cur", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("recording_old", m_FontSmlHeight, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_old_cur", m_FontSmlHeight, m_FontSmlHeight);
}
