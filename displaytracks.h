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
 private:
        cPixmap *TracksPixmap;
        cPixmap *TracksLogoPixmap;

        cImage *img_ac3;
        cImage *img_stereo;
        int m_Ac3Width, m_StereoWidth;

        int m_ItemHeight, ItemsHeight;
        int m_MaxItemWidth;
        int m_CurrentIndex;

        void SetItem(const char *Text, int Index, bool Current);
 public:
        cFlatDisplayTracks(const char *Title, int NumTracks, const char * const *Tracks);
        virtual ~cFlatDisplayTracks();
        virtual void SetTrack(int Index, const char * const *Tracks);
        virtual void SetAudioChannel(int AudioChannel);
        virtual void Flush(void);
};
