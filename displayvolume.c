#include "displayvolume.h"
#include "flat.h"

cFlatDisplayVolume::cFlatDisplayVolume(void) {
    // Muted = false;  // Unused?

    m_LabelHeight = m_FontHeight + m_MarginItem * 2;

    CreateFullOsd();
    TopBarCreate();
    int width = m_OsdWidth / 4 * 3;

    int top =
        m_OsdHeight - 50 - Config.decorProgressVolumeSize - m_LabelHeight - m_MarginItem - Config.decorBorderVolumeSize * 2;
    int left = m_OsdWidth - width - Config.decorBorderVolumeSize;
    left /= 2;

    LabelPixmap = CreatePixmap(osd, "LabelPixmap", 1, cRect(0, top, m_OsdWidth, m_LabelHeight));
    MuteLogoPixmap = CreatePixmap(osd, "MuteLogoPixmap", 2, cRect(0, top, m_OsdWidth, m_LabelHeight));

    ProgressBarCreate(left, m_OsdHeight - 50 - Config.decorProgressVolumeSize, width, Config.decorProgressVolumeSize,
                      m_MarginItem, m_MarginItem, Config.decorProgressVolumeFg, Config.decorProgressVolumeBarFg,
                      Config.decorProgressVolumeBg, Config.decorProgressVolumeType, true);
}

cFlatDisplayVolume::~cFlatDisplayVolume() {
    osd->DestroyPixmap(LabelPixmap);
    osd->DestroyPixmap(MuteLogoPixmap);
}

void cFlatDisplayVolume::SetVolume(int Current, int Total, bool Mute) {
    PixmapFill(LabelPixmap, clrTransparent);
    PixmapFill(MuteLogoPixmap, clrTransparent);

    cString label = cString::sprintf("%s: %d", tr("Volume"), Current);
    cString MaxLabel = cString::sprintf("%s: %d", tr("Volume"), 555);
    int MaxLabelWidth = m_Font->Width(*MaxLabel) + m_MarginItem;
    int left = m_OsdWidth / 2 - MaxLabelWidth / 2;

    int DecorTop = m_OsdHeight - 50 - Config.decorProgressVolumeSize - m_LabelHeight - Config.decorBorderVolumeSize * 2;

    LabelPixmap->DrawRectangle(cRect(left - m_MarginItem, m_MarginItem, m_MarginItem, m_FontHeight), Theme.Color(clrVolumeBg));

    DecorBorderClear(left - m_MarginItem, DecorTop, MaxLabelWidth + m_MarginItem * 4 + m_FontHeight, m_FontHeight,
                     Config.decorBorderVolumeSize);
    DecorBorderClear(left - m_MarginItem, DecorTop, MaxLabelWidth + m_MarginItem, m_FontHeight, Config.decorBorderVolumeSize);

    if (Mute) {
        LabelPixmap->DrawText(cPoint(left, m_MarginItem), *label, Theme.Color(clrVolumeFont), Theme.Color(clrVolumeBg),
            m_Font, MaxLabelWidth + m_MarginItem + m_LabelHeight, m_FontHeight, taLeft);
        cImage *img = ImgLoader.LoadIcon("mute", m_FontHeight, m_FontHeight);
        if (img) {
            MuteLogoPixmap->DrawImage(cPoint(left + MaxLabelWidth + m_MarginItem, m_MarginItem), *img);
        }
        DecorBorderDraw(left - m_MarginItem, DecorTop, MaxLabelWidth + m_MarginItem * 4 + m_FontHeight, m_FontHeight,
                        Config.decorBorderVolumeSize, Config.decorBorderVolumeType, Config.decorBorderVolumeFg,
                        Config.decorBorderVolumeBg);
    } else {
        LabelPixmap->DrawText(cPoint(left, m_MarginItem), *label, Theme.Color(clrVolumeFont), Theme.Color(clrVolumeBg),
            m_Font, MaxLabelWidth, m_FontHeight, taLeft);
        DecorBorderDraw(left - m_MarginItem, DecorTop, MaxLabelWidth + m_MarginItem, m_FontHeight,
                        Config.decorBorderVolumeSize, Config.decorBorderVolumeType, Config.decorBorderVolumeFg,
                        Config.decorBorderVolumeBg);
    }

    ProgressBarDraw(Current, Total);

    int width = (m_OsdWidth / 4 * 3);
    left = m_OsdWidth - width - Config.decorBorderVolumeSize;
    left /= 2;
    DecorBorderDraw(left - m_MarginItem, m_OsdHeight - 50 - Config.decorProgressVolumeSize - m_MarginItem,
                    width + m_MarginItem * 2, Config.decorProgressVolumeSize + m_MarginItem * 2,
                    Config.decorBorderVolumeSize, Config.decorBorderVolumeType, Theme.Color(clrTopBarBg),
                    Theme.Color(clrTopBarBg));
}

void cFlatDisplayVolume::Flush(void) {
    TopBarUpdate();
    osd->Flush();
}

void cFlatDisplayVolume::PreLoadImages(void) {
    ImgLoader.LoadIcon("mute", m_FontHeight, m_FontHeight);
}
