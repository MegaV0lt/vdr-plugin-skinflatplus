#pragma once

#include <vdr/status.h>
#include "baserender.h"
#include "flat.h"
#include "services/scraper2vdr.h"
#include "services/dvbapi.h"

class cFlatDisplayChannel : public cFlatBaseRender, public cSkinDisplayChannel, public cStatus {
    private:
        bool doOutput;
        const cEvent *present;

        int channelWidth, channelHeight;

        cString channelName;
        const cChannel *CurChannel;

        cPixmap *chanInfoTopPixmap;
        cPixmap *chanInfoBottomPixmap;
        cPixmap *chanLogoPixmap;
        cPixmap *chanLogoBGPixmap;
        cPixmap *chanIconsPixmap;
        cPixmap *chanEpgImagesPixmap;

        int screenWidth, lastScreenWidth;
        int screenHeight;
        double screenAspect;
        int heightBottom, heightImageLogo;

        int LastSignalStrength, LastSignalQuality;
        int SignalStrengthRight;

        // TVScraper
        int TVSLeft, TVSTop, TVSWidth, TVSHeight;

        // TextScroller
        cTextScrollers scrollers;

        bool isRecording;
        bool isRadioChannel;
        bool isGroup;

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
