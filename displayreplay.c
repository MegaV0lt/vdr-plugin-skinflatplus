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

    int TVSLeft = 20 + Config.decorBorderChannelEPGSize;
    int TVSTop = topBarHeight + Config.decorBorderTopBarSize*2 + 20 + Config.decorBorderChannelEPGSize;
    int TVSWidth = osdWidth - 40 - Config.decorBorderChannelEPGSize*2;
    int TVSHeight = osdHeight - topBarHeight - labelHeight - 40 - Config.decorBorderChannelEPGSize*2;

    chanEpgImagesPixmap = CreatePixmap(2, cRect(TVSLeft, TVSTop, TVSWidth, TVSHeight));
    chanEpgImagesPixmap->Fill( clrTransparent );

    labelPixmap = CreatePixmap(1, cRect(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorBorderReplaySize,
        osdWidth - Config.decorBorderReplaySize*2, labelHeight));
    iconsPixmap = CreatePixmap(2, cRect(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorBorderReplaySize,
        osdWidth - Config.decorBorderReplaySize*2, labelHeight));

    ProgressBarCreate(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize - marginItem,
        osdWidth - Config.decorBorderReplaySize*2, Config.decorProgressReplaySize, marginItem, 0,
        Config.decorProgressReplayFg, Config.decorProgressReplayBarFg, Config.decorProgressReplayBg, Config.decorProgressReplayType);

    labelJump = CreatePixmap(1, cRect(Config.decorBorderReplaySize,
        osdHeight - labelHeight - Config.decorProgressReplaySize*2 - marginItem*3 - fontHeight - Config.decorBorderReplaySize*2,
        osdWidth - Config.decorBorderReplaySize*2, fontHeight));

    dimmPixmap = CreatePixmap(MAXPIXMAPLAYERS-1, cRect(0, 0, osdWidth, osdHeight));

    labelPixmap->Fill(Theme.Color(clrReplayBg));
    labelJump->Fill(clrTransparent);
    iconsPixmap->Fill(clrTransparent);
    dimmPixmap->Fill(clrTransparent);

    fontSecs = cFont::CreateFont(Setup.FontOsd, Setup.FontOsdSize * Config.TimeSecsScale * 100.0);

    if( Config.PlaybackWeatherShow )
        DrawWidgetWeather();
}

cFlatDisplayReplay::~cFlatDisplayReplay() {
    if( fontSecs != NULL )
        delete fontSecs;

    if( labelPixmap )
        osd->DestroyPixmap(labelPixmap);
    if( labelJump )
        osd->DestroyPixmap(labelJump);
    if( iconsPixmap )
        osd->DestroyPixmap(iconsPixmap);
    if( chanEpgImagesPixmap )
        osd->DestroyPixmap(chanEpgImagesPixmap);
    if( dimmPixmap )
        osd->DestroyPixmap(dimmPixmap);
}

void cFlatDisplayReplay::SetRecording(const cRecording *Recording) {
    if( modeOnly )
        return;
    const cRecordingInfo *recInfo = Recording->Info();
    recording = Recording;

    SetTitle( recInfo->Title() );
    cString info("");
    if (recInfo->ShortText())
        info = cString::sprintf("%s - %s %s", recInfo->ShortText(), *ShortDateString(Recording->Start()), *TimeString(Recording->Start()));
    else
        info = cString::sprintf("%s %s", *ShortDateString(Recording->Start()), *TimeString(Recording->Start()));

    labelPixmap->DrawText(cPoint(marginItem, fontHeight), info, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), fontSml, osdWidth - Config.decorBorderReplaySize*2);
}

void cFlatDisplayReplay::SetTitle(const char *Title) {
    TopBarSetTitle(Title);
    TopBarSetMenuIcon("extraIcons/Playing");
}

void cFlatDisplayReplay::Action(void) {
    time_t curTime;
    while( Running() ) {
        time(&curTime);
        if( (curTime - dimmStartTime) > Config.RecordingDimmOnPauseDelay ) {
            dimmActive = true;
            for(int alpha = 0; (alpha <= Config.RecordingDimmOnPauseOpaque) && Running(); alpha+=2) {
                dimmPixmap->Fill(ArgbToColor(alpha, 0, 0, 0));
                Flush();
            }
            Cancel(-1);
            return;
        }
        cCondWait::SleepMs(100);
    }
}

void cFlatDisplayReplay::SetMode(bool Play, bool Forward, int Speed) {
    int left = 0;
    if( Play == false && Config.RecordingDimmOnPause ) {
        time(&dimmStartTime);
        Start();
    } else if( Play == true && Config.RecordingDimmOnPause ) {
        Cancel(-1);
        while( Active() )
            cCondWait::SleepMs(10);
        if( dimmActive ) {
            dimmPixmap->Fill(clrTransparent);
            Flush();
        }
    }
    if( Setup.ShowReplayMode ) {
        left = osdWidth - Config.decorBorderReplaySize*2 - (fontHeight * 4 + marginItem * 3);
        left /= 2;

        if( modeOnly )
            labelPixmap->Fill(clrTransparent);

        iconsPixmap->Fill(clrTransparent);
        labelPixmap->DrawRectangle(cRect( left - font->Width("33") - marginItem, 0, fontHeight*4 + marginItem*6 + font->Width("33")*2, fontHeight), Theme.Color(clrReplayBg) );

        cString rewind(""), pause(""), play(""), forward("");
        cString speed("");

        if( Speed == -1 ) {
            if( Play ) {
                rewind = "rewind";
                pause = "pause";
                play = "play_sel";
                forward = "forward";
            } else {
                rewind = "rewind";
                pause = "pause_sel";
                play = "play";
                forward = "forward";
            }
        } else {
            speed = cString::sprintf("%d", Speed);
            if( Forward ) {
                rewind = "rewind";
                pause = "pause";
                play = "play";
                forward = "forward_sel";
                labelPixmap->DrawText(cPoint(left + fontHeight*4 + marginItem*4, 0), speed, Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), font);
            } else {
                rewind = "rewind_sel";
                pause = "pause";
                play = "play";
                forward = "forward";
                labelPixmap->DrawText(cPoint(left - font->Width(speed) - marginItem, 0), speed, Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), font);
            }
        }
        cImage *img = imgLoader.LoadIcon(*rewind, fontHeight, fontHeight);
        if( img )
            iconsPixmap->DrawImage( cPoint(left, 0), *img );

        img = imgLoader.LoadIcon(*pause, fontHeight, fontHeight);
        if( img )
            iconsPixmap->DrawImage( cPoint(left + fontHeight + marginItem, 0), *img );

        img = imgLoader.LoadIcon(*play, fontHeight, fontHeight);
        if( img )
            iconsPixmap->DrawImage( cPoint(left + fontHeight*2 + marginItem*2, 0), *img );

        img = imgLoader.LoadIcon(*forward, fontHeight, fontHeight);
        if( img )
            iconsPixmap->DrawImage( cPoint(left + fontHeight*3 + marginItem*3, 0), *img );

    }

    if( ProgressShown ) {
        DecorBorderDraw(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize - marginItem,
            osdWidth - Config.decorBorderReplaySize*2, labelHeight + Config.decorProgressReplaySize + marginItem,
            Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
    } else {
        if( modeOnly ) {
            DecorBorderDraw(left - font->Width("33") - marginItem + Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorBorderReplaySize,
                fontHeight*4 + marginItem*6 + font->Width("33")*2, fontHeight,
                Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
        } else {
            DecorBorderDraw(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorBorderReplaySize,
                osdWidth - Config.decorBorderReplaySize*2, labelHeight,
                Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
        }
    }

    ResolutionAspectDraw();
}

void cFlatDisplayReplay::SetProgress(int Current, int Total) {
    if( dimmActive ) {
        dimmPixmap->Fill(clrTransparent);
        Flush();
    }

    if( modeOnly )
        return;
    ProgressShown = true;
    ProgressBarDrawMarks(Current, Total, marks, Theme.Color(clrReplayMarkFg), Theme.Color(clrReplayMarkCurrentFg));
}

void cFlatDisplayReplay::SetCurrent(const char *Current) {
    if( modeOnly )
        return;
    current = Current;
    UpdateInfo();
}

void cFlatDisplayReplay::SetTotal(const char *Total) {
    if( modeOnly )
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
        labelPixmap->DrawText(cPoint(marginItem, 0), current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font,
                              font->Width(current), fontHeight);
    else {
        std::string cur = *current;
        size_t found = cur.find_last_of(':');
        if (found != std::string::npos) {
            std::string hm = cur.substr(0, found);
            std::string secs = cur.substr(found, cur.length() - found);

            labelPixmap->DrawText(cPoint(marginItem, 0), hm.c_str(), Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  font, font->Width(hm.c_str()), fontHeight);
            labelPixmap->DrawText(cPoint(marginItem + font->Width(hm.c_str()), topSecs), secs.c_str(), Theme.Color(clrReplayFont),
                                  Theme.Color(clrReplayBg), fontSecs, fontSecs->Width(secs.c_str()), fontSecs->Height());
        } else {
            labelPixmap->DrawText(cPoint(marginItem, 0), current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg),
                                  font, font->Width(current), fontHeight);
        }
    }

    if( recording ) {
        cMarks marks;
        bool hasMarks = marks.Load(recording->FileName(), recording->FramesPerSecond(), recording->IsPesRecording()) && marks.Count();
        cIndexFile *index = new cIndexFile(recording->FileName(), false, recording->IsPesRecording());
        int cuttedLength = 0;
        long cutinframe = 0;
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
            if (recording->IsPesRecording())
                filename = cString::sprintf("%s/%03d.vdr", recording->FileName(), ++i);
            else {
                filename = cString::sprintf("%s/%05d.ts", recording->FileName(), ++i);
                imax = 99999;
            }
            rc=stat(filename, &filebuf);
            if (rc == 0)
                filesize[i] = filesize[i-1] + filebuf.st_size;
            else {
                if (ENOENT != errno) {
                    esyslog ("skinflatplus: error determining file size of \"%s\" %d (%s)", (const char *)filename, errno, strerror(errno));
                }
            }
        } while( i <= imax && !rc );

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
                    cutinoffset = filesize[FileNumber-1] + FileOffset;
                } else {
                    cuttedLength += position - cutinframe;
                    cutin = true;
                    recsizecutted += filesize[FileNumber-1] + FileOffset - cutinoffset;
                }
                cMark *nextmark = marks.Next(mark);
                mark = nextmark;
            }
            if( !cutin ) {
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
        int mediaWidth = 0;
        int mediaHeight = 0;
        // TVScraper
        // first try scraper2vdr
        static cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
        if( !pScraper ) // if it doesn't exit, try tvscraper
            pScraper = cPluginManager::GetPlugin("tvscraper");
        if( Config.TVScraperReplayInfoShowPoster && pScraper ) {
            ScraperGetEventType call;
            call.recording = recording;
            int seriesId = 0;
            int episodeId = 0;
            int movieId = 0;

            if (pScraper->Service("GetEventType", &call)) {
                seriesId = call.seriesId;
                episodeId = call.episodeId;
                movieId = call.movieId;
            }
            if( seriesId > 0 ) {
                cSeries series;
                series.seriesId = seriesId;
                series.episodeId = episodeId;
                if (pScraper->Service("GetSeries", &series)) {
                    if( series.banners.size() > 0 ) {
                        mediaPath = series.banners[0].path;
                        mediaWidth = series.banners[0].width * Config.TVScraperReplayInfoPosterSize*100;
                        mediaHeight = series.banners[0].height * Config.TVScraperReplayInfoPosterSize*100;
                    }
                }
            } else if (movieId > 0) {
                cMovie movie;
                movie.movieId = movieId;
                if (pScraper->Service("GetMovie", &movie)) {
                    mediaPath = movie.poster.path;
                    mediaWidth = movie.poster.width * 0.5 * Config.TVScraperReplayInfoPosterSize*100;
                    mediaHeight = movie.poster.height * 0.5 * Config.TVScraperReplayInfoPosterSize*100;
                }
            }
        }

        chanEpgImagesPixmap->Fill(clrTransparent);
        DecorBorderClearByFrom(BorderTVSPoster);
        if( mediaPath.length() > 0 ) {
            cImage *img = imgLoader.LoadFile(mediaPath.c_str(), mediaWidth, mediaHeight);
            if( img ) {
                chanEpgImagesPixmap->DrawImage(cPoint(0, 0), *img);

                DecorBorderDraw(20 + Config.decorBorderChannelEPGSize, topBarHeight + Config.decorBorderTopBarSize*2 + 20 + Config.decorBorderChannelEPGSize, img->Width(), img->Height(),
                    Config.decorBorderChannelEPGSize, Config.decorBorderChannelEPGType, Config.decorBorderChannelEPGFg, Config.decorBorderChannelEPGBg, BorderTVSPoster);
            }
        }

    }

    if( iscutted ) {
        cImage *imgRecCut = imgLoader.LoadIcon("recording_cutted_extra", fontHeight, fontHeight);
        int imgWidth = 0;
        if( imgRecCut )
            imgWidth = imgRecCut->Width();

        int right = osdWidth - Config.decorBorderReplaySize*2 - font->Width(total) - marginItem - imgWidth - font->Width(" ") - font->Width(cutted);
        if( Config.TimeSecsScale < 1.0 ) {
            std::string tot = *total;
            size_t found = tot.find_last_of(':');
            if( found != std::string::npos ) {
                std::string hm = tot.substr(0, found);
                std::string secs = tot.substr(found, tot.length() - found);

                std::string cutt = *cutted;
                size_t found2 = cutt.find_last_of(':');
                if( found2 != std::string::npos ) {
                    std::string hm2 = cutt.substr(0, found);
                    std::string secs2 = cutt.substr(found, cutt.length() - found);

                    right = osdWidth - Config.decorBorderReplaySize*2 - font->Width(hm.c_str()) - fontSecs->Width(secs.c_str()) - marginItem - imgWidth - font->Width(" ") - font->Width(hm2.c_str()) - fontSecs->Width(secs2.c_str());
                } else
                    right = osdWidth - Config.decorBorderReplaySize*2 - font->Width(hm.c_str()) - fontSecs->Width(secs.c_str()) - marginItem - imgWidth - font->Width(" ") - font->Width(cutted);

                labelPixmap->DrawText(cPoint(right - marginItem, 0), hm.c_str(), Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(hm.c_str()), fontHeight);
                labelPixmap->DrawText(cPoint(right - marginItem + font->Width(hm.c_str()), topSecs), secs.c_str(), Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), fontSecs, fontSecs->Width(secs.c_str()), fontSecs->Height());
                right += font->Width(hm.c_str()) + fontSecs->Width(secs.c_str());
                right += font->Width(" ");
            } else {
                labelPixmap->DrawText(cPoint(right - marginItem, 0), total, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(total), fontHeight);
                right += font->Width(total);
                right += font->Width(" ");
            }
        } else {
            labelPixmap->DrawText(cPoint(right - marginItem, 0), total, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(total), fontHeight);
            right += font->Width(total);
            right += font->Width(" ");
        }

        if( imgRecCut ) {
            iconsPixmap->DrawImage( cPoint(right, 0), *imgRecCut );
            right += imgRecCut->Width() + marginItem*2;
        }

        if( Config.TimeSecsScale < 1.0 ) {
            std::string cutt = *cutted;
            size_t found = cutt.find_last_of(':');
            if( found != std::string::npos ) {
                std::string hm = cutt.substr(0, found);
                std::string secs = cutt.substr(found, cutt.length() - found);

                labelPixmap->DrawText(cPoint(right - marginItem, 0), hm.c_str(), Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), font, font->Width(hm.c_str()), fontHeight);
                labelPixmap->DrawText(cPoint(right - marginItem + font->Width(hm.c_str()), topSecs), secs.c_str(), Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), fontSecs, fontSecs->Width(secs.c_str()), fontSecs->Height());
            } else {
                labelPixmap->DrawText(cPoint(right - marginItem, 0), cutted, Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), font, font->Width(cutted), fontHeight);
            }
        } else {
            labelPixmap->DrawText(cPoint(right - marginItem, 0), cutted, Theme.Color(clrMenuItemExtraTextFont), Theme.Color(clrReplayBg), font, font->Width(cutted), fontHeight);
        }
    } else {
        int right = osdWidth - Config.decorBorderReplaySize*2 - font->Width(total);
        if( Config.TimeSecsScale < 1.0 ) {
            std::string tot = *total;
            size_t found = tot.find_last_of(':');
            if( found != std::string::npos ) {
                std::string hm = tot.substr(0, found);
                std::string secs = tot.substr(found, tot.length() - found);

                right = osdWidth - Config.decorBorderReplaySize*2 - font->Width(hm.c_str()) - fontSecs->Width(secs.c_str());
                labelPixmap->DrawText(cPoint(right - marginItem, 0), hm.c_str(), Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(hm.c_str()), fontHeight);
                labelPixmap->DrawText(cPoint(right - marginItem + font->Width(hm.c_str()), topSecs), secs.c_str(), Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), fontSecs, fontSecs->Width(secs.c_str()), fontSecs->Height());
            } else {
                labelPixmap->DrawText(cPoint(right - marginItem, 0), total, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(total), fontHeight);
            }
        } else {
            labelPixmap->DrawText(cPoint(right - marginItem, 0), total, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(total), fontHeight);
        }
    }
}

void cFlatDisplayReplay::SetJump(const char *Jump) {
    DecorBorderClearByFrom(BorderRecordJump);

    if( !Jump )
    {
        labelJump->Fill(clrTransparent);
        return;
    }
    int left = osdWidth - Config.decorBorderReplaySize*2 - font->Width(Jump);
    left /= 2;

    labelJump->DrawText(cPoint(left, 0), Jump, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(Jump), fontHeight, taCenter);

    DecorBorderDraw(left + Config.decorBorderReplaySize,
    osdHeight - labelHeight - Config.decorProgressReplaySize*2 - marginItem*3 - fontHeight - Config.decorBorderReplaySize*2,
        font->Width(Jump), fontHeight,
        Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg, BorderRecordJump);
}

void cFlatDisplayReplay::ResolutionAspectDraw(void) {
    if (modeOnly)
        return;
    int left = osdWidth - Config.decorBorderReplaySize*2;
    int imageTop = 0;
    cImage *img = NULL;

    if (screenWidth > 0) {
    if (Config.RecordingResolutionAspectShow) {  // Show Aspect
        cString asp("unknown_asp");              // ???
        if(Config.RecordingSimpleAspectFormat && screenWidth > 720) {
            switch (screenWidth) {               // No aspect for HD
            case 7680:
            case 3840:
                asp = "uhd";
                break;
            default:
                asp = "hd";
                break;
            }
        } else {
            if (screenAspect == 4.0/3.0)
                asp = "43";
            else if (screenAspect == 16.0/9.0)
                asp = "169";
            else if (screenAspect == 20.0/11.0 || screenAspect == 15.0/11.0)
                asp = "169w";
            else if (screenAspect == 2.21)
                asp = "221";
        }
        img = imgLoader.LoadIcon(*asp, 999, fontSmlHeight);
        if (img) {
            imageTop = fontHeight + (fontSmlHeight - img->Height())/2;
            left -= img->Width();
            iconsPixmap->DrawImage(cPoint(left, imageTop), *img);
            left -= marginItem*2;
        }
    }

    if (Config.RecordingResolutionAspectShow) {  // Show Resolution
        cString res("");
        switch (screenWidth) {
            case 7680:                        // 7680×4320 (UHD-2 / 8K)
                res = "7680x4320";
                break;
            case 3840:                        // 3840×2160 (UHD-1 / 4K)
                res = "3840x2160";
                break;
            case 1920:                        // 1920x1080 (HD1080 Full HDTV)
                res = "1920x1080";
                break;
            case 1440:                        // 1440x1080 (HD1080 DV)
                res = "1440x1080";
                break;
            case 1280:                        // 1280x720 (HD720)
                res = "1280x720";
                break;
            case 960:                         // 960x720 (HD720 DV)
                res = "960x720";
                break;
            case 704:                         // 704x576 (PAL)
                res = "704x576";
                break;
            case 720:                         // 720x576 (PAL)
                res = "720x576";
                break;
            case 544:                         // 544x576 (PAL)
                res = "544x576";
                break;
            case 528:                         // 528x576 (PAL)
                res = "528x576";
                break;
            case 480:                         // 480x576 (PAL SVCD)
                res = "480x576";
                break;
            case 352:                         // 352x576 (PAL CVD)
                res = "352x576";
                break;
            default:
                res = "unknown_res";         // TODO: Log resolution
                dsyslog("unkown resolution Width: %d Height: %d Aspect: %.2f\n", screenWidth, screenHeight, screenAspect);
                break;
            }

            //printf("Width: %d Height: %d Aspect: %.2f\n", screenWidth, screenHeight, screenAspect);

            img = imgLoader.LoadIcon(*res, 999, fontSmlHeight);
            if (img) {
                imageTop = fontHeight + (fontSmlHeight - img->Height())/2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, imageTop), *img);
                left -= marginItem*2;
            }
        }
        if (Config.RecordingFormatShow && !Config.RecordingSimpleAspectFormat) {
            cString iconName("");        // Show Format
            switch (screenWidth) {
                case 7680:
                case 3840:
                    iconName = "uhd";
                    break;
                case 1920:
                case 1440:
                case 1280:
                    iconName = "hd";
                    break;
                case 720:
                    iconName = "sd";
                    break;
                default:
                    iconName = "sd";
                    break;
            }

            img = imgLoader.LoadIcon(*iconName, 999, fontSmlHeight);
            if (img) {
                imageTop = fontHeight + (fontSmlHeight - img->Height())/2;
                left -= img->Width();
                iconsPixmap->DrawImage(cPoint(left, imageTop), *img);
                left -= marginItem*2 ;
            }
        }
    }
}

void cFlatDisplayReplay::SetMessage(eMessageType Type, const char *Text) {
    if (Text)
        MessageSet(Type, Text);
    else
        MessageClear();
}

void cFlatDisplayReplay::Flush(void) {
    TopBarUpdate();

    if( Config.RecordingResolutionAspectShow ) {
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
