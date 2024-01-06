/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include "./baserender.h"

class cFlatDisplayVolume : public cFlatBaseRender, public cSkinDisplayVolume {
 private:
        // bool Muted;  // Unused?

        cPixmap *LabelPixmap {nullptr};
        cPixmap *MuteLogoPixmap {nullptr};

        int m_LabelHeight{0};
 public:
        cFlatDisplayVolume(void);
        virtual ~cFlatDisplayVolume();
        virtual void SetVolume(int Current, int Total, bool Mute);
        // virtual void SetAudioChannel(int AudioChannel);
        virtual void Flush(void);

        void PreLoadImages(void);
};
