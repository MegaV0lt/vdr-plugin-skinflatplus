#include "displayreplay.h"
#include "flat.h"

cFlatDisplayReplay::cFlatDisplayReplay(bool ModeOnly) {
    g_LabelHeight = g_FontHight + g_FontSmlHight;
    current = "";
    total = "";
    g_Recording = NULL;

    g_ModeOnly = ModeOnly;
    g_DimmActive = false;

    ProgressShown = false;
    CreateFullOsd();
    TopBarCreate();
    MessageCreate();

    ScreenWidth = LastScreenWidth = -1;

    TVSLeft = 20 + Config.decorBorderChannelEPGSize;
    TVSTop = g_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize;
    TVSWidth = g_OsdWidth - 40 - Config.decorBorderChannelEPGSize * 2;
    TVSHeight = g_OsdHeight - g_TopBarHeight - g_LabelHeight - 40 - Config.decorBorderChannelEPGSize * 2;

    ChanEpgImagesPixmap = CreatePixmap(osd, "ChanEpgImagesPixmap", 2, cRect(TVSLeft, TVSTop, TVSWidth, TVSHeight));
    PixmapFill(ChanEpgImagesPixmap, clrTransparent);

    LabelPixmap = CreatePixmap(osd, "LabelPixmap", 1,
                               cRect(Config.decorBorderReplaySize,
                               g_OsdHeight - g_LabelHeight - Config.decorBorderReplaySize,
                               g_OsdWidth - Config.decorBorderReplaySize * 2, g_LabelHeight));
    iconsPixmap = CreatePixmap(osd, "iconsPixmap", 2,
                               cRect(Config.decorBorderReplaySize,
                               g_OsdHeight - g_LabelHeight - Config.decorBorderReplaySize,
                               g_OsdWidth - Config.decorBorderReplaySize * 2, g_LabelHeight));

    ProgressBarCreate(Config.decorBorderReplaySize,
                      g_OsdHeight - g_LabelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize
                       - g_MarginItem, g_OsdWidth - Config.decorBorderReplaySize * 2, Config.decorProgressReplaySize,
                       g_MarginItem, 0, Config.decorProgressReplayFg, Config.decorProgressReplayBarFg,
                       Config.decorProgressReplayBg, Config.decorProgressReplayType);

    labelJump = CreatePixmap(osd, "labelJump", 1, cRect(Config.decorBorderReplaySize,
        g_OsdHeight - g_LabelHeight - Config.decorProgressReplaySize * 2 - g_MarginItem*3 - g_FontHight
         - Config.decorBorderReplaySize * 2, g_OsdWidth - Config.decorBorderReplaySize * 2, g_FontHight));

    dimmPixmap = CreatePixmap(osd, "dimmPixmap", MAXPIXMAPLAYERS-1, cRect(0, 0, g_OsdWidth, g_OsdHeight));

    PixmapFill(LabelPixmap, Theme.Color(clrReplayBg));
    PixmapFill(labelJump, clrTransparent);
    PixmapFill(iconsPixmap, clrTransparent);
    PixmapFill(dimmPixmap, clrTransparent);

    g_FontSecs = cFont::CreateFont(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);

    if (Config.PlaybackWeatherShow)
        DrawWidgetWeather();
}

cFlatDisplayReplay::~cFlatDisplayReplay() {
    if (g_FontSecs)
        delete g_FontSecs;

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
    if (g_ModeOnly) return;

    int left = g_MarginItem;  // Position for recordingsymbol/shorttext/date
    const cRecordingInfo *RecInfo = Recording->Info();
    g_Recording = Recording;

    PixmapFill(iconsPixmap, clrTransparent);

    SetTitle(RecInfo->Title());

    // Show if still recording
    cImage *img = NULL;
    if ((g_Recording->IsInUse() & ruTimer) != 0) {  // The recording is currently written to by a timer
        img = imgLoader.LoadIcon("timerRecording", 999, g_FontSmlHight);  // Small image

        if (img) {
            int ImageTop = g_FontHight + (g_FontSmlHight - img->Height()) / 2;
            iconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left += img->Width() + g_MarginItem;
        }
    }

    cString info("");
    if (RecInfo->ShortText()) {
        if (Config.PlaybackShowRecordingDate)  //  Date Time - ShortText
            info = cString::sprintf("%s  %s - %s", *ShortDateString(g_Recording->Start()),
                                    *TimeString(g_Recording->Start()), RecInfo->ShortText());
        else
            info = cString::sprintf("%s", RecInfo->ShortText());
    } else {  // No shorttext
        info = cString::sprintf("%s  %s", *ShortDateString(g_Recording->Start()), *TimeString(g_Recording->Start()));
    }

    int infoWidth = g_FontSml->Width(*info);  // Width of shorttext
    // TODO: How to get width of aspect and format icons?
    //  Done: Substract 'left' in case of displayed recording icon
    //  Done: Substract 'g_FontSmlHight' in case of recordingerror icon is displayed later
    //  Workaround: Substract width of aspect and format icons (ResolutionAspectDraw()) ???
    int MaxWidth = g_OsdWidth - left - Config.decorBorderReplaySize * 2;

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors)
        MaxWidth -= g_FontSmlHight;  // Substract width of imgRecErr
#endif

    img = imgLoader.LoadIcon("1920x1080", 999, g_FontSmlHight);
    if (img)
        MaxWidth -= img->Width() * 3;  // Substract guessed max. used space of aspect and format icons

    if (infoWidth > MaxWidth) {  // Shorttext too long
        if (Config.ScrollerEnable) {
            MessageScroller.AddScroller(*info,
                                        cRect(Config.decorBorderReplaySize + left,
                                              g_OsdHeight - g_LabelHeight - Config.decorBorderReplaySize + g_FontHight,
                                              MaxWidth, g_FontSmlHight),
                                        Theme.Color(clrMenuEventFontInfo), clrTransparent, g_FontSml);
            left += MaxWidth;
        } else {  // Add ... if info ist too long
            dsyslog("flatPlus: Shorttext too long! (%d) Setting MaxWidth to %d", infoWidth, MaxWidth);
            int DotsWidth = g_FontSml->Width("...");
            LabelPixmap->DrawText(cPoint(left, g_FontHight), *info, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  g_FontSml, MaxWidth - DotsWidth);
            LabelPixmap->DrawText(cPoint(left, g_FontHight), "...", Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  g_FontSml, DotsWidth);
            left += MaxWidth + DotsWidth;
        }
    } else {  // Shorttext fits into maxwidth
        LabelPixmap->DrawText(cPoint(left, g_FontHight), *info, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              g_FontSml, infoWidth);
        left += infoWidth;
    }

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors) {  // Separate config option
        cString recErrIcon = GetRecordingerrorIcon(RecInfo->Errors());
        cString RecErrIcon = cString::sprintf("%s_replay", *recErrIcon);

        img = imgLoader.LoadIcon(*RecErrIcon, 999, g_FontSmlHight);  // Small image
        if (img) {
            left += g_MarginItem;
            int ImageTop = g_FontHight + (g_FontSmlHight - img->Height()) / 2;
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
        if ((curTime - g_DimmStartTime) > Config.RecordingDimmOnPauseDelay) {
            g_DimmActive = true;
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
        time(&g_DimmStartTime);
        Start();
    } else if (Play == true && Config.RecordingDimmOnPause) {
        Cancel(-1);
        while (Active())
            cCondWait::SleepMs(10);
        if (g_DimmActive) {
            PixmapFill(dimmPixmap, clrTransparent);
            Flush();
        }
    }
    if (Setup.ShowReplayMode) {
        left = g_OsdWidth - Config.decorBorderReplaySize * 2 - (g_FontHight * 4 + g_MarginItem * 3);
        left /= 2;

        if (g_ModeOnly)
            PixmapFill(LabelPixmap, clrTransparent);

        // PixmapFill(iconsPixmap, clrTransparent);  // Moved to SetRecording
        LabelPixmap->DrawRectangle(cRect(left - g_Font->Width("99") - g_MarginItem, 0,
                                         g_FontHight * 4 + g_MarginItem * 6 + g_Font->Width("99") * 2, g_FontHight),
                                   Theme.Color(clrReplayBg));

        cString rewind("rewind"), pause("pause"), play("play"), forward("forward");
        if (Speed == -1) {  // Replay or pause
            (Play) ? play = "play_sel" : pause = "pause_sel";
        } else {
            cString speed = cString::sprintf("%d", Speed);
            if (Forward) {
                forward = "forward_sel";
                LabelPixmap->DrawText(cPoint(left + g_FontHight * 4 + g_MarginItem * 4, 0), speed,
                                      Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), g_Font);
            } else {
                rewind = "rewind_sel";
                LabelPixmap->DrawText(cPoint(left - g_Font->Width(speed) - g_MarginItem, 0), speed,
                                      Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), g_Font);
            }
        }
        cImage *img = imgLoader.LoadIcon(*rewind, g_FontHight, g_FontHight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left, 0), *img);

        img = imgLoader.LoadIcon(*pause, g_FontHight, g_FontHight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left + g_FontHight + g_MarginItem, 0), *img);

        img = imgLoader.LoadIcon(*play, g_FontHight, g_FontHight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left + g_FontHight * 2 + g_MarginItem * 2, 0), *img);

        img = imgLoader.LoadIcon(*forward, g_FontHight, g_FontHight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left + g_FontHight * 3 + g_MarginItem * 3, 0), *img);
    }

    if (ProgressShown) {
        DecorBorderDraw(Config.decorBorderReplaySize,
                        g_OsdHeight - g_LabelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize -
                            g_MarginItem,
                        g_OsdWidth - Config.decorBorderReplaySize * 2,
                        g_LabelHeight + Config.decorProgressReplaySize + g_MarginItem, Config.decorBorderReplaySize,
                        Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
    } else {
        if (g_ModeOnly) {
            DecorBorderDraw(left - g_Font->Width("99") - g_MarginItem + Config.decorBorderReplaySize,
                            g_OsdHeight - g_LabelHeight - Config.decorBorderReplaySize,
                            g_FontHight * 4 + g_MarginItem * 6 + g_Font->Width("99") * 2, g_FontHight,
                            Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg,
                            Config.decorBorderReplayBg);
        } else {
            DecorBorderDraw(Config.decorBorderReplaySize, g_OsdHeight - g_LabelHeight - Config.decorBorderReplaySize,
                            g_OsdWidth - Config.decorBorderReplaySize * 2, g_LabelHeight, Config.decorBorderReplaySize,
                            Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
        }
    }

    ResolutionAspectDraw();
}

void cFlatDisplayReplay::SetProgress(int Current, int Total) {
    if (g_DimmActive) {
        PixmapFill(dimmPixmap, clrTransparent);
        Flush();
    }

    if (g_ModeOnly) return;

    ProgressShown = true;
    ProgressBarDrawMarks(Current, Total, marks, Theme.Color(clrReplayMarkFg), Theme.Color(clrReplayMarkCurrentFg));
}

void cFlatDisplayReplay::SetCurrent(const char *Current) {
    if (g_ModeOnly) return;

    current = Current;
    UpdateInfo();
}

void cFlatDisplayReplay::SetTotal(const char *Total) {
    if (g_ModeOnly) return;

    total = Total;
    UpdateInfo();
}

void cFlatDisplayReplay::UpdateInfo(void) {
    if (g_ModeOnly) return;

    cString cutted(""), Dummy("");
    bool IsCutted = false;

    int fontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);
    int fontSecsAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);
    int topSecs = fontAscender - fontSecsAscender;

    if (Config.TimeSecsScale == 1.0)
        LabelPixmap->DrawText(cPoint(g_MarginItem, 0), *current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              g_Font, g_Font->Width(*current), g_FontHight);
    else {
        std::string cur = *current;
        size_t found = cur.find_last_of(':');
        if (found != std::string::npos) {
            std::string hm = cur.substr(0, found);
            std::string secs = cur.substr(found, cur.length() - found);
            secs.append(1, ' ');  // Fix for extra pixel glitch

            LabelPixmap->DrawText(cPoint(g_MarginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), g_Font, g_Font->Width(hm.c_str()), g_FontHight);
            LabelPixmap->DrawText(cPoint(g_MarginItem + g_Font->Width(hm.c_str()), topSecs), secs.c_str(),
                                  Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), g_FontSecs,
                                  g_FontSecs->Width(secs.c_str()), g_FontSecs->Height());
        } else {
            LabelPixmap->DrawText(cPoint(g_MarginItem, 0), *current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  g_Font, g_Font->Width(*current), g_FontHight);
        }
    }

    cImage *img = NULL;
    if (g_Recording) {
        IsCutted = GetCuttedLengthMarks(g_Recording, Dummy, cutted, false);  // Process marks and get cutted time

        cString MediaPath("");
        int MediaWidth {0}, MediaHeight {0};
        static cPlugin *pScraper = GetScraperPlugin();
        if (Config.TVScraperReplayInfoShowPoster && pScraper) {
            ScraperGetEventType call;
            call.recording = g_Recording;
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
            img = imgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);
            if (img) {
                ChanEpgImagesPixmap->DrawImage(cPoint(0, 0), *img);

                DecorBorderDraw(20 + Config.decorBorderChannelEPGSize,
                                g_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize,
                                img->Width(), img->Height(), Config.decorBorderChannelEPGSize,
                                Config.decorBorderChannelEPGType, Config.decorBorderChannelEPGFg,
                                Config.decorBorderChannelEPGBg, BorderTVSPoster);
            }
        }
    }

    if (IsCutted) {
        img = imgLoader.LoadIcon("recording_cutted_extra", g_FontHight, g_FontHight);
        int imgWidth {0};
        if (img)
            imgWidth = img->Width();

        int right = g_OsdWidth - Config.decorBorderReplaySize * 2 - g_Font->Width(total) - g_MarginItem - imgWidth -
                    g_Font->Width(' ') - g_Font->Width(cutted);
        if (Config.TimeSecsScale < 1.0) {
            std::string tot = *total;
            size_t found = tot.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = tot.substr(0, found);
                std::string secs = tot.substr(found, tot.length() - found);

                std::string cutt = *cutted;
                size_t found2 = cutt.find_last_of(':');
                if (found2 != std::string::npos) {
                    std::string hm2 = cutt.substr(0, found);
                    std::string secs2 = cutt.substr(found, cutt.length() - found);

                    right = g_OsdWidth - Config.decorBorderReplaySize * 2 - g_Font->Width(hm.c_str()) -
                            g_FontSecs->Width(secs.c_str()) - g_MarginItem - imgWidth - g_Font->Width(' ') -
                            g_Font->Width(hm2.c_str()) - g_FontSecs->Width(secs2.c_str());
                } else
                    right = g_OsdWidth - Config.decorBorderReplaySize * 2 - g_Font->Width(hm.c_str()) -
                            g_FontSecs->Width(secs.c_str()) - g_MarginItem - imgWidth - g_Font->Width(' ') -
                            g_Font->Width(cutted);

                LabelPixmap->DrawText(cPoint(right - g_MarginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), g_Font, g_Font->Width(hm.c_str()), g_FontHight);
                LabelPixmap->DrawText(cPoint(right - g_MarginItem + g_Font->Width(hm.c_str()), topSecs), secs.c_str(),
                                      Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), g_FontSecs,
                                      g_FontSecs->Width(secs.c_str()), g_FontSecs->Height());
                right += g_Font->Width(hm.c_str()) + g_FontSecs->Width(secs.c_str());
                right += g_Font->Width(' ');
            } else {
                LabelPixmap->DrawText(cPoint(right - g_MarginItem, 0), total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), g_Font, g_Font->Width(total), g_FontHight);
                right += g_Font->Width(total);
                right += g_Font->Width(' ');
            }
        } else {
            LabelPixmap->DrawText(cPoint(right - g_MarginItem, 0), total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), g_Font, g_Font->Width(total), g_FontHight);
            right += g_Font->Width(total);
            right += g_Font->Width(' ');
        }

        if (img) {  // Draw 'recording_cuttet_extra' icon
            iconsPixmap->DrawImage(cPoint(right, 0), *img);
            right += img->Width() + g_MarginItem * 2;
        }

        if (Config.TimeSecsScale < 1.0) {
            std::string cutt = *cutted;
            size_t found = cutt.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = cutt.substr(0, found);
                std::string secs = cutt.substr(found, cutt.length() - found);

                LabelPixmap->DrawText(cPoint(right - g_MarginItem, 0), hm.c_str(), Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), g_Font, g_Font->Width(hm.c_str()), g_FontHight);
                LabelPixmap->DrawText(cPoint(right - g_MarginItem + g_Font->Width(hm.c_str()), topSecs), secs.c_str(),
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), g_FontSecs,
                                      g_FontSecs->Width(secs.c_str()), g_FontSecs->Height());
            } else {
                LabelPixmap->DrawText(cPoint(right - g_MarginItem, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), g_Font, g_Font->Width(cutted), g_FontHight);
            }
        } else {
            LabelPixmap->DrawText(cPoint(right - g_MarginItem, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                  Theme.Color(clrReplayBg), g_Font, g_Font->Width(cutted), g_FontHight);
        }
    } else {  // Not cutted
        int right = g_OsdWidth - Config.decorBorderReplaySize * 2 - g_Font->Width(total);
        if (Config.TimeSecsScale < 1.0) {
            std::string tot = *total;
            size_t found = tot.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = tot.substr(0, found);
                std::string secs = tot.substr(found, tot.length() - found);

                right = g_OsdWidth - Config.decorBorderReplaySize * 2 - g_Font->Width(hm.c_str()) -
                        g_FontSecs->Width(secs.c_str());
                LabelPixmap->DrawText(cPoint(right - g_MarginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), g_Font, g_Font->Width(hm.c_str()), g_FontHight);
                LabelPixmap->DrawText(cPoint(right - g_MarginItem + g_Font->Width(hm.c_str()), topSecs), secs.c_str(),
                                      Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), g_FontSecs,
                                      g_FontSecs->Width(secs.c_str()), g_FontSecs->Height());
            } else {
                LabelPixmap->DrawText(cPoint(right - g_MarginItem, 0), *total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), g_Font, g_Font->Width(total), g_FontHight);
            }
        } else {
            LabelPixmap->DrawText(cPoint(right - g_MarginItem, 0), *total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), g_Font, g_Font->Width(total), g_FontHight);
        }
    }  // IsCutted
}

void cFlatDisplayReplay::SetJump(const char *Jump) {
    DecorBorderClearByFrom(BorderRecordJump);

    if (!Jump) {
        PixmapFill(labelJump, clrTransparent);
        return;
    }
    int left = g_OsdWidth - Config.decorBorderReplaySize * 2 - g_Font->Width(Jump);
    left /= 2;

    labelJump->DrawText(cPoint(left, 0), Jump, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), g_Font,
                        g_Font->Width(Jump), g_FontHight, taCenter);

    DecorBorderDraw(left + Config.decorBorderReplaySize,
                    g_OsdHeight - g_LabelHeight - Config.decorProgressReplaySize * 2 - g_MarginItem * 3 - g_FontHight -
                        Config.decorBorderReplaySize * 2,
                    g_Font->Width(Jump), g_FontHight, Config.decorBorderReplaySize, Config.decorBorderReplayType,
                    Config.decorBorderReplayFg, Config.decorBorderReplayBg, BorderRecordJump);
}

void cFlatDisplayReplay::ResolutionAspectDraw(void) {
    if (g_ModeOnly) return;

    if (ScreenWidth > 0) {
        int left = g_OsdWidth - Config.decorBorderReplaySize * 2;
        int ImageTop {0};
        cImage *img = NULL;
        cString IconName("");
        if (Config.RecordingResolutionAspectShow) {  // Show Aspect
            IconName = GetAspectIcon(ScreenWidth, ScreenAspect);
            img = imgLoader.LoadIcon(*IconName, 999, g_FontSmlHight);
            if (img) {
                ImageTop = g_FontHight + (g_FontSmlHight - img->Height()) / 2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= g_MarginItem * 2;
            }

            IconName = GetScreenResolutionIcon(ScreenWidth, ScreenHeight, ScreenAspect);  // Show Resolution
            img = imgLoader.LoadIcon(*IconName, 999, g_FontSmlHight);
            if (img) {
                ImageTop = g_FontHight + (g_FontSmlHight - img->Height()) / 2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= g_MarginItem * 2;
            }
        }

        if (Config.RecordingFormatShow && !Config.RecordingSimpleAspectFormat) {
            IconName = GetFormatIcon(ScreenWidth);  // Show Format
            img = imgLoader.LoadIcon(*IconName, 999, g_FontSmlHight);
            if (img) {
                ImageTop = g_FontHight + (g_FontSmlHight - img->Height()) / 2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= g_MarginItem * 2;
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
    imgLoader.LoadIcon("rewind", g_FontHight, g_FontHight);
    imgLoader.LoadIcon("pause", g_FontHight, g_FontHight);
    imgLoader.LoadIcon("play", g_FontHight, g_FontHight);
    imgLoader.LoadIcon("forward", g_FontHight, g_FontHight);
    imgLoader.LoadIcon("rewind_sel", g_FontHight, g_FontHight);
    imgLoader.LoadIcon("pause_sel", g_FontHight, g_FontHight);
    imgLoader.LoadIcon("play_sel", g_FontHight, g_FontHight);
    imgLoader.LoadIcon("forward_sel", g_FontHight, g_FontHight);
    imgLoader.LoadIcon("recording_cutted_extra", g_FontHight, g_FontHight);

    imgLoader.LoadIcon("recording_untested_replay", 999, g_FontSmlHight);
    imgLoader.LoadIcon("recording_ok_replay", 999, g_FontSmlHight);
    imgLoader.LoadIcon("recording_warning_replay", 999, g_FontSmlHight);
    imgLoader.LoadIcon("recording_error_replay", 999, g_FontSmlHight);

    imgLoader.LoadIcon("43", 999, g_FontSmlHight);
    imgLoader.LoadIcon("169", 999, g_FontSmlHight);
    imgLoader.LoadIcon("169w", 999, g_FontSmlHight);
    imgLoader.LoadIcon("221", 999, g_FontSmlHight);
    imgLoader.LoadIcon("7680x4320", 999, g_FontSmlHight);
    imgLoader.LoadIcon("3840x2160", 999, g_FontSmlHight);
    imgLoader.LoadIcon("1920x1080", 999, g_FontSmlHight);
    imgLoader.LoadIcon("1440x1080", 999, g_FontSmlHight);
    imgLoader.LoadIcon("1280x720", 999, g_FontSmlHight);
    imgLoader.LoadIcon("960x720", 999, g_FontSmlHight);
    imgLoader.LoadIcon("704x576", 999, g_FontSmlHight);
    imgLoader.LoadIcon("720x576", 999, g_FontSmlHight);
    imgLoader.LoadIcon("544x576", 999, g_FontSmlHight);
    imgLoader.LoadIcon("528x576", 999, g_FontSmlHight);
    imgLoader.LoadIcon("480x576", 999, g_FontSmlHight);
    imgLoader.LoadIcon("352x576", 999, g_FontSmlHight);
    imgLoader.LoadIcon("unknown_res", 999, g_FontSmlHight);
    imgLoader.LoadIcon("uhd", 999, g_FontSmlHight);
    imgLoader.LoadIcon("hd", 999, g_FontSmlHight);
    imgLoader.LoadIcon("sd", 999, g_FontSmlHight);
}
