/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displaymessage.h"

cFlatDisplayMessage::cFlatDisplayMessage() {
    CreateFullOsd();
    TopBarCreate();
    MessageCreate();

    m_OSDMessageTime = Setup.OSDMessageTime;  // Save value to be restored later
}

cFlatDisplayMessage::~cFlatDisplayMessage() {
    if (Setup.OSDMessageTime != m_OSDMessageTime) {
#ifdef DEBUGFUNCSCALL
        dsyslog("flatPlus: ~cFlatDisplayMessage() Restoring 'OSDMessageTime' to %d", m_OSDMessageTime);
#endif
        Setup.OSDMessageTime = m_OSDMessageTime;  // Restore original 'OSDMessageTime'
    }
}

void cFlatDisplayMessage::SetMessage(eMessageType Type, const char *Text) {
#ifdef DEBUGFUNCSCALL
    dsyslog("flatPlus: cFlatDisplayMessage::SetMessage()");
    dsyslog("   Setup.OSDMessageTime: %d, m_OSDMessageTime: %d", Setup.OSDMessageTime, m_OSDMessageTime);
#endif

    if (Text) {
        if (Config.ScrollerEnable)
            MessageSetExtraTime(Text);  // For long messages increase 'MessageTime'

        MessageSet(Type, Text);
    } else {
        MessageClear();
    }
}

void cFlatDisplayMessage::Flush() {
    TopBarUpdate();
    m_Osd->Flush();
}
