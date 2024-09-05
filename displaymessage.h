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
        virtual ~cFlatDisplayMessage();
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush();

 private:
};
