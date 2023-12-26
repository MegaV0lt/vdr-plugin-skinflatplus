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

#include "./baserender.h"
#include "./complexcontent.h"

#if APIVERSNUM < 20107
    enum eMenuSortMode {
        msmUnknown = 0,
        msmNumber,
        msmName,
        msmTime,
        msmProvider
    };
#endif

class cFlatDisplayMenu : public cFlatBaseRender, public cSkinDisplayMenu {
 private:
        cPixmap *MenuPixmap;
        cPixmap *MenuIconsPixmap;
        cPixmap *MenuIconsBgPixmap;
        cPixmap *MenuIconsOvlPixmap;

        int m_MenuTop, m_MenuWidth;
        int m_MenuItemWidth;
        int m_MenuItemLastHeight;
        bool m_MenuFullOsdIsDrawn;

        eMenuCategory m_MenuCategory;
        // int m_VideoDiskUsageState;  // Also in cFlatBaseRender

        int m_LastTimerCount, m_LastTimerActiveCount;
        cString m_LastTitle;

        int m_chLeft, m_chTop, m_chWidth, m_chHeight;
        cPixmap *ContentHeadPixmap;
        cPixmap *ContentHeadIconsPixmap;

        int m_cLeft, m_cTop, m_cWidth, m_cHeight;

        cPixmap *ScrollbarPixmap;
        int m_ScrollBarTop/*, m_ScrollBarWidth, m_ScrollBarHeight*/;  // Also in cFlatBaseRender

        int m_ItemHeight, m_ItemChannelHeight, m_ItemTimerHeight, m_ItemEventHeight, m_ItemRecordingHeight;

        std::list<sDecorBorder> ItemsBorder;
        sDecorBorder EventBorder, RecordingBorder, TextBorder;

        bool m_IsScrolling;
        bool m_IsGroup;
        bool m_ShowEvent, m_ShowRecording, m_ShowText;

        cComplexContent ComplexContent;

        // Content for Widgets
        cComplexContent ContentWidget;

        // TextScroller
        cTextScrollers MenuItemScroller;

        cString m_ItemEventLastChannelName;

        std::string m_RecFolder, m_LastRecFolder;
        int m_LastItemRecordingLevel;

        // Icons
        cImage *IconTimerFull;
        // cImage *IconTimerPartial;
        cImage *IconArrowTurn;
        cImage *IconRec;
        // cImage *iconVps;
        // cImage *iconNew;
        // Icons

        void ItemBorderInsertUnique(sDecorBorder ib);
        void ItemBorderDrawAllWithScrollbar(void);
        void ItemBorderDrawAllWithoutScrollbar(void);
        void ItemBorderClear(void);

        static std::string items[16];
        std::string MainMenuText(std::string Text);
        cString GetIconName(std::string element);

        std::string GetRecordingName(const cRecording *Recording, int Level, bool IsFolder);
        // std::string XmlSubstring(std::string source, const char* StrStart, const char* StrEnd);  // Moved to flat.h

        bool IsRecordingOld(const cRecording *Recording, int Level);
        const char *GetGenreIcon(uchar genre);

        time_t GetLastRecTimeFromFolder(const cRecording *Recording, int Level);

        void DrawScrollbar(int Total, int Offset, int Shown, int Top, int Height, bool CanScrollUp,
                           bool CanScrollDown, bool IsContent = false);
        int ItemsHeight(void);
        bool CheckProgressBar(const char *text);
        void DrawProgressBarFromText(cRect rec, cRect recBg, const char *bar, tColor ColorFg,
                                     tColor ColorBarFg, tColor ColorBg);

        static cBitmap bmCNew, bmCRec, bmCArrowTurn, bmCHD, bmCVPS;
        void DrawItemExtraEvent(const cEvent *Event, cString EmptyText);
        void DrawItemExtraRecording(const cRecording *Recording, cString EmptyText);
        void DrawMainMenuWidgets(void);
        int DrawMainMenuWidgetDVBDevices(int wLeft, int wWidth, int ContentTop);
        int DrawMainMenuWidgetActiveTimers(int wLeft, int wWidth, int ContentTop);
        int DrawMainMenuWidgetLastRecordings(int wLeft, int wWidth, int ContentTop);
        int DrawMainMenuWidgetTimerConflicts(int wLeft, int wWidth, int ContentTop);
        int DrawMainMenuWidgetSystemInformation(int wLeft, int wWidth, int ContentTop);
        int DrawMainMenuWidgetSystemUpdates(int wLeft, int wWidth, int ContentTop);
        int DrawMainMenuWidgetTemperaturs(int wLeft, int wWidth, int ContentTop);
        int DrawMainMenuWidgetCommand(int wLeft, int wWidth, int ContentTop);
        int DrawMainMenuWidgetWeather(int wLeft, int wWidth, int ContentTop);

 public:
#ifdef DEPRECATED_SKIN_SETITEMEVENT
    using cSkinDisplayMenu::SetItemEvent;
#endif
        cFlatDisplayMenu(void);
        virtual ~cFlatDisplayMenu();
        virtual void Scroll(bool Up, bool Page);
        virtual int MaxItems(void);
        virtual void Clear(void);

        virtual void SetMenuCategory(eMenuCategory MenuCategory);
        // virtual void SetTabs(int Tab1, int Tab2 = 0, int Tab3 = 0, int Tab4 = 0, int Tab5 = 0);

        virtual void SetTitle(const char *Title);
        virtual void SetButtons(const char *Red, const char *Green = NULL,
                                const char *Yellow = NULL, const char *Blue = NULL);
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void SetItem(const char *Text, int Index, bool Current, bool Selectable);

        #if APIVERSNUM >= 20308
                virtual bool SetItemEvent(const cEvent *Event, int Index, bool Current, bool Selectable,
                                          const cChannel *Channel, bool WithDate, eTimerMatch TimerMatch,
                                          bool TimerActive);
        #else
                virtual bool SetItemEvent(const cEvent *Event, int Index, bool Current, bool Selectable,
                                          const cChannel *Channel, bool WithDate, eTimerMatch TimerMatch);
        #endif
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
        virtual int GetTextAreaWidth(void) const;
        virtual const cFont *GetTextAreaFont(bool FixedFont) const;
        virtual void Flush(void);

        void PreLoadImages(void);
};
