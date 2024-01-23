/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displaymessage.h"

cFlatDisplayMessage::cFlatDisplayMessage(void) {
    CreateFullOsd();
    TopBarCreate();
    MessageCreate();
}

cFlatDisplayMessage::~cFlatDisplayMessage() {
}

void cFlatDisplayMessage::SetMessage(eMessageType Type, const char *Text) {
    if (Text)
        MessageSet(Type, Text);
    else
        MessageClear();
}

void cFlatDisplayMessage::Flush(void) {
    TopBarUpdate();
    m_Osd->Flush();
}
