/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displayreplay.h"
#include "./flat.h"
#include "./fontcache.h"

cFlatDisplayReplay::cFlatDisplayReplay(bool ModeOnly) : cThread("DisplayReplay") {
    m_LabelHeight = m_FontHeight + m_FontSmlHeight;
    if (Config.PlaybackShowEndTime > 0)  // 1 = End time, 2 = End time and cutted end time
        m_LabelHeight += m_FontHeight;   // Use second line for displaying end time of recording

    m_ModeOnly = ModeOnly;

    CreateFullOsd();
    TopBarCreate();
    MessageCreate();

    const int EffectiveOsdWidth {m_OsdWidth - Config.decorBorderReplaySize * 2};
    m_TVSRect.Set(
        m_MarginEPGImage + Config.decorBorderChannelEPGSize,
        m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_MarginEPGImage + Config.decorBorderChannelEPGSize,
        EffectiveOsdWidth - m_MarginEPGImage * 2,
        m_OsdHeight - m_TopBarHeight - m_LabelHeight - m_MarginEPGImage * 2 - Config.decorBorderChannelEPGSize * 2);
    ChanEpgImagesPixmap = CreatePixmap(m_Osd, "ChanEpgImagesPixmap", 2, m_TVSRect);

    const cRect LabelPixmapViewPort {Config.decorBorderReplaySize,
                                     m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize, EffectiveOsdWidth,
                                     m_LabelHeight};
    LabelPixmap = CreatePixmap(m_Osd, "LabelPixmap", 1, LabelPixmapViewPort);
    IconsPixmap = CreatePixmap(m_Osd, "IconsPixmap", 2, LabelPixmapViewPort);

    /* Make Config.decorProgressReplaySize even
    https://stackoverflow.com/questions/4739388/make-an-integer-even
    number = (number / 2) * 2; */
    const int ProgressBarHeight {((Config.decorProgressReplaySize + 1) / 2) * 2};  // Make it even (And round up)
    ProgressBarCreate(
        cRect(Config.decorBorderReplaySize,
              m_OsdHeight - m_LabelHeight - ProgressBarHeight - Config.decorBorderReplaySize - m_MarginItem,
              EffectiveOsdWidth, ProgressBarHeight),
        m_MarginItem, 0, Config.decorProgressReplayFg, Config.decorProgressReplayBarFg, Config.decorProgressReplayBg,
        Config.decorProgressReplayType);

    LabelJumpPixmap = CreatePixmap(m_Osd, "LabelJumpPixmap", 1,
                                   cRect(Config.decorBorderReplaySize,
                                         m_OsdHeight - m_LabelHeight - ProgressBarHeight * 2 - m_MarginItem3 -
                                             m_FontHeight - Config.decorBorderReplaySize * 2,
                                         EffectiveOsdWidth, m_FontHeight));

    DimmPixmap = CreatePixmap(m_Osd, "DimmPixmap", MAXPIXMAPLAYERS - 1, cRect(0, 0, m_OsdWidth, m_OsdHeight));

    PixmapClear(ChanEpgImagesPixmap);
    PixmapFill(LabelPixmap, Theme.Color(clrReplayBg));
    PixmapClear(LabelJumpPixmap);
    PixmapClear(IconsPixmap);
    PixmapClear(DimmPixmap);

    m_FontSecs = FontCache.GetFont(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);

    if (Config.PlaybackWeatherShow) DrawWidgetWeather();
}

cFlatDisplayReplay::~cFlatDisplayReplay() {
    m_Osd->DestroyPixmap(LabelPixmap);
    m_Osd->DestroyPixmap(LabelJumpPixmap);
    m_Osd->DestroyPixmap(IconsPixmap);
    m_Osd->DestroyPixmap(ChanEpgImagesPixmap);
    m_Osd->DestroyPixmap(DimmPixmap);
}

void cFlatDisplayReplay::SetRecording(const cRecording *Recording) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayReplay::SetRecording()");
#endif

    if (m_ModeOnly || !IconsPixmap || !LabelPixmap) return;

    if (!Recording || !Recording->Info()) return;
    const cRecordingInfo *RecInfo {Recording->Info()};
    m_Recording = Recording;

    PixmapClear(IconsPixmap);

    SetTitle(RecInfo->Title());

    // First line for current, total and cutted length, second for end time
    const int SmallTop {(Config.PlaybackShowEndTime > 0) ? m_FontHeight2 : m_FontHeight};  // Top for bottom line

    // Show if still recording
    int left {m_MarginItem};  // Position for recording symbol/short text/date
    cImage *img {nullptr};
    if ((Recording->IsInUse() & ruTimer) != 0) {  // The recording is currently written to by a timer
        img = ImgLoader.GetIcon("timerRecording", kIconMaxSize, m_FontSmlHeight);  // Small image
        if (img) {
            IconsPixmap->DrawImage(cPoint(left, SmallTop), *img);
            left += img->Width() + m_MarginItem;
        }
    }

    cString InfoText {""};
    if (isempty(RecInfo->ShortText())) {  // No short text. Show date and time instead
        InfoText = cString::sprintf("%s  %s", *ShortDateString(Recording->Start()), *TimeString(Recording->Start()));
    } else {
        if (Config.PlaybackShowRecordingDate)  // Date  Time - ShortText
            InfoText = cString::sprintf("%s  %s - %s", *ShortDateString(Recording->Start()),
                                        *TimeString(Recording->Start()), RecInfo->ShortText());
        else
            InfoText = RecInfo->ShortText();
    }

    const int InfoWidth {m_FontSml->Width(*InfoText)};  // Width of infotext
    // TODO: How to get width of aspect and format icons?
    //  Done: Substract 'left' in case of displayed recording icon
    //  Done: Substract 'm_FontSmlHeight' in case of recording error icon is displayed later
    //* Workaround: Substract width of aspect and format icons (ResolutionAspectDraw()) ???
    int MaxWidth {m_OsdWidth - left - Config.decorBorderReplaySize * 2};

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors) MaxWidth -= m_FontSmlHeight;  // Substract width of imgRecErr
#endif

    img = ImgLoader.GetIcon("1920x1080", kIconMaxSize, m_FontSmlHeight);
    if (img) MaxWidth -= img->Width() * 3;  //* Substract guessed max. used space of aspect and format icons

    if (InfoWidth > MaxWidth) {  // Infotext too long
        if (Config.ScrollerEnable) {
            MessageScroller.AddScroller(*InfoText,
                                        cRect(Config.decorBorderReplaySize + left,
                                              m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize + SmallTop,
                                              MaxWidth, m_FontSmlHeight),
                                        Theme.Color(clrMenuEventFontInfo), clrTransparent, m_FontSml);
            left += MaxWidth;
        } else {  // Add ... if info ist too long
            dsyslog("flatPlus: Short text too long! (%d) Setting MaxWidth to %d", InfoWidth, MaxWidth);
            const int DotsWidth {FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "...")};
            LabelPixmap->DrawText(cPoint(left, SmallTop), *InfoText, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_FontSml, MaxWidth - DotsWidth);
            left += (MaxWidth - DotsWidth);
            LabelPixmap->DrawText(cPoint(left, SmallTop), "...", Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  m_FontSml, DotsWidth);
            left += DotsWidth;
        }
    } else {  // Short text fits into maxwidth
        LabelPixmap->DrawText(cPoint(left, SmallTop), *InfoText, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              m_FontSml, InfoWidth);
        left += InfoWidth;
    }

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors) {  // Separate config option
        const cString RecErrIcon = cString::sprintf("%s_replay", *GetRecordingErrorIcon(RecInfo->Errors()));

        img = ImgLoader.GetIcon(*RecErrIcon, kIconMaxSize, m_FontSmlHeight);  // Small image
        if (img) {
            left += m_MarginItem;
            IconsPixmap->DrawImage(cPoint(left, SmallTop), *img);
        }
    }  // PlaybackShowRecordingErrors
#endif
}

void cFlatDisplayReplay::SetTitle(const char *Title) {
    TopBarSetTitle(Title);
    TopBarSetMenuIcon("extraIcons/Playing");
}

void cFlatDisplayReplay::Action() {
    if (m_DimmActive) return;  // Already dimmed, no need to start thread again

    // Start dimming thread
    while (Running()) {
        time_t now {time(0)};
        if ((now - m_DimmStartTime) > Config.RecordingDimmOnPauseDelay && Running()) {
            m_DimmActive = true;
            // Use batch: fade in, then go back to sleep
            const int step {4};
            for (int alpha {0}; alpha <= Config.RecordingDimmOnPauseOpaque && Running(); alpha += step) {
                PixmapFill(DimmPixmap, ArgbToColor(alpha, 0, 0, 0));
                Flush();
                cCondWait::SleepMs(6);  // Not too fast
            }
            Cancel(-1);
        }
        cCondWait::SleepMs(100);  // Sleep for 100ms before checking again
    }
}

void cFlatDisplayReplay::SetMode(bool Play, bool Forward, int Speed) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayReplay::SetMode()");
    dsyslog("   Setup.ShowReplayMode: %d, Speed %d", Setup.ShowReplayMode, Speed);
#endif

    if (!LabelPixmap || !IconsPixmap) return;

    if (Config.RecordingDimmOnPause) {
        if (Play == false) {
            time(&m_DimmStartTime);  // Set start time for dimming
            if (!Start()) esyslog("flatPlus: cFlatDisplayReplay::SetMode() Could not start thread in DisplayReplay");
        } else {
            Cancel(-1);
            while (Active())
                cCondWait::SleepMs(10);
            if (m_DimmActive) {
                m_DimmActive = false;  // Reset dimming state to false
                PixmapClear(DimmPixmap);
                Flush();
            }
        }
    }

    int left {0};
    const int FontWidth00 {FontCache.GetStringWidth(m_FontName, m_FontHeight, "00")};  // Width of '00'
    const int EffectiveOsdWidth {m_OsdWidth - Config.decorBorderReplaySize * 2};
    if (Setup.ShowReplayMode) {
        left = (EffectiveOsdWidth - (m_FontHeight * 4 + m_MarginItem3)) / 2;

        if (m_ModeOnly) PixmapClear(LabelPixmap);

        // PixmapClear(IconsPixmap);  //* Moved to SetRecording
        LabelPixmap->DrawRectangle(cRect(left - FontWidth00 - m_MarginItem, 0,
                                         m_FontHeight * 4 + m_MarginItem * 6 + FontWidth00 * 2, m_FontHeight),
                                   Theme.Color(clrReplayBg));

        cString rewind {"rewind"}, pause {"pause"}, play {"play"}, forward {"forward"};
        if (Speed == -1) {  // Replay or pause
            (Play) ? play = "play_sel" : pause = "pause_sel";
        } else {
            const cString speed = itoa(Speed);
            if (Forward) {
                forward = "forward_sel";
                LabelPixmap->DrawText(cPoint(left + m_FontHeight * 4 + m_MarginItem * 4, 0), speed,
                                      Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), m_Font);
            } else {
                rewind = "rewind_sel";
                LabelPixmap->DrawText(cPoint(left - m_Font->Width(speed) - m_MarginItem, 0), speed,
                                      Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), m_Font);
            }
        }
        cImage *img {ImgLoader.GetIcon(*rewind, m_FontHeight, m_FontHeight)};
        if (img) IconsPixmap->DrawImage(cPoint(left, 0), *img);

        img = ImgLoader.GetIcon(*pause, m_FontHeight, m_FontHeight);
        if (img) IconsPixmap->DrawImage(cPoint(left + m_FontHeight + m_MarginItem, 0), *img);

        img = ImgLoader.GetIcon(*play, m_FontHeight, m_FontHeight);
        if (img) IconsPixmap->DrawImage(cPoint(left + m_FontHeight2 + m_MarginItem2, 0), *img);

        img = ImgLoader.GetIcon(*forward, m_FontHeight, m_FontHeight);
        if (img) IconsPixmap->DrawImage(cPoint(left + m_FontHeight * 3 + m_MarginItem3, 0), *img);
    }

    if (m_ProgressShown) {
        const sDecorBorder ib {Config.decorBorderReplaySize,
                               m_OsdHeight - m_LabelHeight - m_ProgressBarHeight - Config.decorBorderReplaySize -
                                   m_MarginItem,
                               EffectiveOsdWidth,
                               m_LabelHeight + m_ProgressBarHeight + m_MarginItem,
                               Config.decorBorderReplaySize,
                               Config.decorBorderReplayType,
                               Config.decorBorderReplayFg,
                               Config.decorBorderReplayBg};
        DecorBorderDraw(ib);
    } else {
        if (m_ModeOnly) {
            const sDecorBorder ib {left - FontWidth00 - m_MarginItem + Config.decorBorderReplaySize,
                                   m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                                   m_FontHeight * 4 + m_MarginItem * 6 + FontWidth00 * 2,
                                   m_FontHeight,
                                   Config.decorBorderReplaySize,
                                   Config.decorBorderReplayType,
                                   Config.decorBorderReplayFg,
                                   Config.decorBorderReplayBg};
            DecorBorderDraw(ib);
        } else {
            const sDecorBorder ib {Config.decorBorderReplaySize,
                                   m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                                   EffectiveOsdWidth,
                                   m_LabelHeight,
                                   Config.decorBorderReplaySize,
                                   Config.decorBorderReplayType,
                                   Config.decorBorderReplayFg,
                                   Config.decorBorderReplayBg};
            DecorBorderDraw(ib);
        }
    }
    ResolutionAspectDraw();
}

void cFlatDisplayReplay::SetProgress(int Current, int Total) {
    if (m_DimmActive) {
        m_DimmActive = false;  // Reset dimming state to false
        PixmapClear(DimmPixmap);
        Flush();
    }

    if (m_ModeOnly) return;

    m_CurrentFrame = Current;
    m_ProgressShown = true;

#if APIVERSNUM >= 30004
    ProgressBarDrawMarks(Current, Total, marks, errors, Theme.Color(clrReplayMarkFg),
                         Theme.Color(clrReplayMarkCurrentFg));
#else
    ProgressBarDrawMarks(Current, Total, marks, Theme.Color(clrReplayMarkFg), Theme.Color(clrReplayMarkCurrentFg));
#endif
}

void cFlatDisplayReplay::SetCurrent(const char *Current) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayReplay::SetCurrent() '%s'", Current);
#endif
    if (m_ModeOnly) return;

    m_Current = Current;
    UpdateInfo();
}

void cFlatDisplayReplay::SetTotal(const char *Total) {
    if (m_ModeOnly) return;

    m_Total = Total;
    if (m_Current[0] != '\0')  // Do not call 'UpdateInfo()' when 'm_Current' is not set
        UpdateInfo();
}

void cFlatDisplayReplay::UpdateInfo() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayReplay::UpdateInfo()");
#endif

    if (m_ModeOnly || !ChanEpgImagesPixmap || !IconsPixmap || !LabelPixmap) return;

    const int FontSecsSize {static_cast<int>(Setup.FontOsdSize * Config.TimeSecsScale * 100.0)};  // Size for seconds
    const int TopSecs {m_FontAscender - ((Config.TimeSecsScale < 1.0)
                                             ? FontCache.GetFontAscender(Setup.FontOsd, FontSecsSize)
                                             : 0)};                                   // Top position for seconds
    const int FontSecsHeight {FontCache.GetFontHeight(Setup.FontOsd, FontSecsSize)};  // Height of seconds font
    static constexpr uint32_t kCharCode {0x0030};                                     // U+0030 DIGIT ZERO
    // Cached glyph size
    static int CachedGlyphSize = -1;
    static cString CachedFontOsd {""};
    static int CachedFontOsdSize = -1;
    if (strcmp(Setup.FontOsd, CachedFontOsd) != 0 || Setup.FontOsdSize != CachedFontOsdSize) {
        CachedFontOsd = Setup.FontOsd;
        CachedFontOsdSize = Setup.FontOsdSize;
        CachedGlyphSize = FontCache.GetGlyphSize(Setup.FontOsd, kCharCode, Setup.FontOsdSize);
    }
    const int GlyphSize {CachedGlyphSize};
    const int TopOffset {m_FontAscender - GlyphSize};

#ifdef DEBUGFUNCSCALL
    dsyslog("   GlyphSize %d, Setup.FontOsdSize %d, m_FontHeight %d, FontAscender %d", GlyphSize, Setup.FontOsdSize,
            m_FontHeight, m_FontAscender);
#endif

    //* Draw current position with symbol (1. line)
    int left {m_MarginItem};
    cImage *img {ImgLoader.GetIcon("recording_pos", kIconMaxSize, GlyphSize)};
    if (img) {
        IconsPixmap->DrawImage(cPoint(left, TopOffset), *img);
        left += img->Width() + m_MarginItem;
    }

    if (Config.TimeSecsScale < 1.0) {
        std::string_view cur {*m_Current};
        const std::size_t found {cur.find_last_of(':')};
        if (found != std::string_view::npos) {
            cString hm {*m_Current};
            hm.Truncate(found);  // Hours and minutes
            std::string_view secs {cur.substr(found, cur.length() - found)};  // Seconds (:00 or :00.00)
            // Fix for leftover .00 when in edit mode. Add margin to fix extra pixel glitch
            const int FontSecsWidth {FontCache.GetStringWidth(m_FontName, FontSecsHeight, ":00.00") + m_MarginItem};

            const int HmWidth {m_Font->Width(*hm)};  // Width of hours and minutes
            LabelPixmap->DrawText(cPoint(left, 0), *hm, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_Font,
                                  HmWidth, m_FontHeight);
            left += HmWidth;
            LabelPixmap->DrawText(cPoint(left, TopSecs), secs.data(), Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_FontSecs, FontSecsWidth, FontSecsHeight);
            // left += FontSecsWidth;
        } else {
            // Fix for leftover .00 when in edit mode. Add margin to fix extra pixel glitch
            const int CurrentWidth {std::max(m_Font->Width(*m_Current) + m_MarginItem, m_LastCurrentWidth)};
            m_LastCurrentWidth = CurrentWidth;

            LabelPixmap->DrawText(cPoint(left, 0), *m_Current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  m_Font, CurrentWidth, m_FontHeight);
            // left += CurrentWidth;
        }
    } else {
        // Fix for leftover .00 when in edit mode. Add margin to fix extra pixel glitch
        const int CurrentWidth {std::max(m_Font->Width(*m_Current) + m_MarginItem, m_LastCurrentWidth)};
        m_LastCurrentWidth = CurrentWidth;

        LabelPixmap->DrawText(cPoint(left, 0), *m_Current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_Font,
                              CurrentWidth, m_FontHeight);
        // left += CurrentWidth;
    }

    // Check if m_Recording is null and return if so
    if (!m_Recording) return;

    int FramesAfterEdit {-1};
    int CurrentFramesAfterEdit {-1};
    const int NumFrames {m_Recording->NumFrames()};  // Total frames in recording
    if (NumFrames <= 0) {
        esyslog("flatPlus: cFlatDisplayReplay::UpdateInfo() Invalid NumFrames: %d", NumFrames);
        return;
    }

    if (marks && m_Recording->HasMarks()) {
#if (APIVERSNUM >= 20608)
        FramesAfterEdit = marks->GetFrameAfterEdit(NumFrames, NumFrames);
        if (FramesAfterEdit >= 0) CurrentFramesAfterEdit = marks->GetFrameAfterEdit(m_CurrentFrame, NumFrames);
#else
        FramesAfterEdit = GetFrameAfterEdit(marks, NumFrames, NumFrames);
        if (FramesAfterEdit >= 0) CurrentFramesAfterEdit = GetFrameAfterEdit(marks, m_CurrentFrame, NumFrames);
#endif
    }

    const double FramesPerSecond {m_Recording->FramesPerSecond()};
    if (FramesPerSecond <= 0.0) {  // Avoid DIV/0
        esyslog("flatPlus: cFlatDisplayReplay::UpdateInfo() Invalid FramesPerSecond: %.2f", FramesPerSecond);
        return;
    }

    //* Draw total and cutted length with cutted symbol (Right side, 1. line)
    const int BorderSize {Config.decorBorderReplaySize * 2};                     // Border size
    const int TotalWidth {m_Font->Width(m_Total)};                               // Width of total length
    const int Spacer {FontCache.GetStringWidth(m_FontName, m_FontHeight, "0")};  // Space between total and cutt length
    img = ImgLoader.GetIcon("recording_total", kIconMaxSize, GlyphSize);
    const int ImgWidth {(img) ? img->Width() : 0};
    if (FramesAfterEdit > 0) {
        const cString cutted = *IndexToHMSF(FramesAfterEdit, false, FramesPerSecond);
        const int CuttedWidth {m_Font->Width(cutted)};  // Width of cutted length
        cImage *ImgCutted {ImgLoader.GetIcon("recording_cutted_extra", kIconMaxSize, GlyphSize)};
        const int ImgCuttedWidth {(ImgCutted) ? ImgCutted->Width() : 0};

        int right {m_OsdWidth - BorderSize - ImgWidth - m_MarginItem - TotalWidth - Spacer - ImgCuttedWidth -
                   m_MarginItem - CuttedWidth};

        if (Config.TimeSecsScale < 1.0) {
            std::string_view tot {*m_Total};  // Total length
            const std::size_t found {tot.find_last_of(':')};
            if (found != std::string_view::npos) {
                cString hm {*m_Total};
                hm.Truncate(found);  // Hours and minutes
                std::string_view secs {tot.substr(found, tot.length() - found)};
                const int HmWidth {m_Font->Width(*hm)};  // Width of hours and minutes
                const int SecsWidth {FontCache.GetStringWidth(m_FontName, FontSecsHeight, ":00")};  // Width of seconds

                std::string_view cutt {*cutted};  // Cutted length
                const std::size_t found2 {cutt.find_last_of(':')};
                if (found2 != std::string_view::npos) {
                    cString hm2 {*cutted};
                    hm2.Truncate(found2);  // Hours and minutes
                    std::string_view secs2 {cutt.substr(found, cutt.length() - found)};

                    right = m_OsdWidth - BorderSize - ImgWidth - m_MarginItem - HmWidth - SecsWidth - Spacer -
                            ImgCuttedWidth - m_MarginItem - m_Font->Width(*hm2) - SecsWidth;
                } else {
                    right = m_OsdWidth - BorderSize - ImgWidth - m_MarginItem - HmWidth - SecsWidth - Spacer -
                            ImgCuttedWidth - m_MarginItem - CuttedWidth;
                }

                if (img) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                    right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right, 0), *hm, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                      m_Font, HmWidth, m_FontHeight);
                LabelPixmap->DrawText(cPoint(right + HmWidth, TopSecs), secs.data(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_FontSecs, SecsWidth + m_MarginItem, FontSecsHeight);
                right += HmWidth + SecsWidth + Spacer;
            } else {
                if (img) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                    right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right, 0), m_Total, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                      m_Font, TotalWidth, m_FontHeight);
                right += TotalWidth + Spacer;
            }
        } else {
            if (img) {  // Draw preloaded 'recording_total' icon
                IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
            }
            LabelPixmap->DrawText(cPoint(right, 0), m_Total, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  m_Font, TotalWidth, m_FontHeight);
            right += TotalWidth + Spacer;
        }  // Config.TimeSecsScale < 1.0

        if (ImgCutted) {  // Draw preloaded 'recording_cutted_extra' icon
            IconsPixmap->DrawImage(cPoint(right, TopOffset), *ImgCutted);
            right += ImgCuttedWidth + m_MarginItem;  // 'ImgCuttedWidth' is already set
        }

        if (Config.TimeSecsScale < 1.0) {
            std::string_view cutt {*cutted};
            const std::size_t found {cutt.find_last_of(':')};
            if (found != std::string_view::npos) {
                cString hm {*cutted};
                hm.Truncate(found);  // Hours and minutes
                std::string_view secs {cutt.substr(found, cutt.length() - found)};
                const int HmWidth {m_Font->Width(*hm)};  // Width of hours and minutes
                const int SecsWidth {FontCache.GetStringWidth(m_FontName, FontSecsHeight, ":00")};  // Width of seconds

                LabelPixmap->DrawText(cPoint(right, 0), *hm, Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), m_Font, HmWidth, m_FontHeight);
                LabelPixmap->DrawText(cPoint(right + HmWidth, TopSecs), secs.data(),
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      SecsWidth + m_MarginItem, FontSecsHeight);
            } else {
                LabelPixmap->DrawText(cPoint(right, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), m_Font, CuttedWidth, m_FontHeight);
            }
        } else {
            LabelPixmap->DrawText(cPoint(right, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                  Theme.Color(clrReplayBg), m_Font, CuttedWidth, m_FontHeight);
        }
    } else {  // Not cutted
        int right {m_OsdWidth - BorderSize - ImgWidth - m_MarginItem - TotalWidth};
        if (Config.TimeSecsScale < 1.0) {
            std::string_view tot {*m_Total};
            const std::size_t found {tot.find_last_of(':')};
            if (found != std::string_view::npos) {
                cString hm {*m_Total};
                hm.Truncate(found);  // Hours and minutes
                std::string_view secs {tot.substr(found, tot.length() - found)};
                const int HmWidth {m_Font->Width(*hm)};                // Width of hours and minutes
                const int SecsWidth {FontCache.GetStringWidth(m_FontName, FontSecsHeight, ":00")};  // Width of seconds

                right = m_OsdWidth - BorderSize - ImgWidth - m_MarginItem - HmWidth - SecsWidth;
                if (img) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                    right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right, 0), *hm, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                      m_Font, HmWidth, m_FontHeight);
                LabelPixmap->DrawText(cPoint(right + HmWidth, TopSecs), secs.data(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_FontSecs, SecsWidth + m_MarginItem, FontSecsHeight);
            } else {
                if (img) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                    right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right, 0), *m_Total, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                      m_Font, TotalWidth, m_FontHeight);
            }
        } else {
            if (img) {  // Draw preloaded 'recording_total' icon
                IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
            }
            LabelPixmap->DrawText(cPoint(right, 0), *m_Total, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  m_Font, TotalWidth, m_FontHeight);
        }
    }  // HasMarks

    //* Draw end time of recording with symbol for cutted end time (2. line)
    const time_t now {time(0)};  // Fix 'jumping' end times - Update once per minute or 'm_Current' changed
    if (Config.PlaybackShowEndTime > 0 &&  // 1 = End time, 2 = End time and cutted end time
        (m_LastEndTimeUpdate + 60 < now || strcmp(*m_Current, *m_LastCurrent) != 0)) {
        m_LastEndTimeUpdate = now;
        m_LastCurrent = m_Current;
        left = m_MarginItem;
        //* Image instead of 'ends at:' text
        /* img = ImgLoader.GetIcon("recording_finish", m_FontHeight, m_FontHeight);
        if (img) {
            IconsPixmap->DrawImage(cPoint(left, m_FontHeight), *img);
            left += img->Width() + m_MarginItem;
        } */

        const int Rest {NumFrames - m_CurrentFrame};
        const cString TimeStr = (Rest >= 0) ? *TimeString(now + (Rest / FramesPerSecond)) : "??:??";  // HH:MM
        cString EndTime = cString::sprintf("%s: %s", tr("ends at"), *TimeStr);
        const int EndTimeWidth {FontCache.GetStringWidth(
            m_FontName, m_FontHeight, cString::sprintf("%s: 00:00", tr("ends at")))};  // Width of 'ends at: HH:MM' text
        LabelPixmap->DrawText(cPoint(left, m_FontHeight), *EndTime, Theme.Color(clrReplayFont),
                              Theme.Color(clrReplayBg), m_Font, EndTimeWidth, m_FontHeight);
        left += EndTimeWidth + Spacer;

        //* Draw end time of cutted recording with cutted symbol
        if (Config.PlaybackShowEndTime == 2 && FramesAfterEdit > 0) {
            const int RestCutted {FramesAfterEdit - CurrentFramesAfterEdit};
            EndTime = *TimeString(now + (RestCutted / FramesPerSecond));  // HH:MM
            if (strcmp(TimeStr, EndTime) != 0) {                          // Only if not equal
                img = ImgLoader.GetIcon("recording_cutted_extra", kIconMaxSize, GlyphSize);
                if (img) {
                    IconsPixmap->DrawImage(cPoint(left, m_FontHeight + TopOffset), *img);
                    left += img->Width() + m_MarginItem;
                }

                LabelPixmap->DrawText(cPoint(left, m_FontHeight), *EndTime, Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(*EndTime), m_FontHeight);
                // left += m_Font->Width(*EndTime) + m_MarginItem;  //* 'left' is not used anymore from here
            }
        }
    }  // Config.PlaybackShowEndTime

    //* Draw Banner/Poster (Update only every 5 seconds)
    if (m_LastPosterBannerUpdate + 5 < now) {
        m_LastPosterBannerUpdate = now;
        cString MediaPath {""};
        cSize MediaSize {0, 0};
        if (Config.TVScraperReplayInfoShowPoster) {
            GetScraperMediaTypeSize(MediaPath, MediaSize, nullptr, m_Recording);
            if (MediaPath[0] == '\0' && Config.TVScraperSearchLocalPosters) {  // Prio for tvscraper poster
                const cString RecPath = m_Recording->FileName();
                if (ImgLoader.SearchRecordingPoster(RecPath, MediaPath)) {
                    img = ImgLoader.GetFile(*MediaPath, m_TVSRect.Width(), m_TVSRect.Height());
                    if (img)
                        MediaSize.Set(img->Width(), img->Height());  // Get values for SetMediaSize()
                    else
                        MediaPath = "";  // Just in case image can not be loaded
                }
            }
        }

        PixmapClear(ChanEpgImagesPixmap);
        DecorBorderClearByFrom(BorderTVSPoster);
        if (MediaPath[0] != '\0') {
            SetMediaSize(m_TVSRect.Size(), MediaSize,
            Config.TVScraperReplayInfoPosterSize * 100);  // Set size and apply user setting
            img = ImgLoader.GetFile(*MediaPath, MediaSize.Width(), MediaSize.Height());
            if (img) {
                PixmapSetAlpha(ChanEpgImagesPixmap, 255 * Config.TVScraperPosterOpacity * 100);  // Set transparency
                ChanEpgImagesPixmap->DrawImage(cPoint(0, 0), *img);

                const sDecorBorder ib {m_MarginEPGImage + Config.decorBorderChannelEPGSize,
                                       m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_MarginEPGImage +
                                           Config.decorBorderChannelEPGSize,
                                       img->Width(),
                                       img->Height(),
                                       Config.decorBorderChannelEPGSize,
                                       Config.decorBorderChannelEPGType,
                                       Config.decorBorderChannelEPGFg,
                                       Config.decorBorderChannelEPGBg,
                                       BorderTVSPoster};
                DecorBorderDraw(ib);
            }
        }
    }  // m_LastPosterBannerUpdate
}

void cFlatDisplayReplay::SetJump(const char *Jump) {
    if (!LabelJumpPixmap) return;

    DecorBorderClearByFrom(BorderRecordJump);

    if (!Jump) {
        PixmapClear(LabelJumpPixmap);
        return;
    }
    const int JumpWidth {m_Font->Width(Jump)};
    const int left {(m_OsdWidth - Config.decorBorderReplaySize * 2 - JumpWidth) / 2};

    LabelJumpPixmap->DrawText(cPoint(left, 0), Jump, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_Font,
                              JumpWidth, m_FontHeight, taCenter);

    const sDecorBorder ib {left + Config.decorBorderReplaySize,
                           m_OsdHeight - m_LabelHeight - m_ProgressBarHeight * 2 - m_MarginItem3 - m_FontHeight -
                               Config.decorBorderReplaySize * 2,
                           JumpWidth,
                           m_FontHeight,
                           Config.decorBorderReplaySize,
                           Config.decorBorderReplayType,
                           Config.decorBorderReplayFg,
                           Config.decorBorderReplayBg,
                           BorderRecordJump};
    DecorBorderDraw(ib);
}

void cFlatDisplayReplay::ResolutionAspectDraw() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayReplay::ResolutionAspectDraw()");
#endif

    if (m_ModeOnly || !IconsPixmap || m_ScreenWidth <= 0 || m_ScreenHeight <= 0) return;

    // First line for current, total and cutted length, second line for end time
    const int ImageTop {(Config.PlaybackShowEndTime > 0) ? m_FontHeight2 : m_FontHeight};  // Top of bottom line

    int left {m_OsdWidth - Config.decorBorderReplaySize * 2};
    cImage *img {nullptr};
    cString IconName {""};
    if (Config.RecordingResolutionAspectShow) {  // Show Aspect (16:9)
        IconName = *GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
        img = ImgLoader.GetIcon(*IconName, kIconMaxSize, m_FontSmlHeight);
        if (img) {
            left -= img->Width();
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left -= m_MarginItem2;
        }

        IconName = *GetScreenResolutionIcon(m_ScreenWidth, m_ScreenHeight);  // Show Resolution (1920x1080)
        img = ImgLoader.GetIcon(*IconName, kIconMaxSize, m_FontSmlHeight);
        if (img) {
            left -= img->Width();
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left -= m_MarginItem2;
        }
    }

    if (Config.RecordingFormatShow && !Config.RecordingSimpleAspectFormat) {
        IconName = *GetFormatIcon(m_ScreenWidth);  // Show Format (HD)
        img = ImgLoader.GetIcon(*IconName, kIconMaxSize, m_FontSmlHeight);
        if (img) {
            left -= img->Width();
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left -= m_MarginItem2;
        }
    }

    if (Config.RecordingAudioFormatShow) {  // Show audio icon (Dolby, Stereo)
        IconName = *GetCurrentAudioIcon();
        img = ImgLoader.GetIcon(*IconName, kIconMaxSize, m_FontSmlHeight);
        if (img) {
            left -= img->Width();
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            // left -= m_MarginItem2;
        }
    }
}

void cFlatDisplayReplay::SetMessage(eMessageType Type, const char *Text) {
    (Text) ? MessageSet(Type, Text) : MessageClear();
}

void cFlatDisplayReplay::Flush() {
    if (Config.RecordingResolutionAspectShow) {
        cDevice::PrimaryDevice()->GetVideoSize(m_ScreenWidth, m_ScreenHeight, m_ScreenAspect);
        if (m_ScreenWidth != m_LastScreenWidth) {
            m_LastScreenWidth = m_ScreenWidth;
            ResolutionAspectDraw();
        }
    }

    TopBarUpdate();
    m_Osd->Flush();
}

void cFlatDisplayReplay::PreLoadImages() {
    static constexpr const char *icons[] {"rewind",     "pause",     "play",     "forward",
                                          "rewind_sel", "pause_sel", "play_sel", "forward_sel"};
    for (const auto &icon : icons) {
        ImgLoader.GetIcon(icon, m_FontHeight, m_FontHeight);
    }

    static constexpr uint32_t kCharCode {0x0030};  // U+0030 DIGIT ZERO
    const int GlyphSize = FontCache.GetGlyphSize(Setup.FontOsd, kCharCode, Setup.FontOsdSize);  // Narrowing conversion
    static constexpr const char *icons1[] {"recording_pos", "recording_total", "recording_cutted_extra"};
    for (const auto &icon : icons1) {
        ImgLoader.GetIcon(icon, kIconMaxSize, GlyphSize);
    }

    static constexpr const char *icons2[] {"recording_untested_replay", "recording_ok_replay",
                                           "recording_warning_replay", "recording_error_replay", "timerRecording"};
    for (const auto &icon : icons2) {
        ImgLoader.GetIcon(icon, kIconMaxSize, m_FontSmlHeight);
    }

    static constexpr const char *icons3[] {
        "43",          "169",     "169w",    "221",     "7680x4320",    "3840x2160",  "1920x1080", "1440x1080",
        "1280x720",    "960x720", "704x576", "720x576", "544x576",      "528x576",    "480x576",   "352x576",
        "unknown_res", "uhd",     "hd",      "sd",      "audio_stereo", "audio_dolby"};
    for (const auto &icon : icons3) {
        ImgLoader.GetIcon(icon, kIconMaxSize, m_FontSmlHeight);
    }
}
