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
    osd->Flush();
}
