/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include "./baserender.h"

class cFlatDisplayTracks : public cFlatBaseRender, public cSkinDisplayTracks {
 public:
        cFlatDisplayTracks(const char *Title, int NumTracks, const char * const *Tracks);
        ~cFlatDisplayTracks() override;
        void SetTrack(int Index, const char * const *Tracks) override;
        void SetAudioChannel(int AudioChannel) override;
        void Flush() override;

 private:
        cPixmap *TracksPixmap {nullptr};
        cPixmap *TracksLogoPixmap {nullptr};

        cImage *img_ac3 {nullptr};
        cImage *img_stereo {nullptr};
        int m_Ac3Width {0}, m_StereoWidth {0};

        int m_ItemHeight {0}, ItemsHeight {0};
        int m_MaxItemWidth {0};
        int m_CurrentIndex {0};

        void SetItem(const char *Text, int Index, bool Current);
};
