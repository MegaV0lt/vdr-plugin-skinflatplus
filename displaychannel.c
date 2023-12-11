#include "displaychannel.h"
#include "flat.h"

cFlatDisplayChannel::cFlatDisplayChannel(bool WithInfo) {
    if (g_FirstDisplay) {
        g_FirstDisplay = false;
        g_DoOutput = false;
        return;
    } else
        g_DoOutput = true;

    present = NULL;
    g_ChannelName = "";
    ChanInfoTopPixmap = NULL;
    ChanInfoBottomPixmap = NULL;
    ChanLogoPixmap = NULL;
    ChanLogoBGPixmap = NULL;
    ChanIconsPixmap = NULL;
    ChanEpgImagesPixmap = NULL;

    IsGroup = false;
    IsRecording = false,
    IsRadioChannel = false;

    ScreenWidth = LastScreenWidth = -1;
    LastSignalStrength = -1;
    LastSignalQuality = -1;

    SignalStrengthRight = 0;

    CreateFullOsd();
    if (!osd) return;

    TopBarCreate();
    MessageCreate();

    g_ChannelWidth = g_OsdWidth - Config.decorBorderChannelSize * 2;
    g_ChannelHeight = g_OsdHeight - Config.decorBorderChannelSize * 2;
    // From bottom to top
    // 2 * EPG + 2 * EPGsml
    HeightBottom = (g_FontHight * 2) + (g_FontSmlHight * 2) + g_MarginItem;  // Top, Bottom, Between
    HeightImageLogo = HeightBottom;
    if (Config.SignalQualityShow)
        HeightBottom += std::max(g_FontSmlHight, (Config.decorProgressSignalSize * 2) + g_MarginItem) + g_MarginItem;
    else if (Config.ChannelIconsShow)
        HeightBottom += g_FontSmlHight + g_MarginItem;

    int HeightTop = g_FontHight;

    int height = HeightBottom;
    ChanInfoBottomPixmap =
        CreatePixmap(osd, "ChanInfoBottomPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + g_ChannelHeight - height,
                           g_ChannelWidth, HeightBottom));
    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));

    ChanIconsPixmap =
        CreatePixmap(osd, "ChanIconsPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + g_ChannelHeight - height,
                           g_ChannelWidth, HeightBottom));
    PixmapFill(ChanIconsPixmap, clrTransparent);
    // Area for TVScraper images
    TVSLeft = 20 + Config.decorBorderChannelEPGSize;
    TVSTop = g_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize;
    TVSWidth = g_OsdWidth - 40 - Config.decorBorderChannelEPGSize * 2;
    TVSHeight = g_OsdHeight - g_TopBarHeight - HeightBottom - 40 - Config.decorBorderChannelEPGSize * 2;

    ChanEpgImagesPixmap = CreatePixmap(osd, "ChanEpgImagesPixmap", 2, cRect(TVSLeft, TVSTop, TVSWidth, TVSHeight));
    PixmapFill(ChanEpgImagesPixmap, clrTransparent);

    ChanLogoBGPixmap =
        CreatePixmap(osd, "ChanLogoBGPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + g_ChannelHeight - height,
                           HeightBottom * 2, HeightBottom * 2));
    PixmapFill(ChanLogoBGPixmap, clrTransparent);

    ChanLogoPixmap =
        CreatePixmap(osd, "ChanLogoPixmap", 3,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + g_ChannelHeight - height,
                           HeightBottom * 2, HeightBottom * 2));
    PixmapFill(ChanLogoPixmap, clrTransparent);

    height += Config.decorProgressChannelSize + g_MarginItem * 2;
    ProgressBarCreate(Config.decorBorderChannelSize,
                      Config.decorBorderChannelSize + g_ChannelHeight - height + g_MarginItem, g_ChannelWidth,
                      Config.decorProgressChannelSize, g_MarginItem, 0, Config.decorProgressChannelFg,
                      Config.decorProgressChannelBarFg, Config.decorProgressChannelBg,
                      Config.decorProgressChannelType, true);

    ProgressBarDrawBgColor();

    height += HeightTop;
    ChanInfoTopPixmap =
        CreatePixmap(osd, "ChanInfoTopPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + g_ChannelHeight - height,
                           g_ChannelWidth, HeightTop));
    PixmapFill(ChanInfoTopPixmap, clrTransparent);

    scrollers.SetOsd(osd);
    scrollers.SetScrollStep(Config.ScrollerStep);
    scrollers.SetScrollDelay(Config.ScrollerDelay);
    scrollers.SetScrollType(Config.ScrollerType);

    if (Config.ChannelWeatherShow)
        DrawWidgetWeather();

    DecorBorderDraw(Config.decorBorderChannelSize, Config.decorBorderChannelSize + g_ChannelHeight - height, g_ChannelWidth,
                    HeightTop + HeightBottom + Config.decorProgressChannelSize + g_MarginItem * 2,
                    Config.decorBorderChannelSize, Config.decorBorderChannelType, Config.decorBorderChannelFg,
                    Config.decorBorderChannelBg);
}

cFlatDisplayChannel::~cFlatDisplayChannel() {
    if (!g_DoOutput) return;

    if (osd) {
        scrollers.Clear();

        if (ChanInfoTopPixmap)
            osd->DestroyPixmap(ChanInfoTopPixmap);
        if (ChanInfoBottomPixmap)
            osd->DestroyPixmap(ChanInfoBottomPixmap);
        if (ChanLogoPixmap)
            osd->DestroyPixmap(ChanLogoPixmap);
        if (ChanLogoBGPixmap)
            osd->DestroyPixmap(ChanLogoBGPixmap);
        if (ChanIconsPixmap)
            osd->DestroyPixmap(ChanIconsPixmap);
        if (ChanEpgImagesPixmap)
            osd->DestroyPixmap(ChanEpgImagesPixmap);
    }
}

void cFlatDisplayChannel::SetChannel(const cChannel *Channel, int Number) {
    if (!g_DoOutput) return;

    IsRecording = false;
    PixmapFill(ChanIconsPixmap, clrTransparent);
    LastScreenWidth = -1;

    cString channelNumber("");
    if (Channel) {
        IsRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;
        IsGroup = Channel->GroupSep();

        g_ChannelName = Channel->Name();
        if (!IsGroup)
            channelNumber = cString::sprintf("%d%s", Channel->Number(), Number ? "-" : "");
        else if (Number)
            channelNumber = cString::sprintf("%d-", Number);

        CurChannel = Channel;
    } else
        g_ChannelName = ChannelString(NULL, 0);

    cString channelString = cString::sprintf("%s  %s", *channelNumber, *g_ChannelName);

    PixmapFill(ChanInfoTopPixmap, Theme.Color(clrChannelBg));
    ChanInfoTopPixmap->DrawText(cPoint(50, 0), *channelString, Theme.Color(clrChannelFontTitle),
                                Theme.Color(clrChannelBg), g_Font);

    PixmapFill(ChanLogoPixmap, clrTransparent);
    PixmapFill(ChanLogoBGPixmap, clrTransparent);
    int ImageHeight = HeightImageLogo - g_MarginItem * 2;
    int ImageBgHeight = ImageHeight;
    int ImageBgWidth = ImageHeight;
    int ImageLeft = g_MarginItem * 2;
    int ImageTop = g_MarginItem;
    cImage *img = imgLoader.LoadIcon("logo_background", ImageHeight * 1.34, ImageHeight);
    if (img) {
        ImageBgHeight = img->Height();
        ImageBgWidth = img->Width();
        ChanLogoBGPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    }

    img = imgLoader.LoadLogo(*g_ChannelName, ImageBgWidth - 4, ImageBgHeight - 4);
    if (img) {
        ImageTop = g_MarginItem + (ImageBgHeight - img->Height()) / 2;
        ImageLeft = g_MarginItem * 2 + (ImageBgWidth - img->Width()) / 2;
        ChanLogoPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
    } else if (!IsGroup) {  // Draw default logo
        img = imgLoader.LoadIcon((IsRadioChannel) ? "radio" : "tv", ImageBgWidth - 10, ImageBgHeight - 10);
        if (img) {
            ImageTop = g_MarginItem + (ImageHeight - img->Height()) / 2;
            ImageLeft = g_MarginItem * 2 + (ImageBgWidth - img->Width()) / 2;
            ChanLogoPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }
    }
}

void cFlatDisplayChannel::ChannelIconsDraw(const cChannel *Channel, bool Resolution) {
    if (!g_DoOutput) return;

    // if (!Resolution)
        PixmapFill(ChanIconsPixmap, clrTransparent);

    int top = HeightBottom - g_FontSmlHight - g_MarginItem;
    int ImageTop {0};
    int left = g_ChannelWidth - g_FontSmlHight - g_MarginItem * 2;

    cImage *img = NULL;
    if (Channel) {
        img = imgLoader.LoadIcon((Channel->Ca()) ? "crypted" : "uncrypted", 999, g_FontSmlHight);
        if (img) {
            ImageTop = top + (g_FontSmlHight - img->Height()) / 2;
            ChanIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left -= g_MarginItem * 2;
        }
    }

    if (Resolution && !IsRadioChannel && ScreenWidth > 0) {
        cString IconName("");
        if (Config.ChannelResolutionAspectShow) {  // Show Aspect
            IconName = GetAspectIcon(ScreenWidth, ScreenAspect);
            img = imgLoader.LoadIcon(*IconName, 999, g_FontSmlHight);
            if (img) {
                ImageTop = top + (g_FontSmlHight - img->Height()) / 2;
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= g_MarginItem * 2;
            }

            IconName = GetScreenResolutionIcon(ScreenWidth, ScreenHeight, ScreenAspect);  // Show Resolution
            img = imgLoader.LoadIcon(*IconName, 999, g_FontSmlHight);
            if (img) {
                ImageTop = top + (g_FontSmlHight - img->Height()) / 2;
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= g_MarginItem * 2;
            }
        }

        if (Config.ChannelFormatShow && !Config.ChannelSimpleAspectFormat) {
            IconName = GetFormatIcon(ScreenWidth);  // Show Format
            img = imgLoader.LoadIcon(*IconName, 999, g_FontSmlHight);
            if (img) {
                ImageTop = top + (g_FontSmlHight - img->Height()) / 2;
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= g_MarginItem * 2;
            }
        }
    }
}

void cFlatDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following) {
    if (!g_DoOutput) return;

    present = Present;
    cString EpgShort("");
    cString epg("");

    scrollers.Clear();

    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));
    PixmapFill(ChanIconsPixmap, clrTransparent);

    bool IsRec = false;
    int RecWidth = g_FontSml->Width("REC");

    int left = HeightBottom * 1.34 + g_MarginItem;
    int StartTimeLeft = left;

    if (Config.ChannelShowStartTime)
        left += g_Font->Width("00:00  ");

    if (Present) {
        cString StartTime = Present->GetTimeString();
        cString EndTime = Present->GetEndTimeString();

        cString StrTime = cString::sprintf("%s - %s", *StartTime, *EndTime);
        int StrTimeWidth = g_FontSml->Width(*StrTime) + g_FontSml->Width("  ");

        epg = Present->Title();
        EpgShort = Present->ShortText();
        int EpgWidth = g_Font->Width(*epg) + g_MarginItem * 2;
        int EpgShortWidth = g_FontSml->Width(*EpgShort) + g_MarginItem * 2;

        if (Present->HasTimer()) {
            IsRec = true;
            EpgWidth += g_MarginItem + RecWidth;
        }

        int s = (time(NULL) - Present->StartTime()) / 60;
        int dur = Present->Duration() / 60;
        int sleft = dur - s;

        cString seen("");
        if (Config.ChannelTimeLeft == 0)
            seen = cString::sprintf("%d-/%d+ %d min", s, sleft, dur);
        else if (Config.ChannelTimeLeft == 1)
            seen = cString::sprintf("%d- %d min", s, dur);
        else if (Config.ChannelTimeLeft == 2)
            seen = cString::sprintf("%d+ %d min", sleft, dur);

        int SeenWidth = g_FontSml->Width(*seen) + g_FontSml->Width("  ");
        int MaxWidth = std::max(StrTimeWidth, SeenWidth);

        ChanInfoBottomPixmap->DrawText(cPoint(g_ChannelWidth - StrTimeWidth - g_MarginItem * 2, 0), *StrTime,
            Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), g_FontSml, StrTimeWidth, 0, taRight);
        ChanInfoBottomPixmap->DrawText(cPoint(g_ChannelWidth - SeenWidth - g_MarginItem * 2, g_FontSmlHight), *seen,
                Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), g_FontSml, SeenWidth, 0, taRight);

        if (Config.ChannelShowStartTime) {
            ChanInfoBottomPixmap->DrawText(cPoint(StartTimeLeft, 0), *StartTime,
                                           Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), g_Font);
        }

        if ((EpgWidth > g_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            scrollers.AddScroller(*epg, cRect(Config.decorBorderChannelSize + left,
                                              Config.decorBorderChannelSize+g_ChannelHeight - HeightBottom,
                                              g_ChannelWidth - left - MaxWidth,
                                              g_FontHight), Theme.Color(clrChannelFontEpg), clrTransparent, g_Font);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, 0), *epg, Theme.Color(clrChannelFontEpg),
                                           Theme.Color(clrChannelBg), g_Font, g_ChannelWidth - left - MaxWidth);
        }

        if ((EpgShortWidth > g_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            scrollers.AddScroller(*EpgShort, cRect(Config.decorBorderChannelSize + left,
                                  Config.decorBorderChannelSize+g_ChannelHeight - HeightBottom + g_FontHight,
                                  g_ChannelWidth - left - MaxWidth,
                                  g_FontSmlHight), Theme.Color(clrChannelFontEpg), clrTransparent, g_FontSml);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, g_FontHight), *EpgShort, Theme.Color(clrChannelFontEpg),
                                           Theme.Color(clrChannelBg), g_FontSml, g_ChannelWidth - left - MaxWidth);
        }

        if (IsRec) {
            ChanInfoBottomPixmap->DrawText(cPoint(left + EpgWidth + g_MarginItem - RecWidth, 0), "REC",
                Theme.Color(clrChannelRecordingPresentFg), Theme.Color(clrChannelRecordingPresentBg), g_FontSml);
        }
    }

    if (Following) {
        IsRec = false;
        cString StartTime = Following->GetTimeString();
        cString EndTime = Following->GetEndTimeString();

        cString StrTime = cString::sprintf("%s - %s", *StartTime, *EndTime);
        int StrTimeWidth = g_FontSml->Width(*StrTime) + g_FontSml->Width("  ");

        epg = Following->Title();
        EpgShort = Following->ShortText();
        int EpgWidth = g_Font->Width(*epg) + g_MarginItem * 2;
        int EpgShortWidth = g_FontSml->Width(*EpgShort) + g_MarginItem * 2;

        if (Following->HasTimer()) {
            EpgWidth += g_MarginItem + RecWidth;
            IsRec = true;
        }

        cString dur = cString::sprintf("%d min", Following->Duration() / 60);
        int DurWidth = g_FontSml->Width(*dur) + g_FontSml->Width("  ");
        int MaxWidth = std::max(StrTimeWidth, DurWidth);

        ChanInfoBottomPixmap->DrawText(cPoint(g_ChannelWidth - StrTimeWidth - g_MarginItem * 2,
                                       g_FontHight + g_FontSmlHight), *StrTime, Theme.Color(clrChannelFontEpgFollow),
                                       Theme.Color(clrChannelBg), g_FontSml, StrTimeWidth, 0, taRight);
        ChanInfoBottomPixmap->DrawText(cPoint(g_ChannelWidth - DurWidth - g_MarginItem * 2, g_FontHight + g_FontSmlHight * 2),
                                       *dur, Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg),
                                       g_FontSml, DurWidth, 0, taRight);

        if (Config.ChannelShowStartTime)
            ChanInfoBottomPixmap->DrawText(cPoint(StartTimeLeft, g_FontHight + g_FontSmlHight), *StartTime,
                                           Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg), g_Font);

        if ((EpgWidth > g_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            scrollers.AddScroller(*epg, cRect(Config.decorBorderChannelSize + left,
                                  Config.decorBorderChannelSize + g_ChannelHeight - HeightBottom + g_FontHight
                                   + g_FontSmlHight, g_ChannelWidth - left - MaxWidth, g_FontHight),
                                   Theme.Color(clrChannelFontEpgFollow), clrTransparent, g_Font);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, g_FontHight + g_FontSmlHight), *epg,
                Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg), g_Font, g_ChannelWidth - left - MaxWidth);
        }

        if ((EpgShortWidth > g_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            scrollers.AddScroller(*EpgShort, cRect(Config.decorBorderChannelSize + left,
                Config.decorBorderChannelSize+g_ChannelHeight - HeightBottom + g_FontHight*2 + g_FontSmlHight,
                g_ChannelWidth - left - MaxWidth, g_FontSmlHight), Theme.Color(clrChannelFontEpgFollow),
                clrTransparent, g_FontSml);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, g_FontHight*2 + g_FontSmlHight), *EpgShort,
                                           Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg),
                                           g_FontSml, g_ChannelWidth - left - MaxWidth);
        }

        if (IsRec) {
            ChanInfoBottomPixmap->DrawText(cPoint(left + EpgWidth + g_MarginItem - RecWidth, g_FontHight + g_FontSmlHight),
                                           "REC", Theme.Color(clrChannelRecordingFollowFg),
                                           Theme.Color(clrChannelRecordingFollowBg), g_FontSml);
        }
    }

    if (Config.ChannelIconsShow && CurChannel)
        ChannelIconsDraw(CurChannel, false);

    cString MediaPath("");
    int MediaWidth {0}, MediaHeight {0};

    static cPlugin *pScraper = GetScraperPlugin();
    if (Config.TVScraperChanInfoShowPoster && pScraper) {
        ScraperGetPosterBannerV2 call;
        call.event = Present;
        call.recording = NULL;
        if (pScraper->Service("GetPosterBannerV2", &call)) {
            if ((call.type == tSeries) && call.banner.path.size() > 0) {
                MediaWidth = call.banner.width * Config.TVScraperChanInfoPosterSize * 100;
                MediaHeight = call.banner.height * Config.TVScraperChanInfoPosterSize * 100;
                MediaPath = call.banner.path.c_str();
            } else if (call.type == tMovie && call.poster.path.size() > 0) {
                MediaWidth = call.poster.width * Config.TVScraperChanInfoPosterSize * 100;
                MediaHeight = call.poster.height * Config.TVScraperChanInfoPosterSize * 100;
                MediaPath = call.poster.path.c_str();
            }
        }
    }

    PixmapFill(ChanEpgImagesPixmap, clrTransparent);
    DecorBorderClearByFrom(BorderTVSPoster);
    if (!isempty(*MediaPath)) {
        if (MediaHeight > TVSHeight || MediaWidth > TVSWidth) {  // Resize too big poster/banner
            dsyslog("flatPlus: Poster/Banner size (%d x %d) is too big!", MediaWidth, MediaHeight);
            MediaHeight = TVSHeight * 0.7 * Config.TVScraperChanInfoPosterSize * 100;  // Max 70% of pixmap height
            if (Config.ChannelWeatherShow)
                // Max 50% of pixmap width. Aspect is preserved in LoadFile()
                MediaWidth = TVSWidth * 0.5 * Config.TVScraperChanInfoPosterSize * 100;
            else
                // Max 70% of pixmap width. Aspect is preserved in LoadFile()
                MediaWidth = TVSWidth * 0.7 * Config.TVScraperChanInfoPosterSize * 100;

            dsyslog("flatPlus: Poster/Banner resized to max %d x %d", MediaWidth, MediaHeight);
        }
        cImage *img = imgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);
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

void cFlatDisplayChannel::SetMessage(eMessageType Type, const char *Text) {
    if (!g_DoOutput) return;

    (Text) ? MessageSet(Type, Text) : MessageClear();
}

void cFlatDisplayChannel::SignalQualityDraw(void) {
    if (!g_DoOutput) return;

    int SignalStrength = cDevice::ActualDevice()->SignalStrength();
    int SignalQuality = cDevice::ActualDevice()->SignalQuality();
    if (LastSignalStrength == SignalStrength && LastSignalQuality == SignalQuality)
        return;

    LastSignalStrength = SignalStrength;
    LastSignalQuality = SignalQuality;

    cFont *SignalFont = cFont::CreateFont(Setup.FontOsd, Config.decorProgressSignalSize);

    int top = g_FontHight * 2 + g_FontSmlHight * 2 + g_MarginItem;
    top += std::max(g_FontSmlHight, Config.decorProgressSignalSize) - (Config.decorProgressSignalSize * 2) - g_MarginItem;
    int left = g_MarginItem * 2;
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "STR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), SignalFont);
    int ProgressLeft = left + SignalFont->Width("STR ") + g_MarginItem;
    int SignalWidth = g_ChannelWidth / 2;
    int ProgressWidth = SignalWidth / 2 - ProgressLeft - g_MarginItem;
    ProgressBarDrawRaw(ChanInfoBottomPixmap, ChanInfoBottomPixmap,
                       cRect(ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize),
                       cRect(ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize), SignalStrength, 100,
                       Config.decorProgressSignalFg, Config.decorProgressSignalBarFg, Config.decorProgressSignalBg,
                       Config.decorProgressSignalType, false, Config.SignalQualityUseColors);

    // left = SignalWidth / 2 + g_MarginItem;
    top += Config.decorProgressSignalSize + g_MarginItem;
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "SNR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), SignalFont);
    ProgressLeft = left + SignalFont->Width("STR ") + g_MarginItem;
    // ProgressWidth = SignalWidth - ProgressLeft - g_MarginItem;

    ProgressBarDrawRaw(ChanInfoBottomPixmap, ChanInfoBottomPixmap,
                       cRect(ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize),
                       cRect(ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize), SignalQuality, 100,
                       Config.decorProgressSignalFg, Config.decorProgressSignalBarFg, Config.decorProgressSignalBg,
                       Config.decorProgressSignalType, false, Config.SignalQualityUseColors);

    SignalStrengthRight = ProgressLeft + ProgressWidth;

    delete SignalFont;
}

// You need oscam min rev 10653
// You need dvbapi min commit 85da7b2
void cFlatDisplayChannel::DvbapiInfoDraw(void) {
    if (!g_DoOutput) return;

    // dsyslog("flatPlus: DvbapiInfoDraw");
    static cPlugin *pDVBApi = cPluginManager::GetPlugin("dvbapi");
    if (!pDVBApi) return;

    sDVBAPIEcmInfo ecmInfo;
    ecmInfo.ecmtime = -1;
    ecmInfo.hops = -1;

    int ChannelSid = CurChannel->Sid();
    // dsyslog("flatPlus: ChannelSid: %d Channel: %s", ChannelSid, CurChannel->Name());

    ecmInfo.sid = ChannelSid;
    if (!pDVBApi->Service("GetEcmInfo", &ecmInfo)) return;
/*
    dsyslog("flatPlus: caid: %d", ecmInfo.caid);
    dsyslog("flatPlus: cardsystem: %s", *ecmInfo.cardsystem);
    dsyslog("flatPlus: reader: %s", *ecmInfo.reader);
    dsyslog("flatPlus: from: %s", *ecmInfo.from);
    dsyslog("flatPlus: protocol: %s", *ecmInfo.protocol);
*/
    if (ecmInfo.hops < 0 || ecmInfo.ecmtime <= 0 || ecmInfo.ecmtime > 9999)
        return;

    int top = g_FontHight * 2 + g_FontSmlHight * 2 + g_MarginItem;
    top += std::max(g_FontSmlHight, Config.decorProgressSignalSize) - (Config.decorProgressSignalSize * 2)
                     - g_MarginItem * 2;
    int left = SignalStrengthRight + g_MarginItem * 2;

    cFont *DvbapiInfoFont = cFont::CreateFont(Setup.FontOsd, (Config.decorProgressSignalSize * 2) + g_MarginItem);

    cString DvbapiInfoText = cString::sprintf("DVBAPI: ");
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), DvbapiInfoFont,
                                   DvbapiInfoFont->Width(*DvbapiInfoText) * 2);
    left += DvbapiInfoFont->Width(*DvbapiInfoText) + g_MarginItem;

    cString IconName = cString::sprintf("crypt_%s", *ecmInfo.cardsystem);
    cImage *img = imgLoader.LoadIcon(*IconName, 999, DvbapiInfoFont->Height());
    if (img) {
        ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
        left += img->Width() + g_MarginItem;
    } else {
        IconName = "crypt_unknown";
        img = imgLoader.LoadIcon(*IconName, 999, DvbapiInfoFont->Height());
        if (img) {
            ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
            left += img->Width() + g_MarginItem;
        }
        dsyslog("flatPlus: Unknown cardsystem: %s (CAID: %d)", *ecmInfo.cardsystem, ecmInfo.caid);
    }

    DvbapiInfoText = cString::sprintf(" %s (%d ms)", *ecmInfo.reader, ecmInfo.ecmtime);
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), DvbapiInfoFont,
                                   DvbapiInfoFont->Width(*DvbapiInfoText) * 2);
}

void cFlatDisplayChannel::Flush(void) {
    if (!g_DoOutput) return;

    int Current {0}, Total {0};
    if (present) {
        time_t t = time(NULL);
        if (t > present->StartTime())
            Current = t - present->StartTime();
        Total = present->Duration();
        ProgressBarDraw(Current, Total);
    }

    if (Config.SignalQualityShow)
        SignalQualityDraw();

    if (Config.ChannelIconsShow) {
        cDevice::PrimaryDevice()->GetVideoSize(ScreenWidth, ScreenHeight, ScreenAspect);
        if (ScreenWidth != LastScreenWidth) {
            LastScreenWidth = ScreenWidth;
            ChannelIconsDraw(CurChannel, true);
        }
    }

    if (Config.ChannelDvbapiInfoShow)
        DvbapiInfoDraw();

    TopBarUpdate();
    osd->Flush();
}

void cFlatDisplayChannel::PreLoadImages(void) {
    int height = (g_FontHight * 2) + (g_FontSmlHight * 2) + g_MarginItem - g_MarginItem * 2;
    int ImageBgHeight {height}, ImageBgWidth {height};
    imgLoader.LoadIcon("logo_background", height, height);

    cImage *img = imgLoader.LoadIcon("logo_background", height * 1.34, height);
    if (img) {
        ImageBgHeight = img->Height();
        ImageBgWidth = img->Width();
    }
    imgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
    imgLoader.LoadIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);

    int index {0};
#if VDRVERSNUM >= 20301
    LOCK_CHANNELS_READ;
    for (const cChannel *Channel = Channels->First(); Channel && index < LOGO_PRE_CACHE;
         Channel = Channels->Next(Channel)) {
#else
    for (cChannel *Channel = Channels.First(); Channel && index < LOGO_PRE_CACHE;
         Channel = Channels.Next(Channel)) {
#endif
        img = imgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
        if (img) ++index;
    }

    height = std::max(g_FontSmlHight, Config.decorProgressSignalSize);
    imgLoader.LoadIcon("crypted", 999, height);
    imgLoader.LoadIcon("uncrypted", 999, height);
    imgLoader.LoadIcon("unknown_asp", 999, height);
    imgLoader.LoadIcon("43", 999, height);
    imgLoader.LoadIcon("169", 999, height);
    imgLoader.LoadIcon("169w", 999, height);
    imgLoader.LoadIcon("221", 999, height);
    imgLoader.LoadIcon("7680x4320", 999, height);
    imgLoader.LoadIcon("3840x2160", 999, height);
    imgLoader.LoadIcon("1920x1080", 999, height);
    imgLoader.LoadIcon("1440x1080", 999, height);
    imgLoader.LoadIcon("1280x720", 999, height);
    imgLoader.LoadIcon("960x720", 999, height);
    imgLoader.LoadIcon("704x576", 999, height);
    imgLoader.LoadIcon("720x576", 999, height);
    imgLoader.LoadIcon("544x576", 999, height);
    imgLoader.LoadIcon("528x576", 999, height);
    imgLoader.LoadIcon("480x576", 999, height);
    imgLoader.LoadIcon("352x576", 999, height);
    imgLoader.LoadIcon("unknown_res", 999, height);
    imgLoader.LoadIcon("uhd", 999, height);
    imgLoader.LoadIcon("hd", 999, height);
    imgLoader.LoadIcon("sd", 999, height);
}
