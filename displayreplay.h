/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include "./baserender.h"
#include "./services/scraper2vdr.h"

class cFlatDisplayReplay : public cFlatBaseRender, public cSkinDisplayReplay, public cThread {
 public:
        explicit cFlatDisplayReplay(bool ModeOnly);
        virtual ~cFlatDisplayReplay();
        virtual void SetRecording(const cRecording *Recording);
        virtual void SetTitle(const char *Title);
        virtual void SetMode(bool Play, bool Forward, int Speed);
        virtual void SetProgress(int Current, int Total);
        virtual void SetCurrent(const char *Current);
        virtual void SetTotal(const char *Total);
        virtual void SetJump(const char *Jump);
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush();

        void PreLoadImages();

 private:
        cString m_Current {""}, m_Total {""};
        int m_LastCurrentWidth {0};

        int m_LabelHeight {0};
        cPixmap *LabelPixmap {nullptr};
        cPixmap *LabelJumpPixmap {nullptr};
        cPixmap *IconsPixmap {nullptr};
        cPixmap *ChanEpgImagesPixmap {nullptr};
        cPixmap *DimmPixmap {nullptr};

        cFont *m_FontSecs {nullptr};
        const cRecording *m_Recording {nullptr};

        int m_ScreenWidth {-1}, m_LastScreenWidth {-1};
        int m_ScreenHeight {0};
        double m_ScreenAspect {0.0};

        time_t m_LastEndTimeUpdate {0};
        time_t m_LastPosterBannerUpdate {0};
        cString m_LastCurrent {""};

        // TVScraper
        cRect m_TVSRect {0, 0, 0, 0};

        // Dimm on pause
        bool m_DimmActive {false};
        time_t m_DimmStartTime {0};

        int m_CurrentFrame {0};

        bool m_ProgressShown {false};
        bool m_ModeOnly {false};
        void UpdateInfo();
        void ResolutionAspectDraw();

        virtual void Action();
};
