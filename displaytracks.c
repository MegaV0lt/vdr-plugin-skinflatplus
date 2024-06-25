/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./displaytracks.h"
#include "./flat.h"

cFlatDisplayTracks::cFlatDisplayTracks(const char *Title, int NumTracks, const char * const *Tracks) {
    CreateFullOsd();
    TopBarCreate();

    img_ac3 = ImgLoader.LoadIcon("tracks_ac3", 999, m_FontHeight);
    img_stereo = ImgLoader.LoadIcon("tracks_stereo", 999, m_FontHeight);

    if (img_ac3)
        m_Ac3Width = img_ac3->Width();
    if (img_stereo)
        m_StereoWidth = img_stereo->Width();

    const int imgWidthMax = std::max(m_Ac3Width, m_StereoWidth);
    m_ItemHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderTrackSize * 2;
    m_CurrentIndex = -1;
    m_MaxItemWidth = m_Font->Width(Title) + m_MarginItem * 4;
    for (int i {0}; i < NumTracks; ++i)
        m_MaxItemWidth = std::max(m_MaxItemWidth, m_Font->Width(Tracks[i]) + m_MarginItem2);

    const int headerWidth = m_Font->Width(Title) + m_Font->Width(' ') + imgWidthMax;
    m_MaxItemWidth = std::max(m_MaxItemWidth, headerWidth);

    ItemsHeight = (NumTracks + 1) * m_ItemHeight;
    const int left = (m_OsdWidth - m_MaxItemWidth) / 2;  // left /= 2;
    TopBarSetTitle(Title);

    TracksPixmap = CreatePixmap(m_Osd, "TracksPixmap", 1,
                                cRect(left, m_OsdHeight - ItemsHeight - m_MarginItem, m_MaxItemWidth, ItemsHeight));
    PixmapFill(TracksPixmap, clrTransparent);

    TracksLogoPixmap = CreatePixmap(m_Osd, "TracksLogoPixmap", 1,
                                    cRect(left, m_OsdHeight - ItemsHeight - m_MarginItem, m_MaxItemWidth, ItemsHeight));
    PixmapFill(TracksLogoPixmap, clrTransparent);

    SetItem(Title, -1, false);

    for (int i {0}; i < NumTracks; ++i)
        SetItem(Tracks[i], i, false);
}

cFlatDisplayTracks::~cFlatDisplayTracks() {
    m_Osd->DestroyPixmap(TracksPixmap);
    m_Osd->DestroyPixmap(TracksLogoPixmap);
}

void cFlatDisplayTracks::SetItem(const char *Text, int Index, bool Current) {
    if (!TracksPixmap) return;

    const int y = (Index + 1) * m_ItemHeight;
    tColor ColorFg = Theme.Color(clrTrackItemFont);
    tColor ColorBg = Theme.Color(clrTrackItemBg);
    if (Current) {
        ColorFg = Theme.Color(clrTrackItemCurrentFont);
        ColorBg = Theme.Color(clrTrackItemCurrentBg);
        m_CurrentIndex = Index;
    } else if (Index >= 0) {
        ColorFg = Theme.Color(clrTrackItemSelableFont);
        ColorBg = Theme.Color(clrTrackItemSelableBg);
    }

    if (Index == -1)
        TracksPixmap->DrawText(cPoint(0, y), Text, ColorFg, ColorBg, m_Font, m_MaxItemWidth,
                               m_ItemHeight - Config.MenuItemPadding - Config.decorBorderTrackSize * 2, taLeft);
    else
        TracksPixmap->DrawText(cPoint(0, y), Text, ColorFg, ColorBg, m_Font, m_MaxItemWidth,
                               m_ItemHeight - Config.MenuItemPadding - Config.decorBorderTrackSize * 2, taCenter);

    const int left = (m_OsdWidth - m_MaxItemWidth) / 2;
    //  left /= 2;

    const int top = m_OsdHeight - ItemsHeight - m_MarginItem + y;

    if (Current)
        DecorBorderDraw(left, top, m_MaxItemWidth, m_FontHeight, Config.decorBorderTrackSize, Config.decorBorderTrackType,
                        Config.decorBorderTrackCurFg, Config.decorBorderTrackCurBg);
    else if (Index >= 0)
        DecorBorderDraw(left, top, m_MaxItemWidth, m_FontHeight, Config.decorBorderTrackSize, Config.decorBorderTrackType,
                        Config.decorBorderTrackSelFg, Config.decorBorderTrackSelBg);
    else
        DecorBorderDraw(left, top, m_MaxItemWidth, m_FontHeight, Config.decorBorderTrackSize, Config.decorBorderTrackType,
                        Config.decorBorderTrackFg, Config.decorBorderTrackBg);
}

void cFlatDisplayTracks::SetTrack(int Index, const char * const *Tracks) {
    if (m_CurrentIndex >= 0)
        SetItem(Tracks[m_CurrentIndex], m_CurrentIndex, false);

    SetItem(Tracks[Index], Index, true);
}

void cFlatDisplayTracks::SetAudioChannel(int AudioChannel) {
    if (!TracksLogoPixmap) return;

    PixmapFill(TracksLogoPixmap, clrTransparent);
    // From vdr: 0=stereo, 1=left, 2=right, -1=don't display the audio channel indicator.
    // From skinnopacity: -1 ac3, else stereo
    if (AudioChannel == -1 && img_ac3) {
        const int IconLeft = m_MaxItemWidth - img_ac3->Width() - m_MarginItem;
        const int IconTop = (m_FontHeight - img_ac3->Height()) / 2;
        TracksLogoPixmap->DrawImage(cPoint(IconLeft, IconTop), *img_ac3);
    } else if (img_stereo) {
        const int IconLeft = m_MaxItemWidth - img_stereo->Width() - m_MarginItem;
        const int IconTop = (m_FontHeight - img_stereo->Height()) / 2;
        TracksLogoPixmap->DrawImage(cPoint(IconLeft, IconTop), *img_stereo);
    }
    return;
}

void cFlatDisplayTracks::Flush(void) {
    TopBarUpdate();
    m_Osd->Flush();
}
