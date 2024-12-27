/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/status.h>

#include "./baserender.h"
#include "./flat.h"
#include "./services/dvbapi.h"
#include "./services/scraper2vdr.h"

class cFlatDisplayChannel : public cFlatBaseRender, public cSkinDisplayChannel, public cStatus {
 public:
        explicit cFlatDisplayChannel(bool WithInfo);
        virtual ~cFlatDisplayChannel();
        virtual void SetChannel(const cChannel *Channel, int Number);
        virtual void SetEvents(const cEvent *Present, const cEvent *Following);
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush();

        void PreLoadImages();

 private:
        const cEvent *m_Present {nullptr};

        int m_ChannelWidth {0}, m_ChannelHeight {0};

        const cChannel *m_CurChannel {nullptr};

        cPixmap *ChanInfoTopPixmap {nullptr};
        cPixmap *ChanInfoBottomPixmap {nullptr};
        cPixmap *ChanLogoPixmap {nullptr};
        cPixmap *ChanLogoBGPixmap {nullptr};
        cPixmap *ChanIconsPixmap {nullptr};
        cPixmap *ChanEpgImagesPixmap {nullptr};

        int m_ScreenWidth {-1}, m_LastScreenWidth {-1};
        int m_ScreenHeight {0};
        double m_ScreenAspect {0.0};
        int m_HeightBottom {0};  //, m_HeightImageLogo {0};

        int m_LastSignalStrength {-1}, m_LastSignalQuality {-1};
        int m_SignalStrengthRight {0};

        // TVScraper
        cRect m_TVSRect {0, 0, 0, 0};

        // TextScroller
        cTextScrollers Scrollers;

        bool m_IsRadioChannel {false};
        // bool m_IsGroup {false};

        void SignalQualityDraw();
        void ChannelIconsDraw(const cChannel *Channel, bool Resolution);
        void DvbapiInfoDraw();
};
