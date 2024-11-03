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
    CreateFullOsd();
    // if (!m_Osd) return;

    TopBarCreate();
    MessageCreate();

    m_ChannelWidth = m_OsdWidth - Config.decorBorderChannelSize * 2;
    m_ChannelHeight = m_OsdHeight - Config.decorBorderChannelSize * 2;
    // From bottom to top (2 * EPG + 2 * EPGsml)
    HeightBottom = m_FontHeight2 + (m_FontSmlHeight * 2) + m_MarginItem;  // Top, Bottom, Between
    HeightImageLogo = HeightBottom;
    if (Config.SignalQualityShow)
        HeightBottom += std::max(m_FontSmlHeight, (Config.decorProgressSignalSize * 2) + m_MarginItem) + m_MarginItem;
    else if (Config.ChannelIconsShow)
        HeightBottom += m_FontSmlHeight + m_MarginItem;

    const int HeightTop {m_FontHeight};

    int height {HeightBottom};
    ChanInfoBottomPixmap =
        CreatePixmap(m_Osd, "ChanInfoBottomPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth, HeightBottom));
    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));

    ChanIconsPixmap =
        CreatePixmap(m_Osd, "ChanIconsPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth, HeightBottom));
    PixmapFill(ChanIconsPixmap, clrTransparent);
    // Area for TVScraper images
    TVSRect.Set(20 + Config.decorBorderChannelEPGSize,
                m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize,
                m_OsdWidth - 40 - Config.decorBorderChannelEPGSize * 2,
                m_OsdHeight - m_TopBarHeight - HeightBottom - 40 - Config.decorBorderChannelEPGSize * 2);

    ChanEpgImagesPixmap = CreatePixmap(m_Osd, "ChanEpgImagesPixmap", 2, TVSRect);
    PixmapFill(ChanEpgImagesPixmap, clrTransparent);

    ChanLogoBGPixmap =
        CreatePixmap(m_Osd, "ChanLogoBGPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           HeightBottom * 2, HeightBottom * 2));
    PixmapFill(ChanLogoBGPixmap, clrTransparent);

    ChanLogoPixmap =
        CreatePixmap(m_Osd, "ChanLogoPixmap", 3,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           HeightBottom * 2, HeightBottom * 2));
    PixmapFill(ChanLogoPixmap, clrTransparent);

    height += Config.decorProgressChannelSize + m_MarginItem2;
    ProgressBarCreate(cRect(Config.decorBorderChannelSize,
                            Config.decorBorderChannelSize + m_ChannelHeight - height + m_MarginItem,
                            m_ChannelWidth, Config.decorProgressChannelSize),
                      m_MarginItem, 0, Config.decorProgressChannelFg,
                      Config.decorProgressChannelBarFg, Config.decorProgressChannelBg,
                      Config.decorProgressChannelType, true);

    ProgressBarDrawBgColor();

    height += HeightTop;
    ChanInfoTopPixmap =
        CreatePixmap(m_Osd, "ChanInfoTopPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth, HeightTop));
    PixmapFill(ChanInfoTopPixmap, clrTransparent);

    Scrollers.SetOsd(m_Osd);
    Scrollers.SetScrollStep(Config.ScrollerStep);
    Scrollers.SetScrollDelay(Config.ScrollerDelay);
    Scrollers.SetScrollType(Config.ScrollerType);

    if (Config.ChannelWeatherShow)
        DrawWidgetWeather();

    const sDecorBorder ib {Config.decorBorderChannelSize,
                           Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth,
                           HeightTop + HeightBottom + Config.decorProgressChannelSize + m_MarginItem2,
                           Config.decorBorderChannelSize,
                           Config.decorBorderChannelType,
                           Config.decorBorderChannelFg,
                           Config.decorBorderChannelBg};
    DecorBorderDraw(ib);
}

cFlatDisplayChannel::~cFlatDisplayChannel() {
    // if (m_Osd) {
        Scrollers.Clear();
        m_Osd->DestroyPixmap(ChanInfoTopPixmap);
        m_Osd->DestroyPixmap(ChanInfoBottomPixmap);
        m_Osd->DestroyPixmap(ChanLogoPixmap);
        m_Osd->DestroyPixmap(ChanLogoBGPixmap);
        m_Osd->DestroyPixmap(ChanIconsPixmap);
        m_Osd->DestroyPixmap(ChanEpgImagesPixmap);
    // }
}

void cFlatDisplayChannel::SetChannel(const cChannel *Channel, int Number) {
    if (!ChanIconsPixmap || !ChanInfoTopPixmap || !ChanLogoBGPixmap || !ChanLogoPixmap)
        return;

    PixmapFill(ChanIconsPixmap, clrTransparent);
    m_LastScreenWidth = -1;

    bool IsGroup {false};
    cString ChannelName {""}, ChannelNumber {""};
    if (Channel) {
        m_IsRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;
        IsGroup = Channel->GroupSep();

        ChannelName = Channel->Name();
        if (!IsGroup)
            ChannelNumber = cString::sprintf("%d%s", Channel->Number(), (Number) ? "-" : "");
        else if (Number)
            ChannelNumber = cString::sprintf("%d-", Number);

        m_CurChannel = Channel;
    } else {
        ChannelName = ChannelString(NULL, 0);
    }
    const cString ChannelString = cString::sprintf("%s  %s", *ChannelNumber, *ChannelName);

    PixmapFill(ChanInfoTopPixmap, Theme.Color(clrChannelBg));
    ChanInfoTopPixmap->DrawText(cPoint(50, 0), *ChannelString, Theme.Color(clrChannelFontTitle),
                                Theme.Color(clrChannelBg), m_Font);

    PixmapFill(ChanLogoPixmap, clrTransparent);
    PixmapFill(ChanLogoBGPixmap, clrTransparent);

    if (!IsGroup) {
        const int ImageHeight {HeightImageLogo - m_MarginItem2};
        int ImageBgHeight {ImageHeight};
        int ImageBgWidth {ImageHeight};
        int ImageLeft {m_MarginItem2};
        int ImageTop {m_MarginItem};
        cImage *img {ImgLoader.LoadIcon("logo_background", ImageHeight * 1.34, ImageHeight)};
        if (img) {
            ImageBgHeight = img->Height();
            ImageBgWidth = img->Width();
            ChanLogoBGPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }

        img = ImgLoader.LoadLogo(*ChannelName, ImageBgWidth - 4, ImageBgHeight - 4);
        if (img) {
            ImageTop = m_MarginItem + (ImageBgHeight - img->Height()) / 2;
            ImageLeft = m_MarginItem2 + (ImageBgWidth - img->Width()) / 2;
            ChanLogoPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        } else {  // Draw default logo
            img = ImgLoader.LoadIcon((m_IsRadioChannel) ? "radio" : "tv", ImageBgWidth - 10, ImageBgHeight - 10);
            if (img) {
                ImageTop = m_MarginItem + (ImageHeight - img->Height()) / 2;
                ImageLeft = m_MarginItem2 + (ImageBgWidth - img->Width()) / 2;
                ChanLogoPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
            }
        }
    }
}

void cFlatDisplayChannel::ChannelIconsDraw(const cChannel *Channel, bool Resolution) {
    if (!ChanIconsPixmap) return;

    // if (!Resolution)
        PixmapFill(ChanIconsPixmap, clrTransparent);

    const int top {HeightBottom - m_FontSmlHeight - m_MarginItem};
    int left {m_ChannelWidth - m_FontSmlHeight - m_MarginItem2};

    cImage *img {nullptr};
    if (Channel) {
        img = ImgLoader.LoadIcon((Channel->Ca()) ? "crypted" : "uncrypted", 999, m_FontSmlHeight);
        if (img) {
            ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
            left -= m_MarginItem2;
        }
    }

    if (Resolution && !m_IsRadioChannel && m_ScreenWidth > 0) {
        cString IconName {""};
        if (Config.ChannelResolutionAspectShow) {  // Show Aspect
            IconName = *GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                left -= m_MarginItem2;
            }

            IconName = *GetScreenResolutionIcon(m_ScreenWidth, m_ScreenHeight);  // Show Resolution
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                left -= m_MarginItem2;
            }
        }

        if (Config.ChannelFormatShow && !Config.ChannelSimpleAspectFormat) {
            IconName = *GetFormatIcon(m_ScreenWidth);  // Show Format
            img = ImgLoader.LoadIcon(*IconName, 999, m_FontSmlHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                left -= m_MarginItem2;
            }
        }
    }
}

void cFlatDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following) {
    if (!ChanInfoBottomPixmap || !ChanEpgImagesPixmap) return;

    m_Present = Present;
    cString EpgShort {""};
    cString epg {""};

    Scrollers.Clear();

    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));
    PixmapFill(ChanIconsPixmap, clrTransparent);

    bool IsRec {false};
    const int RecWidth {m_FontSml->Width("REC")};

    int left = HeightBottom * 1.34 + m_MarginItem;  // Narrowing conversion
    const int StartTimeLeft {left};

    if (Config.ChannelShowStartTime)
        left += m_Font->Width("00:00  ");

    if (Present) {
        const cString StartTime = *Present->GetTimeString();
        const cString StrTime = cString::sprintf("%s - %s", *StartTime, *Present->GetEndTimeString());
        const int StrTimeWidth {m_FontSml->Width(*StrTime) + m_FontSml->Width("  ")};

        epg = Present->Title();
        EpgShort = Present->ShortText();
        int EpgWidth {m_Font->Width(*epg) + m_MarginItem2};
        const int EpgShortWidth {m_FontSml->Width(*EpgShort) + m_MarginItem2};

        if (Present->HasTimer()) {
            IsRec = true;
            EpgWidth += m_MarginItem + RecWidth;
        }

        const int s = (time(0) - Present->StartTime()) / 60;  // Narrowing conversion
        const int dur {Present->Duration() / 60};
        const int sleft {dur - s};

        cString seen {""};
        switch (Config.ChannelTimeLeft) {
        case 0:
            seen = cString::sprintf("%d-/%d+ %d min", s, sleft, dur);
            break;
        case 1:
            seen = cString::sprintf("%d- %d min", s, dur);
            break;
        case 2:
            seen = cString::sprintf("%d+ %d min", sleft, dur);
            break;
        }


        const int SeenWidth {m_FontSml->Width(*seen) + m_FontSml->Width("  ")};
        const int MaxWidth {std::max(StrTimeWidth, SeenWidth)};

        ChanInfoBottomPixmap->DrawText(cPoint(m_ChannelWidth - StrTimeWidth - m_MarginItem2, 0), *StrTime,
            Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), m_FontSml, StrTimeWidth, 0, taRight);
        ChanInfoBottomPixmap->DrawText(cPoint(m_ChannelWidth - SeenWidth - m_MarginItem2, m_FontSmlHeight), *seen,
                Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), m_FontSml, SeenWidth, 0, taRight);

        if (Config.ChannelShowStartTime) {
            ChanInfoBottomPixmap->DrawText(cPoint(StartTimeLeft, 0), *StartTime,
                                           Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), m_Font);
        }

        if ((EpgWidth > m_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            Scrollers.AddScroller(*epg, cRect(Config.decorBorderChannelSize + left,
                                              Config.decorBorderChannelSize + m_ChannelHeight - HeightBottom,
                                              m_ChannelWidth - left - MaxWidth,
                                              m_FontHeight), Theme.Color(clrChannelFontEpg), clrTransparent, m_Font);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, 0), *epg, Theme.Color(clrChannelFontEpg),
                                           Theme.Color(clrChannelBg), m_Font, m_ChannelWidth - left - MaxWidth);
        }

        if ((EpgShortWidth > m_ChannelWidth - left - MaxWidth) && Config.ScrollerEnable) {
            Scrollers.AddScroller(*EpgShort, cRect(Config.decorBorderChannelSize + left,
                                  Config.decorBorderChannelSize + m_ChannelHeight - HeightBottom + m_FontHeight,
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
    }  // Present

    if (Following) {
        IsRec = false;
        const cString StartTime = *Following->GetTimeString();
        const cString StrTime = cString::sprintf("%s - %s", *StartTime, *Following->GetEndTimeString());
        const int StrTimeWidth {m_FontSml->Width(*StrTime) + m_FontSml->Width("  ")};

        epg = Following->Title();
        EpgShort = Following->ShortText();
        int EpgWidth {m_Font->Width(*epg) + m_MarginItem2};
        const int EpgShortWidth {m_FontSml->Width(*EpgShort) + m_MarginItem2};

        if (Following->HasTimer()) {
            EpgWidth += m_MarginItem + RecWidth;
            IsRec = true;
        }

        const cString dur = cString::sprintf("%d min", Following->Duration() / 60);
        const int DurWidth {m_FontSml->Width(*dur) + m_FontSml->Width("  ")};
        const int MaxWidth {std::max(StrTimeWidth, DurWidth)};

        ChanInfoBottomPixmap->DrawText(cPoint(m_ChannelWidth - StrTimeWidth - m_MarginItem2,
                                       m_FontHeight + m_FontSmlHeight), *StrTime, Theme.Color(clrChannelFontEpgFollow),
                                       Theme.Color(clrChannelBg), m_FontSml, StrTimeWidth, 0, taRight);
        ChanInfoBottomPixmap->DrawText(
            cPoint(m_ChannelWidth - DurWidth - m_MarginItem2, m_FontHeight + m_FontSmlHeight * 2), *dur,
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
                Config.decorBorderChannelSize+m_ChannelHeight - HeightBottom + m_FontHeight2 + m_FontSmlHeight,
                m_ChannelWidth - left - MaxWidth, m_FontSmlHeight), Theme.Color(clrChannelFontEpgFollow),
                clrTransparent, m_FontSml);
        } else {
            ChanInfoBottomPixmap->DrawText(cPoint(left, m_FontHeight2 + m_FontSmlHeight), *EpgShort,
                                           Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg),
                                           m_FontSml, m_ChannelWidth - left - MaxWidth);
        }

        if (IsRec) {
            ChanInfoBottomPixmap->DrawText(cPoint(left + EpgWidth + m_MarginItem - RecWidth,
                                                  m_FontHeight + m_FontSmlHeight),
                                           "REC", Theme.Color(clrChannelRecordingFollowFg),
                                           Theme.Color(clrChannelRecordingFollowBg), m_FontSml);
        }
    }  // Following

    if (Config.ChannelIconsShow && m_CurChannel)
        ChannelIconsDraw(m_CurChannel, false);

    cString MediaPath {""};
    cSize MediaSize {0, 0};  // Width, Height

    static cPlugin *pScraper = GetScraperPlugin();
    if (Config.TVScraperChanInfoShowPoster && pScraper) {
        ScraperGetPosterBannerV2 call;
        call.event = Present;
        if (pScraper->Service("GetPosterBannerV2", &call)) {
            if ((call.type == tSeries) && call.banner.path.size() > 0) {
                MediaSize.Set(call.banner.width, call.banner.height);
                MediaPath = call.banner.path.c_str();
            } else if (call.type == tMovie && call.poster.path.size() > 0) {
                MediaSize.Set(call.poster.width, call.poster.height);
                MediaPath = call.poster.path.c_str();
            }
        }
    }

    PixmapFill(ChanEpgImagesPixmap, clrTransparent);
    PixmapSetAlpha(ChanEpgImagesPixmap, 255 * Config.TVScraperPosterOpacity * 100);  // Set transparency
    DecorBorderClearByFrom(BorderTVSPoster);
    if (!isempty(*MediaPath)) {
        SetMediaSize(MediaSize, TVSRect.Size());  // Check for too big images
        MediaSize.SetWidth(MediaSize.Width() * Config.TVScraperChanInfoPosterSize * 100);
        MediaSize.SetHeight(MediaSize.Height() * Config.TVScraperChanInfoPosterSize * 100);
        cImage *img {ImgLoader.LoadFile(*MediaPath, MediaSize.Width(), MediaSize.Height())};
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
}

void cFlatDisplayChannel::SetMessage(eMessageType Type, const char *Text) {
    (Text) ? MessageSet(Type, Text) : MessageClear();
}

void cFlatDisplayChannel::SignalQualityDraw() {
    if (!ChanInfoBottomPixmap) return;

    const int SignalStrength {cDevice::ActualDevice()->SignalStrength()};
    const int SignalQuality {cDevice::ActualDevice()->SignalQuality()};
    if (m_LastSignalStrength == SignalStrength && m_LastSignalQuality == SignalQuality)
        return;

    m_LastSignalStrength = SignalStrength;
    m_LastSignalQuality = SignalQuality;

    const cFont *SignalFont = cFont::CreateFont(Setup.FontOsd, Config.decorProgressSignalSize);

    const int left {m_MarginItem2};
    int top {m_FontHeight2 + m_FontSmlHeight * 2 + m_MarginItem};
    top += std::max(m_FontSmlHeight, Config.decorProgressSignalSize) - (Config.decorProgressSignalSize * 2)
                    - m_MarginItem;
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "STR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), SignalFont);
    int ProgressLeft {left + SignalFont->Width("STR ") + m_MarginItem};
    const int SignalWidth {m_ChannelWidth / 2};
    const int ProgressWidth {SignalWidth / 2 - ProgressLeft - m_MarginItem};
    cRect ProgressBar {ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize};
    ProgressBarDrawRaw(ChanInfoBottomPixmap, ChanInfoBottomPixmap, ProgressBar, ProgressBar, SignalStrength, 100,
                       Config.decorProgressSignalFg, Config.decorProgressSignalBarFg, Config.decorProgressSignalBg,
                       Config.decorProgressSignalType, false, Config.SignalQualityUseColors);

    // left = SignalWidth / 2 + m_MarginItem;
    top += Config.decorProgressSignalSize + m_MarginItem;
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "SNR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), SignalFont);
    ProgressLeft = left + SignalFont->Width("STR ") + m_MarginItem;
    // ProgressWidth = SignalWidth - ProgressLeft - m_MarginItem;
    ProgressBar.SetY(top);
    ProgressBarDrawRaw(ChanInfoBottomPixmap, ChanInfoBottomPixmap, ProgressBar, ProgressBar, SignalQuality, 100,
                       Config.decorProgressSignalFg, Config.decorProgressSignalBarFg, Config.decorProgressSignalBg,
                       Config.decorProgressSignalType, false, Config.SignalQualityUseColors);

    m_SignalStrengthRight = ProgressLeft + ProgressWidth;

    delete SignalFont;
}

// You need oscam min rev 10653
// You need dvbapi min commit 85da7b2
void cFlatDisplayChannel::DvbapiInfoDraw() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: DvbapiInfoDraw()");
#endif
    if (!ChanInfoBottomPixmap || !ChanIconsPixmap) return;

    static cPlugin *pDVBApi = cPluginManager::GetPlugin("dvbapi");
    if (!pDVBApi) return;

    sDVBAPIEcmInfo ecmInfo {
        .sid = static_cast<uint16_t>(m_CurChannel->Sid()),
        .ecmtime = 0,
        .hops = -1
    };

    if (!pDVBApi->Service("GetEcmInfo", &ecmInfo)) return;

#ifdef DEBUGFUNCSCALL
    dsyslog("  ChannelSid: %d Channel: %s", m_CurChannel->Sid(), m_CurChannel->Name());
    dsyslog("  CAID: %d", ecmInfo.caid);
    dsyslog("  Card system: %s", *ecmInfo.cardsystem);
    dsyslog("  Reader: %s", *ecmInfo.reader);
    dsyslog("  From: %s", *ecmInfo.from);
    dsyslog("  Protocol: %s", *ecmInfo.protocol);
#endif

    if (ecmInfo.hops < 0 || ecmInfo.ecmtime == 0 || ecmInfo.ecmtime > 9999)
        return;

    int left {m_SignalStrengthRight + m_MarginItem2};
    int top {m_FontHeight2 + m_FontSmlHeight * 2 + m_MarginItem};
    top += std::max(m_FontSmlHeight, Config.decorProgressSignalSize) - (Config.decorProgressSignalSize * 2)
                     - m_MarginItem2;

    const cFont *DvbapiInfoFont = cFont::CreateFont(Setup.FontOsd, (Config.decorProgressSignalSize * 2) + m_MarginItem);

    cString DvbapiInfoText = "DVBAPI: ";
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), DvbapiInfoFont,
                                   DvbapiInfoFont->Width(*DvbapiInfoText) * 2);
    left += DvbapiInfoFont->Width(*DvbapiInfoText) + m_MarginItem;

    cString IconName = cString::sprintf("crypt_%s", *ecmInfo.cardsystem);
    cImage *img {ImgLoader.LoadIcon(*IconName, 999, DvbapiInfoFont->Height())};
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
        dsyslog("flatPlus: Unknown card system: %s (CAID: %d)", *ecmInfo.cardsystem, ecmInfo.caid);
    }

    DvbapiInfoText = cString::sprintf(" %s (%d ms)", *ecmInfo.reader, ecmInfo.ecmtime);
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), DvbapiInfoFont,
                                   DvbapiInfoFont->Width(*DvbapiInfoText) * 2);
    delete DvbapiInfoFont;
}

void cFlatDisplayChannel::Flush() {
    if (m_Present) {
        time_t t {time(0)};
        int Current {0};
        if (t > m_Present->StartTime())
            Current = t - m_Present->StartTime();

        const int Total {m_Present->Duration()};
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
    m_Osd->Flush();
}

void cFlatDisplayChannel::PreLoadImages() {
    int height {m_FontHeight2 + (m_FontSmlHeight * 2) + m_MarginItem - m_MarginItem2};
    int ImageBgHeight {height}, ImageBgWidth {height};
    ImgLoader.LoadIcon("logo_background", height, height);

    cImage *img {ImgLoader.LoadIcon("logo_background", height * 1.34, height)};
    if (img) {
        ImageBgHeight = img->Height();
        ImageBgWidth = img->Width();
    }
    ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
    ImgLoader.LoadIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);

    int index {0};
    LOCK_CHANNELS_READ;
    for (const cChannel *Channel = Channels->First(); Channel && index < LogoPreCache;
         Channel = Channels->Next(Channel)) {
        if (!Channel->GroupSep()) {  // Don't cache named channel group logo
            img = ImgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
            if (img)
                ++index;
        }
    }  // for cChannel

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
