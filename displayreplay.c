/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displayreplay.h"
#include "./flat.h"

cFlatDisplayReplay::cFlatDisplayReplay(bool ModeOnly) : cThread("DisplayReplay") {
    m_LabelHeight = m_FontHeight + m_FontSmlHeight;
    if (Config.PlaybackShowEndTime > 0)  // 1 = End time, 2 = End time and cutted end time
        m_LabelHeight += m_FontHeight;   // Use second line for displaying end time of recording

    m_ModeOnly = ModeOnly;

    CreateFullOsd();
    TopBarCreate();
    MessageCreate();

    m_TVSRect.Set(20 + Config.decorBorderChannelEPGSize,
                m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize,
                m_OsdWidth - 40 - Config.decorBorderChannelEPGSize * 2,
                m_OsdHeight - m_TopBarHeight - m_LabelHeight - 40 - Config.decorBorderChannelEPGSize * 2);
    ChanEpgImagesPixmap = CreatePixmap(m_Osd, "ChanEpgImagesPixmap", 2, m_TVSRect);

    const cRect LabelPixmapViewPort{Config.decorBorderReplaySize,
                                    m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                                    m_OsdWidth - Config.decorBorderReplaySize * 2, m_LabelHeight};
    LabelPixmap = CreatePixmap(m_Osd, "LabelPixmap", 1, LabelPixmapViewPort);
    IconsPixmap = CreatePixmap(m_Osd, "IconsPixmap", 2, LabelPixmapViewPort);

    /* Make Config.decorProgressReplaySize even
    https://stackoverflow.com/questions/4739388/make-an-integer-even
    number = (number / 2) * 2; */
    const int ProgressBarHeight {((Config.decorProgressReplaySize + 1) / 2) * 2};  // Make it even (And round up)
    ProgressBarCreate(
        cRect(Config.decorBorderReplaySize,
              m_OsdHeight - m_LabelHeight - ProgressBarHeight - Config.decorBorderReplaySize - m_MarginItem,
              m_OsdWidth - Config.decorBorderReplaySize * 2, ProgressBarHeight),
        m_MarginItem, 0, Config.decorProgressReplayFg, Config.decorProgressReplayBarFg, Config.decorProgressReplayBg,
        Config.decorProgressReplayType);

    LabelJumpPixmap = CreatePixmap(m_Osd, "LabelJumpPixmap", 1,
                                   cRect(Config.decorBorderReplaySize,
                                         m_OsdHeight - m_LabelHeight - ProgressBarHeight * 2 - m_MarginItem3 -
                                             m_FontHeight - Config.decorBorderReplaySize * 2,
                                         m_OsdWidth - Config.decorBorderReplaySize * 2, m_FontHeight));

    DimmPixmap = CreatePixmap(m_Osd, "DimmPixmap", MAXPIXMAPLAYERS-1, cRect(0, 0, m_OsdWidth, m_OsdHeight));

    PixmapClear(ChanEpgImagesPixmap);
    PixmapFill(LabelPixmap, Theme.Color(clrReplayBg));
    PixmapClear(LabelJumpPixmap);
    PixmapClear(IconsPixmap);
    PixmapClear(DimmPixmap);

    m_FontSecs = cFont::CreateFont(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);

    if (Config.PlaybackWeatherShow)
        DrawWidgetWeather();
}

cFlatDisplayReplay::~cFlatDisplayReplay() {
    if (m_FontSecs)
        delete m_FontSecs;

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

    const cRecordingInfo *RecInfo = Recording->Info();
    m_Recording = Recording;

    PixmapClear(IconsPixmap);

    SetTitle(RecInfo->Title());

    // First line for current, total and cutted length, second for end time
    const int SmallTop {(Config.PlaybackShowEndTime > 0) ? m_FontHeight2 : m_FontHeight};

    // Show if still recording
    int left {m_MarginItem};  // Position for recording symbol/short text/date
    cImage *img {nullptr};
    if ((Recording->IsInUse() & ruTimer) != 0) {  // The recording is currently written to by a timer
        img = ImgLoader.LoadIcon("timerRecording", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);  // Small image
        if (img) {
            const int ImageTop {SmallTop};
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left += img->Width() + m_MarginItem;
        }
    }

    cString InfoText {""};
    if (isempty(RecInfo->ShortText())) {  // No short text. Show date and time instead
        InfoText = cString::sprintf("%s  %s", *ShortDateString(Recording->Start()),
                                    *TimeString(Recording->Start()));
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
    if (Config.PlaybackShowRecordingErrors)
        MaxWidth -= m_FontSmlHeight;  // Substract width of imgRecErr
#endif

    img = ImgLoader.LoadIcon("1920x1080", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    if (img)
        MaxWidth -= img->Width() * 3;  //* Substract guessed max. used space of aspect and format icons

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
            const int DotsWidth {m_FontSml->Width("...")};
            LabelPixmap->DrawText(cPoint(left, SmallTop), *InfoText, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_FontSml, MaxWidth - DotsWidth);
            left += (MaxWidth - DotsWidth);
            LabelPixmap->DrawText(cPoint(left, SmallTop), "...", Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_FontSml, DotsWidth);
            left += DotsWidth;
        }
    } else {  // Short text fits into maxwidth
        LabelPixmap->DrawText(cPoint(left, SmallTop), *InfoText, Theme.Color(clrReplayFont),
                                     Theme.Color(clrReplayBg), m_FontSml, InfoWidth);
        left += InfoWidth;
    }

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors) {  // Separate config option
        const cString RecErrIcon = cString::sprintf("%s_replay", *GetRecordingErrorIcon(RecInfo->Errors()));

        img = ImgLoader.LoadIcon(*RecErrIcon, ICON_WIDTH_UNLIMITED, m_FontSmlHeight);  // Small image
        if (img) {
            left += m_MarginItem;
            const int ImageTop {SmallTop};
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
        }
    }  // PlaybackShowRecordingErrors
#endif
}

void cFlatDisplayReplay::SetTitle(const char *Title) {
    TopBarSetTitle(Title);
    TopBarSetMenuIcon("extraIcons/Playing");
}

void cFlatDisplayReplay::Action() {
    time_t CurTime {0};
    while (Running()) {
        time(&CurTime);
        if ((CurTime - m_DimmStartTime) > Config.RecordingDimmOnPauseDelay) {
            m_DimmActive = true;
            for (int alpha {0}; (alpha <= Config.RecordingDimmOnPauseOpaque) && Running(); alpha += 2) {
                PixmapFill(DimmPixmap, ArgbToColor(alpha, 0, 0, 0));
                Flush();
            }
            Cancel(-1);
            return;
        }
        cCondWait::SleepMs(100);
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
            time(&m_DimmStartTime);
            Start();
        } else {
            Cancel(-1);
            while (Active())
                cCondWait::SleepMs(10);
            if (m_DimmActive) {
                PixmapClear(DimmPixmap);
                Flush();
            }
        }
    }

    int left {0};
    if (Setup.ShowReplayMode) {
        left = (m_OsdWidth - Config.decorBorderReplaySize * 2 - (m_FontHeight * 4 + m_MarginItem3)) / 2;

        if (m_ModeOnly)
            PixmapClear(LabelPixmap);

        // PixmapClear(IconsPixmap);  //* Moved to SetRecording
        LabelPixmap->DrawRectangle(cRect(left - m_Font->Width("99") - m_MarginItem, 0,
                                         m_FontHeight * 4 + m_MarginItem * 6 + m_Font->Width("99") * 2, m_FontHeight),
                                   Theme.Color(clrReplayBg));

        cString rewind {"rewind"}, pause {"pause"}, play {"play"}, forward {"forward"};
        if (Speed == -1) {  // Replay or pause
            (Play) ? play = "play_sel" : pause = "pause_sel";
        } else {
            const cString speed = cString::sprintf("%d", Speed);
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
        cImage *img {ImgLoader.LoadIcon(*rewind, m_FontHeight, m_FontHeight)};
        if (img) IconsPixmap->DrawImage(cPoint(left, 0), *img);

        img = ImgLoader.LoadIcon(*pause, m_FontHeight, m_FontHeight);
        if (img) IconsPixmap->DrawImage(cPoint(left + m_FontHeight + m_MarginItem, 0), *img);

        img = ImgLoader.LoadIcon(*play, m_FontHeight, m_FontHeight);
        if (img) IconsPixmap->DrawImage(cPoint(left + m_FontHeight2 + m_MarginItem2, 0), *img);

        img = ImgLoader.LoadIcon(*forward, m_FontHeight, m_FontHeight);
        if (img) IconsPixmap->DrawImage(cPoint(left + m_FontHeight * 3 + m_MarginItem3, 0), *img);
    }

    if (m_ProgressShown) {
        const sDecorBorder ib {Config.decorBorderReplaySize,
                               m_OsdHeight - m_LabelHeight - m_ProgressBarHeight -
                                   Config.decorBorderReplaySize - m_MarginItem,
                               m_OsdWidth - Config.decorBorderReplaySize * 2,
                               m_LabelHeight + m_ProgressBarHeight + m_MarginItem,
                               Config.decorBorderReplaySize,
                               Config.decorBorderReplayType,
                               Config.decorBorderReplayFg,
                               Config.decorBorderReplayBg};
        DecorBorderDraw(ib);
    } else {
        if (m_ModeOnly) {
            const sDecorBorder ib {left - m_Font->Width("99") - m_MarginItem + Config.decorBorderReplaySize,
                                   m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                                   m_FontHeight * 4 + m_MarginItem * 6 + m_Font->Width("99") * 2,
                                   m_FontHeight,
                                   Config.decorBorderReplaySize,
                                   Config.decorBorderReplayType,
                                   Config.decorBorderReplayFg,
                                   Config.decorBorderReplayBg};
            DecorBorderDraw(ib);
        } else {
            const sDecorBorder ib {Config.decorBorderReplaySize,
                                   m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                                   m_OsdWidth - Config.decorBorderReplaySize * 2,
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
    if (!isempty(*m_Current))  // Do not call 'UpdateInfo()' when 'm_Current' is not set
        UpdateInfo();
}

void cFlatDisplayReplay::UpdateInfo() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayReplay::UpdateInfo()");
#endif

    if (m_ModeOnly || !ChanEpgImagesPixmap || !IconsPixmap || !LabelPixmap) return;

    const int FontSecsAscender {GetFontAscender(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0)};
    const int TopSecs {m_FontAscender - FontSecsAscender};

    constexpr ulong CharCode {0x0030};  // U+0030 DIGIT ZERO
    const int GlyphSize = GetGlyphSize(Setup.FontOsd, CharCode, Setup.FontOsdSize);  // Narrowing conversion
    const int TopOffset {m_FontAscender - GlyphSize};

#ifdef DEBUGFUNCSCALL
    dsyslog("   GlyphSize %d, Setup.FontOsdSize %d, m_FontHeight %d, FontAscender %d",
             GlyphSize, Setup.FontOsdSize, m_FontHeight, m_FontAscender);
#endif

    //* Draw current position with symbol (1. line)
    int left {m_MarginItem};
    cImage *img {ImgLoader.LoadIcon("recording_pos", ICON_WIDTH_UNLIMITED, GlyphSize)};
    if (img) {
        IconsPixmap->DrawImage(cPoint(left, TopOffset), *img);
        left += img->Width() + m_MarginItem;
    }

    if (Config.TimeSecsScale < 1.0) {
        std::string_view cur {*m_Current};
        const std::size_t found {cur.find_last_of(':')};
        if (found != std::string::npos) {
            const std::string hm {cur.substr(0, found)};
            const std::string secs {cur.substr(found, cur.length() - found)};
            // Fix for leftover .00 when in edit mode. Add margin to fix extra pixel glitch
            const int FontSecsWidth {std::max(m_FontSecs->Width(secs.c_str()) + m_MarginItem, m_LastCurrentWidth)};
            m_LastCurrentWidth = FontSecsWidth;

            LabelPixmap->DrawText(cPoint(left, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHeight);
            left += m_Font->Width(hm.c_str());
            LabelPixmap->DrawText(cPoint(left, TopSecs), secs.c_str(),
                                  Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                  FontSecsWidth, m_FontSecs->Height());
            left += FontSecsWidth;
        } else {
            // Fix for leftover .00 when in edit mode. Add margin to fix extra pixel glitch
            const int CurrentWidth {std::max(m_Font->Width(*m_Current) + m_MarginItem, m_LastCurrentWidth)};
            m_LastCurrentWidth = CurrentWidth;

            LabelPixmap->DrawText(cPoint(left, 0), *m_Current, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, CurrentWidth, m_FontHeight);
            left += CurrentWidth;
        }
    } else {
        // Fix for leftover .00 when in edit mode. Add margin to fix extra pixel glitch
        const int CurrentWidth {std::max(m_Font->Width(*m_Current) + m_MarginItem, m_LastCurrentWidth)};
        m_LastCurrentWidth = CurrentWidth;

        LabelPixmap->DrawText(cPoint(left, 0), *m_Current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              m_Font, CurrentWidth, m_FontHeight);
        left += CurrentWidth;
    }

    // Check if m_Recording is null and return if so
    if (!m_Recording) return;

    int FramesAfterEdit {-1};
    int CurrentFramesAfterEdit {-1};
    const int NumFrames {m_Recording->NumFrames()};  // Total frames in recording

    if (marks && m_Recording->HasMarks()) {
#if (APIVERSNUM >= 20608)
        FramesAfterEdit = marks->GetFrameAfterEdit(NumFrames, NumFrames);
        if (FramesAfterEdit >= 0)
            CurrentFramesAfterEdit = marks->GetFrameAfterEdit(m_CurrentFrame, NumFrames);
#else
        FramesAfterEdit = GetFrameAfterEdit(marks, NumFrames, NumFrames);
        if (FramesAfterEdit >= 0)
            CurrentFramesAfterEdit = GetFrameAfterEdit(marks, m_CurrentFrame, NumFrames);
#endif
    }

    const double FramesPerSecond {m_Recording->FramesPerSecond()};
    if (FramesPerSecond == 0.0) {  // Avoid DIV/0
        esyslog("flatPlus: Error in cFlatDisplayReplay::UpdateInfo() FramesPerSecond is 0!");
        return;
    }

    //* Draw total and cutted length with cutted symbol (Right side, 1. line)
    const int Spacer {m_Font->Width("0")};  // One digit width space between total and cutted length
    img = ImgLoader.LoadIcon("recording_total", ICON_WIDTH_UNLIMITED, GlyphSize);
    const int ImgWidth {(img) ? img->Width() : 0};
    if (FramesAfterEdit > 0) {
        const cString cutted = *IndexToHMSF(FramesAfterEdit, false, FramesPerSecond);
        cImage *ImgCutted {ImgLoader.LoadIcon("recording_cutted_extra", ICON_WIDTH_UNLIMITED, GlyphSize)};
        const int ImgCuttedWidth {(ImgCutted) ? ImgCutted->Width() : 0};

        int right {m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgWidth - m_MarginItem -
                   m_Font->Width(m_Total) - Spacer - ImgCuttedWidth - m_MarginItem - m_Font->Width(cutted)};

        if (Config.TimeSecsScale < 1.0) {
            std::string_view tot {*m_Total};  // Total length
            const std::size_t found {tot.find_last_of(':')};
            if (found != std::string::npos) {
                const std::string hm {tot.substr(0, found)};
                const std::string secs {tot.substr(found, tot.length() - found)};

                std::string_view cutt {*cutted};  // Cutted length
                const std::size_t found2 {cutt.find_last_of(':')};
                if (found2 != std::string::npos) {
                    const std::string hm2 {cutt.substr(0, found)};
                    const std::string secs2 {cutt.substr(found, cutt.length() - found)};

                    right = m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgWidth - m_MarginItem -
                            m_Font->Width(hm.c_str()) - m_FontSecs->Width(secs.c_str()) - Spacer -
                            ImgCuttedWidth - m_MarginItem - m_Font->Width(hm2.c_str()) -
                            m_FontSecs->Width(secs2.c_str());
                } else {
                    right = m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgWidth - m_MarginItem -
                            m_Font->Width(hm.c_str()) - m_FontSecs->Width(secs.c_str()) - Spacer -
                            ImgCuttedWidth - m_MarginItem - m_Font->Width(cutted);
                }

                if (img) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                    right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHeight);
                LabelPixmap->DrawText(cPoint(right + m_Font->Width(hm.c_str()), TopSecs),
                                      secs.c_str(), Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
                right += m_Font->Width(hm.c_str()) + m_FontSecs->Width(secs.c_str()) + Spacer;
            } else {
                if (img) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                    right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right, 0), m_Total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
                right += m_Font->Width(m_Total) + Spacer;
            }
        } else {
            if (img) {  // Draw preloaded 'recording_total' icon
                IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
            }
            LabelPixmap->DrawText(cPoint(right, 0), m_Total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
            right += m_Font->Width(m_Total) + Spacer;
        }  // Config.TimeSecsScale < 1.0

        if (ImgCutted) {  // Draw preloaded 'recording_cutted_extra' icon
            IconsPixmap->DrawImage(cPoint(right, TopOffset), *ImgCutted);
            right += ImgCuttedWidth + m_MarginItem;  // 'ImgCuttedWidth' is already set
        }

        if (Config.TimeSecsScale < 1.0) {
            std::string_view cutt {*cutted};
            const std::size_t found {cutt.find_last_of(':')};
            if (found != std::string::npos) {
                const std::string hm {cutt.substr(0, found)};
                const std::string secs {cutt.substr(found, cutt.length() - found)};

                LabelPixmap->DrawText(cPoint(right, 0), hm.c_str(),
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), m_Font,
                                      m_Font->Width(hm.c_str()), m_FontHeight);
                LabelPixmap->DrawText(cPoint(right + m_Font->Width(hm.c_str()), TopSecs),
                                      secs.c_str(), Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg),
                                      m_FontSecs, m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
            } else {
                LabelPixmap->DrawText(cPoint(right, 0), *cutted,
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), m_Font,
                                      m_Font->Width(cutted), m_FontHeight);
            }
        } else {
            LabelPixmap->DrawText(cPoint(right, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(cutted), m_FontHeight);
        }
    } else {  // Not cutted
        int right =
            m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgWidth - m_MarginItem - m_Font->Width(m_Total);
        if (Config.TimeSecsScale < 1.0) {
            std::string_view tot {*m_Total};
            const std::size_t found {tot.find_last_of(':')};
            if (found != std::string::npos) {
                const std::string hm {tot.substr(0, found)};
                const std::string secs {tot.substr(found, tot.length() - found)};

                right = m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgWidth - m_MarginItem -
                        m_Font->Width(hm.c_str()) - m_FontSecs->Width(secs.c_str());
                if (img) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                    right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHeight);
                LabelPixmap->DrawText(cPoint(right + m_Font->Width(hm.c_str()), TopSecs),
                                      secs.c_str(), Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
            } else {
                if (img) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                    right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right, 0), *m_Total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
            }
        } else {
            if (img) {  // Draw preloaded 'recording_total' icon
                IconsPixmap->DrawImage(cPoint(right, TopOffset), *img);
                right += ImgWidth + m_MarginItem;  // 'ImgWidth' is already set
            }
            LabelPixmap->DrawText(cPoint(right, 0), *m_Total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
        }
    }  // HasMarks

    //* Draw end time of recording with symbol for cutted end time (2. line)
    const time_t CurTime {time(0)};  // Fix 'jumping' end times - Update once per minute or 'm_Current' current changed
    if (Config.PlaybackShowEndTime > 0 &&  // 1 = End time, 2 = End time and cutted end time
        (m_LastEndTimeUpdate + 60 < CurTime || strcmp(*m_Current, *m_LastCurrent) != 0)) {
        m_LastEndTimeUpdate = CurTime;
        m_LastCurrent = m_Current;
        left = m_MarginItem;
        //* Image instead of 'ends at:' text
        /* img = ImgLoader.LoadIcon("recording_finish", m_FontHeight, m_FontHeight);
        if (img) {
            IconsPixmap->DrawImage(cPoint(left, m_FontHeight), *img);
            left += img->Width() + m_MarginItem;
        } */

        const int Rest {NumFrames - m_CurrentFrame};
        const cString TimeStr = cString::sprintf("%s" , *TimeString(time(0) + (Rest / FramesPerSecond)));  // HH:MM
        cString EndTime = cString::sprintf("%s: %s", tr("ends at"), *TimeStr);
        LabelPixmap->DrawText(cPoint(left, m_FontHeight), *EndTime, Theme.Color(clrReplayFont),
                              Theme.Color(clrReplayBg), m_Font, m_Font->Width(*EndTime), m_FontHeight);
        left += m_Font->Width(*EndTime) + Spacer;

        //* Draw end time of cutted recording with cutted symbol
        if (Config.PlaybackShowEndTime == 2 && FramesAfterEdit > 0) {
            const int RestCutted {FramesAfterEdit - CurrentFramesAfterEdit};
            EndTime = *TimeString(time(0) + (RestCutted / FramesPerSecond));  // HH:MM
            if (strcmp(TimeStr, EndTime) != 0) {  // Only if not equal
                img = ImgLoader.LoadIcon("recording_cutted_extra", ICON_WIDTH_UNLIMITED, GlyphSize);
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
    if (m_LastPosterBannerUpdate + 5 < CurTime) {
        m_LastPosterBannerUpdate = CurTime;
        cString MediaPath {""};
        cSize MediaSize {0, 0};
        if (Config.TVScraperReplayInfoShowPoster) {
            GetScraperMediaTypeSize(MediaPath, MediaSize, nullptr, m_Recording);
            if (isempty(*MediaPath)) {  // Prio for tvscraper poster
                const cString RecPath = m_Recording->FileName();
                if (ImgLoader.SearchRecordingPoster(RecPath, MediaPath)) {
                    img = ImgLoader.LoadFile(*MediaPath, m_TVSRect.Width(), m_TVSRect.Height());
                    if (img)
                        MediaSize.Set(img->Width(), img->Height());  // Get values for SetMediaSize()
                    else
                        MediaPath = "";  // Just in case image can not be loaded
                }
            }
        }

        PixmapClear(ChanEpgImagesPixmap);
        PixmapSetAlpha(ChanEpgImagesPixmap, 255 * Config.TVScraperPosterOpacity * 100);  // Set transparency
        DecorBorderClearByFrom(BorderTVSPoster);
        if (!isempty(*MediaPath)) {
            SetMediaSize(MediaSize, m_TVSRect.Size());  // Check for too big images
            MediaSize.SetWidth(MediaSize.Width() * Config.TVScraperReplayInfoPosterSize * 100);
            MediaSize.SetHeight(MediaSize.Height() * Config.TVScraperReplayInfoPosterSize * 100);
            img = ImgLoader.LoadFile(*MediaPath, MediaSize.Width(), MediaSize.Height());
            if (img) {
                ChanEpgImagesPixmap->DrawImage(cPoint(0, 0), *img);

                const sDecorBorder ib {20 + Config.decorBorderChannelEPGSize,
                                       m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 +
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
                           m_OsdHeight - m_LabelHeight - m_ProgressBarHeight * 2 - m_MarginItem3 -
                               m_FontHeight - Config.decorBorderReplaySize * 2,
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
    const int ImageTop {(Config.PlaybackShowEndTime > 0) ? m_FontHeight2 : m_FontHeight};

    int left {m_OsdWidth - Config.decorBorderReplaySize * 2};
    cImage *img {nullptr};
    cString IconName {""};
    if (Config.RecordingResolutionAspectShow) {  // Show Aspect (16:9)
        IconName = *GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
        img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
        if (img) {
            left -= img->Width();
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left -= m_MarginItem2;
        }

        IconName = *GetScreenResolutionIcon(m_ScreenWidth, m_ScreenHeight);  // Show Resolution (1920x1080)
        img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
        if (img) {
            left -= img->Width();
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left -= m_MarginItem2;
        }
    }

    if (Config.RecordingFormatShow && !Config.RecordingSimpleAspectFormat) {
        IconName = *GetFormatIcon(m_ScreenWidth);  // Show Format (HD)
        img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
        if (img) {
            left -= img->Width();
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left -= m_MarginItem2;
        }
    }

    if (Config.RecordingResolutionAspectShow) {  //? Add separate config option
        IconName = *GetCurrentAudioIcon();  // Show audio icon (Dolby, Stereo)
        img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
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
    ImgLoader.LoadIcon("rewind", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("pause", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("play", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("forward", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("rewind_sel", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("pause_sel", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("play_sel", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("forward_sel", m_FontHeight, m_FontHeight);

    constexpr ulong CharCode {0x0030};  // U+0030 DIGIT ZERO
    const int GlyphSize = GetGlyphSize(Setup.FontOsd, CharCode, Setup.FontOsdSize);  // Narrowing conversion
    ImgLoader.LoadIcon("recording_pos", ICON_WIDTH_UNLIMITED, GlyphSize);
    ImgLoader.LoadIcon("recording_total", ICON_WIDTH_UNLIMITED, GlyphSize);
    ImgLoader.LoadIcon("recording_cutted_extra", ICON_WIDTH_UNLIMITED, GlyphSize);

    ImgLoader.LoadIcon("recording_untested_replay", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_ok_replay", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_warning_replay", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_error_replay", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("timerRecording", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);  // Small image

    ImgLoader.LoadIcon("43", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("169", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("169w", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("221", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("7680x4320", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("3840x2160", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("1920x1080", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("1440x1080", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("1280x720", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("960x720", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("704x576", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("720x576", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("544x576", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("528x576", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("480x576", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("352x576", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("unknown_res", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("uhd", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("hd", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("sd", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("audio_stereo", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
    ImgLoader.LoadIcon("audio_dolby", ICON_WIDTH_UNLIMITED, m_FontSmlHeight);
}
