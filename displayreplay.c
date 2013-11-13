#include "displayreplay.h"

cFlatDisplayReplay::cFlatDisplayReplay(bool ModeOnly) {
    labelHeight = fontHeight + fontSmlHeight;
    current = "";
    total = "";

    ProgressShown = false;
    CreateFullOsd();
    TopBarCreate();
    MessageCreate();

    labelPixmap = osd->CreatePixmap(1, cRect(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorBorderReplaySize,
        osdWidth - Config.decorBorderReplaySize*2, labelHeight));
    iconsPixmap = osd->CreatePixmap(2, cRect(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorBorderReplaySize,
        osdWidth - Config.decorBorderReplaySize*2, labelHeight));
    
    ProgressBarCreate(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize - marginItem,
        osdWidth - Config.decorBorderReplaySize*2, Config.decorProgressReplaySize, marginItem, 0,
        Config.decorProgressReplayFg, Config.decorProgressReplayBarFg, Config.decorProgressReplayBg, Config.decorProgressReplayType);

    labelJump = osd->CreatePixmap(1, cRect(Config.decorBorderReplaySize, 
        osdHeight - labelHeight - Config.decorProgressReplaySize - marginItem*3 - fontHeight - Config.decorBorderReplaySize*2,
        osdWidth - Config.decorBorderReplaySize*2, fontHeight));
    
    labelPixmap->Fill(Theme.Color(clrReplayBg));
    labelJump->Fill(clrTransparent);

}

cFlatDisplayReplay::~cFlatDisplayReplay() {
    osd->DestroyPixmap(labelPixmap);
    osd->DestroyPixmap(labelJump);
    osd->DestroyPixmap(iconsPixmap);
}

void cFlatDisplayReplay::SetRecording(const cRecording *Recording) {
    const cRecordingInfo *recInfo = Recording->Info();
    SetTitle( recInfo->Title() );
    cString info = "";
    if (recInfo->ShortText())
        info = cString::sprintf("%s - %s %s", recInfo->ShortText(), *ShortDateString(Recording->Start()), *TimeString(Recording->Start()));
    else
        info = cString::sprintf("%s %s", *ShortDateString(Recording->Start()), *TimeString(Recording->Start()));

    labelPixmap->DrawText(cPoint(marginItem, fontHeight), info, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), fontSml, osdWidth - Config.decorBorderReplaySize*2);
}

void cFlatDisplayReplay::SetTitle(const char *Title) {
    TopBarSetTitle(Title);
}

void cFlatDisplayReplay::SetMode(bool Play, bool Forward, int Speed) {
    if( Setup.ShowReplayMode ) {
        int left = osdWidth - Config.decorBorderReplaySize*2 - (fontHeight * 4 + marginItem * 3);
        left /= 2;

        iconsPixmap->Fill(clrTransparent);

        labelPixmap->DrawRectangle(cRect( left - font->Width("33") - marginItem, 0, fontHeight*4 + marginItem*6 + font->Width("33")*2, fontHeight), Theme.Color(clrReplayBg) );

        cString rewind, pause, play, forward;
        cString speed;
        
        if( Speed == -1 ) {
            if( Play ) {
                rewind = "rewind";
                pause = "pause";
                play = "play_sel";
                forward = "forward";
            } else {
                rewind = "rewind";
                pause = "pause_sel";
                play = "play";
                forward = "forward";
            }                
        } else {
            speed = cString::sprintf("%d", Speed);
            if( Forward ) {
                rewind = "rewind";
                pause = "pause";
                play = "play";
                forward = "forward_sel";
                labelPixmap->DrawText(cPoint(left + fontHeight*4 + marginItem*4, 0), speed, Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), font);
            } else {
                rewind = "rewind_sel";
                pause = "pause";
                play = "play";
                forward = "forward";
                labelPixmap->DrawText(cPoint(left - font->Width(speed) - marginItem, 0), speed, Theme.Color(clrReplayFontSpeed), Theme.Color(clrReplayBg), font);
            }                
        }

        if( imgLoader.LoadIcon(*rewind, fontHeight, fontHeight) )
            iconsPixmap->DrawImage( cPoint(left, 0), imgLoader.GetImage() );
        if( imgLoader.LoadIcon(*pause, fontHeight, fontHeight) )
            iconsPixmap->DrawImage( cPoint(left + fontHeight + marginItem, 0), imgLoader.GetImage() );
        if( imgLoader.LoadIcon(*play, fontHeight, fontHeight) )
            iconsPixmap->DrawImage( cPoint(left + fontHeight*2 + marginItem*2, 0), imgLoader.GetImage() );
        if( imgLoader.LoadIcon(*forward, fontHeight, fontHeight) )
            iconsPixmap->DrawImage( cPoint(left + fontHeight*3 + marginItem*3, 0), imgLoader.GetImage() );

    }
    
    if( ProgressShown )
        DecorBorderDraw(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorProgressReplaySize - Config.decorBorderReplaySize - marginItem,
            osdWidth - Config.decorBorderReplaySize*2, labelHeight + Config.decorProgressReplaySize + marginItem,
            Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
    else
        DecorBorderDraw(Config.decorBorderReplaySize, osdHeight - labelHeight - Config.decorBorderReplaySize,
            osdWidth - Config.decorBorderReplaySize*2, labelHeight,
            Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg);
        
        
}

void cFlatDisplayReplay::SetProgress(int Current, int Total) {
    ProgressShown = true;
    ProgressBarDrawMarks(Current, Total, marks, Theme.Color(clrReplayMarkFg), Theme.Color(clrReplayMarkCurrentFg));
}

void cFlatDisplayReplay::SetCurrent(const char *Current) {
    current = Current;
    UpdateInfo();
}

void cFlatDisplayReplay::SetTotal(const char *Total) {
    total = Total;
    UpdateInfo();
}

void cFlatDisplayReplay::UpdateInfo(void) {
    int right = osdWidth - Config.decorBorderReplaySize*2 - font->Width(total);
    labelPixmap->DrawText(cPoint(marginItem, 0), current, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(current), fontHeight);
    labelPixmap->DrawText(cPoint(right - marginItem, 0), total, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(total), fontHeight);
}

void cFlatDisplayReplay::SetJump(const char *Jump) {
    DecorBorderClearByFrom(BorderRecordJump);
    
    if( !Jump )
    {
        labelJump->Fill(clrTransparent);
        return;
    }        
    int left = osdWidth - Config.decorBorderReplaySize*2 - font->Width(Jump);
    left /= 2;
    
    labelJump->DrawText(cPoint(left, 0), Jump, Theme.Color(clrReplayFont), Theme.Color(clrReplayBg), font, font->Width(Jump), fontHeight, taCenter);

    DecorBorderDraw(left + Config.decorBorderReplaySize,
    osdHeight - labelHeight - Config.decorProgressReplaySize - marginItem*3 - fontHeight - Config.decorBorderReplaySize*2,
        font->Width(Jump), fontHeight,
        Config.decorBorderReplaySize, Config.decorBorderReplayType, Config.decorBorderReplayFg, Config.decorBorderReplayBg, BorderRecordJump);
}

void cFlatDisplayReplay::SetMessage(eMessageType Type, const char *Text) {
    if (Text)
        MessageSet(Type, Text);
    else
        MessageClear();
}

void cFlatDisplayReplay::Flush(void) {
    TopBarUpdate();
    osd->Flush();
}
