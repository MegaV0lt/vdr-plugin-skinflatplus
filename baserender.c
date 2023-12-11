#include "./baserender.h"

#include <vdr/menu.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fstream>
#include <iostream>
#include <utility>

#if VDRVERSNUM >= 20301
#include <future>
#endif

#include "./flat.h"

cFlatBaseRender::cFlatBaseRender(void) {
    g_Font = cFont::CreateFont(Setup.FontOsd, Setup.FontOsdSize);
    g_FontSml = cFont::CreateFont(Setup.FontSml, Setup.FontSmlSize);
    g_FontFixed = cFont::CreateFont(Setup.FontFix, Setup.FontFixSize);
    g_TopBarFontClock = NULL;

    g_FontHight = g_Font->Height();
    g_FontSmlHight = g_FontSml->Height();
    g_FontFixedHight = g_FontFixed->Height();

    g_TopBarTitle = "";
    g_TopBarTitleExtra1 = "";
    g_TopBarTitleExtra2 = "";
    g_TopBarLastDate = "";
    g_TopBarUpdateTitle = false;
    g_TopBarHeight = 0;
    g_TopBarExtraIconSet = false;
    g_TopBarMenuIcon = "";
    g_TopBarMenuIconSet = false;
    g_TopBarMenuIconRight = "";
    g_TopBarMenuIconRightSet = false;
    g_TopBarMenuLogo = "";
    g_TopBarMenuLogoSet = false;

    g_MarginItem = 5;

    g_ScrollBarWidth = Config.decorScrollBarSize;

    g_ButtonsHeight = 0;
    g_ButtonsDrawn = false;

    osd = NULL;
    TopBarPixmap = NULL;
    ButtonsPixmap = NULL;
    MessagePixmap = NULL;
    MessageIconPixmap = NULL;
    ContentPixmap = NULL;
    ContentEpgImagePixmap = NULL;
    ProgressBarPixmap = NULL;
    ProgressBarPixmapBg = NULL;
    DecorPixmap = NULL;

    Config.ThemeCheckAndInit();
    Config.DecorCheckAndInit();
}

cFlatBaseRender::~cFlatBaseRender(void) {
    delete g_Font;
    delete g_FontSml;
    delete g_FontFixed;
    if (g_TopBarFontClock) delete g_TopBarFontClock;

    if (osd) {
        MessageScroller.Clear();
        if (TopBarPixmap)
            osd->DestroyPixmap(TopBarPixmap);
        if (ButtonsPixmap)
            osd->DestroyPixmap(ButtonsPixmap);
        if (MessagePixmap)
            osd->DestroyPixmap(MessagePixmap);
        if (MessageIconPixmap)
            osd->DestroyPixmap(MessageIconPixmap);
        if (ContentPixmap)
            osd->DestroyPixmap(ContentPixmap);
        if (ProgressBarPixmap)
            osd->DestroyPixmap(ProgressBarPixmap);
        if (ProgressBarPixmapBg)
            osd->DestroyPixmap(ProgressBarPixmapBg);
        if (DecorPixmap)
            osd->DestroyPixmap(DecorPixmap);
        if (TopBarIconPixmap)
            osd->DestroyPixmap(TopBarIconPixmap);
        if (TopBarIconBgPixmap)
            osd->DestroyPixmap(TopBarIconBgPixmap);
        if (ContentEpgImagePixmap)
            osd->DestroyPixmap(ContentEpgImagePixmap);

        delete osd;
    }
}

void cFlatBaseRender::CreateFullOsd(void) {
    CreateOsd(cOsd::OsdLeft() + Config.marginOsdHor, cOsd::OsdTop() + Config.marginOsdVer,
              cOsd::OsdWidth() - Config.marginOsdHor * 2, cOsd::OsdHeight() - Config.marginOsdVer * 2);
}

void cFlatBaseRender::CreateOsd(int left, int top, int width, int height) {
    g_OsdLeft = left;
    g_OsdTop = top;
    g_OsdWidth = width;
    g_OsdHeight = height;

    osd = cOsdProvider::NewOsd(left, top);
    if (osd) {
        tArea Area = {0, 0, width, height, 32};
        if (osd->SetAreas(&Area, 1) == oeOk) {
            // dsyslog("flatPlus: Create osd SUCCESS left: %d top: %d width: %d height: %d", left, top, width, height);
            return;
        }
    }
    esyslog("flatPlus: Create osd FAILED left: %d top: %d width: %d height: %d", left, top, width, height);
    return;
}

void cFlatBaseRender::TopBarCreate(void) {
    int fs = round(cOsd::OsdHeight() * Config.TopBarFontSize);
    g_TopBarFont = cFont::CreateFont(Setup.FontOsd, fs);
    g_TopBarFontClock = cFont::CreateFont(Setup.FontOsd, fs * Config.TopBarFontClockScale * 100.0);
    g_TopBarFontSml = cFont::CreateFont(Setup.FontOsd, fs / 2);
    g_TopBarFontHeight = g_TopBarFont->Height();
    g_TopBarFontSmlHeight = g_TopBarFontSml->Height();
    g_TopBarFontClockHeight = g_TopBarFontClock->Height();

    if (g_TopBarFontHeight > g_TopBarFontSmlHeight * 2)
        g_TopBarHeight = g_TopBarFontHeight;
    else
        g_TopBarHeight = g_TopBarFontSmlHeight * 2;

    TopBarPixmap = CreatePixmap(osd, "TopBarPixmap", 1,
                                cRect(Config.decorBorderTopBarSize, Config.decorBorderTopBarSize,
                                      g_OsdWidth - Config.decorBorderTopBarSize * 2, g_TopBarHeight));
    // dsyslog("flatPlus: TopBarPixmap left: %d top: %d width: %d height: %d", Config.decorBorderTopBarSize,
    //         Config.decorBorderTopBarSize, g_OsdWidth - Config.decorBorderTopBarSize*2, g_TopBarHeight);
    TopBarIconBgPixmap = CreatePixmap(osd, "TopBarIconBgPixmap", 2,
                                      cRect(Config.decorBorderTopBarSize, Config.decorBorderTopBarSize,
                                            g_OsdWidth - Config.decorBorderTopBarSize * 2, g_TopBarHeight));
    // dsyslog("flatPlus: TopBarIconBgPixmap left: %d top: %d width: %d height: %d", Config.decorBorderTopBarSize,
    //         Config.decorBorderTopBarSize, g_OsdWidth - Config.decorBorderTopBarSize*2, g_TopBarHeight);
    TopBarIconPixmap = CreatePixmap(osd, "TopBarIconPixmap", 3,
                                    cRect(Config.decorBorderTopBarSize, Config.decorBorderTopBarSize,
                                          g_OsdWidth - Config.decorBorderTopBarSize * 2, g_TopBarHeight));
    // dsyslog("flatPlus: TopBarIconPixmap left: %d top: %d width: %d height: %d", Config.decorBorderTopBarSize,
    //         Config.decorBorderTopBarSize, g_OsdWidth - Config.decorBorderTopBarSize*2, g_TopBarHeight);
    PixmapFill(TopBarPixmap, clrTransparent);
    PixmapFill(TopBarIconBgPixmap, clrTransparent);
    PixmapFill(TopBarIconPixmap, clrTransparent);

    if (Config.DiskUsageShow == 3)
        TopBarEnableDiskUsage();
}

void cFlatBaseRender::TopBarSetTitle(cString title) {
    g_TopBarTitle = title;
    g_TopBarTitleExtra1 = "";
    g_TopBarTitleExtra2 = "";
    g_TopBarExtraIcon = "";
    g_TopBarMenuIcon = "";
    g_TopBarUpdateTitle = true;
    g_TopBarExtraIconSet = false;
    g_TopBarMenuIconSet = false;
    g_TopBarMenuLogo = "";
    g_TopBarMenuLogoSet = false;
    if (Config.DiskUsageShow == 3)
        TopBarEnableDiskUsage();
}

void cFlatBaseRender::TopBarSetTitleWithoutClear(cString title) {
    g_TopBarTitle = title;
    g_TopBarUpdateTitle = true;
    if (Config.DiskUsageShow == 3)
        TopBarEnableDiskUsage();
}

void cFlatBaseRender::TopBarSetTitleExtra(cString Extra1, cString Extra2) {
    g_TopBarTitleExtra1 = Extra1;
    g_TopBarTitleExtra2 = Extra2;
    g_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarSetExtraIcon(cString icon) {
    if (!strcmp(*icon, "")) return;

    g_TopBarExtraIcon = icon;
    g_TopBarExtraIconSet = true;
    g_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarSetMenuIcon(cString icon) {
    if (!strcmp(*icon, "")) return;

    g_TopBarMenuIcon = icon;
    g_TopBarMenuIconSet = true;
    g_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarSetMenuIconRight(cString icon) {
    if (!strcmp(*icon, "")) return;

    g_TopBarMenuIconRight = icon;
    g_TopBarMenuIconRightSet = true;
    g_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarClearMenuIconRight(void) {
    g_TopBarMenuIconRight = "";
    g_TopBarMenuIconRightSet = false;
}

void cFlatBaseRender::TopBarSetMenuLogo(cString icon) {
    if (!strcmp(*icon, "")) return;

    g_TopBarMenuLogo = icon;
    g_TopBarMenuLogoSet = true;
    g_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarEnableDiskUsage(void) {
    // cVideoDiskUsage::HasChanged(g_VideoDiskUsageState);    // Moved to cFlatDisplayMenu::cFlatDisplayMenu()
    int DiskUsagePercent = cVideoDiskUsage::UsedPercent();  // Used %
    int DiskFreePercent = (100 - DiskUsagePercent);         // Free %
    // Division is typically twice as slow as addition or multiplication. Rewrite divisions by a constant into a
    // multiplication with the inverse (For example, x = x / 3.0 becomes x = x * (1.0/3.0).
    // The constant is calculated during compilation.).
    double FreeGB = cVideoDiskUsage::FreeMB() * (1.0 / 1024.0);
    double AllGB = FreeGB / DiskFreePercent * (1.0 / 100.0);
    int FreeMinutes = cVideoDiskUsage::FreeMinutes();
    double AllMinutes = FreeMinutes / DiskFreePercent * (1.0 / 100.0);
    cString IconName("");
    cString Extra1(""), Extra2("");

    if (Config.DiskUsageFree == 1) {              // Show in free mode
        if (Config.DiskUsageShort == false) {     // Long format
            Extra1 = cString::sprintf("%s: %d%% %s", tr("Disk"), DiskFreePercent, tr("free"));
            if (FreeGB < 1000.0) {  // Less than 1000 GB
                Extra2 = cString::sprintf("%.1f GB ~%02d:%02d", FreeGB, FreeMinutes / 60, FreeMinutes % 60);
            } else {  // 1000 GB+
                Extra2 = cString::sprintf("%.2f TB ~%02d:%02d", FreeGB * (1.0 / 1024.0), FreeMinutes / 60,
                                          FreeMinutes % 60);
            }
        } else {  // Short format
            Extra1 = cString::sprintf("%d%% %s", DiskFreePercent, tr("free"));
            Extra2 = cString::sprintf("~%02d:%02d", FreeMinutes / 60, FreeMinutes % 60);
        }
        switch (DiskFreePercent) {  // Show free space
        case 0 ... 2: IconName = "chart0b"; break;  // < 2% (chart1b in red)
        case 3 ... 4: IconName = "chart1b"; break;  // 3,125 (4)
        case 5 ... 6: IconName = "chart2b"; break;  // 6,25
        case 7 ... 9: IconName = "chart3b"; break;  // 9,375
        case 10 ... 13: IconName = "chart4b"; break;  // 12,5
        case 14 ... 16: IconName = "chart5b"; break;  // 15,625
        case 17 ... 19: IconName = "chart6b"; break;  // 18,75
        case 20 ... 22: IconName = "chart7b"; break;  // 21,875
        case 23 ... 25: IconName = "chart8b"; break;  // 25
        case 26 ... 28: IconName = "chart9b"; break;  // 28,125
        case 29 ... 31: IconName = "chart10b"; break;  // 31,25
        case 32 ... 34: IconName = "chart11b"; break;  // 34,375
        case 35 ... 38: IconName = "chart12b"; break;  // 37,5
        case 39 ... 41: IconName = "chart13b"; break;  // 40,625
        case 42 ... 44: IconName = "chart14b"; break;  // 43,75
        case 45 ... 47: IconName = "chart15b"; break;  // 46,875
        case 48 ... 50: IconName = "chart16b"; break;  // 50
        case 51 ... 53: IconName = "chart17b"; break;  // 53,125
        case 54 ... 56: IconName = "chart18b"; break;  // 56,25
        case 57 ... 59: IconName = "chart19b"; break;  // 59,375
        case 60 ... 63: IconName = "chart20b"; break;  // 62,5
        case 64 ... 66: IconName = "chart21b"; break;  // 65,625
        case 67 ... 69: IconName = "chart22b"; break;  // 68,75
        case 70 ... 72: IconName = "chart23b"; break;  // 71,875
        case 73 ... 75: IconName = "chart24b"; break;  // 75
        case 76 ... 78: IconName = "chart25b"; break;  // 78,125
        case 79 ... 81: IconName = "chart26b"; break;  // 81,25
        case 82 ... 84: IconName = "chart27b"; break;  // 84,375
        case 85 ... 88: IconName = "chart28b"; break;  // 87,5
        case 89 ... 91: IconName = "chart29b"; break;  // 90,625
        case 92 ... 94: IconName = "chart30b"; break;  // 93,75
        case 95 ... 100: IconName = "chart31b"; break;  // 96,875 - 100
        }
    } else {  // Show in occupied mode
        double OccupiedGB = AllGB - FreeGB;
        int OccupiedMinutes = AllMinutes - FreeMinutes;
        if (Config.DiskUsageShort == false) {  // Long format
            Extra1 = cString::sprintf("%s: %d%% %s", tr("Disk"), DiskUsagePercent, tr("occupied"));
            if (OccupiedGB < 1000.0) {  // Less than 1000 GB
                Extra2 =
                    cString::sprintf("%.1f GB ~%02d:%02d", OccupiedGB, OccupiedMinutes / 60, OccupiedMinutes % 60);
            } else {  // 1000 GB+
                Extra2 = cString::sprintf("%.2f TB ~%02d:%02d", OccupiedGB * (1.0 / 1024.0), OccupiedMinutes / 60,
                                          OccupiedMinutes % 60);
            }
        } else {  // Short format
            Extra1 = cString::sprintf("%d%% %s", DiskUsagePercent, tr("occupied"));
            Extra2 = cString::sprintf("~%02d:%02d", OccupiedMinutes / 60, OccupiedMinutes % 60);
        }
        switch (DiskUsagePercent) {  // show used space
        case 0 ... 3: IconName = "chart1"; break;  // 3,125
        case 4 ... 6: IconName = "chart2"; break;  // 6,25
        case 7 ... 9: IconName = "chart3"; break;  // 9,375
        case 10 ... 13: IconName = "chart4"; break;  // 12,5
        case 14 ... 16: IconName = "chart5"; break;  // 15,625
        case 17 ... 19: IconName = "chart6"; break;  // 18,75
        case 20 ... 22: IconName = "chart7"; break;  // 21,875
        case 23 ... 25: IconName = "chart8"; break;  // 25
        case 26 ... 28: IconName = "chart9"; break;  // 28,125
        case 29 ... 31: IconName = "chart10"; break;  // 31,25
        case 32 ... 34: IconName = "chart11"; break;  // 34,375
        case 35 ... 38: IconName = "chart12"; break;  // 37,5
        case 39 ... 41: IconName = "chart13"; break;  // 40,625
        case 42 ... 44: IconName = "chart14"; break;  // 43,75
        case 45 ... 47: IconName = "chart15"; break;  // 46,875
        case 48 ... 50: IconName = "chart16"; break;  // 50
        case 51 ... 53: IconName = "chart17"; break;  // 53,125
        case 54 ... 56: IconName = "chart18"; break;  // 56,25
        case 57 ... 59: IconName = "chart19"; break;  // 59,375
        case 60 ... 63: IconName = "chart20"; break;  // 62,5
        case 64 ... 66: IconName = "chart21"; break;  // 65,625
        case 67 ... 69: IconName = "chart22"; break;  // 68,75
        case 70 ... 72: IconName = "chart23"; break;  // 71,875
        case 73 ... 75: IconName = "chart24"; break;  // 75
        case 76 ... 78: IconName = "chart25"; break;  // 78,125
        case 79 ... 81: IconName = "chart26"; break;  // 81,25
        case 82 ... 84: IconName = "chart27"; break;  // 84,375
        case 85 ... 88: IconName = "chart28"; break;  // 87,5
        case 89 ... 91: IconName = "chart29"; break;  // 90,625
        case 92 ... 94: IconName = "chart30"; break;  // 93,75
        case 95 ... 97: IconName = "chart31"; break;  // 96,875
        case 98 ... 100: IconName = "chart32"; break;  // > 98% (chart31 in red)
        }
    }
    TopBarSetTitleExtra(*Extra1, *Extra2);
    TopBarSetExtraIcon(*IconName);
}
// Should be called with every "Flush"!
void cFlatBaseRender::TopBarUpdate(void) {
    cString Buffer(""), CurDate = DayDateTime();
    int TopBarWidth = g_OsdWidth - Config.decorBorderTopBarSize * 2;
    int MenuIconWidth {0};

    if (strcmp(CurDate, g_TopBarLastDate) || g_TopBarUpdateTitle) {
        g_TopBarUpdateTitle = false;
        g_TopBarLastDate = CurDate;

        int FontTop = (g_TopBarHeight - g_TopBarFontHeight) / 2;
        int FontSmlTop = (g_TopBarHeight - g_TopBarFontSmlHeight * 2) / 2;
        int FontClockTop = (g_TopBarHeight - g_TopBarFontClockHeight) / 2;

        PixmapFill(TopBarPixmap, Theme.Color(clrTopBarBg));
        PixmapFill(TopBarIconPixmap, clrTransparent);
        PixmapFill(TopBarIconBgPixmap, clrTransparent);

        cImage *img = NULL;
        if (g_TopBarMenuIconSet && Config.TopBarMenuIconShow) {
            int IconLeft = g_MarginItem;
            img = imgLoader.LoadIcon(*g_TopBarMenuIcon, 999, g_TopBarHeight - g_MarginItem * 2);
            if (img) {
                int IconTop = (g_TopBarHeight / 2 - img->Height() / 2);
                TopBarIconPixmap->DrawImage(cPoint(IconLeft, IconTop), *img);
                MenuIconWidth = img->Width() + g_MarginItem * 2;
            }
        }

        if (g_TopBarMenuLogoSet && Config.TopBarMenuIconShow) {
            PixmapFill(TopBarIconPixmap, clrTransparent);
            int IconLeft = g_MarginItem;
            int ImageBGHeight = g_TopBarHeight - g_MarginItem * 2;
            int ImageBGWidth = ImageBGHeight * 1.34;
            int IconTop {0};

            img = imgLoader.LoadIcon("logo_background", ImageBGWidth, ImageBGHeight);
            if (img) {
                ImageBGHeight = img->Height();
                ImageBGWidth = img->Width();
                IconTop = (g_TopBarHeight / 2 - ImageBGHeight / 2);
                TopBarIconBgPixmap->DrawImage(cPoint(IconLeft, IconTop), *img);
            }

            img = imgLoader.LoadLogo(*g_TopBarMenuLogo, ImageBGWidth - 4, ImageBGHeight - 4);
            if (img) {
                IconTop += (ImageBGHeight - img->Height()) / 2;
                IconLeft += (ImageBGWidth - img->Width()) / 2;
                TopBarIconPixmap->DrawImage(cPoint(IconLeft, IconTop), *img);
            }
            MenuIconWidth = ImageBGWidth + g_MarginItem * 2;
        }
        int TitleLeft = MenuIconWidth + g_MarginItem * 2;

        time_t t = time(NULL);
        cString time = TimeString(t);
        Buffer = cString::sprintf("%s %s", *time, tr("clock"));
        if (Config.TopBarHideClockText)
            Buffer = cString::sprintf("%s", *time);

        int TimeWidth = g_TopBarFontClock->Width(*Buffer) + g_MarginItem * 2;
        int Right = TopBarWidth - TimeWidth;
        TopBarPixmap->DrawText(cPoint(Right, FontClockTop), *Buffer, Theme.Color(clrTopBarTimeFont),
                               Theme.Color(clrTopBarBg), g_TopBarFontClock);

        cString weekday = WeekDayNameFull(t);
        int WeekdayWidth = g_TopBarFontSml->Width(*weekday);

        cString date = ShortDateString(t);
        int DateWidth = g_TopBarFontSml->Width(*date);

        Right = TopBarWidth - TimeWidth - std::max(WeekdayWidth, DateWidth) - g_MarginItem;
        TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *weekday, Theme.Color(clrTopBarDateFont),
                               Theme.Color(clrTopBarBg), g_TopBarFontSml, std::max(WeekdayWidth, DateWidth), 0, taRight);
        TopBarPixmap->DrawText(cPoint(Right, FontSmlTop + g_TopBarFontSmlHeight), *date, Theme.Color(clrTopBarDateFont),
                               Theme.Color(clrTopBarBg), g_TopBarFontSml, std::max(WeekdayWidth, DateWidth), 0, taRight);

        int MiddleWidth {0}, NumConflicts {0};
        cImage *ImgCon = NULL, *ImgRec = NULL;
        if (Config.TopBarRecConflictsShow) {
            NumConflicts = GetEpgsearchConflichts();  // Get conflicts from plugin Epgsearch
            if (NumConflicts) {
                if (NumConflicts < Config.TopBarRecConflictsHigh)
                    ImgCon = imgLoader.LoadIcon("topbar_timerconflict_low", g_TopBarFontHeight - g_MarginItem * 2,
                                                g_TopBarFontHeight - g_MarginItem * 2);
                else
                    ImgCon = imgLoader.LoadIcon("topbar_timerconflict_high", g_TopBarFontHeight - g_MarginItem * 2,
                                                g_TopBarFontHeight - g_MarginItem * 2);

                if (ImgCon) {
                    Buffer = cString::sprintf("%d", NumConflicts);
                    Right -= ImgCon->Width() + g_TopBarFontSml->Width(*Buffer) + g_MarginItem;
                    MiddleWidth += ImgCon->Width() + g_TopBarFontSml->Width(*Buffer) + g_MarginItem;
                }
            }
        }  // Config.TopBarRecConflictsShow

        int NumRec {0};
        if (Config.TopBarRecordingShow) {
// Look for timers
#if VDRVERSNUM >= 20301
            auto recCounterFuture = std::async([&NumRec]() {
                LOCK_TIMERS_READ;
                for (const cTimer *ti = Timers->First(); ti; ti = Timers->Next(ti)) {
                    if (ti->HasFlags(tfRecording))
                        ++NumRec;
                }
            });
            recCounterFuture.get();
#else
            for (cTimer *ti = Timers.First(); ti; ti = Timers.Next(ti))
                if (ti->HasFlags(tfRecording))
                    ++NumRec;
#endif
            if (NumRec) {
                ImgRec = imgLoader.LoadIcon("topbar_timer", g_TopBarFontHeight - g_MarginItem * 2,
                                            g_TopBarFontHeight - g_MarginItem * 2);
                if (ImgRec) {
                    Buffer = cString::sprintf("%d", NumRec);
                    Right -= ImgRec->Width() + g_TopBarFontSml->Width(*Buffer) + g_MarginItem;
                    MiddleWidth += ImgRec->Width() + g_TopBarFontSml->Width(*Buffer) + g_MarginItem;
                }
            }
        }  // Config.TopBarRecordingShow

        if (g_TopBarExtraIconSet) {
            img = imgLoader.LoadIcon(*g_TopBarExtraIcon, 999, g_TopBarHeight);
            if (img) {
                Right -= img->Width() + g_MarginItem;
                MiddleWidth += img->Width() + g_MarginItem;
            }
        }
        int TopBarMenuIconRightWidth {0};
        int TopBarMenuIconRightLeft {0};
        int TitleWidth = g_TopBarFont->Width(*g_TopBarTitle);
        if (g_TopBarMenuIconRightSet) {
            img = imgLoader.LoadIcon(*g_TopBarMenuIconRight, 999, g_TopBarHeight);
            if (img) {
                TopBarMenuIconRightWidth = img->Width() + g_MarginItem * 3;
                TitleWidth += TopBarMenuIconRightWidth;
            }
        }

        int Extra1Width = g_TopBarFontSml->Width(*g_TopBarTitleExtra1);
        int Extra2Width = g_TopBarFontSml->Width(*g_TopBarTitleExtra2);
        int ExtraMaxWidth = std::max(Extra1Width, Extra2Width);
        MiddleWidth += ExtraMaxWidth;
        Right -= ExtraMaxWidth + g_MarginItem;

        if ((TitleLeft + TitleWidth) < (TopBarWidth / 2 - MiddleWidth / 2)) {
            Right = TopBarWidth / 2 - MiddleWidth / 2;
        } else if ((TitleLeft + TitleWidth) < Right) {
            Right = TitleLeft + TitleWidth + g_MarginItem;
        }

        int TitleMaxWidth = Right - TitleLeft - g_MarginItem;
        if (TitleWidth + TopBarMenuIconRightWidth > TitleMaxWidth) {
            TopBarMenuIconRightLeft = TitleMaxWidth + g_MarginItem * 2;
            TitleMaxWidth -= TopBarMenuIconRightWidth;
        } else {
            TopBarMenuIconRightLeft = TitleLeft + TitleWidth + g_MarginItem * 2;
        }

        TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *g_TopBarTitleExtra1, Theme.Color(clrTopBarDateFont),
                               Theme.Color(clrTopBarBg), g_TopBarFontSml, ExtraMaxWidth, 0, taRight);
        TopBarPixmap->DrawText(cPoint(Right, FontSmlTop + g_TopBarFontSmlHeight), *g_TopBarTitleExtra2,
                               Theme.Color(clrTopBarDateFont), Theme.Color(clrTopBarBg), g_TopBarFontSml, ExtraMaxWidth,
                               0, taRight);
        Right += ExtraMaxWidth + g_MarginItem;

        if (g_TopBarExtraIconSet) {
            img = imgLoader.LoadIcon(*g_TopBarExtraIcon, 999, g_TopBarHeight);
            if (img) {
                int IconTop {0};
                TopBarIconPixmap->DrawImage(cPoint(Right, IconTop), *img);
                Right += img->Width() + g_MarginItem;
            }
        }

        if (NumRec && ImgRec) {
            int IconTop = (g_TopBarFontHeight - ImgRec->Height()) / 2;
            TopBarIconPixmap->DrawImage(cPoint(Right, IconTop), *ImgRec);
            Right += ImgRec->Width();
            Buffer = cString::sprintf("%d", NumRec);
            TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *Buffer, Theme.Color(clrTopBarRecordingActiveFg),
                                   Theme.Color(clrTopBarRecordingActiveBg), g_TopBarFontSml);
            Right += g_TopBarFontSml->Width(*Buffer) + g_MarginItem;
        }

        if (NumConflicts && ImgCon) {
            int IconTop = (g_TopBarFontHeight - ImgCon->Height()) / 2;
            TopBarIconPixmap->DrawImage(cPoint(Right, IconTop), *ImgCon);
            Right += ImgCon->Width();

            Buffer = cString::sprintf("%d", NumConflicts);
            if (NumConflicts < Config.TopBarRecConflictsHigh)
                TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *Buffer, Theme.Color(clrTopBarConflictLowFg),
                                       Theme.Color(clrTopBarConflictLowBg), g_TopBarFontSml);
            else
                TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *Buffer, Theme.Color(clrTopBarConflictHighFg),
                                       Theme.Color(clrTopBarConflictHighBg), g_TopBarFontSml);
            Right += g_TopBarFontSml->Width(*Buffer) + g_MarginItem;
        }

        if (g_TopBarMenuIconRightSet) {
            img = imgLoader.LoadIcon(*g_TopBarMenuIconRight, 999, g_TopBarHeight);
            if (img) {
                int IconTop = (g_TopBarHeight / 2 - img->Height() / 2);
                TopBarIconPixmap->DrawImage(cPoint(TopBarMenuIconRightLeft, IconTop), *img);
            }
        }
        TopBarPixmap->DrawText(cPoint(TitleLeft, FontTop), *g_TopBarTitle, Theme.Color(clrTopBarFont),
                               Theme.Color(clrTopBarBg), g_TopBarFont, TitleMaxWidth);

        DecorBorderDraw(Config.decorBorderTopBarSize, Config.decorBorderTopBarSize,
                        g_OsdWidth - Config.decorBorderTopBarSize * 2, g_TopBarHeight, Config.decorBorderTopBarSize,
                        Config.decorBorderTopBarType, Config.decorBorderTopBarFg, Config.decorBorderTopBarBg);
    }
}

void cFlatBaseRender::ButtonsCreate(void) {
    g_MarginButtonColor = 10;
    g_ButtonColorHeight = 8;
    g_ButtonsHeight = g_FontHight + g_MarginButtonColor + g_ButtonColorHeight;
    g_ButtonsWidth = g_OsdWidth;
    g_ButtonsTop = g_OsdHeight - g_ButtonsHeight - Config.decorBorderButtonSize;

    ButtonsPixmap = CreatePixmap(osd, "ButtonsPixmap", 1,
                                 cRect(Config.decorBorderButtonSize, g_ButtonsTop,
                                       g_ButtonsWidth - Config.decorBorderButtonSize * 2, g_ButtonsHeight));
    PixmapFill(ButtonsPixmap, clrTransparent);
    // dsyslog("flatPlus: ButtonsPixmap left: %d top: %d width: %d height: %d",
    //         Config.decorBorderButtonSize, g_ButtonsTop, g_ButtonsWidth - Config.decorBorderButtonSize*2, g_ButtonsHeight);
}

void cFlatBaseRender::ButtonsSet(const char *Red, const char *Green, const char *Yellow, const char *Blue) {
    int WidthMargin = g_ButtonsWidth - g_MarginItem * 3;
    int ButtonWidth = (WidthMargin / 4) - Config.decorBorderButtonSize * 2;

    PixmapFill(ButtonsPixmap, clrTransparent);
    DecorBorderClearByFrom(BorderButton);

    g_ButtonsDrawn = false;

    int x {0};
    if (!(!Config.ButtonsShowEmpty && !Red)) {
        switch (Setup.ColorKey0) {
        case 0:
            ButtonsPixmap->DrawText(cPoint(x, 0), Red, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonRed));
            break;
        case 1:
            ButtonsPixmap->DrawText(cPoint(x, 0), Green, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonGreen));
            break;
        case 2:
            ButtonsPixmap->DrawText(cPoint(x, 0), Yellow, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonYellow));
            break;
        case 3:
            ButtonsPixmap->DrawText(cPoint(x, 0), Blue, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonBlue));
            break;
        }
        DecorBorderDraw(x + Config.decorBorderButtonSize, g_ButtonsTop, ButtonWidth, g_ButtonsHeight,
                        Config.decorBorderButtonSize, Config.decorBorderButtonType, Config.decorBorderButtonFg,
                        Config.decorBorderButtonBg, BorderButton);
        g_ButtonsDrawn = true;
    }

    x += ButtonWidth + g_MarginItem + Config.decorBorderButtonSize * 2;
    if (!(!Config.ButtonsShowEmpty && !Green)) {
        switch (Setup.ColorKey1) {
        case 0:
            ButtonsPixmap->DrawText(cPoint(x, 0), Red, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonRed));
            break;
        case 1:
            ButtonsPixmap->DrawText(cPoint(x, 0), Green, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonGreen));
            break;
        case 2:
            ButtonsPixmap->DrawText(cPoint(x, 0), Yellow, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonYellow));
            break;
        case 3:
            ButtonsPixmap->DrawText(cPoint(x, 0), Blue, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonBlue));
            break;
        }

        DecorBorderDraw(x + Config.decorBorderButtonSize, g_ButtonsTop, ButtonWidth, g_ButtonsHeight,
                        Config.decorBorderButtonSize, Config.decorBorderButtonType, Config.decorBorderButtonFg,
                        Config.decorBorderButtonBg, BorderButton);
        g_ButtonsDrawn = true;
    }

    x += ButtonWidth + g_MarginItem + Config.decorBorderButtonSize * 2;
    if (!(!Config.ButtonsShowEmpty && !Yellow)) {
        switch (Setup.ColorKey2) {
        case 0:
            ButtonsPixmap->DrawText(cPoint(x, 0), Red, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonRed));
            break;
        case 1:
            ButtonsPixmap->DrawText(cPoint(x, 0), Green, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonGreen));
            break;
        case 2:
            ButtonsPixmap->DrawText(cPoint(x, 0), Yellow, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonYellow));
            break;
        case 3:
            ButtonsPixmap->DrawText(cPoint(x, 0), Blue, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonBlue));
            break;
        }

        DecorBorderDraw(x + Config.decorBorderButtonSize, g_ButtonsTop, ButtonWidth, g_ButtonsHeight,
                        Config.decorBorderButtonSize, Config.decorBorderButtonType, Config.decorBorderButtonFg,
                        Config.decorBorderButtonBg, BorderButton);
        g_ButtonsDrawn = true;
    }

    x += ButtonWidth + g_MarginItem + Config.decorBorderButtonSize * 2;
    if (x + ButtonWidth + Config.decorBorderButtonSize * 2 < g_ButtonsWidth)
        ButtonWidth += g_ButtonsWidth - (x + ButtonWidth + Config.decorBorderButtonSize * 2);
    if (!(!Config.ButtonsShowEmpty && !Blue)) {
        switch (Setup.ColorKey3) {
        case 0:
            ButtonsPixmap->DrawText(cPoint(x, 0), Red, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonRed));
            break;
        case 1:
            ButtonsPixmap->DrawText(cPoint(x, 0), Green, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonGreen));
            break;
        case 2:
            ButtonsPixmap->DrawText(cPoint(x, 0), Yellow, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonYellow));
            break;
        case 3:
            ButtonsPixmap->DrawText(cPoint(x, 0), Blue, Theme.Color(clrButtonFont), Theme.Color(clrButtonBg), g_Font,
                                    ButtonWidth, g_FontHight + g_MarginButtonColor, taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, g_FontHight + g_MarginButtonColor, ButtonWidth, g_ButtonColorHeight),
                                         Theme.Color(clrButtonBlue));
            break;
        }

        DecorBorderDraw(x + Config.decorBorderButtonSize, g_ButtonsTop, ButtonWidth, g_ButtonsHeight,
                        Config.decorBorderButtonSize, Config.decorBorderButtonType, Config.decorBorderButtonFg,
                        Config.decorBorderButtonBg, BorderButton);
        g_ButtonsDrawn = true;
    }
}

bool cFlatBaseRender::ButtonsDrawn(void) {
    return g_ButtonsDrawn;
}

void cFlatBaseRender::MessageCreate(void) {
    MessageHeight = g_FontHight + g_MarginItem * 2;
    if (Config.MessageColorPosition == 1)
        MessageHeight += 8;

    int top = g_OsdHeight - Config.MessageOffset - MessageHeight - Config.decorBorderMessageSize;
    MessagePixmap = CreatePixmap(
        osd, "MessagePixmap", 5,
        cRect(Config.decorBorderMessageSize, top, g_OsdWidth - Config.decorBorderMessageSize * 2, MessageHeight));
    PixmapFill(MessagePixmap, clrTransparent);
    MessageIconPixmap = CreatePixmap(
        osd, "MessageIconPixmap", 5,
        cRect(Config.decorBorderMessageSize, top, g_OsdWidth - Config.decorBorderMessageSize * 2, MessageHeight));
    PixmapFill(MessageIconPixmap, clrTransparent);
    // dsyslog("flatPlus: MessagePixmap left: %d top: %d width: %d height: %d", Config.decorBorderMessageSize,
    //         top, g_OsdWidth - Config.decorBorderMessageSize*2, MessageHeight);
    // dsyslog("flatPlus: MessageIconPixmap left: %d top: %d width: %d height: %d", Config.decorBorderMessageSize,
    //         top, g_OsdWidth - Config.decorBorderMessageSize*2, MessageHeight);

    MessageScroller.SetOsd(osd);
    MessageScroller.SetScrollStep(Config.ScrollerStep);
    MessageScroller.SetScrollDelay(Config.ScrollerDelay);
    MessageScroller.SetScrollType(Config.ScrollerType);
    MessageScroller.SetPixmapLayer(5);
}

void cFlatBaseRender::MessageSet(eMessageType Type, const char *Text) {
    tColor Col = Theme.Color(clrMessageInfo);
    cString Icon("message_info");
    switch (Type) {
    case mtInfo: // Already preset
        break;
    case mtWarning:
        Col = Theme.Color(clrMessageWarning);
        Icon = "message_warning";
        break;
    case mtStatus:
        Col = Theme.Color(clrMessageStatus);
        Icon = "message_status";
        break;
    case mtError:
        Col = Theme.Color(clrMessageError);
        Icon = "message_error";
        break;
    }
    PixmapFill(MessagePixmap, Theme.Color(clrMessageBg));
    MessageScroller.Clear();

    cImage *img = imgLoader.LoadIcon(*Icon, g_FontHight, g_FontHight);
    if (img)
        MessageIconPixmap->DrawImage(cPoint(g_MarginItem + 10, g_MarginItem), *img);

    if (Config.MessageColorPosition == 0) {
        MessagePixmap->DrawRectangle(cRect(0, 0, 8, MessageHeight), Col);
        MessagePixmap->DrawRectangle(cRect(g_OsdWidth - 8 - Config.decorBorderMessageSize * 2, 0, 8, MessageHeight), Col);
    } else {
        MessagePixmap->DrawRectangle(cRect(0, MessageHeight - 8, g_OsdWidth, 8), Col);
    }

    int TextWidth = g_Font->Width(Text);
    int MaxWidth = g_OsdWidth - Config.decorBorderMessageSize * 2 - g_FontHight - g_MarginItem * 3 - 10;

    if ((TextWidth > MaxWidth) && Config.ScrollerEnable) {
        MessageScroller.AddScroller(
            Text,
            cRect(Config.decorBorderMessageSize + g_FontHight + g_MarginItem * 3 + 10,
                  g_OsdHeight - Config.MessageOffset - MessageHeight - Config.decorBorderMessageSize + g_MarginItem,
                  MaxWidth, g_FontHight),
            Theme.Color(clrMessageFont), clrTransparent, g_Font, Theme.Color(clrMenuItemExtraTextFont));
    } else if (Config.MenuItemParseTilde) {
        std::string tilde = Text;
        size_t found = tilde.find('~');  // Search for ~
        if (found != std::string::npos) {
            std::string first = tilde.substr(0, found);
            std::string second = tilde.substr(found + 1);  // Default end is npos
            RightTrim(first);   // Trim possible space on right side
            LeftTrim(second);  // Trim possible space at begin

            MessagePixmap->DrawText(cPoint((g_OsdWidth - TextWidth) / 2, g_MarginItem), first.c_str(),
                                    Theme.Color(clrMessageFont), Theme.Color(clrMessageBg), g_Font);
            int l = g_Font->Width(first.c_str()) + g_Font->Width('X');
            MessagePixmap->DrawText(cPoint((g_OsdWidth - TextWidth) / 2 + l, g_MarginItem), second.c_str(),
                                    Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrMessageBg), g_Font);
        } else {  // ~ not found
            if ((TextWidth > MaxWidth) && Config.ScrollerEnable)
                MessageScroller.AddScroller(
                    Text,
                    cRect(Config.decorBorderMessageSize + g_FontHight + g_MarginItem * 3 + 10,
                          g_OsdHeight - Config.MessageOffset - MessageHeight - Config.decorBorderMessageSize + g_MarginItem,
                          MaxWidth, g_FontHight),
                    Theme.Color(clrMessageFont), clrTransparent, g_Font);
            else
                MessagePixmap->DrawText(cPoint((g_OsdWidth - TextWidth) / 2, g_MarginItem), Text,
                                        Theme.Color(clrMessageFont), Theme.Color(clrMessageBg), g_Font);
        }
    } else {
        if ((TextWidth > MaxWidth) && Config.ScrollerEnable)
            MessageScroller.AddScroller(
                Text,
                cRect(Config.decorBorderMessageSize + g_FontHight + g_MarginItem * 3 + 10,
                      g_OsdHeight - Config.MessageOffset - MessageHeight - Config.decorBorderMessageSize + g_MarginItem,
                      MaxWidth, g_FontHight),
                Theme.Color(clrMessageFont), clrTransparent, g_Font);
        else
            MessagePixmap->DrawText(cPoint((g_OsdWidth - TextWidth) / 2, g_MarginItem), Text, Theme.Color(clrMessageFont),
                                    Theme.Color(clrMessageBg), g_Font);
    }

    int top = g_OsdHeight - Config.MessageOffset - MessageHeight - Config.decorBorderMessageSize;
    DecorBorderDraw(Config.decorBorderMessageSize, top, g_OsdWidth - Config.decorBorderMessageSize * 2, MessageHeight,
                    Config.decorBorderMessageSize, Config.decorBorderMessageType, Config.decorBorderMessageFg,
                    Config.decorBorderMessageBg, BorderMessage);
}

void cFlatBaseRender::MessageClear(void) {
    PixmapFill(MessagePixmap, clrTransparent);
    PixmapFill(MessageIconPixmap, clrTransparent);
    DecorBorderClearByFrom(BorderMessage);
    DecorBorderRedrawAll();
    MessageScroller.Clear();
}

void cFlatBaseRender::ProgressBarCreate(int Left, int Top, int Width, int Height, int MarginHor, int MarginVer,
                                        tColor ColorFg, tColor ColorBarFg, tColor ColorBg, int Type, bool SetBackground,
                                        bool IsSignal) {
    g_ProgressBarTop = Top;
    g_ProgressBarWidth = Width;
    g_ProgressBarHeight = Height;
    g_ProgressType = Type;
    g_ProgressBarMarginHor = MarginHor;
    g_ProgressBarMarginVer = MarginVer;

    g_ProgressBarColorFg = ColorFg;
    g_ProgressBarColorBarFg = ColorBarFg;
    g_ProgressBarColorBg = ColorBg;

    g_ProgressBarSetBackground = SetBackground;
    g_ProgressBarIsSignal = IsSignal;

    g_ProgressBarColorBarCurFg = Theme.Color(clrReplayProgressBarCurFg);

    ProgressBarPixmap =
        CreatePixmap(osd, "ProgressBarPixmap", 3, cRect(Left, g_ProgressBarTop, g_ProgressBarWidth, g_ProgressBarHeight));
    ProgressBarPixmapBg =
        CreatePixmap(osd, "ProgressBarPixmapBg", 2,
                     cRect(Left - g_ProgressBarMarginVer, g_ProgressBarTop - g_ProgressBarMarginHor,
                           g_ProgressBarWidth + g_ProgressBarMarginVer * 2, g_ProgressBarHeight + g_ProgressBarMarginHor * 2));
    PixmapFill(ProgressBarPixmap, clrTransparent);
    PixmapFill(ProgressBarPixmapBg, clrTransparent);
}

void cFlatBaseRender::ProgressBarDraw(int Current, int Total) {
    ProgressBarDrawRaw(
        ProgressBarPixmap, ProgressBarPixmapBg, cRect(0, 0, g_ProgressBarWidth, g_ProgressBarHeight),
        cRect(0, 0, g_ProgressBarWidth + g_ProgressBarMarginVer * 2, g_ProgressBarHeight + g_ProgressBarMarginHor * 2), Current,
        Total, g_ProgressBarColorFg, g_ProgressBarColorBarFg, g_ProgressBarColorBg, g_ProgressType, g_ProgressBarSetBackground,
        g_ProgressBarIsSignal);
}

void cFlatBaseRender::ProgressBarDrawBgColor(void) {
    PixmapFill(ProgressBarPixmapBg, g_ProgressBarColorBg);
}

void cFlatBaseRender::ProgressBarDrawRaw(cPixmap *Pixmap, cPixmap *PixmapBg, cRect rect, cRect rectBg, int Current,
                                         int Total, tColor ColorFg, tColor ColorBarFg, tColor ColorBg, int Type,
                                         bool SetBackground, bool IsSignal) {
    int Middle = rect.Height() / 2;
    double PercentLeft = Current * 1.0 / Total;

    if (PixmapBg && SetBackground)
        PixmapBg->DrawRectangle(rectBg, ColorBg);

    if (SetBackground) {
        if (PixmapBg == Pixmap)
            Pixmap->DrawRectangle(rect, ColorBg);
        else
            Pixmap->DrawRectangle(rect, clrTransparent);
    }
    switch (Type) {
    case 0:  // Small line + big line
    {
        int sml = rect.Height() / 10 * 2;
        if (sml <= 1) sml = 2;
        int big = rect.Height();

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + Middle - (sml / 2), rect.Width(), sml), ColorFg);

        if (Current > 0)
            Pixmap->DrawRectangle(
                cRect(rect.Left(), rect.Top() + Middle - (big / 2), (rect.Width() * PercentLeft), big), ColorBarFg);
        break;
    }
    case 1:  // big line
    {
        int big = rect.Height();
        if (Current > 0)
            Pixmap->DrawRectangle(
                cRect(rect.Left(), rect.Top() + Middle - (big / 2), (rect.Width() * PercentLeft), big), ColorBarFg);
        break;
    }
    case 2:  // big line + outline
    {
        int big = rect.Height();
        int out {1};
        if (big > 10) out = 2;
        // Outline
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + big - out, rect.Width(), out), ColorFg);

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), out, big), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left() + rect.Width() - out, rect.Top(), out, big), ColorFg);

        if (Current > 0) {
            if (IsSignal) {
                double perc = 100.0 / Total * Current * (1.0 / 100.0);
                if (perc > 0.666) {
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + Middle - (big / 2) + out,
                                                (rect.Width() * PercentLeft) - out * 2, big - out * 2),
                                          Theme.Color(clrButtonGreen));
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + Middle - (big / 2) + out,
                                                (rect.Width() * 0.666) - out * 2, big - out * 2),
                                          Theme.Color(clrButtonYellow));
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + Middle - (big / 2) + out,
                                                (rect.Width() * 0.333) - out * 2, big - out * 2),
                                          Theme.Color(clrButtonRed));
                } else if (perc > 0.333) {
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + Middle - (big / 2) + out,
                                                (rect.Width() * PercentLeft) - out * 2, big - out * 2),
                                          Theme.Color(clrButtonYellow));
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + Middle - (big / 2) + out,
                                                (rect.Width() * 0.333) - out * 2, big - out * 2),
                                          Theme.Color(clrButtonRed));
                } else
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + Middle - (big / 2) + out,
                                                (rect.Width() * PercentLeft) - out * 2, big - out * 2),
                                          Theme.Color(clrButtonRed));
            } else
                Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + Middle - (big / 2) + out,
                                            (rect.Width() * PercentLeft) - out * 2, big - out * 2),
                                      ColorBarFg);
        }
        break;
    }
    case 3:  // Small line + big line + dot
    {
        int sml = rect.Height() / 10 * 2;
        if (sml <= 1) sml = 2;
        int big = rect.Height();

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + Middle - (sml / 2), rect.Width(), sml), ColorFg);

        if (Current > 0) {
            Pixmap->DrawRectangle(
                cRect(rect.Left(), rect.Top() + Middle - (big / 2), (rect.Width() * PercentLeft), big), ColorBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + (rect.Width() * PercentLeft) - (big / 2),
                                      rect.Top() + Middle - (big / 2), big, big),
                                ColorBarFg, 0);
        }
        break;
    }
    case 4:  // big line + dot
    {
        int big = rect.Height();
        if (Current > 0) {
            Pixmap->DrawRectangle(
                cRect(rect.Left(), rect.Top() + Middle - (big / 2), (rect.Width() * PercentLeft), big), ColorBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + (rect.Width() * PercentLeft) - (big / 2),
                                      rect.Top() + Middle - (big / 2), big, big),
                                ColorBarFg, 0);
        }
        break;
    }
    case 5:  // big line + outline + dot
    {
        int big = rect.Height();
        int out {1};
        if (big > 10) out = 2;
        // Outline
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + big - out, rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), out, big), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left() + rect.Width() - out, rect.Top(), out, big), ColorFg);

        if (Current > 0) {
            Pixmap->DrawRectangle(
                cRect(rect.Left(), rect.Top() + Middle - (big / 2), (rect.Width() * PercentLeft), big), ColorBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + (rect.Width() * PercentLeft) - (big / 2),
                                      rect.Top() + Middle - (big / 2), big, big),
                                ColorBarFg, 0);
        }
        break;
    }
    case 6:  // Small line + dot
    {
        int sml = rect.Height() / 10 * 2;
        if (sml <= 1) sml = 2;
        int big = rect.Height();

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + Middle - (sml / 2), rect.Width(), sml), ColorFg);

        if (Current > 0) {
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + (rect.Width() * PercentLeft) - (big / 2),
                                      rect.Top() + Middle - (big / 2), big, big),
                                ColorBarFg, 0);
        }
        break;
    }
    case 7:  // Outline + dot
    {
        int big = rect.Height();
        int out {1};
        if (big > 10) out = 2;
        // Outline
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + big - out, rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), out, big), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left() + rect.Width() - out, rect.Top(), out, big), ColorFg);

        if (Current > 0) {
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + (rect.Width() * PercentLeft) - (big / 2),
                                      rect.Top() + Middle - (big / 2), big, big),
                                ColorBarFg, 0);
        }
        break;
    }
    case 8:  // Small line + big line + alpha blend
    {
        int sml = rect.Height() / 10 * 2;
        if (sml <= 1) sml = 2;
        int big = rect.Height() / 2 - sml / 2;

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + Middle - (sml / 2), rect.Width(), sml), ColorFg);

        if (Current > 0) {
            DecorDrawGlowRectHor(Pixmap, rect.Left(), rect.Top(), (rect.Width() * PercentLeft), big, ColorBarFg);
            DecorDrawGlowRectHor(Pixmap, rect.Left(), rect.Top() + Middle + sml / 2, (rect.Width() * PercentLeft),
                                 big * -1, ColorBarFg);
        }
        break;
    }
    case 9:  // big line + alpha blend
    {
        int big = rect.Height();
        if (Current > 0) {
            DecorDrawGlowRectHor(Pixmap, rect.Left(), rect.Top() + Middle - big / 2, (rect.Width() * PercentLeft),
                                 big / 2, ColorBarFg);
            DecorDrawGlowRectHor(Pixmap, rect.Left(), rect.Top() + Middle, (rect.Width() * PercentLeft), big / -2,
                                 ColorBarFg);
        }
        break;
    }
    }
}

void cFlatBaseRender::ProgressBarDrawMarks(int Current, int Total, const cMarks *Marks, tColor Color,
                                           tColor ColorCurrent) {
    g_ProgressBarColorMark = Color;
    g_ProgressBarColorMarkCurrent = ColorCurrent;
    int PosMark {0}, PosMarkLast {0}, PosCurrent {0};

    int top = g_ProgressBarHeight / 2;
    if (ProgressBarPixmapBg)
        ProgressBarPixmapBg->DrawRectangle(
            cRect(0, g_ProgressBarMarginHor + g_ProgressBarHeight, g_ProgressBarWidth, g_ProgressBarMarginHor),
            g_ProgressBarColorBg);

    PixmapFill(ProgressBarPixmap, g_ProgressBarColorBg);

    int sml = Config.decorProgressReplaySize / 10 * 2;
    if (sml <= 4) sml = 4;
    int big = Config.decorProgressReplaySize - sml * 2 - 2;

    if (!Marks) {
        // g_ProgressBarColorFg = g_ProgressBarColorBarFg;
        g_ProgressBarColorBarFg = g_ProgressBarColorBarCurFg;

        ProgressBarDraw(Current, Total);
        return;
    }
    if (!Marks->First()) {
        // g_ProgressBarColorFg = g_ProgressBarColorBarCurFg;
        g_ProgressBarColorBarFg = g_ProgressBarColorBarCurFg;

        ProgressBarDraw(Current, Total);
        return;
    }

    // the small line
    ProgressBarPixmap->DrawRectangle(cRect(0, top - sml / 2, g_ProgressBarWidth, sml), g_ProgressBarColorFg);

    bool Start = true;

    for (const cMark *m = Marks->First(); m; m = Marks->Next(m)) {
        PosMark = ProgressBarMarkPos(m->Position(), Total);
        PosCurrent = ProgressBarMarkPos(Current, Total);

        ProgressBarDrawMark(PosMark, PosMarkLast, PosCurrent, Start, m->Position() == Current);
        PosMarkLast = PosMark;
        Start = !Start;
    }

    // Draw last marker vertical line
    if (PosCurrent == PosMark)
        ProgressBarPixmap->DrawRectangle(cRect(PosMark - sml, 0, sml * 2, g_ProgressBarHeight),
                                         g_ProgressBarColorMarkCurrent);
    else
        ProgressBarPixmap->DrawRectangle(cRect(PosMark - sml / 2, 0, sml, g_ProgressBarHeight), g_ProgressBarColorMark);

    if (!Start) {
        // ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast + sml / 2, top - big / 2,
        //                                  g_ProgressBarWidth - PosMarkLast, big), g_ProgressBarColorBarFg);
        if (PosCurrent > PosMarkLast)
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast + sml / 2, top - big / 2, PosCurrent - PosMarkLast, big),
                                             g_ProgressBarColorBarCurFg);
    } else {
        // Marker
        ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - sml / 2, PosCurrent - PosMarkLast, sml),
                                         g_ProgressBarColorBarCurFg);
        ProgressBarPixmap->DrawRectangle(cRect(PosCurrent - big / 2, top - big / 2, big, big),
                                         g_ProgressBarColorBarCurFg);

        if (PosCurrent > PosMarkLast + sml / 2)
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast - sml / 2, 0, sml, g_ProgressBarHeight),
                                             g_ProgressBarColorMark);
    }
}

int cFlatBaseRender::ProgressBarMarkPos(int P, int Total) {
    return (int64_t)P * g_ProgressBarWidth / Total;
}

void cFlatBaseRender::ProgressBarDrawMark(int PosMark, int PosMarkLast, int PosCurrent, bool Start, bool IsCurrent) {
    int top = g_ProgressBarHeight / 2;
    int sml = Config.decorProgressReplaySize / 10 * 2;
    if (sml <= 4) sml = 4;
    int big = Config.decorProgressReplaySize - sml * 2 - 2;

    int mbig = Config.decorProgressReplaySize * 2;
    if (Config.decorProgressReplaySize > 15)
        mbig = Config.decorProgressReplaySize;

    // Marker vertical line
    if (PosCurrent == PosMark)
        ProgressBarPixmap->DrawRectangle(cRect(PosMark - sml, 0, sml * 2, g_ProgressBarHeight),
                                         g_ProgressBarColorMarkCurrent);
    else
        ProgressBarPixmap->DrawRectangle(cRect(PosMark - sml / 2, 0, sml, g_ProgressBarHeight), g_ProgressBarColorMark);

    if (Start) {
        if (PosCurrent > PosMark)
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - sml / 2, PosMark - PosMarkLast, sml),
                                             g_ProgressBarColorBarCurFg);
        else {
            // Marker
            ProgressBarPixmap->DrawRectangle(cRect(PosCurrent - big / 2, top - big / 2, big, big),
                                             g_ProgressBarColorBarCurFg);
            if (PosCurrent > PosMarkLast)
                ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - sml / 2, PosCurrent - PosMarkLast, sml),
                                                 g_ProgressBarColorBarCurFg);
        }
        // Marker top
        if (IsCurrent)
            ProgressBarPixmap->DrawRectangle(cRect(PosMark - mbig / 2, 0, mbig, sml), g_ProgressBarColorMarkCurrent);
        else
            ProgressBarPixmap->DrawRectangle(cRect(PosMark - mbig / 2, 0, mbig, sml), g_ProgressBarColorMark);
    } else {
        // Big line
        if (PosCurrent > PosMark) {
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - big / 2, PosMark - PosMarkLast, big),
                                             g_ProgressBarColorBarCurFg);
            // Draw last marker top
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast - mbig / 2, 0, mbig, g_MarginItem / 2),
                                             g_ProgressBarColorMark);
        } else {
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - big / 2, PosMark - PosMarkLast, big),
                                             g_ProgressBarColorBarFg);
            if (PosCurrent > PosMarkLast) {
                ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - big / 2, PosCurrent - PosMarkLast, big),
                                                 g_ProgressBarColorBarCurFg);
                // Draw last marker top
                ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast - mbig / 2, 0, mbig, g_MarginItem / 2),
                                                 g_ProgressBarColorMark);
            }
        }
        // Marker bottom
        if (IsCurrent)
            ProgressBarPixmap->DrawRectangle(cRect(PosMark - mbig / 2, g_ProgressBarHeight - sml, mbig, sml),
                                             g_ProgressBarColorMarkCurrent);
        else
            ProgressBarPixmap->DrawRectangle(cRect(PosMark - mbig / 2, g_ProgressBarHeight - sml, mbig, sml),
                                             g_ProgressBarColorMark);
    }

    if (PosCurrent == PosMarkLast && PosMarkLast != 0)
        ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast - sml, 0, sml * 2, g_ProgressBarHeight),
                                         g_ProgressBarColorMarkCurrent);
    else if (PosMarkLast != 0)
        ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast - sml / 2, 0, sml, g_ProgressBarHeight), g_ProgressBarColorMark);
}

void cFlatBaseRender::ScrollbarDraw(cPixmap *Pixmap, int Left, int Top, int Height, int Total, int Offset, int Shown,
                                    bool CanScrollUp, bool CanScrollDown) {
    if (!Pixmap) return;

    int ScrollHeight = std::max(static_cast<int>(Height * 1.0f * Shown / Total + 0.5f), 5);
    int ScrollTop =
        std::min(static_cast<int>(Top * 1.0f + Height * Offset / Total + 0.5f), Top + Height - ScrollHeight);

    /* Types
     * 0 = left line + rect bar
     * 1 = left line + round bar
     * 2 = middle line + rect bar
     * 3 = middle line + round bar
     * 4 = outline + rect bar
     * 5 = outline + round bar
     * 6 = rect bar
     * 7 = round bar
     */
    int Type = Config.decorScrollBarType;

    if (Total > 0 && Total > Shown) {
        PixmapFill(Pixmap, clrTransparent);
        Pixmap->DrawRectangle(cRect(Left, Top, g_ScrollBarWidth, Height), Config.decorScrollBarBg);
        switch (Type) {
        default:
        case 0: {
            int LineWidth {6};
            if (g_ScrollBarWidth <= 10)
                LineWidth = 2;
            else if (g_ScrollBarWidth <= 20)
                LineWidth = 4;
            Pixmap->DrawRectangle(cRect(Left, Top, LineWidth, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(cRect(Left + LineWidth, ScrollTop, g_ScrollBarWidth - LineWidth, ScrollHeight),
                                  Config.decorScrollBarBarFg);
            break;
        }
        case 1: {
            int DotHeight = g_ScrollBarWidth / 2;
            int LineWidth {6};
            if (g_ScrollBarWidth <= 10)
                LineWidth = 2;
            else if (g_ScrollBarWidth <= 20)
                LineWidth = 4;
            Pixmap->DrawRectangle(cRect(Left, Top, LineWidth, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(cRect(Left + LineWidth, ScrollTop + DotHeight, g_ScrollBarWidth - LineWidth,
                                        ScrollHeight - DotHeight * 2),
                                  Config.decorScrollBarBarFg);
            // Dot
            Pixmap->DrawEllipse(
                cRect(Left + LineWidth, ScrollTop, g_ScrollBarWidth - LineWidth, g_ScrollBarWidth - LineWidth),
                Config.decorScrollBarBarFg, 0);
            Pixmap->DrawEllipse(cRect(Left + LineWidth, ScrollTop + ScrollHeight - DotHeight * 2,
                                      g_ScrollBarWidth - LineWidth, g_ScrollBarWidth - LineWidth),
                                Config.decorScrollBarBarFg, 0);
            break;
        }
        case 2: {
            int Middle = Left + g_ScrollBarWidth / 2;
            int LineWidth {6};
            if (g_ScrollBarWidth <= 10)
                LineWidth = 2;
            else if (g_ScrollBarWidth <= 20)
                LineWidth = 4;

            Pixmap->DrawRectangle(cRect(Middle - LineWidth / 2, Top, LineWidth, Height), Config.decorScrollBarFg);
            // Bar
            Pixmap->DrawRectangle(cRect(Left, ScrollTop, g_ScrollBarWidth, ScrollHeight), Config.decorScrollBarBarFg);
            break;
        }
        case 3: {
            int DotHeight = g_ScrollBarWidth / 2;
            int Middle = Left + g_ScrollBarWidth / 2;
            int LineWidth {6};
            if (g_ScrollBarWidth <= 10)
                LineWidth = 2;
            else if (g_ScrollBarWidth <= 20)
                LineWidth = 4;

            Pixmap->DrawRectangle(cRect(Middle - LineWidth / 2, Top, LineWidth, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(cRect(Left, ScrollTop + DotHeight, g_ScrollBarWidth, ScrollHeight - DotHeight * 2),
                                  Config.decorScrollBarBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(Left, ScrollTop, g_ScrollBarWidth, g_ScrollBarWidth), Config.decorScrollBarBarFg, 0);
            Pixmap->DrawEllipse(cRect(Left, ScrollTop + ScrollHeight - DotHeight * 2, g_ScrollBarWidth, g_ScrollBarWidth),
                                Config.decorScrollBarBarFg, 0);
            break;
        }
        case 4: {
            int out {1};
            if (g_ScrollBarWidth > 10) out = 2;
            // Outline
            Pixmap->DrawRectangle(cRect(Left, Top, g_ScrollBarWidth, out), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left, Top + Height - out, g_ScrollBarWidth, out), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left, Top, out, Height), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left + g_ScrollBarWidth - out, Top, out, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(cRect(Left + out, ScrollTop + out, g_ScrollBarWidth - out * 2, ScrollHeight - out * 2),
                                  Config.decorScrollBarBarFg);
            break;
        }
        case 5: {
            int DotHeight = g_ScrollBarWidth / 2;
            int out {1};
            if (g_ScrollBarWidth > 10) out = 2;
            // Outline
            Pixmap->DrawRectangle(cRect(Left, Top, g_ScrollBarWidth, out), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left, Top + Height - out, g_ScrollBarWidth, out), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left, Top, out, Height), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left + g_ScrollBarWidth - out, Top, out, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(cRect(Left + out, ScrollTop + DotHeight + out, g_ScrollBarWidth - out * 2,
                                        ScrollHeight - DotHeight * 2 - out * 2),
                                  Config.decorScrollBarBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(Left + out, ScrollTop + out, g_ScrollBarWidth - out * 2, g_ScrollBarWidth - out * 2),
                                Config.decorScrollBarBarFg, 0);
            Pixmap->DrawEllipse(cRect(Left + out, ScrollTop + ScrollHeight - DotHeight * 2 + out,
                                      g_ScrollBarWidth - out * 2, g_ScrollBarWidth - out * 2),
                                Config.decorScrollBarBarFg, 0);
            break;
        }
        case 6: {
            Pixmap->DrawRectangle(cRect(Left, ScrollTop, g_ScrollBarWidth, ScrollHeight), Config.decorScrollBarBarFg);
            break;
        }
        case 7: {
            int DotHeight = g_ScrollBarWidth / 2;

            Pixmap->DrawRectangle(cRect(Left, ScrollTop + DotHeight, g_ScrollBarWidth, ScrollHeight - DotHeight * 2),
                                  Config.decorScrollBarBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(Left, ScrollTop, g_ScrollBarWidth, g_ScrollBarWidth), Config.decorScrollBarBarFg, 0);
            Pixmap->DrawEllipse(cRect(Left, ScrollTop + ScrollHeight - DotHeight * 2, g_ScrollBarWidth, g_ScrollBarWidth),
                                Config.decorScrollBarBarFg, 0);

            break;
        }
        }
    }
}

int cFlatBaseRender::ScrollBarWidth(void) {
    return g_ScrollBarWidth;
}

void cFlatBaseRender::DecorBorderClear(int Left, int Top, int Width, int Height, int Size) {
    int LeftDecor = Left - Size;
    int TopDecor = Top - Size;
    int WidthDecor = Width + Size * 2;
    int HeightDecor = Height + Size * 2;
    int BottomDecor = Height + Size;

    if (DecorPixmap) {
        // Top
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor, WidthDecor, Size), clrTransparent);
        // Right
        DecorPixmap->DrawRectangle(cRect(LeftDecor + Size + Width, TopDecor, Size, HeightDecor), clrTransparent);
        // Bottom
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor + BottomDecor, WidthDecor, Size), clrTransparent);
        // Left
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor, Size, HeightDecor), clrTransparent);
    }
}

void cFlatBaseRender::DecorBorderClearByFrom(int From) {
    std::list<sDecorBorder>::iterator it, end = Borders.end();
    for (it = Borders.begin(); it != end;) {
        if ((*it).From == From) {
            DecorBorderClear((*it).Left, (*it).Top, (*it).Width, (*it).Height, (*it).Size);
            it = Borders.erase(it);
        } else
            ++it;
    }
}

void cFlatBaseRender::DecorBorderRedrawAll(void) {
    std::list<sDecorBorder>::iterator it, end = Borders.end();
    for (it = Borders.begin(); it != end; ++it) {
        DecorBorderDraw((*it).Left, (*it).Top, (*it).Width, (*it).Height, (*it).Size, (*it).Type, (*it).ColorFg,
                        (*it).ColorBg, (*it).From, false);
    }
}

void cFlatBaseRender::DecorBorderClearAll(void) {
    PixmapFill(DecorPixmap, clrTransparent);
}

void cFlatBaseRender::DecorBorderDraw(int Left, int Top, int Width, int Height, int Size, int Type, tColor ColorFg,
                                      tColor ColorBg, int From, bool Store) {
    if (Size == 0 || Type <= 0) return;

    if (Store) {
        sDecorBorder f {
            Left, Top, Width, Height, Size, Type, ColorFg, ColorBg, From
        };
        Borders.emplace_back(f);
    }

    int LeftDecor = Left - Size;
    int TopDecor = Top - Size;
    int WidthDecor = Width + Size * 2;
    int HeightDecor = Height + Size * 2;
    int BottomDecor = Height + Size;

    if (!DecorPixmap) {
        DecorPixmap = CreatePixmap(osd, "DecorPixmap", 4, cRect(0, 0, cOsd::OsdWidth(), cOsd::OsdHeight()));
        PixmapFill(DecorPixmap, clrTransparent);
    }

    switch (Type) {
    case 1:  // rect
        // Top
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor, WidthDecor, Size), ColorBg);
        // Right
        DecorPixmap->DrawRectangle(cRect(LeftDecor + Size + Width, TopDecor, Size, HeightDecor), ColorBg);
        // Bottom
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor + BottomDecor, WidthDecor, Size), ColorBg);
        // Left
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor, Size, HeightDecor), ColorBg);
        break;
    case 2:  // Round
        // Top
        DecorPixmap->DrawRectangle(cRect(LeftDecor + Size, TopDecor, Width, Size), ColorBg);
        // Right
        DecorPixmap->DrawRectangle(cRect(LeftDecor + Size + Width, TopDecor + Size, Size, Height), ColorBg);
        // Bottom
        DecorPixmap->DrawRectangle(cRect(LeftDecor + Size, TopDecor + BottomDecor, Width, Size), ColorBg);
        // Left
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor + Size, Size, Height), ColorBg);

        // Top,left corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor, TopDecor, Size, Size), ColorBg, 2);
        // Top,right corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor + Size + Width, TopDecor, Size, Size), ColorBg, 1);
        // Bottom,left corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor, TopDecor + BottomDecor, Size, Size), ColorBg, 3);
        // Bottom,right corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor + Size + Width, TopDecor + BottomDecor, Size, Size), ColorBg, 4);
        break;
    case 3:  // Invert round
        // Top
        DecorPixmap->DrawRectangle(cRect(LeftDecor + Size, TopDecor, Width, Size), ColorBg);
        // Right
        DecorPixmap->DrawRectangle(cRect(LeftDecor + Size + Width, TopDecor + Size, Size, Height), ColorBg);
        // Bottom
        DecorPixmap->DrawRectangle(cRect(LeftDecor + Size, TopDecor + BottomDecor, Width, Size), ColorBg);
        // Left
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor + Size, Size, Height), ColorBg);

        // Top,left corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor, TopDecor, Size, Size), ColorBg, -4);
        // Top,right corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor + Size + Width, TopDecor, Size, Size), ColorBg, -3);
        // Bottom,left corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor, TopDecor + BottomDecor, Size, Size), ColorBg, -1);
        // Bottom,right corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor + Size + Width, TopDecor + BottomDecor, Size, Size), ColorBg, -2);
        break;
    case 4:  // rect + alpha blend
        // Top
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + Size, TopDecor, WidthDecor - Size * 2, Size, ColorBg);
        // Bottom
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + Size, TopDecor + BottomDecor, WidthDecor - Size * 2, -1 * Size,
                             ColorBg);
        // Left
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor, TopDecor + Size, Size, HeightDecor - Size * 2, ColorBg);
        // Right
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor + Size + Width, TopDecor + Size, -1 * Size, HeightDecor - Size * 2,
                             ColorBg);

        DecorDrawGlowRectTL(DecorPixmap, LeftDecor, TopDecor, Size, Size, ColorBg);
        DecorDrawGlowRectTR(DecorPixmap, LeftDecor + Size + Width, TopDecor, Size, Size, ColorBg);
        DecorDrawGlowRectBL(DecorPixmap, LeftDecor, TopDecor + Size + Height, Size, Size, ColorBg);
        DecorDrawGlowRectBR(DecorPixmap, LeftDecor + Size + Width, TopDecor + Size + Height, Size, Size, ColorBg);
        break;
    case 5:  // Round + alpha blend
        // Top
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + Size, TopDecor, WidthDecor - Size * 2, Size, ColorBg);
        // Bottom
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + Size, TopDecor + BottomDecor, WidthDecor - Size * 2, -1 * Size,
                             ColorBg);
        // Left
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor, TopDecor + Size, Size, HeightDecor - Size * 2, ColorBg);
        // Right
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor + Size + Width, TopDecor + Size, -1 * Size, HeightDecor - Size * 2,
                             ColorBg);

        DecorDrawGlowEllipseTL(DecorPixmap, LeftDecor, TopDecor, Size, Size, ColorBg, 2);
        DecorDrawGlowEllipseTR(DecorPixmap, LeftDecor + Size + Width, TopDecor, Size, Size, ColorBg, 1);
        DecorDrawGlowEllipseBL(DecorPixmap, LeftDecor, TopDecor + Size + Height, Size, Size, ColorBg, 3);
        DecorDrawGlowEllipseBR(DecorPixmap, LeftDecor + Size + Width, TopDecor + Size + Height, Size, Size, ColorBg, 4);
        break;
    case 6:  // Invert round + alpha blend
        // Top
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + Size, TopDecor, WidthDecor - Size * 2, Size, ColorBg);
        // Bottom
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + Size, TopDecor + BottomDecor, WidthDecor - Size * 2, -1 * Size,
                             ColorBg);
        // Left
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor, TopDecor + Size, Size, HeightDecor - Size * 2, ColorBg);
        // Right
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor + Size + Width, TopDecor + Size, -1 * Size, HeightDecor - Size * 2,
                             ColorBg);

        DecorDrawGlowEllipseTL(DecorPixmap, LeftDecor, TopDecor, Size, Size, ColorBg, -4);
        DecorDrawGlowEllipseTR(DecorPixmap, LeftDecor + Size + Width, TopDecor, Size, Size, ColorBg, -3);
        DecorDrawGlowEllipseBL(DecorPixmap, LeftDecor, TopDecor + Size + Height, Size, Size, ColorBg, -1);
        DecorDrawGlowEllipseBR(DecorPixmap, LeftDecor + Size + Width, TopDecor + Size + Height, Size, Size, ColorBg,
                               -2);
        break;
    }
}

/*
tColor cFlatBaseRender::Multiply(tColor Color, uint8_t Alpha)
{
  tColor RB = (Color & 0x00FF00FF) * Alpha;
  RB = ((RB + ((RB >> 8) & 0x00FF00FF) + 0x00800080) >> 8) & 0x00FF00FF;
  tColor AG = ((Color >> 8) & 0x00FF00FF) * Alpha;
  AG = ((AG + ((AG >> 8) & 0x00FF00FF) + 0x00800080)) & 0xFF00FF00;
  return AG | RB;
}
*/

tColor cFlatBaseRender::SetAlpha(tColor Color, double am) {
    uint8_t A = (Color & 0xFF000000) >> 24;
    uint8_t R = (Color & 0x00FF0000) >> 16;
    uint8_t G = (Color & 0x0000FF00) >> 8;
    uint8_t B = (Color & 0x000000FF);

    A = A * am;
    return ArgbToColor(A, R, G, B);
}

void cFlatBaseRender::DecorDrawGlowRectHor(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    if (Height < 0) {
        Height *= -1;
        for (int i = Height, j {0}; i >= 0; --i, ++j) {
            Alpha = 255.0 / Height * j;
            col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
            pixmap->DrawRectangle(cRect(Left, Top + i, Width, 1), col);
        }
    } else {
        for (int i {0}; i < Height; ++i) {
            Alpha = 255.0 / Height * i;
            col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
            pixmap->DrawRectangle(cRect(Left, Top + i, Width, 1), col);
        }
    }
}

void cFlatBaseRender::DecorDrawGlowRectVer(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    if (Width < 0) {
        Width *= -1;
        for (int i = Width, j {0}; i >= 0; --i, ++j) {
            Alpha = 255.0 / Width * j;
            col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
            pixmap->DrawRectangle(cRect(Left + i, Top, 1, Height), col);
        }
    } else {
        for (int i {0}; i < Width; ++i) {
            Alpha = 255.0 / Width * i;
            col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
            pixmap->DrawRectangle(cRect(Left + i, Top, 1, Height), col);
        }
    }
}

void cFlatBaseRender::DecorDrawGlowRectTL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    for (int i {0}; i < Width; ++i) {
        Alpha = 255.0 / Width * i;
        col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
        pixmap->DrawRectangle(cRect(Left + i, Top + i, Width - i, Height - i), col);
    }
}

void cFlatBaseRender::DecorDrawGlowRectTR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    for (int i {0}, j = Width; i < Width; ++i, --j) {
        Alpha = 255.0 / Width * i;
        col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
        pixmap->DrawRectangle(cRect(Left, Top + Height - j, j, j), col);
    }
}

void cFlatBaseRender::DecorDrawGlowRectBL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    for (int i {0}, j = Width; i < Width; ++i, --j) {
        Alpha = 255.0 / Width * i;
        col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
        pixmap->DrawRectangle(cRect(Left + Width - j, Top, j, j), col);
    }
}

void cFlatBaseRender::DecorDrawGlowRectBR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    for (int i {0}, j = Width; i < Width; ++i, --j) {
        Alpha = 255 / Width * i;
        col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
        pixmap->DrawRectangle(cRect(Left, Top, j, j), col);
    }
}

void cFlatBaseRender::DecorDrawGlowEllipseTL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                             int type) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    for (int i {0}, j = Width; i < Width; ++i, --j) {
        if (VDRVERSNUM < 20002 && j == 1)  // in VDR Version < 2.0.2 osd breaks if width & height == 1
            continue;
        Alpha = 255 / Width * i;
        col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
        pixmap->DrawEllipse(cRect(Left + i, Top + i, j, j), col, type);
    }
}

void cFlatBaseRender::DecorDrawGlowEllipseTR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                             int type) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    for (int i {0}, j = Width; i < Width; ++i, --j) {
        if (VDRVERSNUM < 20002 && j == 1)  // in VDR Version < 2.0.2 osd breaks if width & height == 1
            continue;
        Alpha = 255 / Width * i;
        col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
        pixmap->DrawEllipse(cRect(Left, Top + Height - j, j, j), col, type);
    }
}

void cFlatBaseRender::DecorDrawGlowEllipseBL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                             int type) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    for (int i {0}, j = Width; i < Width; ++i, --j) {
        if (VDRVERSNUM < 20002 && j == 1)  // in VDR Version < 2.0.2 osd breaks if width & height == 1
            continue;
        Alpha = 255 / Width * i;
        col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
        pixmap->DrawEllipse(cRect(Left + Width - j, Top, j, j), col, type);
    }
}

void cFlatBaseRender::DecorDrawGlowEllipseBR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                             int type) {
    double Alpha {0.0};
    tColor col {};  // Init outside of loop
    for (int i {0}, j = Width; i < Width; ++i, --j) {
        if (VDRVERSNUM < 20002 && j == 1)  // in VDR Version < 2.0.2 osd breaks if width & height == 1
            continue;
        Alpha = 255 / Width * i;
        col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
        pixmap->DrawEllipse(cRect(Left, Top, j, j), col, type);
    }
}

int cFlatBaseRender::GetFontAscender(const char *Name, int CharHeight, int CharWidth) {
    FT_Library library;
    FT_Face face;
    cString FontFileName = cFont::GetFontFileName(Name);
    int Ascender = CharHeight;
    int rc = FT_Init_FreeType(&library);
    if (!rc) {
        rc = FT_New_Face(library, *FontFileName, 0, &face);
        if (!rc) {
            if (face->num_fixed_sizes && face->available_sizes) {  // Fixed font
                // TODO what exactly does all this mean?
                Ascender = face->available_sizes->height;
            } else {
                rc = FT_Set_Char_Size(face, CharWidth * 64, CharHeight * 64, 0, 0);
                if (!rc) {
                    Ascender = face->size->metrics.ascender / 64;
                } else
                    esyslog("ERROR: FreeType: error %d during FT_Set_Char_Size (font = %s)\n", rc, *FontFileName);
            }
        } else
            esyslog("ERROR: FreeType: load error %d (font = %s)", rc, *FontFileName);
    } else
        esyslog("ERROR: FreeType: initialization error %d (font = %s)", rc, *FontFileName);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return Ascender;
}

void cFlatBaseRender::DrawWidgetWeather(void) {
    int fs = round(cOsd::OsdHeight() * Config.WeatherFontSize);
    cFont *WeatherFont = cFont::CreateFont(Setup.FontOsd, fs);
    cFont *WeatherFontSml = cFont::CreateFont(Setup.FontOsd, fs * (1.0 / 2.0));
    cFont *WeatherFontSign = cFont::CreateFont(Setup.FontOsd, fs * (1.0 / 2.5));

    std::string TempToday(""), TempTodaySign("");
    std::string IconToday(""), IconTomorrow("");
    std::string TempMaxToday(""), TempMaxTomorrow("");
    std::string TempMinToday(""), TempMinTomorrow("");
    std::string PrecToday(""), PrecTomorrow("");

    std::ifstream file;
    cString FileName("");

    FileName = cString::sprintf("%s/weather/weather.0.temp", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, TempToday);
        file.close();
        std::size_t found = TempToday.find("°");
        if (found != std::string::npos) {
            TempTodaySign = TempToday.substr(found);
            TempToday = TempToday.substr(0, found);
        }
    } else
        return;

    FileName = cString::sprintf("%s/weather/weather.0.icon-act", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, IconToday);
        file.close();
    } else
        return;

    FileName = cString::sprintf("%s/weather/weather.1.icon", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, IconTomorrow);
        file.close();
    } else
        return;

    FileName = cString::sprintf("%s/weather/weather.0.TempMax", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, TempMaxToday);
        file.close();
    } else
        return;

    FileName = cString::sprintf("%s/weather/weather.1.TempMax", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, TempMaxTomorrow);
        file.close();
    } else
        return;

    FileName = cString::sprintf("%s/weather/weather.0.TempMin", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, TempMinToday);
        file.close();
    } else
        return;

    FileName = cString::sprintf("%s/weather/weather.1.TempMin", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, TempMinTomorrow);
        file.close();
    } else
        return;

    double p {0.0};
    FileName = cString::sprintf("%s/weather/weather.0.precipitation", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, PrecToday);
        std::replace(PrecToday.begin(), PrecToday.end(), '.', ',');
        file.close();
        p = atof(PrecToday.c_str()) * 100.0f;
        p = RoundUp(p, 10);
        PrecToday = cString::sprintf("%.0f%%", p);
    }

    FileName = cString::sprintf("%s/weather/weather.1.precipitation", WIDGETOUTPUTPATH);
    file.open(*FileName, std::ifstream::in);
    if (file.is_open()) {
        std::getline(file, PrecTomorrow);
        std::replace(PrecTomorrow.begin(), PrecTomorrow.end(), '.', ',');
        file.close();
        p = atof(PrecTomorrow.c_str()) * 100.0f;
        p = RoundUp(p, 10);
        PrecTomorrow = cString::sprintf("%.0f%%", p);
    }

    int left = g_MarginItem;

    int WidthTempToday =
        std::max(WeatherFontSml->Width(TempMaxToday.c_str()), WeatherFontSml->Width(TempMinToday.c_str()));
    int WidthTempTomorrow =
        std::max(WeatherFontSml->Width(TempMaxTomorrow.c_str()), WeatherFontSml->Width(TempMinTomorrow.c_str()));
    int WeatherFontHeight = WeatherFont->Height();  // Used multiple times
    int WeatherFontSmlHeight = WeatherFontSml->Height();

    int wTop = g_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize;
    int wWidth = g_MarginItem + WeatherFont->Width(TempToday.c_str()) + WeatherFontSign->Width(TempTodaySign.c_str()) +
                 g_MarginItem * 2 + WeatherFontHeight + g_MarginItem + WidthTempToday + g_MarginItem +
                 WeatherFontHeight - g_MarginItem * 2 + WeatherFontSml->Width(PrecToday.c_str()) + g_MarginItem * 4 +
                 WeatherFontHeight + g_MarginItem + WidthTempTomorrow + g_MarginItem + WeatherFontHeight -
                 g_MarginItem * 2 + WeatherFontSml->Width(PrecTomorrow.c_str()) + g_MarginItem * 2;
    int wLeft = g_OsdWidth - wWidth - 20;

    WeatherWidget.Clear();
    WeatherWidget.SetOsd(osd);
    WeatherWidget.SetPosition(cRect(wLeft, wTop, wWidth, WeatherFontHeight));
    WeatherWidget.SetBGColor(Theme.Color(clrItemCurrentBg));
    WeatherWidget.SetScrollingActive(false);

    WeatherWidget.AddText(TempToday.c_str(), false, cRect(left, 0, 0, 0), Theme.Color(clrChannelFontEpg),
                          Theme.Color(clrItemCurrentBg), WeatherFont);
    left += WeatherFont->Width(TempToday.c_str());

    int FontAscender = GetFontAscender(Setup.FontOsd, fs);
    int FontAscender2 = GetFontAscender(Setup.FontOsd, fs * (1.0 / 2.5));
    int t = (WeatherFontHeight - FontAscender) - (WeatherFontSign->Height() - FontAscender2);

    WeatherWidget.AddText(TempTodaySign.c_str(), false, cRect(left, t, 0, 0), Theme.Color(clrChannelFontEpg),
                          Theme.Color(clrItemCurrentBg), WeatherFontSign);
    left += WeatherFontSign->Width(TempTodaySign.c_str()) + g_MarginItem * 2;

    cString WeatherIcon = cString::sprintf("widgets/%s", IconToday.c_str());
    cImage *img = imgLoader.LoadIcon(*WeatherIcon, WeatherFontHeight, WeatherFontHeight - g_MarginItem * 2);
    if (img) {
        WeatherWidget.AddImage(img, cRect(left, 0 + g_MarginItem, WeatherFontHeight, WeatherFontHeight));
        left += WeatherFontHeight + g_MarginItem;
    }
    WeatherWidget.AddText(TempMaxToday.c_str(), false, cRect(left, 0, 0, 0), Theme.Color(clrChannelFontEpg),
                          Theme.Color(clrItemCurrentBg), WeatherFontSml, WidthTempToday, WeatherFontSmlHeight,
                          taRight);
    WeatherWidget.AddText(TempMinToday.c_str(), false, cRect(left, 0 + WeatherFontSmlHeight, 0, 0),
                          Theme.Color(clrChannelFontEpg), Theme.Color(clrItemCurrentBg), WeatherFontSml, WidthTempToday,
                          WeatherFontSmlHeight, taRight);
    left += WidthTempToday + g_MarginItem;

    img = imgLoader.LoadIcon("widgets/umbrella", WeatherFontHeight, WeatherFontHeight - g_MarginItem * 2);
    if (img) {
        WeatherWidget.AddImage(img, cRect(left, 0 + g_MarginItem, WeatherFontHeight, WeatherFontHeight));
        left += WeatherFontHeight - g_MarginItem * 2;
    }
    WeatherWidget.AddText(PrecToday.c_str(), false,
                          cRect(left, 0 + (WeatherFontHeight / 2 - WeatherFontSmlHeight / 2), 0, 0),
                          Theme.Color(clrChannelFontEpg), Theme.Color(clrItemCurrentBg), WeatherFontSml);
    left += WeatherFontSml->Width(PrecToday.c_str()) + g_MarginItem * 4;

    WeatherWidget.AddRect(cRect(left - g_MarginItem * 2, 0, wWidth - left + g_MarginItem * 2, WeatherFontHeight),
                          Theme.Color(clrChannelBg));

    WeatherIcon = cString::sprintf("widgets/%s", IconTomorrow.c_str());
    img = imgLoader.LoadIcon(*WeatherIcon, WeatherFontHeight, WeatherFontHeight - g_MarginItem * 2);
    if (img) {
        WeatherWidget.AddImage(img, cRect(left, 0 + g_MarginItem, WeatherFontHeight, WeatherFontHeight));
        left += WeatherFontHeight + g_MarginItem;
    }
    WeatherWidget.AddText(TempMaxTomorrow.c_str(), false, cRect(left, 0, 0, 0), Theme.Color(clrChannelFontEpg),
                          Theme.Color(clrChannelBg), WeatherFontSml, WidthTempTomorrow, WeatherFontSmlHeight,
                          taRight);
    WeatherWidget.AddText(TempMinTomorrow.c_str(), false, cRect(left, 0 + WeatherFontSmlHeight, 0, 0),
                          Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), WeatherFontSml, WidthTempTomorrow,
                          WeatherFontSmlHeight, taRight);
    left += WidthTempTomorrow + g_MarginItem;

    img = imgLoader.LoadIcon("widgets/umbrella", WeatherFontHeight, WeatherFontHeight - g_MarginItem * 2);
    if (img) {
        WeatherWidget.AddImage(img, cRect(left, 0 + g_MarginItem, WeatherFontHeight, WeatherFontHeight));
        left += WeatherFontHeight - g_MarginItem * 2;
    }
    WeatherWidget.AddText(PrecTomorrow.c_str(), false,
                          cRect(left, 0 + (WeatherFontHeight / 2 - WeatherFontSmlHeight / 2), 0, 0),
                          Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), WeatherFontSml);
    left += WeatherFontSml->Width(PrecTomorrow.c_str());

    // WeatherWidget.AddRect(cRect(left, 0, wWidth - left, g_FontHight), clrTransparent);

    WeatherWidget.CreatePixmaps(false);
    WeatherWidget.Draw();
}
