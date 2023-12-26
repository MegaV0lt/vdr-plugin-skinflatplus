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
 private:
        // bool m_DoOutput;
        const cEvent *m_Present;

        int m_ChannelWidth, m_ChannelHeight;

        // cString m_ChannelName;
        const cChannel *m_CurChannel;

        cPixmap *ChanInfoTopPixmap;
        cPixmap *ChanInfoBottomPixmap;
        cPixmap *ChanLogoPixmap;
        cPixmap *ChanLogoBGPixmap;
        cPixmap *ChanIconsPixmap;
        cPixmap *ChanEpgImagesPixmap;

        int m_ScreenWidth, m_LastScreenWidth;
        int m_ScreenHeight;
        double m_ScreenAspect;
        int HeightBottom, HeightImageLogo;

        int m_LastSignalStrength, m_LastSignalQuality;
        int m_SignalStrengthRight;

        // TVScraper
        int TVSLeft, TVSTop, TVSWidth, TVSHeight;

        // TextScroller
        cTextScrollers Scrollers;

        // bool IsRecording;  / Unused?
        bool m_IsRadioChannel;
        bool m_IsGroup;

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
