/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/status.h>

#include <utility>
#include <vector>

#include "./baserender.h"

#ifdef USE_ZAPCOCKPIT
class cFlatDisplayChannel : public cFlatBaseRender, public cSkinDisplayChannelExtended, public cStatus {
#else
class cFlatDisplayChannel : public cFlatBaseRender, public cSkinDisplayChannel, public cStatus {
#endif
 public:
        explicit cFlatDisplayChannel(bool WithInfo);
        ~cFlatDisplayChannel() override;
        void SetChannel(const cChannel *Channel, int Number) override;
        void SetEvents(const cEvent *Present, const cEvent *Following) override;
        void SetMessage(eMessageType Type, const char *Text) override;
        void Flush() override;

#ifdef USE_ZAPCOCKPIT
        // Zapcockpit (extended channel display) support
        // The whole input handling is done inside the patched VDR
        // (cDisplayChannelExtended in menu.c), the skin only renders.
        void SetViewType(eDisplaychannelView ViewType) override;
        int MaxItems() override;
        bool KeyRightOpensChannellist() override;
        void SetChannelInfo(const cChannel *Channel) override;
        void SetChannelList(const cChannel *Channel, int Index, bool Current) override;
        void SetGroupList(const char *Group, int NumChannels, int Index, bool Current) override;
        void SetGroupChannelList(const cChannel *Channel, int Index, bool Current) override;
        void ClearList() override;
        void SetNumChannelHints(int Num) override;
        void SetChannelHint(const cChannel *Channel) override;
#endif

        void PreLoadImages();

 private:
        const cEvent *m_Present {nullptr};

        int m_ChannelWidth {0}, m_ChannelHeight {0};

        const cChannel *m_CurChannel {nullptr};

        cPixmap *ChanInfoTopPixmap {nullptr};
        cPixmap *ChanInfoBottomPixmap {nullptr};
        cPixmap *ChanLogoPixmap {nullptr};
        cPixmap *ChanLogoBgPixmap {nullptr};
        cPixmap *ChanIconsPixmap {nullptr};
        cPixmap *ChanEpgImagesPixmap {nullptr};

        int m_ScreenWidth {-1}, m_LastScreenWidth {-1};
        int m_ScreenHeight {0};
        double m_ScreenAspect {0.0};
        int m_HeightBottom {0}, m_HeightImageLogo {0};

        cFont *m_SignalFont {nullptr};
        int m_LastSignalStrength {-1}, m_LastSignalQuality {-1};
        int m_SignalStrengthRight {0};

        cFont *m_DvbapiInfoFont {nullptr};
        int m_LastDvbapiInfoTextWidth {0};

        // TVScraper
        cRect m_TVSRect {0, 0, 0, 0};

        // TextScroller
        cTextScrollers Scrollers;

        bool m_IsRadioChannel {false};

#ifdef USE_ZAPCOCKPIT
        // Zapcockpit rendering
        // The group list and the channellist of the selected group are two
        // separate panels which are visible side by side, because the VDR
        // state machine does not redraw the group list when returning from
        // the group channel list (dcGroupsChannelList -> dcGroupsList).
        // The lists slide in from the side of the pressed key: The list opened
        // with 'right' is anchored at the left edge, the list opened with
        // 'left' at the right edge (mirrored when the setup option
        // 'Zapcockpit: Key right opens channellist' is disabled).
        // While a list view is shown, all other OSD elements (top bar and the
        // channel info display at the bottom) are hidden.
        cPixmap *ZapListPixmap {nullptr};      // 1st column: Channellist, grouplist, channel hints
        cPixmap *ZapList2Pixmap {nullptr};     // 2nd column: Channellist of the selected group
        cPixmap *ZapInfoPixmap {nullptr};      // Detailed EPG info panel (beside the lists or full width)

        eDisplaychannelView m_ZapViewType {dcDefault};
        cRect m_ZapListRectChan {0, 0, 0, 0};   // Channellist panel
        cRect m_ZapInfoRectChan {0, 0, 0, 0};   // Info panel beside the channellist
        cRect m_ZapListRectGroup {0, 0, 0, 0};  // Grouplist panel (opposite side of the channellist)
        cRect m_ZapList2RectGroup {0, 0, 0, 0};  // Channellist of the selected group (beside the grouplist)
        cRect m_ZapInfoRectGroup {0, 0, 0, 0};   // Info panel beside the group channel list
        cRect m_ZapInfoWideRect {0, 0, 0, 0};    // Info panel over the full width (dcChannelInfo)
        cRect m_ZapHintsRect {0, 0, 0, 0};       // Channel hints panel (base OSD elements stay visible)
        // Fonts of the list items: The same fonts as in the channel info
        // display at the bottom, scaled by 'ChannelZapcockpitFontSize'
        cFont *m_ZapFont {nullptr}, *m_ZapFontSml {nullptr};
        int m_ZapFontHeight {0}, m_ZapFontSmlHeight {0};

        int m_ZapColWidth {0};                 // Width of one list column
        int m_ZapItemHeightChan {0};           // Height of one channellist item (logo + 3 text lines)
        int m_ZapItemHeightGroup {0};          // Height of one grouplist item (one text line)
        int m_ZapMaxItemsChan {0};             // Number of items fitting into a channellist panel
        int m_ZapMaxItemsGroup {0};            // Number of items fitting into the grouplist panel
        int m_ZapHintIndex {0};                // Running index while drawing channel hints
        int m_ZapNumHints {0};                 // Number of currently displayed channel hints

        bool m_ZapBaseHidden {false};          // Base OSD elements currently hidden?
        std::vector<std::pair<cPixmap *, int>> m_ZapHiddenPixmaps;  // Hidden pixmaps with their original layer

        // Show animation (fade-in/shift-in of newly shown list panels),
        // executed in Flush() after the panel content has been drawn
        bool m_ZapAnimPending {false};
        cPixmap *m_ZapAnimPixmap1 {nullptr}, *m_ZapAnimPixmap2 {nullptr};

        bool ZapIsGroupChannelView() const {
            return (m_ZapViewType == dcGroupsChannelList) || (m_ZapViewType == dcGroupsChannelListInfo);
        }
        bool ZapEnsureListPixmap(cPixmap *&Pixmap, const char *Name, const cRect &Rect, bool Clear);  // NOLINT
        void ZapCreateInfoPixmap(const cRect &Rect);
        void ZapHideList(cPixmap *Pixmap);
        void ZapHideLists();
        void ZapHideInfo();
        void ZapHideBaseElements();
        void ZapShowBaseElements();
        void ZapRunShowAnimation();
        void ZapDrawChannelItem(cPixmap *Pixmap, const cChannel *Channel, int Index, bool Current);
        void ZapDrawGroupItem(cPixmap *Pixmap, const cString &Text, int Index, bool Current);
#endif

        void SignalQualityDraw();
        void ChannelIconsDraw(const cChannel *Channel, bool Resolution);
        void DvbapiInfoDraw();
};
