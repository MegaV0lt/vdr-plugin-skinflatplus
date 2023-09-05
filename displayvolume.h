#pragma once

#include "baserender.h"

class cFlatDisplayVolume : public cFlatBaseRender, public cSkinDisplayVolume {
 private:
        bool muted;

        cPixmap *labelPixmap;
        cPixmap *muteLogoPixmap;

        int labelHeight;
 public:
        cFlatDisplayVolume(void);
        virtual ~cFlatDisplayVolume();
        virtual void SetVolume(int Current, int Total, bool Mute);
        // virtual void SetAudioChannel(int AudioChannel);
        virtual void Flush(void);

        void PreLoadImages(void);
};
