#pragma once

#include <vdr/status.h>
#include "./baserender.h"
#include "./flat.h"
#include "services/scraper2vdr.h"
#include "services/dvbapi.h"

class cFlatDisplayChannel : public cFlatBaseRender, public cSkinDisplayChannel, public cStatus {
 private:
        bool g_DoOutput;
        const cEvent *present;

        int g_ChannelWidth, g_ChannelHeight;

        cString g_ChannelName;
        const cChannel *CurChannel;

        cPixmap *ChanInfoTopPixmap;
        cPixmap *ChanInfoBottomPixmap;
        cPixmap *ChanLogoPixmap;
        cPixmap *ChanLogoBGPixmap;
        cPixmap *ChanIconsPixmap;
        cPixmap *ChanEpgImagesPixmap;

        int ScreenWidth, LastScreenWidth;
        int ScreenHeight;
        double ScreenAspect;
        int HeightBottom, HeightImageLogo;

        int LastSignalStrength, LastSignalQuality;
        int SignalStrengthRight;

        // TVScraper
        int TVSLeft, TVSTop, TVSWidth, TVSHeight;

        // TextScroller
        cTextScrollers Scrollers;

        bool IsRecording;
        bool IsRadioChannel;
        bool IsGroup;

        void SignalQualityDraw(void);
        void ChannelIconsDraw(const cChannel *Channel, bool Resolution);
        void DvbapiInfoDraw(void);

 public:
        cFlatDisplayChannel(bool WithInfo);
        virtual ~cFlatDisplayChannel();
        virtual void SetChannel(const cChannel *Channel, int Number);
        virtual void SetEvents(const cEvent *Present, const cEvent *Following);
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush(void);

        void PreLoadImages(void);
};
