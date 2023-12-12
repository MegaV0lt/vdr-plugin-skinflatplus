#include "displayreplay.h"
#include "flat.h"

cFlatDisplayReplay::cFlatDisplayReplay(bool ModeOnly) {
    m_LabelHeight = m_FontHight + m_FontSmlHight;
    current = "";
    total = "";
    m_Recording = NULL;

    m_ModeOnly = ModeOnly;
    m_DimmActive = false;

    ProgressShown = false;
    CreateFullOsd();
    TopBarCreate();
    MessageCreate();

    ScreenWidth = LastScreenWidth = -1;

    TVSLeft = 20 + Config.decorBorderChannelEPGSize;
    TVSTop = m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize;
    TVSWidth = m_OsdWidth - 40 - Config.decorBorderChannelEPGSize * 2;
    TVSHeight = m_OsdHeight - m_TopBarHeight - m_LabelHeight - 40 - Config.decorBorderChannelEPGSize * 2;

    ChanEpgImagesPixmap = CreatePixmap(osd, "ChanEpgImagesPixmap", 2, cRect(TVSLeft, TVSTop, TVSWidth, TVSHeight));
    PixmapFill(ChanEpgImagesPixmap, clrTransparent);

    LabelPixmap = CreatePixmap(osd, "LabelPixmap", 1,
                               cRect(Config.decorBorderReplaySize,
                               m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                               m_OsdWidth - Config.decorBorderReplaySize * 2, m_LabelHeight));
    iconsPixmap = CreatePixmap(osd, "iconsPixmap", 2,
                               cRect(Config.decorBorderReplaySize,
                               m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize,
                               m_OsdWidth - Config.decorBorderReplaySize * 2, m_LabelHeight));

    ProgressBarCreate(Config.decorBorderReplaySize,
                      m_OsdHeight - m_LabelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize
                       - m_MarginItem, m_OsdWidth - Config.decorBorderReplaySize * 2, Config.decorProgressReplaySize,
                       m_MarginItem, 0, Config.decorProgressReplayFg, Config.decorProgressReplayBarFg,
                       Config.decorProgressReplayBg, Config.decorProgressReplayType);

    labelJump = CreatePixmap(osd, "labelJump", 1, cRect(Config.decorBorderReplaySize,
        m_OsdHeight - m_LabelHeight - Config.decorProgressReplaySize * 2 - m_MarginItem*3 - m_FontHight
         - Config.decorBorderReplaySize * 2, m_OsdWidth - Config.decorBorderReplaySize * 2, m_FontHight));

    dimmPixmap = CreatePixmap(osd, "dimmPixmap", MAXPIXMAPLAYERS-1, cRect(0, 0, m_OsdWidth, m_OsdHeight));

    PixmapFill(LabelPixmap, Theme.Color(clrReplayBg));
    PixmapFill(labelJump, clrTransparent);
    PixmapFill(iconsPixmap, clrTransparent);
    PixmapFill(dimmPixmap, clrTransparent);

    m_FontSecs = cFont::CreateFont(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);

    if (Config.PlaybackWeatherShow)
        DrawWidgetWeather();
}

cFlatDisplayReplay::~cFlatDisplayReplay() {
    if (m_FontSecs)
        delete m_FontSecs;

    if (LabelPixmap)
        osd->DestroyPixmap(LabelPixmap);
    if (labelJump)
        osd->DestroyPixmap(labelJump);
    if (iconsPixmap)
        osd->DestroyPixmap(iconsPixmap);
    if (ChanEpgImagesPixmap)
        osd->DestroyPixmap(ChanEpgImagesPixmap);
    if (dimmPixmap)
        osd->DestroyPixmap(dimmPixmap);
}

void cFlatDisplayReplay::SetRecording(const cRecording *Recording) {
    if (m_ModeOnly) return;

    int left = m_MarginItem;  // Position for recordingsymbol/shorttext/date
    const cRecordingInfo *RecInfo = Recording->Info();
    m_Recording = Recording;

    PixmapFill(iconsPixmap, clrTransparent);

    SetTitle(RecInfo->Title());

    // Show if still recording
    cImage *img = NULL;
    if ((m_Recording->IsInUse() & ruTimer) != 0) {  // The recording is currently written to by a timer
        img = ImgLoader.LoadIcon("timerRecording", 999, m_FontSmlHight);  // Small image

        if (img) {
            int ImageTop = m_FontHight + (m_FontSmlHight - img->Height()) / 2;
            iconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left += img->Width() + m_MarginItem;
        }
    }

    cString info("");
    if (RecInfo->ShortText()) {
        if (Config.PlaybackShowRecordingDate)  //  Date Time - ShortText
            info = cString::sprintf("%s  %s - %s", *ShortDateString(m_Recording->Start()),
                                    *TimeString(m_Recording->Start()), RecInfo->ShortText());
        else
            info = cString::sprintf("%s", RecInfo->ShortText());
    } else {  // No shorttext
        info = cString::sprintf("%s  %s", *ShortDateString(m_Recording->Start()), *TimeString(m_Recording->Start()));
    }

    int infoWidth = m_FontSml->Width(*info);  // Width of shorttext
    // TODO: How to get width of aspect and format icons?
    //  Done: Substract 'left' in case of displayed recording icon
    //  Done: Substract 'm_FontSmlHight' in case of recordingerror icon is displayed later
    //  Workaround: Substract width of aspect and format icons (ResolutionAspectDraw()) ???
    int MaxWidth = m_OsdWidth - left - Config.decorBorderReplaySize * 2;

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors)
        MaxWidth -= m_FontSmlHight;  // Substract width of imgRecErr
#endif

    img = ImgLoader.LoadIcon("1920x1080", 999, m_FontSmlHight);
    if (img)
        MaxWidth -= img->Width() * 3;  // Substract guessed max. used space of aspect and format icons

    if (infoWidth > MaxWidth) {  // Shorttext too long
        if (Config.ScrollerEnable) {
            MessageScroller.AddScroller(*info,
                                        cRect(Config.decorBorderReplaySize + left,
                                              m_OsdHeight - m_LabelHeight - Config.decorBorderReplaySize + m_FontHight,
                                              MaxWidth, m_FontSmlHight),
                                        Theme.Color(clrMenuEventFontInfo), clrTransparent, m_FontSml);
            left += MaxWidth;
        } else {  // Add ... if info ist too long
            dsyslog("flatPlus: Shorttext too long! (%d) Setting MaxWidth to %d", infoWidth, MaxWidth);
            int DotsWidth = m_FontSml->Width("...");
            LabelPixmap->DrawText(cPoint(left, m_FontHight), *info, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  m_FontSml, MaxWidth - DotsWidth);
            LabelPixmap->DrawText(cPoint(left, m_FontHight), "...", Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  m_FontSml, DotsWidth);
            left += MaxWidth + DotsWidth;
        }
    } else {  // Shorttext fits into maxwidth
        LabelPixmap->DrawText(cPoint(left, m_FontHight), *info, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              m_FontSml, infoWidth);
        left += infoWidth;
    }

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors) {  // Separate config option
        cString ErrIcon = GetRecordingerrorIcon(RecInfo->Errors());
        cString RecErrIcon = cString::sprintf("%s_replay", *ErrIcon);

        img = ImgLoader.LoadIcon(*RecErrIcon, 999, m_FontSmlHight);  // Small image
        if (img) {
            left += m_MarginItem;
            int ImageTop = m_FontHight + (m_FontSmlHight - img->Height()) / 2;
            iconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
        }
    }  // PlaybackShowRecordingErrors
#endif
}

void cFlatDisplayReplay::SetTitle(const char *Title) {
    TopBarSetTitle(Title);
    TopBarSetMenuIcon("extraIcons/Playing");
}

void cFlatDisplayReplay::Action(void) {
    time_t curTime;
    while (Running()) {
        time(&curTime);
        if ((curTime - m_DimmStartTime) > Config.RecordingDimmOnPauseDelay) {
            m_DimmActive = true;
            for (int alpha {0}; (alpha <= Config.RecordingDimmOnPauseOpaque) && Running(); alpha+=2) {
                PixmapFill(dimmPixmap, ArgbToColor(alpha, 0, 0, 0));
                Flush();
            }
            Cancel(-1);
            return;
        }
        cCondWait::SleepMs(100);
    }
}

void cFlatDisplayReplay::SetMode(bool Play, bool Forward, int Speed) {
    int left {0};
    if (Play == false && Config.RecordingDimmOnPause) {
        time(&m_DimmStartTime);
        Start();
    } else if (Play == true && Config.RecordingDimmOnPause) {
        Cancel(-1);
        while (Active())
            cCondWait::SleepMs(10);
        if (m_DimmActive) {
            PixmapFill(dimmPixmap, clrTransparent);
            Flush();
        }
    }
    if (Setup.ShowReplayMode) {
        left = m_OsdWidth - Config.decorBorderReplaySize * 2 - (m_FontHight * 4 + m_MarginItem * 3);
        left /= 2;

        if (m_ModeOnly)
            PixmapFill(LabelPixmap, clrTransparent);

        // PixmapFill(iconsPixmap, clrTransparent);  // Moved to SetRecording
        LabelPixmap->DrawRectangle(cRect(left - m_Font->Width("99") - m_MarginItem, 0,
                                         m_FontHight * 4 + m_MarginItem * 6 + m_Font->Width("99") * 2, m_FontHight),
                                   Theme.Color(clrReplayBg));

        cString rewind("rewind"), pause("pause"), play("play"), forward("forward");
        if (Speed == -1) {  // Replay or pause
            (Play) ? play = "play_sel" : pause = "pause_sel";
        } else {
            cString speed = cString::sprintf("%d", Speed);
            if (Forward) {
                forward = "forward_sel";
                LabelPixmap->DrawText(cPoint(left + m_FontHight * 4 + m_MarginItem * 4, 0), speed,
                                      Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), m_Font);
            } else {
                rewind = "rewind_sel";
                LabelPixmap->DrawText(cPoint(left - m_Font->Width(speed) - m_MarginItem, 0), speed,
                                      Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), m_Font);
            }
        }
        cImage *img = ImgLoader.LoadIcon(*rewind, m_FontHight, m_FontHight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left, 0), *img);

        img = ImgLoader.LoadIcon(*pause, m_FontHight, m_FontHight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left + m_FontHight + m_MarginItem, 0), *img);

        img = ImgLoader.LoadIcon(*play, m_FontHight, m_FontHight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left + m_FontHight * 2 + m_MarginItem * 2, 0), *img);

        img = ImgLoader.LoadIcon(*forward, m_FontHight, m_FontHight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left + m_FontHight * 3 + m_MarginItem * 3, 0), *img);
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
                            m_FontHight * 4 + m_MarginItem * 6 + m_Font->Width("99") * 2, m_FontHight,
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
        PixmapFill(dimmPixmap, clrTransparent);
        Flush();
    }

    if (m_ModeOnly) return;

    ProgressShown = true;
    ProgressBarDrawMarks(Current, Total, marks, Theme.Color(clrReplayMarkFg), Theme.Color(clrReplayMarkCurrentFg));
}

void cFlatDisplayReplay::SetCurrent(const char *Current) {
    if (m_ModeOnly) return;

    current = Current;
    UpdateInfo();
}

void cFlatDisplayReplay::SetTotal(const char *Total) {
    if (m_ModeOnly) return;

    total = Total;
    UpdateInfo();
}

void cFlatDisplayReplay::UpdateInfo(void) {
    if (m_ModeOnly) return;

    cString cutted(""), Dummy("");
    bool IsCutted = false;

    int fontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);
    int fontSecsAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);
    int topSecs = fontAscender - fontSecsAscender;

    if (Config.TimeSecsScale == 1.0)
        LabelPixmap->DrawText(cPoint(m_MarginItem, 0), *current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              m_Font, m_Font->Width(*current), m_FontHight);
    else {
        std::string cur = *current;
        std::size_t found = cur.find_last_of(':');
        if (found != std::string::npos) {
            std::string hm = cur.substr(0, found);
            std::string secs = cur.substr(found, cur.length() - found);
            secs.append(1, ' ');  // Fix for extra pixel glitch

            LabelPixmap->DrawText(cPoint(m_MarginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHight);
            LabelPixmap->DrawText(cPoint(m_MarginItem + m_Font->Width(hm.c_str()), topSecs), secs.c_str(),
                                  Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                  m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
        } else {
            LabelPixmap->DrawText(cPoint(m_MarginItem, 0), *current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  m_Font, m_Font->Width(*current), m_FontHight);
        }
    }

    cImage *img = NULL;
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
            if (MediaHeight > TVSHeight || MediaWidth > TVSWidth) {  // Resize too big poster/banner
                dsyslog("flatPlus: Poster/Banner size (%d x %d) is too big!", MediaWidth, MediaHeight);
                MediaHeight = TVSHeight * 0.7 * Config.TVScraperReplayInfoPosterSize * 100;  // Max 70% of pixmap height
                if (Config.ChannelWeatherShow)
                    // Max 50% of pixmap width. Aspect is preserved in LoadFile()
                    MediaWidth = TVSWidth * 0.5 * Config.TVScraperReplayInfoPosterSize * 100;
                else
                    // Max 70% of pixmap width. Aspect is preserved in LoadFile()
                    MediaWidth = TVSWidth * 0.7 * Config.TVScraperReplayInfoPosterSize * 100;

                dsyslog("flatPlus: Poster/Banner resized to max %d x %d", MediaWidth, MediaHeight);
            }
            img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);
            if (img) {
                ChanEpgImagesPixmap->DrawImage(cPoint(0, 0), *img);

                DecorBorderDraw(20 + Config.decorBorderChannelEPGSize,
                                m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize,
                                img->Width(), img->Height(), Config.decorBorderChannelEPGSize,
                                Config.decorBorderChannelEPGType, Config.decorBorderChannelEPGFg,
                                Config.decorBorderChannelEPGBg, BorderTVSPoster);
            }
        }
    }

    if (IsCutted) {
        img = ImgLoader.LoadIcon("recording_cutted_extra", m_FontHight, m_FontHight);
        int imgWidth {0};
        if (img)
            imgWidth = img->Width();

        int right = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(total) - m_MarginItem - imgWidth -
                    m_Font->Width(' ') - m_Font->Width(cutted);
        if (Config.TimeSecsScale < 1.0) {
            std::string tot = *total;
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
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHight);
                LabelPixmap->DrawText(cPoint(right - m_MarginItem + m_Font->Width(hm.c_str()), topSecs), secs.c_str(),
                                      Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
                right += m_Font->Width(hm.c_str()) + m_FontSecs->Width(secs.c_str());
                right += m_Font->Width(' ');
            } else {
                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(total), m_FontHight);
                right += m_Font->Width(total);
                right += m_Font->Width(' ');
            }
        } else {
            LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(total), m_FontHight);
            right += m_Font->Width(total);
            right += m_Font->Width(' ');
        }

        if (img) {  // Draw 'recording_cuttet_extra' icon
            iconsPixmap->DrawImage(cPoint(right, 0), *img);
            right += img->Width() + m_MarginItem * 2;
        }

        if (Config.TimeSecsScale < 1.0) {
            std::string cutt = *cutted;
            std::size_t found = cutt.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = cutt.substr(0, found);
                std::string secs = cutt.substr(found, cutt.length() - found);

                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), hm.c_str(), Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHight);
                LabelPixmap->DrawText(cPoint(right - m_MarginItem + m_Font->Width(hm.c_str()), topSecs), secs.c_str(),
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
            } else {
                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(cutted), m_FontHight);
            }
        } else {
            LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(cutted), m_FontHight);
        }
    } else {  // Not cutted
        int right = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(total);
        if (Config.TimeSecsScale < 1.0) {
            std::string tot = *total;
            std::size_t found = tot.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = tot.substr(0, found);
                std::string secs = tot.substr(found, tot.length() - found);

                right = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(hm.c_str()) -
                        m_FontSecs->Width(secs.c_str());
                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(hm.c_str()), m_FontHight);
                LabelPixmap->DrawText(cPoint(right - m_MarginItem + m_Font->Width(hm.c_str()), topSecs), secs.c_str(),
                                      Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_FontSecs,
                                      m_FontSecs->Width(secs.c_str()), m_FontSecs->Height());
            } else {
                LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), *total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), m_Font, m_Font->Width(total), m_FontHight);
            }
        } else {
            LabelPixmap->DrawText(cPoint(right - m_MarginItem, 0), *total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), m_Font, m_Font->Width(total), m_FontHight);
        }
    }  // IsCutted
}

void cFlatDisplayReplay::SetJump(const char *Jump) {
    DecorBorderClearByFrom(BorderRecordJump);

    if (!Jump) {
        PixmapFill(labelJump, clrTransparent);
        return;
    }
    int left = m_OsdWidth - Config.decorBorderReplaySize * 2 - m_Font->Width(Jump);
    left /= 2;

    labelJump->DrawText(cPoint(left, 0), Jump, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), m_Font,
                        m_Font->Width(Jump), m_FontHight, taCenter);

    DecorBorderDraw(left + Config.decorBorderReplaySize,
                    m_OsdHeight - m_LabelHeight - Config.decorProgressReplaySize * 2 - m_MarginItem * 3 - m_FontHight -
                        Config.decorBorderReplaySize * 2,
                    m_Font->Width(Jump), m_FontHight, Config.decorBorderReplaySize, Config.decorBorderReplayType,
                    Config.decorBorderReplayFg, Config.decorBorderReplayBg, BorderRecordJump);
}

void cFlatDisplayReplay::ResolutionAspectDraw(void) {
    if (m_ModeOnly) return;

    if (ScreenWidth > 0) {
        int left = m_OsdWidth - Config.decorBorderReplaySize * 2;
        int ImageTop {0};
        cImage *img = NULL;
        cString IconName("");
        if (Config.RecordingResolutionAspectShow) {  // Show Aspect
            IconName = GetAspectIcon(ScreenWidth, ScreenAspect);
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHight);
            if (img) {
                ImageTop = m_FontHight + (m_FontSmlHight - img->Height()) / 2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem * 2;
            }

            IconName = GetScreenResolutionIcon(ScreenWidth, ScreenHeight, ScreenAspect);  // Show Resolution
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHight);
            if (img) {
                ImageTop = m_FontHight + (m_FontSmlHight - img->Height()) / 2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem * 2;
            }
        }

        if (Config.RecordingFormatShow && !Config.RecordingSimpleAspectFormat) {
            IconName = GetFormatIcon(ScreenWidth);  // Show Format
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHight);
            if (img) {
                ImageTop = m_FontHight + (m_FontSmlHight - img->Height()) / 2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
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
        cDevice::PrimaryDevice()->GetVideoSize(ScreenWidth, ScreenHeight, ScreenAspect);
        if (ScreenWidth != LastScreenWidth) {
            LastScreenWidth = ScreenWidth;
            ResolutionAspectDraw();
        }
    }

    osd->Flush();
}

void cFlatDisplayReplay::PreLoadImages(void) {
    ImgLoader.LoadIcon("rewind", m_FontHight, m_FontHight);
    ImgLoader.LoadIcon("pause", m_FontHight, m_FontHight);
    ImgLoader.LoadIcon("play", m_FontHight, m_FontHight);
    ImgLoader.LoadIcon("forward", m_FontHight, m_FontHight);
    ImgLoader.LoadIcon("rewind_sel", m_FontHight, m_FontHight);
    ImgLoader.LoadIcon("pause_sel", m_FontHight, m_FontHight);
    ImgLoader.LoadIcon("play_sel", m_FontHight, m_FontHight);
    ImgLoader.LoadIcon("forward_sel", m_FontHight, m_FontHight);
    ImgLoader.LoadIcon("recording_cutted_extra", m_FontHight, m_FontHight);

    ImgLoader.LoadIcon("recording_untested_replay", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("recording_ok_replay", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("recording_warning_replay", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("recording_error_replay", 999, m_FontSmlHight);

    ImgLoader.LoadIcon("43", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("169", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("169w", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("221", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("7680x4320", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("3840x2160", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("1920x1080", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("1440x1080", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("1280x720", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("960x720", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("704x576", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("720x576", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("544x576", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("528x576", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("480x576", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("352x576", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("unknown_res", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("uhd", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("hd", 999, m_FontSmlHight);
    ImgLoader.LoadIcon("sd", 999, m_FontSmlHight);
}
