#pragma once

#include "baserender.h"

class cFlatDisplayMessage : public cFlatBaseRender, public cSkinDisplayMessage {
 private:
 public:
        cFlatDisplayMessage(void);
        virtual ~cFlatDisplayMessage();
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush(void);
};
