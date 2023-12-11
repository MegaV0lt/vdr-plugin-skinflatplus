#pragma once

#include "baserender.h"

class cFlatDisplayVolume : public cFlatBaseRender, public cSkinDisplayVolume {
 private:
        // bool Muted;  // Unused?

        cPixmap *LabelPixmap;
        cPixmap *MuteLogoPixmap;

        int g_LabelHeight;
 public:
        cFlatDisplayVolume(void);
        virtual ~cFlatDisplayVolume();
        virtual void SetVolume(int Current, int Total, bool Mute);
        // virtual void SetAudioChannel(int AudioChannel);
        virtual void Flush(void);

        void PreLoadImages(void);
};
