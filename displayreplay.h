#pragma once

#include "./baserender.h"
#include "./services/scraper2vdr.h"

class cFlatDisplayReplay : public cFlatBaseRender, public cSkinDisplayReplay, public cThread {
 private:
        cString current, total;

        int m_LabelHeight;
        cPixmap *LabelPixmap;
        cPixmap *labelJump;
        cPixmap *iconsPixmap;
        cPixmap *ChanEpgImagesPixmap;
        cPixmap *DimmPixmap;

        cFont *m_FontSecs;
        const cRecording *m_Recording;

        int m_ScreenWidth, m_LastScreenWidth;
        int m_ScreenHeight;
        double m_ScreenAspect;

        // TVScraper
        int TVSLeft, TVSTop, TVSWidth, TVSHeight;

        // Dimm on pause
        bool m_DimmActive;
        time_t m_DimmStartTime;

        bool ProgressShown;
        bool m_ModeOnly;
        void UpdateInfo(void);
        void ResolutionAspectDraw(void);

        virtual void Action(void);

 public:
        cFlatDisplayReplay(bool ModeOnly);
        virtual ~cFlatDisplayReplay();
        virtual void SetRecording(const cRecording *Recording);
        virtual void SetTitle(const char *Title);
        virtual void SetMode(bool Play, bool Forward, int Speed);
        virtual void SetProgress(int Current, int Total);
        virtual void SetCurrent(const char *Current);
        virtual void SetTotal(const char *Total);
        virtual void SetJump(const char *Jump);
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush(void);

        void PreLoadImages(void);
};
