#include "displaychannel.h"
#include "flat.h"

cFlatDisplayChannel::cFlatDisplayChannel(bool WithInfo) {
    if (firstDisplay) {
        firstDisplay = false;
        doOutput = false;
        return;
    } else
        doOutput = true;

    present = NULL;
    channelName = "";
    chanInfoTopPixmap = NULL;
    chanInfoBottomPixmap = NULL;
    chanLogoPixmap = NULL;
    chanLogoBGPixmap = NULL;
    chanIconsPixmap = NULL;
    chanEpgImagesPixmap = NULL;

    isGroup = false;
    isRecording = false,
    isRadioChannel = false;

    screenWidth = lastScreenWidth = -1;
    LastSignalStrength = -1;
    LastSignalQuality = -1;

    SignalStrengthRight = 0;

    CreateFullOsd();
    if (!osd)
        return;

    TopBarCreate();
    MessageCreate();

    channelWidth = osdWidth - Config.decorBorderChannelSize * 2;
    channelHeight = osdHeight - Config.decorBorderChannelSize * 2;
    // From bottom to top
    // 2 * EPG + 2 * EPGsml
    heightBottom = (fontHeight * 2) + (fontSmlHeight * 2) + marginItem;  // Top, Bottom, Between
    heightImageLogo = heightBottom;
    if (Config.SignalQualityShow)
        heightBottom += std::max(fontSmlHeight, (Config.decorProgressSignalSize * 2) + marginItem) + marginItem;
    else if (Config.ChannelIconsShow)
        heightBottom += fontSmlHeight + marginItem;

    int heightTop = fontHeight;

    int height = heightBottom;
    chanInfoBottomPixmap =
        CreatePixmap(osd, "chanInfoBottomPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + channelHeight - height,
                           channelWidth, heightBottom));
    PixmapFill(chanInfoBottomPixmap, Theme.Color(clrChannelBg));

    chanIconsPixmap =
        CreatePixmap(osd, "chanIconsPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + channelHeight - height,
                           channelWidth, heightBottom));
    PixmapFill(chanIconsPixmap, clrTransparent);

    TVSLeft = 20 + Config.decorBorderChannelEPGSize;
    TVSTop = topBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize;
    TVSWidth = osdWidth - 40 - Config.decorBorderChannelEPGSize * 2;
    TVSHeight = osdHeight - topBarHeight - heightBottom - 40 - Config.decorBorderChannelEPGSize * 2;

    chanEpgImagesPixmap = CreatePixmap(osd, "chanEpgImagesPixmap", 2, cRect(TVSLeft, TVSTop, TVSWidth, TVSHeight));
    PixmapFill(chanEpgImagesPixmap, clrTransparent);

    chanLogoBGPixmap =
        CreatePixmap(osd, "chanLogoBGPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + channelHeight - height,
                           heightBottom * 2, heightBottom * 2));
    PixmapFill(chanLogoBGPixmap, clrTransparent);

    chanLogoPixmap =
        CreatePixmap(osd, "chanLogoPixmap", 3,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + channelHeight - height,
                           heightBottom * 2, heightBottom * 2));
    PixmapFill(chanLogoPixmap, clrTransparent);

    height += Config.decorProgressChannelSize + marginItem * 2;
    ProgressBarCreate(Config.decorBorderChannelSize,
                      Config.decorBorderChannelSize + channelHeight - height + marginItem, channelWidth,
                      Config.decorProgressChannelSize, marginItem, 0, Config.decorProgressChannelFg,
                      Config.decorProgressChannelBarFg, Config.decorProgressChannelBg,
                      Config.decorProgressChannelType, true);

    ProgressBarDrawBgColor();

    height += heightTop;
    chanInfoTopPixmap =
        CreatePixmap(osd, "chanInfoTopPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + channelHeight - height,
                           channelWidth, heightTop));
    PixmapFill(chanInfoTopPixmap, clrTransparent);

    scrollers.SetOsd(osd);
    scrollers.SetScrollStep(Config.ScrollerStep);
    scrollers.SetScrollDelay(Config.ScrollerDelay);
    scrollers.SetScrollType(Config.ScrollerType);

    if (Config.ChannelWeatherShow)
        DrawWidgetWeather();

    DecorBorderDraw(Config.decorBorderChannelSize, Config.decorBorderChannelSize + channelHeight - height, channelWidth,
                    heightTop + heightBottom + Config.decorProgressChannelSize + marginItem * 2,
                    Config.decorBorderChannelSize, Config.decorBorderChannelType, Config.decorBorderChannelFg,
                    Config.decorBorderChannelBg);
}

cFlatDisplayChannel::~cFlatDisplayChannel() {
    if (!doOutput)
        return;

    if (osd) {
        scrollers.Clear();

        if (chanInfoTopPixmap)
            osd->DestroyPixmap(chanInfoTopPixmap);
        if (chanInfoBottomPixmap)
            osd->DestroyPixmap(chanInfoBottomPixmap);
        if (chanLogoPixmap)
            osd->DestroyPixmap(chanLogoPixmap);
        if (chanLogoBGPixmap)
            osd->DestroyPixmap(chanLogoBGPixmap);
        if (chanIconsPixmap)
            osd->DestroyPixmap(chanIconsPixmap);
        if (chanEpgImagesPixmap)
            osd->DestroyPixmap(chanEpgImagesPixmap);
    }
}

void cFlatDisplayChannel::SetChannel(const cChannel *Channel, int Number) {
    if (!doOutput)
        return;

    cString channelNumber("");
    isRecording = false;
    PixmapFill(chanIconsPixmap, clrTransparent);
    lastScreenWidth = -1;

    if (Channel) {
        isRadioChannel = ((!Channel->Vpid())&&(Channel->Apid(0))) ? true : false;
        isGroup = Channel->GroupSep();

        channelName = Channel->Name();
        if (!Channel->GroupSep())
            channelNumber = cString::sprintf("%d%s", Channel->Number(), Number ? "-" : "");
        else if (Number)
            channelNumber = cString::sprintf("%d-", Number);

        CurChannel = Channel;
    } else
        channelName = ChannelString(NULL, 0);

    cString channelString = cString::sprintf("%s  %s", *channelNumber, *channelName);

    PixmapFill(chanInfoTopPixmap, Theme.Color(clrChannelBg));
    chanInfoTopPixmap->DrawText(cPoint(50, 0), channelString, Theme.Color(clrChannelFontTitle),
                                Theme.Color(clrChannelBg), font);

    PixmapFill(chanLogoPixmap, clrTransparent);
    PixmapFill(chanLogoBGPixmap, clrTransparent);
    int imageHeight = heightImageLogo - marginItem * 2;
    int imageBGHeight = imageHeight;
    int imageBGWidth = imageHeight;
    int imageLeft = marginItem * 2;
    int imageTop = marginItem;
    cImage *imgBG = imgLoader.LoadIcon("logo_background", imageHeight * 1.34, imageHeight);
    if (imgBG) {
        imageBGHeight = imgBG->Height();
        imageBGWidth = imgBG->Width();
        chanLogoBGPixmap->DrawImage(cPoint(imageLeft, imageTop), *imgBG);
    }

    cImage *img = imgLoader.LoadLogo(*channelName, imageBGWidth - 4, imageBGHeight - 4);
    if (img) {
        imageTop = marginItem + (imageBGHeight - img->Height()) / 2;
        imageLeft = marginItem * 2 + (imageBGWidth - img->Width()) / 2;
        chanLogoPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
    } else if (!isGroup) {  // Draw default logo
        img = imgLoader.LoadIcon((isRadioChannel) ? "radio" : "tv", imageBGWidth - 10, imageBGHeight - 10);
        if (img) {
            imageTop = marginItem + (imageHeight - img->Height()) / 2;
            imageLeft = marginItem * 2 + (imageBGWidth - img->Width()) / 2;
            chanLogoPixmap->DrawImage(cPoint(imageLeft, imageTop), *img);
        }
    }
}

void cFlatDisplayChannel::ChannelIconsDraw(const cChannel *Channel, bool Resolution) {
    if (!doOutput)
        return;

    if (!Resolution)
        PixmapFill(chanIconsPixmap, clrTransparent);

    // int width = fontSmlHeight;
    // int height = fontSmlHeight;
    int top = heightBottom - fontSmlHeight - marginItem;
    int imageTop = 0;
    cImage *img = NULL;

    int left = channelWidth - fontSmlHeight - marginItem * 2;

    if (Channel) {
        img = imgLoader.LoadIcon((Channel->Ca()) ? "crypted" : "uncrypted", 999, fontSmlHeight);
        if (img) {
            imageTop = top + (fontSmlHeight - img->Height()) / 2;
            chanIconsPixmap->DrawImage(cPoint(left, imageTop), *img);
            left -= marginItem * 2;
        }
    }

    if (Resolution && !isRadioChannel && screenWidth > 0) {
        if (Config.ChannelResolutionAspectShow) {  // Show Aspect
            cString asp = GetAspectIcon(screenWidth, screenAspect);
            img = imgLoader.LoadIcon(*asp, 999, fontSmlHeight);
            if (img) {
                imageTop = top + (fontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                chanIconsPixmap->DrawImage(cPoint(left, imageTop), *img);
                left -= marginItem * 2;
            }

            cString res = GetScreenResolutionIcon(screenWidth, screenHeight, screenAspect);  // Show Resolution
            img = imgLoader.LoadIcon(*res, 999, fontSmlHeight);
            if (img) {
                imageTop = top + (fontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                chanIconsPixmap->DrawImage(cPoint(left, imageTop), *img);
                left -= marginItem * 2;
            }
        }

        if (Config.ChannelFormatShow && !Config.ChannelSimpleAspectFormat) {
            cString iconName = GetFormatIcon(screenWidth);  // Show Format
            img = imgLoader.LoadIcon(*iconName, 999, fontSmlHeight);
            if (img) {
                imageTop = top + (fontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                chanIconsPixmap->DrawImage(cPoint(left, imageTop), *img);
                left -= marginItem * 2;
            }
        }
    }
}

void cFlatDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following) {
    if (!doOutput)
        return;

    present = Present;
    cString epgShort("");
    cString epg("");

    scrollers.Clear();

    PixmapFill(chanInfoBottomPixmap, Theme.Color(clrChannelBg));
    PixmapFill(chanIconsPixmap, clrTransparent);

    bool isRec = false;
    int RecWidth = fontSml->Width("REC");

    int left = heightBottom * 1.34 + marginItem;
    int StartTimeLeft = left;

    if (Config.ChannelShowStartTime) {
        left += font->Width("00:00  ");
    }

    if (Present) {
        cString startTime = Present->GetTimeString();
        cString endTime = Present->GetEndTimeString();

        cString timeString = cString::sprintf("%s - %s", *startTime, *endTime);
        int timeStringWidth = fontSml->Width(*timeString) + fontSml->Width("  ");

        int epgWidth = font->Width(Present->Title()) + marginItem * 2;
        int epgShortWidth = fontSml->Width(Present->ShortText()) + marginItem * 2;

        if (Present->HasTimer()) {
            isRec = true;
            epgWidth += marginItem + RecWidth;
        }

        int s = static_cast<int>(time(NULL) - Present->StartTime()) / 60;
        int sleft = (Present->Duration() / 60) - s;

        cString seen("");
        if (Config.ChannelTimeLeft == 0)
            seen = cString::sprintf("%d-/%d+ %d min", s, sleft, Present->Duration() / 60);
        else if (Config.ChannelTimeLeft == 1)
            seen = cString::sprintf("%d- %d min", s, Present->Duration() / 60);
        else if (Config.ChannelTimeLeft == 2)
            seen = cString::sprintf("%d+ %d min", sleft, Present->Duration() / 60);

        int seenWidth = fontSml->Width(*seen) + fontSml->Width("  ");

        epg = Present->Title();
        epgShort = Present->ShortText();
        int maxWidth = std::max(timeStringWidth, seenWidth);

        chanInfoBottomPixmap->DrawText(cPoint(channelWidth - timeStringWidth - marginItem * 2, 0), *timeString,
            Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), fontSml, timeStringWidth, 0, taRight);
        chanInfoBottomPixmap->DrawText(cPoint(channelWidth - seenWidth - marginItem * 2, fontSmlHeight), *seen,
                Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), fontSml, seenWidth, 0, taRight);

        if (Config.ChannelShowStartTime) {
            chanInfoBottomPixmap->DrawText(cPoint(StartTimeLeft, 0), *startTime,
                                           Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), font);
        }

        if ((epgWidth > channelWidth - left - maxWidth) && Config.ScrollerEnable) {
            scrollers.AddScroller(*epg, cRect(Config.decorBorderChannelSize + left,
                                              Config.decorBorderChannelSize+channelHeight - heightBottom,
                                              channelWidth - left - maxWidth,
                                              fontHeight), Theme.Color(clrChannelFontEpg), clrTransparent, font);
        } else {
            chanInfoBottomPixmap->DrawText(cPoint(left, 0), *epg, Theme.Color(clrChannelFontEpg),
                                           Theme.Color(clrChannelBg), font, channelWidth - left - maxWidth);
        }

        if ((epgShortWidth > channelWidth - left - maxWidth) && Config.ScrollerEnable) {
            scrollers.AddScroller(*epgShort, cRect(Config.decorBorderChannelSize + left,
                                  Config.decorBorderChannelSize+channelHeight - heightBottom + fontHeight,
                                  channelWidth - left - maxWidth,
                                  fontSmlHeight), Theme.Color(clrChannelFontEpg), clrTransparent, fontSml);
        } else {
            chanInfoBottomPixmap->DrawText(cPoint(left, fontHeight), *epgShort, Theme.Color(clrChannelFontEpg),
                                           Theme.Color(clrChannelBg), fontSml, channelWidth - left - maxWidth);
        }

        if (isRec) {
            chanInfoBottomPixmap->DrawText(cPoint(left + epgWidth + marginItem - RecWidth, 0), "REC",
                Theme.Color(clrChannelRecordingPresentFg), Theme.Color(clrChannelRecordingPresentBg), fontSml);
        }
    }

    if (Following) {
        isRec = false;
        cString startTime = Following->GetTimeString();
        cString endTime = Following->GetEndTimeString();

        cString timeString = cString::sprintf("%s - %s", *startTime, *endTime);
        int timeStringWidth = fontSml->Width(*timeString) + fontSml->Width("  ");

        int epgWidth = font->Width(Following->Title()) + marginItem * 2;
        int epgShortWidth = fontSml->Width(Following->ShortText()) + marginItem * 2;

        if (Following->HasTimer()) {
            epgWidth += marginItem + RecWidth;
            isRec = true;
        }

        cString dur = cString::sprintf("%d min", Following->Duration() / 60);
        int durWidth = fontSml->Width(*dur) + fontSml->Width("  ");

        int maxWidth = std::max(timeStringWidth, durWidth);

        epg = Following->Title();
        epgShort = Following->ShortText();

        chanInfoBottomPixmap->DrawText(cPoint(channelWidth - timeStringWidth - marginItem * 2,
                                       fontHeight + fontSmlHeight), *timeString, Theme.Color(clrChannelFontEpgFollow),
                                       Theme.Color(clrChannelBg), fontSml, timeStringWidth, 0, taRight);
        chanInfoBottomPixmap->DrawText(cPoint(channelWidth - durWidth - marginItem * 2, fontHeight + fontSmlHeight * 2),
                                       *dur, Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg),
                                       fontSml, durWidth, 0, taRight);

        if (Config.ChannelShowStartTime)
            chanInfoBottomPixmap->DrawText(cPoint(StartTimeLeft, fontHeight + fontSmlHeight), *startTime,
                                           Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg), font);

        if ((epgWidth > channelWidth - left - maxWidth) && Config.ScrollerEnable) {
            scrollers.AddScroller(*epg, cRect(Config.decorBorderChannelSize + left,
                                  Config.decorBorderChannelSize + channelHeight - heightBottom + fontHeight
                                   + fontSmlHeight, channelWidth - left - maxWidth, fontHeight),
                                   Theme.Color(clrChannelFontEpgFollow), clrTransparent, font);
        } else {
            chanInfoBottomPixmap->DrawText(cPoint(left, fontHeight + fontSmlHeight), *epg,
                Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg), font, channelWidth - left - maxWidth);
        }

        if ((epgShortWidth > channelWidth - left - maxWidth) && Config.ScrollerEnable) {
            scrollers.AddScroller(*epgShort, cRect(Config.decorBorderChannelSize + left,
                Config.decorBorderChannelSize+channelHeight - heightBottom + fontHeight*2 + fontSmlHeight,
                channelWidth - left - maxWidth, fontSmlHeight), Theme.Color(clrChannelFontEpgFollow),
                clrTransparent, fontSml);
        } else {
            chanInfoBottomPixmap->DrawText(cPoint(left, fontHeight*2 + fontSmlHeight), *epgShort,
                                           Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg),
                                           fontSml, channelWidth - left - maxWidth);
        }

        if (isRec) {
            chanInfoBottomPixmap->DrawText(cPoint(left + epgWidth + marginItem - RecWidth, fontHeight + fontSmlHeight),
                                           "REC", Theme.Color(clrChannelRecordingFollowFg),
                                           Theme.Color(clrChannelRecordingFollowBg), fontSml);
        }
    }

    if (Config.ChannelIconsShow && CurChannel)
        ChannelIconsDraw(CurChannel, false);

    std::string mediaPath("");
    int mediaWidth = 0;
    int mediaHeight = 0;

    static cPlugin *pScraper = GetScraperPlugin();
    if (Config.TVScraperChanInfoShowPoster && pScraper) {
        ScraperGetPosterBannerV2 call;
        call.event = Present;
        call.recording = NULL;
        if (pScraper->Service("GetPosterBannerV2", &call)) {
            if ((call.type == tSeries) && call.banner.path.size() > 0) {
                mediaWidth = call.banner.width * Config.TVScraperChanInfoPosterSize * 100;
                mediaHeight = call.banner.height * Config.TVScraperChanInfoPosterSize * 100;
                mediaPath = call.banner.path;
            } else if (call.type == tMovie && call.poster.path.size() > 0) {
                mediaWidth = call.poster.width * 0.5 * Config.TVScraperChanInfoPosterSize * 100;
                mediaHeight = call.poster.height * 0.5 * Config.TVScraperChanInfoPosterSize * 100;
                mediaPath = call.poster.path;
            }
        }
    }

    PixmapFill(chanEpgImagesPixmap, clrTransparent);
    DecorBorderClearByFrom(BorderTVSPoster);
    if (mediaPath.length() > 0) {
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

void cFlatDisplayChannel::SetMessage(eMessageType Type, const char *Text) {
    if (!doOutput)
        return;

    (Text) ? MessageSet(Type, Text) : MessageClear();
}

void cFlatDisplayChannel::SignalQualityDraw(void) {
    if (!doOutput)
        return;

    int SignalStrength = cDevice::ActualDevice()->SignalStrength();
    int SignalQuality = cDevice::ActualDevice()->SignalQuality();
    int signalWidth = channelWidth / 2;

    if (LastSignalStrength == SignalStrength && LastSignalQuality == SignalQuality)
        return;

    LastSignalStrength = SignalStrength;
    LastSignalQuality = SignalQuality;

    cFont *SignalFont = cFont::CreateFont(Setup.FontOsd, Config.decorProgressSignalSize);

    int top = fontHeight*2 + fontSmlHeight*2 + marginItem;
    top += std::max(fontSmlHeight, Config.decorProgressSignalSize) - (Config.decorProgressSignalSize*2) - marginItem;
    int left = marginItem * 2;
    /*
    int progressTop = fontHeight*2 + fontSmlHeight*2 + marginItem;
    progressTop += std::max(fontSmlHeight, Config.decorProgressSignalSize) / 2 - Config.decorProgressSignalSize / 2;
    */
    int progressTop = top;

    chanInfoBottomPixmap->DrawText(cPoint(left, top), "STR",
        Theme.Color(clrChannelSignalFont), Theme.Color(clrChannelBg), SignalFont);
    int progressLeft = left + SignalFont->Width("STR") + SignalFont->Width(" ") + marginItem;
    int progressWidth = signalWidth / 2 - progressLeft - marginItem;
    ProgressBarDrawRaw(chanInfoBottomPixmap, chanInfoBottomPixmap, cRect(progressLeft, progressTop, progressWidth,
                       Config.decorProgressSignalSize), cRect(progressLeft, progressTop, progressWidth,
                       Config.decorProgressSignalSize), SignalStrength, 100, Config.decorProgressSignalFg,
                       Config.decorProgressSignalBarFg, Config.decorProgressSignalBg, Config.decorProgressSignalType,
                       false, Config.SignalQualityUseColors);

    // left = signalWidth / 2 + marginItem;
    top += Config.decorProgressSignalSize + marginItem;
    progressTop = top;

    chanInfoBottomPixmap->DrawText(cPoint(left, top), "SNR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), SignalFont);
    progressLeft = left + SignalFont->Width("STR") + SignalFont->Width(" ") + marginItem;
    // progressWidth = signalWidth - progressLeft - marginItem;

    ProgressBarDrawRaw(chanInfoBottomPixmap, chanInfoBottomPixmap, cRect(progressLeft, progressTop, progressWidth,
                       Config.decorProgressSignalSize), cRect(progressLeft, progressTop, progressWidth,
                       Config.decorProgressSignalSize), SignalQuality, 100, Config.decorProgressSignalFg,
                       Config.decorProgressSignalBarFg, Config.decorProgressSignalBg, Config.decorProgressSignalType,
                       false, Config.SignalQualityUseColors);

    SignalStrengthRight = progressLeft + progressWidth;

    delete SignalFont;
}

// You need oscam min rev 10653
// You need dvbapi min commit 85da7b2
void cFlatDisplayChannel::DvbapiInfoDraw(void) {
    if (!doOutput)
        return;

    // dsyslog("DvbapiInfoDraw");
    static cPlugin *pDVBApi = cPluginManager::GetPlugin("dvbapi");
    if (!pDVBApi)
        return;

    sDVBAPIEcmInfo ecmInfo;
    ecmInfo.ecmtime = -1;
    ecmInfo.hops = -1;

    int ChannelSid = CurChannel->Sid();
    // dsyslog("ChannelSid: %d Channel: %s", ChannelSid, CurChannel->Name());

    ecmInfo.sid = ChannelSid;
    if (!pDVBApi->Service("GetEcmInfo", &ecmInfo))
        return;
/*
    dsyslog("caid: %d", ecmInfo.caid);
    dsyslog("cardsystem: %s", *ecmInfo.cardsystem);
    dsyslog("reader: %s", *ecmInfo.reader);
    dsyslog("from: %s", *ecmInfo.from);
    dsyslog("protocol: %s", *ecmInfo.protocol);
*/
    if (ecmInfo.hops < 0 || ecmInfo.ecmtime <= 0)
        return;

    if (ecmInfo.ecmtime > 9999)
        return;

    int top = fontHeight * 2 + fontSmlHeight * 2 + marginItem;
    top += std::max(fontSmlHeight, Config.decorProgressSignalSize) - (Config.decorProgressSignalSize * 2)
                     - marginItem * 2;
    int left = SignalStrengthRight + marginItem * 2;

    cFont *dvbapiInfoFont = cFont::CreateFont(Setup.FontOsd, (Config.decorProgressSignalSize * 2) + marginItem);

    cString dvbapiInfoText = cString::sprintf("DVBAPI: ");
    chanInfoBottomPixmap->DrawText(cPoint(left, top), dvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), dvbapiInfoFont,
                                   dvbapiInfoFont->Width(dvbapiInfoText) * 2);
    left += dvbapiInfoFont->Width(dvbapiInfoText) + marginItem;

    cImage *img = NULL;
    cString iconName = cString::sprintf("crypt_%s", *ecmInfo.cardsystem);
    img = imgLoader.LoadIcon(*iconName, 999, dvbapiInfoFont->Height());
    if (img) {
        chanIconsPixmap->DrawImage(cPoint(left, top), *img);
        left += img->Width() + marginItem;
    } else {
        iconName = "crypt_unknown";
        img = imgLoader.LoadIcon(*iconName, 999, dvbapiInfoFont->Height());
        if (img) {
            chanIconsPixmap->DrawImage(cPoint(left, top), *img);
            left += img->Width() + marginItem;
        }
        dsyslog("Unknown cardsystem: %s (CAID: %d)", *ecmInfo.cardsystem, ecmInfo.caid);
    }

    dvbapiInfoText = cString::sprintf(" %s (%d ms)", *ecmInfo.reader, ecmInfo.ecmtime);
    chanInfoBottomPixmap->DrawText(cPoint(left, top), dvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), dvbapiInfoFont,
                                   dvbapiInfoFont->Width(dvbapiInfoText) * 2);
}

void cFlatDisplayChannel::Flush(void) {
    if (!doOutput)
        return;

    int Current = 0, Total = 0;
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
        cDevice::PrimaryDevice()->GetVideoSize(screenWidth, screenHeight, screenAspect);
        if (screenWidth != lastScreenWidth) {
            lastScreenWidth = screenWidth;
            ChannelIconsDraw(CurChannel, true);
        }
    }

    if (Config.ChannelDvbapiInfoShow)
        DvbapiInfoDraw();

    TopBarUpdate();
    osd->Flush();
}

void cFlatDisplayChannel::PreLoadImages(void) {
    int height = (fontHeight * 2) + (fontSmlHeight * 2) + marginItem - marginItem * 2;
    imgLoader.LoadIcon("logo_background", height, height);
    int imageBGHeight, imageBGWidth;
    imageBGHeight = imageBGWidth = height;

    cImage *imgBG = imgLoader.LoadIcon("logo_background", height * 1.34, height);
    if (imgBG) {
        imageBGHeight = imgBG->Height();
        imageBGWidth = imgBG->Width();
    }
    imgLoader.LoadIcon("radio", imageBGWidth - 10, imageBGHeight - 10);
    imgLoader.LoadIcon("tv", imageBGWidth - 10, imageBGHeight - 10);

    int index = 0;
    // height = ((fontHeight * 2) + (fontSmlHeight * 2) + marginItem) - marginItem * 2;  // Double calculation of height
    cImage *img = NULL;
#if VDRVERSNUM >= 20301
    LOCK_CHANNELS_READ;
    for (const cChannel *Channel = Channels->First(); Channel && index < LOGO_PRE_CACHE;
         Channel = Channels->Next(Channel)) {
#else
    for (cChannel *Channel = Channels.First(); Channel && index < LOGO_PRE_CACHE;
         Channel = Channels.Next(Channel)) {
#endif
        img = imgLoader.LoadLogo(Channel->Name(), imageBGWidth - 4, imageBGHeight - 4);
        if (img)
            ++index;
    }

    height = std::max(fontSmlHeight, Config.decorProgressSignalSize);
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
