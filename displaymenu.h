/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <ctype.h>
#include <vdr/menu.h>
#include <vdr/tools.h>

#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

#include "./services/scraper2vdr.h"

#include "./baserender.h"
#include "./complexcontent.h"

class cFlatDisplayMenu : public cFlatBaseRender, public cSkinDisplayMenu {
 public:
#ifdef DEPRECATED_SKIN_SETITEMEVENT
    using cSkinDisplayMenu::SetItemEvent;
#endif
        cFlatDisplayMenu();
        virtual ~cFlatDisplayMenu();
        virtual void Scroll(bool Up, bool Page);
        virtual int MaxItems();
        virtual void Clear();

        virtual void SetMenuCategory(eMenuCategory MenuCategory);
        // virtual void SetTabs(int Tab1, int Tab2 = 0, int Tab3 = 0, int Tab4 = 0, int Tab5 = 0);

        virtual void SetTitle(const char *Title);
        virtual void SetButtons(const char *Red, const char *Green = nullptr,
                                const char *Yellow = nullptr, const char *Blue = nullptr);
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void SetItem(const char *Text, int Index, bool Current, bool Selectable);

        virtual bool SetItemEvent(const cEvent *Event, int Index, bool Current, bool Selectable,
                                  const cChannel *Channel, bool WithDate, eTimerMatch TimerMatch,
                                  bool TimerActive);
        virtual bool SetItemTimer(const cTimer *Timer, int Index, bool Current, bool Selectable);
        virtual bool SetItemChannel(const cChannel *Channel, int Index, bool Current, bool Selectable,
                                    bool WithProvider);
        virtual bool SetItemRecording(const cRecording *Recording, int Index, bool Current, bool Selectable,
                                      int Level, int Total, int New);

        virtual void SetMenuSortMode(eMenuSortMode MenuSortMode);

        virtual void SetScrollbar(int Total, int Offset);
        virtual void SetEvent(const cEvent *Event);
        virtual void SetRecording(const cRecording *Recording);
        virtual void SetText(const char *Text, bool FixedFont);
        virtual int GetTextAreaWidth() const;
        virtual const cFont *GetTextAreaFont(bool FixedFont) const;
        virtual void Flush();

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

        int m_FontAscender {0};  // Top of capital letter
        // int m_VideoDiskUsageState;  // Also in cFlatBaseRender

        uint m_LastTimerCount {0}, m_LastTimerActiveCount {0};
        cString m_LastTitle {""};

        int m_chLeft {0}, m_chTop {0}, m_chWidth {0}, m_chHeight {0};
        cPixmap *ContentHeadPixmap {nullptr};
        cPixmap *ContentHeadIconsPixmap {nullptr};

        int m_cLeft {0}, m_cTop {0}, m_cWidth {0}, m_cHeight {0};

        cPixmap *ScrollbarPixmap {nullptr};
        int m_ScrollBarTop {0};
        int m_ScrollBarWidth {0}, m_ScrollBarHeight {0};  //? 'm_ScrollBarWidth' also in cFlatBaseRender

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

        // Icons
        cImage *IconTimerFull {nullptr};
        cImage *IconArrowTurn {nullptr};
        cImage *IconRec {nullptr};

        void ItemBorderInsertUnique(const sDecorBorder &ib);
        void ItemBorderDrawAllWithScrollbar();
        void ItemBorderDrawAllWithoutScrollbar();
        void ItemBorderClear();

        const cString items[16]{"Schedule", "Channels",      "Timers",  "Recordings", "Setup", "Commands",
                                "OSD",      "EPG",           "DVB",     "LNB",        "CAM",   "Recording",
                                "Replay",   "Miscellaneous", "Plugins", "Restart"};
        cString MainMenuText(const cString &Text);
        cString GetIconName(const std::string &element);

        cString GetRecordingName(const cRecording *Recording, int Level, bool IsFolder);
        cString GetRecCounts();  // Get number of recordings and new recordings (35*/53)

        bool IsRecordingOld(const cRecording *Recording, int Level);

        const char *GetGenreIcon(uchar genre);
        void InsertGenreInfo(const cEvent *Event, cString &Text);  // NOLINT
        void InsertGenreInfo(const cEvent *Event, cString &Text, std::vector<std::string> &GenreIcons);  // NOLINT

        void InsertSeriesInfos(const cSeries &Series, cString &SeriesInfo);  // NOLINT
        void InsertMovieInfos(const cMovie &Movie, cString &MovieInfo);      // NOLINT

        time_t GetLastRecTimeFromFolder(const cRecording *Recording, int Level);

        void DrawScrollbar(int Total, int Offset, int Shown, int Top, int Height, bool CanScrollUp,
                           bool CanScrollDown, bool IsContent = false);
        int ItemsHeight();
        bool CheckProgressBar(const char *text);
        void DrawProgressBarFromText(cRect rec, cRect recBg, const char *bar, tColor ColorFg,
                                     tColor ColorBarFg, tColor ColorBg);

        // static cBitmap bmCNew, bmCRec, bmCArrowTurn, bmCHD, bmCVPS;  // Unused?
        void DrawItemExtraEvent(const cEvent *Event, const cString EmptyText);
        void DrawItemExtraRecording(const cRecording *Recording, const cString EmptyText);
        void AddActors(cComplexContent &ComplexContent, std::vector<cString> &ActorsPath,   // NOLINT
                       std::vector<cString> &ActorsName, std::vector<cString> &ActorsRole,  // NOLINT
                       int NumActors);  // Add Actors to complexcontent

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
};
