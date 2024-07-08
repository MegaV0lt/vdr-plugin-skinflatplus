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

    m_TVSLeft = 20 + Config.decorBorderChannelEPGSize;
    m_TVSTop = m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize;
    m_TVSWidth = m_OsdWidth - 40 - Config.decorBorderChannelEPGSize * 2;
    m_TVSHeight = m_OsdHeight - m_TopBarHeight - m_LabelHeight - 40 - Config.decorBorderChannelEPGSize * 2;

    ChanEpgImagesPixmap =
        CreatePixmap(m_Osd, "ChanEpgImagesPixmap", 2, cRect(m_TVSLeft, m_TVSTop, m_TVSWidth, m_TVSHeight));
    PixmapFill(ChanEpgImagesPixmap, clrTransparent);

    LabelPixmap = CreatePixmap(m_Osd, "LabelPixmap", 1,
                               cRect(Config.decorBorderReplaySize,
                               m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                               m_OsdWidth - Config.decorBorderReplaySize * 2, m_LabelHeight));
    IconsPixmap = CreatePixmap(m_Osd, "IconsPixmap", 2,
                               cRect(Config.decorBorderReplaySize,
                               m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                               m_OsdWidth - Config.decorBorderReplaySize * 2, m_LabelHeight));

    ProgressBarCreate(cRect(Config.decorBorderReplaySize,
                            m_OsdHeight - m_LabelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize
                            - m_MarginItem,
                            m_OsdWidth - Config.decorBorderReplaySize * 2, Config.decorProgressReplaySize),
                      m_MarginItem, 0, Config.decorProgressReplayFg, Config.decorProgressReplayBarFg,
                      Config.decorProgressReplayBg, Config.decorProgressReplayType);

    LabelJumpPixmap = CreatePixmap(m_Osd, "LabelJumpPixmap", 1, cRect(Config.decorBorderReplaySize,
        m_OsdHeight - m_LabelHeight - Config.decorProgressReplaySize * 2 - m_MarginItem * 3 - m_FontHeight
         - Config.decorBorderReplaySize * 2, m_OsdWidth - Config.decorBorderReplaySize * 2, m_FontHeight));

    DimmPixmap = CreatePixmap(m_Osd, "DimmPixmap", MAXPIXMAPLAYERS-1, cRect(0, 0, m_OsdWidth, m_OsdHeight));

    PixmapFill(LabelPixmap, Theme.Color(clrReplayBg));
    PixmapFill(LabelJumpPixmap, clrTransparent);
    PixmapFill(IconsPixmap, clrTransparent);
    PixmapFill(DimmPixmap, clrTransparent);

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
    if (m_ModeOnly) return;
    if (!IconsPixmap || !LabelPixmap) return;

    const cRecordingInfo *RecInfo = Recording->Info();
    m_Recording = Recording;

    PixmapFill(IconsPixmap, clrTransparent);

    SetTitle(RecInfo->Title());

    // First line for current, total and cutted length, second for end time
    const int SmallTop = (Config.PlaybackShowEndTime > 0) ? m_FontHeight2 : m_FontHeight;

    // Show if still recording
    int left = m_MarginItem;  // Position for recording symbol/short text/date
    cImage *img {nullptr};
    if ((Recording->IsInUse() & ruTimer) != 0) {  // The recording is currently written to by a timer
        img = ImgLoader.LoadIcon("timerRecording", 999, m_FontSmlHeight);  // Small image

        if (img) {
            int ImageTop = SmallTop + (m_FontSmlHeight - img->Height()) / 2;
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left += img->Width() + m_MarginItem;
        }
    }

    cString InfoText {""};
    if (RecInfo->ShortText()) {
        if (Config.PlaybackShowRecordingDate)  // Date Time - ShortText
            InfoText = cString::sprintf("%s  %s - %s", *ShortDateString(Recording->Start()),
                                    *TimeString(Recording->Start()), RecInfo->ShortText());
        else
            InfoText = cString::sprintf("%s", RecInfo->ShortText());
    } else {  // No short text
        InfoText = cString::sprintf("%s  %s", *ShortDateString(Recording->Start()),
                                    *TimeString(Recording->Start()));
    }

    const int InfoWidth = m_FontSml->Width(*InfoText);  // Width of infotext
    // TODO: How to get width of aspect and format icons?
    //  Done: Substract 'left' in case of displayed recording icon
    //  Done: Substract 'm_FontSmlHeight' in case of recording error icon is displayed later
    //* Workaround: Substract width of aspect and format icons (ResolutionAspectDraw()) ???
    int MaxWidth = m_OsdWidth - left - Config.decorBorderReplaySize * 2;

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors)
        MaxWidth -= m_FontSmlHeight;  // Substract width of imgRecErr
#endif

    img = ImgLoader.LoadIcon("1920x1080", 999, m_FontSmlHeight);
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
            const int DotsWidth = m_FontSml->Width("...");
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
        const cString RecErrIcon = cString::sprintf("%s_replay", *GetRecordingerrorIcon(RecInfo->Errors()));

        img = ImgLoader.LoadIcon(*RecErrIcon, 999, m_FontSmlHeight);  // Small image
        if (img) {
            left += m_MarginItem;
            const int ImageTop = SmallTop + (m_FontSmlHeight - img->Height()) / 2;
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
        }
    }  // PlaybackShowRecordingErrors
#endif
}

void cFlatDisplayReplay::SetTitle(const char *Title) {
    TopBarSetTitle(Title);
    TopBarSetMenuIcon("extraIcons/Playing");
}

void cFlatDisplayReplay::Action(void) {
    time_t CurTime;
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
    if (!LabelPixmap || !IconsPixmap) return;

    int left {0};
    if (Play == false && Config.RecordingDimmOnPause) {
        time(&m_DimmStartTime);
        Start();
    } else if (Play == true && Config.RecordingDimmOnPause) {
        Cancel(-1);
        while (Active())
            cCondWait::SleepMs(10);
        if (m_DimmActive) {
            PixmapFill(DimmPixmap, clrTransparent);
            Flush();
        }
    }
    if (Setup.ShowReplayMode) {
        left = (m_OsdWidth - Config.decorBorderReplaySize * 2 - (m_FontHeight * 4 + m_MarginItem * 3)) / 2;

        if (m_ModeOnly)
            PixmapFill(LabelPixmap, clrTransparent);

        // PixmapFill(IconsPixmap, clrTransparent);  //* Moved to SetRecording
        LabelPixmap->DrawRectangle(cRect(left - m_Font->Width("99") - m_MarginItem, 0,
                                         m_FontHeight * 4 + m_MarginItem * 6 + m_Font->Width("99") * 2, m_FontHeight),
                                   Theme.Color(clrReplayBg));

        cString rewind("rewind"), pause("pause"), play("play"), forward("forward");
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
        cImage *img = ImgLoader.LoadIcon(*rewind, m_FontHeight, m_FontHeight);
        if (img)
            IconsPixmap->DrawImage(cPoint(left, 0), *img);

        img = ImgLoader.LoadIcon(*pause, m_FontHeight, m_FontHeight);
        if (img)
            IconsPixmap->DrawImage(cPoint(left + m_FontHeight + m_MarginItem, 0), *img);

        img = ImgLoader.LoadIcon(*play, m_FontHeight, m_FontHeight);
        if (img)
            IconsPixmap->DrawImage(cPoint(left + m_FontHeight2 + m_MarginItem2, 0), *img);

        img = ImgLoader.LoadIcon(*forward, m_FontHeight, m_FontHeight);
        if (img)
            IconsPixmap->DrawImage(cPoint(left + m_FontHeight * 3 + m_MarginItem * 3, 0), *img);
    }

    if (m_ProgressShown) {
        DecorBorderDraw(Config.decorBorderReplaySize,
                        m_OsdHeight - m_LabelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize -
                            m_MarginItem,
                        m_OsdWidth - Config.decorBorderReplaySize * 2,
                        m_LabelHeight + Config.decorProgressReplaySize + m_MarginItem, Config.decorBorderReplaySize,
                        Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
    } else {
        if (m_ModeOnly) {
            DecorBorderDraw(left - m_Font->Width("99") - m_MarginItem + Config.decorBorderReplaySize,
                            m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                            m_FontHeight * 4 + m_MarginItem * 6 + m_Font->Width("99") * 2, m_FontHeight,
                            Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg,
                            Config.decorBorderReplayBg);
        } else {
            DecorBorderDraw(Config.decorBorderReplaySize, m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                            m_OsdWidth - Config.decorBorderReplaySize * 2, m_LabelHeight, Config.decorBorderReplaySize,
                            Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
        }
    }

    ResolutionAspectDraw();
}

void cFlatDisplayReplay::SetProgress(int Current, int Total) {
    if (m_DimmActive) {
        PixmapFill(DimmPixmap, clrTransparent);
        Flush();
    }

    if (m_ModeOnly) return;
    m_CurrentFrame = Current;

    m_ProgressShown = true;
    ProgressBarDrawMarks(Current, Total, marks, Theme.Color(clrReplayMarkFg), Theme.Color(clrReplayMarkCurrentFg));
}

void cFlatDisplayReplay::SetCurrent(const char *Current) {
    if (m_ModeOnly) return;

    m_Current = Current;
    UpdateInfo();
}

void cFlatDisplayReplay::SetTotal(const char *Total) {
    if (m_ModeOnly) return;

    // dsyslog("flatPlus: SetTotal() %s", Total);
    m_Total = Total;
    if (!isempty(*m_Current))  // Do not call 'UpdateInfo()' when 'm_Current' is not set
        UpdateInfo();
}

void cFlatDisplayReplay::UpdateInfo(void) {
    #ifdef DEBUGFUNCSCALL
        dsyslog("flatPlus: cFlatDisplayReplay::UpdateInfo()");
    #endif

    if (m_ModeOnly) return;
    if (!LabelPixmap || !ChanEpgImagesPixmap || !IconsPixmap) return;

    const int FontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);
    const int FontSecsAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);
    const int TopSecs = FontAscender - FontSecsAscender;

    //* Draw current position with symbol
    int left {m_MarginItem};
    cImage *img = ImgLoader.LoadIcon("recording_pos", m_FontHeight, m_FontHeight);
    if (img) {
        IconsPixmap->DrawImage(cPoint(left, 0), *img);
        left += img->Width() + m_MarginItem;
    }

    if (Config.TimeSecsScale == 1.0) {
        LabelPixmap->DrawText(cPoint(left, 0), *m_Current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              m_Font, m_Font->Width(*m_Current), m_FontHeight);
        left += m_Font->Width(*m_Current);
    } else {
        std::string_view cur {*m_Current};
        const std::size_t found = cur.find_last_of(':');
        if (found != std::string::npos) {
            const std::string hm {cur.substr(0, found)};
            std::string secs {cur.substr(found, cur.length() - found)};
            secs.append(1, ' ');  // Fix for extra pixel glitch

            LabelPixmap->DrawText(cPoint(left, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHeight);
            left += m_Font->Width(hm.c_str());
            LabelPixmap->DrawText(cPoint(left, TopSecs), secs.c_str(),
                                  Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                  m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
            left += m_FontSecs->Width(secs.c_str());
        } else {
            LabelPixmap->DrawText(cPoint(left, 0), *m_Current, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(*m_Current), m_FontHeight);
            left += m_Font->Width(*m_Current);
        }
    }

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

    const int FontWidthSpace = m_Font->Width(' ');
    const double FramesPerSecond{m_Recording->FramesPerSecond()};

    //* Draw end time of recording with symbol for cutted end time
    if (Config.PlaybackShowEndTime > 0) {  // 1 = End time, 2 = End time and cutted end time
        left = m_MarginItem;
        /* img = ImgLoader.LoadIcon("recording_finish", m_FontHeight, m_FontHeight);
        if (img) {
            IconsPixmap->DrawImage(cPoint(left, m_FontHeight), *img);
            left += img->Width() + m_MarginItem;
        } */

        const int Rest = NumFrames - m_CurrentFrame;
        cString EndTime = cString::sprintf("%s: %s", tr("ends at"), *TimeString(time(0) + (Rest / FramesPerSecond)));
        LabelPixmap->DrawText(cPoint(left, m_FontHeight), *EndTime, Theme.Color(clrReplayFont),
                              Theme.Color(clrReplayBg), m_Font, m_Font->Width(*EndTime), m_FontHeight);
        left += m_Font->Width(*EndTime) + FontWidthSpace;

        //* Draw end time of cutted recording with cutted symbol
        if (Config.PlaybackShowEndTime == 2 && FramesAfterEdit >= 0) {
            // if (CurrentFramesAfterEdit >= FramesAfterEdit) {  // Only if greater than '00:00'
                img = ImgLoader.LoadIcon("recording_cutted_extra", m_FontHeight, m_FontHeight);
                if (img) {
                    IconsPixmap->DrawImage(cPoint(left, m_FontHeight), *img);
                    left += img->Width() + m_MarginItem;
                }

                const int RestCutted = FramesAfterEdit - CurrentFramesAfterEdit;
                EndTime = cString::sprintf("%s", *TimeString(time(0) + (RestCutted / FramesPerSecond)));
                LabelPixmap->DrawText(cPoint(left, m_FontHeight), *EndTime, Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(*EndTime), m_FontHeight);
                // left += m_Font->Width(*EndTime) + m_MarginItem;  //* 'left' is no used anymore from here
            // }
        }
    }  // Config.PlaybackShowEndTime

    //* Draw total and cutted length with cutted symbol (Right side)
    cImage *ImgTotal = ImgLoader.LoadIcon("recording_total", m_FontHeight, m_FontHeight);
    const int ImgTotalWidth = (ImgTotal) ? ImgTotal->Width() : 0;
    if (/*m_Recording->HasMarks()*/ FramesAfterEdit > 0) {
        const cString cutted = cString::sprintf("%s", *IndexToHMSF(FramesAfterEdit, false, FramesPerSecond));
        cImage *ImgCutted = ImgLoader.LoadIcon("recording_cutted_extra", m_FontHeight, m_FontHeight);
        const int ImgCuttedWidth = (ImgCutted) ? ImgCutted->Width() : 0;

        int right = m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgTotalWidth - m_MarginItem -
                    m_Font->Width(m_Total) - FontWidthSpace - ImgCuttedWidth - m_MarginItem - m_Font->Width(cutted);

        if (Config.TimeSecsScale < 1.0) {
            std::string_view tot {*m_Total};  // Total length
            const std::size_t found = tot.find_last_of(':');
            if (found != std::string::npos) {
                const std::string hm {tot.substr(0, found)};
                const std::string secs {tot.substr(found, tot.length() - found)};

                std::string_view cutt {*cutted};  // Cutted length
                const std::size_t found2 = cutt.find_last_of(':');
                if (found2 != std::string::npos) {
                    const std::string hm2 {cutt.substr(0, found)};
                    const std::string secs2 {cutt.substr(found, cutt.length() - found)};

                    right = m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgTotalWidth - m_MarginItem -
                            m_Font->Width(hm.c_str()) - m_FontSecs->Width(secs.c_str()) - FontWidthSpace -
                            ImgCuttedWidth - m_MarginItem - m_Font->Width(hm2.c_str()) -
                            m_FontSecs->Width(secs2.c_str());
                } else {
                    right = m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgTotalWidth - m_MarginItem -
                            m_Font->Width(hm.c_str()) - m_FontSecs->Width(secs.c_str()) - FontWidthSpace -
                            ImgCuttedWidth - m_MarginItem - m_Font->Width(cutted);
                }

                if (ImgTotal) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, 0), *ImgTotal);
                    right += ImgTotalWidth + m_MarginItem;  // 'ImgTotalWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHeight);
                LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/ + m_Font->Width(hm.c_str()), TopSecs),
                                      secs.c_str(), Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
                right += m_Font->Width(hm.c_str()) + m_FontSecs->Width(secs.c_str()) + FontWidthSpace;
            } else {
                if (ImgTotal) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, 0), *ImgTotal);
                    right += ImgTotalWidth + m_MarginItem;  // 'ImgTotalWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/, 0), m_Total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
                right += m_Font->Width(m_Total) + FontWidthSpace;
            }
        } else {
            if (ImgTotal) {  // Draw preloaded 'recording_total' icon
                IconsPixmap->DrawImage(cPoint(right, 0), *ImgTotal);
                right += ImgTotalWidth + m_MarginItem;  // 'ImgTotalWidth' is already set
            }
            LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/, 0), m_Total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
            right += m_Font->Width(m_Total) + FontWidthSpace;
        }  // Config.TimeSecsScale < 1.0

        if (ImgCutted) {  // Draw preloaded 'recording_cutted_extra' icon
            IconsPixmap->DrawImage(cPoint(right, 0), *ImgCutted);
            right += ImgCuttedWidth + m_MarginItem;  // 'ImgCuttedWidth' is already set
        }

        if (Config.TimeSecsScale < 1.0) {
            std::string_view cutt{*cutted};
            const std::size_t found = cutt.find_last_of(':');
            if (found != std::string::npos) {
                const std::string hm{cutt.substr(0, found)};
                const std::string secs{cutt.substr(found, cutt.length() - found)};

                LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/, 0), hm.c_str(),
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), m_Font,
                                      m_Font->Width(hm.c_str()), m_FontHeight);
                LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/ + m_Font->Width(hm.c_str()), TopSecs),
                                      secs.c_str(), Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg),
                                      m_FontSecs, m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
            } else {
                LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/, 0), *cutted,
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), m_Font,
                                      m_Font->Width(cutted), m_FontHeight);
            }
        } else {
            LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(cutted), m_FontHeight);
        }
    } else {  // Not cutted
        int right =
            m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgTotalWidth - m_MarginItem - m_Font->Width(m_Total);
        if (Config.TimeSecsScale < 1.0) {
            std::string_view tot{*m_Total};
            const std::size_t found = tot.find_last_of(':');
            if (found != std::string::npos) {
                const std::string hm{tot.substr(0, found)};
                const std::string secs{tot.substr(found, tot.length() - found)};

                right = m_OsdWidth - Config.decorBorderReplaySize * 2 - ImgTotalWidth - m_MarginItem -
                        m_Font->Width(hm.c_str()) - m_FontSecs->Width(secs.c_str());
                if (ImgTotal) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, 0), *ImgTotal);
                    right += ImgTotalWidth + m_MarginItem;  // 'ImgTotalWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHeight);
                LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/ + m_Font->Width(hm.c_str()), TopSecs),
                                      secs.c_str(), Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
            } else {
                if (ImgTotal) {  // Draw preloaded 'recording_total' icon
                    IconsPixmap->DrawImage(cPoint(right, 0), *ImgTotal);
                    right += ImgTotalWidth + m_MarginItem;  // 'ImgTotalWidth' is already set
                }
                LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/, 0), *m_Total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
            }
        } else {
            if (ImgTotal) {  // Draw preloaded 'recording_total' icon
                IconsPixmap->DrawImage(cPoint(right, 0), *ImgTotal);
                right += ImgTotalWidth + m_MarginItem;  // 'ImgTotalWidth' is already set
            }
            LabelPixmap->DrawText(cPoint(right /*- m_MarginItem*/, 0), *m_Total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
        }
    }  // HasMarks

    //* Draw Banner/Poster
    if (m_Recording) {
        cString MediaPath {""};
        cSize MediaSize {0, 0};  // Width, Height
        static cPlugin *pScraper = GetScraperPlugin();
        if (Config.TVScraperReplayInfoShowPoster && pScraper) {
            ScraperGetEventType call;
            call.recording = m_Recording;
            int seriesId {0};
            int episodeId {0};
            int movieId {0};

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
                    if (series.banners.size() > 1) {  // Use random banner
                        // Gets 'entropy' from device that generates random numbers itself
                        // to seed a mersenne twister (pseudo) random generator
                        std::mt19937 generator(std::random_device {}());

                        // Make sure all numbers have an equal chance.
                        // Range is inclusive (so we need -1 for vector index)
                        std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                        const std::size_t number = distribution(generator);

                        MediaPath = series.banners[number].path.c_str();
                        MediaSize.Set(series.banners[number].width, series.banners[number].height);
                        dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                                static_cast<int>(number + 1), *MediaPath,
                                static_cast<int>(series.banners.size()));  // Log result
                    } else if (series.banners.size() == 1) {               // Just one banner
                        MediaPath = series.banners[0].path.c_str();
                        MediaSize.Set(series.banners[0].width, series.banners[0].height);
                    }
                }
            } else if (call.type == tMovie) {
                cMovie movie;
                movie.movieId = movieId;
                if (pScraper->Service("GetMovie", &movie)) {
                    MediaPath = movie.poster.path.c_str();
                    MediaSize.Set(movie.poster.width, movie.poster.height);
                }
            }

            if (isempty(*MediaPath)) {  // Prio for tvscraper poster
                const cString RecPath = cString::sprintf("%s", m_Recording->FileName());
                cString RecImage {""};
                if (ImgLoader.SearchRecordingPoster(*RecPath, RecImage)) {
                    MediaPath = RecImage;
                    img = ImgLoader.LoadFile(*MediaPath, m_TVSWidth, m_TVSHeight);
                    if (img)
                        MediaSize.Set(img->Width(), img->Height());  // Get values for SetMediaSize()
                    else
                        MediaPath = "";  // Just in case image can not be loaded
                }
            }
        }

        PixmapFill(ChanEpgImagesPixmap, clrTransparent);
        PixmapSetAlpha(ChanEpgImagesPixmap, 255 * Config.TVScraperPosterOpacity * 100);  // Set transparency
        DecorBorderClearByFrom(BorderTVSPoster);
        if (!isempty(*MediaPath)) {
            SetMediaSize(MediaSize, cSize(m_TVSWidth, m_TVSHeight));  // Check for too big images
            MediaSize.SetWidth(MediaSize.Width() * Config.TVScraperReplayInfoPosterSize * 100);
            MediaSize.SetHeight(MediaSize.Height() * Config.TVScraperReplayInfoPosterSize * 100);
            img = ImgLoader.LoadFile(*MediaPath, MediaSize.Width(), MediaSize.Height());
            if (img) {
                ChanEpgImagesPixmap->DrawImage(cPoint(0, 0), *img);

                DecorBorderDraw(
                    20 + Config.decorBorderChannelEPGSize,
                    m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize,
                    img->Width(), img->Height(), Config.decorBorderChannelEPGSize, Config.decorBorderChannelEPGType,
                    Config.decorBorderChannelEPGFg, Config.decorBorderChannelEPGBg, BorderTVSPoster);
            }
        }
    }
}

void cFlatDisplayReplay::SetJump(const char *Jump) {
    if (!LabelJumpPixmap) return;

    DecorBorderClearByFrom(BorderRecordJump);

    if (!Jump) {
        PixmapFill(LabelJumpPixmap, clrTransparent);
        return;
    }
    const int left = (m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(Jump)) / 2;

    LabelJumpPixmap->DrawText(cPoint(left, 0), Jump, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_Font,
                              m_Font->Width(Jump), m_FontHeight, taCenter);

    DecorBorderDraw(left + Config.decorBorderReplaySize,
                    m_OsdHeight - m_LabelHeight - Config.decorProgressReplaySize * 2 - m_MarginItem * 3 - m_FontHeight -
                        Config.decorBorderReplaySize * 2,
                    m_Font->Width(Jump), m_FontHeight, Config.decorBorderReplaySize, Config.decorBorderReplayType,
                    Config.decorBorderReplayFg, Config.decorBorderReplayBg, BorderRecordJump);
}

void cFlatDisplayReplay::ResolutionAspectDraw(void) {
    if (m_ModeOnly) return;
    if (!IconsPixmap) return;

    if (m_ScreenWidth > 0) {
        // First line for current, total and cutted length, second line for end time
        const int SmallTop = (Config.PlaybackShowEndTime > 0) ? m_FontHeight2 : m_FontHeight;

        int left = m_OsdWidth - Config.decorBorderReplaySize * 2;
        int ImageTop {0};
        cImage *img {nullptr};
        cString IconName {""};
        if (Config.RecordingResolutionAspectShow) {  // Show Aspect
            IconName = GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                ImageTop = SmallTop + (m_FontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem2;
            }

            IconName = GetScreenResolutionIcon(m_ScreenWidth, m_ScreenHeight, m_ScreenAspect);  // Show Resolution
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                ImageTop = SmallTop + (m_FontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem2;
            }
        }

        if (Config.RecordingFormatShow && !Config.RecordingSimpleAspectFormat) {
            IconName = GetFormatIcon(m_ScreenWidth);  // Show Format
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                ImageTop = SmallTop + (m_FontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem2;
            }
        }
    }
}

void cFlatDisplayReplay::SetMessage(eMessageType Type, const char *Text) {
    (Text) ? MessageSet(Type, Text) : MessageClear();
}

void cFlatDisplayReplay::Flush(void) {
    TopBarUpdate();

    if (Config.RecordingResolutionAspectShow) {
        cDevice::PrimaryDevice()->GetVideoSize(m_ScreenWidth, m_ScreenHeight, m_ScreenAspect);
        if (m_ScreenWidth != m_LastScreenWidth) {
            m_LastScreenWidth = m_ScreenWidth;
            ResolutionAspectDraw();
        }
    }

    m_Osd->Flush();
}

void cFlatDisplayReplay::PreLoadImages(void) {
    ImgLoader.LoadIcon("rewind", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("pause", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("play", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("forward", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("rewind_sel", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("pause_sel", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("play_sel", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("forward_sel", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("recording_cutted_extra", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("recording_pos", m_FontHeight, m_FontHeight);
    ImgLoader.LoadIcon("recording_total", m_FontHeight, m_FontHeight);

    ImgLoader.LoadIcon("recording_untested_replay", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_ok_replay", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_warning_replay", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("recording_error_replay", 999, m_FontSmlHeight);

    ImgLoader.LoadIcon("43", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("169", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("169w", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("221", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("7680x4320", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("3840x2160", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("1920x1080", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("1440x1080", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("1280x720", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("960x720", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("704x576", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("720x576", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("544x576", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("528x576", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("480x576", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("352x576", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("unknown_res", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("uhd", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("hd", 999, m_FontSmlHeight);
    ImgLoader.LoadIcon("sd", 999, m_FontSmlHeight);
}
