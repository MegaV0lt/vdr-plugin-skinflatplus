/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displaychannel.h"

#include <vdr/device.h>

#include <algorithm>
#include <cstddef>
#include <string>

#include "./flat.h"
#include "./fontcache.h"
#include "./services/dvbapi.h"

cFlatDisplayChannel::cFlatDisplayChannel(bool WithInfo) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayChannel::cFlatDisplayChannel()");
    cTimeMs Timer;  // Start Timer
#endif

    CreateFullOsd();
    TopBarCreate();
    MessageCreate();

    m_ChannelWidth = m_OsdWidth - Config.decorBorderChannelSize * 2;
    m_ChannelHeight = m_OsdHeight - Config.decorBorderChannelSize * 2;

    // From bottom to top (2 * EPG + 2 * EPGsml)
    m_HeightBottom = m_FontHeight2 + (m_FontSmlHeight * 2) + m_MarginItem;  // Top, Bottom, Between
    m_HeightImageLogo = m_HeightBottom;                                     // High of channel logo image
    if (Config.SignalQualityShow || Config.ChannelDvbapiInfoShow)
        m_HeightBottom += std::max(m_FontSmlHeight, (Config.decorProgressSignalSize * 2) + m_MarginItem) + m_MarginItem;
    else if (Config.ChannelIconsShow)
        m_HeightBottom += m_FontSmlHeight + m_MarginItem;

    int height {m_HeightBottom};
    const cRect ChanInfoBottomViewPort {Config.decorBorderChannelSize,
                                        Config.decorBorderChannelSize + m_ChannelHeight - height, m_ChannelWidth,
                                        m_HeightBottom};
    ChanInfoBottomPixmap = CreatePixmap(m_Osd, "ChanInfoBottomPixmap", 1, ChanInfoBottomViewPort);
    ChanIconsPixmap = CreatePixmap(m_Osd, "ChanIconsPixmap", 2, ChanInfoBottomViewPort);

    // Pixmap for channel logo and background (4:3 aspect ratio, scaled to height of m_HeightImageLogo)
    const cRect ChanLogoViewPort {Config.decorBorderChannelSize,
                                  Config.decorBorderChannelSize + m_ChannelHeight - height,
                                  static_cast<int>(m_HeightImageLogo * 1.34f), m_HeightImageLogo};
    ChanLogoBgPixmap = CreatePixmap(m_Osd, "ChanLogoBGPixmap", 2, ChanLogoViewPort);
    ChanLogoPixmap = CreatePixmap(m_Osd, "ChanLogoPixmap", 3, ChanLogoViewPort);

    height += Config.decorProgressChannelSize + m_MarginItem2;
    ProgressBarCreate(cRect(Config.decorBorderChannelSize,
                            Config.decorBorderChannelSize + m_ChannelHeight - height + m_MarginItem, m_ChannelWidth,
                            Config.decorProgressChannelSize),
                      m_MarginItem, 0, Config.decorProgressChannelFg, Config.decorProgressChannelBarFg,
                      Config.decorProgressChannelBg, Config.decorProgressChannelType, true);

    ProgressBarDrawBgColor();

    // Pixmap for channel number and name
    // Top of ChanInfoTopViewPort depending on setting ChannelShowNameWithShadow
    const int HeightTop {m_FontBigHeight +
                         (Config.ChannelShowNameWithShadow ? m_FontBigHeight / 10 + Config.decorBorderChannelSize : 0)};
    height += HeightTop;
    const cRect ChanInfoTopViewPort {Config.decorBorderChannelSize,
                                     Config.decorBorderChannelSize + m_ChannelHeight - height, m_ChannelWidth,
                                     HeightTop};
    ChanInfoTopPixmap = CreatePixmap(m_Osd, "ChanInfoTopPixmap", 1, ChanInfoTopViewPort);

    // Area for TVScraper images
    m_TVSRect.Set(m_MarginEPGImage + Config.decorBorderChannelEPGSize,
                  m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_MarginEPGImage +
                      Config.decorBorderChannelEPGSize,
                  m_OsdWidth - m_MarginEPGImage * 2 - Config.decorBorderChannelEPGSize * 2,
                  m_ChannelHeight - m_TopBarHeight - Config.decorBorderTopBarSize * 2 - HeightTop - m_HeightBottom -
                      m_MarginEPGImage * 2 - Config.decorBorderChannelEPGSize * 2);

    ChanEpgImagesPixmap = CreatePixmap(m_Osd, "ChanEpgImagesPixmap", 2, m_TVSRect);

    // Clear Pixmaps
    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));
    PixmapClear(ChanIconsPixmap);
    PixmapClear(ChanEpgImagesPixmap);
    PixmapClear(ChanLogoBgPixmap);
    PixmapClear(ChanLogoPixmap);
    PixmapClear(ChanInfoTopPixmap);

    Scrollers.SetOsd(m_Osd);
    Scrollers.SetScrollStep(Config.ScrollerStep);
    Scrollers.SetScrollDelay(Config.ScrollerDelay);
    Scrollers.SetScrollType(Config.ScrollerType);

    if (Config.ChannelWeatherShow) DrawWidgetWeather();

#ifdef USE_ZAPCOCKPIT
    // Geometry for the zapcockpit panels: The area between top bar and the channel info display at the bottom is used
    // for the lists and the detailed EPG info. The lists are anchored at the side of the pressed key: The list opened
    // with key 'right' at the left edge, the list opened with key 'left' at the right edge. With the default setup
    // ('Key right opens channellist') the channellist is therefore shown at the left and the grouplist at the right
    // edge (mirrored when the option is disabled). The channellist of the selected group is shown beside the grouplist
    // and the EPG info of the selected channel in the remaining space. For dcChannelInfo (2nd 'Ok') the EPG info uses
    // the full width. Pixmaps are created lazily when one of the extended views is opened. While a list view is shown
    // all base OSD elements are hidden, so the lists and the EPG info beside them can use the full OSD height. Only the
    // channel hints (shown while entering a channel number) and the full width EPG info (2nd 'Ok') use the area between
    // top bar and the channel info display, because there the base OSD elements stay visible.
    const int ZapTop {m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_MarginItem};
    const int ZapBottom {Config.decorBorderChannelSize + m_ChannelHeight - height - m_MarginItem2};
    const int ZapHeight {std::max(0, ZapBottom - ZapTop)};
    const int ZapFullHeight {m_OsdHeight};
    const int ZapLeft {Config.decorBorderChannelSize};
    const int ZapRight {m_OsdWidth - Config.decorBorderChannelSize};
    const int ZapListWidthPct {std::clamp(Config.ChannelZapcockpitListWidth, 20, 40)};
    m_ZapColWidth = (ZapRight - ZapLeft) * ZapListWidthPct / 100;

    // Columns anchored at the left edge (L) and at the right edge (R)
    const int Lx1 {ZapLeft};                              // 1st column from left
    const int Lx2 {Lx1 + m_ZapColWidth + m_MarginItem2};  // 2nd column from left
    const int Lx3 {Lx2 + m_ZapColWidth + m_MarginItem2};  // 3rd column from left
    const int Rx1 {ZapRight - m_ZapColWidth};             // 1st column from right
    const int Rx2 {Rx1 - m_MarginItem2 - m_ZapColWidth};  // 2nd column from right
    const int Rx3 {Rx2 - m_MarginItem2 - m_ZapColWidth};  // 3rd column from right

    // The list items use the same fonts as the channel info display at the bottom, scaled by the setup option
    // 'Zapcockpit: List font size', so that more items fit on the screen
    const int ZapFontPct {std::clamp(Config.ChannelZapcockpitFontSize, 60, 100)};
    const int ZapFontSize {std::max(1, Setup.FontOsdSize * ZapFontPct / 100)};
    const int ZapFontSmlSize {std::max(1, Setup.FontSmlSize * ZapFontPct / 100)};
    m_ZapFont = FontCache.GetFont(Setup.FontOsd, ZapFontSize);
    m_ZapFontSml = FontCache.GetFont(Setup.FontSml, ZapFontSmlSize);
    m_ZapFontHeight = FontCache.GetFontHeight(Setup.FontOsd, ZapFontSize);
    m_ZapFontSmlHeight = FontCache.GetFontHeight(Setup.FontSml, ZapFontSmlSize);

    // Channellist items: Channel logo left, three text lines right of it
    // (remaining time, title of the running event, following event)
    m_ZapItemHeightChan = m_ZapFontSmlHeight * 2 + m_ZapFontHeight + m_MarginItem + Config.MenuItemPadding;
    // Grouplist items: One text line
    m_ZapItemHeightGroup = m_ZapFontHeight + Config.MenuItemPadding;
    m_ZapMaxItemsChan = (m_ZapItemHeightChan > 0) ? ZapFullHeight / m_ZapItemHeightChan : 0;
    m_ZapMaxItemsGroup = (m_ZapItemHeightGroup > 0) ? ZapFullHeight / m_ZapItemHeightGroup : 0;

    if (Config.ChannelZapcockpitKeyRightOpensList) {  // Channellist left, grouplist right
        m_ZapListRectChan.Set(Lx1, 0, m_ZapColWidth, ZapFullHeight);
        m_ZapInfoRectChan.Set(Lx2, 0, ZapRight - Lx2, ZapFullHeight);
        m_ZapListRectGroup.Set(Rx1, 0, m_ZapColWidth, ZapFullHeight);
        m_ZapList2RectGroup.Set(Rx2, 0, m_ZapColWidth, ZapFullHeight);
        m_ZapInfoRectGroup.Set(ZapLeft, 0, std::max(0, Rx3 + m_ZapColWidth - m_MarginItem2 - ZapLeft),
                               ZapFullHeight);
        m_ZapHintsRect.Set(Lx1, ZapTop, m_ZapColWidth, ZapHeight);
    } else {  // Channellist right, grouplist left
        m_ZapListRectChan.Set(Rx1, 0, m_ZapColWidth, ZapFullHeight);
        m_ZapInfoRectChan.Set(ZapLeft, 0, Rx1 - m_MarginItem2 - ZapLeft, ZapFullHeight);
        m_ZapListRectGroup.Set(Lx1, 0, m_ZapColWidth, ZapFullHeight);
        m_ZapList2RectGroup.Set(Lx2, 0, m_ZapColWidth, ZapFullHeight);
        m_ZapInfoRectGroup.Set(Lx3, 0, std::max(0, ZapRight - Lx3), ZapFullHeight);
        m_ZapHintsRect.Set(Rx1, ZapTop, m_ZapColWidth, ZapHeight);
    }
    // The area between top bar and the channel info display at the bottom without the ChanInfoTopPixmap area
    // m_ZapInfoWideRect.Set(ZapLeft, ZapTop, ZapRight - ZapLeft, ZapHeight);
    m_ZapInfoWideRect.Set(ZapLeft + m_MarginEPGImage, ZapTop + m_MarginEPGImage,
                          ZapRight - ZapLeft - m_MarginEPGImage * 2, ZapHeight + HeightTop - m_MarginEPGImage * 2);

    // DecorBorder for the channel info display at the bottom without the ChanInfoTopPixmap area (Channel name)
    m_ZapChanInfoBottomDecorBorder = {
        Config.decorBorderChannelSize,
        ChanInfoTopViewPort.Y() + HeightTop,
        m_ChannelWidth,
        m_HeightBottom + Config.decorProgressChannelSize + m_MarginItem2,
        Config.decorBorderChannelSize,
        Config.decorBorderChannelType,
        Config.decorBorderChannelFg,
        Config.decorBorderChannelBg};
#endif

    // Decor border depending on setting 'ChannelShowNameWithShadow'
    const sDecorBorder ib {Config.decorBorderChannelSize,
                           ChanInfoTopViewPort.Y() + (Config.ChannelShowNameWithShadow ? HeightTop : 0),
                           m_ChannelWidth,
                           m_HeightBottom + Config.decorProgressChannelSize + m_MarginItem2 +
                               (Config.ChannelShowNameWithShadow ? 0 : HeightTop),
                           Config.decorBorderChannelSize,
                           Config.decorBorderChannelType,
                           Config.decorBorderChannelFg,
                           Config.decorBorderChannelBg,
                           BorderChannelInfoBottom};
    DecorBorderDraw(ib);
#ifdef DEBUGFUNCSCALL
    dsyslog("   cFlatDisplayChannel() done, elapsed time %ld ms", Timer.Elapsed());
#endif
}

cFlatDisplayChannel::~cFlatDisplayChannel() {
    Scrollers.Clear();
    if (m_Osd) {
        m_Osd->DestroyPixmap(ChanInfoTopPixmap);
        m_Osd->DestroyPixmap(ChanInfoBottomPixmap);
        m_Osd->DestroyPixmap(ChanLogoBgPixmap);
        m_Osd->DestroyPixmap(ChanLogoPixmap);
        m_Osd->DestroyPixmap(ChanIconsPixmap);
        m_Osd->DestroyPixmap(ChanEpgImagesPixmap);
#ifdef USE_ZAPCOCKPIT
        if (ZapListPixmap) m_Osd->DestroyPixmap(ZapListPixmap);
        if (ZapList2Pixmap) m_Osd->DestroyPixmap(ZapList2Pixmap);
        if (ZapInfoPixmap) m_Osd->DestroyPixmap(ZapInfoPixmap);
#endif
    }
}

void cFlatDisplayChannel::SetChannel(const cChannel *Channel, int Number) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayChannel::SetChannel(%s, %d)", Channel->Name(), Number);
#endif

    if (!ChanIconsPixmap || !ChanInfoTopPixmap || !ChanLogoBgPixmap || !ChanLogoPixmap) return;

#ifdef USE_ZAPCOCKPIT
    // Number entry finished: Hide the channel hint list again
    if (Number == 0 && m_ZapNumHints > 0) ZapHideLists();
#endif

    PixmapClear(ChanIconsPixmap);
    m_LastScreenWidth = -1;

    bool IsGroup {false};
    cString ChannelName {""}, ChannelNumber {""};
#ifdef SHOW_TRANSPONDERINFO
    cString TransponderInfo {""};
#endif
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

    const cString ChannelString {cString::sprintf("%s  %s", *ChannelNumber, *ChannelName)};

    if (Config.ChannelShowNameWithShadow) {
        PixmapClear(ChanInfoTopPixmap);
        const int ShadowSize {m_FontBigHeight / 10};  // Shadow size is 10 % of font height
        // Ensure shadow size is reasonable
        const int MinShadowSize {m_MarginItem / 2 + 1};  // Minimum shadow size
        const int MaxShadowSize {m_MarginItem3};         // Shadow should not be too large
        const int BoundedShadowSize {std::clamp(ShadowSize, MinShadowSize, MaxShadowSize)};
        // Set shadow color to the same as background color and remove transparency
        const tColor ShadowColor = 0xFF000000 | Theme.Color(clrChannelBg);
#ifdef DBUGFUNCSCALL
        dsyslog("   m_FontBigHeight %d, ShadowSize %d", m_FontBigHeight, ShadowSize);
#endif
        // Draw text with shadow
        DrawTextWithShadow(ChanInfoTopPixmap, cPoint(m_MarginItem10, 0), *ChannelString,
                           Theme.Color(clrChannelFontTitle), ShadowColor, m_FontBig, BoundedShadowSize);
    } else {
        // Channel name on background
        PixmapFill(ChanInfoTopPixmap, Theme.Color(clrChannelBg));
        ChanInfoTopPixmap->DrawText(cPoint(m_MarginItem10, 0), *ChannelString, Theme.Color(clrChannelFontTitle),
                                    Theme.Color(clrChannelBg), m_FontBig);
    }
#ifdef SHOW_TRANSPONDERINFO
    if (TransponderInfo[0] != '\0') {
        // Transponder info on background
        const int top {m_FontBig->Height() - m_FontHeight};
        ChanInfoTopPixmap->DrawText(cPoint(m_ChannelWidth - left - m_Font->Width(*ChannelString), top),
                                    *TransponderInfo, Theme.Color(clrChannelFontTitle), Theme.Color(clrChannelBg),
                                    m_Font);
    }
#endif

    PixmapClear(ChanLogoPixmap);
    PixmapClear(ChanLogoBgPixmap);
    // Draw channel logo and background
    if (!IsGroup) {
        const int ImageHeight {m_HeightImageLogo - m_MarginItem2};
        int ImageBgHeight {ImageHeight};
        int ImageBgWidth {ImageHeight};
        int ImageLeft {m_MarginItem2};
        int ImageTop {m_MarginItem};
        cImage *img {ImgLoader.GetLogoBg(ImageHeight * 1.34f, ImageHeight)};  // Load 'logo_background'
        if (img) {
            ImageBgHeight = img->Height();
            ImageBgWidth = img->Width();
            ChanLogoBgPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }

        img = ImgLoader.GetLogo(*ChannelName, ImageBgWidth - 4, ImageBgHeight - 4);
        if (!img) img = ImgLoader.GetIcon((m_IsRadioChannel) ? "radio" : "tv", ImageBgWidth - 10, ImageBgHeight - 10);

        if (img) {  // Draw the logo
            ImageTop = m_MarginItem + (ImageBgHeight - img->Height()) / 2;
            ImageLeft = m_MarginItem2 + (ImageBgWidth - img->Width()) / 2;
            ChanLogoPixmap->DrawImage(cPoint(ImageLeft, ImageTop), *img);
        }
    }
}

void cFlatDisplayChannel::ChannelIconsDraw(const cChannel *Channel, bool Resolution) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayChannel::ChannelIconsDraw()");
    cTimeMs Timer;  // Start Timer
#endif
    // if (!ChanIconsPixmap) return;  // Remove redundant check since caller already checks

    if (Resolution)  // Clear only when resolution has changed
        PixmapClear(ChanIconsPixmap);

    const int top {m_HeightBottom - m_FontSmlHeight - m_MarginItem};
    int left {m_ChannelWidth - m_MarginItem2};

    cImage *img {nullptr};
    if (Channel) {
        img = ImgLoader.GetIcon((Channel->Ca()) ? "crypted" : "uncrypted", kIconMaxSize, m_FontSmlHeight);
        if (img) {
            left -= img->Width();
            ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
            left -= m_MarginItem2;
        }
    }

    if (Resolution && !m_IsRadioChannel && m_ScreenWidth > 0) {
        cString IconName {""};
        if (Config.ChannelResolutionAspectShow) {  // Show Aspect (16:9)
            // If Config.ChannelSimpleAspectFormat is enabled, the aspect ratio is only shown for
            // sd program, else format (HD/UHD) is shown
            IconName = *GetAspectIcon(m_ScreenWidth, m_ScreenAspect);
            img = ImgLoader.GetIcon(*IconName, kIconMaxSize, m_FontSmlHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                left -= m_MarginItem2;
            }

            IconName = *GetScreenResolutionIcon(m_ScreenWidth, m_ScreenHeight);  // Show Resolution (1920x1080)
            img = ImgLoader.GetIcon(*IconName, kIconMaxSize, m_FontSmlHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                left -= m_MarginItem2;
            }
        }

        if (Config.ChannelFormatShow && !Config.ChannelSimpleAspectFormat) {
            IconName = *GetFormatIcon(m_ScreenWidth);  // Show Format (HD)
            img = ImgLoader.GetIcon(*IconName, kIconMaxSize, m_FontSmlHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                left -= m_MarginItem2;
            }
        }

        if (Config.ChannelAudioFormatShow) {  // Show audio icon (Dolby, Stereo)
            IconName = *GetCurrentAudioIcon();
            img = ImgLoader.GetIcon(*IconName, kIconMaxSize, m_FontSmlHeight);
            if (img) {
                left -= img->Width();
                ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
                // left -= m_MarginItem2;
            }
        }
    }
#ifdef DEBUGFUNCSCALL
    if (Timer.Elapsed() > 0) dsyslog("   Done in %ld ms", Timer.Elapsed());
#endif
}

void cFlatDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayChannel::SetEvents()");
#endif
    if (!ChanInfoBottomPixmap || !ChanEpgImagesPixmap) return;

    m_Present = Present;
    m_Following = Following;  // Store the events for later use in Zapcockpit extended channel display
    Scrollers.Clear();

    // Epg related variables
    cString Epg {""}, EpgShort {""};
    int EpgWidth {0}, EpgShortWidth {0};
    tColor EpgColor {clrTransparent};

    // Time related variables
    cString StartTime {""}, StrTime {""};
    int StrTimeWidth {0};
    int EventDuration {0};

    cString SeenDur {""};
    int SeenDurWidth {0}, SeenDurMaxWidth {0};
    int MaxAvailWidth {0};

    int left {static_cast<int>(m_HeightImageLogo * 1.34f) + m_MarginItem3};
    const int StartTimeLeft {left};
    int TopSeen {0}, TopEpg {0};

    const int RecWidth {FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "REC")};
    const int SmlSpaceWidth2 {FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, " ") * 2};

    if (Config.ChannelShowStartTime)
        left += FontCache.GetStringWidth(m_FontName, m_FontHeight, "00:00") + SmlSpaceWidth2;

    PixmapFill(ChanInfoBottomPixmap, Theme.Color(clrChannelBg));
    for (int8_t i {0}; i < 2; i++) {
        bool IsRec {false};
        const bool IsPresent {(i) ? false : true};
        const cEvent *Event {(IsPresent) ? Present : Following};
        if (Event) {
            StartTime = *Event->GetTimeString();  // Start time (Left side)
            // Use of – (EN DASH, U+2013) instead of - (HYPHEN-MINUS, U+002D) for better readability
            // https://en.wikipedia.org/wiki/Wikipedia:Manual_of_Style/Dates_and_numbers#Time_ranges
            StrTime = cString::sprintf("%s–%s", *StartTime, *Event->GetEndTimeString());  // Start – End (Right side)
            StrTimeWidth = FontCache.GetStringWidth(m_FontSmlName, m_FontSmlHeight, "00:00–00:00") + SmlSpaceWidth2;
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
                const std::time_t s {(time(0) - Event->StartTime()) / 60};
                const int sleft {static_cast<int>(EventDuration - s)};

                switch (Config.ChannelTimeLeft) {
                case 0: SeenDur = cString::sprintf("%ld-/%d+ %d min", s, sleft, EventDuration); break;
                case 1: SeenDur = cString::sprintf("%ld- %d min", s, EventDuration); break;
                case 2: SeenDur = cString::sprintf("%d+ %d min", sleft, EventDuration); break;
                }
            } else {  // Following
                TopEpg = m_FontHeight + m_FontSmlHeight;
                TopSeen = m_FontHeight + m_FontSmlHeight * 2;
                EpgColor = clrChannelFontEpgFollow;
                SeenDur = cString::sprintf("%d min", EventDuration);
            }  // if (IsPresent)

            SeenDurWidth = m_FontSml->Width(*SeenDur) + SmlSpaceWidth2;
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
                                            MaxAvailWidth - ((IsRec) ? RecWidth + m_MarginItem2 : 0), m_FontHeight),
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

    if (Config.ChannelIconsShow && m_CurChannel) ChannelIconsDraw(m_CurChannel, true);

    if (Config.TVScraperChanInfoShowPoster) {
        cString MediaPath {""};
        cSize MediaSize {0, 0};  // Width, Height
        GetScraperMediaTypeSize(MediaPath, MediaSize, Present);

        PixmapClear(ChanEpgImagesPixmap);
        DecorBorderClearByFrom(BorderTVSPoster);
        if (MediaPath[0] != '\0') {
            SetMediaSize(m_TVSRect.Size(), MediaSize,
            Config.TVScraperChanInfoPosterSize * 100);  // Set size and apply user setting
            cImage *img {ImgLoader.GetFile(*MediaPath, MediaSize.Width(), MediaSize.Height())};
            if (img) {
                PixmapSetAlpha(ChanEpgImagesPixmap, 255 * Config.TVScraperPosterOpacity * 100);  // Set transparency
                ChanEpgImagesPixmap->DrawImage(cPoint(0, 0), *img);

                const sDecorBorder ib {m_MarginEPGImage + Config.decorBorderChannelEPGSize,
                                    m_TopBarHeight + Config.decorBorderTopBarSize * 2 + m_MarginEPGImage +
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
    }  // if (Config.TVScraperChanInfoShowPoster)
}

void cFlatDisplayChannel::SetMessage(eMessageType Type, const char *Text) {
    (Text) ? MessageSet(Type, Text) : MessageClear();
}

#ifdef USE_ZAPCOCKPIT
//
//* Zapcockpit (extended channel display)
// The complete input handling and list logic is implemented in the patched VDR (cDisplayChannelExtended in menu.c). The
// skin only has to render the channel-/grouplist, the channellist of the selected group, the channel hints and the
// detailed EPG info of the selected channel.
//

// Alpha blends Fg over Bg (straight alpha 'over' operator). Used to compose the channel logo on the 'logo_background'
// image and the list item background in software, because cPixmap::DrawImage() replaces the pixels including their
// alpha value instead of blending them (in the channel info display at the bottom the blending is done by the OSD
// composition of the separate logo pixmap layers)
static tColor ZapAlphaOver(tColor Fg, tColor Bg) {
    const int Af {static_cast<int>((Fg >> 24) & 0xFF)};
    if (Af >= 255) return Fg;
    if (Af <= 0) return Bg;
    const int Ab {static_cast<int>((Bg >> 24) & 0xFF)};
    if (Ab <= 0) return Fg;
    const int A {Af + Ab * (255 - Af) / 255};
    if (A <= 0) return clrTransparent;
    const auto Channel = [&](int Shift) -> int {
        const int Cf {static_cast<int>((Fg >> Shift) & 0xFF)};
        const int Cb {static_cast<int>((Bg >> Shift) & 0xFF)};
        return (Cf * Af + Cb * Ab * (255 - Af) / 255) / A;
    };
    return (static_cast<tColor>(A) << 24) | (Channel(16) << 16) | (Channel(8) << 8) | Channel(0);
}

// Blends the given image at the given position over the existing content of the composed image
static void ZapBlendImage(cImage &Composed, const cImage *Image, int Left, int Top) {  // NOLINT
#ifdef DEBUGIMAGELOADTIME
    dsyslog("flatPlus: ZapBlendImage(%d, %d)", Left, Top);
    cTimeMs Timer;  // Start Timer
#endif

    if (!Image) return;
    const int ComposedWidth {Composed.Width()};
    const int ComposedHeight {Composed.Height()};
    const int ImageWidth {Image->Width()};
    const int ImageHeight {Image->Height()};
    const int StartX {std::max(0, -Left)};
    const int StartY {std::max(0, -Top)};
    const int EndX {std::min(ImageWidth, ComposedWidth - Left)};
    const int EndY {std::min(ImageHeight, ComposedHeight - Top)};
    if (StartX >= EndX || StartY >= EndY) return;

    const tColor *ImageData {Image->Data()};
    tColor *ComposedData {const_cast<tColor *>(Composed.Data())};
    for (int y {StartY}; y < EndY; ++y) {
        const int SrcIndexY {y * ImageWidth};
        const int DstIndexY {(Top + y) * ComposedWidth};
        for (int x {StartX}; x < EndX; ++x) {
            const int SrcIndex {SrcIndexY + x};
            const int DstIndex {DstIndexY + Left + x};
            ComposedData[DstIndex] = ZapAlphaOver(ImageData[SrcIndex], ComposedData[DstIndex]);
        }
    }
#ifdef DEBUGIMAGELOADTIME
    if (Timer.Elapsed() > 0) dsyslog("   Done in %ld ms", Timer.Elapsed());
#endif
}

// Shortens the given text so that it fits into MaxWidth: If the text is too long, it is cut off (UTF-8 safe) and the
// end is replaced by '...' - like the truncated texts in the extended channel lists of skindesigner.
static cString ZapShortenText(const char *Text, const cFont *Font, int MaxWidth) {
    if (!Text || !*Text || MaxWidth <= 0) return "";

    int CurrentWidth {Font->Width(Text)};
    if (CurrentWidth <= MaxWidth) return Text;

    const int EllipsisWidth {Font->Width("...")};
    if (EllipsisWidth >= MaxWidth) return "";

    std::string Shortened {Text};
    while (!Shortened.empty() && CurrentWidth + EllipsisWidth > MaxWidth) {
        std::size_t i {Shortened.size()};  // Remove the last UTF-8 character.
        do {
            --i;
        } while (i > 0 && (Shortened[i] & 0xC0) == 0x80);  // Skip UTF-8 continuation bytes.

        if (i == 0) break;

        const std::string Removed {Shortened.substr(i)};
        const int RemovedWidth {Font->Width(Removed.c_str())};  // Or better: Width of one char
        Shortened.erase(i);
        CurrentWidth -= RemovedWidth;
    }

    return Shortened.empty() ? cString("...") : cString::sprintf("%s...", Shortened.c_str());
}

// Creates the list pixmap if not yet existing and shows it. If the pixmap exists but at a different position
// (channellist and grouplist are anchored at opposite edges) it is recreated at the given position
bool cFlatDisplayChannel::ZapEnsureListPixmap(cPixmap *&Pixmap, const char *Name, const cRect &Rect, bool Clear) {
    if (Pixmap && !(Pixmap->ViewPort() == Rect)) {  // Anchored at the other edge: Recreate
        m_Osd->DestroyPixmap(Pixmap);
        Pixmap = nullptr;
    }
    bool BecameVisible {false};
    if (!Pixmap) {
        Pixmap = CreatePixmap(m_Osd, Name, 5, Rect);
        PixmapClear(Pixmap);
        BecameVisible = true;
    } else if (Clear) {
        PixmapClear(Pixmap);
    }
    if (Pixmap && Pixmap->Layer() < 0) {
        Pixmap->SetLayer(5);
        BecameVisible = true;
    }
    return BecameVisible;
}

void cFlatDisplayChannel::ZapCreateInfoPixmap(const cRect &Rect) {
    if (ZapInfoPixmap && m_Osd) {
        m_Osd->DestroyPixmap(ZapInfoPixmap);
        ZapInfoPixmap = nullptr;
    }
    if (Rect.Width() <= 0) return;  // No space left (extreme list width setting)
    ZapInfoPixmap = CreatePixmap(m_Osd, "ZapInfoPixmap", 5, Rect);
    PixmapFill(ZapInfoPixmap, Theme.Color(clrChannelBg));
}

void cFlatDisplayChannel::ZapHideList(cPixmap *Pixmap) {
    if (Pixmap) {
        PixmapClear(Pixmap);
        Pixmap->SetLayer(-1);  // Negative layer hides the pixmap
    }
}

void cFlatDisplayChannel::ZapHideLists() {
    ZapHideList(ZapListPixmap);
    ZapHideList(ZapList2Pixmap);
    m_ZapNumHints = 0;
    m_ZapHintIndex = 0;
    m_ZapAnimPending = false;  // Cancel a pending show animation
    m_ZapAnimPixmap1 = m_ZapAnimPixmap2 = nullptr;
}

void cFlatDisplayChannel::ZapHideInfo() {
    if (ZapInfoPixmap && m_Osd) {
        m_Osd->DestroyPixmap(ZapInfoPixmap);
        ZapInfoPixmap = nullptr;
    }
}

// Hides all base OSD elements (Top bar, channel info display at the bottom, progress bar, decor borders, weather widget
// and text scrollers) while one of the zapcockpit list views is shown, so that only the lists are visible. The original
// pixmap layers are stored for restoring in ZapShowBaseElements()
void cFlatDisplayChannel::ZapHideBaseElements() {
    if (m_ZapBaseHidden) return;
    m_ZapBaseHidden = true;
    m_ZapEpgImagesHidden = true;  // EPG image and weather widget are also hidden when the base elements are hidden

    cPixmap *BasePixmaps[] {TopBarPixmap,     TopBarIconBgPixmap,      TopBarIconPixmap,   ChanInfoTopPixmap,
                            ChanInfoBottomPixmap, ChanLogoBgPixmap,    ChanLogoPixmap,     ChanIconsPixmap,
                            ChanEpgImagesPixmap,  ProgressBarPixmapBg, ProgressBarPixmap,  ProgressBarMarkerPixmap,
                            DecorPixmap};
    m_ZapHiddenPixmaps.clear();
    m_ZapHiddenPixmaps.reserve(sizeof(BasePixmaps) / sizeof(BasePixmaps[0]));
    for (cPixmap *Pixmap : BasePixmaps) {
        if (Pixmap && Pixmap->Layer() >= 0) {
            m_ZapHiddenPixmaps.emplace_back(Pixmap, Pixmap->Layer());
            Pixmap->SetLayer(-1);
        }
    }
    Scrollers.SetVisible(false);
    WeatherWidget.SetVisible(false);
}

// Hide the weather widget, ChanEpgImagesPixmap and the channel name when the zapcockipt channel info is shown, because
// they would overlap the zapcockpit info pixmap. The original pixmap. Layers are stored for restoring in
// ZapShowBaseElements()
void cFlatDisplayChannel::ZapHideInfoElements() {
    if (m_ZapEpgImagesHidden) return;
    m_ZapEpgImagesHidden = true;

    cPixmap *InfoElementPixmaps[] {ChanEpgImagesPixmap, ChanInfoTopPixmap};
    m_ZapHiddenPixmaps.clear();
    m_ZapHiddenPixmaps.reserve(sizeof(InfoElementPixmaps) / sizeof(InfoElementPixmaps[0]));
    for (cPixmap *Pixmap : InfoElementPixmaps) {
        if (Pixmap && Pixmap->Layer() >= 0) {
            m_ZapHiddenPixmaps.emplace_back(Pixmap, Pixmap->Layer());
            Pixmap->SetLayer(-1);
        }
    }

    // Remove the border around the poster image, because it would overlap the zapcockpit info pixmap
    DecorBorderClearByFrom(BorderTVSPoster);

    // If channel name is drawn without shadow, redraw the border for the bottom pixmap
    if (!Config.ChannelShowNameWithShadow) {
        DecorBorderClearByFrom(BorderChannelInfoBottom);  // Remove the Border
        // Draw the border for the remaining bottom pixmap
        DecorBorderDraw(m_ZapChanInfoBottomDecorBorder, false);
    }

    WeatherWidget.SetVisible(false);
}

void cFlatDisplayChannel::ZapShowBaseElements() {
    if (!m_ZapBaseHidden && !m_ZapEpgImagesHidden) return;
    m_ZapBaseHidden = false;
    m_ZapEpgImagesHidden = false;

    for (const auto &Hidden : m_ZapHiddenPixmaps)
        Hidden.first->SetLayer(Hidden.second);
    m_ZapHiddenPixmaps.clear();  // Clear the list of hidden pixmaps after restoring their layers

    Scrollers.SetVisible(true);
    if (Config.ChannelWeatherShow) WeatherWidget.SetVisible(true);
}

// Fade-in/shift-in animation for newly shown list panels, executed in Flush() after the panel content has been drawn
// (like fadetimezapcockpit/shifttimezapcockpit in skindesigner themes). The lists slide in from the edge they are
// anchored to. Runs blocking for at most one second.
void cFlatDisplayChannel::ZapRunShowAnimation() {
    m_ZapAnimPending = false;
    cPixmap *Pixmaps[2] {m_ZapAnimPixmap1, m_ZapAnimPixmap2};
    m_ZapAnimPixmap1 = m_ZapAnimPixmap2 = nullptr;

    const int ShiftTime {std::clamp(Config.ChannelZapcockpitShiftTime, 0, 1000)};
    const int FadeTime {std::clamp(Config.ChannelZapcockpitFadeTime, 0, 1000)};
    const int TotalTime {std::max(ShiftTime, FadeTime)};
    if (TotalTime <= 0 || (!Pixmaps[0] && !Pixmaps[1])) return;

    static constexpr int kFrameTime {10};  // Duration of one animation step in ms
    for (int t {kFrameTime}; t < TotalTime; t += kFrameTime) {
        for (cPixmap *Pixmap : Pixmaps) {
            if (!Pixmap || Pixmap->Layer() < 0) continue;
            const cRect ViewPort {Pixmap->ViewPort()};
            if (ShiftTime > 0) {  // Slide the content in from the anchored edge
                const double Progress {std::min(1.0, static_cast<double>(t) / ShiftTime)};
                const int Offset {static_cast<int>((1.0 - Progress) * ViewPort.Width())};
                const bool LeftAnchored {ViewPort.X() < m_OsdWidth / 2};
                Pixmap->SetDrawPortPoint(cPoint(LeftAnchored ? -Offset : Offset, 0));
            }
            if (FadeTime > 0) {  // Fade the content in
                const double Progress {std::min(1.0, static_cast<double>(t) / FadeTime)};
                PixmapSetAlpha(Pixmap, static_cast<int>(ALPHA_OPAQUE * Progress));
            }
        }
        m_Osd->Flush();
        cCondWait::SleepMs(kFrameTime);
    }

    for (cPixmap *Pixmap : Pixmaps) {  // Ensure the final state
        if (Pixmap) {
            Pixmap->SetDrawPortPoint(cPoint(0, 0));
            PixmapSetAlpha(Pixmap, ALPHA_OPAQUE);
        }
    }
}

// Shows/hides the panels for the given view. The list contents are only cleared when a view of a different kind is
// entered, because the VDR state machine partially redraws the lists while scrolling and does not repaint the group
// list when returning from the group channel list.
void cFlatDisplayChannel::SetViewType(eDisplaychannelView ViewType) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayChannel::SetViewType(%d)", ViewType);
#endif
    const eDisplaychannelView OldViewType {m_ZapViewType};
    m_ZapViewType = ViewType;
    switch (ViewType) {
    case dcDefault:
        ZapHideLists();
        ZapHideInfo();
        ZapShowBaseElements();
        break;
    case dcChannelInfo:  // Info pixmap is (re)created in SetChannelInfo()
        ZapHideLists();
        ZapShowBaseElements();
        ZapHideInfoElements();  // Hide the weather widget, the EPG image and the channel name, because they would
                                // overlap the info pixmap
        break;
    case dcChannelList:
    case dcChannelListInfo: {
        // Keep the list content when toggling between list and list + info
        const bool Clear {(OldViewType != dcChannelList) && (OldViewType != dcChannelListInfo)};
        if (ZapEnsureListPixmap(ZapListPixmap, "ZapListPixmap", m_ZapListRectChan, Clear)) {
            m_ZapAnimPending = true;  // Newly shown: Animate in Flush()
            m_ZapAnimPixmap1 = ZapListPixmap;
        }
        ZapHideList(ZapList2Pixmap);
        if (ViewType == dcChannelList) ZapHideInfo();  // Info pixmap is (re)created in SetChannelInfo()
        ZapHideBaseElements();
        break;
        }
    case dcGroupsList: {
        // Keep the group list content when returning from the group channel list
        const bool Clear {(OldViewType != dcGroupsList) && (OldViewType != dcGroupsChannelList) &&
                          (OldViewType != dcGroupsChannelListInfo)};
        if (ZapEnsureListPixmap(ZapListPixmap, "ZapListPixmap", m_ZapListRectGroup, Clear)) {
            m_ZapAnimPending = true;  // Newly shown: Animate in Flush()
            m_ZapAnimPixmap1 = ZapListPixmap;
        }
        ZapHideList(ZapList2Pixmap);
        ZapHideInfo();
        ZapHideBaseElements();
        break;
        }
    case dcGroupsChannelList:
    case dcGroupsChannelListInfo: {
        // The group list (1st column) stays visible; the channellist of the
        // selected group is shown in the 2nd column beside it
        if (ZapEnsureListPixmap(ZapListPixmap, "ZapListPixmap", m_ZapListRectGroup, false)) {
            m_ZapAnimPending = true;  // Newly shown: Animate in Flush()
            m_ZapAnimPixmap1 = ZapListPixmap;
        }
        const bool Clear {(OldViewType != dcGroupsChannelList) && (OldViewType != dcGroupsChannelListInfo)};
        if (ZapEnsureListPixmap(ZapList2Pixmap, "ZapList2Pixmap", m_ZapList2RectGroup, Clear)) {
            m_ZapAnimPending = true;  // Newly shown: Animate in Flush()
            m_ZapAnimPixmap2 = ZapList2Pixmap;
        }
        if (ViewType == dcGroupsChannelList) ZapHideInfo();  // Info pixmap is (re)created in SetChannelInfo()
        ZapHideBaseElements();
        break;
        }
    default:
        break;
    }
}

// The grouplist uses single line items, the channellists use higher items with three text lines. MaxItems() is queried
// by the VDR state machine after SetViewType(), so the value can depend on the current view
int cFlatDisplayChannel::MaxItems() {
    return (m_ZapViewType == dcGroupsList) ? m_ZapMaxItemsGroup : m_ZapMaxItemsChan;
}

bool cFlatDisplayChannel::KeyRightOpensChannellist() {
    return Config.ChannelZapcockpitKeyRightOpensList;
}

// Draws one item of a channellist (channellist, group channel list, channel hints): The channel logo at the left (if
// available) and three text lines right of it: Remaining time of the running event (small font), title of the running
// event and start-/end time plus title of the following event (small font) - like the extended channel lists of
// skindesigner
void cFlatDisplayChannel::ZapDrawChannelItem(cPixmap *Pixmap, const cChannel *Channel, int Index, bool Current) {
    if (!Pixmap || !Channel || Index < 0) return;
    // The hints panel is smaller than the full height channellist panels,
    // so the capacity is calculated from the pixmap actually drawn to
    if (Index >= Pixmap->ViewPort().Height() / m_ZapItemHeightChan) return;

    const int Width {m_ZapColWidth};
    const int Top {Index * m_ZapItemHeightChan};
    const int InnerHeight {m_ZapItemHeightChan - Config.MenuItemPadding};
    const tColor ColorFg {Theme.Color(Current ? clrItemCurrentFont : clrItemSelableFont)};
    const tColor ColorBg {Theme.Color(Current ? clrItemCurrentBg : clrItemSelableBg)};

    Pixmap->DrawRectangle(cRect(0, Top, Width, InnerHeight), ColorBg);

    // Channel logo on the 'logo_background' image (like in the channel info display at the bottom) in a fixed width
    // column at the left, so that the text lines of all items are aligned. Item background color, 'logo_background' and
    // the logo are alpha blended in software into one image, because cPixmap::DrawImage() would replace the pixels
    // including their alpha value (the semi transparent parts of the images would then punch through to the live
    // picture instead of showing the layer below)
    const int LogoHeight {InnerHeight - m_MarginItem2};
    const int LogoWidth {static_cast<int>(LogoHeight * 1.34f)};  // 1.34 = 4:3
    int left {m_MarginItem};
    int BgWidth {LogoWidth}, BgHeight {LogoHeight};
    cImage *ImgBg {ImgLoader.GetLogoBg(LogoWidth, LogoHeight)};  // Load 'logo_background'
    if (ImgBg) {
        BgWidth = ImgBg->Width();
        BgHeight = ImgBg->Height();
    }
    cImage *img {ImgLoader.GetLogo(Channel->Name(), BgWidth - 4, BgHeight - 4)};
    if (img) {  // Draw background and logo only if a channel logo is available
        cImage Composed(cSize(BgWidth, BgHeight));
        Composed.Fill(ColorBg);  // Base: Item background color
        ZapBlendImage(Composed, ImgBg, 0, 0);  // 'logo_background' over it
        ZapBlendImage(Composed, img, (BgWidth - img->Width()) / 2, (BgHeight - img->Height()) / 2);  // Logo on top
        Pixmap->DrawImage(cPoint(left + (LogoWidth - BgWidth) / 2, Top + (InnerHeight - BgHeight) / 2), Composed);
    }
    left += LogoWidth + m_MarginItem2;
    const int MaxTextWidth {Width - left - m_MarginItem};

    // Get the running and the following event of the channel
    const cEvent *Present {nullptr}, *Following {nullptr};
    {
        LOCK_SCHEDULES_READ;  // Creates local const cSchedules *Schedules
        const cSchedule *Schedule {Schedules->GetSchedule(Channel)};
        if (Schedule) {
            Present = Schedule->GetPresentEvent();
            Following = Schedule->GetFollowingEvent();
        }
    }

    int top {Top + m_MarginItem / 2};
    if (Present) {
        // 1st line (small font): Remaining time of the running event
        const int Remaining {static_cast<int>((Present->EndTime() - time(0)) / 60)};
        const cString StrRemaining {cString::sprintf("+%d min", std::max(0, Remaining))};
        Pixmap->DrawText(cPoint(left, top), *StrRemaining, ColorFg, ColorBg, m_ZapFontSml, MaxTextWidth);
        top += m_ZapFontSmlHeight;

        // 2nd line: Title of the running event
        Pixmap->DrawText(cPoint(left, top), *ZapShortenText(Present->Title(), m_ZapFont, MaxTextWidth), ColorFg,
                         ColorBg, m_ZapFont, MaxTextWidth);
        top += m_ZapFontHeight;
    } else {
        // No EPG data: Show the channel name instead, so the item is not empty
        top += m_ZapFontSmlHeight;
        Pixmap->DrawText(cPoint(left, top), *ZapShortenText(Channel->Name(), m_ZapFont, MaxTextWidth), ColorFg,
                         ColorBg, m_ZapFont, MaxTextWidth);
        top += m_ZapFontHeight;
    }

    if (Following) {
        // 3rd line (small font): Start-/end time and title of the following event
        const cString StrFollowing {cString::sprintf("%s–%s: %s", *Following->GetTimeString(),
                                                      *Following->GetEndTimeString(), Following->Title())};
        Pixmap->DrawText(cPoint(left, top), *ZapShortenText(*StrFollowing, m_ZapFontSml, MaxTextWidth),
                         clrChannelFontEpgFollow, ColorBg, m_ZapFontSml, MaxTextWidth);
    }
}

// Draws one item of the grouplist (one text line)
void cFlatDisplayChannel::ZapDrawGroupItem(cPixmap *Pixmap, const cString &Text, int Index, bool Current) {
    if (!Pixmap || Index < 0) return;
    if (Index >= Pixmap->ViewPort().Height() / m_ZapItemHeightGroup) return;

    const int Width {m_ZapColWidth};
    const int Top {Index * m_ZapItemHeightGroup};
    const tColor ColorFg {Theme.Color(Current ? clrItemCurrentFont : clrItemSelableFont)};
    const tColor ColorBg {Theme.Color(Current ? clrItemCurrentBg : clrItemSelableBg)};

    Pixmap->DrawRectangle(cRect(0, Top, Width, m_ZapFontHeight), ColorBg);
    Pixmap->DrawText(cPoint(m_MarginItem, Top), *ZapShortenText(*Text, m_ZapFont, Width - m_MarginItem2), ColorFg,
                     ColorBg, m_ZapFont, Width - m_MarginItem2);
}

// Also used for the channellist of the selected group: The VDR state machine draws that list via SetChannelList() too,
// so the target panel is selected by the current view type
void cFlatDisplayChannel::SetChannelList(const cChannel *Channel, int Index, bool Current) {
    if (!Channel) return;
    ZapDrawChannelItem(ZapIsGroupChannelView() ? ZapList2Pixmap : ZapListPixmap, Channel, Index, Current);
}

void cFlatDisplayChannel::SetGroupList(const char *Group, int NumChannels, int Index, bool Current) {
    const cString Text {cString::sprintf("%s (%d)", Group ? Group : "", NumChannels)};
    ZapDrawGroupItem(ZapListPixmap, Text, Index, Current);
}

void cFlatDisplayChannel::SetGroupChannelList(const cChannel *Channel, int Index, bool Current) {
    if (!Channel) return;  // Not called by the current patch, but part of the interface
    ZapDrawChannelItem(ZapList2Pixmap, Channel, Index, Current);
}

void cFlatDisplayChannel::ClearList() {
    PixmapClear(ZapIsGroupChannelView() ? ZapList2Pixmap : ZapListPixmap);
}

void cFlatDisplayChannel::SetNumChannelHints(int Num) {
    // Hints are shown in a smaller panel between top bar and channel info display while entering a channel number; the
    // other OSD elements stay visible there (unlike in the full height list views)
    ZapEnsureListPixmap(ZapListPixmap, "ZapListPixmap", m_ZapHintsRect, true);
    m_ZapNumHints = Num;
    m_ZapHintIndex = 0;
}

void cFlatDisplayChannel::SetChannelHint(const cChannel *Channel) {
    if (!Channel) return;
    ZapDrawChannelItem(ZapListPixmap, Channel, ++m_ZapHintIndex, false);
}

// Draws the detailed EPG info for the given channel. Depending on the current view type the full width (dcChannelInfo:
// 2nd 'Ok' on the current channel) or the area beside the visible list panels is used.
void cFlatDisplayChannel::SetChannelInfo(const cChannel *Channel) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayChannel::SetChannelInfo(%s)", Channel ? Channel->Name() : "nullptr");
    cTimeMs Timer;  // Set Timer
#endif

    if (!Channel) return;

    const bool IsWideInfo {m_ZapViewType == dcChannelInfo};  // Full width info pixmap (2nd 'Ok' on the current channel)
    const cRect &Rect {(IsWideInfo)                                 ? m_ZapInfoWideRect
                       : (m_ZapViewType == dcGroupsChannelListInfo) ? m_ZapInfoRectGroup
                                                                    : m_ZapInfoRectChan};
    ZapCreateInfoPixmap(Rect);
    if (!ZapInfoPixmap) return;

    const int Width {Rect.Width()};
    const int Height {Rect.Height()};
    const int MaxTextWidth {Width - m_MarginItem2 * 2};
    int top {m_MarginItem};

    // Header: Channel number and name. When view is wide (2nd 'Ok') we use the same size as in normal channel info.
    const cFont *FontHdr {(IsWideInfo) ? m_FontBig : m_Font};  // Big or normal
    const int FontHdrHeight {(IsWideInfo) ? m_FontBigHeight : m_FontHeight};  // Big or normal
    const cString Header {cString::sprintf("%d  %s", Channel->Number(), Channel->Name())};
    ZapInfoPixmap->DrawText(cPoint(m_MarginItem2, top), *ZapShortenText(*Header, FontHdr, MaxTextWidth),
                            Theme.Color(clrChannelFontTitle),
                            Theme.Color(clrChannelBg), FontHdr, MaxTextWidth);
    top += FontHdrHeight;
    ZapInfoPixmap->DrawRectangle(cRect(m_MarginItem2, top + m_MarginItem / 2, MaxTextWidth, m_LineWidth),
                                 Theme.Color(clrChannelFontTitle));
    top += m_MarginItem2 + m_LineMargin;

    // Use m_FontMedium when m_ZapViewType == dcChannelInfo, because the info pixmap is wider and can show more text
    // lines. Use m_FontSml for other view types.
    const cFont *Font {(IsWideInfo) ? m_FontMedium : m_FontSml};  // Medium or small
    const int FontHeight {(IsWideInfo) ? m_FontMediumHeight : m_FontSmlHeight};  // Medium or small

    const cEvent *Present {nullptr}, *Following {nullptr};
    if (IsWideInfo) {
        // Use the events from SetEvents() instead of querying the schedule again
        Present = m_Present;
        Following = m_Following;
    } else {
        LOCK_SCHEDULES_READ;  // Creates local const cSchedules *Schedules
        const cSchedule *Schedule {Schedules->GetSchedule(Channel)};
        if (Schedule) {
            Present = Schedule->GetPresentEvent();
            Following = Schedule->GetFollowingEvent();
        }
    }

    if (!Present) {
        ZapInfoPixmap->DrawText(cPoint(m_MarginItem2, top), tr("No EPG info available."),
                                Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), Font, MaxTextWidth);
        return;
    }

    // Reserve one line at the bottom for the following event
    const int BottomFollowing {(Following) ? Height - FontHeight - m_MarginItem : Height};

    // Present event: Time, title, short text and description
    const cString StrTime {cString::sprintf("%s–%s  (%d min)", *Present->GetTimeString(),
                                             *Present->GetEndTimeString(), Present->Duration() / 60)};
    ZapInfoPixmap->DrawText(cPoint(m_MarginItem2, top), *StrTime, Theme.Color(clrChannelFontEpg),
                            Theme.Color(clrChannelBg), Font, MaxTextWidth);
    top += FontHeight + m_MarginItem;

    if (top + m_FontHeight <= BottomFollowing) {  // Title
        ZapInfoPixmap->DrawText(cPoint(m_MarginItem2, top), *ZapShortenText(Present->Title(), m_Font, MaxTextWidth),
                                Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), m_Font, MaxTextWidth);
        top += m_FontHeight;
    }
    if (!isempty(Present->ShortText()) && (top + FontHeight <= BottomFollowing)) {  // Short text
        ZapInfoPixmap->DrawText(cPoint(m_MarginItem2, top),
                                *ZapShortenText(Present->ShortText(), Font, MaxTextWidth),
                                Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg), Font,
                                MaxTextWidth);
        top += FontHeight;
    }
    top += m_MarginItem2;  // Some space between the short text and the description.

    if (!isempty(Present->Description())) {  // Description (wrapped, cut off at the bottom)
        cTextFloatingWrapper Description;  // Wraps the description text into lines of the given width and font
        Description.Set(Present->Description(), Font, MaxTextWidth);
        const int NumLines {Description.Lines()};
        std::string Line {""};
        Line.reserve(256);  // Avoids repeated memory allocations when appending the lines

        for (int i {0}; i < NumLines && (top + FontHeight <= BottomFollowing); ++i) {
            Line = Description.GetLine(i);
            // Justify all lines except the last one
            if (Config.MenuEventRecordingViewJustify == 1 && i < (NumLines - 1))
                JustifyLine(Line, Font, MaxTextWidth);

            ZapInfoPixmap->DrawText(cPoint(m_MarginItem2, top), Line.c_str(),
                                    Theme.Color(clrChannelFontEpg), Theme.Color(clrChannelBg), Font,
                                    MaxTextWidth);
            top += FontHeight;
        }
    }

    if (Following) {  // Following event in the reserved line at the bottom
        const cString Next {cString::sprintf("%s  %s%s%s", *Following->GetTimeString(), Following->Title(),
                                              isempty(Following->ShortText()) ? "" : " - ",
                                              isempty(Following->ShortText()) ? "" : Following->ShortText())};
        ZapInfoPixmap->DrawText(cPoint(m_MarginItem2, Height - FontHeight),
                                *ZapShortenText(*Next, Font, MaxTextWidth),
                                Theme.Color(clrChannelFontEpgFollow), Theme.Color(clrChannelBg), Font,
                                MaxTextWidth);
    }
#ifdef DEBUGFUNCSCALL
    dsyslog("   SetChannelInfo() done in %ld ms", Timer.Elapsed());
#endif
}
#endif  // USE_ZAPCOCKPIT

void cFlatDisplayChannel::SignalQualityDraw() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayChannel::SignalQualityDraw()");
#endif

    if (!ChanInfoBottomPixmap) return;

    const int SignalStrength {cDevice::ActualDevice()->SignalStrength()};
    const int SignalQuality {cDevice::ActualDevice()->SignalQuality()};
    if (m_LastSignalStrength == SignalStrength && m_LastSignalQuality == SignalQuality) return;

    m_LastSignalStrength = SignalStrength;
    m_LastSignalQuality = SignalQuality;

    m_SignalFont = FontCache.GetFont(Setup.FontOsd, Config.decorProgressSignalSize);
    const int SignalFontHeight {FontCache.GetFontHeight(Setup.FontOsd, Config.decorProgressSignalSize)};
    const int left {m_MarginItem2};
    int top {m_HeightBottom -
             (Config.decorProgressSignalSize * 2 + m_MarginItem2)};  // One margin for progress bar to bottom

    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "STR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), m_SignalFont);
    const int ProgressLeft {left +
                            std::max(FontCache.GetStringWidth(m_FontName, SignalFontHeight, "STR "),
                                     FontCache.GetStringWidth(m_FontName, SignalFontHeight, "SNR ")) +
                            m_MarginItem};
    const int ProgressWidth {m_ChannelWidth / 4 - ProgressLeft - m_MarginItem};
    cRect ProgressBar {ProgressLeft, top, ProgressWidth, Config.decorProgressSignalSize};
    ProgressBarDrawRaw(ChanInfoBottomPixmap, ChanInfoBottomPixmap, ProgressBar, ProgressBar, SignalStrength, 100,
                       Config.decorProgressSignalFg, Config.decorProgressSignalBarFg, Config.decorProgressSignalBg,
                       Config.decorProgressSignalType, false, Config.SignalQualityUseColors);

    top += Config.decorProgressSignalSize + m_MarginItem;
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), "SNR", Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), m_SignalFont);
    ProgressBar.SetY(top);
    ProgressBarDrawRaw(ChanInfoBottomPixmap, ChanInfoBottomPixmap, ProgressBar, ProgressBar, SignalQuality, 100,
                       Config.decorProgressSignalFg, Config.decorProgressSignalBarFg, Config.decorProgressSignalBg,
                       Config.decorProgressSignalType, false, Config.SignalQualityUseColors);

    m_SignalStrengthRight = ProgressLeft + ProgressWidth;
}

// You need oscam min rev 10653
// You need dvbapi min commit 85da7b2
void cFlatDisplayChannel::DvbapiInfoDraw() {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: DvbapiInfoDraw()");
#endif
    static cPlugin *pDVBApi {nullptr};
    static bool dvbapiChecked {false};

    if (!dvbapiChecked) {
        pDVBApi = cPluginManager::GetPlugin("dvbapi");
        dvbapiChecked = true;
        dsyslog("flatPlus: DVBApi plugin %s", pDVBApi ? "found and loaded" : "not found");
    }
    if (!pDVBApi) return;
    if (!m_CurChannel) return;  // Not set yet when an OSD size change re-created this object

    sDVBAPIEcmInfo ecmInfo {.sid = static_cast<uint16_t>(m_CurChannel->Sid()), .ecmtime = 0, .hops = -1};

    if (!pDVBApi->Service("GetEcmInfo", &ecmInfo)) {
#ifdef DEBUGFUNCSCALL
        const int *caids = m_CurChannel->Caids();
        if (caids && caids[0] != 0) {
            dsyslog("   No ECM info for channel %s (SID: %d)", m_CurChannel->Name(), m_CurChannel->Sid());
        }
#endif
        return;
    }

#ifdef DEBUGFUNCSCALL
    dsyslog("   ChannelSid: %d, Channel: %s", m_CurChannel->Sid(), m_CurChannel->Name());
    dsyslog("   CAID: %d, Card system: %s", ecmInfo.caid, *ecmInfo.cardsystem);
    dsyslog("   Reader: %s", *ecmInfo.reader);
    dsyslog("   From: %s, Hops: %d", *ecmInfo.from, ecmInfo.hops);
    dsyslog("   Protocol: %s", *ecmInfo.protocol);
#endif

    if (ecmInfo.hops < 0 || ecmInfo.ecmtime == 0 || ecmInfo.ecmtime > 9999) return;

    int left {m_SignalStrengthRight + m_MarginItem10};
    const int SignalBarsHeight {Config.decorProgressSignalSize * 2 + m_MarginItem};
    static constexpr uint32_t kCharCode {0x0044};  // U+0044 LATIN CAPITAL LETTER D

    m_DvbapiInfoFont = FontCache.GetFont(Setup.FontOsd, SignalBarsHeight);
    const int DvbapiInfoFontHeight {FontCache.GetFontHeight(Setup.FontOsd, SignalBarsHeight)};
    const int FontAscender {FontCache.GetFontAscender(Setup.FontOsd, SignalBarsHeight)};
    const int GlyphSize {FontCache.GetGlyphSize(Setup.FontOsd, kCharCode, SignalBarsHeight)};
    const int TopOffset {(FontAscender - GlyphSize) / 2};  // Center vertically
    const int top {m_HeightBottom - SignalBarsHeight - TopOffset - m_MarginItem};

    cString DvbapiInfoText {"DVBAPI: "};
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), m_DvbapiInfoFont);

    left += FontCache.GetStringWidth(Setup.FontOsd, DvbapiInfoFontHeight, DvbapiInfoText) + m_MarginItem;

    cString IconName {cString::sprintf("crypt_%s", *ecmInfo.cardsystem)};
    cImage *img {ImgLoader.GetIcon(*IconName, kIconMaxSize, DvbapiInfoFontHeight)};
    if (!img) {
        img = ImgLoader.GetIcon("crypt_unknown", kIconMaxSize, DvbapiInfoFontHeight);
        dsyslog("flatPlus: Unknown card system: %s (CAID: %d)", *ecmInfo.cardsystem, ecmInfo.caid);
    }
    if (img) {  // Draw the card system icon
        ChanIconsPixmap->DrawImage(cPoint(left, top), *img);
        left += img->Width() + m_MarginItem;
    }

    DvbapiInfoText = cString::sprintf(" %s (%d ms)", *ecmInfo.reader, ecmInfo.ecmtime);
    if (ecmInfo.hops > 1) DvbapiInfoText.Append(cString::sprintf(" (%d hops)", ecmInfo.hops));

    // Store the width of the drawn dvbapi info text for the next draw call, so that we can ensure that the text
    // is drawn at the correct position even if the text changes (e.g. when the channel is changed).
    // This is done by storing the maximum width of the text seen so far.
    m_LastDvbapiInfoTextWidth = std::max(m_DvbapiInfoFont->Width(*DvbapiInfoText), m_LastDvbapiInfoTextWidth);
    ChanInfoBottomPixmap->DrawText(cPoint(left, top), *DvbapiInfoText, Theme.Color(clrChannelSignalFont),
                                   Theme.Color(clrChannelBg), m_DvbapiInfoFont, m_LastDvbapiInfoTextWidth);
}

void cFlatDisplayChannel::Flush() {
#ifdef USE_ZAPCOCKPIT
    // Run a pending show animation of the zapcockpit lists after their
    // content has been drawn by the VDR state machine
    if (m_ZapAnimPending) ZapRunShowAnimation();
#endif

    if (m_Present) {
        const time_t now {time(0)};
        const time_t Current {(now > m_Present->StartTime()) ? now - m_Present->StartTime() : 0};
        ProgressBarDraw(Current, m_Present->Duration());
    }

    if (Config.ChannelIconsShow) {
        cDevice::PrimaryDevice()->GetVideoSize(m_ScreenWidth, m_ScreenHeight, m_ScreenAspect);
        if (m_ScreenWidth != m_LastScreenWidth) {
            m_LastScreenWidth = m_ScreenWidth;
            ChannelIconsDraw(m_CurChannel, true);  // Full redraw when resolution changes
        }
    }

    if (Config.SignalQualityShow) SignalQualityDraw();

    if (Config.ChannelDvbapiInfoShow) DvbapiInfoDraw();

    TopBarUpdate();
    m_Osd->Flush();
}

void cFlatDisplayChannel::PreLoadImages() {
    const int height {m_HeightImageLogo - m_MarginItem2};
    int ImageBgWidth {static_cast<int>(height * 1.34f)};
    int ImageBgHeight {height};

    // Load 'logo_background' and determine if logo was found in channel logo path
    cImage *img {ImgLoader.GetLogo("logo_background", ImageBgWidth, ImageBgHeight, true)};  // Miss is expected
    if (img) {
        g_LogoBgOverwrite = true;  // Used for GetLogoBg()
    } else {
        img = ImgLoader.GetIcon("logo_background", ImageBgWidth, ImageBgHeight);
    }
    dsyslog("flatPlus: cFlatDisplayChannel::PreLoadImages() Using 'logo_background' from %s path",
            g_LogoBgOverwrite ? "logo" : "theme");

    if (img) {
        ImageBgHeight = img->Height();
        ImageBgWidth = img->Width();
    }
    ImgLoader.GetIcon("radio", ImageBgWidth - 10, ImageBgHeight - 10);
    ImgLoader.GetIcon("tv", ImageBgWidth - 10, ImageBgHeight - 10);

    // Preload channel icons
    uint16_t i {0};
    LOCK_CHANNELS_READ;  // Creates local const cChannels *Channels
    for (const cChannel *Channel {Channels->First()}; Channel && i < kLogoPreCache; Channel = Channels->Next(Channel)) {
        if (!Channel->GroupSep()) {  // Don'now cache named channel group logo
            img = ImgLoader.GetLogo(Channel->Name(), ImageBgWidth - 4, ImageBgHeight - 4);
            if (img) ++i;
        }
    }  // for cChannel

    if (Config.ChannelIconsShow) {
        ImgLoader.GetIcon("crypted", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("uncrypted", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("unknown_asp", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("43", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("169", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("169w", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("221", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("7680x4320", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("3840x2160", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("1920x1080", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("1440x1080", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("1280x720", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("960x720", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("704x576", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("720x576", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("544x576", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("528x576", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("480x576", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("352x576", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("unknown_res", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("uhd", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("hd", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("sd", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("audio_dolby", kIconMaxSize, m_FontSmlHeight);
        ImgLoader.GetIcon("audio_stereo", kIconMaxSize, m_FontSmlHeight);

        // Audio tracks (displaytracks.c)
        ImgLoader.GetIcon("tracks_ac3", kIconMaxSize, m_FontHeight);
        ImgLoader.GetIcon("tracks_stereo", kIconMaxSize, m_FontHeight);
    }
}
