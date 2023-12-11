#include "displayvolume.h"
#include "flat.h"

cFlatDisplayVolume::cFlatDisplayVolume(void) {
    // Muted = false;  // Unused?

    g_LabelHeight = g_FontHight + g_MarginItem * 2;

    CreateFullOsd();
    TopBarCreate();
    int width = g_OsdWidth / 4 * 3;

    int top =
        g_OsdHeight - 50 - Config.decorProgressVolumeSize - g_LabelHeight - g_MarginItem - Config.decorBorderVolumeSize * 2;
    int left = g_OsdWidth - width - Config.decorBorderVolumeSize;
    left /= 2;

    LabelPixmap = CreatePixmap(osd, "LabelPixmap", 1, cRect(0, top, g_OsdWidth, g_LabelHeight));
    MuteLogoPixmap = CreatePixmap(osd, "MuteLogoPixmap", 2, cRect(0, top, g_OsdWidth, g_LabelHeight));

    ProgressBarCreate(left, g_OsdHeight - 50 - Config.decorProgressVolumeSize, width, Config.decorProgressVolumeSize,
                      g_MarginItem, g_MarginItem, Config.decorProgressVolumeFg, Config.decorProgressVolumeBarFg,
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
    int MaxLabelWidth = g_Font->Width(*MaxLabel) + g_MarginItem;
    int left = g_OsdWidth / 2 - MaxLabelWidth / 2;

    int DecorTop = g_OsdHeight - 50 - Config.decorProgressVolumeSize - g_LabelHeight - Config.decorBorderVolumeSize * 2;

    LabelPixmap->DrawRectangle(cRect(left - g_MarginItem, g_MarginItem, g_MarginItem, g_FontHight), Theme.Color(clrVolumeBg));

    DecorBorderClear(left - g_MarginItem, DecorTop, MaxLabelWidth + g_MarginItem * 4 + g_FontHight, g_FontHight,
                     Config.decorBorderVolumeSize);
    DecorBorderClear(left - g_MarginItem, DecorTop, MaxLabelWidth + g_MarginItem, g_FontHight, Config.decorBorderVolumeSize);

    if (Mute) {
        LabelPixmap->DrawText(cPoint(left, g_MarginItem), *label, Theme.Color(clrVolumeFont), Theme.Color(clrVolumeBg),
            g_Font, MaxLabelWidth + g_MarginItem + g_LabelHeight, g_FontHight, taLeft);
        cImage *img = imgLoader.LoadIcon("mute", g_FontHight, g_FontHight);
        if (img) {
            MuteLogoPixmap->DrawImage(cPoint(left + MaxLabelWidth + g_MarginItem, g_MarginItem), *img);
        }
        DecorBorderDraw(left - g_MarginItem, DecorTop, MaxLabelWidth + g_MarginItem * 4 + g_FontHight, g_FontHight,
                        Config.decorBorderVolumeSize, Config.decorBorderVolumeType, Config.decorBorderVolumeFg,
                        Config.decorBorderVolumeBg);
    } else {
        LabelPixmap->DrawText(cPoint(left, g_MarginItem), *label, Theme.Color(clrVolumeFont), Theme.Color(clrVolumeBg),
            g_Font, MaxLabelWidth, g_FontHight, taLeft);
        DecorBorderDraw(left - g_MarginItem, DecorTop, MaxLabelWidth + g_MarginItem, g_FontHight,
                        Config.decorBorderVolumeSize, Config.decorBorderVolumeType, Config.decorBorderVolumeFg,
                        Config.decorBorderVolumeBg);
    }

    ProgressBarDraw(Current, Total);

    int width = (g_OsdWidth / 4 * 3);
    left = g_OsdWidth - width - Config.decorBorderVolumeSize;
    left /= 2;
    DecorBorderDraw(left - g_MarginItem, g_OsdHeight - 50 - Config.decorProgressVolumeSize - g_MarginItem,
                    width + g_MarginItem * 2, Config.decorProgressVolumeSize + g_MarginItem * 2,
                    Config.decorBorderVolumeSize, Config.decorBorderVolumeType, Theme.Color(clrTopBarBg),
                    Theme.Color(clrTopBarBg));
}

void cFlatDisplayVolume::Flush(void) {
    TopBarUpdate();
    osd->Flush();
}

void cFlatDisplayVolume::PreLoadImages(void) {
    imgLoader.LoadIcon("mute", g_FontHight, g_FontHight);
}
