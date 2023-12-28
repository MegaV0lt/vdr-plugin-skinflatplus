/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displaychannel.h"
#include "./flat.h"

cFlatDisplayChannel::cFlatDisplayChannel(bool WithInfo) {
    /* Disabled because all pixmaps are checked before use
    if (m_FirstDisplay) {
        m_FirstDisplay = false;
        m_DoOutput = false;
        return;
    } else
        m_DoOutput = true; */

    m_Present = NULL;
    // m_ChannelName = "";

    ChanInfoTopPixmap = NULL;
    ChanInfoBottomPixmap = NULL;
    ChanLogoPixmap = NULL;
    ChanLogoBGPixmap = NULL;
    ChanIconsPixmap = NULL;
    ChanEpgImagesPixmap = NULL;

    m_IsGroup = false;
    // IsRecording = false,  / Unused?
    m_IsRadioChannel = false;

    m_ScreenWidth = m_LastScreenWidth = -1;
    m_LastSignalStrength = -1;
    m_LastSignalQuality = -1;

    m_SignalStrengthRight = 0;

    CreateFullOsd();
    if (!osd) return;

    TopBarCreate();
    MessageCreate();

    m_ChannelWidth = m_OsdWidth - Config.decorBorderChannelSize * 2;
    m_ChannelHeight = m_OsdHeight - Config.decorBorderChannelSize * 2;
    // From bottom to top (2 * EPG + 2 * EPGsml)
    HeightBottom = (m_FontHeight * 2) + (m_FontSmlHeight * 2) + m_MarginItem;  // Top, Bottom, Between
    HeightImageLogo = HeightBottom;
    if (Config.SignalQualityShow)
        HeightBottom += std::max(m_FontSmlHeight, (Config.decorProgressSignalSize * 2) + m_MarginItem) + m_MarginItem;
    else if (Config.ChannelIconsShow)
        HeightBottom += m_FontSmlHeight + m_MarginItem;

    int HeightTop = m_FontHeight;

    int height = HeightBottom;
    ChanInfoBottomPixmap =
        CreatePixmap(osd, "ChanInfoBottomPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth, HeightBottom));
    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));

    ChanIconsPixmap =
        CreatePixmap(osd, "ChanIconsPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth, HeightBottom));
    PixmapFill(ChanIconsPixmap, clrTransparent);
    // Area for TVScraper images
    TVSLeft = 20 + Config.decorBorderChannelEPGSize;
    TVSTop = m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize;
    TVSWidth = m_OsdWidth - 40 - Config.decorBorderChannelEPGSize * 2;
    TVSHeight = m_OsdHeight - m_TopBarHeight - HeightBottom - 40 - Config.decorBorderChannelEPGSize * 2;

    ChanEpgImagesPixmap = CreatePixmap(osd, "ChanEpgImagesPixmap", 2, cRect(TVSLeft, TVSTop, TVSWidth, TVSHeight));
    PixmapFill(ChanEpgImagesPixmap, clrTransparent);

    ChanLogoBGPixmap =
        CreatePixmap(osd, "ChanLogoBGPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           HeightBottom * 2, HeightBottom * 2));
    PixmapFill(ChanLogoBGPixmap, clrTransparent);

    ChanLogoPixmap =
        CreatePixmap(osd, "ChanLogoPixmap", 3,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           HeightBottom * 2, HeightBottom * 2));
    PixmapFill(ChanLogoPixmap, clrTransparent);

    height += Config.decorProgressChannelSize + m_MarginItem * 2;
    ProgressBarCreate(Config.decorBorderChannelSize,
                      Config.decorBorderChannelSize + m_ChannelHeight - height + m_MarginItem, m_ChannelWidth,
                      Config.decorProgressChannelSize, m_MarginItem, 0, Config.decorProgressChannelFg,
                      Config.decorProgressChannelBarFg, Config.decorProgressChannelBg,
                      Config.decorProgressChannelType, true);

    ProgressBarDrawBgColor();

    height += HeightTop;
    ChanInfoTopPixmap =
        CreatePixmap(osd, "ChanInfoTopPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth, HeightTop));
    PixmapFill(ChanInfoTopPixmap, clrTransparent);

    Scrollers.SetOsd(osd);
    Scrollers.SetScrollStep(Config.ScrollerStep);
    Scrollers.SetScrollDelay(Config.ScrollerDelay);
    Scrollers.SetScrollType(Config.ScrollerType);

    if (Config.ChannelWeatherShow)
        DrawWidgetWeather();

    DecorBorderDraw(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                    m_ChannelWidth, HeightTop + HeightBottom + Config.decorProgressChannelSize + m_MarginItem * 2,
                    Config.decorBorderChannelSize, Config.decorBorderChannelType, Config.decorBorderChannelFg,
                    Config.decorBorderChannelBg);
}

cFlatDisplayChannel::~cFlatDisplayChannel() {
    // if (!m_DoOutput) return;

    if (osd) {
        Scrollers.Clear();

        osd->DestroyPixmap(ChanInfoTopPixmap);
        osd->DestroyPixmap(ChanInfoBottomPixmap);
        osd->DestroyPixmap(ChanLogoPixmap);
        osd->DestroyPixmap(ChanLogoBGPixmap);
        osd->DestroyPixmap(ChanIconsPixmap);
        osd->DestroyPixmap(ChanEpgImagesPixmap);
    }
}

void cFlatDisplayChannel::SetChannel(const cChannel *Channel, int Number) {
    if (!ChanIconsPixmap || !ChanInfoTopPixmap || !ChanLogoBGPixmap || !ChanLogoPixmap)
        return;

    // if (!m_DoOutput) return;

    // IsRecording = false;  // Unused?
    PixmapFill(ChanIconsPixmap, clrTransparent);
    m_LastScreenWidth = -1;

    cString ChannelName(""), ChannelNumber("");
    if (Channel) {
        m_IsRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;
        m_IsGroup = Channel->GroupSep();

        ChannelName = Channel->Name();
        if (!m_IsGroup)
            ChannelNumber = cString::sprintf("%d%s", Channel->Number(), Number ? "-" : "");
        else if (Number)
            ChannelNumber = cString::sprintf("%d-", Number);

        m_CurChannel = Channel;
    } else
        ChannelName = ChannelString(NULL, 0);

    cString ChannelString = cString::sprintf("%s  %s", *ChannelNumber, *ChannelName);

    PixmapFill(ChanInfoTopPixmap, Theme.Color(clrChannelBg));
    ChanInfoTopPixmap->DrawText(cPoint(50, 0), *ChannelString, Theme.Color(clrChannelFontTitle),
                                Theme.Color(clrChannelBg), m_Font);

    PixmapFill(ChanLogoPixmap, clrTransparent);
    PixmapFill(ChanLogoBGPixmap, clrTransparent);

    if (!m_IsGroup) {
        int ImageHeight = HeightImageLogo - m_MarginItem * 2;
        int ImageBgHeight = ImageHeight;
        int ImageBgWidth = ImageHeight;
        int ImageLeft = m_MarginItem * 2;
        int ImageTop = m_MarginItem;
        cImage *img = ImgLoader.LoadIcon("logo_background", ImageHeight * 1.34, ImageHeight);
        if (img) {
            ImageBgHeight = img->Height();
            ImageBgWidth = img->Width();
            ChanLogoBGPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }

        img = ImgLoader.LoadLogo(*ChannelName, ImageBgWidth - 4, ImageBgHeight - 4);
        if (img) {
            ImageTop = m_MarginItem + (ImageBgHeight - img->Height()) / 2;
            ImageLeft = m_MarginItem * 2 + (ImageBgWidth - img->Width()) / 2;
            ChanLogoPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        } else /*if (!m_IsGroup)*/ {  // Draw default logo
            img = ImgLoader.LoadIcon((m_IsRadioChannel) ? "radio" : "tv", ImageBgWidth - 10, ImageBgHeight - 10);
            if (img) {
                ImageTop = m_MarginItem + (ImageHeight - img->Height()) / 2;
                ImageLeft = m_MarginItem * 2 + (ImageBgWidth - img->Width()) / 2;
                ChanLogoPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
            }
        }
    }
}

void cFlatDisplayChannel::ChannelIconsDraw(const cChannel *Channel, bool Resolution) {
    // if (!m_DoOutput) return;
    if (!ChanIconsPixmap) return;

    // if (!Resolution)
        PixmapFill(ChanIconsPixmap, clrTransparent);

    int top = HeightBottom - m_FontSmlHeight - m_MarginItem;
    int ImageTop {0};
    int left = m_ChannelWidth - m_FontSmlHeight - m_MarginItem * 2;

    cImage *img {nullptr};
    if (Channel) {
        img = ImgLoader.LoadIcon((Channel->Ca()) ? "crypted" : "uncrypted", 999, m_FontSmlHeight);
        if (img) {
            ImageTop = top + (m_FontSmlHeight - img->Height()) / 2;
            ChanIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
            left -= m_MarginItem * 2;
        }
    }

    if (Resolution && !m_IsRadioChannel && m_ScreenWidth > 0) {
        cString IconName("");
        if (Config.ChannelResolutionAspectShow) {  // Show Aspect
            IconName = GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                ImageTop = top + (m_FontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem * 2;
            }

            IconName = GetScreenResolutionIcon(m_ScreenWidth, m_ScreenHeight, m_ScreenAspect);  // Show Resolution
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                ImageTop = top + (m_FontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem * 2;
            }
        }

        if (Config.ChannelFormatShow && !Config.ChannelSimpleAspectFormat) {
            IconName = GetFormatIcon(m_ScreenWidth);  // Show Format
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                ImageTop = top + (m_FontSmlHeight - img->Height()) / 2;
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, ImageTop), *img);
                left -= m_MarginItem * 2;
            }
        }
    }
}

void cFlatDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following) {
    // if (!m_DoOutput) return;
    if (!ChanInfoBottomPixmap || !ChanEpgImagesPixmap) return;

    m_Present = Present;
    cString EpgShort("");
    cString epg("");

    Scrollers.Clear();

    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));
    PixmapFill(ChanIconsPixmap, clrTransparent);

    bool IsRec = false;
    int RecWidth = m_FontSml->Width("REC");

    int left = HeightBottom * 1.34 + m_MarginItem;
    int StartTimeLeft = left;

    if (Config.ChannelShowStartTime)
        left += m_Font->Width("00:00  ");

    if (Present) {
        cString StartTime = Present->GetTimeString();
        cString EndTime = Present->GetEndTimeString();

        cString StrTime = cString::sprintf("%s - %s", *StartTime, *EndTime);
        int StrTimeWidth = m_FontSml->Width(*StrTime) + m_FontSml->Width("  ");

        epg = Present->Title();
        EpgShort = Present->ShortText();
        int EpgWidth = m_Font->Width(*epg) + m_MarginItem * 2;
        int EpgShortWidth = m_FontSml->Width(*EpgShort) + m_MarginItem * 2;

        if (Present->HasTimer()) {
            IsRec = true;
            EpgWidth += m_MarginItem + RecWidth;
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

        int SeenWidth = m_FontSml->Width(*seen) + m_FontSml->Width("  ");
        int MaxWidth = std::max(StrTimeWidth, SeenWidth);

        ChanInfoBottomPixmap->DrawText(cPoint(m_ChannelWidth - StrTimeWidth - m_MarginItem * 2, 0), *StrTime,
            Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), m_FontSml, StrTimeWidth, 0, taRight);
        ChanInfoBottomPixmap->DrawText(cPoint(m_ChannelWidth - SeenWidth - m_MarginItem * 2, m_FontSmlHeight), *seen,
                Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), m_FontSml, SeenWidth, 0, taRight);

        if (Config.ChannelShowStartTime) {
            ChanInfoBottomPixmap->DrawText(cPoint(StartTimeLeft, 0), *StartTime,
                                           Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), m_Font);
        }

        if ((EpgWidth > m_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            Scrollers.AddScroller(*epg, cRect(Config.decorBorderChannelSize + left,
                                              Config.decorBorderChannelSize+m_ChannelHeight - HeightBottom,
                                              m_ChannelWidth - left - MaxWidth,
                                              m_FontHeight), Theme.Color(clrChannelFontEpg), clrTransparent, m_Font);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, 0), *epg, Theme.Color(clrChannelFontEpg),
                                           Theme.Color(clrChannelBg), m_Font, m_ChannelWidth - left - MaxWidth);
        }

        if ((EpgShortWidth > m_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            Scrollers.AddScroller(*EpgShort, cRect(Config.decorBorderChannelSize + left,
                                  Config.decorBorderChannelSize+m_ChannelHeight - HeightBottom + m_FontHeight,
                                  m_ChannelWidth - left - MaxWidth,
                                  m_FontSmlHeight), Theme.Color(clrChannelFontEpg), clrTransparent, m_FontSml);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, m_FontHeight), *EpgShort, Theme.Color(clrChannelFontEpg),
                                           Theme.Color(clrChannelBg), m_FontSml, m_ChannelWidth - left - MaxWidth);
        }

        if (IsRec) {
            ChanInfoBottomPixmap->DrawText(cPoint(left + EpgWidth + m_MarginItem - RecWidth, 0), "REC",
                Theme.Color(clrChannelRecordingPresentFg), Theme.Color(clrChannelRecordingPresentBg), m_FontSml);
        }
    }

    if (Following) {
        IsRec = false;
        cString StartTime = Following->GetTimeString();
        cString EndTime = Following->GetEndTimeString();

        cString StrTime = cString::sprintf("%s - %s", *StartTime, *EndTime);
        int StrTimeWidth = m_FontSml->Width(*StrTime) + m_FontSml->Width("  ");

        epg = Following->Title();
        EpgShort = Following->ShortText();
        int EpgWidth = m_Font->Width(*epg) + m_MarginItem * 2;
        int EpgShortWidth = m_FontSml->Width(*EpgShort) + m_MarginItem * 2;

        if (Following->HasTimer()) {
            EpgWidth += m_MarginItem + RecWidth;
            IsRec = true;
        }

        cString dur = cString::sprintf("%d min", Following->Duration() / 60);
        int DurWidth = m_FontSml->Width(*dur) + m_FontSml->Width("  ");
        int MaxWidth = std::max(StrTimeWidth, DurWidth);

        ChanInfoBottomPixmap->DrawText(cPoint(m_ChannelWidth - StrTimeWidth - m_MarginItem * 2,
                                       m_FontHeight + m_FontSmlHeight), *StrTime, Theme.Color(clrChannelFontEpgFollow),
                                       Theme.Color(clrChannelBg), m_FontSml, StrTimeWidth, 0, taRight);
        ChanInfoBottomPixmap->DrawText(
            cPoint(m_ChannelWidth - DurWidth - m_MarginItem * 2, m_FontHeight + m_FontSmlHeight * 2), *dur,
            Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg), m_FontSml, DurWidth, 0, taRight);

        if (Config.ChannelShowStartTime)
            ChanInfoBottomPixmap->DrawText(cPoint(StartTimeLeft, m_FontHeight + m_FontSmlHeight), *StartTime,
                                           Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg), m_Font);

        if ((EpgWidth > m_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            Scrollers.AddScroller(*epg, cRect(Config.decorBorderChannelSize + left,
                                  Config.decorBorderChannelSize + m_ChannelHeight - HeightBottom + m_FontHeight
                                   + m_FontSmlHeight, m_ChannelWidth - left - MaxWidth, m_FontHeight),
                                   Theme.Color(clrChannelFontEpgFollow), clrTransparent, m_Font);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, m_FontHeight + m_FontSmlHeight), *epg,
                Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg), m_Font,
                m_ChannelWidth - left - MaxWidth);
        }

        if ((EpgShortWidth > m_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            Scrollers.AddScroller(*EpgShort, cRect(Config.decorBorderChannelSize + left,
                Config.decorBorderChannelSize+m_ChannelHeight - HeightBottom + m_FontHeight*2 + m_FontSmlHeight,
                m_ChannelWidth - left - MaxWidth, m_FontSmlHeight), Theme.Color(clrChannelFontEpgFollow),
                clrTransparent, m_FontSml);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, m_FontHeight*2 + m_FontSmlHeight), *EpgShort,
                                           Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg),
                                           m_FontSml, m_ChannelWidth - left - MaxWidth);
        }

        if (IsRec) {
            ChanInfoBottomPixmap->DrawText(cPoint(left + EpgWidth + m_MarginItem - RecWidth, m_FontHeight + m_FontSmlHeight),
                                           "REC", Theme.Color(clrChannelRecordingFollowFg),
                                           Theme.Color(clrChannelRecordingFollowBg), m_FontSml);
        }
    }

    if (Config.ChannelIconsShow && m_CurChannel)
        ChannelIconsDraw(m_CurChannel, false);

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
        cImage *img = ImgLoader.LoadFile(*MediaPath, MediaWidth, MediaHeight);
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

void cFlatDisplayChannel::SetMessage(eMessageType Type, const char *Text) {
    // if (!m_DoOutput) return;

    (Text) ? MessageSet(Type, Text) : MessageClear();
}

void cFlatDisplayChannel::SignalQualityDraw(void) {
    // if (!m_DoOutput) return;
    if (!ChanInfoBottomPixmap) return;

    int SignalStrength = cDevice::ActualDevice()->SignalStrength();
    int SignalQuality = cDevice::ActualDevice()->SignalQuality();
    if (m_LastSignalStrength == SignalStrength && m_LastSignalQuality == SignalQuality)
        return;

    m_LastSignalStrength = SignalStrength;
    m_LastSignalQuality = SignalQuality;

    cFont *SignalFont = cFont::CreateFont(Setup.FontOsd, Config.decorProgressSignalSize);

    int top = m_FontHeight * 2 + m_FontSmlHeight * 2 + m_MarginItem;
    top += std::max(m_FontSmlHeight, Config.decorProgressSignalSize) - (Config.decorProgressSignalSize * 2) - m_MarginItem;
    int left = m_MarginItem * 2;
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "STR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), SignalFont);
    int ProgressLeft = left + SignalFont->Width("STR ") + m_MarginItem;
    int SignalWidth = m_ChannelWidth / 2;
    int ProgressWidth = SignalWidth / 2 - ProgressLeft - m_MarginItem;
    ProgressBarDrawRaw(ChanInfoBottomPixmap, ChanInfoBottomPixmap,
                       cRect(ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize),
                       cRect(ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize), SignalStrength, 100,
                       Config.decorProgressSignalFg, Config.decorProgressSignalBarFg, Config.decorProgressSignalBg,
                       Config.decorProgressSignalType, false, Config.SignalQualityUseColors);

    // left = SignalWidth / 2 + m_MarginItem;
    top += Config.decorProgressSignalSize + m_MarginItem;
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "SNR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), SignalFont);
    ProgressLeft = left + SignalFont->Width("STR ") + m_MarginItem;
    // ProgressWidth = SignalWidth - ProgressLeft - m_MarginItem;

    ProgressBarDrawRaw(ChanInfoBottomPixmap, ChanInfoBottomPixmap,
                       cRect(ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize),
                       cRect(ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize), SignalQuality, 100,
                       Config.decorProgressSignalFg, Config.decorProgressSignalBarFg, Config.decorProgressSignalBg,
                       Config.decorProgressSignalType, false, Config.SignalQualityUseColors);

    m_SignalStrengthRight = ProgressLeft + ProgressWidth;

    delete SignalFont;
}

// You need oscam min rev 10653
// You need dvbapi min commit 85da7b2
void cFlatDisplayChannel::DvbapiInfoDraw(void) {
    // if (!m_DoOutput) return;
    if (!ChanInfoBottomPixmap || !ChanIconsPixmap) return;

    // dsyslog("flatPlus: DvbapiInfoDraw");
    static cPlugin *pDVBApi = cPluginManager::GetPlugin("dvbapi");
    if (!pDVBApi) return;

    sDVBAPIEcmInfo ecmInfo;
    ecmInfo.ecmtime = -1;
    ecmInfo.hops = -1;

    int ChannelSid = m_CurChannel->Sid();
    // dsyslog("flatPlus: ChannelSid: %d Channel: %s", ChannelSid, m_CurChannel->Name());

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

    int top = m_FontHeight * 2 + m_FontSmlHeight * 2 + m_MarginItem;
    top += std::max(m_FontSmlHeight, Config.decorProgressSignalSize) - (Config.decorProgressSignalSize * 2)
                     - m_MarginItem * 2;
    int left = m_SignalStrengthRight + m_MarginItem * 2;

    cFont *DvbapiInfoFont = cFont::CreateFont(Setup.FontOsd, (Config.decorProgressSignalSize * 2) + m_MarginItem);

    cString DvbapiInfoText = cString::sprintf("DVBAPI: ");
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), DvbapiInfoFont,
                                   DvbapiInfoFont->Width(*DvbapiInfoText) * 2);
    left += DvbapiInfoFont->Width(*DvbapiInfoText) + m_MarginItem;

    cString IconName = cString::sprintf("crypt_%s", *ecmInfo.cardsystem);
    cImage *img = ImgLoader.LoadIcon(*IconName, 999, DvbapiInfoFont->Height());
    if (img) {
        ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
        left += img->Width() + m_MarginItem;
    } else {
        IconName = "crypt_unknown";
        img = ImgLoader.LoadIcon(*IconName, 999, DvbapiInfoFont->Height());
        if (img) {
            ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
            left += img->Width() + m_MarginItem;
        }
        dsyslog("flatPlus: Unknown cardsystem: %s (CAID: %d)", *ecmInfo.cardsystem, ecmInfo.caid);
    }

    DvbapiInfoText = cString::sprintf(" %s (%d ms)", *ecmInfo.reader, ecmInfo.ecmtime);
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), DvbapiInfoFont,
                                   DvbapiInfoFont->Width(*DvbapiInfoText) * 2);
}

void cFlatDisplayChannel::Flush(void) {
    // if (!m_DoOutput) return;
    // if (!osd) return;

    int Current {0}, Total {0};
    if (m_Present) {
        time_t t = time(NULL);
        if (t > m_Present->StartTime())
            Current = t - m_Present->StartTime();
        Total = m_Present->Duration();
        ProgressBarDraw(Current, Total);
    }

    if (Config.SignalQualityShow)
        SignalQualityDraw();

    if (Config.ChannelIconsShow) {
        cDevice::PrimaryDevice()->GetVideoSize(m_ScreenWidth, m_ScreenHeight, m_ScreenAspect);
        if (m_ScreenWidth != m_LastScreenWidth) {
            m_LastScreenWidth = m_ScreenWidth;
            ChannelIconsDraw(m_CurChannel, true);
        }
    }

    if (Config.ChannelDvbapiInfoShow)
        DvbapiInfoDraw();

    TopBarUpdate();
    osd->Flush();
}

void cFlatDisplayChannel::PreLoadImages(void) {
    int height = (m_FontHeight * 2) + (m_FontSmlHeight * 2) + m_MarginItem - m_MarginItem * 2;
    int ImageBgHeight {height}, ImageBgWidth {height};
    ImgLoader.LoadIcon("logo_background", height, height);

    cImage *img = ImgLoader.LoadIcon("logo_background", height * 1.34, height);
    if (img) {
        ImageBgHeight = img->Height();
        ImageBgWidth = img->Width();
    }
    ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
    ImgLoader.LoadIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);

    int index {0};
#if VDRVERSNUM >= 20301
    LOCK_CHANNELS_READ;
    for (const cChannel *Channel = Channels->First(); Channel && index < LOGO_PRE_CACHE;
         Channel = Channels->Next(Channel)) {
#else
    for (cChannel *Channel = Channels.First(); Channel && index < LOGO_PRE_CACHE;
         Channel = Channels.Next(Channel)) {
#endif
        if (!Channel->GroupSep()) {  // Don't cache named channelgroup logo
            img = ImgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
            if (img)
                ++index;
        }
    }

    height = std::max(m_FontSmlHeight, Config.decorProgressSignalSize);
    ImgLoader.LoadIcon("crypted", 999, height);
    ImgLoader.LoadIcon("uncrypted", 999, height);
    ImgLoader.LoadIcon("unknown_asp", 999, height);
    ImgLoader.LoadIcon("43", 999, height);
    ImgLoader.LoadIcon("169", 999, height);
    ImgLoader.LoadIcon("169w", 999, height);
    ImgLoader.LoadIcon("221", 999, height);
    ImgLoader.LoadIcon("7680x4320", 999, height);
    ImgLoader.LoadIcon("3840x2160", 999, height);
    ImgLoader.LoadIcon("1920x1080", 999, height);
    ImgLoader.LoadIcon("1440x1080", 999, height);
    ImgLoader.LoadIcon("1280x720", 999, height);
    ImgLoader.LoadIcon("960x720", 999, height);
    ImgLoader.LoadIcon("704x576", 999, height);
    ImgLoader.LoadIcon("720x576", 999, height);
    ImgLoader.LoadIcon("544x576", 999, height);
    ImgLoader.LoadIcon("528x576", 999, height);
    ImgLoader.LoadIcon("480x576", 999, height);
    ImgLoader.LoadIcon("352x576", 999, height);
    ImgLoader.LoadIcon("unknown_res", 999, height);
    ImgLoader.LoadIcon("uhd", 999, height);
    ImgLoader.LoadIcon("hd", 999, height);
    ImgLoader.LoadIcon("sd", 999, height);
}
