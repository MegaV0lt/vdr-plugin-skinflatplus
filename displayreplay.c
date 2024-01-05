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
    /* m_Current = "";
    m_Total = "";
    m_Recording = NULL; */

    m_ModeOnly = ModeOnly;
    // m_DimmActive = false;

    // ProgressShown = false;
    CreateFullOsd();
    TopBarCreate();
    MessageCreate();

    // m_ScreenWidth = m_LastScreenWidth = -1;

    m_TVSLeft = 20 + Config.decorBorderChannelEPGSize;
    m_TVSTop = m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize;
    m_TVSWidth = m_OsdWidth - 40 - Config.decorBorderChannelEPGSize * 2;
    m_TVSHeight = m_OsdHeight - m_TopBarHeight - m_LabelHeight - 40 - Config.decorBorderChannelEPGSize * 2;

    ChanEpgImagesPixmap = CreatePixmap(osd, "ChanEpgImagesPixmap", 2, cRect(m_TVSLeft, m_TVSTop, m_TVSWidth, m_TVSHeight));
    PixmapFill(ChanEpgImagesPixmap, clrTransparent);

    LabelPixmap = CreatePixmap(osd, "LabelPixmap", 1,
                               cRect(Config.decorBorderReplaySize,
                               m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                               m_OsdWidth - Config.decorBorderReplaySize * 2, m_LabelHeight));
    IconsPixmap = CreatePixmap(osd, "IconsPixmap", 2,
                               cRect(Config.decorBorderReplaySize,
                               m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                               m_OsdWidth - Config.decorBorderReplaySize * 2, m_LabelHeight));

    ProgressBarCreate(Config.decorBorderReplaySize,
                      m_OsdHeight - m_LabelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize
                       - m_MarginItem, m_OsdWidth - Config.decorBorderReplaySize * 2, Config.decorProgressReplaySize,
                       m_MarginItem, 0, Config.decorProgressReplayFg, Config.decorProgressReplayBarFg,
                       Config.decorProgressReplayBg, Config.decorProgressReplayType);

    LabelJumpPixmap = CreatePixmap(osd, "LabelJumpPixmap", 1, cRect(Config.decorBorderReplaySize,
        m_OsdHeight - m_LabelHeight - Config.decorProgressReplaySize * 2 - m_MarginItem*3 - m_FontHeight
         - Config.decorBorderReplaySize * 2, m_OsdWidth - Config.decorBorderReplaySize * 2, m_FontHeight));

    DimmPixmap = CreatePixmap(osd, "DimmPixmap", MAXPIXMAPLAYERS-1, cRect(0, 0, m_OsdWidth, m_OsdHeight));

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

    osd->DestroyPixmap(LabelPixmap);
    osd->DestroyPixmap(LabelJumpPixmap);
    osd->DestroyPixmap(IconsPixmap);
    osd->DestroyPixmap(ChanEpgImagesPixmap);
    osd->DestroyPixmap(DimmPixmap);
}

void cFlatDisplayReplay::SetRecording(const cRecording *Recording) {
    if (m_ModeOnly) return;
    if (!IconsPixmap || !LabelPixmap) return;

    const cRecordingInfo *RecInfo = Recording->Info();
    m_Recording = Recording;

    PixmapFill(IconsPixmap, clrTransparent);

    SetTitle(RecInfo->Title());

    // Show if still recording
    int left = m_MarginItem;  // Position for recordingsymbol/shorttext/date
    cImage *img {nullptr};
    if ((m_Recording->IsInUse() & ruTimer) != 0) {  // The recording is currently written to by a timer
        img = ImgLoader.LoadIcon("timerRecording", 999, m_FontSmlHeight);  // Small image

        if (img) {
            int ImageTop = m_FontHeight + (m_FontSmlHeight - img->Height()) / 2;
            IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left += img->Width() + m_MarginItem;
        }
    }

    cString InfoText("");
    if (RecInfo->ShortText()) {
        if (Config.PlaybackShowRecordingDate)  // Date Time - ShortText
            InfoText = cString::sprintf("%s  %s - %s", *ShortDateString(m_Recording->Start()),
                                    *TimeString(m_Recording->Start()), RecInfo->ShortText());
        else
            InfoText = cString::sprintf("%s", RecInfo->ShortText());
    } else {  // No shorttext
        InfoText = cString::sprintf("%s  %s", *ShortDateString(m_Recording->Start()),
                                    *TimeString(m_Recording->Start()));
    }

    int InfoWidth = m_FontSml->Width(*InfoText);  // Width of infotext
    // TODO: How to get width of aspect and format icons?
    //  Done: Substract 'left' in case of displayed recording icon
    //  Done: Substract 'm_FontSmlHeight' in case of recordingerror icon is displayed later
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
                                              m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize + m_FontHeight,
                                              MaxWidth, m_FontSmlHeight),
                                        Theme.Color(clrMenuEventFontInfo), clrTransparent, m_FontSml);
            left += MaxWidth;
        } else {  // Add ... if info ist too long
            dsyslog("flatPlus: Shorttext too long! (%d) Setting MaxWidth to %d", InfoWidth, MaxWidth);
            int DotsWidth = m_FontSml->Width("...");
            LabelPixmap->DrawText(cPoint(left, m_FontHeight), *InfoText, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_FontSml, MaxWidth - DotsWidth);
            left += (MaxWidth - DotsWidth);
            LabelPixmap->DrawText(cPoint(left, m_FontHeight), "...", Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_FontSml, DotsWidth);
            left += DotsWidth;
        }
    } else {  // Shorttext fits into maxwidth
        LabelPixmap->DrawText(cPoint(left, m_FontHeight), *InfoText, Theme.Color(clrReplayFont),
                                     Theme.Color(clrReplayBg), m_FontSml, InfoWidth);
        left += InfoWidth;
    }

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors) {  // Separate config option
        cString ErrIcon = GetRecordingerrorIcon(RecInfo->Errors());
        cString RecErrIcon = cString::sprintf("%s_replay", *ErrIcon);

        img = ImgLoader.LoadIcon(*RecErrIcon, 999, m_FontSmlHeight);  // Small image
        if (img) {
            left += m_MarginItem;
            int ImageTop = m_FontHeight + (m_FontSmlHeight - img->Height()) / 2;
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
        left = m_OsdWidth - Config.decorBorderReplaySize * 2 - (m_FontHeight * 4 + m_MarginItem * 3);
        left /= 2;

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
            cString speed = cString::sprintf("%d", Speed);
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
            IconsPixmap->DrawImage(cPoint(left + m_FontHeight * 2 + m_MarginItem * 2, 0), *img);

        img = ImgLoader.LoadIcon(*forward, m_FontHeight, m_FontHeight);
        if (img)
            IconsPixmap->DrawImage(cPoint(left + m_FontHeight * 3 + m_MarginItem * 3, 0), *img);
    }

    if (ProgressShown) {
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

    ProgressShown = true;
    ProgressBarDrawMarks(Current, Total, marks, Theme.Color(clrReplayMarkFg), Theme.Color(clrReplayMarkCurrentFg));
}

void cFlatDisplayReplay::SetCurrent(const char *Current) {
    if (m_ModeOnly) return;

    m_Current = Current;
    UpdateInfo();
}

void cFlatDisplayReplay::SetTotal(const char *Total) {
    if (m_ModeOnly) return;

    m_Total = Total;
    UpdateInfo();
}

void cFlatDisplayReplay::UpdateInfo(void) {
    if (m_ModeOnly) return;
    if (!LabelPixmap || !ChanEpgImagesPixmap || !IconsPixmap) return;

    int FontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);
    int FontSecsAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);
    int TopSecs = FontAscender - FontSecsAscender;

    if (Config.TimeSecsScale == 1.0)
        LabelPixmap->DrawText(cPoint(m_MarginItem, 0), *m_Current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              m_Font, m_Font->Width(*m_Current), m_FontHeight);
    else {
        std::string cur = *m_Current;
        std::size_t found = cur.find_last_of(':');
        if (found != std::string::npos) {
            std::string hm = cur.substr(0, found);
            std::string secs = cur.substr(found, cur.length() - found);
            secs.append(1, ' ');  // Fix for extra pixel glitch

            LabelPixmap->DrawText(cPoint(m_MarginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHeight);
            LabelPixmap->DrawText(cPoint(m_MarginItem + m_Font->Width(hm.c_str()), TopSecs), secs.c_str(),
                                  Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                  m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
        } else {
            LabelPixmap->DrawText(cPoint(m_MarginItem, 0), *m_Current, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(*m_Current), m_FontHeight);
        }
    }

    cImage *img {nullptr};
    cString cutted(""), Dummy("");
    bool IsCutted = false;
    if (m_Recording) {
        IsCutted = GetCuttedLengthMarks(m_Recording, Dummy, cutted, false);  // Process marks and get cutted time

        cString MediaPath("");
        int MediaWidth {0}, MediaHeight {0};
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
                    if (series.banners.size() > 0) {  // Use random banner
                        // Gets 'entropy' from device that generates random numbers itself
                        // to seed a mersenne twister (pseudo) random generator
                        std::mt19937 generator(std::random_device {}());

                        // Make sure all numbers have an equal chance.
                        // Range is inclusive (so we need -1 for vector index)
                        std::uniform_int_distribution<std::size_t> distribution(0, series.banners.size() - 1);

                        std::size_t number = distribution(generator);

                        MediaPath = series.banners[number].path.c_str();
                        MediaWidth = series.banners[number].width * Config.TVScraperReplayInfoPosterSize * 100;
                        MediaHeight = series.banners[number].height * Config.TVScraperReplayInfoPosterSize * 100;
                        if (series.banners.size() > 1)
                            dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                                    static_cast<int>(number + 1), *MediaPath,
                                    static_cast<int>(series.banners.size()));  // Log result
                    }
                }
            } else if (call.type == tMovie) {
                cMovie movie;
                movie.movieId = movieId;
                if (pScraper->Service("GetMovie", &movie)) {
                    MediaPath = movie.poster.path.c_str();
                    MediaWidth = movie.poster.width * Config.TVScraperReplayInfoPosterSize * 100;
                    MediaHeight = movie.poster.height * Config.TVScraperReplayInfoPosterSize * 100;
                }
            }
        }

        PixmapFill(ChanEpgImagesPixmap, clrTransparent);
        DecorBorderClearByFrom(BorderTVSPoster);
        if (!isempty(*MediaPath)) {
            if (MediaHeight > m_TVSHeight || MediaWidth > m_TVSWidth) {  // Resize too big poster/banner
                dsyslog("flatPlus: Poster/Banner size (%d x %d) is too big!", MediaWidth, MediaHeight);
                MediaHeight = m_TVSHeight * 0.7 * Config.TVScraperReplayInfoPosterSize * 100;  // Max 70% of pixmap height
                if (Config.ChannelWeatherShow)
                    // Max 50% of pixmap width. Aspect is preserved in LoadFile()
                    MediaWidth = m_TVSWidth * 0.5 * Config.TVScraperReplayInfoPosterSize * 100;
                else
                    // Max 70% of pixmap width. Aspect is preserved in LoadFile()
                    MediaWidth = m_TVSWidth * 0.7 * Config.TVScraperReplayInfoPosterSize * 100;

                dsyslog("flatPlus: Poster/Banner resized to max %d x %d", MediaWidth, MediaHeight);
            }
            img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);
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

    if (IsCutted) {
        img = ImgLoader.LoadIcon("recording_cutted_extra", m_FontHeight, m_FontHeight);
        int imgWidth {0};
        if (img)
            imgWidth = img->Width();

        int right = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(m_Total) - m_MarginItem - imgWidth -
                    m_Font->Width(' ') - m_Font->Width(cutted);
        if (Config.TimeSecsScale < 1.0) {
            std::string tot = *m_Total;
            std::size_t found = tot.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = tot.substr(0, found);
                std::string secs = tot.substr(found, tot.length() - found);

                std::string cutt = *cutted;
                std::size_t found2 = cutt.find_last_of(':');
                if (found2 != std::string::npos) {
                    std::string hm2 = cutt.substr(0, found);
                    std::string secs2 = cutt.substr(found, cutt.length() - found);

                    right = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(hm.c_str()) -
                            m_FontSecs->Width(secs.c_str()) - m_MarginItem - imgWidth - m_Font->Width(' ') -
                            m_Font->Width(hm2.c_str()) - m_FontSecs->Width(secs2.c_str());
                } else
                    right = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(hm.c_str()) -
                            m_FontSecs->Width(secs.c_str()) - m_MarginItem - imgWidth - m_Font->Width(' ') -
                            m_Font->Width(cutted);

                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHeight);
                LabelPixmap->DrawText(cPoint(right - m_MarginItem + m_Font->Width(hm.c_str()), TopSecs), secs.c_str(),
                                      Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
                right += m_Font->Width(hm.c_str()) + m_FontSecs->Width(secs.c_str());
                right += m_Font->Width(' ');
            } else {
                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), m_Total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
                right += m_Font->Width(m_Total);
                right += m_Font->Width(' ');
            }
        } else {
            LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), m_Total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
            right += m_Font->Width(m_Total);
            right += m_Font->Width(' ');
        }

        if (img) {  // Draw 'recording_cuttet_extra' icon
            IconsPixmap->DrawImage(cPoint(right, 0), *img);
            right += img->Width() + m_MarginItem * 2;
        }

        if (Config.TimeSecsScale < 1.0) {
            std::string cutt = *cutted;
            std::size_t found = cutt.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = cutt.substr(0, found);
                std::string secs = cutt.substr(found, cutt.length() - found);

                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), hm.c_str(),
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), m_Font,
                                      m_Font->Width(hm.c_str()), m_FontHeight);
                LabelPixmap->DrawText(cPoint(right - m_MarginItem + m_Font->Width(hm.c_str()), TopSecs), secs.c_str(),
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
            } else {
                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(cutted), m_FontHeight);
            }
        } else {
            LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(cutted), m_FontHeight);
        }
    } else {  // Not cutted
        int right = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(m_Total);
        if (Config.TimeSecsScale < 1.0) {
            std::string tot = *m_Total;
            std::size_t found = tot.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = tot.substr(0, found);
                std::string secs = tot.substr(found, tot.length() - found);

                right = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(hm.c_str()) -
                        m_FontSecs->Width(secs.c_str());
                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHeight);
                LabelPixmap->DrawText(cPoint(right - m_MarginItem + m_Font->Width(hm.c_str()), TopSecs), secs.c_str(),
                                      Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
            } else {
                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), *m_Total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
            }
        } else {
            LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), *m_Total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(m_Total), m_FontHeight);
        }
    }  // IsCutted
}

void cFlatDisplayReplay::SetJump(const char *Jump) {
    if (!LabelJumpPixmap) return;

    DecorBorderClearByFrom(BorderRecordJump);

    if (!Jump) {
        PixmapFill(LabelJumpPixmap, clrTransparent);
        return;
    }
    int left = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(Jump);
    left /= 2;

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
        int left = m_OsdWidth - Config.decorBorderReplaySize * 2;
        int ImageTop {0};
        cImage *img {nullptr};
        cString IconName("");
        if (Config.RecordingResolutionAspectShow) {  // Show Aspect
            IconName = GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                ImageTop = m_FontHeight + (m_FontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem * 2;
            }

            IconName = GetScreenResolutionIcon(m_ScreenWidth, m_ScreenHeight, m_ScreenAspect);  // Show Resolution
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                ImageTop = m_FontHeight + (m_FontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem * 2;
            }
        }

        if (Config.RecordingFormatShow && !Config.RecordingSimpleAspectFormat) {
            IconName = GetFormatIcon(m_ScreenWidth);  // Show Format
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                ImageTop = m_FontHeight + (m_FontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                IconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem * 2;
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

    osd->Flush();
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
