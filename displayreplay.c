#include "displayreplay.h"
#include "flat.h"

cFlatDisplayReplay::cFlatDisplayReplay(bool ModeOnly) {
    labelHeight = fontHeight + fontSmlHeight;
    current = "";
    total = "";
    recording = NULL;

    modeOnly = ModeOnly;
    dimmActive = false;

    ProgressShown = false;
    CreateFullOsd();
    TopBarCreate();
    MessageCreate();

    screenWidth = lastScreenWidth = -1;

    TVSLeft = 20 + Config.decorBorderChannelEPGSize;
    TVSTop = topBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize;
    TVSWidth = osdWidth - 40 - Config.decorBorderChannelEPGSize * 2;
    TVSHeight = osdHeight - topBarHeight - labelHeight - 40 - Config.decorBorderChannelEPGSize * 2;

    chanEpgImagesPixmap = CreatePixmap(osd, "chanEpgImagesPixmap", 2, cRect(TVSLeft, TVSTop, TVSWidth, TVSHeight));
    PixmapFill(chanEpgImagesPixmap, clrTransparent);

    labelPixmap = CreatePixmap(osd, "labelPixmap", 1,
                               cRect(Config.decorBorderReplaySize,
                               osdHeight - labelHeight - Config.decorBorderReplaySize,
                               osdWidth - Config.decorBorderReplaySize * 2, labelHeight));
    iconsPixmap = CreatePixmap(osd, "iconsPixmap", 2,
                               cRect(Config.decorBorderReplaySize,
                               osdHeight - labelHeight - Config.decorBorderReplaySize,
                               osdWidth - Config.decorBorderReplaySize * 2, labelHeight));

    ProgressBarCreate(Config.decorBorderReplaySize,
                      osdHeight - labelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize
                       - marginItem, osdWidth - Config.decorBorderReplaySize * 2, Config.decorProgressReplaySize,
                       marginItem, 0, Config.decorProgressReplayFg, Config.decorProgressReplayBarFg,
                       Config.decorProgressReplayBg, Config.decorProgressReplayType);

    labelJump = CreatePixmap(osd, "labelJump", 1, cRect(Config.decorBorderReplaySize,
        osdHeight - labelHeight - Config.decorProgressReplaySize * 2 - marginItem*3 - fontHeight
         - Config.decorBorderReplaySize * 2, osdWidth - Config.decorBorderReplaySize * 2, fontHeight));

    dimmPixmap = CreatePixmap(osd, "dimmPixmap", MAXPIXMAPLAYERS-1, cRect(0, 0, osdWidth, osdHeight));

    PixmapFill(labelPixmap, Theme.Color(clrReplayBg));
    PixmapFill(labelJump, clrTransparent);
    PixmapFill(iconsPixmap, clrTransparent);
    PixmapFill(dimmPixmap, clrTransparent);

    fontSecs = cFont::CreateFont(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);

    if (Config.PlaybackWeatherShow)
        DrawWidgetWeather();
}

cFlatDisplayReplay::~cFlatDisplayReplay() {
    if (fontSecs)
        delete fontSecs;

    if (labelPixmap)
        osd->DestroyPixmap(labelPixmap);
    if (labelJump)
        osd->DestroyPixmap(labelJump);
    if (iconsPixmap)
        osd->DestroyPixmap(iconsPixmap);
    if (chanEpgImagesPixmap)
        osd->DestroyPixmap(chanEpgImagesPixmap);
    if (dimmPixmap)
        osd->DestroyPixmap(dimmPixmap);
}

void cFlatDisplayReplay::SetRecording(const cRecording *Recording) {
    if (modeOnly)
        return;

    int left = marginItem;  // Position for recordingsymbol/shorttext/date
    const cRecordingInfo *recInfo = Recording->Info();
    recording = Recording;

    PixmapFill(iconsPixmap, clrTransparent);

    SetTitle(recInfo->Title());

    // Show if still recording
    if ((recording->IsInUse() & ruTimer) != 0) {  // The recording is currently written to by a timer
        cImage *imgRecRecording = imgLoader.LoadIcon("timerRecording", 999, fontSmlHeight);  // Small image

        if (imgRecRecording) {
            int imageTop = fontHeight + (fontSmlHeight - imgRecRecording->Height()) / 2;
            iconsPixmap->DrawImage(cPoint(left, imageTop), *imgRecRecording);
            left += imgRecRecording->Width() + marginItem;
        }
    }

    cString info("");
    if (recInfo->ShortText()) {
        if (Config.PlaybackShowRecordingDate)  //  Date Time - ShortText
            info = cString::sprintf("%s  %s - %s", *ShortDateString(recording->Start()),
                                    *TimeString(recording->Start()), recInfo->ShortText());
        else
            info = cString::sprintf("%s", recInfo->ShortText());
    } else {  // No shorttext
        info = cString::sprintf("%s  %s", *ShortDateString(recording->Start()), *TimeString(recording->Start()));
    }

    int infoWidth = fontSml->Width(*info);  // Width of shorttext
    // TODO: How to get width of aspect and format icons?
    //  Done: Substract 'left' in case of displayed recording icon
    //  Done: Substract 'fontSmlHeight' in case of recordingerror icon is displayed later
    //  Workaround: Substract width of aspect and format icons (ResolutionAspectDraw()) ???
    int maxWidth = osdWidth - left - Config.decorBorderReplaySize * 2;

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors)
        maxWidth -= fontSmlHeight;  // Substract width of imgRecErr
#endif

    cImage *imgRes = imgLoader.LoadIcon("1920x1080", 999, fontSmlHeight);
    maxWidth -= imgRes->Width() * 3;  // Substract guessed max. used space of aspect and format icons

    if (infoWidth > maxWidth) {  // Shorttext too long
        if (Config.ScrollerEnable) {
            messageScroller.AddScroller(*info,
                                        cRect(Config.decorBorderReplaySize + left,
                                              osdHeight - labelHeight - Config.decorBorderReplaySize + fontHeight,
                                              maxWidth, fontSmlHeight),
                                        Theme.Color(clrMenuEventFontInfo), clrTransparent, fontSml);
            left += maxWidth;
        } else {  // Add ... if info ist too long
            dsyslog("flatPlus: Shorttext too long! (%d) Setting maxWidth to %d", infoWidth, maxWidth);
            int dotsWidth = fontSml->Width("...");
            labelPixmap->DrawText(cPoint(left, fontHeight), *info, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  fontSml, maxWidth - dotsWidth);
            labelPixmap->DrawText(cPoint(left, fontHeight), "...", Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  fontSml, dotsWidth);
            left += maxWidth + dotsWidth;
        }
    } else {  // Shorttext fits into maxwidth
        labelPixmap->DrawText(cPoint(left, fontHeight), *info, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              fontSml, infoWidth);
        left += infoWidth;
    }

#if APIVERSNUM >= 20505
    if (Config.PlaybackShowRecordingErrors) {  // Separate config option
        cString recErrIcon = GetRecordingerrorIcon(recInfo->Errors());
        cString RecErrIcon = cString::sprintf("%s_replay", *recErrIcon);

        cImage *imgRecErr = imgLoader.LoadIcon(*RecErrIcon, 999, fontSmlHeight);  // Small image
        if (imgRecErr) {
            left += marginItem;
            int imageTop = fontHeight + (fontSmlHeight - imgRecErr->Height()) / 2;
            iconsPixmap->DrawImage(cPoint(left, imageTop), *imgRecErr);
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
        if ((curTime - dimmStartTime) > Config.RecordingDimmOnPauseDelay) {
            dimmActive = true;
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
        time(&dimmStartTime);
        Start();
    } else if (Play == true && Config.RecordingDimmOnPause) {
        Cancel(-1);
        while (Active())
            cCondWait::SleepMs(10);
        if (dimmActive) {
            PixmapFill(dimmPixmap, clrTransparent);
            Flush();
        }
    }
    if (Setup.ShowReplayMode) {
        left = osdWidth - Config.decorBorderReplaySize * 2 - (fontHeight * 4 + marginItem * 3);
        left /= 2;

        if (modeOnly)
            PixmapFill(labelPixmap, clrTransparent);

        // PixmapFill(iconsPixmap, clrTransparent);  // Moved to SetRecording
        labelPixmap->DrawRectangle(cRect(left - font->Width("99") - marginItem, 0,
                                         fontHeight * 4 + marginItem * 6 + font->Width("99") * 2, fontHeight),
                                   Theme.Color(clrReplayBg));

        cString rewind("rewind"), pause("pause"), play("play"), forward("forward");
        if (Speed == -1) {  // Replay or pause
            (Play) ? play = "play_sel" : pause = "pause_sel";
        } else {
            cString speed = cString::sprintf("%d", Speed);
            if (Forward) {
                forward = "forward_sel";
                labelPixmap->DrawText(cPoint(left + fontHeight * 4 + marginItem * 4, 0), speed,
                                      Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), font);
            } else {
                rewind = "rewind_sel";
                labelPixmap->DrawText(cPoint(left - font->Width(speed) - marginItem, 0), speed,
                                      Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), font);
            }
        }
        cImage *img = imgLoader.LoadIcon(*rewind, fontHeight, fontHeight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left, 0), *img);

        img = imgLoader.LoadIcon(*pause, fontHeight, fontHeight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left + fontHeight + marginItem, 0), *img);

        img = imgLoader.LoadIcon(*play, fontHeight, fontHeight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left + fontHeight * 2 + marginItem * 2, 0), *img);

        img = imgLoader.LoadIcon(*forward, fontHeight, fontHeight);
        if (img)
            iconsPixmap->DrawImage(cPoint(left + fontHeight * 3 + marginItem * 3, 0), *img);
    }

    if (ProgressShown) {
        DecorBorderDraw(Config.decorBorderReplaySize,
                        osdHeight - labelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize -
                            marginItem,
                        osdWidth - Config.decorBorderReplaySize * 2,
                        labelHeight + Config.decorProgressReplaySize + marginItem, Config.decorBorderReplaySize,
                        Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
    } else {
        if (modeOnly) {
            DecorBorderDraw(left - font->Width("99") - marginItem + Config.decorBorderReplaySize,
                            osdHeight - labelHeight - Config.decorBorderReplaySize,
                            fontHeight * 4 + marginItem * 6 + font->Width("99") * 2, fontHeight,
                            Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg,
                            Config.decorBorderReplayBg);
        } else {
            DecorBorderDraw(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorBorderReplaySize,
                            osdWidth - Config.decorBorderReplaySize * 2, labelHeight, Config.decorBorderReplaySize,
                            Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
        }
    }

    ResolutionAspectDraw();
}

void cFlatDisplayReplay::SetProgress(int Current, int Total) {
    if (dimmActive) {
        PixmapFill(dimmPixmap, clrTransparent);
        Flush();
    }

    if (modeOnly)
        return;

    ProgressShown = true;
    ProgressBarDrawMarks(Current, Total, marks, Theme.Color(clrReplayMarkFg), Theme.Color(clrReplayMarkCurrentFg));
}

void cFlatDisplayReplay::SetCurrent(const char *Current) {
    if (modeOnly)
        return;

    current = Current;
    UpdateInfo();
}

void cFlatDisplayReplay::SetTotal(const char *Total) {
    if (modeOnly)
        return;

    total = Total;
    UpdateInfo();
}

void cFlatDisplayReplay::UpdateInfo(void) {
    if (modeOnly)
        return;

    cString cutted("");
    bool iscutted = false;

    int fontAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize);
    int fontSecsAscender = GetFontAscender(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);
    int topSecs = fontAscender - fontSecsAscender;

    if (Config.TimeSecsScale == 1.0)
        labelPixmap->DrawText(cPoint(marginItem, 0), *current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                              font, font->Width(*current), fontHeight);
    else {
        std::string cur = *current;
        size_t found = cur.find_last_of(':');
        if (found != std::string::npos) {
            std::string hm = cur.substr(0, found);
            std::string secs = cur.substr(found, cur.length() - found);
            secs.append(1, ' ');  // Fix for extra pixel glitch

            labelPixmap->DrawText(cPoint(marginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), font, font->Width(hm.c_str()), fontHeight);
            labelPixmap->DrawText(cPoint(marginItem + font->Width(hm.c_str()), topSecs), secs.c_str(),
                                  Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), fontSecs,
                                  fontSecs->Width(secs.c_str()), fontSecs->Height());
        } else {
            labelPixmap->DrawText(cPoint(marginItem, 0), *current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  font, font->Width(*current), fontHeight);
        }
    }

    if (recording) {
        cMarks marks;
        // From skinElchiHD - Avoid triggering index generation for recordings with empty/missing index
        bool hasMarks = false;
        cIndexFile *index = NULL;
        if (recording->NumFrames() > 0) {
            hasMarks = marks.Load(recording->FileName(), recording->FramesPerSecond(), recording->IsPesRecording()) &&
                       marks.Count();
            index = new cIndexFile(recording->FileName(), false, recording->IsPesRecording());
        }

        int cuttedLength {0};
        int32_t cutinframe {0};
        uint64_t recsizecutted {0};
        uint64_t cutinoffset {0};
        uint64_t filesize[100000];
        uint16_t maxFiles = (recording->IsPesRecording()) ? 999 : 65535;
        filesize[0] = 0;

        int i {0}, rc {0};
        struct stat filebuf;
        cString filename("");

        do {
            ++i;
            if (recording->IsPesRecording())
                filename = cString::sprintf("%s/%03d.vdr", recording->FileName(), i);
            else
                filename = cString::sprintf("%s/%05d.ts", recording->FileName(), i);
            rc = stat(*filename, &filebuf);
            if (rc == 0)
                filesize[i] = filesize[i - 1] + filebuf.st_size;
            else {
                if (ENOENT != errno) {
                    esyslog("flatPlus: Error determining file size of \"%s\" %d (%s)", (const char *)filename, errno,
                            strerror(errno));
                }
            }
        } while (i <= maxFiles && !rc);

        if (hasMarks && index) {
            uint16_t FileNumber;
            off_t FileOffset;

            bool cutin = true;
            int32_t position {0};
            cMark *mark = marks.First();
            while (mark) {
                position = mark->Position();
                index->Get(position, &FileNumber, &FileOffset);
                if (cutin) {
                    cutinframe = position;
                    cutin = false;
                    cutinoffset = filesize[FileNumber-1] + FileOffset;
                } else {
                    cuttedLength += position - cutinframe;
                    cutin = true;
                    recsizecutted += filesize[FileNumber-1] + FileOffset - cutinoffset;
                }
                cMark *nextmark = marks.Next(mark);
                mark = nextmark;
            }
            if (!cutin) {
                cuttedLength += index->Last() - cutinframe;
                index->Get(index->Last() - 1, &FileNumber, &FileOffset);
                recsizecutted += filesize[FileNumber-1] + FileOffset - cutinoffset;
            }
        }
        if (index) {
            if (hasMarks) {
                cutted = IndexToHMSF(cuttedLength, false, recording->FramesPerSecond());
                iscutted = true;
            }
        }
        delete index;

        std::string mediaPath("");
        int mediaWidth {0};
        int mediaHeight {0};
        static cPlugin *pScraper = GetScraperPlugin();
        if (Config.TVScraperReplayInfoShowPoster && pScraper) {
            ScraperGetEventType call;
            call.recording = recording;
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

                        mediaPath = series.banners[number].path;
                        mediaWidth = series.banners[number].width * Config.TVScraperReplayInfoPosterSize * 100;
                        mediaHeight = series.banners[number].height * Config.TVScraperReplayInfoPosterSize * 100;
                        if (series.banners.size() > 1)
                            dsyslog("flatPlus: Using random image %d (%s) out of %d available images",
                                    static_cast<int>(number + 1), mediaPath.c_str(),
                                    static_cast<int>(series.banners.size()));  // Log result
                    }
                }
            } else if (call.type == tMovie) {
                cMovie movie;
                movie.movieId = movieId;
                if (pScraper->Service("GetMovie", &movie)) {
                    mediaPath = movie.poster.path;
                    mediaWidth = movie.poster.width /* * 0.5 */ * Config.TVScraperReplayInfoPosterSize * 100;
                    mediaHeight = movie.poster.height /* * 0.5 */ * Config.TVScraperReplayInfoPosterSize * 100;
                }
            }
        }

        PixmapFill(chanEpgImagesPixmap, clrTransparent);
        DecorBorderClearByFrom(BorderTVSPoster);
        if (mediaPath.length() > 0) {
            if (mediaHeight > TVSHeight || mediaWidth > TVSWidth) {  // Resize too big poster/banner
                dsyslog("flatPlus: Poster/Banner size (%d x %d) is too big!", mediaWidth, mediaHeight);
                if (Config.PlaybackWeatherShow) {
                    mediaHeight = TVSHeight * 0.5;  // Max 50% of pixmap height/width
                    mediaWidth = TVSWidth * 0.5;    // Aspect is preserved in LoadFile()
                } else {
                    mediaHeight = TVSHeight * 0.7;  // Max 70% of pixmap height/width
                    mediaWidth = TVSWidth * 0.7;    // Aspect is preserved in LoadFile()
                }
                dsyslog("flatPlus: Poster/Banner resized to max %d x %d", mediaWidth, mediaHeight);
            }
            cImage *img = imgLoader.LoadFile(mediaPath.c_str(), mediaWidth, mediaHeight);
            if (img) {
                chanEpgImagesPixmap->DrawImage(cPoint(0, 0), *img);

                DecorBorderDraw(20 + Config.decorBorderChannelEPGSize,
                                topBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize,
                                img->Width(), img->Height(), Config.decorBorderChannelEPGSize,
                                Config.decorBorderChannelEPGType, Config.decorBorderChannelEPGFg,
                                Config.decorBorderChannelEPGBg, BorderTVSPoster);
            }
        }
    }

    if (iscutted) {
        cImage *imgRecCut = imgLoader.LoadIcon("recording_cutted_extra", fontHeight, fontHeight);
        int imgWidth {0};
        if (imgRecCut)
            imgWidth = imgRecCut->Width();

        int right = osdWidth - Config.decorBorderReplaySize * 2 - font->Width(total) - marginItem - imgWidth -
                    font->Width(' ') - font->Width(cutted);
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

                    right = osdWidth - Config.decorBorderReplaySize * 2 - font->Width(hm.c_str()) -
                            fontSecs->Width(secs.c_str()) - marginItem - imgWidth - font->Width(' ') -
                            font->Width(hm2.c_str()) - fontSecs->Width(secs2.c_str());
                } else
                    right = osdWidth - Config.decorBorderReplaySize * 2 - font->Width(hm.c_str()) -
                            fontSecs->Width(secs.c_str()) - marginItem - imgWidth - font->Width(' ') -
                            font->Width(cutted);

                labelPixmap->DrawText(cPoint(right - marginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), font, font->Width(hm.c_str()), fontHeight);
                labelPixmap->DrawText(cPoint(right - marginItem + font->Width(hm.c_str()), topSecs), secs.c_str(),
                                      Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), fontSecs,
                                      fontSecs->Width(secs.c_str()), fontSecs->Height());
                right += font->Width(hm.c_str()) + fontSecs->Width(secs.c_str());
                right += font->Width(' ');
            } else {
                labelPixmap->DrawText(cPoint(right - marginItem, 0), total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), font, font->Width(total), fontHeight);
                right += font->Width(total);
                right += font->Width(' ');
            }
        } else {
            labelPixmap->DrawText(cPoint(right - marginItem, 0), total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), font, font->Width(total), fontHeight);
            right += font->Width(total);
            right += font->Width(' ');
        }

        if (imgRecCut) {
            iconsPixmap->DrawImage(cPoint(right, 0), *imgRecCut);
            right += imgRecCut->Width() + marginItem * 2;
        }

        if (Config.TimeSecsScale < 1.0) {
            std::string cutt = *cutted;
            size_t found = cutt.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = cutt.substr(0, found);
                std::string secs = cutt.substr(found, cutt.length() - found);

                labelPixmap->DrawText(cPoint(right - marginItem, 0), hm.c_str(), Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), font, font->Width(hm.c_str()), fontHeight);
                labelPixmap->DrawText(cPoint(right - marginItem + font->Width(hm.c_str()), topSecs), secs.c_str(),
                                      Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), fontSecs,
                                      fontSecs->Width(secs.c_str()), fontSecs->Height());
            } else {
                labelPixmap->DrawText(cPoint(right - marginItem, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                      Theme.Color(clrReplayBg), font, font->Width(cutted), fontHeight);
            }
        } else {
            labelPixmap->DrawText(cPoint(right - marginItem, 0), *cutted, Theme.Color(clrMenuItemExtraTextFont),
                                  Theme.Color(clrReplayBg), font, font->Width(cutted), fontHeight);
        }
    } else {
        int right = osdWidth - Config.decorBorderReplaySize * 2 - font->Width(total);
        if (Config.TimeSecsScale < 1.0) {
            std::string tot = *total;
            size_t found = tot.find_last_of(':');
            if (found != std::string::npos) {
                std::string hm = tot.substr(0, found);
                std::string secs = tot.substr(found, tot.length() - found);

                right = osdWidth - Config.decorBorderReplaySize * 2 - font->Width(hm.c_str()) -
                        fontSecs->Width(secs.c_str());
                labelPixmap->DrawText(cPoint(right - marginItem, 0), hm.c_str(), Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), font, font->Width(hm.c_str()), fontHeight);
                labelPixmap->DrawText(cPoint(right - marginItem + font->Width(hm.c_str()), topSecs), secs.c_str(),
                                      Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), fontSecs,
                                      fontSecs->Width(secs.c_str()), fontSecs->Height());
            } else {
                labelPixmap->DrawText(cPoint(right - marginItem, 0), *total, Theme.Color(clrReplayFont),
                                      Theme.Color(clrReplayBg), font, font->Width(total), fontHeight);
            }
        } else {
            labelPixmap->DrawText(cPoint(right - marginItem, 0), *total, Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), font, font->Width(total), fontHeight);
        }
    }
}

void cFlatDisplayReplay::SetJump(const char *Jump) {
    DecorBorderClearByFrom(BorderRecordJump);

    if (!Jump) {
        PixmapFill(labelJump, clrTransparent);
        return;
    }
    int left = osdWidth - Config.decorBorderReplaySize * 2 - font->Width(Jump);
    left /= 2;

    labelJump->DrawText(cPoint(left, 0), Jump, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font,
                        font->Width(Jump), fontHeight, taCenter);

    DecorBorderDraw(left + Config.decorBorderReplaySize,
                    osdHeight - labelHeight - Config.decorProgressReplaySize * 2 - marginItem * 3 - fontHeight -
                        Config.decorBorderReplaySize * 2,
                    font->Width(Jump), fontHeight, Config.decorBorderReplaySize, Config.decorBorderReplayType,
                    Config.decorBorderReplayFg, Config.decorBorderReplayBg, BorderRecordJump);
}

void cFlatDisplayReplay::ResolutionAspectDraw(void) {
    if (modeOnly)
        return;

    if (screenWidth > 0) {
        int left = osdWidth - Config.decorBorderReplaySize * 2;
        int imageTop {0};
        cImage *img = NULL;
        cString iconName("");
        if (Config.RecordingResolutionAspectShow) {  // Show Aspect
            iconName = GetAspectIcon(screenWidth, screenAspect);
            img = imgLoader.LoadIcon(*iconName, 999, fontSmlHeight);
            if (img) {
                imageTop = fontHeight + (fontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, imageTop), *img);
                left -= marginItem * 2;
            }

            iconName = GetScreenResolutionIcon(screenWidth, screenHeight, screenAspect);  // Show Resolution
            img = imgLoader.LoadIcon(*iconName, 999, fontSmlHeight);
            if (img) {
                imageTop = fontHeight + (fontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, imageTop), *img);
                left -= marginItem * 2;
            }
        }

        if (Config.RecordingFormatShow && !Config.RecordingSimpleAspectFormat) {
            iconName = GetFormatIcon(screenWidth);  // Show Format
            img = imgLoader.LoadIcon(*iconName, 999, fontSmlHeight);
            if (img) {
                imageTop = fontHeight + (fontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, imageTop), *img);
                left -= marginItem * 2;
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
        cDevice::PrimaryDevice()->GetVideoSize(screenWidth, screenHeight, screenAspect);
        if (screenWidth != lastScreenWidth) {
            lastScreenWidth = screenWidth;
            ResolutionAspectDraw();
        }
    }

    osd->Flush();
}

void cFlatDisplayReplay::PreLoadImages(void) {
    imgLoader.LoadIcon("rewind", fontHeight, fontHeight);
    imgLoader.LoadIcon("pause", fontHeight, fontHeight);
    imgLoader.LoadIcon("play", fontHeight, fontHeight);
    imgLoader.LoadIcon("forward", fontHeight, fontHeight);
    imgLoader.LoadIcon("rewind_sel", fontHeight, fontHeight);
    imgLoader.LoadIcon("play_sel", fontHeight, fontHeight);
    imgLoader.LoadIcon("pause_sel", fontHeight, fontHeight);
    imgLoader.LoadIcon("forward_sel", fontHeight, fontHeight);
    imgLoader.LoadIcon("recording_cutted_extra", fontHeight, fontHeight);

    imgLoader.LoadIcon("recording_untested_replay", 999, fontSmlHeight);
    imgLoader.LoadIcon("recording_ok_replay", 999, fontSmlHeight);
    imgLoader.LoadIcon("recording_warning_replay", 999, fontSmlHeight);
    imgLoader.LoadIcon("recording_error_replay", 999, fontSmlHeight);

    imgLoader.LoadIcon("43", 999, fontSmlHeight);
    imgLoader.LoadIcon("169", 999, fontSmlHeight);
    imgLoader.LoadIcon("169w", 999, fontSmlHeight);
    imgLoader.LoadIcon("221", 999, fontSmlHeight);
    imgLoader.LoadIcon("7680x4320", 999, fontSmlHeight);
    imgLoader.LoadIcon("3840x2160", 999, fontSmlHeight);
    imgLoader.LoadIcon("1920x1080", 999, fontSmlHeight);
    imgLoader.LoadIcon("1440x1080", 999, fontSmlHeight);
    imgLoader.LoadIcon("1280x720", 999, fontSmlHeight);
    imgLoader.LoadIcon("960x720", 999, fontSmlHeight);
    imgLoader.LoadIcon("704x576", 999, fontSmlHeight);
    imgLoader.LoadIcon("720x576", 999, fontSmlHeight);
    imgLoader.LoadIcon("544x576", 999, fontSmlHeight);
    imgLoader.LoadIcon("528x576", 999, fontSmlHeight);
    imgLoader.LoadIcon("480x576", 999, fontSmlHeight);
    imgLoader.LoadIcon("352x576", 999, fontSmlHeight);
    imgLoader.LoadIcon("unknown_res", 999, fontSmlHeight);
    imgLoader.LoadIcon("uhd", 999, fontSmlHeight);
    imgLoader.LoadIcon("hd", 999, fontSmlHeight);
    imgLoader.LoadIcon("sd", 999, fontSmlHeight);
}
