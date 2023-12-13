#include "./displaytracks.h"
#include "./flat.h"

cFlatDisplayTracks::cFlatDisplayTracks(const char *Title, int NumTracks, const char * const *Tracks) {
    CreateFullOsd();
    TopBarCreate();

    img_ac3 = ImgLoader.LoadIcon("tracks_ac3", 999, m_FontHeight);
    img_stereo = ImgLoader.LoadIcon("tracks_stereo", 999, m_FontHeight);

    Ac3Width = StereoWidth = 0;
    if (img_ac3)
        Ac3Width = img_ac3->Width();
    if (img_stereo)
        StereoWidth = img_stereo->Width();

    int imgWidthMax = std::max(Ac3Width, StereoWidth);
    ItemHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderTrackSize * 2;
    CurrentIndex = -1;
    MaxItemWidth = m_Font->Width(Title) + m_MarginItem * 4;
    for (int i {0}; i < NumTracks; ++i)
        MaxItemWidth = std::max(MaxItemWidth, m_Font->Width(Tracks[i]) + m_MarginItem * 2);

    int headerWidth = m_Font->Width(Title) + m_Font->Width(' ') + imgWidthMax;
    MaxItemWidth = std::max(MaxItemWidth, headerWidth);

    ItemsHeight = (NumTracks + 1) * ItemHeight;
    int left = m_OsdWidth - MaxItemWidth;
    left /= 2;
    TopBarSetTitle(Title);

    TracksPixmap = CreatePixmap(osd, "TracksPixmap", 1,
                                cRect(left, m_OsdHeight - ItemsHeight - m_MarginItem, MaxItemWidth, ItemsHeight));
    PixmapFill(TracksPixmap, clrTransparent);

    TracksLogoPixmap = CreatePixmap(osd, "TracksLogoPixmap", 1,
                                    cRect(left, m_OsdHeight - ItemsHeight - m_MarginItem, MaxItemWidth, ItemsHeight));
    PixmapFill(TracksLogoPixmap, clrTransparent);

    SetItem(Title, -1, false);

    for (int i {0}; i < NumTracks; ++i)
        SetItem(Tracks[i], i, false);
}

cFlatDisplayTracks::~cFlatDisplayTracks() {
    osd->DestroyPixmap(TracksPixmap);
    osd->DestroyPixmap(TracksLogoPixmap);
}

void cFlatDisplayTracks::SetItem(const char *Text, int Index, bool Current) {
    int y = (Index + 1) * ItemHeight;
    tColor ColorFg = Theme.Color(clrTrackItemFont);
    tColor ColorBg = Theme.Color(clrTrackItemBg);
    if (Current) {
        ColorFg = Theme.Color(clrTrackItemCurrentFont);
        ColorBg = Theme.Color(clrTrackItemCurrentBg);
        CurrentIndex = Index;
    } else if (Index >= 0) {
        ColorFg = Theme.Color(clrTrackItemSelableFont);
        ColorBg = Theme.Color(clrTrackItemSelableBg);
    }

    if (Index == -1)
        TracksPixmap->DrawText(cPoint(0, y), Text, ColorFg, ColorBg, m_Font, MaxItemWidth,
                               ItemHeight - Config.MenuItemPadding - Config.decorBorderTrackSize * 2, taLeft);
    else
        TracksPixmap->DrawText(cPoint(0, y), Text, ColorFg, ColorBg, m_Font, MaxItemWidth,
                               ItemHeight - Config.MenuItemPadding - Config.decorBorderTrackSize * 2, taCenter);

    int left = m_OsdWidth - MaxItemWidth;
    left /= 2;

    int top = m_OsdHeight - ItemsHeight - m_MarginItem + y;

    if (Current)
        DecorBorderDraw(left, top, MaxItemWidth, m_FontHeight, Config.decorBorderTrackSize, Config.decorBorderTrackType,
                        Config.decorBorderTrackCurFg, Config.decorBorderTrackCurBg);
    else if (Index >= 0)
        DecorBorderDraw(left, top, MaxItemWidth, m_FontHeight, Config.decorBorderTrackSize, Config.decorBorderTrackType,
                        Config.decorBorderTrackSelFg, Config.decorBorderTrackSelBg);
    else
        DecorBorderDraw(left, top, MaxItemWidth, m_FontHeight, Config.decorBorderTrackSize, Config.decorBorderTrackType,
                        Config.decorBorderTrackFg, Config.decorBorderTrackBg);
}

void cFlatDisplayTracks::SetTrack(int Index, const char * const *Tracks) {
    if (CurrentIndex >= 0)
        SetItem(Tracks[CurrentIndex], CurrentIndex, false);

    SetItem(Tracks[Index], Index, true);
}

void cFlatDisplayTracks::SetAudioChannel(int AudioChannel) {
    // From vdr: 0=stereo, 1=left, 2=right, -1=don't display the audio channel indicator.
    // From skinnopacity: -1 ac3, else stereo
    PixmapFill(TracksLogoPixmap, clrTransparent);
    if (AudioChannel == -1 && img_ac3) {
        int IconLeft = MaxItemWidth - img_ac3->Width() - m_MarginItem;
        int IconTop = (m_FontHeight - img_ac3->Height()) / 2;
        TracksLogoPixmap->DrawImage(cPoint(IconLeft, IconTop), *img_ac3);
    } else if (img_stereo) {
        int IconLeft = MaxItemWidth - img_stereo->Width() - m_MarginItem;
        int IconTop = (m_FontHeight - img_stereo->Height()) / 2;
        TracksLogoPixmap->DrawImage(cPoint(IconLeft, IconTop), *img_stereo);
    }
    return;
}

void cFlatDisplayTracks::Flush(void) {
    TopBarUpdate();
    osd->Flush();
}
