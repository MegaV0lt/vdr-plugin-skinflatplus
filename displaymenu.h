/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/menu.h>
#include <vdr/tools.h>

#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

#include "./baserender.h"
#include "./complexcontent.h"

class cFlatDisplayMenu : public cFlatBaseRender, public cSkinDisplayMenu {
 public:
#ifdef DEPRECATED_SKIN_SETITEMEVENT
    using cSkinDisplayMenu::SetItemEvent;
#endif
    cFlatDisplayMenu();
    ~cFlatDisplayMenu() override;
    void Scroll(bool Up, bool Page) override;
    int MaxItems() override;
    void Clear() override;

    void SetMenuCategory(eMenuCategory MenuCategory) override;
    // void SetTabs(int Tab1, int Tab2 = 0, int Tab3 = 0, int Tab4 = 0, int Tab5 = 0) override;

    void SetTitle(const char *Title) override;
    void SetButtons(const char *Red, const char *Green = nullptr, const char *Yellow = nullptr,
                    const char *Blue = nullptr) override;
    void SetMessage(eMessageType Type, const char *Text) override;
    void SetItem(const char *Text, int Index, bool Current, bool Selectable) override;

    bool SetItemEvent(const cEvent *Event, int Index, bool Current, bool Selectable, const cChannel *Channel,
                      bool WithDate, eTimerMatch TimerMatch, bool TimerActive) override;
    bool SetItemTimer(const cTimer *Timer, int Index, bool Current, bool Selectable) override;
    bool SetItemChannel(const cChannel *Channel, int Index, bool Current, bool Selectable, bool WithProvider) override;
    bool SetItemRecording(const cRecording *Recording, int Index, bool Current, bool Selectable, int Level, int Total,
                          int New) override;

    void SetMenuSortMode(eMenuSortMode MenuSortMode) override;

    void SetScrollbar(int Total, int Offset) override;
    void SetEvent(const cEvent *Event) override;
    void SetRecording(const cRecording *Recording) override;
    void SetText(const char *Text, bool FixedFont) override;
    int GetTextAreaWidth() const override;
    const cFont *GetTextAreaFont(bool FixedFont) const override;
    void Flush() override;

    void PreLoadImages();

 private:
    cPixmap *MenuPixmap {nullptr};
    cPixmap *MenuIconsPixmap {nullptr};
    cPixmap *MenuIconsBgPixmap {nullptr};   // Background for icons/logos
    cPixmap *MenuIconsOvlPixmap {nullptr};  // Overlay for icons/logos

    int m_MenuTop {0}, m_MenuWidth {0};
    int m_MenuItemWidth {0};
    int m_MenuItemLastHeight {0};
    bool m_MenuFullOsdIsDrawn {false};

    eMenuCategory m_MenuCategory {mcUndefined};

    uint m_LastTimerActiveCount {0}, m_LastTimerCount {0};
    cString m_LastTitle {""};

    int m_chLeft {0}, m_chTop {0}, m_chWidth {0}, m_chHeight {0};
    cPixmap *ContentHeadPixmap {nullptr};
    cPixmap *ContentHeadIconsPixmap {nullptr};

    int m_cLeft {0}, m_cTop {0}, m_cWidth {0}, m_cHeight {0};

    cPixmap *ScrollbarPixmap {nullptr};
    int m_ScrollBarTop {0}, m_ScrollBarHeight {0};;
    int m_WidthScrollBar {0};  //* 'm_WidthScrollBar' to avoid nameconflict with cFlatBaseRender::m_ScrollBarWidth

    int m_ItemHeight {0}, m_ItemChannelHeight {0}, m_ItemTimerHeight {0};
    int m_ItemEventHeight {0}, m_ItemRecordingHeight {0};

    std::vector<sDecorBorder> ItemsBorder;
    sDecorBorder EventBorder, RecordingBorder, TextBorder;

    bool m_IsScrolling {false};
    // bool m_IsGroup {false};
    bool m_ShowEvent {false};
    bool m_ShowRecording {false};
    bool m_ShowText {false};

    cComplexContent ComplexContent;

    // Content for Widgets
    cComplexContent ContentWidget;

    // TextScroller
    cTextScrollers MenuItemScroller;

    cString m_ItemEventLastChannelName {""};

    std::string m_RecFolder {""}, m_LastRecFolder {""};
    int m_LastItemRecordingLevel {0};

    void ItemBorderInsertUnique(const sDecorBorder &ib);
    void ItemBorderDrawAllWithScrollbar();
    void ItemBorderDrawAllWithoutScrollbar();
    void ItemBorderClear();

    const cString items[16] {"Schedule", "Channels",      "Timers",  "Recordings", "Setup", "Commands",
                             "OSD",      "EPG",           "DVB",     "LNB",        "CAM",   "Recording",
                             "Replay",   "Miscellaneous", "Plugins", "Restart"};

    cString MainMenuText(const cString &Text) const;
    cString GetIconName(const std::string &element) const;
    cString GetMenuIconName() const;

    cString GetRecordingName(const cRecording *Recording, int Level, bool IsFolder) const;
    cString GetRecCounts();  // Get number of recordings and new recordings (35*/53)

    void UpdateTimerCounts(uint &TimerActiveCount, uint &TimerCount) const;  // NOLINT

    bool IsRecordingOld(const cRecording *Recording, int Level) const;

    const char *GetGenreIcon(uchar genre) const;
    void InsertGenreInfo(const cEvent *Event, cString &Text) const;  // NOLINT
    void InsertGenreInfo(const cEvent *Event, cString &Text, std::vector<std::string> &GenreIcons) const;  // NOLINT
    void InsertTSErrors(const cRecordingInfo *RecInfo, cString &Text);  // NOLINT

    void AddExtraInfo(const char *Title, const cString &Text, cComplexContent &ComplexContent, int &ContentTop,  // NOLINT
                      bool IsEvent = false);  // NOLINT

    time_t GetLastRecTimeFromFolder(const cRecording *Recording, int Level) const;

    void DrawScrollbar(int Total, int Offset, int Shown, int Top, int Height, bool CanScrollUp,
                       bool CanScrollDown, bool IsContent = false);
    int ItemsHeight();
    bool CheckProgressBar(const char *text) const;
    void DrawProgressBarFromText(const cRect &rec, const cRect &recBg, const char *bar, tColor ColorFg,
                                 tColor ColorBarFg, tColor ColorBg);

    void DrawItemExtraEvent(const cEvent *Event, const cString EmptyText);
    void DrawItemExtraRecording(const cRecording *Recording, const cString EmptyText);
    void AddActors(cComplexContent &ComplexContent, std::vector<cString> &ActorsPath,   // NOLINT
                   std::vector<cString> &ActorsName, std::vector<cString> &ActorsRole,  // NOLINT
                   int NumActors, bool IsEvent = false);  // Add Actors to complexcontent

    void DrawRecordingStateIcon(const cRecording *Recording, int Left, int Top, bool Current);
    void DrawRecordingFormatIcon(const cRecording *Recording, int Left, int Top);
    void DrawRecordingErrorIcon(const cRecording *Recording, int Left, int Top, bool Current);
    void DrawRecordingIcon(const char *IconName, int &Left, int Top, bool Current);  // NOLINT
    void DrawContentHeadFskGenre(int IconHeight, int &HeadIconLeft, int HeadIconTop, const cString &Fsk,  // NOLINT
                                 std::vector<std::string> &GenreIcons);  // NOLINT

    void DrawMainMenuWidgets();
    int DrawMainMenuWidgetDVBDevices(int wLeft, int wWidth, int ContentTop);
    int DrawMainMenuWidgetActiveTimers(int wLeft, int wWidth, int ContentTop);
    int DrawMainMenuWidgetLastRecordings(int wLeft, int wWidth, int ContentTop);
    int DrawMainMenuWidgetTimerConflicts(int wLeft, int wWidth, int ContentTop);
    int DrawMainMenuWidgetSystemInformation(int wLeft, int wWidth, int ContentTop);
    int DrawMainMenuWidgetSystemUpdates(int wLeft, int wWidth, int ContentTop);
    int DrawMainMenuWidgetTemperatures(int wLeft, int wWidth, int ContentTop);
    int DrawMainMenuWidgetCommand(int wLeft, int wWidth, int ContentTop);
    int DrawMainMenuWidgetWeather(int wLeft, int wWidth, int ContentTop);

    // Helper functions
    // Add Text to ComplexContent in SetText()
    void ComplexContentAddText(const char* Text, bool FixedFont, int Width, int Height) {
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
    }
    int AddWidgetHeader(const char *Icon, const char *Title, int ContentTop, int wWidth);
};
