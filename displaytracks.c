#include "displaytracks.h"
#include "flat.h"

cFlatDisplayTracks::cFlatDisplayTracks(const char *Title, int NumTracks, const char * const *Tracks) {
    CreateFullOsd();
    TopBarCreate();

    img_ac3 = imgLoader.LoadIcon("tracks_ac3", 999, fontHeight);
    img_stereo = imgLoader.LoadIcon("tracks_stereo", 999, fontHeight);

    ac3Width = stereoWidth = 0;
    if (img_ac3)
        ac3Width = img_ac3->Width();
    if (img_stereo)
        stereoWidth = img_stereo->Width();

    int imgWidthMax = std::max(ac3Width, stereoWidth);
    itemHeight = fontHeight + Config.MenuItemPadding + Config.decorBorderTrackSize * 2;
    currentIndex = -1;
    maxItemWidth = font->Width(Title) + marginItem * 4;
    for (int i = 0; i < NumTracks; i++)
        maxItemWidth = std::max(maxItemWidth, font->Width(Tracks[i]) + marginItem * 2);

    int headerWidth = font->Width(Title) + font->Width(" ") + imgWidthMax;
    maxItemWidth = std::max(maxItemWidth, headerWidth);

    itemsHeight = (NumTracks + 1) * itemHeight;
    int left = osdWidth - maxItemWidth;
    left /= 2;
    TopBarSetTitle(Title);

    tracksPixmap = CreatePixmap(osd, 1, cRect(left, osdHeight - itemsHeight - marginItem, maxItemWidth, itemsHeight));
    tracksPixmap->Fill(clrTransparent);

    tracksLogoPixmap = CreatePixmap(osd, 1, cRect(left, osdHeight - itemsHeight - marginItem, maxItemWidth, itemsHeight));
    tracksLogoPixmap->Fill(clrTransparent);

    SetItem(Title, -1, false);

    for (int i = 0; i < NumTracks; i++)
        SetItem(Tracks[i], i, false);
}

cFlatDisplayTracks::~cFlatDisplayTracks() {
    osd->DestroyPixmap(tracksPixmap);
    osd->DestroyPixmap(tracksLogoPixmap);
}

void cFlatDisplayTracks::SetItem(const char *Text, int Index, bool Current) {
    int y = (Index+1) * itemHeight;
    tColor ColorFg, ColorBg;
    if (Current) {
        ColorFg = Theme.Color(clrTrackItemCurrentFont);
        ColorBg = Theme.Color(clrTrackItemCurrentBg);
        currentIndex = Index;
    }
    else {
        if (Index >= 0) {
            ColorFg = Theme.Color(clrTrackItemSelableFont);
            ColorBg = Theme.Color(clrTrackItemSelableBg);
        } else {
            ColorFg = Theme.Color(clrTrackItemFont);
            ColorBg = Theme.Color(clrTrackItemBg);
        }
    }

    if (Index == -1)
        tracksPixmap->DrawText(cPoint(0, y), Text, ColorFg, ColorBg, font, maxItemWidth, itemHeight - Config.MenuItemPadding - Config.decorBorderTrackSize * 2, taLeft);
    else
        tracksPixmap->DrawText(cPoint(0, y), Text, ColorFg, ColorBg, font, maxItemWidth, itemHeight - Config.MenuItemPadding - Config.decorBorderTrackSize * 2, taCenter);

    int left = osdWidth - maxItemWidth;
    left /= 2;

    int top = osdHeight - itemsHeight - marginItem + y;

    if (Current)
        DecorBorderDraw(left, top, maxItemWidth, fontHeight,
            Config.decorBorderTrackSize, Config.decorBorderTrackType, Config.decorBorderTrackCurFg, Config.decorBorderTrackCurBg);
    else if (Index >= 0)
        DecorBorderDraw(left, top, maxItemWidth, fontHeight,
            Config.decorBorderTrackSize, Config.decorBorderTrackType, Config.decorBorderTrackSelFg, Config.decorBorderTrackSelBg);
    else
        DecorBorderDraw(left, top, maxItemWidth, fontHeight,
            Config.decorBorderTrackSize, Config.decorBorderTrackType, Config.decorBorderTrackFg, Config.decorBorderTrackBg);
}

void cFlatDisplayTracks::SetTrack(int Index, const char * const *Tracks) {
    if (currentIndex >= 0)
        SetItem(Tracks[currentIndex], currentIndex, false);

    SetItem(Tracks[Index], Index, true);
}

void cFlatDisplayTracks::SetAudioChannel(int AudioChannel) {
    // from vdr 0=stereo, 1=left, 2=right, -1=don't display the audio channel indicator.
    // from skinnopacity -1 ac3, else stero
    tracksLogoPixmap->Fill(clrTransparent);
    if (AudioChannel == -1 && img_ac3) {
        int IconLeft = maxItemWidth - img_ac3->Width() - marginItem;
        int IconTop = (fontHeight - img_ac3->Height()) / 2;
        tracksLogoPixmap->DrawImage(cPoint(IconLeft, IconTop), *img_ac3);
    } else if (img_stereo) {
        int IconLeft = maxItemWidth - img_stereo->Width() - marginItem;
        int IconTop = (fontHeight - img_stereo->Height()) / 2;
        tracksLogoPixmap->DrawImage(cPoint(IconLeft, IconTop), *img_stereo);
    }
    return;
}

void cFlatDisplayTracks::Flush(void) {
    TopBarUpdate();
    osd->Flush();
}
