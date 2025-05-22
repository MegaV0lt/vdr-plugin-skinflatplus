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

    img_ac3 = ImgLoader.LoadIcon("tracks_ac3", ICON_WIDTH_UNLIMITED, m_FontHeight);
    img_stereo = ImgLoader.LoadIcon("tracks_stereo", ICON_WIDTH_UNLIMITED, m_FontHeight);

    if (img_ac3)
        m_Ac3Width = img_ac3->Width();
    if (img_stereo)
        m_StereoWidth = img_stereo->Width();

    m_ItemHeight = m_FontHeight + Config.MenuItemPadding + Config.decorBorderTrackSize * 2;
    m_CurrentIndex = -1;
    m_MaxItemWidth = m_Font->Width(Title) + m_MarginItem * 4;
    for (int i {0}; i < NumTracks; ++i)
        m_MaxItemWidth = std::max(m_MaxItemWidth, m_Font->Width(Tracks[i]) + m_MarginItem2);

    const int imgWidthMax {std::max(m_Ac3Width, m_StereoWidth)};
    const int headerWidth {m_Font->Width(Title) + m_Font->Width(' ') + imgWidthMax};
    m_MaxItemWidth = std::max(m_MaxItemWidth, headerWidth);

    ItemsHeight = (NumTracks + 1) * m_ItemHeight;
    const int left {(m_OsdWidth - m_MaxItemWidth) / 2};
    TopBarSetTitle(Title);

    const cRect TraksPixmapViewPort{left, m_OsdHeight - ItemsHeight - m_MarginItem, m_MaxItemWidth, ItemsHeight};
    TracksPixmap = CreatePixmap(m_Osd, "TracksPixmap", 1, TraksPixmapViewPort);
    TracksLogoPixmap = CreatePixmap(m_Osd, "TracksLogoPixmap", 1, TraksPixmapViewPort);
    PixmapClear(TracksPixmap);
    PixmapClear(TracksLogoPixmap);

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

    const int y {(Index + 1) * m_ItemHeight};
    /* tColor ColorFg = Theme.Color(clrTrackItemFont);
    tColor ColorBg = Theme.Color(clrTrackItemBg);
    if (Current) {
        ColorFg = Theme.Color(clrTrackItemCurrentFont);
        ColorBg = Theme.Color(clrTrackItemCurrentBg);
        m_CurrentIndex = Index;
    } else if (Index >= 0) {
        ColorFg = Theme.Color(clrTrackItemSelableFont);
        ColorBg = Theme.Color(clrTrackItemSelableBg);
    } */
    const auto [ColorFg, ColorBg] =
        Current      ? std::make_pair(Theme.Color(clrTrackItemCurrentFont), Theme.Color(clrTrackItemCurrentBg))
        : Index >= 0 ? std::make_pair(Theme.Color(clrTrackItemSelableFont), Theme.Color(clrTrackItemSelableBg))
                     : std::make_pair(Theme.Color(clrTrackItemFont), Theme.Color(clrTrackItemBg));

    if (Index == -1)
        TracksPixmap->DrawText(cPoint(0, y), Text, ColorFg, ColorBg, m_Font, m_MaxItemWidth,
                               m_ItemHeight - Config.MenuItemPadding - Config.decorBorderTrackSize * 2, taLeft);
    else
        TracksPixmap->DrawText(cPoint(0, y), Text, ColorFg, ColorBg, m_Font, m_MaxItemWidth,
                               m_ItemHeight - Config.MenuItemPadding - Config.decorBorderTrackSize * 2, taCenter);

    const int left {(m_OsdWidth - m_MaxItemWidth) / 2};
    const int top {m_OsdHeight - ItemsHeight - m_MarginItem + y};

    sDecorBorder ib {left,
                     top,
                     m_MaxItemWidth,
                     m_FontHeight,
                     Config.decorBorderTrackSize,
                     Config.decorBorderTrackType,
                     Config.decorBorderTrackFg,
                     Config.decorBorderTrackBg};
    if (Current) {
        ib.ColorFg = Config.decorBorderTrackCurFg;
        ib.ColorBg = Config.decorBorderTrackCurBg;
    } else if (Index >= 0) {
        ib.ColorFg = Config.decorBorderTrackSelFg;
        ib.ColorBg = Config.decorBorderTrackSelBg;
    }
    DecorBorderDraw(ib);
}

void cFlatDisplayTracks::SetTrack(int Index, const char * const *Tracks) {
    if (m_CurrentIndex >= 0)
        SetItem(Tracks[m_CurrentIndex], m_CurrentIndex, false);

    SetItem(Tracks[Index], Index, true);
}

void cFlatDisplayTracks::SetAudioChannel(int AudioChannel) {
    if (!TracksLogoPixmap) return;

    PixmapClear(TracksLogoPixmap);
    // From vdr: 0=stereo, 1=left, 2=right, -1=don't display the audio channel indicator.
    // From skinnopacity: -1 ac3, else stereo
    if (AudioChannel == -1 && img_ac3) {
        const int IconLeft {m_MaxItemWidth - img_ac3->Width() - m_MarginItem};
        TracksLogoPixmap->DrawImage(cPoint(IconLeft, 0), *img_ac3);
    } else if (img_stereo) {
        const int IconLeft {m_MaxItemWidth - img_stereo->Width() - m_MarginItem};
        TracksLogoPixmap->DrawImage(cPoint(IconLeft, 0), *img_stereo);
    }
    return;
}

void cFlatDisplayTracks::Flush() {
    TopBarUpdate();
    m_Osd->Flush();
}
