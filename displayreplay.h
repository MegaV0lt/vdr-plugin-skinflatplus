#pragma once

#include "baserender.h"
#include "services/scraper2vdr.h"

class cFlatDisplayReplay : public cFlatBaseRender, public cSkinDisplayReplay, public cThread {
 private:
        cString current, total;

        int labelHeight;
        cPixmap *labelPixmap;
        cPixmap *labelJump;
        cPixmap *iconsPixmap;
        cPixmap *chanEpgImagesPixmap;
        cPixmap *dimmPixmap;

        cFont *fontSecs;
        const cRecording *g_Recording;

        int screenWidth, lastScreenWidth;
        int screenHeight;
        double screenAspect;

        // TVScraper
        int TVSLeft, TVSTop, TVSWidth, TVSHeight;

        // Dimm on pause
        bool g_DimmActive;
        time_t g_DimmStartTime;

        bool ProgressShown;
        bool g_ModeOnly;
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
