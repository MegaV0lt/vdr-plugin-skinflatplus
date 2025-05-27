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
 public:
        cFlatDisplayVolume();
        ~cFlatDisplayVolume() override;
        void SetVolume(int Current, int Total, bool Mute) override;
        // void SetAudioChannel(int AudioChannel) override;
        void Flush() override;

        void PreLoadImages();

 private:
        cPixmap *LabelPixmap {nullptr};
        cPixmap *MuteLogoPixmap {nullptr};

        int m_LabelHeight {0};
};
