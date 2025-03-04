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
    m_HeightBottom = m_FontHeight2 + (m_FontSmlHeight * 2) + m_MarginItem;  // Top, Bottom, Between
    m_HeightImageLogo = m_HeightBottom;  // High of channel logo image
    if (Config.SignalQualityShow || Config.ChannelDvbapiInfoShow)
        m_HeightBottom += std::max(m_FontSmlHeight, (Config.decorProgressSignalSize * 2) + m_MarginItem) + m_MarginItem;
    else if (Config.ChannelIconsShow)
        m_HeightBottom += m_FontSmlHeight + m_MarginItem;

    int height {m_HeightBottom};
    ChanInfoBottomPixmap =
        CreatePixmap(m_Osd, "ChanInfoBottomPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth, m_HeightBottom));

    ChanIconsPixmap =
        CreatePixmap(m_Osd, "ChanIconsPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth, m_HeightBottom));

    // Area for TVScraper images
    m_TVSRect.Set(20 + Config.decorBorderChannelEPGSize,
                m_TopBarHeight + Config.decorBorderTopBarSize * 2 + 20 + Config.decorBorderChannelEPGSize,
                m_OsdWidth - 40 - Config.decorBorderChannelEPGSize * 2,
                m_OsdHeight - m_TopBarHeight - m_HeightBottom - 40 - Config.decorBorderChannelEPGSize * 2);

    ChanEpgImagesPixmap = CreatePixmap(m_Osd, "ChanEpgImagesPixmap", 2, m_TVSRect);

    // Pixmap for channel logo background
    ChanLogoBGPixmap =
        CreatePixmap(m_Osd, "ChanLogoBGPixmap", 2,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_HeightBottom * 2, m_HeightBottom * 2));

    ChanLogoPixmap =
        CreatePixmap(m_Osd, "ChanLogoPixmap", 3,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_HeightBottom * 2, m_HeightBottom * 2));

    height += Config.decorProgressChannelSize + m_MarginItem2;
    ProgressBarCreate(cRect(Config.decorBorderChannelSize,
                            Config.decorBorderChannelSize + m_ChannelHeight - height + m_MarginItem,
                            m_ChannelWidth, Config.decorProgressChannelSize),
                      m_MarginItem, 0, Config.decorProgressChannelFg,
                      Config.decorProgressChannelBarFg, Config.decorProgressChannelBg,
                      Config.decorProgressChannelType, true);

    ProgressBarDrawBgColor();

    const int HeightTop {m_FontHeight};
    height += HeightTop;
    ChanInfoTopPixmap =
        CreatePixmap(m_Osd, "ChanInfoTopPixmap", 1,
                     cRect(Config.decorBorderChannelSize, Config.decorBorderChannelSize + m_ChannelHeight - height,
                           m_ChannelWidth, HeightTop));

    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));
    PixmapFill(ChanIconsPixmap, clrTransparent);
    PixmapFill(ChanEpgImagesPixmap, clrTransparent);
    PixmapFill(ChanLogoBGPixmap, clrTransparent);
    PixmapFill(ChanLogoPixmap, clrTransparent);
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
                           HeightTop + m_HeightBottom + Config.decorProgressChannelSize + m_MarginItem2,
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
    cString ChannelName {""}, ChannelNumber {""}, TransponderInfo {""};
    if (Channel) {
        m_IsRadioChannel = ((!Channel->Vpid()) && (Channel->Apid(0))) ? true : false;
        IsGroup = Channel->GroupSep();

        ChannelName = Channel->Name();
        if (!IsGroup) {
            ChannelNumber = cString::sprintf("%d%s", Channel->Number(), (Number) ? "-" : "");
#ifdef SHOW_TRANSPONDERINFO
            // Config.ChannelShowTransponderInfo (Hidden option?)
            if (!Number) {
                const int tp {Channel->Transponder()};
                const int f {(tp > 200000) ? (tp - 200000) : (tp > 100000) ? (tp - 100000) : tp};
                // dsyslog("Channel: %s, Frequency: %d, Transponder: %d", *ChannelName, f, tp);
                const cString pol = (tp > 200000) ? "V" : (tp > 100000) ? "H" : "?";
                const cString band = (f > Setup.LnbSLOF) ? "H" : "L";
                // Frequency, Polarity (V/H), Band (H/L)
                TransponderInfo = cString::sprintf("  (%d %s%s)", f, *pol, *band);
            }
#endif
        } else if (Number) {
            ChannelNumber = cString::sprintf("%d-", Number);
        }

        m_CurChannel = Channel;
    } else {
        ChannelName = ChannelString(NULL, 0);
    }  // if (Channel)

    const cString ChannelString = cString::sprintf("%s  %s", *ChannelNumber, *ChannelName);
    const int left {m_MarginItem * 10};  // 50 Pixel
    PixmapFill(ChanInfoTopPixmap, Theme.Color(clrChannelBg));
    ChanInfoTopPixmap->DrawText(cPoint(left, 0), *ChannelString, Theme.Color(clrChannelFontTitle),
                                Theme.Color(clrChannelBg), m_Font);

    if (!isempty(*TransponderInfo)) {
        ChanInfoTopPixmap->DrawText(cPoint(m_ChannelWidth - left - m_Font->Width(*ChannelString), 0), *TransponderInfo,
                                    Theme.Color(clrChannelFontTitle), Theme.Color(clrChannelBg), m_Font);
    }

    PixmapFill(ChanLogoPixmap, clrTransparent);
    PixmapFill(ChanLogoBGPixmap, clrTransparent);

    if (!IsGroup) {
        const int ImageHeight {m_HeightImageLogo - m_MarginItem2};
        int ImageBgHeight {ImageHeight};
        int ImageBgWidth {ImageHeight};
        int ImageLeft {m_MarginItem2};
        int ImageTop {m_MarginItem};
        cImage *img {ImgLoader.LoadIcon("logo_background", ImageHeight * 1.34f, ImageHeight)};
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
                ImageTop = m_MarginItem + (ImageBgHeight - img->Height()) / 2;
                ImageLeft = m_MarginItem2 + (ImageBgWidth - img->Width()) / 2;
                ChanLogoPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
            }
        }
    }
}

void cFlatDisplayChannel::ChannelIconsDraw(const cChannel *Channel, bool Resolution) {
    if (!ChanIconsPixmap) return;

    if (!Resolution)
        PixmapFill(ChanIconsPixmap, clrTransparent);

    const int top {m_HeightBottom - m_FontSmlHeight - m_MarginItem};
    int left {m_ChannelWidth - m_MarginItem2};
    const int ImageHeight = std::max(m_FontSmlHeight, Config.decorProgressSignalSize * 2 + m_MarginItem);

    cImage *img {nullptr};
    if (Channel) {
        img = ImgLoader.LoadIcon((Channel->Ca()) ? "crypted" : "uncrypted", ICON_WIDTH_UNLIMITED, ImageHeight);
        if (img) {
            left -= img->Width();
            ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
            left -= m_MarginItem2;
        }
    }

    if (Resolution && !m_IsRadioChannel && m_ScreenWidth > 0) {
        cString IconName {""};
        if (Config.ChannelResolutionAspectShow) {  // Show Aspect (16:9)
            IconName = *GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
            img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, ImageHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                left -= m_MarginItem2;
            }

            IconName = *GetScreenResolutionIcon(m_ScreenWidth, m_ScreenHeight);  // Show Resolution (1920x1080)
            img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, ImageHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                left -= m_MarginItem2;
            }
        }

        if (Config.ChannelFormatShow && !Config.ChannelSimpleAspectFormat) {
            IconName = *GetFormatIcon(m_ScreenWidth);  // Show Format (HD)
            img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, ImageHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                left -= m_MarginItem2;
            }
        }
        // Show audio icon (Dolby, Stereo)
        if (Config.ChannelResolutionAspectShow) {  //? Add separate config option
            IconName = *GetCurrentAudioIcon();
            img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, ImageHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                // left -= m_MarginItem2;
            }
        }
    }
}

void cFlatDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayChannel::SetEvents()");
#endif
    if (!ChanInfoBottomPixmap || !ChanEpgImagesPixmap) return;

    m_Present = Present;

    Scrollers.Clear();

    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));
    PixmapFill(ChanIconsPixmap, clrTransparent);

    cString Epg {""}, EpgShort {""};
    int EpgWidth {0}, EpgShortWidth {0};
    tColor EpgColor {clrTransparent};

    cString StartTime {""}, StrTime {""};
    int StrTimeWidth {0};

    cString SeenDur {""};
    int SeenDurWidth {0}, SeenDurMaxWidth {0};
    int MaxAvailWidth {0};

    int EventDuration {0};

    bool IsRec {false};
    const int RecWidth {m_FontSml->Width("REC")};

    int left = m_HeightImageLogo * 1.34f + m_MarginItem3;  // Narrowing conversion
    const int StartTimeLeft {left};
    int TopSeen {0}, TopEpg {0};

    if (Config.ChannelShowStartTime)
        left += m_Font->Width("00:00  ");

    for (uint i {0}; i < 2; i++) {
        const bool IsPresent = i ? false : true;
        const cEvent *Event = IsPresent ? Present : Following;
        if (Event) {
            StartTime = *Event->GetTimeString();  // Start time (left side)
            StrTime = cString::sprintf("%s - %s", *StartTime, *Event->GetEndTimeString());  // Start - End (right side)
            StrTimeWidth = m_FontSml->Width(*StrTime) + m_FontSml->Width("  ");
            EventDuration = Event->Duration() / 60;  // Duration in minutes

            Epg = Event->Title();
            EpgShort = Event->ShortText();
            EpgWidth = m_Font->Width(*Epg) + m_MarginItem2;
            EpgShortWidth = m_FontSml->Width(*EpgShort) + m_MarginItem2;

            if (Event->HasTimer()) {
                IsRec = true;
                EpgWidth += m_MarginItem + RecWidth;
            }

            if (IsPresent) {  // Present
                TopEpg = 0;
                TopSeen = m_FontSmlHeight;
                EpgColor = clrChannelFontEpg;
                const int s = (time(0) - Event->StartTime()) / 60;  // Narrowing conversion
                const int sleft {EventDuration - s};

                switch (Config.ChannelTimeLeft) {
                case 0:
                    SeenDur = cString::sprintf("%d-/%d+ %d min", s, sleft, EventDuration);
                    break;
                case 1:
                    SeenDur = cString::sprintf("%d- %d min", s, EventDuration);
                    break;
                case 2:
                    SeenDur = cString::sprintf("%d+ %d min", sleft, EventDuration);
                    break;
                }
            } else {  // Following
                TopEpg = m_FontHeight + m_FontSmlHeight;
                TopSeen = m_FontHeight + m_FontSmlHeight * 2;
                EpgColor = clrChannelFontEpgFollow;
                SeenDur = cString::sprintf("%d min", EventDuration);
            }  // if (IsPresent)

            SeenDurWidth = m_FontSml->Width(*SeenDur) + m_FontSml->Width("  ");
            SeenDurMaxWidth = std::max(StrTimeWidth, SeenDurWidth);
            MaxAvailWidth = m_ChannelWidth - left - SeenDurMaxWidth;
#ifdef DEBUGFUNCSCALL
            dsyslog("   EpgWidth: %d, EpgShortWidth: %d, MaxAvailWidth: %d", EpgWidth, EpgShortWidth, MaxAvailWidth);
            dsyslog("   left: %d, m_ChannelWidth: %d, SeenDurMaxWidth: %d", left, m_ChannelWidth, SeenDurMaxWidth);
            dsyslog("   IsRec: %d, RecWidth: %d", IsRec, RecWidth);
#endif
            // Draw EPG info
            if (Config.ChannelShowStartTime) {
                ChanInfoBottomPixmap->DrawText(cPoint(StartTimeLeft, TopEpg), *StartTime, Theme.Color(EpgColor),
                                               Theme.Color(clrChannelBg), m_Font);
            }

            if ((EpgWidth > MaxAvailWidth) && Config.ScrollerEnable) {
                Scrollers.AddScroller(*Epg,
                                      cRect(Config.decorBorderChannelSize + left,
                                            Config.decorBorderChannelSize + m_ChannelHeight - m_HeightBottom + TopEpg,
                                            MaxAvailWidth - ((IsRec) ? RecWidth + m_MarginItem2 : 0),
                                            m_FontHeight),
                                      Theme.Color(EpgColor), clrTransparent, m_Font);
            } else {
                ChanInfoBottomPixmap->DrawText(cPoint(left, TopEpg), *Epg, Theme.Color(EpgColor),
                                               Theme.Color(clrChannelBg), m_Font, MaxAvailWidth);
            }

            if (IsRec) {
                ChanInfoBottomPixmap->DrawText(
                    cPoint(left +
                               ((EpgWidth > MaxAvailWidth) ? MaxAvailWidth - m_MarginItem : EpgWidth + m_MarginItem) -
                               RecWidth,
                           TopEpg),
                    "REC", Theme.Color((IsPresent) ? clrChannelRecordingPresentFg : clrChannelRecordingFollowFg),
                    Theme.Color((IsPresent) ? clrChannelRecordingPresentBg : clrChannelRecordingFollowBg), m_FontSml);
                IsRec = false;  // Reset for next event
            }

            if ((EpgShortWidth > MaxAvailWidth) && Config.ScrollerEnable) {
                Scrollers.AddScroller(
                    *EpgShort,
                    cRect(Config.decorBorderChannelSize + left,
                          Config.decorBorderChannelSize + m_ChannelHeight - m_HeightBottom + TopEpg + m_FontHeight,
                          MaxAvailWidth, m_FontSmlHeight),
                    Theme.Color(EpgColor), clrTransparent, m_FontSml);
            } else {
                ChanInfoBottomPixmap->DrawText(cPoint(left, TopEpg + m_FontHeight), *EpgShort, Theme.Color(EpgColor),
                                               Theme.Color(clrChannelBg), m_FontSml, MaxAvailWidth);
            }

            ChanInfoBottomPixmap->DrawText(cPoint(m_ChannelWidth - StrTimeWidth - m_MarginItem2, TopEpg), *StrTime,
                                           Theme.Color(EpgColor), Theme.Color(clrChannelBg), m_FontSml, StrTimeWidth, 0,
                                           taRight);
            ChanInfoBottomPixmap->DrawText(cPoint(m_ChannelWidth - SeenDurWidth - m_MarginItem2, TopSeen), *SeenDur,
                                           Theme.Color(EpgColor), Theme.Color(clrChannelBg), m_FontSml, SeenDurWidth, 0,
                                           taRight);
        }  // if (Event)
    }  // for

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
        SetMediaSize(MediaSize, m_TVSRect.Size());  // Check for too big images
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
    int top {m_HeightBottom -
             (Config.decorProgressSignalSize * 2 + m_MarginItem2)};  // One margin for progress bar to bottom

    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "STR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), SignalFont);
    const int ProgressLeft {left + std::max(SignalFont->Width("STR "), SignalFont->Width("SNR ")) + m_MarginItem};
    const int ProgressWidth {m_ChannelWidth / 4 - ProgressLeft - m_MarginItem};
    cRect ProgressBar {ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize};
    ProgressBarDrawRaw(ChanInfoBottomPixmap, ChanInfoBottomPixmap, ProgressBar, ProgressBar, SignalStrength, 100,
                       Config.decorProgressSignalFg, Config.decorProgressSignalBarFg, Config.decorProgressSignalBg,
                       Config.decorProgressSignalType, false, Config.SignalQualityUseColors);

    top += Config.decorProgressSignalSize + m_MarginItem;
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "SNR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), SignalFont);
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
    dsyslog("   ChannelSid: %d, Channel: %s", m_CurChannel->Sid(), m_CurChannel->Name());
    dsyslog("   CAID: %d, Card system: %s", ecmInfo.caid, *ecmInfo.cardsystem);
    dsyslog("   Reader: %s", *ecmInfo.reader);
    dsyslog("   From: %s, Hops: %d", *ecmInfo.from, ecmInfo.hops);
    dsyslog("   Protocol: %s", *ecmInfo.protocol);
#endif

    if (ecmInfo.hops < 0 || ecmInfo.ecmtime == 0 || ecmInfo.ecmtime > 9999)
        return;

    int left {m_SignalStrengthRight + m_MarginItem2};
    const int ProgressBarHeight {Config.decorProgressSignalSize * 2 + m_MarginItem};

    const cFont *DvbapiInfoFont = cFont::CreateFont(Setup.FontOsd, ProgressBarHeight);

    const int FontAscender {GetFontAscender(Setup.FontOsd, ProgressBarHeight)};
    constexpr ulong CharCode {0x0044};  // U+0044 LATIN CAPITAL LETTER D
    const int GlyphSize = GetGlyphSize(Setup.FontOsd, CharCode, ProgressBarHeight);  // Narrowing conversion
    const int TopOffset {(FontAscender - GlyphSize) / 2};  // Center vertically
    const int top {m_HeightBottom - ProgressBarHeight - m_MarginItem -
        TopOffset};  // One margin for progress bar to bottom

    cString DvbapiInfoText = "DVBAPI: ";
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), DvbapiInfoFont);

    left += DvbapiInfoFont->Width(*DvbapiInfoText) + m_MarginItem;

    cString IconName = cString::sprintf("crypt_%s", *ecmInfo.cardsystem);
    cImage *img {ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, DvbapiInfoFont->Height())};
    if (img) {
        ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
        left += img->Width() + m_MarginItem;
    } else {
        IconName = "crypt_unknown";
        img = ImgLoader.LoadIcon(*IconName, ICON_WIDTH_UNLIMITED, DvbapiInfoFont->Height());
        if (img) {
            ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
            left += img->Width() + m_MarginItem;
        }
        dsyslog("flatPlus: Unknown card system: %s (CAID: %d)", *ecmInfo.cardsystem, ecmInfo.caid);
    }

    DvbapiInfoText = cString::sprintf(" %s (%d ms)", *ecmInfo.reader, ecmInfo.ecmtime);
    if (ecmInfo.hops > 1)
        DvbapiInfoText.Append(cString::sprintf(" (%d hops)", ecmInfo.hops));

    // Store the width of the drawn dvbapi info text for the next draw call,
    // so that we can ensure that the text is drawn at the correct position
    // even if the text changes (e.g. when the channel is changed).
    // This is done by storing the maximum width of the text seen so far.
    m_LastDvbapiInfoTextWidth = std::max(DvbapiInfoFont->Width(*DvbapiInfoText), m_LastDvbapiInfoTextWidth);

    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), DvbapiInfoFont, m_LastDvbapiInfoTextWidth);

    delete DvbapiInfoFont;
}

void cFlatDisplayChannel::Flush() {
    if (m_Present) {
        const time_t t {time(0)};
        const int Current = (t > m_Present->StartTime()) ? t - m_Present->StartTime() : 0;  // Narrowing conversation
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

    cImage *img {ImgLoader.LoadIcon("logo_background", height * 1.34f, height)};
    if (img) {
        ImageBgHeight = img->Height();
        ImageBgWidth = img->Width();
    }
    ImgLoader.LoadIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
    ImgLoader.LoadIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);

    int index {0};
    LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
    for (const cChannel *Channel = Channels->First(); Channel && index < LogoPreCache;
         Channel = Channels->Next(Channel)) {
        if (!Channel->GroupSep()) {  // Don't cache named channel group logo
            img = ImgLoader.LoadLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
            if (img)
                ++index;
        }
    }  // for cChannel

    height = std::max(m_FontSmlHeight, Config.decorProgressSignalSize * 2 + m_MarginItem);
    ImgLoader.LoadIcon("crypted", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("uncrypted", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("unknown_asp", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("43", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("169", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("169w", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("221", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("7680x4320", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("3840x2160", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("1920x1080", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("1440x1080", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("1280x720", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("960x720", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("704x576", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("720x576", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("544x576", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("528x576", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("480x576", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("352x576", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("unknown_res", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("uhd", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("hd", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("sd", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("audio_dolby", ICON_WIDTH_UNLIMITED, height);
    ImgLoader.LoadIcon("audio_stereo", ICON_WIDTH_UNLIMITED, height);

    // Audio tracks (displaytracks.c)
    ImgLoader.LoadIcon("tracks_ac3", ICON_WIDTH_UNLIMITED, m_FontHeight);
    ImgLoader.LoadIcon("tracks_stereo", ICON_WIDTH_UNLIMITED, m_FontHeight);
}
