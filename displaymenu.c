#include "displaymenu.h"
#include "services/epgsearch.h"
#include "services/remotetimers.h"
#include "services/scraper2vdr.h"
#include <fstream>
#include <iostream>
#include <utility>

#if VDRVERSNUM >= 20301
#include <future>
#endif

#ifndef VDRLOGO
#define VDRLOGO "vdrlogo_default"
#endif

#include "flat.h"
#include "locale"

/* Possible values of the stream content descriptor according to ETSI EN 300 468 */
enum stream_content {
  sc_reserved = 0x00,
  sc_video_MPEG2 = 0x01,
  sc_audio_MP2 = 0x02, // MPEG 1 Layer 2 audio
  sc_subtitle = 0x03,
  sc_audio_AC3 = 0x04,
  sc_video_H264_AVC = 0x05,
  sc_audio_HEAAC = 0x06,
  sc_video_H265_HEVC = 0x09, // stream content 0x09, extension 0x00
  sc_audio_AC4       = 0x19, // stream content 0x09, extension 0x10
};

static int CompareTimers(const void *a, const void *b) { return (*(const cTimer **)a)->Compare(**(const cTimer **)b); }

cFlatDisplayMenu::cFlatDisplayMenu(void) {
  CreateFullOsd();
  TopBarCreate();
  ButtonsCreate();
  MessageCreate();

  VideoDiskUsageState = -1;

  itemHeight = fontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
  itemChannelHeight = fontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
  itemTimerHeight = fontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;

  scrollBarWidth = ScrollBarWidth() + marginItem;
  scrollBarHeight = osdHeight - (topBarHeight + Config.decorBorderTopBarSize * 2 + marginItem * 3 + buttonsHeight +
                                 Config.decorBorderButtonSize * 2);

  scrollBarTop = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2;
  isScrolling = false;
  ShowEvent = false;
  ShowRecording = false;
  ShowText = false;

  ItemEventLastChannelName = "";
  RecFolder = "";
  LastRecFolder = "";

  menuItemLastHeight = 0;
  MenuFullOsdIsDrawn = false;

  menuWidth = osdWidth;
  menuTop = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize;
  menuPixmap = osd->CreatePixmap(1, cRect(0, menuTop, menuWidth, scrollBarHeight));
  // dsyslog("skinflatplus: menuPixmap left: %d top: %d width: %d height: %d", 0, menuTop, menuWidth, scrollBarHeight);

  menuIconsBGPixmap = osd->CreatePixmap(2, cRect(0, menuTop, menuWidth, scrollBarHeight));
  // dsyslog("skinflatplus: menuIconsBGPixmap left: %d top: %d width: %d height: %d", 0, menuTop, menuWidth, scrollBarHeight);
  menuIconsPixmap = osd->CreatePixmap(3, cRect(0, menuTop, menuWidth, scrollBarHeight));
  // dsyslog("skinflatplus: menuIconsPixmap left: %d top: %d width: %d height: %d", 0, menuTop, menuWidth, scrollBarHeight);
  menuIconsOVLPixmap = osd->CreatePixmap(4, cRect(0, menuTop, menuWidth, scrollBarHeight));
  // dsyslog("skinflatplus: menuIconsOVLPixmap left: %d top: %d width: %d height: %d", 0, menuTop, menuWidth, scrollBarHeight);

  chLeft = Config.decorBorderMenuContentHeadSize;
  chTop = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentHeadSize;
  chWidth = menuWidth - Config.decorBorderMenuContentHeadSize * 2;
  chHeight = fontHeight + fontSmlHeight * 2 + marginItem * 2;
  contentHeadPixmap = osd->CreatePixmap(1, cRect(chLeft, chTop, chWidth, chHeight));
  // dsyslog("skinflatplus: contentHeadPixmap left: %d top: %d width: %d height: %d", chLeft, chTop, chWidth, chHeight);
  contentHeadIconsPixmap = osd->CreatePixmap(2, cRect(chLeft, chTop, chWidth, chHeight));

  scrollbarPixmap = osd->CreatePixmap(
      2, cRect(0, scrollBarTop, menuWidth, scrollBarHeight + buttonsHeight + Config.decorBorderButtonSize * 2));
  // dsyslog("skinflatplus: scrollbarPixmap left: %d top: %d width: %d height: %d", 0, scrollBarTop, menuWidth,
  // scrollBarHeight + buttonsHeight + Config.decorBorderButtonSize*2 );

  menuPixmap->Fill(clrTransparent);
  menuIconsPixmap->Fill(clrTransparent);
  menuIconsBGPixmap->Fill(clrTransparent);
  menuIconsOVLPixmap->Fill(clrTransparent);
  scrollbarPixmap->Fill(clrTransparent);
  contentHeadIconsPixmap->Fill(clrTransparent);

  menuCategory = mcUndefined;

  menuItemScroller.SetOsd(osd);
  menuItemScroller.SetScrollStep(Config.ScrollerStep);
  menuItemScroller.SetScrollDelay(Config.ScrollerDelay);
  menuItemScroller.SetScrollType(Config.ScrollerType);
}

cFlatDisplayMenu::~cFlatDisplayMenu() {
  menuItemScroller.Clear();

  osd->DestroyPixmap(menuPixmap);
  osd->DestroyPixmap(menuIconsPixmap);
  osd->DestroyPixmap(menuIconsBGPixmap);
  osd->DestroyPixmap(menuIconsOVLPixmap);
  osd->DestroyPixmap(scrollbarPixmap);
  osd->DestroyPixmap(contentHeadPixmap);
  osd->DestroyPixmap(contentHeadIconsPixmap);
}

void cFlatDisplayMenu::SetMenuCategory(eMenuCategory MenuCategory) {
  menuItemScroller.Clear();

  ItemBorderClear();
  isScrolling = false;
  ShowRecording = ShowEvent = ShowText = false;

  menuCategory = MenuCategory;

  if (menuCategory == mcChannel) {
    if (Config.MenuChannelView == 0 || Config.MenuChannelView == 1 || Config.MenuChannelView == 2)
      itemChannelHeight = fontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
    else if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4)
      itemChannelHeight = fontHeight + fontSmlHeight + marginItem + Config.decorProgressMenuItemSize +
                          Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
  } else if (menuCategory == mcTimer) {
    if (Config.MenuTimerView == 0 || Config.MenuTimerView == 1)
      itemTimerHeight = fontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
    else if (Config.MenuTimerView == 2 || Config.MenuTimerView == 3)
      itemTimerHeight =
          fontHeight + fontSmlHeight + marginItem + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
  } else if (menuCategory == mcSchedule || menuCategory == mcScheduleNow || menuCategory == mcScheduleNext) {
    if (Config.MenuEventView == 0 || Config.MenuEventView == 1)
      itemEventHeight = fontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
    else if (Config.MenuEventView == 2 || Config.MenuEventView == 3)
      itemEventHeight = fontHeight + fontSmlHeight + marginItem * 2 + Config.MenuItemPadding +
                        Config.decorBorderMenuItemSize * 2 + Config.decorProgressMenuItemSize / 2;
  } else if (menuCategory == mcRecording) {
    if (Config.MenuRecordingView == 0 || Config.MenuRecordingView == 1)
      itemRecordingHeight = fontHeight + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
    else if (Config.MenuRecordingView == 2 || Config.MenuRecordingView == 3)
      itemRecordingHeight =
          fontHeight + fontSmlHeight + marginItem + Config.MenuItemPadding + Config.decorBorderMenuItemSize * 2;
  } else if (menuCategory == mcMain && Config.MainMenuWidgetsEnable) {
    DrawMainMenuWidgets();
  }
}

void cFlatDisplayMenu::SetScrollbar(int Total, int Offset) {
  DrawScrollbar(Total, Offset, MaxItems(), 0, ItemsHeight(), Offset > 0, Offset + MaxItems() < Total);
}

void cFlatDisplayMenu::DrawScrollbar(int Total, int Offset, int Shown, int Top, int Height, bool CanScrollUp,
                                     bool CanScrollDown, bool isContent) {
  // dsyslog("Total: %d Offset: %d Shown: %d Top: %d Height: %d", Total, Offset, Shown, Top, Height);

  if (Total > 0 && Total > Shown) {
    if (isScrolling == false && ShowEvent == false && ShowRecording == false && ShowText == false) {
      isScrolling = true;
      DecorBorderClearByFrom(BorderMenuItem);
      ItemBorderDrawAllWithScrollbar();
      ItemBorderClear();

      menuItemScroller.UpdateViewPortWidth(scrollBarWidth);

      if (isContent)
        menuPixmap->DrawRectangle(cRect(menuItemWidth - scrollBarWidth + Config.decorBorderMenuContentSize, 0,
                                        scrollBarWidth + marginItem, scrollBarHeight),
                                  clrTransparent);
      else
        menuPixmap->DrawRectangle(cRect(menuItemWidth - scrollBarWidth + Config.decorBorderMenuItemSize, 0,
                                        scrollBarWidth + marginItem, scrollBarHeight),
                                  clrTransparent);
    }
  } else if (ShowEvent == false && ShowRecording == false && ShowText == false) {
    isScrolling = false;
  }

  if (isContent)
    ScrollbarDraw(scrollbarPixmap, menuItemWidth - scrollBarWidth + Config.decorBorderMenuContentSize * 2 + marginItem,
                  Top, Height, Total, Offset, Shown, CanScrollUp, CanScrollDown);
  else
    ScrollbarDraw(scrollbarPixmap, menuItemWidth - scrollBarWidth + Config.decorBorderMenuItemSize * 2 + marginItem,
                  Top, Height, Total, Offset, Shown, CanScrollUp, CanScrollDown);
}

void cFlatDisplayMenu::Scroll(bool Up, bool Page) {
  // Wird das Menü gescrollt oder Content?
  if (ComplexContent.IsShown() && ComplexContent.IsScrollingActive() && ComplexContent.Scrollable()) {
    bool scrolled = ComplexContent.Scroll(Up, Page);
    if (scrolled) {
      DrawScrollbar(ComplexContent.ScrollTotal(), ComplexContent.ScrollOffset(), ComplexContent.ScrollShown(),
                    ComplexContent.Top() - scrollBarTop, ComplexContent.Height(), ComplexContent.ScrollOffset() > 0,
                    ComplexContent.ScrollOffset() + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);
    }
  } else {
    cSkinDisplayMenu::Scroll(Up, Page);
  }
}

int cFlatDisplayMenu::MaxItems(void) {
  if (menuCategory == mcChannel)
    return scrollBarHeight / itemChannelHeight;
  else if (menuCategory == mcTimer)
    return scrollBarHeight / itemTimerHeight;
  else if (menuCategory == mcSchedule || menuCategory == mcScheduleNow || menuCategory == mcScheduleNext)
    return scrollBarHeight / itemEventHeight;
  else if (menuCategory == mcRecording)
    return scrollBarHeight / itemRecordingHeight;

  return scrollBarHeight / itemHeight;
}

int cFlatDisplayMenu::ItemsHeight(void) {
  if (menuCategory == mcChannel)
    return MaxItems() * itemChannelHeight - Config.MenuItemPadding;
  else if (menuCategory == mcTimer)
    return MaxItems() * itemTimerHeight - Config.MenuItemPadding;
  else if (menuCategory == mcSchedule || menuCategory == mcScheduleNow || menuCategory == mcScheduleNext)
    return MaxItems() * itemEventHeight - Config.MenuItemPadding;
  else if (menuCategory == mcRecording)
    return MaxItems() * itemRecordingHeight - Config.MenuItemPadding;

  return MaxItems() * itemHeight - Config.MenuItemPadding;
}

void cFlatDisplayMenu::Clear(void) {
  menuItemScroller.Clear();
  menuPixmap->Fill(clrTransparent);
  menuIconsPixmap->Fill(clrTransparent);
  menuIconsBGPixmap->Fill(clrTransparent);
  menuIconsOVLPixmap->Fill(clrTransparent);
  scrollbarPixmap->Fill(clrTransparent);
  contentHeadPixmap->Fill(clrTransparent);
  contentHeadIconsPixmap->Fill(clrTransparent);
  DecorBorderClearByFrom(BorderMenuItem);
  DecorBorderClearByFrom(BorderContent);
  DecorBorderClearByFrom(BorderMMWidget);
  DecorBorderClearAll();
  isScrolling = false;

  menuItemLastHeight = 0;
  MenuFullOsdIsDrawn = false;

  ComplexContent.Clear();

  contentWidget.Clear();

  TopBarClearMenuIconRight();

  ShowRecording = ShowEvent = ShowText = false;
}

void cFlatDisplayMenu::SetTitle(const char *Title) {
  TopBarSetTitle(Title);
  LastTitle = Title;

  if (Config.TopBarMenuIconShow) {
    cString icon("");
    switch (menuCategory) {
    case mcMain:
      TopBarSetTitle("");
      icon = cString::sprintf("menuIcons/%s", VDRLOGO);
      break;
    case mcSchedule:
    case mcScheduleNow:
    case mcScheduleNext:
      icon = "menuIcons/Schedule";
      break;
    case mcChannel:
      icon = "menuIcons/Channels";
      if (Config.MenuChannelShowCount) {
        int chanCount = 0;
#if VDRVERSNUM >= 20301
        LOCK_CHANNELS_READ;
        for (const cChannel *Channel = Channels->First(); Channel; Channel = Channels->Next(Channel)) {
#else
        for (cChannel *Channel = Channels.First(); Channel; Channel = Channels.Next(Channel)) {
#endif
          if (!Channel->GroupSep())
            chanCount++;
        }
        cString newTitle = cString::sprintf("%s (%d)", Title, chanCount);
        TopBarSetTitle(*newTitle);
      }
      break;
    case mcTimer:
      icon = "menuIcons/Timers";
      if (Config.MenuTimerShowCount) {
        int timerCount = 0, timerActiveCount = 0;
#if VDRVERSNUM >= 20301
        LOCK_TIMERS_READ;
        for (const cTimer *Timer = Timers->First(); Timer; Timer = Timers->Next(Timer)) {
#else
        for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer)) {
#endif
          timerCount++;
          if (Timer->HasFlags(tfActive))
            timerActiveCount++;
        }
        LastTimerCount = timerCount;
        LastTimerActiveCount = timerActiveCount;
        cString newTitle = cString::sprintf("%s (%d/%d)", Title, timerActiveCount, timerCount);
        TopBarSetTitle(*newTitle);
      }
      break;
    case mcRecording:
      if (Config.MenuRecordingShowCount) {
        int recCount = 0, recNewCount = 0;
        LastRecFolder = RecFolder;
        if (RecFolder != "" && LastItemRecordingLevel > 0) {
#if VDRVERSNUM >= 20301
          LOCK_RECORDINGS_READ;
          for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
#else
          for (cRecording *Rec = Recordings.First(); Rec; Rec = Recordings.Next(Rec)) {
#endif
            std::string RecFolder2 = GetRecordingName(Rec, LastItemRecordingLevel - 1, true).c_str();
            if (RecFolder == RecFolder2) {
              recCount++;
              if (Rec->IsNew())
                recNewCount++;
            }
          }
        } else {
#if VDRVERSNUM >= 20301
          LOCK_RECORDINGS_READ;
          for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
#else
          for (cRecording *Rec = Recordings.First(); Rec; Rec = Recordings.Next(Rec)) {
#endif
            recCount++;
            if (Rec->IsNew())
              recNewCount++;
          }
        }
        cString newTitle = cString::sprintf("%s (%d*/%d)", Title, recNewCount, recCount);
        if (Config.ShortRecordingCount) {
          if (recNewCount == 0) // 0 neue Aufnahmen
            newTitle = cString::sprintf("%s (%d)", Title, recCount);
          else if (recNewCount == recCount) // Nur neue Aufnahmen
            newTitle = cString::sprintf("%s (%d*)", Title, recNewCount);
          else // Anzeige (35*/56)
            newTitle = cString::sprintf("%s (%d*/%d)", Title, recNewCount, recCount);
        }
        TopBarSetTitle(*newTitle);
      }
      /*
      if( RecordingsSortMode == rsmName )
          TopBarSetMenuIconRight("menuIcons/RecsSortName");
      else if( RecordingsSortMode == rsmTime )
          TopBarSetMenuIconRight("menuIcons/RecsSortDate");
      */
      icon = "menuIcons/Recordings";
      break;
    case mcSetup:
      icon = "menuIcons/Setup";
      break;
    case mcCommand:
      icon = "menuIcons/Commands";
      break;
    case mcEvent:
      icon = "extraIcons/Info";
      break;
    case mcRecordingInfo:
      icon = "extraIcons/PlayInfo";
      break;
    default:
      icon = "";
    }
    TopBarSetMenuIcon(icon);

    if ((menuCategory == mcRecording || menuCategory == mcTimer) && Config.DiskUsageShow == 1 ||
        Config.DiskUsageShow == 2 | Config.DiskUsageShow == 3) {
      TopBarEnableDiskUsage();
    }
  }
}

void cFlatDisplayMenu::SetButtons(const char *Red, const char *Green, const char *Yellow, const char *Blue) {
  ButtonsSet(Red, Green, Yellow, Blue);
}

void cFlatDisplayMenu::SetMessage(eMessageType Type, const char *Text) {
  if (Text)
    MessageSet(Type, Text);
  else
    MessageClear();
}

void cFlatDisplayMenu::SetItem(const char *Text, int Index, bool Current, bool Selectable) {
  ShowEvent = false;
  ShowRecording = false;
  ShowText = false;

  int y = Index * itemHeight;
  menuItemWidth = menuWidth - Config.decorBorderMenuItemSize * 2;

  if (menuCategory == mcMain && Config.MainMenuWidgetsEnable)
    menuItemWidth *= Config.MainMenuItemScale;

  int AvailableTextWidth = menuItemWidth - scrollBarWidth;
  if (isScrolling)
    menuItemWidth -= scrollBarWidth;

  tColor ColorFg, ColorBg, ColorExtraTextFg;
  ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextFont);
  if (Current) {
    ColorFg = Theme.Color(clrItemCurrentFont);
    ColorBg = Theme.Color(clrItemCurrentBg);
    ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextCurrentFont);

    iconTimerFull = imgLoader.LoadIcon("text_timer_full_cur", fontHeight, fontHeight);
    iconTimerPartial = imgLoader.LoadIcon("text_timer_partial_cur", fontHeight, fontHeight);
    iconArrowTurn = imgLoader.LoadIcon("text_arrowturn_cur", fontHeight, fontHeight);
    iconRec = imgLoader.LoadIcon("text_rec_cur", fontHeight, fontHeight);
    iconVps = imgLoader.LoadIcon("text_vps_cur", fontHeight, fontHeight);
    iconNew = imgLoader.LoadIcon("text_new_cur", fontHeight, fontHeight);
  } else {
    if (Selectable) {
      ColorFg = Theme.Color(clrItemSelableFont);
      ColorBg = Theme.Color(clrItemSelableBg);

      iconTimerFull = imgLoader.LoadIcon("text_timer_full_sel", fontHeight, fontHeight);
      iconTimerPartial = imgLoader.LoadIcon("text_timer_partial_sel", fontHeight, fontHeight);
      iconArrowTurn = imgLoader.LoadIcon("text_arrowturn_sel", fontHeight, fontHeight);
      iconRec = imgLoader.LoadIcon("text_rec_sel", fontHeight, fontHeight);
      iconVps = imgLoader.LoadIcon("text_vps_sel", fontHeight, fontHeight);
      iconNew = imgLoader.LoadIcon("text_new_sel", fontHeight, fontHeight);
    } else {
      ColorFg = Theme.Color(clrItemFont);
      ColorBg = Theme.Color(clrItemBg);

      iconTimerFull = imgLoader.LoadIcon("text_timer_full", fontHeight, fontHeight);
      iconTimerPartial = imgLoader.LoadIcon("text_timer_partial", fontHeight, fontHeight);
      iconArrowTurn = imgLoader.LoadIcon("text_arrowturn", fontHeight, fontHeight);
      iconRec = imgLoader.LoadIcon("text_rec", fontHeight, fontHeight);
      iconVps = imgLoader.LoadIcon("text_vps", fontHeight, fontHeight);
      iconNew = imgLoader.LoadIcon("text_new", fontHeight, fontHeight);
    }
  }

  if (y + itemHeight > menuItemLastHeight)
    menuItemLastHeight = y + itemHeight;

  menuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, menuItemWidth, fontHeight), ColorBg);
  int lh = fontHeight;
  int xOff = 0;
  for (int i = 0; i < MaxTabs; i++) {
    const char *s = GetTabbedText(Text, i);
    if (s) {
      // from skinelchi
      char buffer[9];
      bool istimer = false;
      bool isnewrecording = false;
      bool hasEventtimer = false;
      bool haspartEventtimer = false;
      bool isRecording = false;
      bool hasVPS = false;
      bool isRunning = false;

      int xt = Tab(i);

      if (true) {
        // check if timer info symbols: " !#>"
        if (i == 0 && strlen(s) == 1 && strchr(" !#>", s[0])) {
          istimer = true; // update status
        } else if ((strlen(s) == 6 && s[5] == '*' && s[2] == ':' && isdigit(*s) && isdigit(*(s + 1)) &&
                    isdigit(*(s + 3)) && isdigit(*(s + 4))) ||
                   (strlen(s) == 5 && s[4] == '*' && s[1] == ':' && isdigit(*s) && isdigit(*(s + 2)) &&
                    isdigit(*(s + 3))) ||
                   (strlen(s) == 9 && s[8] == '*' && s[5] == '.' && s[2] == '.' && isdigit(*s) && isdigit(*(s + 1)) &&
                    isdigit(*(s + 3)) && isdigit(*(s + 4)) && isdigit(*(s + 6)) && isdigit(*(s + 7)))) {
          // check if new recording: "10:10*", "1:10*", "01.01.06*"

          isnewrecording = true;         // update status
          strncpy(buffer, s, strlen(s)); // make a copy
          buffer[strlen(s) - 1] = '\0';  // remove the '*' character
        } else if ((strlen(s) == 3) && (i == 2 || i == 3 || i == 4)) {
          if (s[0] == 'R')
            isRecording = true;
          if (s[0] == 'T')
            hasEventtimer = true;
          if (s[0] == 't')
            haspartEventtimer = true;
          if (s[1] == 'V')
            hasVPS = true;
          if (s[2] == '*')
            isRunning = true;
        } else if ((strlen(s) == 4) && (i == 3)) { // epgsearch What's on now default
          if (s[1] == 'R')
            isRecording = true;
          if (s[1] == 'T')
            hasEventtimer = true;
          if (s[1] == 't')
            haspartEventtimer = true;
          if (s[2] == 'V')
            hasVPS = true;
          if (s[3] == '*')
            isRunning = true;
        }
      }
      xOff = Tab(i) + Config.decorBorderMenuItemSize;

      if (istimer) {
        // timer menu
        switch (s[0]) {
        case '!':
          if (iconArrowTurn)
            menuIconsPixmap->DrawImage(cPoint(xOff, y + (lh - iconArrowTurn->Height()) / 2), *iconArrowTurn);
          break;
        case '#':
          if (iconRec)
            menuIconsPixmap->DrawImage(cPoint(xOff, y + (lh - iconRec->Height()) / 2), *iconRec);
          break;
        case '>':
          if (iconTimerFull)
            menuIconsPixmap->DrawImage(cPoint(xOff, y + (lh - iconTimerFull->Height()) / 2), *iconTimerFull);
          break;
        case ' ':
        default:
          break;
        }
      } else if (isRecording || hasEventtimer || haspartEventtimer || hasVPS || isRunning) {
        // program schedule menu
        if (isRecording && iconRec)
          menuIconsPixmap->DrawImage(cPoint(xOff, y + (lh - iconRec->Height()) / 2), *iconRec);
        else {
          if (hasEventtimer && iconTimerFull)
            menuIconsPixmap->DrawImage(cPoint(xOff, y + (lh - iconTimerFull->Height()) / 2), *iconTimerFull);
          if (haspartEventtimer && iconTimerPartial)
            menuIconsPixmap->DrawImage(cPoint(xOff, y + (lh - iconTimerPartial->Height()) / 2), *iconTimerPartial);
        }
        xOff += iconTimerFull->Width(); // clock is wider than rec

        if (hasVPS && iconVps) {
          menuIconsPixmap->DrawImage(cPoint(xOff, y + (lh - iconVps->Height()) / 2), *iconVps);
          xOff += iconVps->Width();
        }

        if (isRunning)
          menuPixmap->DrawText(cPoint(xOff, y), "*", ColorFg, ColorBg, font, AvailableTextWidth - xOff);

      } else if (isnewrecording) {
        // recordings menu
        menuPixmap->DrawText(cPoint(xOff, y), buffer, ColorFg, ColorBg, font, AvailableTextWidth - xOff);

        // draw symbol "new" centered
        if (iconNew) {
          int gap = std::max(0, (Tab(i + 1) - Tab(i) - font->Width(buffer) - iconNew->Width()) / 2);
          menuIconsPixmap->DrawImage(cPoint(xOff + font->Width(buffer) + gap, y + (lh - iconNew->Height()) / 2),
                                     *iconNew);
        }
      } else if (CheckProgressBar(s)) {
        int colWidth = Tab(i + 1) - Tab(i);

        tColor ColorFg = Config.decorProgressMenuItemFg;
        tColor ColorBarFg = Config.decorProgressMenuItemBarFg;
        tColor ColorBg = Config.decorProgressMenuItemBg;
        if (Current) {
          ColorFg = Config.decorProgressMenuItemCurFg;
          ColorBarFg = Config.decorProgressMenuItemCurBarFg;
          ColorBg = Config.decorProgressMenuItemCurBg;
        }
        cRect rec = cRect(xt + Config.decorBorderMenuItemSize,
                          y + (itemHeight - Config.MenuItemPadding) / 2 - Config.decorProgressMenuItemSize / 2 -
                              Config.decorBorderMenuItemSize,
                          colWidth, Config.decorProgressMenuItemSize);
        cRect recBG = cRect(xt + Config.decorBorderMenuItemSize - marginItem, y, colWidth + marginItem * 2, fontHeight);

        DrawProgressBarFromText(rec, recBG, s, ColorFg, ColorBarFg, ColorBg);
      } else {
        if ((menuCategory == mcMain || menuCategory == mcSetup) && Config.MenuItemIconsShow) {
          cImageLoader imgLoader;
          cString cIcon = GetIconName(MainMenuText(s));
          cString cIconCur("");
          cImage *img = NULL;
          if (Current) {
            cIconCur = cString::sprintf("%s_cur", *cIcon);
            img = imgLoader.LoadIcon(*cIconCur, fontHeight - marginItem * 2, fontHeight - marginItem * 2);
          }
          if (img == NULL)
            img = imgLoader.LoadIcon(*cIcon, fontHeight - marginItem * 2, fontHeight - marginItem * 2);

          if (img) {
            menuIconsPixmap->DrawImage(cPoint(xt + Config.decorBorderMenuItemSize + marginItem, y + marginItem), *img);
          } else {
            img = imgLoader.LoadIcon("menuIcons/blank", fontHeight - marginItem * 2, fontHeight - marginItem * 2);
            if (img) {
              menuIconsPixmap->DrawImage(cPoint(xt + Config.decorBorderMenuItemSize + marginItem, y + marginItem),
                                         *img);
            }
          }
          menuPixmap->DrawText(cPoint(fontHeight + marginItem * 2 + xt + Config.decorBorderMenuItemSize, y), s, ColorFg,
                               ColorBg, font, AvailableTextWidth - xt - marginItem * 2 - fontHeight);
        } else {
          if (Config.MenuItemParseTilde) {
            std::string tilde = s;
            size_t found = tilde.find(" ~ ");
            size_t found2 = tilde.find("~");
            if (found != string::npos) {
              std::string first = tilde.substr(0, found);
              std::string second = tilde.substr(found + 2, tilde.length());

              menuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), first.c_str(), ColorFg, ColorBg,
                                   font, menuItemWidth - xt - Config.decorBorderMenuItemSize);
              int l = font->Width(first.c_str());
              menuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize + l, y), second.c_str(), ColorExtraTextFg,
                                   ColorBg, font, menuItemWidth - xt - Config.decorBorderMenuItemSize - l);
            } else if (found2 != string::npos) {
              std::string first = tilde.substr(0, found2);
              std::string second = tilde.substr(found2 + 1, tilde.length());

              menuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), first.c_str(), ColorFg, ColorBg,
                                   font, menuItemWidth - xt - Config.decorBorderMenuItemSize);
              int l = font->Width(first.c_str());
              l += font->Width("X");
              menuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize + l, y), second.c_str(), ColorExtraTextFg,
                                   ColorBg, font, menuItemWidth - xt - Config.decorBorderMenuItemSize - l);
            } else
              menuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), s, ColorFg, ColorBg, font,
                                   menuItemWidth - xt - Config.decorBorderMenuItemSize);
          } else
            menuPixmap->DrawText(cPoint(xt + Config.decorBorderMenuItemSize, y), s, ColorFg, ColorBg, font,
                                 menuItemWidth - xt - Config.decorBorderMenuItemSize);
        }
      }
    }
    if (!Tab(i + 1))
      break;
  }

  sDecorBorder ib;
  ib.Left = Config.decorBorderMenuItemSize;
  ib.Top = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y;

  ib.Width = menuWidth - Config.decorBorderMenuItemSize * 2;
  if (isScrolling)
    ib.Width -= scrollBarWidth;

  ib.Width = menuItemWidth;

  ib.Height = fontHeight;
  ib.Size = Config.decorBorderMenuItemSize;
  ib.Type = Config.decorBorderMenuItemType;

  if (Current) {
    ib.ColorFg = Config.decorBorderMenuItemCurFg;
    ib.ColorBg = Config.decorBorderMenuItemCurBg;
  } else {
    if (Selectable) {
      ib.ColorFg = Config.decorBorderMenuItemSelFg;
      ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
      ib.ColorFg = Config.decorBorderMenuItemFg;
      ib.ColorBg = Config.decorBorderMenuItemBg;
    }
  }

  DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);

  if (!isScrolling)
    ItemBorderInsertUnique(ib);

  SetEditableWidth(menuWidth - Tab(1));
}

std::string cFlatDisplayMenu::items[16] = {"Schedule", "Channels",      "Timers",  "Recordings", "Setup", "Commands",
                                           "OSD",      "EPG",           "DVB",     "LNB",        "CAM",   "Recording",
                                           "Replay",   "Miscellaneous", "Plugins", "Restart"};

std::string cFlatDisplayMenu::MainMenuText(std::string Text) {
  std::string text = skipspace(Text.c_str());
  std::string menuEntry("");
  std::string menuNumber("");
  bool found = false;
  bool doBreak = false;
  size_t i = 0;
  for (; i < text.length(); i++) {
    char s = text.at(i);
    if (i == 0) {
      // if text directly starts with nonnumeric, break
      if (!(s >= '0' && s <= '9'))
        break;
    }
    if (found) {
      if (!(s >= '0' && s <= '9'))
        doBreak = true;
    }
    if (s >= '0' && s <= '9')
      found = true;

    if (doBreak)
      break;
    if (i > 4)
      break;
  }
  if (found) {
    menuNumber = skipspace(text.substr(0, i).c_str());
    menuEntry = skipspace(text.substr(i).c_str());
  } else {
    menuNumber = "";
    menuEntry = text.c_str();
  }
  return menuEntry;
}

cString cFlatDisplayMenu::GetIconName(std::string element) {
  // check for standard menu entries
  for (int i = 0; i < 16; i++) {
    std::string s = trVDR(items[i].c_str());
    if (s == element) {
      cString menuIcon = cString::sprintf("menuIcons/%s", items[i].c_str());
      return menuIcon;
    }
  }
  // check for special main menu entries "stop recording", "stop replay"
  std::string stopRecording = skipspace(trVDR(" Stop recording "));
  std::string stopReplay = skipspace(trVDR(" Stop replaying"));
  try {
    if (element.substr(0, stopRecording.size()) == stopRecording)
      return "menuIcons/StopRecording";
    if (element.substr(0, stopReplay.size()) == stopReplay)
      return "menuIcons/StopReplay";
  } catch (...) {
  }
  // check for Plugins
  for (int i = 0;; i++) {
    cPlugin *p = cPluginManager::GetPlugin(i);
    if (p) {
      const char *mainMenuEntry = p->MainMenuEntry();
      if (mainMenuEntry) {
        std::string plugMainEntry = mainMenuEntry;
        try {
          if (element.substr(0, plugMainEntry.size()) == plugMainEntry) {
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
  if (strlen(text) > 5 && text[0] == '[' && ((text[1] == '|') || (text[1] == ' ')) &&
      ((text[2] == '|') || (text[2] == ' ')) && text[strlen(text) - 1] == ']')
    return true;
  return false;
}

void cFlatDisplayMenu::DrawProgressBarFromText(cRect rec, cRect recBg, const char *bar, tColor ColorFg,
                                               tColor ColorBarFg, tColor ColorBg) {
  const char *p = bar + 1;
  bool isProgressbar = true;
  int total = 0;
  int now = 0;
  for (; *p != ']'; ++p) {
    if (*p == ' ' || *p == '|') {
      ++total;
      if (*p == '|')
        ++now;
    } else {
      isProgressbar = false;
      break;
    }
  }
  if (isProgressbar) {
    double progress = (double)now / (double)total;
    ProgressBarDrawRaw(menuPixmap, menuPixmap, rec, recBg, progress * total, total, ColorFg, ColorBarFg, ColorBg,
                       Config.decorProgressMenuItemType, true);
  }
}

bool cFlatDisplayMenu::SetItemChannel(const cChannel *Channel, int Index, bool Current, bool Selectable,
                                      bool WithProvider) {
  if (Config.MenuChannelView == 0 || !Channel)
    return false;

  const cEvent *Event = NULL;

  bool DrawProgress = true;
  cString buffer("");
  int y = Index * itemChannelHeight;

  if (Current)
    menuItemScroller.Clear();

  int Height = fontHeight;
  menuItemWidth = menuWidth - Config.decorBorderMenuItemSize * 2;
  if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4) {
    Height = fontHeight + fontSmlHeight + marginItem + Config.decorProgressMenuItemSize;
    menuItemWidth *= 0.5;
  }

  if (isScrolling)
    menuItemWidth -= scrollBarWidth;

  tColor ColorFg, ColorBg;
  if (Current) {
    ColorFg = Theme.Color(clrItemCurrentFont);
    ColorBg = Theme.Color(clrItemCurrentBg);
  } else {
    if (Selectable) {
      ColorFg = Theme.Color(clrItemSelableFont);
      ColorBg = Theme.Color(clrItemSelableBg);
    } else {
      ColorFg = Theme.Color(clrItemFont);
      ColorBg = Theme.Color(clrItemBg);
    }
  }

  if (y + itemChannelHeight > menuItemLastHeight)
    menuItemLastHeight = y + itemChannelHeight;

  menuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, menuItemWidth, Height), ColorBg);

  int Left = 0, Top = 0, Width = 0;
  int LeftName = 0;
  Left = Config.decorBorderMenuItemSize + marginItem;
  Top = y;

  if (Channel->GroupSep())
    DrawProgress = false;
  float progress = 0.0;
  cString EventTitle("");

#if VDRVERSNUM >= 20301
  LOCK_CHANNELS_READ;
  cString ws = cString::sprintf("%d", Channels->MaxNumber());
#else
  cString ws = cString::sprintf("%d", Channels.MaxNumber());
#endif
  int w = font->Width(ws);
  if (!Channel->GroupSep())
    buffer = cString::sprintf("%d", Channel->Number());
  else
    buffer = "";
  Width = font->Width(buffer);
  if (Width < w)
    Width = w;

  menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, font, Width, fontHeight, taRight);
  Left += Width + marginItem;

  int imageHeight = fontHeight;
  int imageLeft = Left;
  int imageTop = Top;
  int imageBGHeight = imageHeight;
  int imageBGWidth = imageHeight * 1.34;

  if (!Channel->GroupSep()) {
    cImage *imgBG = imgLoader.LoadIcon("logo_background", imageHeight * 1.34, imageHeight);
    if (imgBG) {
      imageBGHeight = imgBG->Height();
      imageBGWidth = imgBG->Width();
      imageTop = Top + (fontHeight - imgBG->Height()) / 2;
      menuIconsBGPixmap->DrawImage(cPoint(imageLeft, imageTop), *imgBG);
    }
  }
  cImage *img = imgLoader.LoadLogo(Channel->Name(), imageBGWidth - 4, imageBGHeight - 4);
  if (img) {
    imageTop = Top + (imageBGHeight - img->Height()) / 2;
    imageLeft = Left + (imageBGWidth - img->Width()) / 2;

    menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
    Left += imageBGWidth + marginItem * 2;
  } else {
    bool isRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;

    if (isRadioChannel) {
      if (Current)
        img = imgLoader.LoadIcon("radio_cur", imageBGWidth - 10, imageBGHeight - 10);
      if (img == NULL)
        img = imgLoader.LoadIcon("radio", imageBGWidth - 10, imageBGHeight - 10);

      if (img) {
        imageTop = Top + (imageBGHeight - img->Height()) / 2;
        imageLeft = Left + (imageBGWidth - img->Width()) / 2;
        menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
        Left += imageBGWidth + marginItem * 2;
      }
    } else if (Channel->GroupSep()) {
      img = imgLoader.LoadIcon("changroup", imageBGWidth - 10, imageBGHeight - 10);
      if (img) {
        imageTop = Top + (imageBGHeight - img->Height()) / 2;
        imageLeft = Left + (imageBGWidth - img->Width()) / 2;
        menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
        Left += imageBGWidth + marginItem * 2;
      }
    } else {
      if (Current)
        img = imgLoader.LoadIcon("tv_cur", imageBGWidth - 10, imageBGHeight - 10);
      if (img == NULL)
        img = imgLoader.LoadIcon("tv", imageBGWidth - 10, imageBGHeight - 10);
      if (img) {
        imageTop = Top + (imageBGHeight - img->Height()) / 2;
        imageLeft = Left + (imageBGWidth - img->Width()) / 2;
        menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
        Left += imageBGWidth + marginItem * 2;
      }
    }
  }

  LeftName = Left;

  // event from channel
#if VDRVERSNUM >= 20301
  LOCK_SCHEDULES_READ;
  const cSchedule *Schedule = Schedules->GetSchedule(Channel);
#else
  cSchedulesLock schedulesLock;
  const cSchedules *schedules = cSchedules::Schedules(schedulesLock);
  const cSchedule *Schedule = schedules->GetSchedule(Channel->GetChannelID());
#endif
  if (Schedule) {
    Event = Schedule->GetPresentEvent();
    if (Event) {
      // calculate progress bar
      progress = (int)roundf((float)(time(NULL) - Event->StartTime()) / (float)(Event->Duration()) * 100.0);
      if (progress < 0)
        progress = 0.;
      else if (progress > 100)
        progress = 100;

      EventTitle = Event->Title();
    }
  }

  if (WithProvider)
    buffer = cString::sprintf("%s - %s", Channel->Provider(), Channel->Name());
  else
    buffer = cString::sprintf("%s", Channel->Name());

  if (Config.MenuChannelView == 1) {
    Width = menuItemWidth - LeftName;
    if (Channel->GroupSep()) {
      int lineTop = Top + (fontHeight - 3) / 2;
      menuPixmap->DrawRectangle(cRect(Left, lineTop, menuItemWidth - Left, 3), ColorFg);
      cString groupname = cString::sprintf(" %s ", *buffer);
      menuPixmap->DrawText(cPoint(Left + (menuItemWidth / 10 * 2), Top), groupname, ColorFg, ColorBg, font, 0, 0,
                           taCenter);
    } else {
      if (Current && font->Width(buffer) > (Width) && Config.ScrollerEnable) {
        menuItemScroller.AddScroller(buffer, cRect(LeftName, Top + menuTop, Width, fontHeight), ColorFg, clrTransparent,
                                     font);
      } else {
        menuPixmap->DrawText(cPoint(LeftName, Top), buffer, ColorFg, ColorBg, font, Width);
      }
    }
  } else {
    Width = menuItemWidth / 10 * 2;
    if (isScrolling)
      Width = (menuItemWidth + scrollBarWidth) / 10 * 2;

    if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4)
      Width = menuItemWidth - LeftName;

    if (Channel->GroupSep()) {
      int lineTop = Top + (fontHeight - 3) / 2;
      menuPixmap->DrawRectangle(cRect(Left, lineTop, menuItemWidth - Left, 3), ColorFg);
      cString groupname = cString::sprintf(" %s ", *buffer);
      menuPixmap->DrawText(cPoint(Left + (menuItemWidth / 10 * 2), Top), groupname, ColorFg, ColorBg, font, 0, 0,
                           taCenter);
    } else {
      if (Current && font->Width(buffer) > (Width) && Config.ScrollerEnable) {
        menuItemScroller.AddScroller(buffer, cRect(LeftName, Top + menuTop, Width, fontHeight), ColorFg, clrTransparent,
                                     font);
      } else {
        menuPixmap->DrawText(cPoint(LeftName, Top), buffer, ColorFg, ColorBg, font, Width);
      }

      Left += Width + marginItem;

      if (DrawProgress) {
        int PBTop = y + (itemChannelHeight - Config.MenuItemPadding) / 2 - Config.decorProgressMenuItemSize / 2 -
                    Config.decorBorderMenuItemSize;
        int PBLeft = Left;
        int PBWidth = menuItemWidth / 10;
        if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4) {
          PBTop = Top + fontHeight + fontSmlHeight;
          PBLeft = Left - Width - marginItem;
          PBWidth = menuItemWidth - LeftName - marginItem * 2 - Config.decorBorderMenuItemSize - scrollBarWidth;

          if (isScrolling)
            PBWidth += scrollBarWidth;
        }

        Width = menuItemWidth / 10;
        if (isScrolling)
          Width = (menuItemWidth + scrollBarWidth) / 10;
        if (Current)
          ProgressBarDrawRaw(menuPixmap, menuPixmap, cRect(PBLeft, PBTop, PBWidth, Config.decorProgressMenuItemSize),
                             cRect(PBLeft, PBTop, PBWidth, Config.decorProgressMenuItemSize), progress, 100,
                             Config.decorProgressMenuItemCurFg, Config.decorProgressMenuItemCurBarFg,
                             Config.decorProgressMenuItemCurBg, Config.decorProgressMenuItemType, false);
        else
          ProgressBarDrawRaw(menuPixmap, menuPixmap, cRect(PBLeft, PBTop, PBWidth, Config.decorProgressMenuItemSize),
                             cRect(PBLeft, PBTop, PBWidth, Config.decorProgressMenuItemSize), progress, 100,
                             Config.decorProgressMenuItemFg, Config.decorProgressMenuItemBarFg,
                             Config.decorProgressMenuItemBg, Config.decorProgressMenuItemType, false);
        Left += Width + marginItem;
      }

      if (Config.MenuChannelView == 3 || Config.MenuChannelView == 4) {
        Left = LeftName;
        Top += fontHeight;

        if (Current && fontSml->Width(EventTitle) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
          menuItemScroller.AddScroller(EventTitle,
                                       cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontSmlHeight),
                                       ColorFg, clrTransparent, fontSml);
        } else {
          menuPixmap->DrawText(cPoint(Left, Top), EventTitle, ColorFg, ColorBg, fontSml,
                               menuItemWidth - Left - marginItem);
        }
      } else {
        if (Current && font->Width(EventTitle) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
          menuItemScroller.AddScroller(EventTitle,
                                       cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight),
                                       ColorFg, clrTransparent, font);
        } else {
          menuPixmap->DrawText(cPoint(Left, Top), EventTitle, ColorFg, ColorBg, font,
                               menuItemWidth - Left - marginItem);
        }
      }
    }
  }

  sDecorBorder ib;
  ib.Left = Config.decorBorderMenuItemSize;
  ib.Top = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y;

  ib.Width = menuItemWidth - Config.decorBorderMenuItemSize * 2;

  if (isScrolling)
    ib.Width -= scrollBarWidth;

  ib.Width = menuItemWidth;

  ib.Height = Height;
  ib.Size = Config.decorBorderMenuItemSize;
  ib.Type = Config.decorBorderMenuItemType;

  if (Current) {
    ib.ColorFg = Config.decorBorderMenuItemCurFg;
    ib.ColorBg = Config.decorBorderMenuItemCurBg;
  } else {
    if (Selectable) {
      ib.ColorFg = Config.decorBorderMenuItemSelFg;
      ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
      ib.ColorFg = Config.decorBorderMenuItemFg;
      ib.ColorBg = Config.decorBorderMenuItemBg;
    }
  }

  DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);

  if (!isScrolling)
    ItemBorderInsertUnique(ib);

  if (Config.MenuChannelView == 4 && Current)
    DrawItemExtraEvent(Event, "");

  return true;
}

void cFlatDisplayMenu::DrawItemExtraEvent(const cEvent *Event, cString EmptyText) {
  cLeft = menuItemWidth + Config.decorBorderMenuItemSize * 2 + Config.decorBorderMenuContentSize + marginItem;
  if (isScrolling)
    cLeft += scrollBarWidth;
  cTop = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
  cWidth = menuWidth - cLeft - Config.decorBorderMenuContentSize;
  cHeight = osdHeight - (topBarHeight + Config.decorBorderTopBarSize * 2 + buttonsHeight +
                         Config.decorBorderButtonSize * 2 + marginItem * 3 + Config.decorBorderMenuContentSize * 2);

  bool isEmpty = false;
  // Description
  ostringstream text("");
  if (Event) {
    if (!isempty(Event->Description())) {
      text << Event->Description();
    }

    if (Config.EpgAdditionalInfoShow) {
      text << "\n";
      // Genre
      bool firstContent = true;
      for (int i = 0; Event->Contents(i); i++) {
        if (!isempty(Event->ContentToString(Event->Contents(i)))) { // skip empty (user defined) content
          if (!firstContent)
            text << ", ";
          else
            text << "\n" << tr("Genre") << ": ";
          text << Event->ContentToString(Event->Contents(i));
          firstContent = false;
        }
      }
      // FSK
      if (Event->ParentalRating()) {
        text << "\n" << tr("FSK") << ": " << *Event->GetParentalRatingString();
      }
      const cComponents *Components = Event->Components();
      if (Components) {
        ostringstream audio("");
        bool firstAudio = true;
        const char *audio_type = NULL;
        ostringstream subtitle("");
        bool firstSubtitle = true;
        for (int i = 0; i < Components->NumComponents(); i++) {
          const tComponent *p = Components->Component(i);
          switch (p->stream) {
          case sc_video_MPEG2:
            if (p->description)
              text << "\n" << tr("Video") << ": " << p->description << " (MPEG2)";
            else
              text << "\n" << tr("Video") << ": MPEG2";
            break;
          case sc_video_H264_AVC:
            if (p->description)
              text << "\n" << tr("Video") << ": " << p->description << " (H.264)";
            else
              text << "\n" << tr("Video") << ": H.264";
            break;
          case sc_video_H265_HEVC:  //might be not always correct because stream_content_ext (must be 0x0) is not available in tComponent
            if (p->description)
              text << "\n" << tr("Video") << ": " << p->description << " (H.265)";
            else
              text << "\n" << tr("Video") << ": H.265";
            break;

          case sc_audio_MP2:
          case sc_audio_AC3:
          case sc_audio_HEAAC:
            if (firstAudio)
              firstAudio = false;
            else
              audio << ", ";
            switch (p->stream) {
            case sc_audio_MP2:
              // workaround for wrongfully used stream type X 02 05 for AC3
              if (p->type == 5)
                audio_type = "AC3";
              else
                audio_type = "MP2";
              break;
            case sc_audio_AC3:
              audio_type = "AC3";
              break;
            case sc_audio_HEAAC:
              audio_type = "HEAAC";
              break;
            }
            if (p->description)
              audio << p->description << " (" << audio_type << ", " << p->language << ")";
            else
              audio << p->language << " (" << audio_type << ")";
            break;
          case sc_subtitle:
            if (firstSubtitle)
              firstSubtitle = false;
            else
              subtitle << ", ";
            if (p->description)
              subtitle << p->description << " (" << p->language << ")";
            else
              subtitle << p->language;
            break;
          }
        }
        if (audio.str().length() > 0)
          text << "\n" << tr("Audio") << ": " << audio.str();
        if (subtitle.str().length() > 0)
          text << "\n" << tr("Subtitle") << ": " << subtitle.str();
      }
    }
  } else {
    text << *EmptyText;
    isEmpty = true;
  }

  ComplexContent.Clear();
  ComplexContent.SetScrollSize(fontSmlHeight);
  ComplexContent.SetScrollingActive(false);
  ComplexContent.SetOsd(osd);
  ComplexContent.SetPosition(cRect(cLeft, cTop, cWidth, cHeight));
  ComplexContent.SetBGColor(Theme.Color(clrMenuEventBg));

  if (isEmpty) {
    cImage *img = imgLoader.LoadIcon("timerInactiveBig", 256, 256);
    if (img) {
      ComplexContent.AddImage(img, cRect(marginItem, marginItem, img->Width(), img->Height()));
      ComplexContent.AddText(
          text.str().c_str(), true,
          cRect(marginItem, marginItem + img->Height(), cWidth - marginItem * 2, cHeight - marginItem * 2),
          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml);
    }
  } else {
    std::string mediaPath("");
    int mediaWidth = 0;
    int mediaHeight = 0;
    int mediaType = 0;

    // first try scraper2vdr
    static cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
    if (!pScraper) // if it doesn't exit, try tvscraper
      pScraper = cPluginManager::GetPlugin("tvscraper");

    if (Config.TVScraperEPGInfoShowPoster && pScraper) {
      ScraperGetPosterBannerV2 call;
      call.event = Event;
	    call.recording = NULL;
      if (pScraper->Service("GetPosterBannerV2", &call)) {
        if ((call.type == tSeries) && call.banner.path.size() > 0) {
          mediaWidth = cWidth - marginItem * 2;
          mediaHeight = 999;
          mediaPath = call.banner.path;
          mediaType = 1;
        } else if (call.type == tMovie && call.poster.path.size() > 0) {
          mediaWidth = cWidth / 2 - marginItem * 3;
          mediaHeight = 999;
          mediaPath = call.poster.path;
          mediaType = 2;
        }
      }
    }

    if (mediaPath.length() > 0) {
      cImage *img = imgLoader.LoadFile(mediaPath.c_str(), mediaWidth, mediaHeight);
      if (img && mediaType == 2) {
        ComplexContent.AddImageWithFloatedText(
            img, CIP_Right, text.str().c_str(),
            cRect(marginItem, marginItem, cWidth - marginItem * 2, cHeight - marginItem * 2),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml);
      } else if (img && mediaType == 1) {
        ComplexContent.AddImage(img, cRect(marginItem, marginItem, img->Width(), img->Height()));
        ComplexContent.AddText(
            text.str().c_str(), true,
            cRect(marginItem, marginItem + img->Height(), cWidth - marginItem * 2, cHeight - marginItem * 2),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml);
      } else {
        ComplexContent.AddText(text.str().c_str(), true,
                               cRect(marginItem, marginItem, cWidth - marginItem * 2, cHeight - marginItem * 2),
                               Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml);
      }
    } else {
      ComplexContent.AddText(text.str().c_str(), true,
                             cRect(marginItem, marginItem, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml);
    }
  }
  ComplexContent.CreatePixmaps(Config.MenuContentFullSize);

  ComplexContent.Draw();

  DecorBorderClearByFrom(BorderContent);
  if (Config.MenuContentFullSize)
    DecorBorderDraw(cLeft, cTop, cWidth, ComplexContent.ContentHeight(true), Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg,
                    BorderContent);
  else
    DecorBorderDraw(cLeft, cTop, cWidth, ComplexContent.ContentHeight(false), Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg,
                    BorderContent);
}

bool cFlatDisplayMenu::SetItemTimer(const cTimer *Timer, int Index, bool Current, bool Selectable) {
  if (Config.MenuTimerView == 0 || !Timer)
    return false;

  const cChannel *Channel = Timer->Channel();
  const cEvent *Event = Timer->Event();

  cString buffer("");
  int y = Index * itemTimerHeight;

  if (Current)
    menuItemScroller.Clear();

  int Height = fontHeight;
  menuItemWidth = menuWidth - Config.decorBorderMenuItemSize * 2;
  if (Config.MenuTimerView == 2 || Config.MenuTimerView == 3) {
    Height = fontHeight + fontSmlHeight + marginItem;
    menuItemWidth *= 0.5;
  }

  if (isScrolling)
    menuItemWidth -= scrollBarWidth;

  tColor ColorFg, ColorBg, ColorExtraTextFg;
  ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextFont);
  if (Current) {
    ColorFg = Theme.Color(clrItemCurrentFont);
    ColorBg = Theme.Color(clrItemCurrentBg);
    ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextCurrentFont);
  } else {
    if (Selectable) {
      ColorFg = Theme.Color(clrItemSelableFont);
      ColorBg = Theme.Color(clrItemSelableBg);
    } else {
      ColorFg = Theme.Color(clrItemFont);
      ColorBg = Theme.Color(clrItemBg);
    }
  }

  if (y + itemTimerHeight > menuItemLastHeight)
    menuItemLastHeight = y + itemTimerHeight;

  menuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, menuItemWidth, Height), ColorBg);
  cImage *img = NULL;
  int Left = 0, Top = 0;
  Left = Config.decorBorderMenuItemSize + marginItem;
  Top = y;

  int imageHeight = fontHeight;
  int imageLeft = Left;
  int imageTop = Top;

  cString TimerIconName("");
  if (!(Timer->HasFlags(tfActive))) {
    if (Current)
      TimerIconName = "timerInactive_cur";
    else
      TimerIconName = "timerInactive";

    ColorFg = Theme.Color(clrMenuTimerItemDisabledFont);
  } else if (Timer->Recording()) {
    TimerIconName = "timerRecording";
    ColorFg = Theme.Color(clrMenuTimerItemRecordingFont);
  } else
    TimerIconName = "timerActive";

  img = imgLoader.LoadIcon(TimerIconName, imageHeight, imageHeight);
  if (img) {
    imageTop = Top + (fontHeight - img->Height()) / 2;
    menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
  }
  Left += imageHeight + marginItem * 2;

#if VDRVERSNUM >= 20301
  LOCK_CHANNELS_READ;
  cString ws = cString::sprintf("%d", Channels->MaxNumber());
#else
  cString ws = cString::sprintf("%d", Channels.MaxNumber());
#endif
  int w = font->Width(ws);
  buffer = cString::sprintf("%d", Channel->Number());
  int Width = font->Width(buffer);
  if (Width < w)
    Width = w;
  menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, font, Width, fontHeight, taRight);
  Left += Width + marginItem;

  imageLeft = Left;

  int imageBGHeight = imageHeight;
  int imageBGWidth = imageHeight * 1.34;

  cImage *imgBG = imgLoader.LoadIcon("logo_background", imageBGWidth, imageBGHeight);
  if (imgBG) {
    imageBGHeight = imgBG->Height();
    imageBGWidth = imgBG->Width();
    imageTop = Top + (fontHeight - imgBG->Height()) / 2;
    menuIconsBGPixmap->DrawImage(cPoint(imageLeft, imageTop), *imgBG);
  }

  img = imgLoader.LoadLogo(Channel->Name(), imageBGWidth - 4, imageBGHeight - 4);
  if (img) {
    imageTop = Top + (imageBGHeight - img->Height()) / 2;
    imageLeft = Left + (imageBGWidth - img->Width()) / 2;
    menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
  } else {
    bool isRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;

    if (isRadioChannel) {
      if (Current)
        img = imgLoader.LoadIcon("radio_cur", imageBGWidth - 10, imageBGHeight - 10);
      if (img == NULL)
        img = imgLoader.LoadIcon("radio", imageBGWidth - 10, imageBGHeight - 10);

      if (img) {
        imageTop = Top + (imageBGHeight - img->Height()) / 2;
        imageLeft = Left + (imageBGWidth - img->Width()) / 2;
        menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
      }
    } else if (Channel->GroupSep()) {
      img = imgLoader.LoadIcon("changroup", imageBGWidth - 10, imageBGHeight - 10);
      if (img) {
        imageTop = Top + (imageBGHeight - img->Height()) / 2;
        imageLeft = Left + (imageBGWidth - img->Width()) / 2;
        menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
      }
    } else {
      if (Current)
        img = imgLoader.LoadIcon("tv_cur", imageBGWidth - 10, imageBGHeight - 10);
      if (img == NULL)
        img = imgLoader.LoadIcon("tv", imageBGWidth - 10, imageBGHeight - 10);

      if (img) {
        imageTop = Top + (imageBGHeight - img->Height()) / 2;
        imageLeft = Left + (imageBGWidth - img->Width()) / 2;
        menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
      }
    }
  }
  Left += imageBGWidth + marginItem * 2;

  cString day(""), name("");
  if (Timer->WeekDays())
    day = Timer->PrintDay(0, Timer->WeekDays(), false);
  else if (Timer->Day() - time(NULL) < 28 * SECSINDAY) {
    day = itoa(Timer->GetMDay(Timer->Day()));
    name = WeekDayName(Timer->Day());
  } else {
    struct tm tm_r;
    time_t Day = Timer->Day();
    localtime_r(&Day, &tm_r);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%Y%m%d", &tm_r);
    day = buffer;
  }
  const char *File = Setup.FoldersInTimerMenu ? NULL : strrchr(Timer->File(), FOLDERDELIMCHAR);
  if (File && strcmp(File + 1, TIMERMACRO_TITLE) && strcmp(File + 1, TIMERMACRO_EPISODE))
    File++;
  else
    File = Timer->File();

  if (Config.MenuTimerView == 1) {
    buffer = cString::sprintf("%s%s%s.", *name, *name && **name ? " " : "", *day);
    menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, font, menuItemWidth - Left - marginItem);
    Left += font->Width("XXX 99.  ");
    buffer = cString::sprintf("%02d:%02d - %02d:%02d", Timer->Start() / 100, Timer->Start() % 100, Timer->Stop() / 100,
                              Timer->Stop() % 100);
    menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, font, menuItemWidth - Left - marginItem);
    Left += font->Width("99:99 - 99:99  ");

    if (Current && font->Width(File) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
      menuItemScroller.AddScroller(File, cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight),
                                   ColorFg, clrTransparent, font, ColorExtraTextFg);
    } else {
      if (Config.MenuItemParseTilde) {
        std::string tilde = File;
        size_t found = tilde.find(" ~ ");
        size_t found2 = tilde.find("~");
        if (found != string::npos) {
          std::string first = tilde.substr(0, found);
          std::string second = tilde.substr(found + 2, tilde.length());

          menuPixmap->DrawText(cPoint(Left, Top), first.c_str(), ColorFg, ColorBg, font,
                               menuItemWidth - Left - marginItem);
          int l = font->Width(first.c_str());
          menuPixmap->DrawText(cPoint(Left + l, Top), second.c_str(), ColorExtraTextFg, ColorBg, font,
                               menuItemWidth - Left - l - marginItem);
        } else if (found2 != string::npos) {
          std::string first = tilde.substr(0, found2);
          std::string second = tilde.substr(found2 + 1, tilde.length());

          menuPixmap->DrawText(cPoint(Left, Top), first.c_str(), ColorFg, ColorBg, font,
                               menuItemWidth - Left - marginItem);
          int l = font->Width(first.c_str());
          l += font->Width("X");
          menuPixmap->DrawText(cPoint(Left + l, Top), second.c_str(), ColorExtraTextFg, ColorBg, font,
                               menuItemWidth - Left - l - marginItem);
        } else
          menuPixmap->DrawText(cPoint(Left, Top), File, ColorFg, ColorBg, font, menuItemWidth - Left - marginItem);
      } else {
        menuPixmap->DrawText(cPoint(Left, Top), File, ColorFg, ColorBg, font, menuItemWidth - Left - marginItem);
      }
    }
  } else if (Config.MenuTimerView == 2 || Config.MenuTimerView == 3) {
    buffer = cString::sprintf("%s%s%s.  %02d:%02d - %02d:%02d", *name, *name && **name ? " " : "", *day,
                              Timer->Start() / 100, Timer->Start() % 100, Timer->Stop() / 100, Timer->Stop() % 100);
    menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, font, menuItemWidth - Left - marginItem);

    if (Current && fontSml->Width(File) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
      menuItemScroller.AddScroller(
          File,
          cRect(Left, Top + fontHeight + menuTop, menuItemWidth - Left - marginItem - scrollBarWidth, fontSmlHeight),
          ColorFg, clrTransparent, fontSml, ColorExtraTextFg);
    } else {
      if (Config.MenuItemParseTilde) {
        std::string tilde = File;
        size_t found = tilde.find(" ~ ");
        size_t found2 = tilde.find("~");
        if (found != string::npos) {
          std::string first = tilde.substr(0, found);
          std::string second = tilde.substr(found + 2, tilde.length());

          menuPixmap->DrawText(cPoint(Left, Top + fontHeight), first.c_str(), ColorFg, ColorBg, fontSml,
                               menuItemWidth - Left - marginItem);
          int l = fontSml->Width(first.c_str());
          menuPixmap->DrawText(cPoint(Left + l, Top + fontHeight), second.c_str(), ColorExtraTextFg, ColorBg, fontSml,
                               menuItemWidth - Left - l - marginItem);
        } else if (found2 != string::npos) {
          std::string first = tilde.substr(0, found2);
          std::string second = tilde.substr(found2 + 1, tilde.length());

          menuPixmap->DrawText(cPoint(Left, Top + fontHeight), first.c_str(), ColorFg, ColorBg, fontSml,
                               menuItemWidth - Left - marginItem);
          int l = fontSml->Width(first.c_str());
          l += fontSml->Width("X");
          menuPixmap->DrawText(cPoint(Left + l, Top + fontHeight), second.c_str(), ColorExtraTextFg, ColorBg, fontSml,
                               menuItemWidth - Left - l - marginItem);
        } else
          menuPixmap->DrawText(cPoint(Left, Top + fontHeight), File, ColorFg, ColorBg, fontSml,
                               menuItemWidth - Left - marginItem);
      } else {
        menuPixmap->DrawText(cPoint(Left, Top + fontHeight), File, ColorFg, ColorBg, fontSml,
                             menuItemWidth - Left - marginItem);
      }
    }
  }

  sDecorBorder ib;
  ib.Left = Config.decorBorderMenuItemSize;
  ib.Top = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y;

  ib.Width = menuItemWidth - Config.decorBorderMenuItemSize * 2;

  if (isScrolling)
    ib.Width -= scrollBarWidth;

  ib.Width = menuItemWidth;

  ib.Height = Height;
  ib.Size = Config.decorBorderMenuItemSize;
  ib.Type = Config.decorBorderMenuItemType;

  if (Current) {
    ib.ColorFg = Config.decorBorderMenuItemCurFg;
    ib.ColorBg = Config.decorBorderMenuItemCurBg;
  } else {
    if (Selectable) {
      ib.ColorFg = Config.decorBorderMenuItemSelFg;
      ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
      ib.ColorFg = Config.decorBorderMenuItemFg;
      ib.ColorBg = Config.decorBorderMenuItemBg;
    }
  }

  DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);

  if (!isScrolling)
    ItemBorderInsertUnique(ib);

  if (Config.MenuTimerView == 3 && Current)
    DrawItemExtraEvent(Event, tr("timer not enabled"));

  return true;
}

#if APIVERSNUM >= 20308
bool cFlatDisplayMenu::SetItemEvent(const cEvent *Event, int Index, bool Current, bool Selectable,
                                    const cChannel *Channel, bool WithDate, eTimerMatch TimerMatch, bool TimerActive) {
#else
bool cFlatDisplayMenu::SetItemEvent(const cEvent *Event, int Index, bool Current, bool Selectable,
                                    const cChannel *Channel, bool WithDate, eTimerMatch TimerMatch) {
#endif
  if (Config.MenuEventView == 0)
    return false;

  if (Config.MenuEventViewAllwaysWithDate)
    WithDate = true;

  cImage *img = NULL;
  cString buffer("");
  int y = Index * itemEventHeight;

  if (Current)
    menuItemScroller.Clear();

  int Height = fontHeight;
  menuItemWidth = menuWidth - Config.decorBorderMenuItemSize * 2;
  if (Config.MenuEventView == 2 || Config.MenuEventView == 3) {
    Height = fontHeight + fontSmlHeight + marginItem * 2 + Config.decorProgressMenuItemSize / 2;
    menuItemWidth *= 0.6;
}

  if (isScrolling)
    menuItemWidth -= scrollBarWidth;

  tColor ColorFg, ColorBg, ColorExtraTextFg;
  ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextFont);
  if (Current) {
    ColorFg = Theme.Color(clrItemCurrentFont);
    ColorBg = Theme.Color(clrItemCurrentBg);
    ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextCurrentFont);
  } else {
    if (Selectable) {
      ColorFg = Theme.Color(clrItemSelableFont);
      ColorBg = Theme.Color(clrItemSelableBg);
    } else {
      ColorFg = Theme.Color(clrItemFont);
      ColorBg = Theme.Color(clrItemBg);
    }
  }

  if (y + itemEventHeight > menuItemLastHeight)
    menuItemLastHeight = y + itemEventHeight;

  menuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, menuItemWidth, Height), ColorBg);

  int Left = 0, Top = 0, LeftSecond = 0;
  LeftSecond = Left = Config.decorBorderMenuItemSize + marginItem;
  Top = y;
  int imageTop = Top;
  int w = 0;

  if (!Channel)
    TopBarSetMenuLogo(ItemEventLastChannelName);

  if (Channel) {
    if (Current) {
      ItemEventLastChannelName = Channel->Name();
    }
#if VDRVERSNUM >= 20301
    LOCK_CHANNELS_READ;
    cString ws = cString::sprintf("%d", Channels->MaxNumber());
#else
    cString ws = cString::sprintf("%d", Channels.MaxNumber());
#endif
    w = font->Width(ws);
    if (!Channel->GroupSep()) {
      buffer = cString::sprintf("%d", Channel->Number());
      int Width = font->Width(buffer);
      if (Width < w)
        Width = w;
      menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, font, Width, fontHeight, taRight);
    }
    Left += w + marginItem;

    int imageLeft = Left;
    int imageBGHeight = fontHeight;
    int imageBGWidth = fontHeight * 1.34;
    cImage *imgBG = imgLoader.LoadIcon("logo_background", imageBGWidth, imageBGHeight);
    if (imgBG && !Channel->GroupSep()) {
      imageBGHeight = imgBG->Height();
      imageBGWidth = imgBG->Width();
      imageTop = Top + (fontHeight - imgBG->Height()) / 2;
      menuIconsBGPixmap->DrawImage(cPoint(imageLeft, imageTop), *imgBG);
    }
    img = imgLoader.LoadLogo(Channel->Name(), imageBGWidth - 4, imageBGHeight - 4);
    if (img) {
      imageTop = Top + (imageBGHeight - img->Height()) / 2;
      imageLeft = Left + (imageBGWidth - img->Width()) / 2;
      menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
    } else {
      bool isRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;

      if (isRadioChannel) {
        if (Current)
          img = imgLoader.LoadIcon("radio_cur", imageBGWidth - 10, imageBGHeight - 10);
        if (img == NULL)
          img = imgLoader.LoadIcon("radio", imageBGWidth - 10, imageBGHeight - 10);

        if (img) {
          imageTop = Top + (imageBGHeight - img->Height()) / 2;
          imageLeft = Left + (imageBGWidth - img->Width()) / 2;
          menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
        }
      } else if (Channel->GroupSep()) {
        img = imgLoader.LoadIcon("changroup", imageBGWidth - 10, imageBGHeight - 10);
        if (img) {
          imageTop = Top + (imageBGHeight - img->Height()) / 2;
          imageLeft = Left + (imageBGWidth - img->Width()) / 2;
          menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
        }
      } else {
        if (Current)
          img = imgLoader.LoadIcon("tv_cur", imageBGWidth - 10, imageBGHeight - 10);
        if (img == NULL)
          img = imgLoader.LoadIcon("tv", imageBGWidth - 10, imageBGHeight - 10);

        if (img) {
          imageTop = Top + (imageBGHeight - img->Height()) / 2;
          imageLeft = Left + (imageBGWidth - img->Width()) / 2;
          menuIconsPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
        }
      }
    }
    Left += imageBGWidth + marginItem * 2;
    LeftSecond = Left;

    cString channame("");

    w = menuItemWidth / 10 * 2;
    if (!isScrolling)
      w = (menuItemWidth - scrollBarWidth) / 10 * 2;

    if (Config.MenuEventView == 2 || Config.MenuEventView == 3) {
      channame = Channel->Name();
      w = font->Width(channame);
    } else
      channame = Channel->ShortName(true);

    if (Channel->GroupSep()) {
      int lineTop = Top + (fontHeight - 3) / 2;
      menuPixmap->DrawRectangle(cRect(Left, lineTop, menuItemWidth - Left, 3), ColorFg);
      Left += w / 2;
      cString groupname = cString::sprintf(" %s ", *channame);
      menuPixmap->DrawText(cPoint(Left, Top), groupname, ColorFg, ColorBg, font, 0, 0, taCenter);
    } else
      menuPixmap->DrawText(cPoint(Left, Top), channame, ColorFg, ColorBg, font, w);

    Left += w + marginItem * 2;

    if (Event) {
      int PBWidth = menuItemWidth / 20;

      if (!isScrolling)
        PBWidth = (menuItemWidth - scrollBarWidth) / 20;

      time_t now = time(NULL);

      if ((now >= (Event->StartTime() - 2 * 60))) {
        int total = Event->EndTime() - Event->StartTime();
        if (total >= 0) {
          // calculate progress bar
          double progress = (int)roundf((float)(time(NULL) - Event->StartTime()) / (float)(Event->Duration()) * 100.0);
          if (progress < 0)
            progress = 0.;
          else if (progress > 100)
            progress = 100;
          int PBTop = y + (itemEventHeight - Config.MenuItemPadding) / 2 - Config.decorProgressMenuItemSize / 2 -
                      Config.decorBorderMenuItemSize;
          int PBLeft = Left;
          int PBHeight = Config.decorProgressMenuItemSize;

          if ((Config.MenuEventView == 2 || Config.MenuEventView == 3)) {
            PBTop = y + fontHeight + fontSmlHeight + marginItem;
            PBWidth = menuItemWidth - LeftSecond - scrollBarWidth - marginItem * 2;
            if (isScrolling)
              PBWidth += scrollBarWidth;

            PBLeft = LeftSecond;
            PBHeight = Config.decorProgressMenuItemSize / 2;
          }

          if (Current)
            ProgressBarDrawRaw(menuPixmap, menuPixmap, cRect(PBLeft, PBTop, PBWidth, PBHeight),
                               cRect(PBLeft, PBTop, PBWidth, PBHeight), progress, 100,
                               Config.decorProgressMenuItemCurFg, Config.decorProgressMenuItemCurBarFg,
                               Config.decorProgressMenuItemCurBg, Config.decorProgressMenuItemType, false);
          else
            ProgressBarDrawRaw(menuPixmap, menuPixmap, cRect(PBLeft, PBTop, PBWidth, PBHeight),
                               cRect(PBLeft, PBTop, PBWidth, PBHeight), progress, 100, Config.decorProgressMenuItemFg,
                               Config.decorProgressMenuItemBarFg, Config.decorProgressMenuItemBg,
                               Config.decorProgressMenuItemType, false);
        }
      }
      Left += PBWidth + marginItem * 2;
    }
  }

  if (WithDate && Event && Selectable) {
    if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Channel)
      w = fontSml->Width("XXX 99. ") + marginItem;
    else
      w = font->Width("XXX 99. ") + marginItem;

    struct tm tm_r;
    time_t Day = Event->StartTime();
    localtime_r(&Day, &tm_r);
    char buf[8];
    strftime(buf, sizeof(buf), "%2d", &tm_r);

    cString DateString = cString::sprintf("%s %s. ", *WeekDayName((time_t)Event->StartTime()), buf);
    if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Channel) {
      menuPixmap->DrawText(cPoint(LeftSecond, Top + fontHeight), DateString, ColorFg, ColorBg, fontSml, w);
      LeftSecond += w + marginItem;
    } else
      menuPixmap->DrawText(cPoint(Left, Top), DateString, ColorFg, ColorBg, font, w, fontHeight, taRight);

    Left += w + marginItem;
  }

  int imageHeight = fontHeight;
  if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Channel && Event && Selectable) {
    Top += fontHeight;
    Left = LeftSecond;
    imageHeight = fontSmlHeight;
    menuPixmap->DrawText(cPoint(Left, Top), Event->GetTimeString(), ColorFg, ColorBg, fontSml);
    Left += fontSml->Width(Event->GetTimeString()) + marginItem;
  } else if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Event && Selectable) {
    imageHeight = fontHeight;
    menuPixmap->DrawText(cPoint(Left, Top), Event->GetTimeString(), ColorFg, ColorBg, font);
    Left += font->Width(Event->GetTimeString()) + marginItem;
  } else if (Event && Selectable) {
    menuPixmap->DrawText(cPoint(Left, Top), Event->GetTimeString(), ColorFg, ColorBg, font);
    Left += font->Width(Event->GetTimeString()) + marginItem;
  }

  if (TimerMatch == tmFull) {
    img = NULL;
    if (Current)
      img = imgLoader.LoadIcon("timer_full_cur", imageHeight, imageHeight);
    if (img == NULL)
      img = imgLoader.LoadIcon("timer_full", imageHeight, imageHeight);

    if (img) {
      imageTop = Top;
      menuIconsPixmap->DrawImage(cPoint(Left, imageTop), *img);
    }
  } else if (TimerMatch == tmPartial) {
    img = NULL;
    if (Current)
      img = imgLoader.LoadIcon("timer_partial_cur", imageHeight, imageHeight);
    if (img == NULL)
      img = imgLoader.LoadIcon("timer_partial", imageHeight, imageHeight);

    if (img) {
      imageTop = Top;
      menuIconsPixmap->DrawImage(cPoint(Left, imageTop), *img);
    }
  }
  Left += imageHeight + marginItem;
  if (Event && Selectable) {
    if (Event->Vps() && (Event->Vps() - Event->StartTime())) {
      img = NULL;
      if (Current)
        img = imgLoader.LoadIcon("vps_cur", imageHeight, imageHeight);
      if (img == NULL)
        img = imgLoader.LoadIcon("vps", imageHeight, imageHeight);

      if (img) {
        imageTop = Top;
        menuIconsPixmap->DrawImage(cPoint(Left, imageTop), *img);
      }
    }
    Left += imageHeight + marginItem;

    if ((Config.MenuEventView == 2 || Config.MenuEventView == 3) && Channel) {
      if (Current) {
        if (Event->ShortText()) {
          cString t = cString::sprintf("%s~%s", Event->Title(), Event->ShortText());
          if (fontSml->Width(t) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
            menuItemScroller.AddScroller(*t,
                                         cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontSmlHeight),
                                         ColorFg, clrTransparent, fontSml, ColorExtraTextFg);
          } else {
            menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, fontSml,
                                 menuItemWidth - Left - marginItem);
            Left += fontSml->Width(Event->Title()) + fontSml->Width("~");
            cString ShortText = cString::sprintf("%s", Event->ShortText());
            menuPixmap->DrawText(cPoint(Left, Top), ShortText, ColorExtraTextFg, ColorBg, fontSml,
                                 menuItemWidth - Left - marginItem);
          }
        } else {
          if (fontSml->Width(Event->Title()) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
            menuItemScroller.AddScroller(Event->Title(),
                                         cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontSmlHeight),
                                         ColorFg, clrTransparent, fontSml, ColorExtraTextFg);
          } else {
            menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, fontSml,
                                 menuItemWidth - Left - marginItem);
          }
        }
      } else {
        menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, fontSml,
                             menuItemWidth - Left - marginItem);
        if (Event->ShortText()) {
          Left += fontSml->Width(Event->Title()) + font->Width("~");
          cString ShortText = cString::sprintf("%s", Event->ShortText());
          menuPixmap->DrawText(cPoint(Left, Top), ShortText, ColorExtraTextFg, ColorBg, fontSml,
                               menuItemWidth - Left - marginItem);
        }
      }
    } else if ((Config.MenuEventView == 2 || Config.MenuEventView == 3)) {
      if (Current && font->Width(Event->Title()) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
        menuItemScroller.AddScroller(Event->Title(),
                                     cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight), ColorFg,
                                     clrTransparent, font);
      } else {
        menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, font,
                             menuItemWidth - Left - marginItem);
      }
      if (Event->ShortText()) {
        Top += fontHeight;
        if (Current && fontSml->Width(Event->ShortText()) > (menuItemWidth - Left - marginItem) &&
            Config.ScrollerEnable) {
          menuItemScroller.AddScroller(Event->ShortText(),
                                       cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight),
                                       ColorExtraTextFg, clrTransparent, fontSml);
        } else {
          menuPixmap->DrawText(cPoint(Left, Top), Event->ShortText(), ColorExtraTextFg, ColorBg, fontSml,
                               menuItemWidth - Left - marginItem);
        }
      }
    } else {
      if (Current) {
        if (Event->ShortText()) {
          cString t = cString::sprintf("%s~%s", Event->Title(), Event->ShortText());
          if (font->Width(t) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
            menuItemScroller.AddScroller(*t, cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight),
                                         ColorFg, clrTransparent, font, ColorExtraTextFg);
          } else {
            menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, font,
                                 menuItemWidth - Left - marginItem);
            Left += font->Width(Event->Title()) + font->Width("~");
            cString ShortText = cString::sprintf("%s", Event->ShortText());
            menuPixmap->DrawText(cPoint(Left, Top), ShortText, ColorExtraTextFg, ColorBg, font,
                                 menuItemWidth - Left - marginItem);
          }
        } else {
          if (font->Width(Event->Title()) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
            menuItemScroller.AddScroller(Event->Title(),
                                         cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight),
                                         ColorFg, clrTransparent, font, ColorExtraTextFg);
          } else {
            menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, font,
                                 menuItemWidth - Left - marginItem);
          }
        }
      } else {
        menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, font,
                             menuItemWidth - Left - marginItem);
        if (Event->ShortText()) {
          Left += font->Width(Event->Title()) + font->Width("~");
          cString ShortText = cString::sprintf("%s", Event->ShortText());
          menuPixmap->DrawText(cPoint(Left, Top), ShortText, ColorExtraTextFg, ColorBg, font,
                               menuItemWidth - Left - marginItem);
        }
      }
    }
  } else if (Event) {
    try {
      // extract date from Separator
      std::string sep = Event->Title();
      if (sep.size() > 12) {
        std::size_t found = sep.find(" -");
        if (found >= 10) {
          std::string date = sep.substr(found - 10, 10);
          int lineTop = Top + (fontHeight - 3) / 2;
          menuPixmap->DrawRectangle(cRect(0, lineTop, menuItemWidth, 3), ColorFg);
          cString datespace = cString::sprintf(" %s ", date.c_str());
          menuPixmap->DrawText(cPoint(LeftSecond + menuWidth / 10 * 2, Top), datespace, ColorFg, ColorBg, font, 0, 0,
                               taCenter);
        } else
          menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, font,
                               menuItemWidth - Left - marginItem);
      } else
        menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, font,
                             menuItemWidth - Left - marginItem);
    } catch (...) {
      menuPixmap->DrawText(cPoint(Left, Top), Event->Title(), ColorFg, ColorBg, font,
                           menuItemWidth - Left - marginItem);
    }
  }

  sDecorBorder ib;
  ib.Left = Config.decorBorderMenuItemSize;
  ib.Top = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y;

  ib.Width = menuItemWidth - Config.decorBorderMenuItemSize * 2;

  if (isScrolling)
    ib.Width -= scrollBarWidth;

  ib.Width = menuItemWidth;

  ib.Height = Height;
  ib.Size = Config.decorBorderMenuItemSize;
  ib.Type = Config.decorBorderMenuItemType;

  if (Current) {
    ib.ColorFg = Config.decorBorderMenuItemCurFg;
    ib.ColorBg = Config.decorBorderMenuItemCurBg;
  } else {
    if (Selectable) {
      ib.ColorFg = Config.decorBorderMenuItemSelFg;
      ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
      ib.ColorFg = Config.decorBorderMenuItemFg;
      ib.ColorBg = Config.decorBorderMenuItemBg;
    }
  }

  DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);

  if (!isScrolling)
    ItemBorderInsertUnique(ib);

  if (Config.MenuEventView == 3 && Current)
    DrawItemExtraEvent(Event, "");

  return true;
}

bool cFlatDisplayMenu::SetItemRecording(const cRecording *Recording, int Index, bool Current, bool Selectable,
                                        int Level, int Total, int New) {
  if (Config.MenuRecordingView == 0)
    return false;

  cString buffer("");
  cString RecName = GetRecordingName(Recording, Level, Total == 0).c_str();

  if (Level > 0) {
    RecFolder = GetRecordingName(Recording, Level - 1, true).c_str();
  } else {
    RecFolder = "";
  }
  LastItemRecordingLevel = Level;

  if (LastRecFolder != RecFolder) {
    int recCount = 0, recNewCount = 0;
    LastRecFolder = RecFolder;
    if (RecFolder != "" && LastItemRecordingLevel > 0) {
#if VDRVERSNUM >= 20301
      LOCK_RECORDINGS_READ;
      for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
#else
      for (cRecording *Rec = Recordings.First(); Rec; Rec = Recordings.Next(Rec)) {
#endif
        std::string RecFolder2 = GetRecordingName(Rec, LastItemRecordingLevel - 1, true).c_str();
        if (RecFolder == RecFolder2) {
          recCount++;
          if (Rec->IsNew())
            recNewCount++;
        }
      }
    } else {
#if VDRVERSNUM >= 20301
      LOCK_RECORDINGS_READ;
      for (const cRecording *Rec = Recordings->First(); Rec; Rec = Recordings->Next(Rec)) {
#else
      for (cRecording *Rec = Recordings.First(); Rec; Rec = Recordings.Next(Rec)) {
#endif
        recCount++;
        if (Rec->IsNew())
          recNewCount++;
      }
    }
    cString newTitle = cString::sprintf("%s (%d*/%d)", *LastTitle, recNewCount, recCount);
    TopBarSetTitleWithoutClear(*newTitle);
  }

  if (Current)
    menuItemScroller.Clear();

  int y = Index * itemRecordingHeight;

  int Height = fontHeight;
  menuItemWidth = menuWidth - Config.decorBorderMenuItemSize * 2;
  if (Config.MenuRecordingView == 2 || Config.MenuRecordingView == 3) {
    Height = fontHeight + fontSmlHeight + marginItem;
    menuItemWidth *= 0.5;
  }

  if (isScrolling)
    menuItemWidth -= scrollBarWidth;

  tColor ColorFg, ColorBg, ColorExtraTextFg;
  ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextFont);
  if (Current) {
    ColorFg = Theme.Color(clrItemCurrentFont);
    ColorBg = Theme.Color(clrItemCurrentBg);
    ColorExtraTextFg = Theme.Color(clrMenuItemExtraTextCurrentFont);
  } else {
    if (Selectable) {
      ColorFg = Theme.Color(clrItemSelableFont);
      ColorBg = Theme.Color(clrItemSelableBg);
    } else {
      ColorFg = Theme.Color(clrItemFont);
      ColorBg = Theme.Color(clrItemBg);
    }
  }

  if (y + itemRecordingHeight > menuItemLastHeight)
    menuItemLastHeight = y + itemRecordingHeight;

  menuPixmap->DrawRectangle(cRect(Config.decorBorderMenuItemSize, y, menuItemWidth, Height), ColorBg);
  cImage *img = NULL;
  cImage *imgRecNew = NULL;
  if (Current)
    imgRecNew = imgLoader.LoadIcon("recording_new_cur", fontHeight, fontHeight);
  if (imgRecNew == NULL)
    imgRecNew = imgLoader.LoadIcon("recording_new", fontHeight, fontHeight);

  cImage *imgRecNewSml = NULL;
  if (Current)
    imgRecNewSml = imgLoader.LoadIcon("recording_new_cur", fontSmlHeight, fontSmlHeight);
  if (imgRecNewSml == NULL)
    imgRecNewSml = imgLoader.LoadIcon("recording_new", fontSmlHeight, fontSmlHeight);

  cImage *imgRecCut = NULL;
  if (Current)
    imgRecCut = imgLoader.LoadIcon("recording_cutted_cur", fontHeight, fontHeight);
  if (imgRecCut == NULL)
    imgRecCut = imgLoader.LoadIcon("recording_cutted", fontHeight, fontHeight);

  int Left = 0, Top = 0;
  Left = Config.decorBorderMenuItemSize + marginItem;
  Top = y;

  if (Config.MenuRecordingView == 1) {
    int LeftWidth = Left + fontHeight + imgRecNew->Width() + imgRecCut->Width() + marginItem * 3 +
                    font->Width("99.99.99  99:99   99:99 ");

    if (Total == 0) {
      if (Current)
        img = imgLoader.LoadIcon("recording_cur", fontHeight, fontHeight);
      if (img == NULL)
        img = imgLoader.LoadIcon("recording", fontHeight, fontHeight);
      if (img) {
        menuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
        Left += fontHeight + marginItem;
      }

      // int Minutes = std::max(0, (Recording->LengthInSeconds() + 30) / 60);
      int Minutes = (Recording->LengthInSeconds() + 30) / 60;
      cString Length = cString::sprintf("%02d:%02d", Minutes / 60, Minutes % 60);
      buffer = cString::sprintf("%s  %s   %s ", *ShortDateString(Recording->Start()), *TimeString(Recording->Start()),
                                *Length);

      menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, font, menuItemWidth - Left - marginItem);

      Left += font->Width(buffer);
      if (Recording->IsNew()) {
        if (imgRecNew) {
          menuIconsPixmap->DrawImage(cPoint(Left, Top), *imgRecNew);
        }
      }
#if APIVERSNUM >= 20108
      else {
        int FrameTotal = Recording->NumFrames();
        int FrameResume = Recording->GetResume();
        double FrameSeen = (double)FrameResume / (double)FrameTotal;
        cString SeenIcon("");

        double seenTreshold = Config.MenuItemRecordingSeenTreshold * 100.0;
        // dsyslog("Config.MenuItemRecordingSeenTreshold: %.2f\n", seenTreshold );

        if (FrameSeen < 0.1)
          SeenIcon = "recording_seen_0";
        else if (FrameSeen < 0.2)
          SeenIcon = "recording_seen_1";
        else if (FrameSeen < 0.3)
          SeenIcon = "recording_seen_2";
        else if (FrameSeen < 0.4)
          SeenIcon = "recording_seen_3";
        else if (FrameSeen < 0.5)
          SeenIcon = "recording_seen_4";
        else if (FrameSeen < 0.6)
          SeenIcon = "recording_seen_5";
        else if (FrameSeen < 0.7)
          SeenIcon = "recording_seen_6";
        else if (FrameSeen < 0.8)
          SeenIcon = "recording_seen_7";
        else if (FrameSeen < 0.9)
          SeenIcon = "recording_seen_8";
        else if (FrameSeen < 0.98)
          SeenIcon = "recording_seen_9";
        else
          SeenIcon = "recording_seen_10";

        if (FrameSeen >= seenTreshold)
          SeenIcon = "recording_seen_10";

        cImage *imgSeen = NULL;
        if (Current) {
          cString SeenIconCur = cString::sprintf("%s_cur", *SeenIcon);
          imgSeen = imgLoader.LoadIcon(SeenIconCur, fontHeight, fontHeight);
        }
        if (imgSeen == NULL)
          imgSeen = imgLoader.LoadIcon(SeenIcon, fontHeight, fontHeight);
        if (imgSeen != NULL)
          menuIconsPixmap->DrawImage(cPoint(Left, Top), *imgSeen);
      }
#endif

      Left += imgRecNew->Width() + marginItem;
      if (Recording->IsEdited()) {
        if (imgRecCut) {
          menuIconsPixmap->DrawImage(cPoint(Left, Top), *imgRecCut);
        }
      }
#if APIVERSNUM >= 20505
      if (Config.MenuItemRecordingShowRecordingErrors) {
        const cRecordingInfo *recInfo = Recording->Info();
        int RecErrIconTreshold = Config.MenuItemRecordingShowRecordingErrorsTreshold;

        cString RecErrIcon("recording_untested");
        if (recInfo->Errors() < 0) {         // -1 Untestet recording
          // RecErrIcon = "recording_untested";
        } else if (recInfo->Errors() == 0)    // No errors
          RecErrIcon = "recording_ok";
        else if (recInfo->Errors() < RecErrIconTreshold)
          RecErrIcon = "recording_warning";
        else if (recInfo->Errors() >= RecErrIconTreshold)
          RecErrIcon = "recording_error";

        cImage *imgRecErr = NULL;
        if (Current) {
          cString RecErrIconCur = cString::sprintf("%s_cur", *RecErrIcon);
          imgRecErr = imgLoader.LoadIcon(RecErrIconCur, fontHeight, fontHeight);
        }
        if (imgRecErr == NULL)
          imgRecErr = imgLoader.LoadIcon(RecErrIcon, fontHeight, fontHeight);
        if (imgRecErr != NULL)
          menuIconsOVLPixmap->DrawImage(cPoint(Left, Top), *imgRecErr);
      }  // MenuItemRecordingShowRecordingErrors
#endif

      Left += imgRecCut->Width() + marginItem;

      if (Current && font->Width(RecName) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
        menuItemScroller.AddScroller(RecName, cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight),
                                     ColorFg, clrTransparent, font);
      } else {
        menuPixmap->DrawText(cPoint(Left, Top), RecName, ColorFg, ColorBg, font, menuItemWidth - Left - marginItem);
      }
    } else if (Total > 0) {
      if (Current)
        img = imgLoader.LoadIcon("folder_cur", fontHeight, fontHeight);
      if (img == NULL)
        img = imgLoader.LoadIcon("folder", fontHeight, fontHeight);
      if (img) {
        menuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
        Left += img->Width() + marginItem;
      }

      buffer = cString::sprintf("%d  ", Total);
      menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, font, font->Width("  999"), fontHeight, taLeft);
      Left += font->Width("  999 ");

      if (imgRecNew)
        menuIconsPixmap->DrawImage(cPoint(Left, Top), *imgRecNew);

      Left += imgRecNew->Width() + marginItem;
      buffer = cString::sprintf("%d", New);
      menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, font, menuItemWidth - Left - marginItem);
      Left += font->Width(" 999 ");
      if (Config.MenuItemRecordingShowFolderDate != 0) {
        buffer = cString::sprintf("(%s) ", *ShortDateString(GetLastRecTimeFromFolder(Recording, Level)));
        menuPixmap->DrawText(cPoint(LeftWidth - font->Width(buffer) - fontHeight * 2 - marginItem * 2, Top), buffer,
                             ColorExtraTextFg, ColorBg, font);
        if (isRecordingOld(Recording, Level)) {
          Left = LeftWidth - fontHeight * 2 - marginItem * 2;
          if (Current)
            img = imgLoader.LoadIcon("recording_old_cur", fontHeight, fontHeight);
          else
            img = imgLoader.LoadIcon("recording_old", fontHeight, fontHeight);
          if (img) {
            menuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
            Left += img->Width() + marginItem;
          }
        }
      }

      if (Current && font->Width(RecName) > (menuItemWidth - LeftWidth - marginItem) && Config.ScrollerEnable) {
        menuItemScroller.AddScroller(
            RecName, cRect(LeftWidth, Top + menuTop, menuItemWidth - LeftWidth - marginItem, fontHeight), ColorFg,
            clrTransparent, font);
      } else {
        menuPixmap->DrawText(cPoint(LeftWidth, Top), RecName, ColorFg, ColorBg, font,
                             menuItemWidth - LeftWidth - marginItem);
      }
      LeftWidth += font->Width(RecName) + marginItem * 2;
    } else if (Total == -1) {
      if (Current)
        img = imgLoader.LoadIcon("folder_cur", fontHeight, fontHeight);
      if (img == NULL)
        img = imgLoader.LoadIcon("folder", fontHeight, fontHeight);
      if (img) {
        menuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
        Left += img->Width() + marginItem;
      }

      if (Current && font->Width(Recording->FileName()) > (menuItemWidth - Left - marginItem) &&
          Config.ScrollerEnable) {
        menuItemScroller.AddScroller(Recording->FileName(),
                                     cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight), ColorFg,
                                     clrTransparent, font);
      } else {
        menuPixmap->DrawText(cPoint(Left, Top), Recording->FileName(), ColorFg, ColorBg, font,
                             menuItemWidth - Left - marginItem);
      }
    }
  } else {
    if (Total == 0) {
      if (Current)
        img = imgLoader.LoadIcon("recording_cur", fontHeight, fontHeight);
      if (img == NULL)
        img = imgLoader.LoadIcon("recording", fontHeight, fontHeight);
      if (img) {
        menuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
        Left += fontHeight + marginItem;
      }
      int ImagesWidth = imgRecNew->Width() + imgRecCut->Width() + marginItem * 2 + scrollBarWidth;
      if (isScrolling)
        ImagesWidth -= scrollBarWidth;

      if (Current && font->Width(RecName) > (menuItemWidth - Left - marginItem - ImagesWidth) &&
          Config.ScrollerEnable) {
        menuItemScroller.AddScroller(
            RecName, cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem - ImagesWidth, fontHeight), ColorFg,
            clrTransparent, font);
      } else {
        menuPixmap->DrawText(cPoint(Left, Top), RecName, ColorFg, ColorBg, font,
                             menuItemWidth - Left - marginItem - ImagesWidth);
      }

      Top += fontHeight;

      int Minutes = (Recording->LengthInSeconds() + 30) / 60;
      cString Length = cString::sprintf("%02d:%02d", Minutes / 60, Minutes % 60);
      buffer = cString::sprintf("%s  %s   %s ", *ShortDateString(Recording->Start()), *TimeString(Recording->Start()),
                                *Length);

      menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, fontSml, menuItemWidth - Left - marginItem);

      Top -= fontHeight;
      Left = menuItemWidth - ImagesWidth;
      if (Recording->IsNew()) {
        if (imgRecNew) {
          menuIconsPixmap->DrawImage(cPoint(Left, Top), *imgRecNew);
        }
      }
#if APIVERSNUM >= 20108
      else {
        int FrameTotal = Recording->NumFrames();
        int FrameResume = Recording->GetResume();
        double FrameSeen = (double)FrameResume / (double)FrameTotal;
        cString SeenIcon("");

        if (FrameSeen < 0.1)
          SeenIcon = "recording_seen_0";
        else if (FrameSeen < 0.2)
          SeenIcon = "recording_seen_1";
        else if (FrameSeen < 0.3)
          SeenIcon = "recording_seen_2";
        else if (FrameSeen < 0.4)
          SeenIcon = "recording_seen_3";
        else if (FrameSeen < 0.5)
          SeenIcon = "recording_seen_4";
        else if (FrameSeen < 0.6)
          SeenIcon = "recording_seen_5";
        else if (FrameSeen < 0.7)
          SeenIcon = "recording_seen_6";
        else if (FrameSeen < 0.8)
          SeenIcon = "recording_seen_7";
        else if (FrameSeen < 0.9)
          SeenIcon = "recording_seen_8";
        else if (FrameSeen < 0.98)
          SeenIcon = "recording_seen_9";
        else
          SeenIcon = "recording_seen_10";

        cImage *imgSeen = NULL;
        if (Current) {
          cString SeenIconCur = cString::sprintf("%s_cur", *SeenIcon);
          imgSeen = imgLoader.LoadIcon(SeenIconCur, fontHeight, fontHeight);
        }
        if (imgSeen == NULL)
          imgSeen = imgLoader.LoadIcon(SeenIcon, fontHeight, fontHeight);
        if (imgSeen != NULL)
          menuIconsPixmap->DrawImage(cPoint(Left, Top), *imgSeen);
      }
#endif

      Left += imgRecNew->Width() + marginItem;
      if (Recording->IsEdited()) {
        if (imgRecCut) {
          menuIconsPixmap->DrawImage(cPoint(Left, Top), *imgRecCut);
        }
      }

#if APIVERSNUM >= 20505
      if (Config.MenuItemRecordingShowRecordingErrors) {
        const cRecordingInfo *recInfo = Recording->Info();
        int RecErrIconTreshold = Config.MenuItemRecordingShowRecordingErrorsTreshold;

        cString RecErrIcon("recording_untested");
        if (recInfo->Errors() < 0) {         // -1 Untestet recording
          // RecErrIcon = "recording_untested";
        } else if (recInfo->Errors() == 0)    // No errors
          RecErrIcon = "recording_ok";
        else if (recInfo->Errors() < RecErrIconTreshold)
          RecErrIcon = "recording_warning";
        else if (recInfo->Errors() >= RecErrIconTreshold)
          RecErrIcon = "recording_error";

        cImage *imgRecErr = NULL;
        if (Current) {
          cString RecErrIconCur = cString::sprintf("%s_cur", *RecErrIcon);
          imgRecErr = imgLoader.LoadIcon(RecErrIconCur, fontHeight, fontHeight);
        }
        if (imgRecErr == NULL)
          imgRecErr = imgLoader.LoadIcon(RecErrIcon, fontHeight, fontHeight);
        if (imgRecErr != NULL)
          menuIconsOVLPixmap->DrawImage(cPoint(Left, Top), *imgRecErr);
      }  // MenuItemRecordingShowRecordingErrors
#endif

      Left += imgRecCut->Width() + marginItem;

    } else if (Total > 0) {
      if (Current)
        img = imgLoader.LoadIcon("folder_cur", fontHeight, fontHeight);
      if (img == NULL)
        img = imgLoader.LoadIcon("folder", fontHeight, fontHeight);
      if (img) {
        menuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
        Left += img->Width() + marginItem;
      }

      if (Current && font->Width(RecName) > (menuItemWidth - Left - marginItem) && Config.ScrollerEnable) {
        menuItemScroller.AddScroller(RecName, cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight),
                                     ColorFg, clrTransparent, font);
      } else {
        menuPixmap->DrawText(cPoint(Left, Top), RecName, ColorFg, ColorBg, font, menuItemWidth - Left - marginItem);
      }

      Top += fontHeight;
      buffer = cString::sprintf("  %d", Total);
      menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, fontSml, fontSml->Width("  9999"),
                           fontSmlHeight, taRight);
      Left += fontSml->Width("  9999 ");

      if (imgRecNewSml)
        menuIconsPixmap->DrawImage(cPoint(Left, Top), *imgRecNewSml);

      Left += imgRecNewSml->Width() + marginItem;
      buffer = cString::sprintf("%d", New);
      menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorFg, ColorBg, fontSml, menuItemWidth - Left - marginItem);
      Left += fontSml->Width(" 999 ");

      if (Config.MenuItemRecordingShowFolderDate != 0) {
        buffer = cString::sprintf("  (%s) ", *ShortDateString(GetLastRecTimeFromFolder(Recording, Level)));
        menuPixmap->DrawText(cPoint(Left, Top), buffer, ColorExtraTextFg, ColorBg, fontSml);
        if (isRecordingOld(Recording, Level)) {
          Left += fontSml->Width(buffer);
          if (Current)
            img = imgLoader.LoadIcon("recording_old_cur", fontSmlHeight, fontSmlHeight);
          else
            img = imgLoader.LoadIcon("recording_old", fontSmlHeight, fontSmlHeight);
          if (img) {
            menuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
          }
        }
      }
    } else if (Total == -1) {
      if (Current)
        img = imgLoader.LoadIcon("folder_cur", fontHeight, fontHeight);
      if (img == NULL)
        img = imgLoader.LoadIcon("folder", fontHeight, fontHeight);
      if (img) {
        menuIconsPixmap->DrawImage(cPoint(Left, Top), *img);
        Left += img->Width() + marginItem;
      }
      if (Current && font->Width(Recording->FileName()) > (menuItemWidth - Left - marginItem) &&
          Config.ScrollerEnable) {
        menuItemScroller.AddScroller(Recording->FileName(),
                                     cRect(Left, Top + menuTop, menuItemWidth - Left - marginItem, fontHeight), ColorFg,
                                     clrTransparent, font);
      } else {
        menuPixmap->DrawText(cPoint(Left, Top), Recording->FileName(), ColorFg, ColorBg, font,
                             menuItemWidth - Left - marginItem);
      }
    }
  }

  sDecorBorder ib;
  ib.Left = Config.decorBorderMenuItemSize;
  ib.Top = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuItemSize + y;

  ib.Width = menuItemWidth - Config.decorBorderMenuItemSize * 2;

  if (isScrolling)
    ib.Width -= scrollBarWidth;

  ib.Width = menuItemWidth;

  ib.Height = Height;
  ib.Size = Config.decorBorderMenuItemSize;
  ib.Type = Config.decorBorderMenuItemType;

  if (Current) {
    ib.ColorFg = Config.decorBorderMenuItemCurFg;
    ib.ColorBg = Config.decorBorderMenuItemCurBg;
  } else {
    if (Selectable) {
      ib.ColorFg = Config.decorBorderMenuItemSelFg;
      ib.ColorBg = Config.decorBorderMenuItemSelBg;
    } else {
      ib.ColorFg = Config.decorBorderMenuItemFg;
      ib.ColorBg = Config.decorBorderMenuItemBg;
    }
  }

  DecorBorderDraw(ib.Left, ib.Top, ib.Width, ib.Height, ib.Size, ib.Type, ib.ColorFg, ib.ColorBg, BorderMenuItem);

  if (!isScrolling)
    ItemBorderInsertUnique(ib);

  if (Config.MenuRecordingView == 3 && Current)
    DrawItemExtraRecording(Recording, tr("no recording info"));

  return true;
}

void cFlatDisplayMenu::SetEvent(const cEvent *Event) {
  if (!Event)
    return;

#ifdef DEBUGEPGTIME
  uint32_t tick0 = GetMsTicks();
#endif

  ShowEvent = true;
  ShowRecording = false;
  ShowText = false;
  ItemBorderClear();

  cLeft = Config.decorBorderMenuContentSize;
  cTop = chTop + marginItem * 3 + fontHeight + fontSmlHeight * 2 + Config.decorBorderMenuContentSize +
         Config.decorBorderMenuContentHeadSize;
  cWidth = menuWidth - Config.decorBorderMenuContentSize * 2;
  cHeight = osdHeight - (topBarHeight + Config.decorBorderTopBarSize * 2 + buttonsHeight +
                         Config.decorBorderButtonSize * 2 + marginItem * 3 + chHeight +
                         Config.decorBorderMenuContentHeadSize * 2 + Config.decorBorderMenuContentSize * 2);

  if (!ButtonsDrawn())
    cHeight += buttonsHeight + Config.decorBorderButtonSize * 2;

  menuItemWidth = cWidth;

  contentHeadPixmap->Fill(clrTransparent);
  contentHeadPixmap->DrawRectangle(cRect(0, 0, menuWidth, fontHeight + fontSmlHeight * 2 + marginItem * 2),
                                   Theme.Color(clrScrollbarBg));
  contentHeadIconsPixmap->Fill(clrTransparent);

  cString date = Event->GetDateString();
  cString startTime = Event->GetTimeString();
  cString endTime = Event->GetEndTimeString();

  cString timeString = cString::sprintf("%s %s - %s", *date, *startTime, *endTime);

  cString title = Event->Title();
  cString shortText = Event->ShortText();

  contentHeadPixmap->DrawText(cPoint(marginItem, marginItem), timeString, Theme.Color(clrMenuEventFontInfo),
                              Theme.Color(clrMenuEventBg), fontSml, menuWidth - marginItem * 2);
  contentHeadPixmap->DrawText(cPoint(marginItem, marginItem + fontSmlHeight), title, Theme.Color(clrMenuEventFontTitle),
                              Theme.Color(clrMenuEventBg), font, menuWidth - marginItem * 2);
  contentHeadPixmap->DrawText(cPoint(marginItem, marginItem + fontSmlHeight + fontHeight), shortText,
                              Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                              menuWidth - marginItem * 2);

  DecorBorderDraw(chLeft, chTop, chWidth, chHeight, Config.decorBorderMenuContentHeadSize,
                  Config.decorBorderMenuContentHeadType, Config.decorBorderMenuContentHeadFg,
                  Config.decorBorderMenuContentHeadBg);

  // Description
  ostringstream text(""), textAdditional("");
  std::string Fsk("");
  std::list<std::string> GenreIcons;

  if (!isempty(Event->Description())) {
    text << Event->Description();
  }

  if (Config.EpgAdditionalInfoShow) {
    text << "\n";
    // Genre
    bool firstContent = true;
    for (int i = 0; Event->Contents(i); i++) {
      if (!isempty(Event->ContentToString(Event->Contents(i)))) { // skip empty (user defined) content
        if (!firstContent) {
          text << ", ";
        } else {
          text << "\n" << tr("Genre") << ": ";
        }
        text << Event->ContentToString(Event->Contents(i));
        firstContent = false;
        GenreIcons.push_back(GetGenreIcon(Event->Contents(i)));
      }
    }
    // FSK
    if (Event->ParentalRating()) {
      Fsk = *Event->GetParentalRatingString();
      text << "\n" << tr("FSK") << ": " << Fsk;

    }
    const cComponents *Components = Event->Components();
    if (Components) {
      ostringstream audio("");
      bool firstAudio = true;
      const char *audio_type = NULL;
      ostringstream subtitle("");
      bool firstSubtitle = true;
      for (int i = 0; i < Components->NumComponents(); i++) {
        const tComponent *p = Components->Component(i);
        switch (p->stream) {
        case sc_video_MPEG2:
          if (p->description)
            textAdditional << tr("Video") << ": " << p->description << " (MPEG2)";
          else
            textAdditional << tr("Video") << ": MPEG2";
          break;
        case sc_video_H264_AVC:
          if (p->description)
            textAdditional << tr("Video") << ": " << p->description << " (H.264)";
          else
            textAdditional << tr("Video") << ": H.264";
          break;
        case sc_video_H265_HEVC:  //might be not always correct because stream_content_ext (must be 0x0) is not available in tComponent
          if (p->description)
            textAdditional << tr("Video") << ": " << p->description << " (H.265)";
          else
            textAdditional << tr("Video") << ": H.265";
          break;
        case sc_audio_MP2:
        case sc_audio_AC3:
        case sc_audio_HEAAC:
          if (firstAudio)
            firstAudio = false;
          else
            audio << ", ";
          switch (p->stream) {
          case sc_audio_MP2:
            // workaround for wrongfully used stream type X 02 05 for AC3
            if (p->type == 5)
              audio_type = "AC3";
            else
              audio_type = "MP2";
            break;
          case sc_audio_AC3:
            audio_type = "AC3";
            break;
          case sc_audio_HEAAC:
            audio_type = "HEAAC";
            break;
          }
          if (p->description)
            audio << p->description << " (" << audio_type << ", " << p->language << ")";
          else
            audio << p->language << " (" << audio_type << ")";
          break;
        case sc_subtitle:
          if (firstSubtitle)
            firstSubtitle = false;
          else
            subtitle << ", ";
          if (p->description)
            subtitle << p->description << " (" << p->language << ")";
          else
            subtitle << p->language;
          break;
        }
      }
      if (audio.str().length() > 0) {
        if (textAdditional.str().length() > 0)
          textAdditional << "\n";
        textAdditional << tr("Audio") << ": " << audio.str();
      }
      if (subtitle.str().length() > 0) {
        if (textAdditional.str().length() > 0)
          textAdditional << "\n";
        textAdditional << "\n" << tr("Subtitle") << ": " << subtitle.str();
      }
    }
  }

  int headIconTop = chHeight - fontHeight - marginItem;
  int headIconLeft = chWidth - fontHeight - marginItem;

  if (Fsk.length() > 0) {
    cString iconName = cString::sprintf("EPGInfo/FSK/%s", Fsk.c_str());
    cImage *img = imgLoader.LoadIcon(*iconName, fontHeight, fontHeight);
    if (img) {
      contentHeadIconsPixmap->DrawImage(cPoint(headIconLeft, headIconTop), *img);
      headIconLeft -= fontHeight + marginItem;
    } else {
      isyslog("skinflatplus: FSK icon not found: %s", *iconName);
      img = imgLoader.LoadIcon("EPGInfo/FSK/unknown", fontHeight, fontHeight);
      if (img) {
        contentHeadIconsPixmap->DrawImage(cPoint(headIconLeft, headIconTop), *img);
        headIconLeft -= fontHeight + marginItem;
      }
    }
  }
  while (!GenreIcons.empty()) {
    GenreIcons.sort();
    GenreIcons.unique();
    cString iconName = cString::sprintf("EPGInfo/Genre/%s", GenreIcons.back().c_str());
    cImage *img = imgLoader.LoadIcon(*iconName, fontHeight, fontHeight);
    bool isUnknownDrawn = false;
    if (img) {
      contentHeadIconsPixmap->DrawImage(cPoint(headIconLeft, headIconTop), *img);
      headIconLeft -= fontHeight + marginItem;
    } else {
      isyslog("skinflatplus: Genre icon not found: %s", *iconName);
      if (!isUnknownDrawn) {
        img = imgLoader.LoadIcon("EPGInfo/Genre/unknown", fontHeight, fontHeight);
        if (img) {
          contentHeadIconsPixmap->DrawImage(cPoint(headIconLeft, headIconTop), *img);
          headIconLeft -= fontHeight + marginItem;
          isUnknownDrawn = true;
        }
      }
    }
    GenreIcons.pop_back();
  }

#ifdef DEBUGEPGTIME
  uint32_t tick1 = GetMsTicks();
  dsyslog("SetEvent info-text time: %d ms", tick1 - tick0);
#endif

  std::ostringstream sstrReruns("");
  if (Config.EpgRerunsShow) {
    // lent from nopacity
    cPlugin *epgSearchPlugin = cPluginManager::GetPlugin("epgsearch");
    if (epgSearchPlugin && !isempty(Event->Title())) {
      Epgsearch_searchresults_v1_0 data;
      std::string strQuery = Event->Title();
      data.useSubTitle = false;

      data.query = (char *)strQuery.c_str();
      data.mode = 0;
      data.channelNr = 0;
      data.useTitle = true;
      data.useDescription = false;

      if (epgSearchPlugin->Service("Epgsearch-searchresults-v1.0", &data)) {
        cList<Epgsearch_searchresults_v1_0::cServiceSearchResult> *list = data.pResultList;
        if (list && (list->Count() > 1)) {
          int i = 0;
          for (Epgsearch_searchresults_v1_0::cServiceSearchResult *r = list->First(); r && i < 5; r = list->Next(r)) {
            if ((Event->ChannelID() == r->event->ChannelID()) && (Event->StartTime() == r->event->StartTime()))
              continue;
            i++;
            sstrReruns << *DayDateTime(r->event->StartTime());
#if VDRVERSNUM >= 20301
            LOCK_CHANNELS_READ;
            const cChannel *channel = Channels->GetByChannelID(r->event->ChannelID(), true, true);
#else
            cChannel *channel = Channels.GetByChannelID(r->event->ChannelID(), true, true);
#endif
            if (channel) {
              sstrReruns << ", " << channel->Number() << " -";
              sstrReruns << " " << channel->ShortName(true);
            }
            sstrReruns << ":  " << r->event->Title();
            // if (!isempty(r->event->ShortText()))
            //    sstrReruns << "~" << r->event->ShortText();
            sstrReruns << "\n";
          }
          delete list;
        }
      }
    }
  }
#ifdef DEBUGEPGTIME
  uint32_t tick2 = GetMsTicks();
  dsyslog("SetEvent reruns time: %d ms", tick2 - tick1);
#endif

  bool Scrollable = false;
  bool FirstRun = true;

  do {
    if (Scrollable) {
      FirstRun = false;
      cWidth -= scrollBarWidth;
    }

    ComplexContent.Clear();
    ComplexContent.SetOsd(osd);
    ComplexContent.SetPosition(cRect(cLeft, cTop, cWidth, cHeight));
    ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));
    ComplexContent.SetScrollSize(fontHeight);
    ComplexContent.SetScrollingActive(true);

    int ContentTop = marginItem;

    ostringstream series_info(""), movie_info("");

    std::vector<std::string> actors_path;
    std::vector<std::string> actors_name;
    std::vector<std::string> actors_role;

    std::string mediaPath("");
    int mediaWidth = 0;
    int mediaHeight = 0;

#ifdef DEBUGEPGTIME
    uint32_t tick3 = GetMsTicks();
#endif

    // first try scraper2vdr
    static cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
    if (!pScraper) // if it doesn't exit, try tvscraper
      pScraper = cPluginManager::GetPlugin("tvscraper");
    if ((Config.TVScraperEPGInfoShowPoster || Config.TVScraperEPGInfoShowActors) && pScraper) {
      ScraperGetEventType call;
      call.event = Event;
      int seriesId = 0;
      int episodeId = 0;
      int movieId = 0;

      if (pScraper->Service("GetEventType", &call)) {
        seriesId = call.seriesId;
        episodeId = call.episodeId;
        movieId = call.movieId;
      }
      if (seriesId > 0) {
        cSeries series;
        series.seriesId = seriesId;
        series.episodeId = episodeId;
        if (pScraper->Service("GetSeries", &series)) {
          if (series.banners.size() > 0)
            mediaPath = series.banners[0].path;
          mediaWidth = cWidth / 2 - marginItem * 2;
          mediaHeight = cHeight - marginItem * 2 - fontHeight - 6;
          if (Config.TVScraperEPGInfoShowActors) {
            for (unsigned int i = 0; i < series.actors.size(); i++) {
              if (imgLoader.FileExits(series.actors[i].actorThumb.path)) {
                actors_path.push_back(series.actors[i].actorThumb.path);
                actors_name.push_back(series.actors[i].name);
                actors_role.push_back(series.actors[i].role);
              }
            }
          }
          if (series.name.length() > 0)
            series_info << tr("name: ") << series.name << "\n";
          if (series.firstAired.length() > 0)
            series_info << tr("first aired: ") << series.firstAired << "\n";
          if (series.network.length() > 0)
            series_info << tr("network: ") << series.network << "\n";
          if (series.genre.length() > 0)
            series_info << tr("genre: ") << series.genre << "\n";
          if (series.rating > 0)
            series_info << tr("rating: ") << series.rating << "\n";
          if (series.status.length() > 0)
            series_info << tr("status: ") << series.status << "\n";
          if (series.episode.season > 0)
            series_info << tr("season number: ") << series.episode.season << "\n";
          if (series.episode.number > 0)
            series_info << tr("episode number: ") << series.episode.number << "\n";
        }
      } else if (movieId > 0) {
        cMovie movie;
        movie.movieId = movieId;
        if (pScraper->Service("GetMovie", &movie)) {
          mediaPath = movie.poster.path;
          mediaWidth = cWidth / 2 - marginItem * 3;
          mediaHeight = cHeight - marginItem * 2 - fontHeight - 6;
          if (Config.TVScraperEPGInfoShowActors) {
            for (unsigned int i = 0; i < movie.actors.size(); i++) {
              if (imgLoader.FileExits(movie.actors[i].actorThumb.path)) {
                actors_path.push_back(movie.actors[i].actorThumb.path);
                actors_name.push_back(movie.actors[i].name);
                actors_role.push_back(movie.actors[i].role);
              }
            }
          }
          if (movie.title.length() > 0)
            movie_info << tr("title: ") << movie.title << "\n";
          if (movie.originalTitle.length() > 0)
            movie_info << tr("original title: ") << movie.originalTitle << "\n";
          if (movie.collectionName.length() > 0)
            movie_info << tr("collection name: ") << movie.collectionName << "\n";
          if (movie.genres.length() > 0)
            movie_info << tr("genre: ") << movie.genres << "\n";
          if (movie.releaseDate.length() > 0)
            movie_info << tr("release date: ") << movie.releaseDate << "\n";
          if (movie.popularity > 0)
            movie_info << tr("popularity: ") << movie.popularity << "\n";
          if (movie.voteAverage > 0)
            movie_info << tr("vote average: ") << movie.voteAverage << "\n";
        }
      }
    }

#ifdef DEBUGEPGTIME
    uint32_t tick4 = GetMsTicks();
    dsyslog("SetEvent tvscraper time: %d ms", tick4 - tick3);
#endif

    if (mediaPath.length() > 0) {
      cImage *img = imgLoader.LoadFile(mediaPath.c_str(), mediaWidth, mediaHeight);
      if (img) {
        ComplexContent.AddText(tr("Description"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                               Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
        ContentTop += fontHeight;
        ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuEventTitleLine));
        ContentTop += 6;
        ComplexContent.AddImageWithFloatedText(
            img, CIP_Right, text.str().c_str(),
            cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), font);
      } else if (text.str().length() > 0) {
        ComplexContent.AddText(tr("Description"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                               Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
        ContentTop += fontHeight;
        ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuEventTitleLine));
        ContentTop += 6;
        ComplexContent.AddText(text.str().c_str(), true,
                               cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                               Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), font);
      }
    } else if (text.str().length() > 0) {
      ComplexContent.AddText(tr("Description"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuEventTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(text.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), font);
    }

    if (movie_info.str().length() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Movie information"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuEventTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(movie_info.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), font);
    }

    if (series_info.str().length() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Series information"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuEventTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(series_info.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), font);
    }
#ifdef DEBUGEPGTIME
    uint32_t tick5 = GetMsTicks();
    dsyslog("SetEvent epg-text time: %d ms", tick5 - tick4);
#endif

    if (Config.TVScraperEPGInfoShowActors && actors_path.size() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Actors"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuEventTitleLine));
      ContentTop += 6;

      int actorsPerLine = 6;
      int numActors = actors_path.size();
      int actorWidth = cWidth / actorsPerLine - marginItem * 4;
      int picsPerLine = (cWidth - marginItem * 2) / actorWidth;
      int picLines = numActors / picsPerLine;
      if (numActors % picsPerLine != 0)
        picLines++;
      int actorMargin = ((cWidth - marginItem * 2) - actorWidth * actorsPerLine) / (actorsPerLine - 1);
      int x = marginItem;
      int y = ContentTop;
      int actor = 0;
      for (int row = 0; row < picLines; row++) {
        for (int col = 0; col < picsPerLine; col++) {
          if (actor == numActors)
            break;
          std::string path = actors_path[actor];
          cImage *img = imgLoader.LoadFile(path.c_str(), actorWidth, 999);
          if (img) {
            ComplexContent.AddImage(img, cRect(x, y, 0, 0));
            std::string name = actors_name[actor];
            std::stringstream sstrRole("");
            if (actors_role[actor].length() > 0)
              sstrRole << "\"" << actors_role[actor] << "\"";
            std::string role = sstrRole.str();
            ComplexContent.AddText(name.c_str(), false, cRect(x, y + img->Height() + marginItem, actorWidth, 0),
                                   Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, actorWidth,
                                   fontSmlHeight, taCenter);
            ComplexContent.AddText(role.c_str(), false,
                                   cRect(x, y + img->Height() + marginItem + fontSmlHeight, actorWidth, 0),
                                   Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, actorWidth,
                                   fontSmlHeight, taCenter);
          }
          x += actorWidth + actorMargin;
          actor++;
        }
        x = marginItem;
        y = ComplexContent.BottomContent() + fontHeight;
      }
    }
#ifdef DEBUGEPGTIME
    uint32_t tick6 = GetMsTicks();
    dsyslog("SetEvent actor time: %d ms", tick6 - tick5);
#endif

    if (sstrReruns.str().length() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Reruns"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuEventTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(sstrReruns.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), font);
    }

    if (textAdditional.str().length() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Video information"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuEventTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(textAdditional.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), font);
    }
    Scrollable = ComplexContent.Scrollable(cHeight - marginItem * 2);
  } while (Scrollable && FirstRun);

  if (Config.MenuContentFullSize || Scrollable) {
    ComplexContent.CreatePixmaps(true);
  } else
    ComplexContent.CreatePixmaps(false);

  ComplexContent.Draw();

  if (Scrollable)
    DrawScrollbar(ComplexContent.ScrollTotal(), ComplexContent.ScrollOffset(), ComplexContent.ScrollShown(),
                  ComplexContent.Top() - scrollBarTop, ComplexContent.Height(), ComplexContent.ScrollOffset() > 0,
                  ComplexContent.ScrollOffset() + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);

  if (Config.MenuContentFullSize || Scrollable)
    DecorBorderDraw(cLeft, cTop, cWidth, ComplexContent.ContentHeight(true), Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg,
                    Config.decorBorderMenuContentBg);
  else
    DecorBorderDraw(cLeft, cTop, cWidth, ComplexContent.ContentHeight(false), Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg,
                    Config.decorBorderMenuContentBg);

#ifdef DEBUGEPGTIME
  uint32_t tick7 = GetMsTicks();
  dsyslog("SetEvent total time: %d ms", tick7 - tick0);
  // dsyslog("SetEvent time: %d", tick7);
#endif
}

void cFlatDisplayMenu::DrawItemExtraRecording(const cRecording *Recording, cString EmptyText) {
  cLeft = menuItemWidth + Config.decorBorderMenuItemSize * 2 + Config.decorBorderMenuContentSize + marginItem;
  if (isScrolling)
    cLeft += scrollBarWidth;
  cTop = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
  cWidth = menuWidth - cLeft - Config.decorBorderMenuContentSize;
  cHeight = osdHeight - (topBarHeight + Config.decorBorderTopBarSize * 2 + buttonsHeight +
                         Config.decorBorderButtonSize * 2 + marginItem * 3 + Config.decorBorderMenuContentSize * 2);

  ostringstream text("");
  if (Recording) {
    const cRecordingInfo *recInfo = Recording->Info();
    text.imbue(std::locale(""));

    if (!isempty(recInfo->Description()))
      text << recInfo->Description() << "\n\n";

    // lent from skinelchi
    if (Config.RecordingAdditionalInfoShow) {
#if VDRVERSNUM >= 20301
      LOCK_CHANNELS_READ;
      const cChannel *channel = Channels->GetByChannelID(((cRecordingInfo *)recInfo)->ChannelID());
#else
      cChannel *channel = Channels.GetByChannelID(((cRecordingInfo *)recInfo)->ChannelID());
#endif
      if (channel)
        text << trVDR("Channel") << ": " << channel->Number() << " - " << channel->Name() << "\n";

      const cEvent *Event = recInfo->GetEvent();
      if (Event) {
        // Genre
        bool firstContent = true;
        for (int i = 0; Event->Contents(i); i++) {
          if (!isempty(Event->ContentToString(Event->Contents(i)))) { // skip empty (user defined) content
            if (!firstContent)
              text << ", ";
            else
              text << tr("Genre") << ": ";
            text << Event->ContentToString(Event->Contents(i));
            firstContent = false;
          }
        }
        if (Event->Contents(0))
          text << "\n";
        // FSK
        if (Event->ParentalRating()) {
          text << tr("FSK") << ": " << *Event->GetParentalRatingString() << "\n";
        }
      }

      cMarks marks;
      bool hasMarks =
          marks.Load(Recording->FileName(), Recording->FramesPerSecond(), Recording->IsPesRecording()) && marks.Count();
      cIndexFile *index = new cIndexFile(Recording->FileName(), false, Recording->IsPesRecording());

      int lastIndex = 0;

      int cuttedLength = 0;
      long cutinframe = 0;
      unsigned long long recsize = 0;
      unsigned long long recsizecutted = 0;
      unsigned long long cutinoffset = 0;
      unsigned long long filesize[100000];
      filesize[0] = 0;

      int i = 0;
      int imax = 999;
      struct stat filebuf;
      cString filename("");
      int rc = 0;

      do {
        if (Recording->IsPesRecording())
          filename = cString::sprintf("%s/%03d.vdr", Recording->FileName(), ++i);
        else {
          filename = cString::sprintf("%s/%05d.ts", Recording->FileName(), ++i);
          imax = 99999;
        }
        rc = stat(filename, &filebuf);
        if (rc == 0)
          filesize[i] = filesize[i - 1] + filebuf.st_size;
        else {
          if (ENOENT != errno) {
            esyslog("skinflatplus: error determining file size of \"%s\" %d (%s)", (const char *)filename, errno,
                    strerror(errno));
            recsize = 0;
          }
        }
      } while (i <= imax && !rc);
      recsize = filesize[i - 1];

      if (hasMarks && index) {
        uint16_t FileNumber;
        off_t FileOffset;

        bool cutin = true;
        cMark *mark = marks.First();
        while (mark) {
          long position = mark->Position();
          index->Get(position, &FileNumber, &FileOffset);
          if (cutin) {
            cutinframe = position;
            cutin = false;
            cutinoffset = filesize[FileNumber - 1] + FileOffset;
          } else {
            cuttedLength += position - cutinframe;
            cutin = true;
            recsizecutted += filesize[FileNumber - 1] + FileOffset - cutinoffset;
          }
          cMark *nextmark = marks.Next(mark);
          mark = nextmark;
        }
        if (!cutin) {
          cuttedLength += index->Last() - cutinframe;
          index->Get(index->Last() - 1, &FileNumber, &FileOffset);
          recsizecutted += filesize[FileNumber - 1] + FileOffset - cutinoffset;
        }
      }
      if (index) {
        lastIndex = index->Last();
        text << tr("Length") << ": " << *IndexToHMSF(lastIndex, false, Recording->FramesPerSecond());
        if (hasMarks)
          text << " (" << tr("cutted") << ": " << *IndexToHMSF(cuttedLength, false, Recording->FramesPerSecond())
               << ")";
        text << "\n";
      }
      delete index;

      if (recsize > MEGABYTE(1023))
        text << tr("Size") << ": " << fixed << setprecision(2) << (float)recsize / MEGABYTE(1024) << " GB";
      else
        text << tr("Size") << ": " << recsize / MEGABYTE(1) << " MB";
      if (hasMarks)
        if (recsize > MEGABYTE(1023))
          text << " (" << tr("cutted") << ": " << fixed << setprecision(2) << (float)recsizecutted / MEGABYTE(1024)
               << " GB)";
        else
          text << " (" << tr("cutted") << ": " << recsizecutted / MEGABYTE(1) << " MB)";

      text << "\n"
           << trVDR("Priority") << ": " << Recording->Priority() << ", " << trVDR("Lifetime") << ": "
           << Recording->Lifetime() << "\n";

      if (lastIndex) {
        text << tr("format") << ": " << (Recording->IsPesRecording() ? "PES" : "TS") << ", " << tr("bit rate") << ": ~ "
             << fixed << setprecision(2) << (float)recsize / lastIndex * Recording->FramesPerSecond() * 8 / MEGABYTE(1)
             << " MBit/s (Video + Audio)";
      }
      // From SkinNopacity
#if APIVERSNUM >= 20505
      if (recInfo) {
        if (recInfo->Errors() >= 1)
          text << tr("TS errors") << ": " << recInfo->Errors() << "\n";
      }
#endif
      const cComponents *Components = recInfo->Components();
      if (Components) {
        ostringstream audio("");
        bool firstAudio = true;
        const char *audio_type = NULL;
        ostringstream subtitle("");
        bool firstSubtitle = true;
        for (int i = 0; i < Components->NumComponents(); i++) {
          const tComponent *p = Components->Component(i);

          switch (p->stream) {
          case sc_video_MPEG2:
            text << "\n" << tr("Video") << ": " << p->description << " (MPEG2)";
            break;
          case sc_video_H264_AVC:
            text << "\n" << tr("Video") << ": " << p->description << " (H.264)";
            break;
          case sc_video_H265_HEVC:  //might be not always correct because stream_content_ext (must be 0x0) is not available in tComponent
            text << "\n" << tr("Video") << ": " << p->description << " (H.265)";
            break;
          case sc_audio_MP2:
          case sc_audio_AC3:
          case sc_audio_HEAAC:
            if (firstAudio)
              firstAudio = false;
            else
              audio << ", ";
            switch (p->stream) {
            case sc_audio_MP2:
              // workaround for wrongfully used stream type X 02 05 for AC3
              if (p->type == 5)
                audio_type = "AC3";
              else
                audio_type = "MP2";
              break;
            case sc_audio_AC3:
              audio_type = "AC3";
              break;
            case sc_audio_HEAAC:
              audio_type = "HEAAC";
              break;
            }
            if (p->description)
              audio << p->description << " (" << audio_type << ", " << p->language << ")";
            else
              audio << p->language << " (" << audio_type << ")";
            break;
          case sc_subtitle:
            if (firstSubtitle)
              firstSubtitle = false;
            else
              subtitle << ", ";
            if (p->description)
              subtitle << p->description << " (" << p->language << ")";
            else
              subtitle << p->language;
            break;
          }
        }
        if (audio.str().length() > 0)
          text << "\n" << tr("Audio") << ": " << audio.str();
        if (subtitle.str().length() > 0)
          text << "\n" << tr("Subtitle") << ": " << subtitle.str();
      }
      if (recInfo->Aux()) {
        string str_epgsearch = xml_substring(recInfo->Aux(), "<epgsearch>", "</epgsearch>");
        string channel(""), searchtimer(""), pattern("");
        if (!str_epgsearch.empty()) {
          channel = xml_substring(str_epgsearch, "<channel>", "</channel>");
          searchtimer = xml_substring(str_epgsearch, "<searchtimer>", "</searchtimer>");
          if (searchtimer.empty())
            searchtimer = xml_substring(str_epgsearch, "<Search timer>", "</Search timer>");
        }

        string str_tvscraper = xml_substring(recInfo->Aux(), "<tvscraper>", "</tvscraper>");
        string causedby(""), reason("");
        if (!str_tvscraper.empty()) {
          causedby = xml_substring(str_tvscraper, "<causedBy>", "</causedBy>");
          reason = xml_substring(str_tvscraper, "<reason>", "</reason>");
        }

        string str_vdradmin = xml_substring(recInfo->Aux(), "<vdradmin-am>", "</vdradmin-am>");
        if (!str_vdradmin.empty()) {
          pattern = xml_substring(str_vdradmin, "<pattern>", "</pattern>");
        }

        if ((!channel.empty() && !searchtimer.empty()) || (!causedby.empty() && !reason.empty()) || !pattern.empty()) {
          text << "\n\n" << tr("additional information") << ":";  // Zusatzinfos anzeigen
          if (!channel.empty() && !searchtimer.empty()) {
            text << "\nEPGsearch: " << tr("channel") << ": " << channel << ", " << tr("search pattern") << ": "
                 << searchtimer;
          }
          if (!causedby.empty() && !reason.empty()) {  // TVScraper
            text << "\nTVScraper: " << tr("caused by") << ": " << causedby << ", " << tr("reason") << ": "
                 << reason;
          }
          if (!pattern.empty()) {
            text << "\nVDRadmin-AM: " << tr("search pattern") << ": " << pattern;
          }
        }
      }
    }  // if Config.RecordingAdditionalInfoShow
  } else
    text << *EmptyText;

  ComplexContent.Clear();
  ComplexContent.SetScrollSize(fontSmlHeight);
  ComplexContent.SetScrollingActive(false);
  ComplexContent.SetOsd(osd);
  ComplexContent.SetPosition(cRect(cLeft, cTop, cWidth, cHeight));
  ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));

  std::string mediaPath("");
  int mediaWidth = 0;
  int mediaHeight = 0;
  int mediaType = 0;

  // first try scraper2vdr
  static cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
  if (!pScraper) // if it doesn't exit, try tvscraper
    pScraper = cPluginManager::GetPlugin("tvscraper");
  if (Config.TVScraperRecInfoShowPoster && pScraper) {
    ScraperGetEventType call;
    call.recording = Recording;
    int seriesId = 0;
    int episodeId = 0;
    int movieId = 0;

    if (pScraper->Service("GetEventType", &call)) {
      seriesId = call.seriesId;
      episodeId = call.episodeId;
      movieId = call.movieId;
    }
    if (seriesId > 0) {
      cSeries series;
      series.seriesId = seriesId;
      series.episodeId = episodeId;
      if (pScraper->Service("GetSeries", &series)) {
        if (series.banners.size() > 0)
          mediaPath = series.banners[0].path;
        mediaWidth = cWidth - marginItem * 2;
        mediaHeight = 999;
        mediaType = 1;
      }
    } else if (movieId > 0) {
      cMovie movie;
      movie.movieId = movieId;
      if (pScraper->Service("GetMovie", &movie)) {
        mediaPath = movie.poster.path;
        mediaWidth = cWidth / 2 - marginItem * 3;
        mediaHeight = 999;
        mediaType = 2;
      }
    }
  }

  cString recPath = cString::sprintf("%s", Recording->FileName());
  cString recImage("");
  if (imgLoader.SearchRecordingPoster(recPath, recImage)) {
    mediaWidth = cWidth / 2 - marginItem * 3;
    mediaHeight = 999;
    mediaType = 2;
    mediaPath = recImage;
  }

  if (mediaPath.length() > 0) {
    cImage *img = imgLoader.LoadFile(mediaPath.c_str(), mediaWidth, mediaHeight);
    if (img && mediaType == 2) {
      ComplexContent.AddImageWithFloatedText(
          img, CIP_Right, text.str().c_str(),
          cRect(marginItem, marginItem, cWidth - marginItem * 2, cHeight - marginItem * 2),
          Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), fontSml);
    } else if (img && mediaType == 1) {
      ComplexContent.AddImage(img, cRect(marginItem, marginItem, img->Width(), img->Height()));
      ComplexContent.AddText(
          text.str().c_str(), true,
          cRect(marginItem, marginItem + img->Height(), cWidth - marginItem * 2, cHeight - marginItem * 2),
          Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), fontSml);
    } else {
      ComplexContent.AddText(text.str().c_str(), true,
                             cRect(marginItem, marginItem, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), fontSml);
    }
  } else {
    ComplexContent.AddText(text.str().c_str(), true,
                           cRect(marginItem, marginItem, cWidth - marginItem * 2, cHeight - marginItem * 2),
                           Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), fontSml);
  }

  ComplexContent.CreatePixmaps(Config.MenuContentFullSize);
  ComplexContent.Draw();

  DecorBorderClearByFrom(BorderContent);
  if (Config.MenuContentFullSize)
    DecorBorderDraw(cLeft, cTop, cWidth, ComplexContent.ContentHeight(true), Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg,
                    BorderContent);
  else
    DecorBorderDraw(cLeft, cTop, cWidth, ComplexContent.ContentHeight(false), Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg,
                    BorderContent);
}

void cFlatDisplayMenu::SetRecording(const cRecording *Recording) {
  if (!Recording)
    return;

#ifdef DEBUGEPGTIME
  uint32_t tick0 = GetMsTicks();
#endif

  ShowEvent = false;
  ShowRecording = true;
  ShowText = false;
  ItemBorderClear();

  const cRecordingInfo *recInfo = Recording->Info();

  chLeft = Config.decorBorderMenuContentHeadSize;
  chTop = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentHeadSize;
  chWidth = menuWidth - Config.decorBorderMenuContentHeadSize * 2;
  chHeight = fontHeight + fontSmlHeight * 2 + marginItem * 2;
  contentHeadPixmap = CreatePixmap(1, cRect(chLeft, chTop, chWidth, chHeight));
  // dsyslog("skinflatplus: contentHeadPixmap left: %d top: %d width: %d height: %d", chLeft, chTop, chWidth, chHeight);

  contentHeadIconsPixmap->Fill(clrTransparent);

  cLeft = Config.decorBorderMenuContentSize;
  cTop = chTop + marginItem * 3 + fontHeight + fontSmlHeight * 2 + Config.decorBorderMenuContentSize +
         Config.decorBorderMenuContentHeadSize;
  cWidth = menuWidth - Config.decorBorderMenuContentSize * 2;
  cHeight = osdHeight - (topBarHeight + Config.decorBorderTopBarSize * 2 + buttonsHeight +
                         Config.decorBorderButtonSize * 2 + marginItem * 3 + chHeight +
                         Config.decorBorderMenuContentHeadSize * 2 + Config.decorBorderMenuContentSize * 2);

  if (!ButtonsDrawn())
    cHeight += buttonsHeight + Config.decorBorderButtonSize * 2;

  menuItemWidth = cWidth;

  ostringstream text(""), textAdditional(""), recAdditional("");
  text.imbue(std::locale(""));
  textAdditional.imbue(std::locale(""));
  recAdditional.imbue(std::locale(""));

  std::string Fsk("");
  std::list<std::string> GenreIcons;

  if (!isempty(recInfo->Description()))
    text << recInfo->Description() << "\n\n";

  // lent from skinelchi
  if (Config.RecordingAdditionalInfoShow) {
#if VDRVERSNUM >= 20301
    auto channelFuture = std::async(
        [&recAdditional](tChannelID channelId) {
          LOCK_CHANNELS_READ;
          const cChannel *channel = Channels->GetByChannelID(channelId);
          if (channel)
            recAdditional << trVDR("Channel") << ": " << channel->Number() << " - " << channel->Name() << "\n";
        },
        recInfo->ChannelID());
    channelFuture.get();
#else
    cChannel *channel = Channels.GetByChannelID(((cRecordingInfo *)recInfo)->ChannelID());
    if (channel)
      recAdditional << trVDR("Channel") << ": " << channel->Number() << " - " << channel->Name() << "\n";
#endif

    const cEvent *Event = recInfo->GetEvent();
    if (Event) {
      // Genre
      bool firstContent = true;
      for (int i = 0; Event->Contents(i); i++) {
        if (!isempty(Event->ContentToString(Event->Contents(i)))) { // skip empty (user defined) content
          if (!firstContent) {
            text << ", ";
          } else {
            text << tr("Genre") << ": ";
          }
          text << Event->ContentToString(Event->Contents(i));
          firstContent = false;
          GenreIcons.push_back(GetGenreIcon(Event->Contents(i)));
        }
      }
      if (Event->Contents(0))
        text << "\n";
      // FSK
      if (Event->ParentalRating()) {
        Fsk = *Event->GetParentalRatingString();
        text << tr("FSK") << ": " << Fsk << "\n";
      }
    }
    cMarks marks;
    bool hasMarks =
        marks.Load(Recording->FileName(), Recording->FramesPerSecond(), Recording->IsPesRecording()) && marks.Count();
    cIndexFile *index = new cIndexFile(Recording->FileName(), false, Recording->IsPesRecording());

    int lastIndex = 0;

    int cuttedLength = 0;
    long cutinframe = 0;
    unsigned long long recsize = 0;
    unsigned long long recsizecutted = 0;
    unsigned long long cutinoffset = 0;
    unsigned long long filesize[100000];
    filesize[0] = 0;

    int i = 0;
    int imax = 999;
    struct stat filebuf;
    cString filename("");
    int rc = 0;

    do {
      if (Recording->IsPesRecording())
        filename = cString::sprintf("%s/%03d.vdr", Recording->FileName(), ++i);
      else {
        filename = cString::sprintf("%s/%05d.ts", Recording->FileName(), ++i);
        imax = 99999;
      }
      rc = stat(filename, &filebuf);
      if (rc == 0)
        filesize[i] = filesize[i - 1] + filebuf.st_size;
      else {
        if (ENOENT != errno) {
          esyslog("skinflatplus: error determining file size of \"%s\" %d (%s)", (const char *)filename, errno,
                  strerror(errno));
          recsize = 0;
        }
      }
    } while (i <= imax && !rc);
    recsize = filesize[i - 1];

    if (hasMarks && index) {
      uint16_t FileNumber;
      off_t FileOffset;

      bool cutin = true;
      cMark *mark = marks.First();
      while (mark) {
        long position = mark->Position();
        index->Get(position, &FileNumber, &FileOffset);
        if (cutin) {
          cutinframe = position;
          cutin = false;
          cutinoffset = filesize[FileNumber - 1] + FileOffset;
        } else {
          cuttedLength += position - cutinframe;
          cutin = true;
          recsizecutted += filesize[FileNumber - 1] + FileOffset - cutinoffset;
        }
        cMark *nextmark = marks.Next(mark);
        mark = nextmark;
      }
      if (!cutin) {
        cuttedLength += index->Last() - cutinframe;
        index->Get(index->Last() - 1, &FileNumber, &FileOffset);
        recsizecutted += filesize[FileNumber - 1] + FileOffset - cutinoffset;
      }
    }
    if (index) {
      lastIndex = index->Last();
      recAdditional << tr("Length") << ": " << *IndexToHMSF(lastIndex, false, Recording->FramesPerSecond());
      if (hasMarks)
        recAdditional << " (" << tr("cutted") << ": " << *IndexToHMSF(cuttedLength, false, Recording->FramesPerSecond())
                      << ")";
      recAdditional << "\n";
    }
    delete index;

    if (recsize > MEGABYTE(1023))
      recAdditional << tr("Size") << ": " << fixed << setprecision(2) << (float)recsize / MEGABYTE(1024) << " GB";
    else
      recAdditional << tr("Size") << ": " << recsize / MEGABYTE(1) << " MB";
    if (hasMarks)
      if (recsize > MEGABYTE(1023))
        recAdditional << " (" << tr("cutted") << ": " << fixed << setprecision(2)
                      << (float)recsizecutted / MEGABYTE(1024) << " GB)";
      else
        recAdditional << " (" << tr("cutted") << ": " << recsizecutted / MEGABYTE(1) << " MB)";

    recAdditional << "\n"
                  << trVDR("Priority") << ": " << Recording->Priority() << ", " << trVDR("Lifetime") << ": "
                  << Recording->Lifetime() << "\n";

    if (lastIndex) {
      recAdditional << tr("format") << ": " << (Recording->IsPesRecording() ? "PES" : "TS") << ", " << tr("bit rate")
                    << ": ~ " << fixed << setprecision(2)
                    << (float)recsize / lastIndex * Recording->FramesPerSecond() * 8 / MEGABYTE(1)
                    << " MBit/s (Video + Audio)";
    }
    // From SkinNopacity
#if APIVERSNUM >= 20505
    if (recInfo) {
      if (recInfo->Errors() >= 1)
        recAdditional << "\n" << tr("TS errors") << ": " << recInfo->Errors();
    }
#endif
    const cComponents *Components = recInfo->Components();
    if (Components) {
      ostringstream audio("");
      bool firstAudio = true;
      const char *audio_type = NULL;
      ostringstream subtitle("");
      bool firstSubtitle = true;
      for (int i = 0; i < Components->NumComponents(); i++) {
        const tComponent *p = Components->Component(i);

        switch (p->stream) {
        case sc_video_MPEG2:
          textAdditional << tr("Video") << ": " << p->description << " (MPEG2)";
          break;
        case sc_video_H264_AVC:
          textAdditional << tr("Video") << ": " << p->description << " (H.264)";
          break;
        case sc_video_H265_HEVC:  //might be not always correct because stream_content_ext (must be 0x0) is not available in tComponent
          textAdditional << tr("Video") << ": " << p->description << " (H.265)";
          break;
        case sc_audio_MP2:
        case sc_audio_AC3:
        case sc_audio_HEAAC:
          if (firstAudio)
            firstAudio = false;
          else
            audio << ", ";
          switch (p->stream) {
          case sc_audio_MP2:
            // workaround for wrongfully used stream type X 02 05 for AC3
            if (p->type == 5)
              audio_type = "AC3";
            else
              audio_type = "MP2";
            break;
          case sc_audio_AC3:
            audio_type = "AC3";
            break;
          case sc_audio_HEAAC:
            audio_type = "HEAAC";
            break;
          }
          if (p->description)
            audio << p->description << " (" << audio_type << ", " << p->language << ")";
          else
            audio << p->language << " (" << audio_type << ")";
          break;
        case sc_subtitle:
          if (firstSubtitle)
            firstSubtitle = false;
          else
            subtitle << ", ";
          if (p->description)
            subtitle << p->description << " (" << p->language << ")";
          else
            subtitle << p->language;
          break;
        }
      }
      if (audio.str().length() > 0) {
        if (textAdditional.str().length() > 0)
          textAdditional << "\n";
        textAdditional << tr("Audio") << ": " << audio.str();
      }
      if (subtitle.str().length() > 0) {
        if (textAdditional.str().length() > 0)
          textAdditional << "\n";
        textAdditional << tr("Subtitle") << ": " << subtitle.str();
      }
    }
    if (recInfo->Aux()) {
      string str_epgsearch = xml_substring(recInfo->Aux(), "<epgsearch>", "</epgsearch>");
      string channel(""), searchtimer(""), pattern("");
      if (!str_epgsearch.empty()) {
        channel = xml_substring(str_epgsearch, "<channel>", "</channel>");
        searchtimer = xml_substring(str_epgsearch, "<searchtimer>", "</searchtimer>");
        if (searchtimer.empty())
          searchtimer = xml_substring(str_epgsearch, "<Search timer>", "</Search timer>");
      }

      string str_tvscraper = xml_substring(recInfo->Aux(), "<tvscraper>", "</tvscraper>");
      string causedby(""), reason("");
      if (!str_tvscraper.empty()) {
        causedby = xml_substring(str_tvscraper, "<causedBy>", "</causedBy>");
        reason = xml_substring(str_tvscraper, "<reason>", "</reason>");
      }

      string str_vdradmin = xml_substring(recInfo->Aux(), "<vdradmin-am>", "</vdradmin-am>");
      if (!str_vdradmin.empty()) {
        pattern = xml_substring(str_vdradmin, "<pattern>", "</pattern>");
      }

      if ((!channel.empty() && !searchtimer.empty()) || (!causedby.empty() && !reason.empty()) || !pattern.empty()) {
        if (!channel.empty() && !searchtimer.empty()) {  // EPGSearch
          recAdditional << "\nEPGsearch: " << tr("channel") << ": " << channel << ", " << tr("search pattern") << ": "
                        << searchtimer;
        }
        if (!causedby.empty() && !reason.empty()) {  // TVScraper
          recAdditional << "\nTVScraper: " << tr("caused by") << ": " << causedby << ", " << tr("reason") << ": "
                        << reason;
        }
        if (!pattern.empty()) {  // VDR-Admin
          recAdditional << "\nVDRadmin-AM: " << tr("search pattern") << ": " << pattern;
        }
      }
    }
  }  // if Config.RecordingAdditionalInfoShow

  int headIconTop = chHeight - fontHeight - marginItem;
  int headIconLeft = chWidth - fontHeight - marginItem;
  if (Fsk.length() > 0) {
    cString iconName = cString::sprintf("EPGInfo/FSK/%s", Fsk.c_str());
    cImage *img = imgLoader.LoadIcon(*iconName, fontHeight, fontHeight);
    if (img) {
      contentHeadIconsPixmap->DrawImage(cPoint(headIconLeft, headIconTop), *img);
      headIconLeft -= fontHeight + marginItem;
    } else {
      isyslog("skinflatplus: FSK icon not found: %s", *iconName);
      img = imgLoader.LoadIcon("EPGInfo/FSK/unknown", fontHeight, fontHeight);
      if (img) {
        contentHeadIconsPixmap->DrawImage(cPoint(headIconLeft, headIconTop), *img);
        headIconLeft -= fontHeight + marginItem;
      }
    }
  }
  while (!GenreIcons.empty()) {
    GenreIcons.sort();
    GenreIcons.unique();
    cString iconName = cString::sprintf("EPGInfo/Genre/%s", GenreIcons.back().c_str());
    cImage *img = imgLoader.LoadIcon(*iconName, fontHeight, fontHeight);
    bool isUnknownDrawn = false;
    if (img) {
      contentHeadIconsPixmap->DrawImage(cPoint(headIconLeft, headIconTop), *img);
      headIconLeft -= fontHeight + marginItem;
    } else {
      isyslog("skinflatplus: Genre icon not found: %s", *iconName);
      if (!isUnknownDrawn) {
        img = imgLoader.LoadIcon("EPGInfo/Genre/unknown", fontHeight, fontHeight);
        if (img) {
          contentHeadIconsPixmap->DrawImage(cPoint(headIconLeft, headIconTop), *img);
          headIconLeft -= fontHeight + marginItem;
          isUnknownDrawn = true;
        }
      }
    }
    GenreIcons.pop_back();
  }

#ifdef DEBUGEPGTIME
  uint32_t tick1 = GetMsTicks();
  dsyslog("SetRecording info-text time: %d ms", tick1 - tick0);
#endif

  bool Scrollable = false;
  bool FirstRun = true;

  do {
    if (Scrollable) {
      FirstRun = false;
      cWidth -= scrollBarWidth;
    }

    ComplexContent.Clear();
    ComplexContent.SetOsd(osd);
    ComplexContent.SetPosition(cRect(cLeft, cTop, cWidth, cHeight));
    ComplexContent.SetBGColor(Theme.Color(clrMenuRecBg));
    ComplexContent.SetScrollSize(fontHeight);
    ComplexContent.SetScrollingActive(true);

    int ContentTop = marginItem;

    ostringstream series_info(""), movie_info("");

    std::vector<std::string> actors_path;
    std::vector<std::string> actors_name;
    std::vector<std::string> actors_role;

    std::string mediaPath("");
    int mediaWidth = 0;
    int mediaHeight = 0;

#ifdef DEBUGEPGTIME
    uint32_t tick2 = GetMsTicks();
#endif

    // first try scraper2vdr
    static cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
    if (!pScraper) // if it doesn't exit, try tvscraper
      pScraper = cPluginManager::GetPlugin("tvscraper");
    if ((Config.TVScraperRecInfoShowPoster || Config.TVScraperRecInfoShowActors) && pScraper) {
      ScraperGetEventType call;
      call.recording = Recording;
      int seriesId = 0;
      int episodeId = 0;
      int movieId = 0;

      if (pScraper->Service("GetEventType", &call)) {
        seriesId = call.seriesId;
        episodeId = call.episodeId;
        movieId = call.movieId;
      }
      if (seriesId > 0) {
        cSeries series;
        series.seriesId = seriesId;
        series.episodeId = episodeId;
        if (pScraper->Service("GetSeries", &series)) {
          if (series.banners.size() > 0)
            mediaPath = series.banners[0].path;
          mediaWidth = cWidth / 2 - marginItem * 2;
          mediaHeight = cHeight - marginItem * 2 - fontHeight - 6;
          if (Config.TVScraperRecInfoShowActors) {
            for (unsigned int i = 0; i < series.actors.size(); i++) {
              if (imgLoader.FileExits(series.actors[i].actorThumb.path)) {
                actors_path.push_back(series.actors[i].actorThumb.path);
                actors_name.push_back(series.actors[i].name);
                actors_role.push_back(series.actors[i].role);
              }
            }
          }
          if (series.name.length() > 0)
            series_info << tr("name: ") << series.name << "\n";
          if (series.firstAired.length() > 0)
            series_info << tr("first aired: ") << series.firstAired << "\n";
          if (series.network.length() > 0)
            series_info << tr("network: ") << series.network << "\n";
          if (series.genre.length() > 0)
            series_info << tr("genre: ") << series.genre << "\n";
          if (series.rating > 0)
            series_info << tr("rating: ") << series.rating << "\n";
          if (series.status.length() > 0)
            series_info << tr("status: ") << series.status << "\n";
          if (series.episode.season > 0)
            series_info << tr("season number: ") << series.episode.season << "\n";
          if (series.episode.number > 0)
            series_info << tr("episode number: ") << series.episode.number << "\n";
        }
      } else if (movieId > 0) {
        cMovie movie;
        movie.movieId = movieId;
        if (pScraper->Service("GetMovie", &movie)) {
          mediaPath = movie.poster.path;
          mediaWidth = cWidth / 2 - marginItem * 3;
          mediaHeight = cHeight - marginItem * 2 - fontHeight - 6;
          if (Config.TVScraperRecInfoShowActors) {
            for (unsigned int i = 0; i < movie.actors.size(); i++) {
              if (imgLoader.FileExits(movie.actors[i].actorThumb.path)) {
                actors_path.push_back(movie.actors[i].actorThumb.path);
                actors_name.push_back(movie.actors[i].name);
                actors_role.push_back(movie.actors[i].role);
              }
            }
          }
          if (movie.title.length() > 0)
            movie_info << tr("title: ") << movie.title << "\n";
          if (movie.originalTitle.length() > 0)
            movie_info << tr("original title: ") << movie.originalTitle << "\n";
          if (movie.collectionName.length() > 0)
            movie_info << tr("collection name: ") << movie.collectionName << "\n";
          if (movie.genres.length() > 0)
            movie_info << tr("genre: ") << movie.genres << "\n";
          if (movie.releaseDate.length() > 0)
            movie_info << tr("release date: ") << movie.releaseDate << "\n";
          if (movie.popularity > 0)
            movie_info << tr("popularity: ") << movie.popularity << "\n";
          if (movie.voteAverage > 0)
            movie_info << tr("vote average: ") << movie.voteAverage << "\n";
        }
      }
    }

#ifdef DEBUGEPGTIME
    uint32_t tick3 = GetMsTicks();
    dsyslog("SetRecording tvscraper time: %d ms", tick3 - tick2);
#endif

    cString recPath = cString::sprintf("%s", Recording->FileName());
    cString recImage("");
    if (imgLoader.SearchRecordingPoster(recPath, recImage)) {
      mediaWidth = cWidth / 2 - marginItem * 2;
      mediaHeight = cHeight - marginItem * 2 - fontHeight - 6;
      mediaPath = recImage;
    }
    if (mediaPath.length() > 0) {
      cImage *img = imgLoader.LoadFile(mediaPath.c_str(), mediaWidth, mediaHeight);
      if (img) {
        ComplexContent.AddText(tr("Description"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                               Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), font);
        ContentTop += fontHeight;
        ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuRecTitleLine));
        ContentTop += 6;
        ComplexContent.AddImageWithFloatedText(
            img, CIP_Right, text.str().c_str(),
            cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
            Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), font);
      } else if (text.str().length() > 0) {
        ComplexContent.AddText(tr("Description"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                               Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), font);
        ContentTop += fontHeight;
        ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuRecTitleLine));
        ContentTop += 6;
        ComplexContent.AddText(text.str().c_str(), true,
                               cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                               Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), font);
      }
    } else if (text.str().length() > 0) {
      ComplexContent.AddText(tr("Description"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuRecTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(text.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), font);
    }

    if (movie_info.str().length() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Movie information"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuRecTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(movie_info.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), font);
    }

    if (series_info.str().length() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Series information"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuRecTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(series_info.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), font);
    }
#ifdef DEBUGEPGTIME
    uint32_t tick4 = GetMsTicks();
    dsyslog("SetRecording epg-text time: %d ms", tick4 - tick3);
#endif

    if (Config.TVScraperRecInfoShowActors && actors_path.size() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Actors"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuRecTitleLine));
      ContentTop += 6;

      int actorsPerLine = 6;
      int numActors = actors_path.size();
      int actorWidth = cWidth / actorsPerLine - marginItem * 4;
      int picsPerLine = (cWidth - marginItem * 2) / actorWidth;
      int picLines = numActors / picsPerLine;
      if (numActors % picsPerLine != 0)
        picLines++;
      int actorMargin = ((cWidth - marginItem * 2) - actorWidth * actorsPerLine) / (actorsPerLine - 1);
      int x = marginItem;
      int y = ContentTop;
      int actor = 0;
      for (int row = 0; row < picLines; row++) {
        for (int col = 0; col < picsPerLine; col++) {
          if (actor == numActors)
            break;
          std::string path = actors_path[actor];
          cImage *img = imgLoader.LoadFile(path.c_str(), actorWidth, 999);
          if (img) {
            ComplexContent.AddImage(img, cRect(x, y, 0, 0));
            std::string name = actors_name[actor];
            std::stringstream sstrRole("");
            sstrRole << "\"" << actors_role[actor] << "\"";
            std::string role = sstrRole.str();
            ComplexContent.AddText(name.c_str(), false, cRect(x, y + img->Height() + marginItem, actorWidth, 0),
                                   Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), fontSml, actorWidth,
                                   fontSmlHeight, taCenter);
            ComplexContent.AddText(role.c_str(), false,
                                   cRect(x, y + img->Height() + marginItem + fontSmlHeight, actorWidth, 0),
                                   Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), fontSml, actorWidth,
                                   fontSmlHeight, taCenter);
          }
          x += actorWidth + actorMargin;
          actor++;
        }
        x = marginItem;
        y = ComplexContent.BottomContent() + fontHeight;
      }
    }
#ifdef DEBUGEPGTIME
    uint32_t tick5 = GetMsTicks();
    dsyslog("SetRecording actor time: %d ms", tick5 - tick4);
#endif

    if (recAdditional.str().length() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Recording information"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuRecTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(recAdditional.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), font);
    }

    if (textAdditional.str().length() > 0) {
      ContentTop = ComplexContent.BottomContent() + fontHeight;
      ComplexContent.AddText(tr("Video information"), false, cRect(marginItem * 10, ContentTop, 0, 0),
                             Theme.Color(clrMenuRecFontTitle), Theme.Color(clrMenuRecBg), font);
      ContentTop += fontHeight;
      ComplexContent.AddRect(cRect(0, ContentTop, cWidth, 3), Theme.Color(clrMenuRecTitleLine));
      ContentTop += 6;
      ComplexContent.AddText(textAdditional.str().c_str(), true,
                             cRect(marginItem, ContentTop, cWidth - marginItem * 2, cHeight - marginItem * 2),
                             Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), font);
    }

    Scrollable = ComplexContent.Scrollable(cHeight - marginItem * 2);
  } while (Scrollable && FirstRun);

  if (Config.MenuContentFullSize || Scrollable) {
    ComplexContent.CreatePixmaps(true);
  } else
    ComplexContent.CreatePixmaps(false);

  ComplexContent.Draw();

  contentHeadPixmap->Fill(clrTransparent);
  contentHeadPixmap->DrawRectangle(cRect(0, 0, menuWidth, fontHeight + fontSmlHeight * 2 + marginItem * 2),
                                   Theme.Color(clrScrollbarBg));

  cString timeString = cString::sprintf("%s  %s  %s", *DateString(Recording->Start()), *TimeString(Recording->Start()),
                                        recInfo->ChannelName() ? recInfo->ChannelName() : "");

  cString title = recInfo->Title();
  if (isempty(title))
    title = Recording->Name();

  cString shortText = recInfo->ShortText();

  contentHeadPixmap->DrawText(cPoint(marginItem, marginItem), timeString, Theme.Color(clrMenuRecFontInfo),
                              Theme.Color(clrMenuRecBg), fontSml, menuWidth - marginItem * 2);
  contentHeadPixmap->DrawText(cPoint(marginItem, marginItem + fontSmlHeight), title, Theme.Color(clrMenuRecFontTitle),
                              Theme.Color(clrMenuRecBg), font, menuWidth - marginItem * 2);
  contentHeadPixmap->DrawText(cPoint(marginItem, marginItem + fontSmlHeight + fontHeight), shortText,
                              Theme.Color(clrMenuRecFontInfo), Theme.Color(clrMenuRecBg), fontSml,
                              menuWidth - marginItem * 2);

  DecorBorderDraw(chLeft, chTop, chWidth, chHeight, Config.decorBorderMenuContentHeadSize,
                  Config.decorBorderMenuContentHeadType, Config.decorBorderMenuContentHeadFg,
                  Config.decorBorderMenuContentHeadBg, BorderSetRecording, false);

  if (Scrollable)
    DrawScrollbar(ComplexContent.ScrollTotal(), ComplexContent.ScrollOffset(), ComplexContent.ScrollShown(),
                  ComplexContent.Top() - scrollBarTop, ComplexContent.Height(), ComplexContent.ScrollOffset() > 0,
                  ComplexContent.ScrollOffset() + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);

  RecordingBorder.Left = cLeft;
  RecordingBorder.Top = cTop;
  RecordingBorder.Width = cWidth;
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
  uint32_t tick6 = GetMsTicks();
  dsyslog("SetRecording total time: %d ms", tick6 - tick0);
#endif
}

void cFlatDisplayMenu::SetText(const char *Text, bool FixedFont) {
  if (!Text)
    return;

  ShowEvent = false;
  ShowRecording = false;
  ShowText = true;
  ItemBorderClear();

  contentHeadPixmap->Fill(clrTransparent);

  int Left = Config.decorBorderMenuContentSize;
  int Top = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;
  int Width = menuWidth - Config.decorBorderMenuContentSize * 2;
  int Height = osdHeight - (topBarHeight + Config.decorBorderTopBarSize * 2 + buttonsHeight +
                            Config.decorBorderButtonSize * 2 + Config.decorBorderMenuContentSize * 2 + marginItem);

  if (!ButtonsDrawn())
    Height += buttonsHeight + Config.decorBorderButtonSize * 2;

  ComplexContent.Clear();

  menuItemWidth = Width;
  bool Scrollable = false;
  if (FixedFont) {
    ComplexContent.AddText(Text, true, cRect(marginItem, marginItem, Width - marginItem * 2, Height - marginItem * 2),
                           Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), fontFixed);
    ComplexContent.SetScrollSize(fontFixedHeight);
  } else {
    ComplexContent.AddText(Text, true, cRect(marginItem, marginItem, Width - marginItem * 2, Height - marginItem * 2),
                           Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), font);
    ComplexContent.SetScrollSize(fontHeight);
  }

  Scrollable = ComplexContent.Scrollable(Height - marginItem * 2);
  if (Scrollable) {
    Width -= scrollBarWidth;
    ComplexContent.Clear();
    if (FixedFont) {
      ComplexContent.AddText(Text, true, cRect(marginItem, marginItem, Width - marginItem * 2, Height - marginItem * 2),
                             Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), fontFixed);
      ComplexContent.SetScrollSize(fontFixedHeight);
    } else {
      ComplexContent.AddText(Text, true, cRect(marginItem, marginItem, Width - marginItem * 2, Height - marginItem * 2),
                             Theme.Color(clrMenuTextFixedFont), Theme.Color(clrMenuTextBg), font);
      ComplexContent.SetScrollSize(fontHeight);
    }
  }

  ComplexContent.SetOsd(osd);
  ComplexContent.SetPosition(cRect(Left, Top, Width, Height));
  ComplexContent.SetBGColor(Theme.Color(clrMenuTextBg));
  ComplexContent.SetScrollingActive(true);

  if (Config.MenuContentFullSize || Scrollable) {
    ComplexContent.CreatePixmaps(true);
  } else
    ComplexContent.CreatePixmaps(false);

  ComplexContent.Draw();

  if (Scrollable)
    DrawScrollbar(ComplexContent.ScrollTotal(), ComplexContent.ScrollOffset(), ComplexContent.ScrollShown(),
                  ComplexContent.Top() - scrollBarTop, ComplexContent.Height(), ComplexContent.ScrollOffset() > 0,
                  ComplexContent.ScrollOffset() + ComplexContent.ScrollShown() < ComplexContent.ScrollTotal(), true);

  if (Config.MenuContentFullSize || Scrollable)
    DecorBorderDraw(Left, Top, Width, ComplexContent.ContentHeight(true), Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg,
                    Config.decorBorderMenuContentBg);
  else
    DecorBorderDraw(Left, Top, Width, ComplexContent.ContentHeight(false), Config.decorBorderMenuContentSize,
                    Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg,
                    Config.decorBorderMenuContentBg);
}

int cFlatDisplayMenu::GetTextAreaWidth(void) const { return menuWidth - (marginItem * 2); }

const cFont *cFlatDisplayMenu::GetTextAreaFont(bool FixedFont) const {
  const cFont *rfont = FixedFont ? fontFixed : font;
  return rfont;
}

void cFlatDisplayMenu::SetMenuSortMode(eMenuSortMode MenuSortMode) {

  cString sortIcon("");
  switch (MenuSortMode) {
  case msmUnknown:
    sortIcon = "SortUnknown";
    // do not set search icon if it is unknown
    return;
    break;
  case msmNumber:
    sortIcon = "SortNumber";
    break;
  case msmName:
    sortIcon = "SortName";
    break;
  case msmTime:
    sortIcon = "SortDate";
    break;
  case msmProvider:
    sortIcon = "SortProvider";
    break;
  default:
    sortIcon = "SortUnknown";
    // do not set search icon if it is unknown
    return;
  }

  TopBarSetMenuIconRight(sortIcon);
}

void cFlatDisplayMenu::Flush(void) {
  TopBarUpdate();

  if (Config.MenuFullOsd && !MenuFullOsdIsDrawn) {
    menuPixmap->DrawRectangle(cRect(0, menuItemLastHeight - Config.decorBorderMenuItemSize,
                                    menuItemWidth + Config.decorBorderMenuItemSize * 2,
                                    menuPixmap->ViewPort().Height() - menuItemLastHeight + marginItem),
                              Theme.Color(clrItemSelableBg));
    // menuPixmap->DrawRectangle(cRect(0, menuPixmap->ViewPort().Height() - 5, menuItemWidth +
    // Config.decorBorderMenuItemSize*2, 5), Theme.Color(clrItemSelableBg));
    MenuFullOsdIsDrawn = true;
  }

  if (Config.MenuTimerShowCount && menuCategory == mcTimer) {
    int timerCount = 0, timerActiveCount = 0;
#if VDRVERSNUM >= 20301
    LOCK_TIMERS_READ;
    for (const cTimer *Timer = Timers->First(); Timer; Timer = Timers->Next(Timer)) {
#else
    for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer)) {
#endif
      timerCount++;
      if (Timer->HasFlags(tfActive))
        timerActiveCount++;
    }
    if (LastTimerCount != timerCount || LastTimerActiveCount != timerActiveCount) {
      LastTimerCount = timerCount;
      LastTimerActiveCount = timerActiveCount;
      cString newTitle = cString::sprintf("%s (%d/%d)", *LastTitle, timerActiveCount, timerCount);
      TopBarSetTitleWithoutClear(*newTitle);
    }
  }

  osd->Flush();
}

void cFlatDisplayMenu::ItemBorderInsertUnique(sDecorBorder ib) {
  std::list<sDecorBorder>::iterator it;
  for (it = ItemsBorder.begin(); it != ItemsBorder.end(); it++) {
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

  ItemsBorder.push_back(ib);
}

void cFlatDisplayMenu::ItemBorderDrawAllWithScrollbar(void) {
  std::list<sDecorBorder>::iterator it;
  for (it = ItemsBorder.begin(); it != ItemsBorder.end(); it++) {
    DecorBorderDraw((*it).Left, (*it).Top, (*it).Width - scrollBarWidth, (*it).Height, (*it).Size, (*it).Type,
                    (*it).ColorFg, (*it).ColorBg, BorderMenuItem);
  }
}

void cFlatDisplayMenu::ItemBorderDrawAllWithoutScrollbar(void) {
  std::list<sDecorBorder>::iterator it;
  for (it = ItemsBorder.begin(); it != ItemsBorder.end(); it++) {
    DecorBorderDraw((*it).Left, (*it).Top, (*it).Width + scrollBarWidth, (*it).Height, (*it).Size, (*it).Type,
                    (*it).ColorFg, (*it).ColorBg, BorderMenuItem);
  }
}

void cFlatDisplayMenu::ItemBorderClear(void) { ItemsBorder.clear(); }

bool cFlatDisplayMenu::isRecordingOld(const cRecording *Recording, int Level) {
  std::string RecFolder = GetRecordingName(Recording, Level, true).c_str();

  int value = Config.GetRecordingOldValue(RecFolder);
  if (value < 0)
    value = Config.MenuItemRecordingDefaultOldDays;
  if (value < 0)
    return false;

  int LastRecTimeFromFolder = GetLastRecTimeFromFolder(Recording, Level);
  time_t now;
  time(&now);

  int diffSecs = now - LastRecTimeFromFolder;
  int days = diffSecs / (60 * 60 * 24);

  // dsyslog("RecFolder: %s LastRecTimeFromFolder: %d time: %d value: %d diff: %d days: %d", RecFolder.c_str(),
  // LastRecTimeFromFolder, now, value, diffSecs, days);
  if (days > value)
    return true;

  return false;
}

time_t cFlatDisplayMenu::GetLastRecTimeFromFolder(const cRecording *Recording, int Level) {
  std::string RecFolder = GetRecordingName(Recording, Level, true).c_str();
  time_t RecStart = Recording->Start();

#if VDRVERSNUM >= 20301
  LOCK_RECORDINGS_READ;
  for (const cRecording *rec = Recordings->First(); rec; rec = Recordings->Next(rec)) {
#else
  for (cRecording *rec = Recordings.First(); rec; rec = Recordings.Next(rec)) {
#endif
    std::string RecFolder2 = GetRecordingName(rec, Level, true).c_str();
    if (RecFolder == RecFolder2) { // recordings must be in the same folder
      time_t RecStart2 = rec->Start();
      if (Config.MenuItemRecordingShowFolderDate == 1) { // newest
        if (RecStart2 > RecStart)
          RecStart = RecStart2;
      } else if (Config.MenuItemRecordingShowFolderDate == 2) // oldest
        if (RecStart2 < RecStart)
          RecStart = RecStart2;
    }
  }

  return RecStart;
}

// returns the string between start and end or an empty string if not found
string cFlatDisplayMenu::xml_substring(string source, const char *str_start, const char *str_end) {
  size_t start = source.find(str_start);
  size_t end = source.find(str_end);

  if (string::npos != start && string::npos != end) {
    return (source.substr(start + strlen(str_start), end - start - strlen(str_start)));
  }

  return string();
}

std::string cFlatDisplayMenu::GetRecordingName(const cRecording *Recording, int Level, bool isFolder) {
  if (!Recording)
    return "";

  std::string recNamePart("");
  std::string recName = Recording->Name();
  try {
    std::vector<std::string> tokens;
    std::istringstream f(recName.c_str());
    std::string s("");
    while (std::getline(f, s, FOLDERDELIMCHAR)) {
      tokens.push_back(s);
    }
    recNamePart = tokens.at(Level);
  } catch (...) {
    recNamePart = recName.c_str();
  }

  if (Config.MenuItemRecordingClearPercent && isFolder) {
    if (recNamePart[0] == '%') {
      recNamePart.erase(0, 1);
    }
  }
  return recNamePart;
}

const char *cFlatDisplayMenu::GetGenreIcon(uchar genre) {
  switch (genre & 0xF0) {
  case ecgMovieDrama:
    switch (genre & 0x0F) {
    case 0x00:
      return "Movie_Drama";
    case 0x01:
      return "Detective_Thriller";
    case 0x02:
      return "Adventure_Western_War";
    case 0x03:
      return "Science Fiction_Fantasy_Horror";
    case 0x04:
      return "Comedy";
    case 0x05:
      return "Soap_Melodrama_Folkloric";
    case 0x06:
      return "Romance";
    case 0x07:
      return "Serious_Classical_Religious_Historical Movie_Drama";
    case 0x08:
      return "Adult Movie_Drama";
    default:
      return "Movie_Drama";
    }
    break;
  case ecgNewsCurrentAffairs:
    switch (genre & 0x0F) {
    case 0x00:
      return "News_Current Affairs";
    case 0x01:
      return "News_Weather Report";
    case 0x02:
      return "News Magazine";
    case 0x03:
      return "Documentary";
    case 0x04:
      return "Discussion_Inverview_Debate";
    default:
      return "News_Current Affairs";
    }
    break;
  case ecgShow:
    switch (genre & 0x0F) {
    case 0x00:
      return "Show_Game Show";
    case 0x01:
      return "Game Show_Quiz_Contest";
    case 0x02:
      return "Variety Show";
    case 0x03:
      return "Talk Show";
    default:
      return "Show_Game Show";
    }
    break;
  case ecgSports:
    switch (genre & 0x0F) {
    case 0x00:
      return "Sports";
    case 0x01:
      return "Special Event";
    case 0x02:
      return "Sport Magazine";
    case 0x03:
      return "Football_Soccer";
    case 0x04:
      return "Tennis_Squash";
    case 0x05:
      return "Team Sports";
    case 0x06:
      return "Athletics";
    case 0x07:
      return "Motor Sport";
    case 0x08:
      return "Water Sport";
    case 0x09:
      return "Winter Sports";
    case 0x0A:
      return "Equestrian";
    case 0x0B:
      return "Martial Sports";
    default:
      return "Sports";
    }
    break;
  case ecgChildrenYouth:
    switch (genre & 0x0F) {
    case 0x00:
      return "Childrens_Youth Programme";
    case 0x01:
      return "Pre-school Childrens Programme";
    case 0x02:
      return "Entertainment Programme for 6 to 14";
    case 0x03:
      return "Entertainment Programme for 10 to 16";
    case 0x04:
      return "Informational_Educational_School Programme";
    case 0x05:
      return "Cartoons_Puppets";
    default:
      return "Childrens_Youth Programme";
    }
    break;
  case ecgMusicBalletDance:
    switch (genre & 0x0F) {
    case 0x00:
      return "Music_Ballet_Dance";
    case 0x01:
      return "Rock_Pop";
    case 0x02:
      return "Serious_Classical Music";
    case 0x03:
      return "Folk_Tradional Music";
    case 0x04:
      return "Jazz";
    case 0x05:
      return "Musical_Opera";
    case 0x06:
      return "Ballet";
    default:
      return "Music_Ballet_Dance";
    }
    break;
  case ecgArtsCulture:
    switch (genre & 0x0F) {
    case 0x00:
      return "Arts_Culture";
    case 0x01:
      return "Performing Arts";
    case 0x02:
      return "Fine Arts";
    case 0x03:
      return "Religion";
    case 0x04:
      return "Popular Culture_Traditional Arts";
    case 0x05:
      return "Literature";
    case 0x06:
      return "Film_Cinema";
    case 0x07:
      return "Experimental Film_Video";
    case 0x08:
      return "Broadcasting_Press";
    case 0x09:
      return "New Media";
    case 0x0A:
      return "Arts_Culture Magazine";
    case 0x0B:
      return "Fashion";
    default:
      return "Arts_Culture";
    }
    break;
  case ecgSocialPoliticalEconomics:
    switch (genre & 0x0F) {
    case 0x00:
      return "Social_Political_Economics";
    case 0x01:
      return "Magazine_Report_Documentary";
    case 0x02:
      return "Economics_Social Advisory";
    case 0x03:
      return "Remarkable People";
    default:
      return "Social_Political_Economics";
    }
    break;
  case ecgEducationalScience:
    switch (genre & 0x0F) {
    case 0x00:
      return "Education_Science_Factual";
    case 0x01:
      return "Nature_Animals_Environment";
    case 0x02:
      return "Technology_Natural Sciences";
    case 0x03:
      return "Medicine_Physiology_Psychology";
    case 0x04:
      return "Foreign Countries_Expeditions";
    case 0x05:
      return "Social_Spiritual Sciences";
    case 0x06:
      return "Further Education";
    case 0x07:
      return "Languages";
    default:
      return "Education_Science_Factual";
    }
    break;
  case ecgLeisureHobbies:
    switch (genre & 0x0F) {
    case 0x00:
      return "Leisure_Hobbies";
    case 0x01:
      return "Tourism_Travel";
    case 0x02:
      return "Handicraft";
    case 0x03:
      return "Motoring";
    case 0x04:
      return "Fitness_Health";
    case 0x05:
      return "Cooking";
    case 0x06:
      return "Advertisement_Shopping";
    case 0x07:
      return "Gardening";
    default:
      return "Leisure_Hobbies";
    }
    break;
  case ecgSpecial:
    switch (genre & 0x0F) {
    case 0x00:
      return "Original Language";
    case 0x01:
      return "Black & White";
    case 0x02:
      return "Unpublished";
    case 0x03:
      return "Live Broadcast";
    default:
      return "Original Language";
    }
    break;
  default:
    isyslog("skinflatplus: Genre not found: %x", genre);
  }
  return "";
}

void cFlatDisplayMenu::DrawMainMenuWidgets(void) {
  int wLeft = osdWidth * Config.MainMenuItemScale + marginItem + Config.decorBorderMenuContentSize +
              Config.decorBorderMenuItemSize;
  int wTop = topBarHeight + marginItem + Config.decorBorderTopBarSize * 2 + Config.decorBorderMenuContentSize;

  int wWidth = osdWidth - wLeft - Config.decorBorderMenuContentSize;
  int wHeight = menuPixmap->ViewPort().Height() - marginItem * 2;
  int ContentTop = 0;

  contentWidget.Clear();
  contentWidget.SetOsd(osd);
  contentWidget.SetPosition(cRect(wLeft, wTop, wWidth, wHeight));
  contentWidget.SetBGColor(Theme.Color(clrMenuRecBg));
  contentWidget.SetScrollingActive(false);

  std::vector<std::pair<int, std::string>> widgets;

  if (Config.MainMenuWidgetDVBDevicesShow)
    widgets.push_back(std::make_pair(Config.MainMenuWidgetDVBDevicesPosition, "dvb_devices"));
  if (Config.MainMenuWidgetActiveTimerShow)
    widgets.push_back(std::make_pair(Config.MainMenuWidgetActiveTimerPosition, "active_timer"));
  if (Config.MainMenuWidgetLastRecShow)
    widgets.push_back(std::make_pair(Config.MainMenuWidgetDVBDevicesPosition, "last_recordings"));
  if (Config.MainMenuWidgetSystemInfoShow)
    widgets.push_back(std::make_pair(Config.MainMenuWidgetSystemInfoPosition, "system_information"));
  if (Config.MainMenuWidgetSystemUpdatesShow)
    widgets.push_back(std::make_pair(Config.MainMenuWidgetSystemUpdatesPosition, "system_updates"));
  if (Config.MainMenuWidgetTemperaturesShow)
    widgets.push_back(std::make_pair(Config.MainMenuWidgetTemperaturesPosition, "temperatures"));
  if (Config.MainMenuWidgetTimerConflictsShow)
    widgets.push_back(std::make_pair(Config.MainMenuWidgetTimerConflictsPosition, "timer_conflicts"));
  if (Config.MainMenuWidgetCommandShow)
    widgets.push_back(std::make_pair(Config.MainMenuWidgetCommandPosition, "custom_command"));
  if (Config.MainMenuWidgetWeatherShow)
    widgets.push_back(std::make_pair(Config.MainMenuWidgetWeatherPosition, "weather"));

  std::sort(widgets.begin(), widgets.end(), pairCompareIntString);

  while (!widgets.empty()) {
    std::pair<int, std::string> pairWidget = widgets.back();
    widgets.pop_back();
    std::string widget = pairWidget.second;

    if (widget.compare("dvb_devices") == 0) {
      int addHeight = DrawMainMenuWidgetDVBDevices(wLeft, wWidth, ContentTop);
      if (addHeight > 0)
        ContentTop = addHeight + marginItem;
    } else if (widget.compare("active_timer") == 0) {
      int addHeight = DrawMainMenuWidgetActiveTimers(wLeft, wWidth, ContentTop);
      if (addHeight > 0)
        ContentTop = addHeight + marginItem;
    } else if (widget.compare("last_recordings") == 0) {
      int addHeight = DrawMainMenuWidgetLastRecordings(wLeft, wWidth, ContentTop);
      if (addHeight > 0)
        ContentTop = addHeight + marginItem;
    } else if (widget.compare("system_information") == 0) {
      int addHeight = DrawMainMenuWidgetSystemInformation(wLeft, wWidth, ContentTop);
      if (addHeight > 0)
        ContentTop = addHeight + marginItem;
    } else if (widget.compare("system_updates") == 0) {
      int addHeight = DrawMainMenuWidgetSystemUpdates(wLeft, wWidth, ContentTop);
      if (addHeight > 0)
        ContentTop = addHeight + marginItem;
    } else if (widget.compare("temperatures") == 0) {
      int addHeight = DrawMainMenuWidgetTemperaturs(wLeft, wWidth, ContentTop);
      if (addHeight > 0)
        ContentTop = addHeight + marginItem;
    } else if (widget.compare("timer_conflicts") == 0) {
      int addHeight = DrawMainMenuWidgetTimerConflicts(wLeft, wWidth, ContentTop);
      if (addHeight > 0)
        ContentTop = addHeight + marginItem;
    } else if (widget.compare("custom_command") == 0) {
      int addHeight = DrawMainMenuWidgetCommand(wLeft, wWidth, ContentTop);
      if (addHeight > 0)
        ContentTop = addHeight + marginItem;
    } else if (widget.compare("weather") == 0) {
      int addHeight = DrawMainMenuWidgetWeather(wLeft, wWidth, ContentTop);
      if (addHeight > 0)
        ContentTop = addHeight + marginItem;
    }
  }

  contentWidget.CreatePixmaps(false);
  contentWidget.Draw();

  DecorBorderDraw(wLeft, wTop, wWidth, contentWidget.ContentHeight(false), Config.decorBorderMenuContentSize,
                  Config.decorBorderMenuContentType, Config.decorBorderMenuContentFg, Config.decorBorderMenuContentBg,
                  BorderMMWidget);
}

int cFlatDisplayMenu::DrawMainMenuWidgetDVBDevices(int wLeft, int wWidth, int ContentTop) {
  int numDevices = cDevice::NumDevices();

  if (ContentTop + fontHeight + 6 + fontSmlHeight > menuPixmap->ViewPort().Height())
    return -1;

  cImage *img = imgLoader.LoadIcon("widgets/dvb_devices", fontHeight, fontHeight - marginItem * 2);
  if (img) {
    contentWidget.AddImage(img, cRect(marginItem, ContentTop + marginItem, fontHeight, fontHeight));
  }
  contentWidget.AddText(tr("DVB Devices"), false, cRect(marginItem * 2 + fontHeight, ContentTop, 0, 0),
                        Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
  ContentTop += fontHeight;
  contentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
  ContentTop += 6;

  // check device which currently displays live tv
  int deviceLiveTV = -1;
  cDevice *primaryDevice = cDevice::PrimaryDevice();
  if (primaryDevice) {
    if (!primaryDevice->Replaying() || primaryDevice->Transferring())
      deviceLiveTV = cDevice::ActualDevice()->DeviceNumber();
    else
      deviceLiveTV = primaryDevice->DeviceNumber();
  }

  // check currently recording devices
  bool *recDevices = new bool[numDevices];
  for (int i = 0; i < numDevices; i++)
    recDevices[i] = false;
#if VDRVERSNUM >= 20301
  LOCK_TIMERS_READ;
  for (const cTimer *timer = Timers->First(); timer; timer = Timers->Next(timer)) {
#else
  for (cTimer *timer = Timers.First(); timer; timer = Timers.Next(timer)) {
#endif
    if (!timer->Recording())
      continue;

    if (cRecordControl *RecordControl = cRecordControls::GetRecordControl(timer)) {
      const cDevice *recDevice = RecordControl->Device();
      if (recDevice) {
        recDevices[recDevice->DeviceNumber()] = true;
      }
    }
  }
  int actualNumDevices = 0;
  for (int i = 0; i < numDevices; i++) {
    if (ContentTop + marginItem > menuPixmap->ViewPort().Height())
      continue;

    const cDevice *device = cDevice::GetDevice(i);
    if (!device || !device->NumProvidedSystems())
      continue;

    actualNumDevices++;
    std::stringstream strDevice("");

    cString channelName("");

    const cChannel *channel = device->GetCurrentlyTunedTransponder();
    if (i == deviceLiveTV) {
      strDevice << tr("LiveTV") << " (";
      cString chanName("");
      if (channel && channel->Number() > 0) {
        chanName = channel->Name();
      } else {
        chanName = tr("Unknown");
      }
      strDevice << *chanName << ")";
    } else if (recDevices[i]) {
      strDevice << tr("recording") << " (";
      cString chanName("");
      if (channel && channel->Number() > 0) {
        chanName = channel->Name();
      } else {
        chanName = tr("Unknown");
      }
      strDevice << *chanName << ")";
    } else {
      if (channel) {
        cString chanName = channel->Name();
        if (!strcmp(*chanName, "")) {
          if (Config.MainMenuWidgetDVBDevicesDiscardNotUsed)
            continue;
          strDevice << tr("not used");
        } else {
          if (Config.MainMenuWidgetDVBDevicesDiscardUnknown)
            continue;
          strDevice << tr("Unknown") << " (" << *chanName << ")";
        }
      } else {
        if (Config.MainMenuWidgetDVBDevicesDiscardNotUsed)
          continue;
        strDevice << tr("not used");
      }
    }
    channelName = strDevice.str().c_str();

    cString str = cString::sprintf("%d", i + 1);  // Display Tuners 1..4
    if (Config.MainMenuWidgetDVBDevicesNativeNumbering) {
       str = cString::sprintf("%d", i);  // Display Tuners 0..3
    }
    int left = marginItem;
    if (numDevices <= 9) {
      contentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml);

      left += fontSml->Width("XX");
    } else {
      contentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                            fontSml->Width("XX"), fontSmlHeight, taRight);

      left += fontSml->Width("XXX");
    }
    str = *(device->DeviceType());
    contentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                          fontSml->Width("XXXXXXX"), fontSmlHeight, taLeft);

    left += fontSml->Width("XXXXXXXX");
    str = *channelName;
    contentWidget.AddText(*str, false, cRect(left, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml);

    ContentTop += fontSmlHeight;
  }

  delete[] recDevices;

  return contentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetActiveTimers(int wLeft, int wWidth, int ContentTop) {
  if (ContentTop + fontHeight + 6 + fontSmlHeight > menuPixmap->ViewPort().Height())
    return -1;

  cImage *img = imgLoader.LoadIcon("widgets/active_timers", fontHeight, fontHeight - marginItem * 2);
  if (img)
    contentWidget.AddImage(img, cRect(marginItem, ContentTop + marginItem, fontHeight, fontHeight));

  contentWidget.AddText(tr("Timer"), false, cRect(marginItem * 2 + fontHeight, ContentTop, 0, 0),
                        Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
  ContentTop += fontHeight;
  contentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
  ContentTop += 6;

  // check if remotetimers plugin is available
  static cPlugin *pRemoteTimers = cPluginManager::GetPlugin("remotetimers");

  time_t now;
  time(&now);
  if ((Config.MainMenuWidgetActiveTimerShowRemoteActive || Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
      pRemoteTimers && (now - remoteTimersLastRefresh) > Config.MainMenuWidgetActiveTimerShowRemoteRefreshTime) {
    remoteTimersLastRefresh = now;
    cString errorMsg("");
    pRemoteTimers->Service("RemoteTimers::RefreshTimers-v1.0", &errorMsg);
  }

  // look for timers
  cVector<const cTimer *> timerRec;
  cVector<const cTimer *> timerActive;
  cVector<const cTimer *> timerRemoteRec;
  cVector<const cTimer *> timerRemoteActive;

#if VDRVERSNUM >= 20301
  LOCK_TIMERS_READ;
  for (const cTimer *ti = Timers->First(); ti; ti = Timers->Next(ti)) {
#else
  for (cTimer *ti = Timers.First(); ti; ti = Timers.Next(ti)) {
#endif
    if (ti->HasFlags(tfRecording) && Config.MainMenuWidgetActiveTimerShowRecording)
      timerRec.Append(ti);

    if (ti->HasFlags(tfActive) && !ti->HasFlags(tfRecording) && Config.MainMenuWidgetActiveTimerShowActive)
      timerActive.Append(ti);

    if (timerRec.Size() + timerActive.Size() >= Config.MainMenuWidgetActiveTimerMaxCount)
      break;
  }

#if VDRVERSNUM >= 20301
  LOCK_SCHEDULES_READ;
#else
  cSchedulesLock SchedulesLock;
  const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
#endif
  if ((Config.MainMenuWidgetActiveTimerShowRemoteActive || Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
      pRemoteTimers && timerRec.Size() + timerActive.Size() < Config.MainMenuWidgetActiveTimerMaxCount) {
    cTimer *remoteTimer = NULL;
    while (pRemoteTimers->Service("RemoteTimers::ForEach-v1.0", &remoteTimer) && remoteTimer != NULL) {
      remoteTimer->SetEventFromSchedule(Schedules); // make sure the event is current
      if (remoteTimer->HasFlags(tfRecording) && Config.MainMenuWidgetActiveTimerShowRemoteRecording)
        timerRemoteRec.Append(remoteTimer);
      if (remoteTimer->HasFlags(tfActive) && !remoteTimer->HasFlags(tfRecording) &&
          Config.MainMenuWidgetActiveTimerShowRemoteActive)
        timerRemoteActive.Append(remoteTimer);
    }
  }
  timerRec.Sort(CompareTimers);
  timerActive.Sort(CompareTimers);
  timerRemoteRec.Sort(CompareTimers);
  timerRemoteActive.Sort(CompareTimers);

  if ((timerRec.Size() == 0 && timerActive.Size() == 0 && timerRemoteRec.Size() == 0 &&
       timerRemoteActive.Size() == 0) &&
      Config.MainMenuWidgetActiveTimerHideEmpty)
    return 0;
  else if (timerRec.Size() == 0 && timerActive.Size() == 0 && timerRemoteRec.Size() == 0 &&
           timerRemoteActive.Size() == 0) {
    contentWidget.AddText(
        tr("no active/recording timer"), false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
        Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);
  } else {
    int count = -1;
    int remotecount = -1;
    // first recording timer
    if (Config.MainMenuWidgetActiveTimerShowRecording) {
      for (int i = 0; i < timerRec.Size(); i++) {
        count++;
        if (ContentTop + marginItem > menuPixmap->ViewPort().Height())
          break;

        if (count >= Config.MainMenuWidgetActiveTimerMaxCount)
          break;

        const cChannel *Channel = (timerRec[i])->Channel();
        // const cEvent *Event = Timer->Event();
        std::stringstream strTimer("");
        if ((Config.MainMenuWidgetActiveTimerShowRemoteActive || Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
            pRemoteTimers && (timerRemoteRec.Size() > 0 || timerRemoteActive.Size() > 0))
          strTimer << "L";
        strTimer << count + 1 << ": ";
        if (Channel)
          strTimer << Channel->Name() << " - ";
        else
          strTimer << tr("Unknown") << " - ";

        strTimer << (timerRec[i])->File();

        contentWidget.AddText(
            strTimer.str().c_str(), false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
            Theme.Color(clrTopBarRecordingActiveFg), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);

        ContentTop += fontSmlHeight;
      }
    }
    if (Config.MainMenuWidgetActiveTimerShowActive) {
      for (int i = 0; i < timerActive.Size(); i++) {
        count++;
        if (ContentTop + marginItem > menuPixmap->ViewPort().Height())
          break;

        if (count >= Config.MainMenuWidgetActiveTimerMaxCount)
          break;

        const cChannel *Channel = (timerActive[i])->Channel();
        // const cEvent *Event = Timer->Event();
        std::stringstream strTimer("");
        if ((Config.MainMenuWidgetActiveTimerShowRemoteActive || Config.MainMenuWidgetActiveTimerShowRemoteRecording) &&
            pRemoteTimers && (timerRemoteRec.Size() > 0 || timerRemoteActive.Size() > 0))
          strTimer << "L";
        strTimer << count + 1 << ": ";
        if (Channel)
          strTimer << Channel->Name() << " - ";
        else
          strTimer << tr("Unknown") << " - ";
        strTimer << (timerActive[i])->File();

        contentWidget.AddText(
            strTimer.str().c_str(), false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);

        ContentTop += fontSmlHeight;
      }
    }
    if (Config.MainMenuWidgetActiveTimerShowRemoteRecording) {
      for (int i = 0; i < timerRemoteRec.Size(); i++) {
        remotecount++;
        if (ContentTop + marginItem > menuPixmap->ViewPort().Height())
          break;

        if (count + remotecount >= Config.MainMenuWidgetActiveTimerMaxCount)
          break;

        const cChannel *Channel = (timerRemoteRec[i])->Channel();
        // const cEvent *Event = Timer->Event();
        std::stringstream strTimer("");
        strTimer << "R" << remotecount + 1 << ": ";
        if (Channel)
          strTimer << Channel->Name() << " - ";
        else
          strTimer << tr("Unknown") << " - ";

        strTimer << (timerRemoteRec[i])->File();

        contentWidget.AddText(
            strTimer.str().c_str(), false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
            Theme.Color(clrTopBarRecordingActiveFg), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);

        ContentTop += fontSmlHeight;
      }
    }
    if (Config.MainMenuWidgetActiveTimerShowRemoteActive) {
      for (int i = 0; i < timerRemoteActive.Size(); i++) {
        remotecount++;
        if (ContentTop + marginItem > menuPixmap->ViewPort().Height())
          break;

        if (count + remotecount >= Config.MainMenuWidgetActiveTimerMaxCount)
          break;

        const cChannel *Channel = (timerRemoteActive[i])->Channel();
        // const cEvent *Event = Timer->Event();
        std::stringstream strTimer("");
        strTimer << "R" << remotecount + 1 << ": ";
        if (Channel)
          strTimer << Channel->Name() << " - ";
        else
          strTimer << tr("Unknown") << " - ";

        strTimer << (timerRemoteActive[i])->File();

        contentWidget.AddText(
            strTimer.str().c_str(), false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);

        ContentTop += fontSmlHeight;
      }
    }
  }

  return contentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetLastRecordings(int wLeft, int wWidth, int ContentTop) {
  if (ContentTop + fontHeight + 6 + fontSmlHeight > menuPixmap->ViewPort().Height())
    return -1;

  cImage *img = imgLoader.LoadIcon("widgets/last_recordings", fontHeight, fontHeight - marginItem * 2);
  if (img)
    contentWidget.AddImage(img, cRect(marginItem, ContentTop + marginItem, fontHeight, fontHeight));

  contentWidget.AddText(tr("Last Recordings"), false, cRect(marginItem * 2 + fontHeight, ContentTop, 0, 0),
                        Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
  ContentTop += fontHeight;
  contentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
  ContentTop += 6;

  std::vector<std::pair<time_t, std::string>> Recs;
#if VDRVERSNUM >= 20301
  LOCK_RECORDINGS_READ;
  for (const cRecording *rec = Recordings->First(); rec; rec = Recordings->Next(rec)) {
#else
  for (cRecording *rec = Recordings.First(); rec; rec = Recordings.Next(rec)) {
#endif
    time_t RecStart = rec->Start();

    int Minutes = (rec->LengthInSeconds() + 30) / 60;
    cString Length = cString::sprintf("%02d:%02d", Minutes / 60, Minutes % 60);
    cString DateTime =
        cString::sprintf("%s  %s  %s", *ShortDateString(rec->Start()), *TimeString(rec->Start()), *Length);

    std::string strRec = *(cString::sprintf("%s - %s", *DateTime, rec->Name()));
    Recs.push_back(std::make_pair(RecStart, strRec));
  }
  // Sort by RecStart
  std::sort(Recs.begin(), Recs.end(), pairCompareTimeStringDesc);
  int index = 0;
  while (!Recs.empty() && index < Config.MainMenuWidgetLastRecMaxCount) {
    if (ContentTop + marginItem > menuPixmap->ViewPort().Height())
      continue;

    std::pair<time_t, std::string> pairRec = Recs.back();
    Recs.pop_back();
    std::string Rec = pairRec.second;

    std::stringstream strRec("");
    strRec << Rec;

    contentWidget.AddText(
        strRec.str().c_str(), false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
        Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);
    ContentTop += fontSmlHeight;
    index++;
  }
  return contentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetTimerConflicts(int wLeft, int wWidth, int ContentTop) {
  if (ContentTop + fontHeight + 6 + fontSmlHeight > menuPixmap->ViewPort().Height())
    return -1;

  cImage *img = imgLoader.LoadIcon("widgets/timer_conflicts", fontHeight, fontHeight - marginItem * 2);
  if (img)
    contentWidget.AddImage(img, cRect(marginItem, ContentTop + marginItem, fontHeight, fontHeight));

  contentWidget.AddText(tr("Timer Conflicts"), false, cRect(marginItem * 2 + fontHeight, ContentTop, 0, 0),
                        Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
  ContentTop += fontHeight;
  contentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
  ContentTop += 6;

  int numConflicts = 0;
  cPlugin *p = cPluginManager::GetPlugin("epgsearch");
  if (p) {
    Epgsearch_lastconflictinfo_v1_0 *serviceData = new Epgsearch_lastconflictinfo_v1_0;
    if (serviceData) {
      serviceData->nextConflict = 0;
      serviceData->relevantConflicts = 0;
      serviceData->totalConflicts = 0;
      p->Service("Epgsearch-lastconflictinfo-v1.0", serviceData);
      if (serviceData->relevantConflicts > 0) {
        numConflicts = serviceData->relevantConflicts;
      }
      delete serviceData;
    }
  }
  if (numConflicts == 0 && Config.MainMenuWidgetTimerConflictsHideEmpty) {
    return 0;
  } else if (numConflicts == 0) {
    contentWidget.AddText(
        tr("no timer conflicts"), false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
        Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);
  } else {
    cString str = cString::sprintf("%s: %d", tr("timer conflicts"), numConflicts);
    contentWidget.AddText(*str, false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                          wWidth - marginItem * 2);
  }

  return contentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetSystemInformation(int wLeft, int wWidth, int ContentTop) {
  if (ContentTop + fontHeight + 6 + fontSmlHeight > menuPixmap->ViewPort().Height())
    return -1;

  cImage *img = imgLoader.LoadIcon("widgets/system_information", fontHeight, fontHeight - marginItem * 2);
  if (img)
    contentWidget.AddImage(img, cRect(marginItem, ContentTop + marginItem, fontHeight, fontHeight));

  contentWidget.AddText(tr("System Information"), false, cRect(marginItem * 2 + fontHeight, ContentTop, 0, 0),
                        Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
  ContentTop += fontHeight;
  contentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
  ContentTop += 6;

  cString execFile = cString::sprintf("\"%s/system_information/system_information\"", WIDGETFOLDER);
  int r = system(*execFile);
  r += 0; // prevent Warning for unused variable

  cString configsPath = cString::sprintf("%s/system_information/", WIDGETOUTPUTPATH);

  std::vector<std::string> files;

  cReadDir d(configsPath);
  struct dirent *e;
  while ((e = d.Next()) != NULL) {
    std::string fname = e->d_name;
    std::size_t found = fname.find("_");
    if (found != std::string::npos) {
      std::string num = fname.substr(0, found);
      if (atoi(num.c_str()) > 0) {
        files.push_back(e->d_name);
      }
    }
  }
  int Column = 1;
  int ContentLeft = marginItem;
  std::sort(files.begin(), files.end(), stringCompare);
  if (files.size() == 0) {
    cString str = cString::sprintf("%s - %s", tr("no information available please check the script"), *execFile);
    contentWidget.AddText(*str, false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                          wWidth - marginItem * 2);
  } else {
    for (unsigned i = 0; i < files.size(); i++) {
      // check for height
      if (ContentTop + marginItem > menuPixmap->ViewPort().Height())
        break;

      std::string fname = files[i];
      std::size_t found = fname.find("_");
      if (found != std::string::npos) {
        std::string num = fname.substr(0, found);
        if (atoi(num.c_str()) > 0) {
          std::string item = fname.substr(found + 1, fname.length() - found);
          cString itemFilename = cString::sprintf("%s/system_information/%s", WIDGETOUTPUTPATH, fname.c_str());
          std::ifstream file(*itemFilename, std::ifstream::in);
          if (file.is_open()) {
            std::string item_content("");
            std::getline(file, item_content);

            if (!strcmp(item.c_str(), "sys_version")) {
              if (Column == 2) {
                Column = 1;
                ContentTop += fontSmlHeight;
                ContentLeft = marginItem;
              }
              cString str = cString::sprintf("%s: %s", tr("System Version"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              ContentTop += fontSmlHeight;
            } else if (!item.compare("kernel_version")) {
              if (Column == 2) {
                Column = 1;
                ContentTop += fontSmlHeight;
                ContentLeft = marginItem;
              }
              cString str = cString::sprintf("%s: %s", tr("Kernel Version"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              ContentTop += fontSmlHeight;
            } else if (!item.compare("uptime")) {
              cString str = cString::sprintf("%s: %s", tr("Uptime"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("load")) {
              cString str = cString::sprintf("%s: %s", tr("Load"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("processes")) {
              cString str = cString::sprintf("%s: %s", tr("Processes"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("mem_usage")) {
              cString str = cString::sprintf("%s: %s", tr("Memory Usage"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("swap_usage")) {
              cString str = cString::sprintf("%s: %s", tr("Swap Usage"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("root_usage")) {
              cString str = cString::sprintf("%s: %s", tr("Root Usage"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("video_usage")) {
              cString str = cString::sprintf("%s: %s", tr("Video Usage"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("vdr_cpu_usage")) {
              cString str = cString::sprintf("%s: %s", tr("VDR CPU Usage"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("vdr_mem_usage")) {
              cString str = cString::sprintf("%s: %s", tr("VDR MEM Usage"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("cpu")) {
              cString str = cString::sprintf("%s: %s", tr("Temp CPU"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("gpu")) {
              cString str = cString::sprintf("%s: %s", tr("Temp GPU"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("pccase")) {
              cString str = cString::sprintf("%s: %s", tr("Temp PC-Case"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("motherboard")) {
              cString str = cString::sprintf("%s: %s", tr("Temp MB"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("updates")) {
              cString str = cString::sprintf("%s: %s", tr("Updates"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            } else if (!item.compare("security_updates")) {
              cString str = cString::sprintf("%s: %s", tr("Security Updates"), item_content.c_str());
              contentWidget.AddText(*str, false, cRect(ContentLeft, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                                    Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                                    wWidth - marginItem * 2);
              if (Column == 1) {
                Column = 2;
                ContentLeft = wWidth / 2;
              } else {
                Column = 1;
                ContentLeft = marginItem;
                ContentTop += fontSmlHeight;
              }
            }
            file.close();
          }
        }
      }
    }
  }

  return contentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetSystemUpdates(int wLeft, int wWidth, int ContentTop) {
  if (ContentTop + fontHeight + 6 + fontSmlHeight > menuPixmap->ViewPort().Height())
    return -1;

  cImage *img = imgLoader.LoadIcon("widgets/system_updates", fontHeight, fontHeight - marginItem * 2);
  if (img)
    contentWidget.AddImage(img, cRect(marginItem, ContentTop + marginItem, fontHeight, fontHeight));

  contentWidget.AddText(tr("System Updates"), false, cRect(marginItem * 2 + fontHeight, ContentTop, 0, 0),
                        Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
  ContentTop += fontHeight;
  contentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
  ContentTop += 6;

  int updates = 0, securityUpdates = 0;
  cString itemFilename = cString::sprintf("%s/system_updatestatus/updates", WIDGETOUTPUTPATH);
  std::ifstream file(*itemFilename, std::ifstream::in);
  if (file.is_open()) {
    std::string cont("");
    std::getline(file, cont);
    updates = atoi(cont.c_str());
    file.close();
  } else {
    updates = -1;
  }

  itemFilename = cString::sprintf("%s/system_updatestatus/security_updates", WIDGETOUTPUTPATH);
  std::ifstream file2(*itemFilename, std::ifstream::in);
  if (file2.is_open()) {
    std::string cont("");
    std::getline(file2, cont);
    securityUpdates = atoi(cont.c_str());
    file2.close();
  } else {
    securityUpdates = -1;
  }

  if (updates == -1 || securityUpdates == -1) {
    contentWidget.AddText(tr("Updatestatus not available please check the widget"), false,
                          cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                          wWidth - marginItem * 2);
  } else if (updates == 0 && securityUpdates == 0 && Config.MainMenuWidgetSystemUpdatesHideIfZero) {
    return 0;
  } else {
    cString str = cString::sprintf("%s: %d", tr("Updates"), updates);
    contentWidget.AddText(*str, false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                          wWidth - marginItem * 2);
    str = cString::sprintf("%s: %d", tr("Security Updates"), securityUpdates);
    contentWidget.AddText(
        *str, false, cRect(wWidth / 2 + marginItem, ContentTop, wWidth / 2 - marginItem * 2, fontSmlHeight),
        Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);
  }

  return contentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetTemperaturs(int wLeft, int wWidth, int ContentTop) {
  if (ContentTop + fontHeight + 6 + fontSmlHeight > menuPixmap->ViewPort().Height())
    return -1;

  cImage *img = imgLoader.LoadIcon("widgets/temperatures", fontHeight, fontHeight - marginItem * 2);
  if (img)
    contentWidget.AddImage(img, cRect(marginItem, ContentTop + marginItem, fontHeight, fontHeight));

  contentWidget.AddText(tr("Temperatures"), false, cRect(marginItem * 2 + fontHeight, ContentTop, 0, 0),
                        Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
  ContentTop += fontHeight;
  contentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
  ContentTop += 6;

  cString execFile =
      cString::sprintf("cd \"%s/temperatures\"; \"%s/temperatures/temperatures\"", WIDGETFOLDER, WIDGETFOLDER);
  int r = system(*execFile);
  r += 0; // prevent Warning for unused variable

  int countTemps = 0;

  std::string tempCPU(""), tempCase(""), tempMB(""), tempGPU("");
  cString itemFilename = cString::sprintf("%s/temperatures/cpu", WIDGETOUTPUTPATH);
  std::ifstream file(*itemFilename, std::ifstream::in);
  if (file.is_open()) {
    std::getline(file, tempCPU);
    file.close();
    countTemps++;
  } else {
    tempCPU = "-1";
  }

  itemFilename = cString::sprintf("%s/temperatures/pccase", WIDGETOUTPUTPATH);
  std::ifstream file2(*itemFilename, std::ifstream::in);
  if (file2.is_open()) {
    std::string cont("");
    std::getline(file2, tempCase);
    file2.close();
    countTemps++;
  } else {
    tempCase = "-1";
  }

  itemFilename = cString::sprintf("%s/temperatures/motherboard", WIDGETOUTPUTPATH);
  std::ifstream file3(*itemFilename, std::ifstream::in);
  if (file3.is_open()) {
    std::string cont("");
    std::getline(file3, tempMB);
    file3.close();
    countTemps++;
  } else {
    tempMB = "-1";
  }
  itemFilename = cString::sprintf("%s/temperatures/gpu", WIDGETOUTPUTPATH);
  std::ifstream file4(*itemFilename, std::ifstream::in);
  if (file4.is_open()) {
    std::string cont("");
    std::getline(file4, tempGPU);
    file4.close();
    countTemps++;
  } else {
    tempGPU = "-1";
  }

  if (!strcmp(tempCPU.c_str(), "-1") && !strcmp(tempCase.c_str(), "-1") && !strcmp(tempMB.c_str(), "-1") &&
      !strcmp(tempGPU.c_str(), "-1")) {
    contentWidget.AddText(tr("Temperatures not available please check the widget"), false,
                          cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                          wWidth - marginItem * 2);
  } else {
    int Left = marginItem;
    int addLeft = wWidth / (countTemps);
    if (strcmp(tempCPU.c_str(), "-1")) {
      cString str = cString::sprintf("%s: %s", tr("CPU"), tempCPU.c_str());
      contentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                            wWidth - marginItem * 2);
      Left += addLeft;
    }
    if (strcmp(tempCase.c_str(), "-1")) {
      cString str = cString::sprintf("%s: %s", tr("PC-Case"), tempCase.c_str());
      contentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth / 3 - marginItem * 2, fontSmlHeight),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                            wWidth - marginItem * 2);
      Left += addLeft;
    }
    if (strcmp(tempMB.c_str(), "-1")) {
      cString str = cString::sprintf("%s: %s", tr("MB"), tempMB.c_str());
      contentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth / 3 * 2 - marginItem * 2, fontSmlHeight),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                            wWidth - marginItem * 2);
      Left += addLeft;
    }
    if (strcmp(tempGPU.c_str(), "-1")) {
      cString str = cString::sprintf("%s: %s", tr("GPU"), tempGPU.c_str());
      contentWidget.AddText(*str, false, cRect(Left, ContentTop, wWidth / 3 * 2 - marginItem * 2, fontSmlHeight),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml,
                            wWidth - marginItem * 2);
    }
  }

  return contentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetCommand(int wLeft, int wWidth, int ContentTop) {
  if (ContentTop + fontHeight + 6 + fontSmlHeight > menuPixmap->ViewPort().Height())
    return -1;

  cString execFile = cString::sprintf("\"%s/command_output/command\"", WIDGETFOLDER);
  int r = system(*execFile);
  r += 0; // prevent Warning for unused variable

  std::string Title("");
  cString itemFilename = cString::sprintf("%s/command_output/title", WIDGETOUTPUTPATH);
  std::ifstream file(*itemFilename, std::ifstream::in);
  if (file.is_open()) {
    std::getline(file, Title);
    file.close();
  } else {
    Title = tr("no title available");
  }

  cImage *img = imgLoader.LoadIcon("widgets/command_output", fontHeight, fontHeight - marginItem * 2);
  if (img)
    contentWidget.AddImage(img, cRect(marginItem, ContentTop + marginItem, fontHeight, fontHeight));

  contentWidget.AddText(Title.c_str(), false, cRect(marginItem * 2 + fontHeight, ContentTop, 0, 0),
                        Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
  ContentTop += fontHeight;
  contentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
  ContentTop += 6;

  std::string Output("");
  itemFilename = cString::sprintf("%s/command_output/output", WIDGETOUTPUTPATH);
  std::ifstream file2(*itemFilename, std::ifstream::in);
  if (file2.is_open()) {
    for (; std::getline(file2, Output);) {
      if (ContentTop + marginItem > menuPixmap->ViewPort().Height())
        break;
      contentWidget.AddText(
          Output.c_str(), false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);
      ContentTop += fontSmlHeight;
    }
    file2.close();
  } else {
    contentWidget.AddText(
        tr("no output available"), false, cRect(marginItem, ContentTop, wWidth - marginItem * 2, fontSmlHeight),
        Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontSml, wWidth - marginItem * 2);
  }

  return contentWidget.ContentHeight(false);
}

int cFlatDisplayMenu::DrawMainMenuWidgetWeather(int wLeft, int wWidth, int ContentTop) {
  if (ContentTop + fontHeight + 6 + fontSmlHeight > menuPixmap->ViewPort().Height())
    return -1;

  cFont *fontTempSml = cFont::CreateFont(Setup.FontOsd, Setup.FontOsdSize / 2.0);

  std::string Location("");
  cString locationFilename = cString::sprintf("%s/weather/weather.location", WIDGETOUTPUTPATH);
  std::ifstream file(*locationFilename, std::ifstream::in);
  if (file.is_open()) {
    std::getline(file, Location);
    file.close();
  } else {
    Location = tr("Unknown");
  }

  std::string tempToday("");
  cString filename("");
  filename = cString::sprintf("%s/weather/weather.0.temp", WIDGETOUTPUTPATH);
  file.open(*filename, std::ifstream::in);
  if (file.is_open()) {
    std::getline(file, tempToday);
    file.close();
  }

  cString Title = cString::sprintf("%s - %s %s", tr("Weather"), Location.c_str(), tempToday.c_str());

  cImage *img = imgLoader.LoadIcon("widgets/weather", fontHeight, fontHeight - marginItem * 2);
  if (img)
    contentWidget.AddImage(img, cRect(marginItem, ContentTop + marginItem, fontHeight, fontHeight));

  contentWidget.AddText(*Title, false, cRect(marginItem * 2 + fontHeight, ContentTop, 0, 0),
                        Theme.Color(clrMenuEventFontTitle), Theme.Color(clrMenuEventBg), font);
  ContentTop += fontHeight;
  contentWidget.AddRect(cRect(0, ContentTop, wWidth, 3), Theme.Color(clrMenuEventTitleLine));
  ContentTop += 6;

  int left = marginItem;
  for (int index = 0; index < Config.MainMenuWidgetWeatherDays; index++) {
    std::string icon("");
    cString iconFilename = cString::sprintf("%s/weather/weather.%d.icon", WIDGETOUTPUTPATH, index);
    std::ifstream file(*iconFilename, std::ifstream::in);
    if (file.is_open()) {
      std::getline(file, icon);
      file.close();
    } else
      continue;

    std::string summary("");
    cString summaryFilename = cString::sprintf("%s/weather/weather.%d.summary", WIDGETOUTPUTPATH, index);
    std::ifstream file2(*summaryFilename, std::ifstream::in);
    if (file2.is_open()) {
      std::getline(file2, summary);
      file2.close();
    } else
      continue;

    std::string tempMax("");
    cString tempMaxFilename = cString::sprintf("%s/weather/weather.%d.tempMax", WIDGETOUTPUTPATH, index);
    std::ifstream file3(*tempMaxFilename, std::ifstream::in);
    if (file3.is_open()) {
      std::getline(file3, tempMax);
      file3.close();
    } else
      continue;

    std::string tempMin("");
    cString tempMinFilename = cString::sprintf("%s/weather/weather.%d.tempMin", WIDGETOUTPUTPATH, index);
    std::ifstream file4(*tempMinFilename, std::ifstream::in);
    if (file4.is_open()) {
      std::getline(file4, tempMin);
      file4.close();
    } else
      continue;

    std::string prec("");
    double p = 0.0;
    cString precString("0%");
    cString precFilename = cString::sprintf("%s/weather/weather.%d.precipitation", WIDGETOUTPUTPATH, index);
    std::ifstream file5(*precFilename, std::ifstream::in);
    if (file5.is_open()) {
      std::getline(file5, prec);
      std::replace(prec.begin(), prec.end(), '.', ',');
      file5.close();
      p = atof(prec.c_str()) * 100.0;
      p = roundUp(p, 10);
      precString = cString::sprintf("%.0f%%", p);
    } else
      continue;

    /* std::string precType("");  // Wird nirgends verwendet?
    cString precTypeFilename = cString::sprintf("%s/weather/weather.%d.precipitationType", WIDGETOUTPUTPATH, index );
    std::ifstream file6(*precTypeFilename, std::ifstream::in);
    if( file6.is_open() ) {
        std::getline(file6, precType);
        file6.close();
    } else
       continue; */

    cString weekDayName("");
    time_t t;
    time(&t);
    struct tm *tm = localtime(&t);
    tm->tm_mday += index;
    time_t t2 = mktime(tm);
    weekDayName = WeekDayName(t2);

    if (Config.MainMenuWidgetWeatherType == 0) { // short
      if (left + fontHeight * 2 + fontTempSml->Width("-99,9°C") + fontTempSml->Width("XXXX") + marginItem * 6 > wWidth)
        break;
      if (index > 0) {
        contentWidget.AddText("|", false, cRect(left, ContentTop, 0, 0), Theme.Color(clrMenuEventFontInfo),
                              Theme.Color(clrMenuEventBg), font);
        left += font->Width("|") + marginItem * 2;
      }

      cString weatherIcon = cString::sprintf("widgets/%s", icon.c_str());
      cImage *img = imgLoader.LoadIcon(*weatherIcon, fontHeight, fontHeight - marginItem * 2);
      if (img) {
        contentWidget.AddImage(img, cRect(left, ContentTop + marginItem, fontHeight, fontHeight));
        left += fontHeight + marginItem;
      }
      int wtemp = myMax(fontTempSml->Width(tempMax.c_str()), fontTempSml->Width(tempMin.c_str()));
      contentWidget.AddText(tempMax.c_str(), false, cRect(left, ContentTop, 0, 0), Theme.Color(clrMenuEventFontInfo),
                            Theme.Color(clrMenuEventBg), fontTempSml, wtemp, fontTempSml->Height(), taRight);
      contentWidget.AddText(tempMin.c_str(), false, cRect(left, ContentTop + fontTempSml->Height(), 0, 0),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontTempSml, wtemp,
                            fontTempSml->Height(), taRight);

      left += wtemp + marginItem;

      img = imgLoader.LoadIcon("widgets/umbrella", fontHeight, fontHeight - marginItem * 2);
      if (img) {
        contentWidget.AddImage(img, cRect(left, ContentTop + marginItem, fontHeight, fontHeight));
        left += fontHeight - marginItem;
      }
      contentWidget.AddText(*precString, false,
                            cRect(left, ContentTop + (fontHeight / 2 - fontTempSml->Height() / 2), 0, 0),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontTempSml);
      left += fontTempSml->Width(*precString) + marginItem * 2;
    } else { // long
      if (ContentTop + marginItem > menuPixmap->ViewPort().Height())
        break;

      left = marginItem;

      cString dayname = cString::sprintf("%s ", *weekDayName);
      contentWidget.AddText(*dayname, false, cRect(left, ContentTop, wWidth - marginItem * 2, fontHeight),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), font,
                            wWidth - marginItem * 2);
      left += font->Width("XXXX") + marginItem;

      cString weatherIcon = cString::sprintf("widgets/%s", icon.c_str());
      cImage *img = imgLoader.LoadIcon(*weatherIcon, fontHeight, fontHeight - marginItem * 2);
      if (img) {
        contentWidget.AddImage(img, cRect(left, ContentTop + marginItem, fontHeight, fontHeight));
        left += fontHeight + marginItem;
      }
      contentWidget.AddText(tempMax.c_str(), false, cRect(left, ContentTop, 0, 0), Theme.Color(clrMenuEventFontInfo),
                            Theme.Color(clrMenuEventBg), fontTempSml, fontTempSml->Width("-99,9°C"),
                            fontTempSml->Height(), taRight);
      contentWidget.AddText(tempMin.c_str(), false, cRect(left, ContentTop + fontTempSml->Height(), 0, 0),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontTempSml,
                            fontTempSml->Width("-99,9°C"), fontTempSml->Height(), taRight);

      left += fontTempSml->Width("-99,9°C ") + marginItem;

      img = imgLoader.LoadIcon("widgets/umbrella", fontHeight, fontHeight - marginItem * 2);
      if (img) {
        contentWidget.AddImage(img, cRect(left, ContentTop + marginItem, fontHeight, fontHeight));
        left += fontHeight - marginItem;
      }
      contentWidget.AddText(*precString, false,
                            cRect(left, ContentTop + (fontHeight / 2 - fontTempSml->Height() / 2), 0, 0),
                            Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontTempSml,
                            fontTempSml->Width("100%"), fontTempSml->Height(), taRight);
      left += fontTempSml->Width("100% ") + marginItem;

      contentWidget.AddText(
          summary.c_str(), false,
          cRect(left, ContentTop + (fontHeight / 2 - fontTempSml->Height() / 2), wWidth - left, fontHeight),
          Theme.Color(clrMenuEventFontInfo), Theme.Color(clrMenuEventBg), fontTempSml, wWidth - left);

      ContentTop += fontHeight;
    }
  }
  return contentWidget.ContentHeight(false);
}

void cFlatDisplayMenu::PreLoadImages(void) {
  // menu icons
  cString Path = cString::sprintf("%s%s/menuIcons", *Config.iconPath, Setup.OSDTheme);
  cReadDir d(Path);
  struct dirent *e;
  while ((e = d.Next()) != NULL) {
    cString FileName = cString::sprintf("menuIcons/%s", GetFilenameWithoutext(e->d_name));
    imgLoader.LoadIcon(*FileName, fontHeight - marginItem * 2, fontHeight - marginItem * 2);
  }

  imgLoader.LoadIcon("menuIcons/blank", fontHeight - marginItem * 2, fontHeight - marginItem * 2);

  int imageHeight = fontHeight;
  int imageBGHeight = imageHeight;
  int imageBGWidth = imageHeight * 1.34;
  cImage *imgBG = imgLoader.LoadIcon("logo_background", imageBGWidth, imageBGHeight);
  if (imgBG) {
    imageBGHeight = imgBG->Height();
    imageBGWidth = imgBG->Width();
  }

  imgLoader.LoadIcon("radio", imageBGWidth - 10, imageBGHeight - 10);
  imgLoader.LoadIcon("changroup", imageBGWidth - 10, imageBGHeight - 10);
  imgLoader.LoadIcon("tv", imageBGWidth - 10, imageBGHeight - 10);
  imgLoader.LoadIcon("timerInactive", imageHeight, imageHeight);
  imgLoader.LoadIcon("timerRecording", imageHeight, imageHeight);
  imgLoader.LoadIcon("timerActive", imageHeight, imageHeight);

  int index = 0;
  cImage *img = NULL;
#if VDRVERSNUM >= 20301
  LOCK_CHANNELS_READ;
  for (const cChannel *Channel = Channels->First(); Channel && index < LOGO_PRE_CACHE;
       Channel = Channels->Next(Channel)) {
#else
  for (cChannel *Channel = Channels.First(); Channel && index < LOGO_PRE_CACHE; Channel = Channels.Next(Channel)) {
#endif
    img = imgLoader.LoadLogo(Channel->Name(), imageBGWidth - 4, imageBGHeight - 4);
    if (img)
      index++;
  }

  imgLoader.LoadIcon("radio", 999, topBarHeight - marginItem * 2);
  imgLoader.LoadIcon("changroup", 999, topBarHeight - marginItem * 2);
  imgLoader.LoadIcon("tv", 999, topBarHeight - marginItem * 2);

  imgLoader.LoadIcon("timer_full", imageHeight, imageHeight);
  imgLoader.LoadIcon("timer_full_cur", imageHeight, imageHeight);
  imgLoader.LoadIcon("timer_partial", imageHeight, imageHeight);
  imgLoader.LoadIcon("vps", imageHeight, imageHeight);
  imgLoader.LoadIcon("vps_cur", imageHeight, imageHeight);

  imgLoader.LoadIcon("recording_new", fontHeight, fontHeight);
  imgLoader.LoadIcon("recording_new", fontSmlHeight, fontSmlHeight);
  imgLoader.LoadIcon("recording_new_cur", fontHeight, fontHeight);
  imgLoader.LoadIcon("recording_new_cur", fontSmlHeight, fontSmlHeight);
  imgLoader.LoadIcon("recording_cutted", fontHeight, fontHeight);
  imgLoader.LoadIcon("recording_cutted_cur", fontHeight, fontHeight);
  imgLoader.LoadIcon("recording", fontHeight, fontHeight);
  imgLoader.LoadIcon("folder", fontHeight, fontHeight);
  imgLoader.LoadIcon("recording_old", fontHeight, fontHeight);
  imgLoader.LoadIcon("recording_old_cur", fontHeight, fontHeight);
  imgLoader.LoadIcon("recording_old", fontSmlHeight, fontSmlHeight);
  imgLoader.LoadIcon("recording_old_cur", fontSmlHeight, fontSmlHeight);
}
