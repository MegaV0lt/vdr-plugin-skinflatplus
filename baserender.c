/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./baserender.h"

#include <sys/stat.h>
#include <vdr/font.h>
#include <vdr/osd.h>
#include <vdr/timers.h>
#include <vdr/tools.h>
#include <vdr/videodir.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <locale>
#include <mutex>  // NOLINT
#include <random>
#include <sstream>
#include <string_view>
#include <vector>

#include "./flat.h"
#include "./fontcache.h"
#include "./glyphmetricscache.h"

cFlatBaseRender::cFlatBaseRender() {
    // Standard fonts
    m_Font = FontCache.GetFont(Setup.FontOsd, Setup.FontOsdSize);
    m_FontSml = FontCache.GetFont(Setup.FontSml, Setup.FontSmlSize);
    m_FontFixed = FontCache.GetFont(Setup.FontFix, Setup.FontFixSize);
    m_FontHeight = m_Font->Height();
    m_FontHeight2 = m_FontHeight * 2;
    m_FontSmlHeight = m_FontSml->Height();
    m_FontFixedHeight = m_FontFixed->Height();

    m_FontBig = FontCache.GetFont(Setup.FontOsd, Setup.FontOsdSize * 1.5);
    m_FontMedium = FontCache.GetFont(Setup.FontOsd, (Setup.FontOsdSize + Setup.FontSmlSize) / 2);
    if (Config.MainMenuWidgetWeatherShow) {
        m_FontTempSml = FontCache.GetFont(Setup.FontOsd, Setup.FontOsdSize / 2);
        m_FontTempSmlHeight = m_FontTempSml->Height();
    }
    if (Config.TVScraperEPGInfoShowActors || Config.TVScraperRecInfoShowActors) {
        m_FontTiny = FontCache.GetFont(Setup.FontSml, Setup.FontSmlSize * 0.8);  // 80% of small font size
        m_FontTinyHeight = m_FontTiny->Height();
    }
    m_FontBigHeight = m_FontBig->Height();

    // Top bar fonts
    const int fs = cOsd::OsdHeight() * Config.TopBarFontSize + 0.5;
    m_TopBarFont = FontCache.GetFont(Setup.FontOsd, fs);
    m_TopBarFontClock = FontCache.GetFont(Setup.FontOsd, fs * Config.TopBarFontClockScale * 100.0);
    m_TopBarFontSml = FontCache.GetFont(Setup.FontOsd, fs / 2);
    m_TopBarFontHeight = m_TopBarFont->Height();
    m_TopBarFontSmlHeight = m_TopBarFontSml->Height();
    m_TopBarFontClockHeight = m_TopBarFontClock->Height();

    m_FontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);  // Top of capital letters

    m_ScrollBarWidth = Config.decorScrollBarSize;

    Borders.reserve(64);

    Config.ThemeCheckAndInit();
    Config.DecorCheckAndInit();
}

cFlatBaseRender::~cFlatBaseRender() {
    // if (m_Osd) {
        MessageScroller.Clear();
        m_Osd->DestroyPixmap(TopBarPixmap);
        m_Osd->DestroyPixmap(TopBarIconPixmap);
        m_Osd->DestroyPixmap(TopBarIconBgPixmap);
        m_Osd->DestroyPixmap(ButtonsPixmap);
        m_Osd->DestroyPixmap(MessagePixmap);
        m_Osd->DestroyPixmap(MessageIconPixmap);
        m_Osd->DestroyPixmap(ProgressBarPixmap);
        m_Osd->DestroyPixmap(ProgressBarPixmapBg);
        m_Osd->DestroyPixmap(DecorPixmap);

        delete m_Osd;
    // }
}

void cFlatBaseRender::CreateFullOsd() {
    CreateOsd(cOsd::OsdLeft() + Config.marginOsdHor, cOsd::OsdTop() + Config.marginOsdVer,
              cOsd::OsdWidth() - Config.marginOsdHor * 2, cOsd::OsdHeight() - Config.marginOsdVer * 2);
}

void cFlatBaseRender::CreateOsd(int Left, int Top, int Width, int Height) {
    m_OsdLeft = Left;
    m_OsdTop = Top;
    m_OsdWidth = Width;
    m_OsdHeight = Height;

    m_Osd = cOsdProvider::NewOsd(Left, Top);  // Is always a valid pointer
    tArea Area {0, 0, Width, Height, 32};
    if (m_Osd->SetAreas(&Area, 1) == oeOk) {
        return;
    }

    esyslog("flatPlus: Create osd FAILED left: %d top: %d width: %d height: %d", Left, Top, Width, Height);
}

void cFlatBaseRender::TopBarCreate() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatBaseRender::TopBarCreate()");
#endif

    m_TopBarHeight = std::max(m_TopBarFontHeight, m_TopBarFontSmlHeight * 2);
    const cRect TopBarViewPort {Config.decorBorderTopBarSize, Config.decorBorderTopBarSize,
                                m_OsdWidth - Config.decorBorderTopBarSize * 2, m_TopBarHeight};
    TopBarPixmap = CreatePixmap(m_Osd, "TopBarPixmap", 1, TopBarViewPort);
    TopBarIconBgPixmap = CreatePixmap(m_Osd, "TopBarIconBgPixmap", 2, TopBarViewPort);
    TopBarIconPixmap = CreatePixmap(m_Osd, "TopBarIconPixmap", 3, TopBarViewPort);
    PixmapClear(TopBarPixmap);
    PixmapClear(TopBarIconBgPixmap);
    PixmapClear(TopBarIconPixmap);

    if (Config.DiskUsageShow == 3)  // 3 = Always
        TopBarEnableDiskUsage();
}

void cFlatBaseRender::TopBarSetTitle(const cString &Title, bool Clear) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: TopBarSetTitle() '%s'", *Title);
    if (Clear) dsyslog("   With clear");
#endif

    if (Clear) {  // Clear is default
        m_TopBarTitleExtra1 = "";
        m_TopBarTitleExtra2 = "";
        m_TopBarExtraIcon = "";
        m_TopBarExtraIconSet = false;
        m_TopBarMenuIcon = "";
        m_TopBarMenuIconSet = false;
        m_TopBarMenuLogo = "";
        m_TopBarMenuLogoSet = false;
    }

    m_TopBarTitle = Title;
    m_TopBarUpdateTitle = true;

    if (Config.DiskUsageShow == 3)  // 3 = Always
        TopBarEnableDiskUsage();
}

void cFlatBaseRender::TopBarSetTitleExtra(const cString &Extra1, const cString &Extra2) {
    m_TopBarTitleExtra1 = Extra1;
    m_TopBarTitleExtra2 = Extra2;
    m_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarSetExtraIcon(const cString &icon) {
    // Check if the string is empty
    if (**icon == '\0') return;  // Double dereference to get the first character

    m_TopBarExtraIcon = icon;
    m_TopBarExtraIconSet = true;
    m_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarSetMenuIcon(const cString &icon) {
    // Check if the string is empty
    if (**icon == '\0') return;  // Double dereference to get the first character

    m_TopBarMenuIcon = icon;
    m_TopBarMenuIconSet = true;
    m_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarSetMenuIconRight(const cString &icon) {
    // Check if the string is empty
    if (**icon == '\0') return;  // Double dereference to get the first character

    m_TopBarMenuIconRight = icon;
    m_TopBarMenuIconRightSet = true;
    m_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarClearMenuIconRight() {
    m_TopBarMenuIconRight = "";
    m_TopBarMenuIconRightSet = false;
}

void cFlatBaseRender::TopBarSetMenuLogo(const cString &icon) {
    // Check if the string is empty
    if (**icon == '\0') return;  // Double dereference to get the first character

    m_TopBarMenuLogo = icon;
    m_TopBarMenuLogoSet = true;
    m_TopBarUpdateTitle = true;
}

void cFlatBaseRender::TopBarEnableDiskUsage() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatBaseRender::TopBarEnableDiskUsage()");
#endif
    // cVideoDiskUsage::HasChanged(m_VideoDiskUsageState);        // Moved to cFlatDisplayMenu::cFlatDisplayMenu()
    const int DiskUsagePercent {cVideoDiskUsage::UsedPercent()};  // Used %
    const int DiskFreePercent {100 - DiskUsagePercent};           // Free %
    // Division is typically twice as slow as addition or multiplication. Rewrite divisions by a constant into a
    // multiplication with the inverse (For example, x = x / 3.0 becomes x = x * (1.0/3.0).
    // The constant is calculated during compilation.).
    const double FreeGB {cVideoDiskUsage::FreeMB() * (1.0 / 1024.0)};
    const int FreeMinutes {cVideoDiskUsage::FreeMinutes()};
    cString IconName{""};
    cString Extra1 {""}, Extra2 {""};

    if (DiskFreePercent == 0) {  // Show something if disk is full. Avoid DIV/0
        if (Config.DiskUsageFree == 1) {  // Show in free mode
            const div_t FreeHM {std::div(FreeMinutes, 60)};
            if (Config.DiskUsageShort == false) {  // Long format
                Extra1 = cString::sprintf("%s: ~ 0%% %s", tr("Disk"), tr("free"));
                Extra2 = cString::sprintf("%.2f GB ≈ %02d:%02d", FreeGB, FreeHM.quot, FreeHM.rem);
            } else {  // Short format
                Extra1 = cString::sprintf("~ 0%% %s", tr("free"));
                Extra2 = cString::sprintf("≈ %02d:%02d", FreeHM.quot, FreeHM.rem);
            }
            IconName = "chart31b";
        } else {  // Show in occupied mode
            if (Config.DiskUsageShort == false) {  // Long format
                Extra1 = cString::sprintf("%s: ~ 100%% %s", tr("Disk"), tr("occupied"));
                Extra2 = "? GB ≈ ??:??";  //* Can not be calculated if disk is full (DIV/0)
            } else {  // Short format
                Extra1 = cString::sprintf("~ 100%% %s", tr("occupied"));
                Extra2 = "≈ ??:??";
            }
            IconName = "chart32";
        }

        TopBarSetTitleExtra(*Extra1, *Extra2);
        TopBarSetExtraIcon(*IconName);

        return;
    }  // DiskFreePercent == 0 (Show something if disk is full)

    const double GBScale {100.0 / DiskFreePercent};
    const double AllGB {FreeGB * GBScale};  // All disk space in GB
    const double AllMinutes{static_cast<double>(FreeMinutes) * GBScale};  // All disk space in minutes

    if (Config.DiskUsageFree == 1) {  // Show in free mode
        const div_t FreeHM {std::div(FreeMinutes, 60)};
#ifdef DEBUGFUNCSCALL
        dsyslog("   DiskFreePercent %d, FreeMinutes %d", DiskFreePercent, FreeMinutes);
        dsyslog("   FreeGB %.2f, AllGB %.2f, AllMinutes %.2f", FreeGB, AllGB, AllMinutes);
        dsyslog("   FreeMinutes/60 %d, FreeMinutes%%60 %d", FreeHM.quot, FreeHM.rem);
#endif
        if (Config.DiskUsageShort == false) {  // Long format
            Extra1 = cString::sprintf("%s: %d%% %s", tr("Disk"), DiskFreePercent, tr("free"));
            if (FreeGB < 1000.0) {  // Less than 1000 GB
                Extra2 = cString::sprintf("%.1f GB ≈ %02d:%02d", FreeGB, FreeHM.quot, FreeHM.rem);
            } else {  // 1000 GB+
                Extra2 = cString::sprintf("%.2f TB ≈ %02d:%02d", FreeGB * (1.0 / 1024.0), FreeHM.quot, FreeHM.rem);
            }
        } else {  // Short format
            Extra1 = cString::sprintf("%d%% %s", DiskFreePercent, tr("free"));
            Extra2 = cString::sprintf("≈ %02d:%02d", FreeHM.quot, FreeHM.rem);
        }
        // Rewrite switch with a mathematical formula. This is a lot faster than a switch with 32 cases.
        const int IconIndex {(DiskFreePercent * 31) / 100};
        IconName = cString::sprintf("chart%db", IconIndex);  // chart0b - chart31b
#ifdef DEBUGFUNCSCALL
        dsyslog("   IconIndex %d, IconName %s", IconIndex, *IconName);
#endif

    } else {  // Show in occupied mode
        const double OccupiedGB {AllGB - FreeGB};
        const int OccupiedMinutes = AllMinutes - FreeMinutes;  // Narrowing conversion
#ifdef DEBUGFUNCSCALL
        dsyslog("   DiskUsagePercent %d, OccupiedMinutes %d", DiskUsagePercent, OccupiedMinutes);
        dsyslog("   OccupiedGB %.2f, AllGB %.2f, OccupiedMinutes %d", OccupiedGB, AllGB, OccupiedMinutes);
#endif

        if (Config.DiskUsageFree == 2) {  //* Special mixed mode free time instead of used
            const div_t FreeHM {std::div(FreeMinutes, 60)};
            if (Config.DiskUsageShort == false) {  // Long format
                Extra1 = cString::sprintf("%s: %d%% %s", tr("Disk"), DiskUsagePercent, tr("occupied"));
                if (OccupiedGB < 1000.0) {  // Less than 1000 GB
                    Extra2 = cString::sprintf("%.1f GB | %02d:%02d", OccupiedGB, FreeHM.quot, FreeHM.rem);
                } else {  // 1000 GB+
                    Extra2 =
                        cString::sprintf("%.2f TB | %02d:%02d", OccupiedGB * (1.0 / 1024.0), FreeHM.quot, FreeHM.rem);
                }
            } else {  // Short format
                Extra1 = cString::sprintf("%d%% %s", DiskUsagePercent, tr("occupied"));
                Extra2 = cString::sprintf("≈ %02d:%02d", FreeHM.quot, FreeHM.rem);
            }
        } else {  // Show in occupied mode
            const div_t OccupiedHM {std::div(OccupiedMinutes, 60)};
            if (Config.DiskUsageShort == false) {  // Long format
                Extra1 = cString::sprintf("%s: %d%% %s", tr("Disk"), DiskUsagePercent, tr("occupied"));
                if (OccupiedGB < 1000.0) {  // Less than 1000 GB
                    Extra2 = cString::sprintf("%.1f GB ≈ %02d:%02d", OccupiedGB, OccupiedHM.quot, OccupiedHM.rem);
                } else {  // 1000 GB+
                    Extra2 = cString::sprintf("%.2f TB ≈ %02d:%02d", OccupiedGB * (1.0 / 1024.0), OccupiedHM.quot,
                                              OccupiedHM.rem);
                }
            } else {  // Short format
                Extra1 = cString::sprintf("%d%% %s", DiskUsagePercent, tr("occupied"));
                Extra2 = cString::sprintf("≈ %02d:%02d", OccupiedHM.quot, OccupiedHM.rem);
            }
        }

        // Rewrite switch with a mathematical formula. This is a lot faster than a switch with 32 cases.
        const int IconIndex {(DiskUsagePercent * 31) / 100};
        IconName = cString::sprintf("chart%d", IconIndex + 1);  // chart1 - chart32
    }

    TopBarSetTitleExtra(*Extra1, *Extra2);
    TopBarSetExtraIcon(*IconName);
}

//* Should be called with every "Flush"!
void cFlatBaseRender::TopBarUpdate() {
    const time_t Now {time(0)};
    if (m_TopBarUpdateTitle || (Now > m_TopBarLastDate + 60)) {
#ifdef DEBUGFUNCSCALL
        dsyslog("flatPlus: cFlatBaseRender::TopBarUpdate() Updating TopBar");
#endif

        m_TopBarUpdateTitle = false;
        m_TopBarLastDate = Now;
        if (!TopBarPixmap || !TopBarIconPixmap || !TopBarIconBgPixmap)  // Check only if we have something to do
            return;

        const int TopBarWidth {m_OsdWidth - Config.decorBorderTopBarSize * 2};
        int MenuIconWidth {0};

        const int FontTop {(m_TopBarHeight - m_TopBarFontHeight) / 2};
        const int FontSmlTop {(m_TopBarHeight - m_TopBarFontSmlHeight * 2) / 2};
        const int FontClockTop {(m_TopBarHeight - m_TopBarFontClockHeight) / 2};

        PixmapFill(TopBarPixmap, Theme.Color(clrTopBarBg));
        PixmapClear(TopBarIconPixmap);
        PixmapClear(TopBarIconBgPixmap);

        const int TopBarLogoHeight {m_TopBarHeight - m_MarginItem2};      // Height of TopBar
        const int TopBarIconHeight {m_TopBarFontHeight - m_MarginItem2};  // Height of font in TopBar

        cImage *img {nullptr};
        if (Config.TopBarMenuIconShow) {
            int IconLeft {m_MarginItem};
            int IconTop {0};
            if (m_TopBarMenuLogoSet) {  // Show menu channel logo
                int ImageBGHeight {TopBarLogoHeight};
                int ImageBGWidth = ImageBGHeight * 1.34f;  // Narrowing conversion

                img = ImgLoader.LoadIcon("logo_background", ImageBGWidth, ImageBGHeight);
                if (img) {
                    ImageBGHeight = img->Height();
                    ImageBGWidth = img->Width();
                    IconTop = (m_TopBarHeight - ImageBGHeight) / 2;
                    TopBarIconBgPixmap->DrawImage(cPoint(IconLeft, IconTop), *img);
                }

                img = ImgLoader.LoadLogo(*m_TopBarMenuLogo, ImageBGWidth - 4, ImageBGHeight - 4);
                if (img) {
                    IconTop += (ImageBGHeight - img->Height()) / 2;
                    IconLeft += (ImageBGWidth - img->Width()) / 2;
                    TopBarIconPixmap->DrawImage(cPoint(IconLeft, IconTop), *img);
                }
                MenuIconWidth = ImageBGWidth + m_MarginItem2;
            } else if (m_TopBarMenuIconSet) {  // Show menu icon
                img = ImgLoader.LoadIcon(*m_TopBarMenuIcon, ICON_WIDTH_UNLIMITED, TopBarLogoHeight);
                if (img) {
                    IconTop = (m_TopBarHeight - img->Height()) / 2;
                    TopBarIconPixmap->DrawImage(cPoint(IconLeft, IconTop), *img);
                    MenuIconWidth = img->Width() + m_MarginItem2;
                }
            }
        }  // Config.TopBarMenuIconShow

        const cString time {*TimeString(Now)};  // Reuse 'Now'
        cString Buffer {""};
        if (Config.TopBarHideClockText)
            Buffer = *time;
        else
            Buffer = cString::sprintf("%s %s", *time, tr("clock"));

        const int TimeWidth {m_TopBarFontClock->Width(*Buffer) + m_MarginItem2};
        int Right {TopBarWidth - TimeWidth};
        TopBarPixmap->DrawText(cPoint(Right, FontClockTop), *Buffer, Theme.Color(clrTopBarTimeFont),
                               Theme.Color(clrTopBarBg), m_TopBarFontClock);

        const cString WeekDay {*WeekDayNameFull(Now)};
        const cString DateStr {*ShortDateString(Now)};
        const int MaxDateWidth {std::max(m_TopBarFontSml->Width(*WeekDay), m_TopBarFontSml->Width(*DateStr))};

        Right = TopBarWidth - TimeWidth - MaxDateWidth - m_MarginItem;
        TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *WeekDay, Theme.Color(clrTopBarDateFont),
                               Theme.Color(clrTopBarBg), m_TopBarFontSml, MaxDateWidth, 0,
                               taRight);
        TopBarPixmap->DrawText(cPoint(Right, FontSmlTop + m_TopBarFontSmlHeight), *DateStr,
                               Theme.Color(clrTopBarDateFont), Theme.Color(clrTopBarBg), m_TopBarFontSml, MaxDateWidth,
                               0, taRight);

        int MiddleWidth {0}, NumConflicts {0};
        cImage *ImgCon {nullptr};
        if (Config.TopBarRecConflictsShow) {  // Load conflict icon
            NumConflicts = GetEpgsearchConflicts();  // Get conflicts from plugin Epgsearch
            if (NumConflicts) {
                if (NumConflicts < Config.TopBarRecConflictsHigh)
                    ImgCon = ImgLoader.LoadIcon("topbar_timerconflict_low", TopBarIconHeight, TopBarIconHeight);
                else
                    ImgCon = ImgLoader.LoadIcon("topbar_timerconflict_high", TopBarIconHeight, TopBarIconHeight);

                if (ImgCon) {
                    Buffer = cString::sprintf("%d", NumConflicts);
                    const int BufferWidth {m_TopBarFontSml->Width(*Buffer)};  // Width of number of conflicts
                    Right -= ImgCon->Width() + BufferWidth + m_MarginItem;
                    MiddleWidth += ImgCon->Width() + BufferWidth + m_MarginItem;
                }
            }
        }  // Config.TopBarRecConflictsShow

        uint16_t NumRec {0};  // 65535 should be enough for the number of recordings
        cImage *ImgRec {nullptr};
        if (Config.TopBarRecordingShow) {  // Load recording icon and number of recording timers
#ifdef DEBUGFUNCSCALL
            dsyslog("   Get number of recording timers");
            cTimeMs Timer;  // Start Timer
#endif

            // The code below is a workaround for a problem with the VDR thread handling.
            // The VDR is not designed to handle multiple threads, which is why we have
            // to use a workaround to get the number of current recordings.
            // The following code creates a new thread that queries the number of
            // recordings and waits for the result. This is necessary because the
            // cTimers::GetTimers() function can only be called from the main thread.
            // The result is then stored in the NumRec variable.
            /* auto RecCounterFuture = std::async(std::launch::async, [&NumRec]() {
                LOCK_TIMERS_READ;  // Creates local const cTimers *Timers
                for (const cTimer *Timer {Timers->First()}; Timer; Timer = Timers->Next(Timer)) {
                    if (Timer->HasFlags(tfRecording))
                        ++NumRec;
                }
            });
            RecCounterFuture.get(); */
            //* FAST RECORD COUNT: Use cached background thread or event value
            RecCountCache.UpdateIfNeeded();
            NumRec = s_NumRecordings.load(std::memory_order_relaxed);
            //* END FAST RECORD COUNT

#ifdef DEBUGFUNCSCALL
            if (Timer.Elapsed() > 0)
                dsyslog("   Got %d recording timers after %ld ms", NumRec, Timer.Elapsed());
#endif

            if (NumRec) {
                ImgRec = ImgLoader.LoadIcon("topbar_timer", TopBarIconHeight, TopBarIconHeight);
                if (ImgRec) {  // Load recording icon
                    Buffer = cString::sprintf("%d", NumRec);
                    const int BufferWidth {m_TopBarFontSml->Width(*Buffer)};  // Width of number of recordings
                    Right -= ImgRec->Width() + BufferWidth + m_MarginItem;
                    MiddleWidth += ImgRec->Width() + BufferWidth + m_MarginItem;
                }
            }
        }  // Config.TopBarRecordingShow

        cImage *ImgExtra {nullptr};
        if (m_TopBarExtraIconSet) {  // Load extra icon (Disk usage) with full height of TopBar
            ImgExtra = ImgLoader.LoadIcon(*m_TopBarExtraIcon, ICON_WIDTH_UNLIMITED, m_TopBarHeight);
            if (ImgExtra) {
                Right -= ImgExtra->Width() + m_MarginItem;
                MiddleWidth += ImgExtra->Width() + m_MarginItem;
            }
        }

        int TopBarMenuIconRightWidth {0};
        int TitleWidth {m_TopBarFont->Width(*m_TopBarTitle)};
        cImage *ImgIconRight {nullptr};
        if (m_TopBarMenuIconRightSet) {  // Load sort icon
            ImgIconRight = ImgLoader.LoadIcon(*m_TopBarMenuIconRight, ICON_WIDTH_UNLIMITED, TopBarLogoHeight);
            if (ImgIconRight) {
                TopBarMenuIconRightWidth = ImgIconRight->Width() + m_MarginItem3;
                TitleWidth += TopBarMenuIconRightWidth;
            }
        }

        const int ExtraMaxWidth {std::max(m_TopBarFontSml->Width(*m_TopBarTitleExtra1),
                                          m_TopBarFontSml->Width(*m_TopBarTitleExtra2))};
        MiddleWidth += ExtraMaxWidth;
        Right -= ExtraMaxWidth + m_MarginItem;

        const int MiddleX {(TopBarWidth - MiddleWidth) / 2};
        const int TitleLeft {MenuIconWidth + m_MarginItem2};
        const int TitleRight {TitleLeft + TitleWidth};
        if (TitleRight < MiddleX)
            Right = MiddleX;
        else if (TitleRight < Right)
            Right = TitleRight + m_MarginItem;

        int TitleMaxWidth {Right - TitleLeft - m_MarginItem};

        TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *m_TopBarTitleExtra1, Theme.Color(clrTopBarDateFont),
                               Theme.Color(clrTopBarBg), m_TopBarFontSml, ExtraMaxWidth, 0, taRight);
        TopBarPixmap->DrawText(cPoint(Right, FontSmlTop + m_TopBarFontSmlHeight), *m_TopBarTitleExtra2,
                               Theme.Color(clrTopBarDateFont), Theme.Color(clrTopBarBg), m_TopBarFontSml, ExtraMaxWidth,
                               0, taRight);
        Right += ExtraMaxWidth + m_MarginItem;

        if (m_TopBarExtraIconSet && ImgExtra) {  // Draw extra icon (Disk usage)
                const int IconTop {(m_TopBarHeight - ImgExtra->Height()) / 2};
                TopBarIconPixmap->DrawImage(cPoint(Right, IconTop), *ImgExtra);
                Right += ImgExtra->Width() + m_MarginItem;
        }

        if (NumRec && ImgRec) {  // Draw recording icon and number of recording timers
            const int IconTop {(m_TopBarHeight - ImgRec->Height()) / 2};
            TopBarIconPixmap->DrawImage(cPoint(Right, IconTop), *ImgRec);
            Right += ImgRec->Width();

            Buffer = cString::sprintf("%d", NumRec);
            TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *Buffer, Theme.Color(clrTopBarRecordingActiveFg),
                                   Theme.Color(clrTopBarRecordingActiveBg), m_TopBarFontSml);
            Right += m_TopBarFontSml->Width(*Buffer) + m_MarginItem;
        }

        if (NumConflicts && ImgCon) {  // Draw conflict icon and number of conflicts
            const int IconTop {(m_TopBarHeight - ImgCon->Height()) / 2};
            TopBarIconPixmap->DrawImage(cPoint(Right, IconTop), *ImgCon);
            Right += ImgCon->Width();

            Buffer = cString::sprintf("%d", NumConflicts);
            if (NumConflicts < Config.TopBarRecConflictsHigh)
                TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *Buffer, Theme.Color(clrTopBarConflictLowFg),
                                       Theme.Color(clrTopBarConflictLowBg), m_TopBarFontSml);
            else
                TopBarPixmap->DrawText(cPoint(Right, FontSmlTop), *Buffer, Theme.Color(clrTopBarConflictHighFg),
                                       Theme.Color(clrTopBarConflictHighBg), m_TopBarFontSml);
            Right += m_TopBarFontSml->Width(*Buffer) + m_MarginItem;
        }

        int TopBarMenuIconRightLeft {0};
        if (TitleWidth + TopBarMenuIconRightWidth > TitleMaxWidth) {
            TopBarMenuIconRightLeft = TitleMaxWidth + m_MarginItem2;
            TitleMaxWidth -= TopBarMenuIconRightWidth;
        } else {
            TopBarMenuIconRightLeft = TitleRight + m_MarginItem2;
        }

        if (m_TopBarMenuIconRightSet && ImgIconRight) {  // Draw sort icon
            const int IconTop {(m_TopBarHeight - ImgIconRight->Height()) / 2};
            TopBarIconPixmap->DrawImage(cPoint(TopBarMenuIconRightLeft, IconTop), *ImgIconRight);
        }

        TopBarPixmap->DrawText(cPoint(TitleLeft, FontTop), *m_TopBarTitle, Theme.Color(clrTopBarFont),
                               Theme.Color(clrTopBarBg), m_TopBarFont, TitleMaxWidth);

        const sDecorBorder ib {Config.decorBorderTopBarSize,
                               Config.decorBorderTopBarSize,
                               m_OsdWidth - Config.decorBorderTopBarSize * 2,
                               m_TopBarHeight,
                               Config.decorBorderTopBarSize,
                               Config.decorBorderTopBarType,
                               Config.decorBorderTopBarFg,
                               Config.decorBorderTopBarBg};
        DecorBorderDraw(ib);
    }
}

void cFlatBaseRender::ButtonsCreate() {
    m_ButtonsHeight = m_FontHeight + m_MarginButtonColor + m_ButtonColorHeight;
    m_ButtonsWidth = m_OsdWidth;
    m_ButtonsTop = m_OsdHeight - m_ButtonsHeight - Config.decorBorderButtonSize;

    ButtonsPixmap = CreatePixmap(m_Osd, "ButtonsPixmap", 1,
                                 cRect(Config.decorBorderButtonSize, m_ButtonsTop,
                                       m_ButtonsWidth - Config.decorBorderButtonSize * 2, m_ButtonsHeight));
    PixmapClear(ButtonsPixmap);
}

void cFlatBaseRender::ButtonsSet(const char *Red, const char *Green, const char *Yellow, const char *Blue) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatBaseRender::ButtonsSet() '%s' '%s' '%s' '%s'", Red, Green, Yellow, Blue);
#endif

    if (!ButtonsPixmap) return;

    const int WidthMargin {m_ButtonsWidth - m_MarginItem3};
    int ButtonWidth {WidthMargin / 4 - Config.decorBorderButtonSize * 2};

    PixmapClear(ButtonsPixmap);
    DecorBorderClearByFrom(BorderButton);

    m_ButtonsDrawn = false;

    const char *ButtonText[] {Red, Green, Yellow, Blue};  // ButtonText
    const tColor ButtonColor[] {clrButtonRed, clrButtonGreen, clrButtonYellow, clrButtonBlue};  // ButtonColor
    const int ColorKey[] {Setup.ColorKey0, Setup.ColorKey1, Setup.ColorKey2, Setup.ColorKey3};  // ColorKey

    int x {0};

    for (int i {0}; i < 4; i++) {  // Four buttons
        // If there is enough space for the last button, add its width to the
        // right edge of the buttons area. This is done to align the last button
        // to the right edge of the screen.
        if (i == 3 && x + ButtonWidth + Config.decorBorderButtonSize * 2 < m_ButtonsWidth)
            ButtonWidth += m_ButtonsWidth - (x + ButtonWidth + Config.decorBorderButtonSize * 2);

        // If buttons should be shown even when empty, or if there is some text to show
        if (!(Config.ButtonsShowEmpty == 0 && ButtonText[ColorKey[i]] == nullptr)) {
            ButtonsPixmap->DrawText(cPoint(x, 0), ButtonText[ColorKey[i]], Theme.Color(clrButtonFont),
                                    Theme.Color(clrButtonBg), m_Font, ButtonWidth, m_FontHeight + m_MarginButtonColor,
                                    taCenter);
            ButtonsPixmap->DrawRectangle(cRect(x, m_FontHeight + m_MarginButtonColor, ButtonWidth, m_ButtonColorHeight),
                                         Theme.Color(ButtonColor[ColorKey[i]]));

            const sDecorBorder ib{x + Config.decorBorderButtonSize,
                                  m_ButtonsTop,
                                  ButtonWidth,
                                  m_ButtonsHeight,
                                  Config.decorBorderButtonSize,
                                  Config.decorBorderButtonType,
                                  Config.decorBorderButtonFg,
                                  Config.decorBorderButtonBg,
                                  BorderButton};
            DecorBorderDraw(ib);
            m_ButtonsDrawn = true;
        }
        x += ButtonWidth + m_MarginItem + Config.decorBorderButtonSize * 2;  // Add button width and margin
    }  // for (int8_t i = 0; i < 4; i++)
}

bool cFlatBaseRender::ButtonsDrawn() const {
    return m_ButtonsDrawn;
}

void cFlatBaseRender::MessageCreate() {
    m_MessageHeight = m_FontHeight + m_MarginItem2;
    if (Config.MessageColorPosition == 1)
        m_MessageHeight += 8;

    const int top {m_OsdHeight - Config.MessageOffset - m_MessageHeight - Config.decorBorderMessageSize};
    const cRect MessagePixmapViewPort {Config.decorBorderMessageSize, top,
                                       m_OsdWidth - Config.decorBorderMessageSize * 2, m_MessageHeight};
    MessagePixmap = CreatePixmap(m_Osd, "MessagePixmap", 5, MessagePixmapViewPort);
    MessageIconPixmap = CreatePixmap(m_Osd, "MessageIconPixmap", 5, MessagePixmapViewPort);
    PixmapClear(MessagePixmap);
    PixmapClear(MessageIconPixmap);

    MessageScroller.SetOsd(m_Osd);
    MessageScroller.SetScrollStep(Config.ScrollerStep);
    MessageScroller.SetScrollDelay(Config.ScrollerDelay);
    MessageScroller.SetScrollType(Config.ScrollerType);
    MessageScroller.SetPixmapLayer(5);
}

void cFlatBaseRender::MessageSet(eMessageType Type, const char *Text) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatBaseRender::MessageSet()");
    dsyslog("   Setup.OSDMessageTime: %d, m_OSDMessageTime: %d", Setup.OSDMessageTime, m_OSDMessageTime);
#endif

    if (!MessagePixmap || !MessageIconPixmap)
        return;

    static const struct {
        tColor color;
        const char* icon;
    } MessageSettings[] = {
        {Theme.Color(clrMessageStatus), "message_status"},    // mtStatus = 0
        {Theme.Color(clrMessageInfo), "message_info"},        // mtInfo
        {Theme.Color(clrMessageWarning), "message_warning"},  // mtWarning
        {Theme.Color(clrMessageError), "message_error"}       // mtError
    };

    const int TypeIndex {static_cast<int>(Type)};
    const tColor Col {MessageSettings[TypeIndex].color};
    const cString Icon = MessageSettings[TypeIndex].icon;

    PixmapFill(MessagePixmap, Theme.Color(clrMessageBg));
    MessageScroller.Clear();

    cImage *img {ImgLoader.LoadIcon(*Icon, m_FontHeight, m_FontHeight)};
    if (img)
        MessageIconPixmap->DrawImage(cPoint(m_MarginItem + 10, m_MarginItem), *img);

    if (Config.MessageColorPosition == 0) {  // Vertical
        MessagePixmap->DrawRectangle(cRect(0, 0, 8, m_MessageHeight), Col);
        MessagePixmap->DrawRectangle(cRect(m_OsdWidth - 8 - Config.decorBorderMessageSize * 2, 0, 8, m_MessageHeight),
                                     Col);
    } else {  // Horizontal
        MessagePixmap->DrawRectangle(cRect(0, m_MessageHeight - 8, m_OsdWidth, 8), Col);
    }

    const int TextWidth {m_Font->Width(Text)};
    const int MaxWidth {m_OsdWidth - Config.decorBorderMessageSize * 2 - m_FontHeight - m_MarginItem3 - 10};

    if ((TextWidth > MaxWidth) && Config.ScrollerEnable) {
        MessageScroller.AddScroller(
            Text,
            cRect(Config.decorBorderMessageSize + m_FontHeight + m_MarginItem3 + 10,
                  m_OsdHeight - Config.MessageOffset - m_MessageHeight - Config.decorBorderMessageSize + m_MarginItem,
                  MaxWidth, m_FontHeight),
            Theme.Color(clrMessageFont), clrTransparent, m_Font, Theme.Color(clrMenuItemExtraTextFont));
    } else if (Config.MenuItemParseTilde) {
        const char *TildePos {strchr(Text, '~')};
        if (TildePos) {
            std::string_view sv1 {Text, static_cast<size_t>(TildePos - Text)};
            std::string_view sv2 {TildePos + 1};
            const std::string first {rtrim(sv1)};  // Trim possible space at end
            std::string_view second {ltrim(sv2)};  // Trim possible space at begin

            MessagePixmap->DrawText(cPoint((m_OsdWidth - TextWidth) / 2, m_MarginItem), first.c_str(),
                                    Theme.Color(clrMessageFont), Theme.Color(clrMessageBg), m_Font);
            const int l {m_Font->Width(first.c_str()) + m_Font->Width('X')};
            MessagePixmap->DrawText(cPoint((m_OsdWidth - TextWidth) / 2 + l, m_MarginItem), second.data(),
                                    Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrMessageBg), m_Font);
        } else {  // ~ not found
            MessagePixmap->DrawText(cPoint((m_OsdWidth - TextWidth) / 2, m_MarginItem), Text,
                                     Theme.Color(clrMessageFont), Theme.Color(clrMessageBg), m_Font);
        }
    } else {  // Default: Not scrolling, not parsing tilde
            MessagePixmap->DrawText(cPoint((m_OsdWidth - TextWidth) / 2, m_MarginItem), Text,
                                    Theme.Color(clrMessageFont), Theme.Color(clrMessageBg), m_Font);
    }

    const int top {m_OsdHeight - Config.MessageOffset - m_MessageHeight - Config.decorBorderMessageSize};
    const sDecorBorder ib {Config.decorBorderMessageSize,
                           top,
                           m_OsdWidth - Config.decorBorderMessageSize * 2,
                           m_MessageHeight,
                           Config.decorBorderMessageSize,
                           Config.decorBorderMessageType,
                           Config.decorBorderMessageFg,
                           Config.decorBorderMessageBg,
                           BorderMessage};
    DecorBorderDraw(ib);
}

void cFlatBaseRender::MessageSetExtraTime(const char *Text) {  // For long messages increase 'OSDMessageTime'
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatBaseRender::MessageSetExtraTime()");
#endif

    static constexpr uint16_t kThreshold {75};  //? Add config option?
    const std::size_t TextLength {strlen(Text)};
    if (TextLength > kThreshold) {  // Message is longer than kThreshold and uses almost the full screen
        // Narrowing conversion
        const int ExtraTime {
            std::min(static_cast<int>((TextLength - kThreshold) / (kThreshold / Setup.OSDMessageTime)),
                     Setup.OSDMessageTime * 3)};  // Max. extra time to add
#ifdef DEBUGFUNCSCALL
        dsyslog("   Adding %d seconds to message time (%d)", ExtraTime + 1, m_OSDMessageTime);
#endif
        Setup.OSDMessageTime += ExtraTime + 1;  // Add time of displaying message
    }
}

void cFlatBaseRender::MessageClear() {
    PixmapClear(MessagePixmap);
    PixmapClear(MessageIconPixmap);
    DecorBorderClearByFrom(BorderMessage);
    DecorBorderRedrawAll();
    MessageScroller.Clear();
}

void cFlatBaseRender::ProgressBarCreate(const cRect &Rect, int MarginHor, int MarginVer, tColor ColorFg,
                                        tColor ColorBarFg, tColor ColorBg, int Type, bool SetBackground,
                                        bool IsSignal) {
    m_ProgressBarTop = Rect.Top();
    m_ProgressBarWidth = Rect.Width();
    m_ProgressBarHeight = Rect.Height();
    m_ProgressType = Type;
    m_ProgressBarMarginHor = MarginHor;
    m_ProgressBarMarginVer = MarginVer;

    m_ProgressBarColorFg = ColorFg;
    m_ProgressBarColorBarFg = ColorBarFg;
    m_ProgressBarColorBg = ColorBg;

    m_ProgressBarSetBackground = SetBackground;
    m_ProgressBarIsSignal = IsSignal;

    m_ProgressBarColorBarCurFg = Theme.Color(clrReplayProgressBarCurFg);

    // Layer 4: Marker pixmap - topmost layer for progress markers
    ProgressBarMarkerPixmap = CreatePixmap(m_Osd, "ProgressBarMarkerPixmap", 4, Rect);

    // Layer 3: Main progress bar and error marks
    ProgressBarPixmap = CreatePixmap(m_Osd, "ProgressBarPixmap", 3, Rect);

    // Layer 2: Background layer with margins
    ProgressBarPixmapBg = CreatePixmap(m_Osd, "ProgressBarPixmapBg", 2,
                                       cRect(Rect.Left() - MarginVer, Rect.Top() - MarginHor,
                                             Rect.Width() + MarginVer * 2, Rect.Height() + MarginHor * 2));
    PixmapClear(ProgressBarMarkerPixmap);
    PixmapClear(ProgressBarPixmap);
    PixmapClear(ProgressBarPixmapBg);
}

void cFlatBaseRender::ProgressBarDraw(int Current, int Total) {
    ProgressBarDrawRaw(
        ProgressBarPixmap, ProgressBarPixmapBg, cRect(0, 0, m_ProgressBarWidth, m_ProgressBarHeight),
        cRect(0, 0, m_ProgressBarWidth + m_ProgressBarMarginVer * 2, m_ProgressBarHeight + m_ProgressBarMarginHor * 2),
        Current, Total, m_ProgressBarColorFg, m_ProgressBarColorBarFg, m_ProgressBarColorBg, m_ProgressType,
        m_ProgressBarSetBackground, m_ProgressBarIsSignal);
}

void cFlatBaseRender::ProgressBarDrawBgColor() const {
    // PixmapFill(ProgressBarPixmapBg, m_ProgressBarColorBg);
    static tColor LastBg {0};
    if (m_ProgressBarColorBg != LastBg) {
        // dsyslog("flatPlus: cFlatBaseRender::ProgressBarDrawBgColor() Fill ProgressBarPixmapBg");
        PixmapFill(ProgressBarPixmapBg, m_ProgressBarColorBg);
        LastBg = m_ProgressBarColorBg;
    }
}

void cFlatBaseRender::ProgressBarDrawRaw(cPixmap *Pixmap, cPixmap *PixmapBg, const cRect &rect, const cRect &rectBg,
                                         int Current, int Total, tColor ColorFg, tColor ColorBarFg, tColor ColorBg,
                                         int Type, bool SetBackground, bool IsSignal) {
    if (!Pixmap) return;

    if (PixmapBg && SetBackground)
        PixmapBg->DrawRectangle(rectBg, ColorBg);

    if (SetBackground) {
        if (PixmapBg == Pixmap)
            Pixmap->DrawRectangle(rect, ColorBg);
        else
            Pixmap->DrawRectangle(rect, clrTransparent);
    }

    if (Total == 0) {  // Avoid DIV/0
        esyslog("flatPlus: Error in cFlatBaseRender::ProgressBarDrawRaw() Total is 0!");
        return;
    }

    if (rect.Width() == 0 || rect.Height() == 0) return;  // Check for zero values

    const int big {rect.Height()};
    const int Middle {big / 2};
    const float Percent {static_cast<float>(Current) / Total};
    const int PercentPos {static_cast<int>(rect.Width() * Percent)};
    switch (Type) {
    case 0:  // Small line + big line
    {
        const int sml {std::max(big / 10 * 2, 2)};

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + Middle - (sml / 2), rect.Width(), sml), ColorFg);

        if (Current > 0)
            Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), PercentPos, big), ColorBarFg);
        break;
    }
    case 1:  // big line
    {
        if (Current > 0)
            Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), PercentPos, big), ColorBarFg);
        break;
    }
    case 2:  // big line + outline
    {
        const int out {big > 10 ? 2 : 1};
        // Outline
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + big - out, rect.Width(), out), ColorFg);

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), out, big), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left() + rect.Width() - out, rect.Top(), out, big), ColorFg);

        if (Current > 0) {
            const int out2 {out * 2};
            if (IsSignal) {
                const int GreenWidth {static_cast<int>(rect.Width() * 0.666) - out2};
                const int YellowWidth {static_cast<int>(rect.Width() * 0.333) - out2};

                if (Percent > 0.666f) {
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + out, PercentPos - out2, big - out2),
                            Theme.Color(clrButtonGreen));
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + out, GreenWidth, big - out2),
                            Theme.Color(clrButtonYellow));
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + out, YellowWidth, big - out2),
                            Theme.Color(clrButtonRed));
                } else if (Percent > 0.333f) {
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + out, PercentPos - out2, big - out2),
                            Theme.Color(clrButtonYellow));
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + out, YellowWidth, big - out2),
                            Theme.Color(clrButtonRed));
                } else {
                    Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + out, PercentPos - out2, big - out2),
                            Theme.Color(clrButtonRed));
                }
            } else {
                Pixmap->DrawRectangle(cRect(rect.Left() + out, rect.Top() + out, PercentPos - out2, big - out2),
                            ColorBarFg);
            }
        }
        break;
    }
    case 3:  // Small line + big line + dot
    {
        const int sml {std::max(big / 10 * 2, 2)};

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + Middle - (sml / 2), rect.Width(), sml), ColorFg);

        if (Current > 0) {
            Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), PercentPos, big), ColorBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + PercentPos - Middle, rect.Top(), big, big), ColorBarFg, 0);
        }
        break;
    }
    case 4:  // big line + dot
    {
        if (Current > 0) {
            Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), PercentPos, big), ColorBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + PercentPos - Middle, rect.Top(), big, big), ColorBarFg, 0);
        }
        break;
    }
    case 5:  // big line + outline + dot
    {
        const int out {big > 10 ? 2 : 1};
        // Outline
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + big - out, rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), out, big), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left() + rect.Width() - out, rect.Top(), out, big), ColorFg);

        if (Current > 0) {
            Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), PercentPos, big), ColorBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + PercentPos - Middle, rect.Top(), big, big), ColorBarFg, 0);
        }
        break;
    }
    case 6:  // Small line + dot
    {
        const int sml {std::max(big / 10 * 2, 2)};

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + Middle - (sml / 2), rect.Width(), sml), ColorFg);

        if (Current > 0) {
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + PercentPos - Middle, rect.Top(), big, big), ColorBarFg, 0);
        }
        break;
    }
    case 7:  // Outline + dot
    {
        const int out {big > 10 ? 2 : 1};
        // Outline
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + big - out, rect.Width(), out), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top(), out, big), ColorFg);
        Pixmap->DrawRectangle(cRect(rect.Left() + rect.Width() - out, rect.Top(), out, big), ColorFg);

        if (Current > 0) {
            // Dot
            Pixmap->DrawEllipse(cRect(rect.Left() + PercentPos - Middle, rect.Top(), big, big), ColorBarFg, 0);
        }
        break;
    }
    case 8:  // Small line + big line + alpha blend
    {
        const int sml {std::max(rect.Height() / 10 * 2, 2)};
        const int big {Middle - (sml / 2)};

        Pixmap->DrawRectangle(cRect(rect.Left(), rect.Top() + Middle - (sml / 2), rect.Width(), sml), ColorFg);

        if (Current > 0) {
            DecorDrawGlowRectHor(Pixmap, rect.Left(), rect.Top(), PercentPos, big, ColorBarFg);
            DecorDrawGlowRectHor(Pixmap, rect.Left(), rect.Top() + Middle + (sml / 2), PercentPos, big * -1,
                                 ColorBarFg);
        }
        break;
    }
    case 9:  // big line + alpha blend
    {
        if (Current > 0) {
            DecorDrawGlowRectHor(Pixmap, rect.Left(), rect.Top(), PercentPos, Middle, ColorBarFg);
            DecorDrawGlowRectHor(Pixmap, rect.Left(), rect.Top() + Middle, PercentPos, (big / -2), ColorBarFg);
        }
        break;
    }
    }
}

#if APIVERSNUM >= 30004
void cFlatBaseRender::ProgressBarDrawMarks(int Current, int Total, const cMarks *Marks, const cErrors *Errors,
                                           tColor Color, tColor ColorCurrent) {
#else
void cFlatBaseRender::ProgressBarDrawMarks(int Current, int Total, const cMarks *Marks, tColor Color,
                                           tColor ColorCurrent) {
#endif
    if (!ProgressBarPixmap || !ProgressBarMarkerPixmap)
        return;

    if (Total == 0)  // Avoid DIV/0
        return;

    m_ProgressBarColorMark = Color;
    m_ProgressBarColorMarkCurrent = ColorCurrent;

    if (ProgressBarPixmapBg)
        ProgressBarPixmapBg->DrawRectangle(
            cRect(0, m_ProgressBarMarginHor + m_ProgressBarHeight, m_ProgressBarWidth, m_ProgressBarMarginHor),
            m_ProgressBarColorBg);

    PixmapFill(ProgressBarPixmap, m_ProgressBarColorBg);
    PixmapClear(ProgressBarMarkerPixmap);

    // Eliminate the division operation in each iteration and potentially reduce the computational cost, especially for
    // large collections of marks.
    const double PosScaleFactor {static_cast<double>(m_ProgressBarWidth) / Total};
    const int PosCurrent {static_cast<int>(Current * PosScaleFactor)};  // Not needed to calculate for every mark
    const int sml {std::max(m_ProgressBarHeight / 10 * 2, 4)};  // 20% of progressbar height with an minimum of 4 pixel
    if (!Marks || !Marks->First()) {
        // m_ProgressBarColorFg = m_ProgressBarColorBarFg; m_ProgressBarColorFg = m_ProgressBarColorBarCurFg;
        m_ProgressBarColorBarFg = m_ProgressBarColorBarCurFg;
        ProgressBarDraw(Current, Total);
    } else {
        const int top {m_ProgressBarHeight / 2};
        const int SmlHalf {sml / 2};
        // The small line
        ProgressBarPixmap->DrawRectangle(cRect(0, top - SmlHalf, m_ProgressBarWidth, sml), m_ProgressBarColorFg);

        int PosMark {0}, PosMarkLast {0};
        bool Start {true};
        for (const cMark *m {Marks->First()}; m; m = Marks->Next(m)) {
            PosMark = static_cast<int>(m->Position() * PosScaleFactor);
            ProgressBarDrawMark(PosMark, PosMarkLast, PosCurrent, Start, m->Position() == Current);
            PosMarkLast = PosMark;
            Start = !Start;
        }

        // Draw last mark vertical line
        if (PosCurrent == PosMark)
            ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMark - sml, 0, sml * 2, m_ProgressBarHeight), ColorCurrent);
        else
            ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMark - SmlHalf, 0, sml, m_ProgressBarHeight), Color);

        const int big {m_ProgressBarHeight - (sml * 2) - 2};
        const int BigHalf {big / 2};
        if (Start) {
            // Marker (Position)
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - SmlHalf, PosCurrent - PosMarkLast, sml),
                                             m_ProgressBarColorBarCurFg);
            ProgressBarPixmap->DrawRectangle(cRect(PosCurrent - BigHalf, top - BigHalf, big, big),
                                             m_ProgressBarColorBarCurFg);

            if (PosCurrent > PosMarkLast + SmlHalf)
                ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMarkLast - SmlHalf, 0, sml, m_ProgressBarHeight),
                                                       Color);
        } else {
            // ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMarkLast + SmlHalf, top - BigHalf,
            //                                        m_ProgressBarWidth - PosMarkLast, big), m_ProgressBarColorBarFg);
            if (PosCurrent > PosMarkLast)
                ProgressBarPixmap->DrawRectangle(
                    cRect(PosMarkLast + SmlHalf, top - BigHalf, PosCurrent - PosMarkLast, big),
                    m_ProgressBarColorBarCurFg);
        }
    }

#if APIVERSNUM >= 30004
    if (Config.PlaybackShowErrorMarks > 0 && Errors) {  // Draw error marks
        int LastPos {-1}, Pos {0};
        const int ErrorsSize {Errors->Size()};
        for (int i {0}; i < ErrorsSize; ++i) {
            Pos = static_cast<int>(Errors->At(i) * PosScaleFactor);  // Position on progressbar in pixel
            if (Pos != LastPos) {                            // Draw mark if pos is not the same as the last one
                ProgressBarDrawError(Pos, sml, Pos == PosCurrent);
                LastPos = Pos;
            }
        }
    }
#endif
}

void cFlatBaseRender::ProgressBarDrawMark(int PosMark, int PosMarkLast, int PosCurrent, bool Start, bool IsCurrent) {
    // if (!ProgressBarPixmap || !ProgressBarMarkerPixmap)  // Checked in calling function 'ProgressBarDrawMarks()'
    //    return;

    const int sml {std::max(m_ProgressBarHeight / 10 * 2, 4)};
    // Mark vertical line
    if (PosCurrent == PosMark)
        ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMark - sml, 0, sml * 2, m_ProgressBarHeight),
                                         m_ProgressBarColorMarkCurrent);
    else
        ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMark - (sml / 2), 0, sml, m_ProgressBarHeight),
                                               m_ProgressBarColorMark);

    const int top {m_ProgressBarHeight / 2};
    const int big {m_ProgressBarHeight - (sml * 2) - 2};
    const int mbig {(m_ProgressBarHeight > 15) ? m_ProgressBarHeight : m_ProgressBarHeight * 2};
    if (Start) {
        if (PosCurrent > PosMark) {
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - (sml / 2), PosMark - PosMarkLast, sml),
                                             m_ProgressBarColorBarCurFg);
        } else {
            // Marker (Position)
            ProgressBarPixmap->DrawRectangle(cRect(PosCurrent - (big / 2), top - (big / 2), big, big),
                                             m_ProgressBarColorBarCurFg);
            if (PosCurrent > PosMarkLast)
                ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - (sml / 2), PosCurrent - PosMarkLast, sml),
                                                 m_ProgressBarColorBarCurFg);
        }
        // Mark top
        if (IsCurrent)
            ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMark - (mbig / 2), 0, mbig, sml),
                                                   m_ProgressBarColorMarkCurrent);
        else
            ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMark - (mbig / 2), 0, mbig, sml), m_ProgressBarColorMark);
    } else {
        // Big line
        if (PosCurrent > PosMark) {
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - (big / 2), PosMark - PosMarkLast, big),
                                             m_ProgressBarColorBarCurFg);
            // Draw last mark top
            ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMarkLast - (mbig / 2), 0, mbig, m_MarginItem / 2),
                                             m_ProgressBarColorMark);
        } else {
            ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - (big / 2), PosMark - PosMarkLast, big),
                                             m_ProgressBarColorBarFg);
            if (PosCurrent > PosMarkLast) {
                ProgressBarPixmap->DrawRectangle(cRect(PosMarkLast, top - big / 2, PosCurrent - PosMarkLast, big),
                                                 m_ProgressBarColorBarCurFg);
                // Draw last mark top
                ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMarkLast - (mbig / 2), 0, mbig, m_MarginItem / 2),
                                                 m_ProgressBarColorMark);
            }
        }
        // Mark bottom
        if (IsCurrent)
            ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMark - (mbig / 2), m_ProgressBarHeight - sml, mbig, sml),
                                             m_ProgressBarColorMarkCurrent);
        else
            ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMark - (mbig / 2), m_ProgressBarHeight - sml, mbig, sml),
                                             m_ProgressBarColorMark);
    }

    if (PosCurrent == PosMarkLast && PosMarkLast != 0)
        ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMarkLast - sml, 0, sml * 2, m_ProgressBarHeight),
                                         m_ProgressBarColorMarkCurrent);
    else if (PosMarkLast != 0)
        ProgressBarMarkerPixmap->DrawRectangle(cRect(PosMarkLast - (sml / 2), 0, sml, m_ProgressBarHeight),
                                         m_ProgressBarColorMark);
}

#if APIVERSNUM >= 30004
void cFlatBaseRender::ProgressBarDrawError(int Pos, int SmallLine, bool IsCurrent) {
    // if (!ProgressBarPixmap) return;  // Checked in calling function 'ProgressBarDrawMarks()'

    const tColor ColorError {Theme.Color(clrReplayErrorMark)};
    const int Middle {m_ProgressBarHeight / 2};
    if (IsCurrent) {  //* Draw current position marker in color of error mark
        const int Big {m_ProgressBarHeight - (SmallLine * 2) - 2};
        ProgressBarMarkerPixmap->DrawRectangle(cRect(Pos - (Big / 2), Middle - (Big / 2), Big, Big), ColorError);
    } else {
        static constexpr int kMarkerWidth {1}, kMarkerWidth3 {3};
        const int Type {Config.PlaybackShowErrorMarks};
        switch (Type) {  // Types: '|' (1, 2), 'I' (3, 4) and '+' (5, 6) small/big
        case 1:
        case 2: {
            const int Top = Middle - (SmallLine * (Type == 1 ? 0.75 : 0.5));
            ProgressBarPixmap->DrawRectangle(cRect(Pos, Top, kMarkerWidth, SmallLine * (Type == 1 ? 1.5 : 1)),
                                             ColorError);
            }
            break;
        case 3:
        case 4: {
            const int Top = Middle - (SmallLine * (Type == 3 ? 0.75 : 0.5));
            ProgressBarPixmap->DrawRectangle(cRect(Pos, Top, kMarkerWidth, SmallLine * (Type == 3 ? 1.5 : 1)),
                                             ColorError);
            ProgressBarPixmap->DrawRectangle(cRect(Pos - 1, Top, kMarkerWidth3, 1), ColorError);
            ProgressBarPixmap->DrawRectangle(
                cRect(Pos - 1, Middle + (SmallLine * (Type == 3 ? 0.75 : 0.5)), kMarkerWidth3, 1), ColorError);
            }
            break;
        case 5:
        case 6: {
            const int Top = Middle - (SmallLine * (Type == 5 ? 0.75 : 0.5));
            ProgressBarPixmap->DrawRectangle(cRect(Pos, Top, kMarkerWidth, SmallLine * (Type == 5 ? 1.5 : 1)),
                                             ColorError);
            ProgressBarPixmap->DrawRectangle(cRect(Pos - 1, Middle - 1, kMarkerWidth3, 2), ColorError);
            }
            break;
        default: esyslog("flatPlus: cFlatBaseRender::ProgressBarDrawError() Type %d not implemented.", Type); break;
        }
    }
}
#endif

void cFlatBaseRender::ScrollbarDraw(cPixmap *Pixmap, int Left, int Top, int Height, int Total, int Offset, int Shown,
                                    bool CanScrollUp, bool CanScrollDown) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatBaseRender::ScrollBarDraw() Total %d, Shown %d, Offset %d", Total, Shown, Offset);
#endif

    if (!Pixmap) return;

    if (Total > 0 && Total > Shown) {
        const int ScrollHeight {std::max(static_cast<int>(static_cast<double>(Height) * Shown / Total + 0.5), 5)};
        const int ScrollTop{std::min(static_cast<int>(static_cast<double>(Top) + Height * Offset / Total + 0.5),
                                     Top + Height - ScrollHeight)};

        // PixmapClear(Pixmap);
        static int lastTotal = -1, lastShown = -1, lastOffset = -1;
        if (Total != lastTotal || Shown != lastShown || Offset != lastOffset) {
            // dsyslog("flatPlus: cFlatBaseRender::ScrollBarDraw() Clear scrollbar pixmap");
            PixmapClear(Pixmap);
            lastTotal = Total;
            lastShown = Shown;
            lastOffset = Offset;
        }

        Pixmap->DrawRectangle(cRect(Left, Top, m_ScrollBarWidth, Height), Config.decorScrollBarBg);

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
        const int Type {Config.decorScrollBarType};
        switch (Type) {
        default:
        case 0: {
            const int LineWidth {(m_ScrollBarWidth <= 10) ? 2 : (m_ScrollBarWidth <= 20) ? 4 : 6};
            Pixmap->DrawRectangle(cRect(Left, Top, LineWidth, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(cRect(Left + LineWidth, ScrollTop, m_ScrollBarWidth - LineWidth, ScrollHeight),
                                  Config.decorScrollBarBarFg);
            break;
        }
        case 1: {
            const int DotHeight {m_ScrollBarWidth / 2};
            const int LineWidth {(m_ScrollBarWidth <= 10) ? 2 : (m_ScrollBarWidth <= 20) ? 4 : 6};
            Pixmap->DrawRectangle(cRect(Left, Top, LineWidth, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(cRect(Left + LineWidth, ScrollTop + DotHeight, m_ScrollBarWidth - LineWidth,
                                        ScrollHeight - DotHeight * 2),
                                  Config.decorScrollBarBarFg);
            // Dot
            Pixmap->DrawEllipse(
                cRect(Left + LineWidth, ScrollTop, m_ScrollBarWidth - LineWidth, m_ScrollBarWidth - LineWidth),
                Config.decorScrollBarBarFg, 0);
            Pixmap->DrawEllipse(cRect(Left + LineWidth, ScrollTop + ScrollHeight - DotHeight * 2,
                                      m_ScrollBarWidth - LineWidth, m_ScrollBarWidth - LineWidth),
                                Config.decorScrollBarBarFg, 0);
            break;
        }
        case 2: {
            const int Middle {Left + m_ScrollBarWidth / 2};
            const int LineWidth {(m_ScrollBarWidth <= 10) ? 2 : (m_ScrollBarWidth <= 20) ? 4 : 6};
            Pixmap->DrawRectangle(cRect(Middle - LineWidth / 2, Top, LineWidth, Height), Config.decorScrollBarFg);
            // Bar
            Pixmap->DrawRectangle(cRect(Left, ScrollTop, m_ScrollBarWidth, ScrollHeight), Config.decorScrollBarBarFg);
            break;
        }
        case 3: {
            const int DotHeight {m_ScrollBarWidth / 2};
            const int Middle {Left + DotHeight};
            const int LineWidth {(m_ScrollBarWidth <= 10) ? 2 : (m_ScrollBarWidth <= 20) ? 4 : 6};
            Pixmap->DrawRectangle(cRect(Middle - LineWidth / 2, Top, LineWidth, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(cRect(Left, ScrollTop + DotHeight, m_ScrollBarWidth, ScrollHeight - DotHeight * 2),
                                  Config.decorScrollBarBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(Left, ScrollTop, m_ScrollBarWidth, m_ScrollBarWidth), Config.decorScrollBarBarFg,
                                0);
            Pixmap->DrawEllipse(
                cRect(Left, ScrollTop + ScrollHeight - DotHeight * 2, m_ScrollBarWidth, m_ScrollBarWidth),
                Config.decorScrollBarBarFg, 0);
            break;
        }
        case 4: {
            const int out {(m_ScrollBarWidth > 10) ? 2 : 1};
            // Outline
            Pixmap->DrawRectangle(cRect(Left, Top, m_ScrollBarWidth, out), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left, Top + Height - out, m_ScrollBarWidth, out), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left, Top, out, Height), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left + m_ScrollBarWidth - out, Top, out, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(
                cRect(Left + out, ScrollTop + out, m_ScrollBarWidth - out * 2, ScrollHeight - out * 2),
                Config.decorScrollBarBarFg);
            break;
        }
        case 5: {
            const int DotHeight {m_ScrollBarWidth / 2};
            const int out {(m_ScrollBarWidth > 10) ? 2 : 1};
            const int out2 {out * 2};
            // Outline
            Pixmap->DrawRectangle(cRect(Left, Top, m_ScrollBarWidth, out), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left, Top + Height - out, m_ScrollBarWidth, out), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left, Top, out, Height), Config.decorScrollBarFg);
            Pixmap->DrawRectangle(cRect(Left + m_ScrollBarWidth - out, Top, out, Height), Config.decorScrollBarFg);

            // Bar
            Pixmap->DrawRectangle(cRect(Left + out, ScrollTop + DotHeight + out, m_ScrollBarWidth - out2,
                                        ScrollHeight - DotHeight * 2 - out2),
                                  Config.decorScrollBarBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(Left + out, ScrollTop + out, m_ScrollBarWidth - out2, m_ScrollBarWidth - out2),
                                Config.decorScrollBarBarFg, 0);
            Pixmap->DrawEllipse(cRect(Left + out, ScrollTop + ScrollHeight - DotHeight * 2 + out,
                                      m_ScrollBarWidth - out2, m_ScrollBarWidth - out2),
                                Config.decorScrollBarBarFg, 0);
            break;
        }
        case 6: {
            Pixmap->DrawRectangle(cRect(Left, ScrollTop, m_ScrollBarWidth, ScrollHeight), Config.decorScrollBarBarFg);
            break;
        }
        case 7: {
            const int DotHeight {m_ScrollBarWidth / 2};

            Pixmap->DrawRectangle(cRect(Left, ScrollTop + DotHeight, m_ScrollBarWidth, ScrollHeight - DotHeight * 2),
                                  Config.decorScrollBarBarFg);
            // Dot
            Pixmap->DrawEllipse(cRect(Left, ScrollTop, m_ScrollBarWidth, m_ScrollBarWidth), Config.decorScrollBarBarFg,
                                0);
            Pixmap->DrawEllipse(
                cRect(Left, ScrollTop + ScrollHeight - DotHeight * 2, m_ScrollBarWidth, m_ScrollBarWidth),
                Config.decorScrollBarBarFg, 0);

            break;
        }
        }  // switch Type
    }  // Total > 0
}

int cFlatBaseRender::ScrollBarWidth() const {
    return m_ScrollBarWidth;
}

void cFlatBaseRender::DecorBorderClear(const cRect &Rect, int Size) {
    const int Size2 {Size * 2};
    const int TopDecor {Rect.Top() - Size};
    const int LeftDecor {Rect.Left() - Size};
    const int WidthDecor {Rect.Width() + Size2};
    const int HeightDecor {Rect.Height() + Size2};
    const int BottomDecor {Rect.Height() + Size};

    if (DecorPixmap) {
        // Top
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor, WidthDecor, Size), clrTransparent);
        // Right
        DecorPixmap->DrawRectangle(cRect(LeftDecor + Size + Rect.Width(), TopDecor, Size, HeightDecor), clrTransparent);
        // Bottom
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor + BottomDecor, WidthDecor, Size), clrTransparent);
        // Left
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor, Size, HeightDecor), clrTransparent);
    }
}

void cFlatBaseRender::DecorBorderClearByFrom(int From) {
    // The erase-remove idiom is a standard C++ pattern for removing elements from containers. It uses std::remove_if to
    // move elements to be removed to the end of the container, then uses erase to remove them all at once.
    auto removeStart = std::remove_if(Borders.begin(), Borders.end(), [this, From](const sDecorBorder &border) {
        if (border.From == From) {
            DecorBorderClear(cRect(border.Left, border.Top, border.Width, border.Height), border.Size);
            return true;
        }
        return false;
    });

    Borders.erase(removeStart, Borders.end());
}

void cFlatBaseRender::DecorBorderRedrawAll() {
    for (const auto &ib : Borders) {
        DecorBorderDraw(ib, false);
    }
}

void cFlatBaseRender::DecorBorderClearAll() const {
    PixmapClear(DecorPixmap);
}

void cFlatBaseRender::DecorBorderDraw(const sDecorBorder &ib, bool Store) {
    if (ib.Size == 0 || ib.Type <= 0) return;
    if (ib.Width == 0 || ib.Height == 0) return;  // Avoid division by zero

    if (Store)
        Borders.emplace_back(ib);

    const int Size2 {ib.Size * 2};
    const int LeftDecor {ib.Left - ib.Size};
    const int TopDecor {ib.Top - ib.Size};
    const int WidthDecor {ib.Width + Size2};
    const int HeightDecor {ib.Height + Size2};
    const int BottomDecor {ib.Height + ib.Size};

    if (!DecorPixmap) {
        DecorPixmap = CreatePixmap(m_Osd, "DecorPixmap", 4, cRect(0, 0, cOsd::OsdWidth(), cOsd::OsdHeight()));
        if (!DecorPixmap) return;

        PixmapClear(DecorPixmap);
    }

    switch (ib.Type) {
    case 1:  // Rect
        // Top
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor, WidthDecor, ib.Size), ib.ColorBg);
        // Right
        DecorPixmap->DrawRectangle(cRect(LeftDecor + ib.Size + ib.Width, TopDecor, ib.Size, HeightDecor), ib.ColorBg);
        // Bottom
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor + BottomDecor, WidthDecor, ib.Size), ib.ColorBg);
        // Left
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor, ib.Size, HeightDecor), ib.ColorBg);
        break;
    case 2:  // Round
        // Top
        DecorPixmap->DrawRectangle(cRect(LeftDecor + ib.Size, TopDecor, ib.Width, ib.Size), ib.ColorBg);
        // Right
        DecorPixmap->DrawRectangle(cRect(LeftDecor + ib.Size + ib.Width, TopDecor + ib.Size, ib.Size, ib.Height),
                                   ib.ColorBg);
        // Bottom
        DecorPixmap->DrawRectangle(cRect(LeftDecor + ib.Size, TopDecor + BottomDecor, ib.Width, ib.Size), ib.ColorBg);
        // Left
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor + ib.Size, ib.Size, ib.Height), ib.ColorBg);

        // Top,left corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor, TopDecor, ib.Size, ib.Size), ib.ColorBg, 2);
        // Top,right corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor + ib.Size + ib.Width, TopDecor, ib.Size, ib.Size), ib.ColorBg, 1);
        // Bottom,left corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor, TopDecor + BottomDecor, ib.Size, ib.Size), ib.ColorBg, 3);
        // Bottom,right corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor + ib.Size + ib.Width, TopDecor + BottomDecor, ib.Size, ib.Size),
                                 ib.ColorBg, 4);
        break;
    case 3:  // Invert round
        // Top
        DecorPixmap->DrawRectangle(cRect(LeftDecor + ib.Size, TopDecor, ib.Width, ib.Size), ib.ColorBg);
        // Right
        DecorPixmap->DrawRectangle(cRect(LeftDecor + ib.Size + ib.Width, TopDecor + ib.Size, ib.Size, ib.Height),
                                   ib.ColorBg);
        // Bottom
        DecorPixmap->DrawRectangle(cRect(LeftDecor + ib.Size, TopDecor + BottomDecor, ib.Width, ib.Size), ib.ColorBg);
        // Left
        DecorPixmap->DrawRectangle(cRect(LeftDecor, TopDecor + ib.Size, ib.Size, ib.Width), ib.ColorBg);

        // Top,left corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor, TopDecor, ib.Size, ib.Size), ib.ColorBg, -4);
        // Top,right corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor + ib.Size + ib.Width, TopDecor, ib.Size, ib.Size), ib.ColorBg, -3);
        // Bottom,left corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor, TopDecor + BottomDecor, ib.Size, ib.Size), ib.ColorBg, -1);
        // Bottom,right corner
        DecorPixmap->DrawEllipse(cRect(LeftDecor + ib.Size + ib.Width, TopDecor + BottomDecor, ib.Size, ib.Size),
                                 ib.ColorBg, -2);
        break;
    case 4:  // Rect + alpha blend
        // Top
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + ib.Size, TopDecor, WidthDecor - Size2, ib.Size, ib.ColorBg);
        // Bottom
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + ib.Size, TopDecor + BottomDecor, WidthDecor - Size2, -1 * ib.Size,
                             ib.ColorBg);
        // Left
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor, TopDecor + ib.Size, ib.Size, HeightDecor - Size2, ib.ColorBg);
        // Right
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor + ib.Size + ib.Width, TopDecor + ib.Size, -1 * ib.Size,
                             HeightDecor - Size2, ib.ColorBg);

        DecorDrawGlowRectTL(DecorPixmap, LeftDecor, TopDecor, ib.Size, ib.Size, ib.ColorBg);
        DecorDrawGlowRectTR(DecorPixmap, LeftDecor + ib.Size + ib.Width, TopDecor, ib.Size, ib.Size, ib.ColorBg);
        DecorDrawGlowRectBL(DecorPixmap, LeftDecor, TopDecor + ib.Size + ib.Height, ib.Size, ib.Size, ib.ColorBg);
        DecorDrawGlowRectBR(DecorPixmap, LeftDecor + ib.Size + ib.Width, TopDecor + ib.Size + ib.Height, ib.Size,
                            ib.Size, ib.ColorBg);
        break;
    case 5:  // Round + alpha blend
        // Top
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + ib.Size, TopDecor, WidthDecor - Size2, ib.Size, ib.ColorBg);
        // Bottom
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + ib.Size, TopDecor + BottomDecor, WidthDecor - Size2, -1 * ib.Size,
                             ib.ColorBg);
        // Left
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor, TopDecor + ib.Size, ib.Size, HeightDecor - Size2, ib.ColorBg);
        // Right
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor + ib.Size + ib.Width, TopDecor + ib.Size, -1 * ib.Size,
                             HeightDecor - Size2, ib.ColorBg);

        DecorDrawGlowEllipseTL(DecorPixmap, LeftDecor, TopDecor, ib.Size, ib.Size, ib.ColorBg, 2);
        DecorDrawGlowEllipseTR(DecorPixmap, LeftDecor + ib.Size + ib.Width, TopDecor, ib.Size, ib.Size, ib.ColorBg, 1);
        DecorDrawGlowEllipseBL(DecorPixmap, LeftDecor, TopDecor + ib.Size + ib.Height, ib.Size, ib.Size, ib.ColorBg, 3);
        DecorDrawGlowEllipseBR(DecorPixmap, LeftDecor + ib.Size + ib.Width, TopDecor + ib.Size + ib.Height, ib.Size,
                               ib.Size, ib.ColorBg, 4);
        break;
    case 6:  // Invert round + alpha blend
        // Top
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + ib.Size, TopDecor, WidthDecor - Size2, ib.Size, ib.ColorBg);
        // Bottom
        DecorDrawGlowRectHor(DecorPixmap, LeftDecor + ib.Size, TopDecor + BottomDecor, WidthDecor - Size2, -1 * ib.Size,
                             ib.ColorBg);
        // Left
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor, TopDecor + ib.Size, ib.Size, HeightDecor - Size2, ib.ColorBg);
        // Right
        DecorDrawGlowRectVer(DecorPixmap, LeftDecor + ib.Size + ib.Width, TopDecor + ib.Size, -1 * ib.Size,
                             HeightDecor - Size2, ib.ColorBg);

        DecorDrawGlowEllipseTL(DecorPixmap, LeftDecor, TopDecor, ib.Size, ib.Size, ib.ColorBg, -4);
        DecorDrawGlowEllipseTR(DecorPixmap, LeftDecor + ib.Size + ib.Width, TopDecor, ib.Size, ib.Size, ib.ColorBg, -3);
        DecorDrawGlowEllipseBL(DecorPixmap, LeftDecor, TopDecor + ib.Size + ib.Height, ib.Size, ib.Size, ib.ColorBg,
                               -1);
        DecorDrawGlowEllipseBR(DecorPixmap, LeftDecor + ib.Size + ib.Width, TopDecor + ib.Size + ib.Height, ib.Size,
                               ib.Size, ib.ColorBg, -2);
        break;
    }
}

/*
tColor cFlatBaseRender::Multiply(tColor Color, uint8_t Alpha) {
  tColor RB = (Color & 0x00FF00FF) * Alpha;
  RB = ((RB + ((RB >> 8) & 0x00FF00FF) + 0x00800080) >> 8) & 0x00FF00FF;
  tColor AG = ((Color >> 8) & 0x00FF00FF) * Alpha;
  AG = ((AG + ((AG >> 8) & 0x00FF00FF) + 0x00800080)) & 0xFF00FF00;
  return AG | RB;
} */

tColor cFlatBaseRender::SetAlpha(tColor Color, double am) {
    uint8_t A = (Color & 0xFF000000) >> 24;
    uint8_t R = (Color & 0x00FF0000) >> 16;
    uint8_t G = (Color & 0x0000FF00) >> 8;
    uint8_t B = (Color & 0x000000FF);

    A = static_cast<uint8_t>(A * am);  // Explicitly cast the result to uint8_t
    return ArgbToColor(A, R, G, B);
}

void cFlatBaseRender::DecorDrawGlowRectHor(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    const int start {(Height < 0) ? Height * -1 : 0};
    const int end {(Height < 0) ? 0 : Height};
    const int step {(Height < 0) ? -1 : 1};
    const double AlphaStep {1.0 / std::abs(Height)};  // Normalized step (0.0-1.0)
    tColor col {0};  // Init outside of loop

    for (int i {start}, j {0}; (Height < 0) ? i >= end : i < end; i += step, ++j) {
        col = SetAlpha(ColorBg, AlphaStep * j);
        pixmap->DrawRectangle(cRect(Left, Top + i, Width, 1), col);
    }
}

void cFlatBaseRender::DecorDrawGlowRectVer(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    const int absWidth {std::abs(Width)};
    const int start {(Width < 0) ? absWidth : 0};
    const int end {(Width < 0) ? 0 : absWidth};
    const int step {(Width < 0) ? -1 : 1};
    const double AlphaStep {1.0 / absWidth};  // Normalized step (0.0-1.0)
    tColor col {0};  // Init outside of loop

    for (int i {start}, j {0}; (Width < 0) ? i >= end : i < end; i += step, ++j) {
        col = SetAlpha(ColorBg, AlphaStep * j);
        pixmap->DrawRectangle(cRect(Left + i, Top, 1, Height), col);
    }
}

void cFlatBaseRender::DecorDrawGlowRectTL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    /* for (int i {0}; i < Width; ++i) {
        Alpha = 255.0 / Width * i;
        col = SetAlpha(ColorBg, 100.0 * (1.0 / 255.0) * Alpha * (1.0 / 100.0));
        pixmap->DrawRectangle(cRect(Left + i, Top + i, Width - i, Height - i), col);
    } */

    const int absWidth {std::abs(Width)};
    const int absHeight {std::abs(Height)};
    const int start {0};
    const int end {absWidth};
    const int step {1};
    const double AlphaStep {1.0 / absWidth};  // Normalized step (0.0-1.0)
    tColor col {0};

    for (int i {start}, j {0}; i < end; i += step, ++j) {
        col = SetAlpha(ColorBg, AlphaStep * j);
        pixmap->DrawRectangle(cRect(Left + i, Top + i, absWidth - i, absHeight - i), col);
    }
}

void cFlatBaseRender::DecorDrawGlowRectTR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    const double AlphaStep {1.0 / Width};  // Normalized step (0.0-1.0)
    tColor col {0};  // Init outside of loop

    for (int i {0}, j = Width; i < Width; ++i, --j) {
        col = SetAlpha(ColorBg, AlphaStep * i);
        pixmap->DrawRectangle(cRect(Left, Top + Height - j, j, j), col);
    }
}

void cFlatBaseRender::DecorDrawGlowRectBL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    const double AlphaStep {1.0 / Width};  // Normalized step (0.0-1.0)
    tColor col {0};  // Init outside of loop

    for (int i {0}, j = Width; i < Width; ++i, --j) {
        col = SetAlpha(ColorBg, AlphaStep * i);
        pixmap->DrawRectangle(cRect(Left + Width - j, Top, j, j), col);
    }
}

void cFlatBaseRender::DecorDrawGlowRectBR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    const double AlphaStep {1.0 / Width};  // Normalized step (0.0-1.0)
    tColor col {0};  // Init outside of loop

    for (int i {0}, j = Width; i < Width; ++i, --j) {
        col = SetAlpha(ColorBg, AlphaStep * i);
        pixmap->DrawRectangle(cRect(Left, Top, j, j), col);
    }
}

void cFlatBaseRender::DecorDrawGlowEllipseTL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                             int type) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    const double AlphaStep {1.0 / Width};  // Normalized step (0.0-1.0)
    tColor col {0};  // Init outside of loop

    for (int i {0}, j = Width; i < Width; ++i, --j) {
        col = SetAlpha(ColorBg, AlphaStep * i);
        pixmap->DrawEllipse(cRect(Left + i, Top + i, j, j), col, type);
    }
}

void cFlatBaseRender::DecorDrawGlowEllipseTR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                             int type) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    const double AlphaStep {1.0 / Width};  // Normalized step (0.0-1.0)
    tColor col {0};  // Init outside of loop

    for (int i {0}, j = Width; i < Width; ++i, --j) {
        col = SetAlpha(ColorBg, AlphaStep * i);
        pixmap->DrawEllipse(cRect(Left, Top + Height - j, j, j), col, type);
    }
}

void cFlatBaseRender::DecorDrawGlowEllipseBL(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                             int type) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    const double AlphaStep {1.0 / Width};  // Normalized step (0.0-1.0)
    tColor col {0};  // Init outside of loop

    for (int i {0}, j = Width; i < Width; ++i, --j) {
        col = SetAlpha(ColorBg, AlphaStep * i);
        pixmap->DrawEllipse(cRect(Left + Width - j, Top, j, j), col, type);
    }
}

void cFlatBaseRender::DecorDrawGlowEllipseBR(cPixmap *pixmap, int Left, int Top, int Width, int Height, tColor ColorBg,
                                             int type) {
    // if (!pixmap) return;  // Checked in DecorBorderDraw()

    const double AlphaStep {1.0 / Width};  // Normalized step (0.0-1.0)
    tColor col {0};  // Init outside of loop

    for (int i {0}, j = Width; i < Width; ++i, --j) {
        col = SetAlpha(ColorBg, AlphaStep * i);
        pixmap->DrawEllipse(cRect(Left, Top, j, j), col, type);
    }
}

int cFlatBaseRender::GetFontAscender(const char *Name, int CharHeight, int CharWidth) {
    GlyphMetricsCache &cache = glyphMetricsCache();
    FT_Face face {cache.GetFace(*cFont::GetFontFileName(Name))};
    if (!face) {
        esyslog("flatPlus: GetFontAscender() error: can't find face (font = %s)", *cFont::GetFontFileName(Name));
        return CharHeight;
    }

    int Ascender {CharHeight};
    if (face->num_fixed_sizes && face->available_sizes) {  // Fixed size
        Ascender = face->available_sizes->height;
    } else if (FT_Set_Char_Size(face, CharWidth * 64, CharHeight * 64, 0, 0) == 0) {
        Ascender = face->size->metrics.ascender / 64;
    } else {
        esyslog("flatPlus: GetFontAscender() FreeType error during FT_Set_Char_Size (font = %s)",
                *cFont::GetFontFileName(Name));
    }

    return Ascender;
}
cString cFlatBaseRender::ReadAndExtractData(const cString &FilePath) const {
    if (isempty(*FilePath)) return "";

    // std::ifstream file(*FilePath);
    // if (!file.is_open()) return "";
    FILE *fp = fopen(*FilePath, "r");
    if (fp == nullptr) return "";   // File doesn't exist

    // Read the first line
    cReadLine ReadLine;
    char *s = ReadLine.Read(fp);  // ReadLine will read from the file pointer
    if (fp != nullptr) {
        fclose(fp);
    }

    /* std::string data {""};
    data.reserve(64);  // Reserve space for the string to avoid multiple reallocations
    std::getline(file, data); */

    return s == nullptr ? "" : cString(s);
}

// --- Disk read batching and stat cache for weather widget and main menu widget
bool cFlatBaseRender::BatchReadWeatherData(FontImageWeatherCache &out, time_t &out_latest_time) {  // NOLINT
    constexpr int kMaxDays {8};
    const std::string prefix = std::string(WIDGETOUTPUTPATH) + "/weather/";
    time_t latest {0};

    // Moved outside the loop to avoid re-creation in each iteration
    std::string temp, icon, tempMax, tempMin, precipitation, location, summary;
    std::string DayString, tempFile, iconFile, tempMaxFile, tempMinFile, precFile, summaryFile, locationFile;

    // Reuse the istringstream object to avoid repeated construction/destruction in the loop.
    std::istringstream istr;
    istr.imbue(std::locale("C"));

    // Read all files and cache data
    for (uint day {0}; day < kMaxDays; ++day) {
        DayString = std::to_string(day);  // Convert day to string
        iconFile = prefix + (day == 0 ? "weather.0.icon-act" : "weather." + DayString + ".icon");
        tempMaxFile = prefix + "weather." + DayString + ".tempMax";
        tempMinFile = prefix + "weather." + DayString + ".tempMin";
        precFile = prefix + "weather." + DayString + ".precipitation";
        summaryFile = prefix + "weather." + DayString + ".summary";

        if (day == 0) {  // Only for day 0, read temp and location file
            tempFile = prefix + "weather.0.temp";
            locationFile = prefix + "weather.location";

            // Check file mtime for 'Temp' only as all files are written together
            struct stat s;
            if (stat(tempFile.c_str(), &s) == 0) latest = s.st_mtime;
            // Check if data is already cached
            if (latest == out.LastReadMTime) {
                break;  // If latest mtime matches cached, skip reading files
            }

            std::ifstream ftemp(tempFile), ficon(iconFile), fmax(tempMaxFile), fmin(tempMinFile),
                fprec(precFile), fsum(summaryFile), floc(locationFile);

            if (!ftemp.is_open() || !floc.is_open() || !ficon.is_open() || !fmax.is_open() || !fmin.is_open() ||
                !fprec.is_open() || !fsum.is_open()) {
                dsyslog("flatPlus: BatchReadWeatherData() File not found for day 0");
                return false;  // Break if any file is missing
            }

            getline(ftemp, temp);    out.Days[day].Temp = temp;
            getline(ficon, icon);    out.Days[day].Icon = icon.c_str();
            getline(fmax, tempMax);  out.Days[day].TempMax = tempMax.c_str();
            getline(fmin, tempMin);  out.Days[day].TempMin = tempMin.c_str();
            getline(fprec, precipitation);
            istr.str(precipitation); istr.clear();  // Clear the error state of the stream
            double p {}; istr >> p;
            out.Days[day].Precipitation = cString::sprintf("%d%%", RoundUp(p * 100.0, 10));
            getline(floc, location); out.Location = location.c_str();
            getline(fsum, summary);  out.Days[day].Summary = summary.c_str();

            // Check if 'Temp' is valid
            if (temp.empty()) {
                dsyslog("flatPlus: BatchReadWeatherData() 'Temp' for day 0 is empty");
                return false;
            }

            // Temp sign extraction
            auto &tt = out.Days[day].Temp;
            size_t deg = tt.find("°");
            if (deg != std::string::npos) {
                out.TempTodaySign = tt.substr(deg).c_str();  // Get the sign (°C or °F)
                tt = tt.substr(0, deg);                      // Get the temperature without the sign
            } else {
                out.TempTodaySign = "";
            }
        } else {
            // For days 1-7, only read icon, tempMax, tempMin, precipitation, and summary
            std::ifstream ficon(iconFile), fmax(tempMaxFile), fmin(tempMinFile), fprec(precFile),
                fsum(summaryFile);

            if (!ficon.is_open() || !fmax.is_open() || !fmin.is_open() || !fprec.is_open() || !fsum.is_open())
                break;  // Break if any file is missing (No more day to expect)

            out.Days[day].Temp = "";  // No temp file for days 1-7
            getline(ficon, icon);   out.Days[day].Icon = icon.c_str();
            getline(fmax, tempMax); out.Days[day].TempMax = tempMax.c_str();
            getline(fmin, tempMin); out.Days[day].TempMin = tempMin.c_str();
            getline(fprec, precipitation);
            istr.str(precipitation); istr.clear();  // Clear the stream state before reusing
            double p {}; istr >> p;
            out.Days[day].Precipitation = cString::sprintf("%d%%", RoundUp(p * 100.0, 10));
            getline(fsum, summary); out.Days[day].Summary = summary.c_str();
        }
    }

    out.LastReadMTime = latest;
    out_latest_time = latest;
    return true;
}

// Font caching for WeatherWidget: reuse across draws/sizes
static void EnsureWeatherWidgetFonts(FontImageWeatherCache &cache, int fs) {  // NOLINT
    // Only update fonts if missing or font size changed
    if (cache.WeatherFont && cache.FontHeight == (FontCache.GetFont(Setup.FontOsd, fs)->Height())) return;
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: EnsureWeatherWidgetFonts() Updating fonts for WeatherWidget");
#endif

    cache.WeatherFont      = FontCache.GetFont(Setup.FontOsd, fs);
    cache.WeatherFontSml   = FontCache.GetFont(Setup.FontOsd, fs / 2);
    cache.WeatherFontSign  = FontCache.GetFont(Setup.FontOsd, fs / 2.5);

    cache.FontAscender     = cFlatBaseRender::GetFontAscender(Setup.FontOsd, fs);
    cache.FontSignAscender = cFlatBaseRender::GetFontAscender(Setup.FontOsd, fs / 2.5);

    cache.FontHeight       = cache.WeatherFont->Height();
    cache.FontSmlHeight    = cache.WeatherFontSml->Height();
    cache.FontSignHeight   = cache.WeatherFontSign->Height();

    cache.TempTodayWidth     = cache.WeatherFont->Width(cache.Days[0].Temp.c_str());
    cache.TempTodaySignWidth = cache.WeatherFontSign->Width(cache.TempTodaySign);
    cache.PrecTodayWidth     = cache.WeatherFontSml->Width(cache.Days[0].Precipitation);
    cache.PrecTomorrowWidth  = cache.WeatherFontSml->Width(cache.Days[1].Precipitation);
    cache.WidthTempToday     = std::max(cache.WeatherFontSml->Width(cache.Days[0].TempMax),
                                        cache.WeatherFontSml->Width(cache.Days[0].TempMin));
    cache.WidthTempTomorrow  = std::max(cache.WeatherFontSml->Width(cache.Days[1].TempMax),
                                        cache.WeatherFontSml->Width(cache.Days[1].TempMin));
}

void cFlatBaseRender::DrawWidgetWeather() {  // Weather widget (repay/channel)
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatBaseRender::DrawWidgetWeather()");
#endif

    static int LastOsdHeight {0};
    const int fs = cOsd::OsdHeight() * Config.WeatherFontSize + 0.5;

    // Only reload/calc data/fonts if input files or screen size changed, else use prepared/cached
    time_t NewestFiletime {WeatherCache.LastReadMTime};  // Last read time from cache
    if ((LastOsdHeight != fs) || !WeatherCache.valid || !BatchReadWeatherData(WeatherCache, NewestFiletime) ||
        (WeatherCache.LastReadMTime != NewestFiletime)) {
#ifdef DEBUGFUNCSCALL
        dsyslog("flatPlus: cFlatBaseRender::DrawWidgetWeather() Need to refresh/calc");
#endif
        // Need to refresh/calc
        if (!BatchReadWeatherData(WeatherCache, NewestFiletime)) return;
#ifdef DEBUGFUNCSCALL
        dsyslog("flatPlus: cFlatBaseRender::DrawWidgetWeather() Data read");
#endif
        EnsureWeatherWidgetFonts(WeatherCache, fs);
        WeatherCache.LastReadMTime = NewestFiletime;
        LastOsdHeight = fs;
        WeatherCache.valid = true;
    }

    const auto &dat = WeatherCache;  // Weather data reference
    cFont* WeatherFont {WeatherCache.WeatherFont};
    cFont* WeatherFontSml {WeatherCache.WeatherFontSml};
    cFont* WeatherFontSign {WeatherCache.WeatherFontSign};

    int left {m_MarginItem};
    const int WeatherFontHeight {WeatherCache.FontHeight};
    const int WeatherFontSmlHeight {WeatherCache.FontSmlHeight};

    const int TempTodayWidth {WeatherCache.TempTodayWidth};
    const int TempTodaySignWidth {WeatherCache.TempTodaySignWidth};
    const int PrecTodayWidth {WeatherCache.PrecTodayWidth};
    const int PrecTomorrowWidth {WeatherCache.PrecTomorrowWidth};
    const int WidthTempToday {WeatherCache.WidthTempToday};        // Max width of temp today
    const int WidthTempTomorrow {WeatherCache.WidthTempTomorrow};  // Max width of temp tomorrow

    const int wTop {m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize};
    const int wWidth {m_MarginItem + TempTodayWidth + TempTodaySignWidth + m_MarginItem2 + WeatherFontHeight +
                      m_MarginItem + WidthTempToday + m_MarginItem + WeatherFontHeight - m_MarginItem2 +
                      PrecTodayWidth + m_MarginItem * 4 + WeatherFontHeight + m_MarginItem + WidthTempTomorrow +
                      m_MarginItem + WeatherFontHeight - m_MarginItem2 + PrecTomorrowWidth + m_MarginItem2};
    const int wLeft {m_OsdWidth - wWidth - 20};

    // Setup widget
    WeatherWidget.Clear();
    WeatherWidget.SetOsd(m_Osd);
    WeatherWidget.SetPosition(cRect(wLeft, wTop, wWidth, WeatherFontHeight));
    WeatherWidget.SetBGColor(Theme.Color(clrItemCurrentBg));
    WeatherWidget.SetScrollingActive(false);

    // Add temperature
    WeatherWidget.AddText(dat.Days[0].Temp.c_str(), false, cRect(left, 0, 0, 0), Theme.Color(clrChannelFontEpg),
                          Theme.Color(clrItemCurrentBg), WeatherFont);
    left += TempTodayWidth;

    const int t {(WeatherFontHeight - WeatherCache.FontAscender) -
                 (WeatherCache.FontSignHeight - WeatherCache.FontSignAscender)};

    // Add temperature sign
    WeatherWidget.AddText(dat.TempTodaySign, false, cRect(left, t, 0, 0), Theme.Color(clrChannelFontEpg),
                          Theme.Color(clrItemCurrentBg), WeatherFontSign);
    left += TempTodaySignWidth + m_MarginItem2;

    // Add weather icon
    cString WeatherIcon = cString::sprintf("widgets/%s", *dat.Days[0].Icon);
    cImage *img {ImgLoader.LoadIcon(*WeatherIcon, WeatherFontHeight, WeatherFontHeight - m_MarginItem2)};
    if (img) {
        WeatherWidget.AddImage(img, cRect(left, 0 + m_MarginItem, WeatherFontHeight, WeatherFontHeight));
        left += WeatherFontHeight + m_MarginItem;
    }

    // Add temperature min/max values
    WeatherWidget.AddText(dat.Days[0].TempMax, false, cRect(left, 0, 0, 0), Theme.Color(clrChannelFontEpg),
                          Theme.Color(clrItemCurrentBg), WeatherFontSml, WidthTempToday, WeatherFontSmlHeight,
                          taRight);
    WeatherWidget.AddText(dat.Days[0].TempMin, false, cRect(left, 0 + WeatherFontSmlHeight, 0, 0),
                          Theme.Color(clrChannelFontEpg), Theme.Color(clrItemCurrentBg), WeatherFontSml,
                          WidthTempToday, WeatherFontSmlHeight, taRight);
    left += WidthTempToday + m_MarginItem;

    // Add precipitation icon
    img = ImgLoader.LoadIcon("widgets/umbrella", WeatherFontHeight, WeatherFontHeight - m_MarginItem2);
    if (img) {
        WeatherWidget.AddImage(img, cRect(left, 0 + m_MarginItem, WeatherFontHeight, WeatherFontHeight));
        left += WeatherFontHeight - m_MarginItem2;
    }

    // Add precipitation
    WeatherWidget.AddText(dat.Days[0].Precipitation, false,
                          cRect(left, 0 + (WeatherFontHeight / 2 - WeatherFontSmlHeight / 2), 0, 0),
                          Theme.Color(clrChannelFontEpg), Theme.Color(clrItemCurrentBg), WeatherFontSml);
    left += PrecTodayWidth + m_MarginItem * 4;

    WeatherWidget.AddRect(cRect(left - m_MarginItem2, 0, wWidth - left + m_MarginItem2, WeatherFontHeight),
                          Theme.Color(clrChannelBg));

    // Add weather icon tomorrow
    WeatherIcon = cString::sprintf("widgets/%s", *dat.Days[1].Icon);
    img = ImgLoader.LoadIcon(*WeatherIcon, WeatherFontHeight, WeatherFontHeight - m_MarginItem2);
    if (img) {
        WeatherWidget.AddImage(img, cRect(left, 0 + m_MarginItem, WeatherFontHeight, WeatherFontHeight));
        left += WeatherFontHeight + m_MarginItem;
    }

    // Add temperature min/max values tomorrow
    WeatherWidget.AddText(dat.Days[1].TempMax, false, cRect(left, 0, 0, 0), Theme.Color(clrChannelFontEpg),
                          Theme.Color(clrChannelBg), WeatherFontSml, WidthTempTomorrow, WeatherFontSmlHeight,
                          taRight);
    WeatherWidget.AddText(dat.Days[1].TempMin, false, cRect(left, 0 + WeatherFontSmlHeight, 0, 0),
                          Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), WeatherFontSml,
                          WidthTempTomorrow, WeatherFontSmlHeight, taRight);
    left += WidthTempTomorrow + m_MarginItem;

    // Add precipitation icon
    img = ImgLoader.LoadIcon("widgets/umbrella", WeatherFontHeight, WeatherFontHeight - m_MarginItem2);
    if (img) {
        WeatherWidget.AddImage(img, cRect(left, 0 + m_MarginItem, WeatherFontHeight, WeatherFontHeight));
        left += WeatherFontHeight - m_MarginItem2;
    }

    // Add precipitation tomorrow
    WeatherWidget.AddText(dat.Days[1].Precipitation, false,
                          cRect(left, 0 + (WeatherFontHeight / 2 - WeatherFontSmlHeight / 2), 0, 0),
                          Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), WeatherFontSml);

    // left += PrecTomorrowWidth;
    // WeatherWidget.AddRect(cRect(left, 0, wWidth - left, m_FontHeight), clrTransparent);

    WeatherWidget.CreatePixmaps(false);
    WeatherWidget.Draw();
}

/**
 * Draws a given text with a shadow effect.
 *
 * @param pixmap The pixmap to draw onto.
 * @param pos The position to draw the text at.
 * @param text The text to draw.
 * @param TextColor The color of the text.
 * @param ShadowColor The color of the shadow.
 * @param font The font to use.
 * @param ShadowSize The size of the shadow (number of steps).
 * @param xOffset The x offset of the shadow (default 1).
 * @param yOffset The y offset of the shadow (default 1).
 *
 * The shadow is drawn by repeatedly drawing the text with increasing alpha values
 * at positions offset by (xOffset, yOffset) from the main text position.
 * The main text is then drawn on top with the given color.
 */
void cFlatBaseRender::DrawTextWithShadow(cPixmap *pixmap, const cPoint &pos, const char *text, tColor TextColor,
                                         tColor ShadowColor, const cFont *font, int ShadowSize, int xOffset,
                                         int yOffset) {
    double Alpha {0.0};
    const double AlphaStep {1.0 / ShadowSize};  // Normalized step (0.0-1.0)
    static constexpr double MaxAlpha {1.0};     // Maximum alpha value
    int ShadowX {0}, ShadowY {0};               // Shadow position variables
    const int BaseX {pos.X()};                  // Cache position for faster access
    const int BaseY {pos.Y()};
    tColor CurrentShadowColor {0};              // Current shadow color with alpha

    // Loop through the shadow from outer to inner size to create the shadow effect
    // Adjust the xOffset and yOffset for the shadow direction
    for (int i {ShadowSize}; i >= 1; --i) {
        Alpha = std::min(i * AlphaStep, MaxAlpha);  // Ensure it does not exceed 1.0
        CurrentShadowColor = SetAlpha(ShadowColor, Alpha);
        ShadowX = BaseX + (xOffset * i);
        ShadowY = BaseY + (yOffset * i);
#ifdef DEBUGFUNCSCALL
        dsyslog("flatPlus: DrawTextWithShadow() ShadowColor %08X, Alpha %f", CurrentShadowColor, Alpha);
#endif

        pixmap->DrawText(cPoint(ShadowX, ShadowY), text, CurrentShadowColor, clrTransparent, font);
    }

    // Draw the main text
    pixmap->DrawText(pos, text, TextColor, clrTransparent, font);
}
