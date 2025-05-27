/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include "./baserender.h"

class cFlatDisplayMessage : public cFlatBaseRender, public cSkinDisplayMessage {
 public:
        cFlatDisplayMessage();
        ~cFlatDisplayMessage() override;
        void SetMessage(eMessageType Type, const char *Text) override;
        void Flush() override;

 private:
};
