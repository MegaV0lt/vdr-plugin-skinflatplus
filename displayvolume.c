#include "displayvolume.h"
#include "flat.h"

cFlatDisplayVolume::cFlatDisplayVolume(void) {
    muted = false;

    labelHeight = fontHeight + marginItem * 2;

    CreateFullOsd();
    TopBarCreate();
    int width = osdWidth / 4 * 3;

    int top =
        osdHeight - 50 - Config.decorProgressVolumeSize - labelHeight - marginItem - Config.decorBorderVolumeSize * 2;
    int left = osdWidth - width - Config.decorBorderVolumeSize;
    left /= 2;

    labelPixmap = CreatePixmap(osd, 1, cRect(0, top, osdWidth, labelHeight));
    muteLogoPixmap = CreatePixmap(osd, 2, cRect(0, top, osdWidth, labelHeight));

    ProgressBarCreate(left, osdHeight - 50 - Config.decorProgressVolumeSize, width, Config.decorProgressVolumeSize,
                      marginItem, marginItem, Config.decorProgressVolumeFg, Config.decorProgressVolumeBarFg,
                      Config.decorProgressVolumeBg, Config.decorProgressVolumeType, true);
}

cFlatDisplayVolume::~cFlatDisplayVolume() {
    osd->DestroyPixmap(labelPixmap);
    osd->DestroyPixmap(muteLogoPixmap);
}

void cFlatDisplayVolume::SetVolume(int Current, int Total, bool Mute) {
    labelPixmap->Fill(clrTransparent);
    muteLogoPixmap->Fill(clrTransparent);

    cString label = cString::sprintf("%s: %d", tr("Volume"), Current);
    cString maxLabel = cString::sprintf("%s: %d", tr("Volume"), 555);
    int maxlabelWidth = font->Width(maxLabel) + marginItem;
    int left = osdWidth / 2 - maxlabelWidth / 2;

    int DecorTop = osdHeight - 50 - Config.decorProgressVolumeSize - labelHeight - Config.decorBorderVolumeSize * 2;

    labelPixmap->DrawRectangle(cRect(left - marginItem, marginItem, marginItem, fontHeight), Theme.Color(clrVolumeBg));

    DecorBorderClear(left - marginItem, DecorTop, maxlabelWidth + marginItem * 4 + fontHeight, fontHeight,
                     Config.decorBorderVolumeSize);
    DecorBorderClear(left - marginItem, DecorTop, maxlabelWidth + marginItem, fontHeight, Config.decorBorderVolumeSize);

    if (Mute) {
        labelPixmap->DrawText(cPoint(left, marginItem), *label, Theme.Color(clrVolumeFont), Theme.Color(clrVolumeBg),
            font, maxlabelWidth + marginItem + labelHeight, fontHeight, taLeft);
        cImage *img = imgLoader.LoadIcon("mute", fontHeight, fontHeight);
        if (img) {
            muteLogoPixmap->DrawImage(cPoint(left + maxlabelWidth + marginItem, marginItem), *img);
        }
        DecorBorderDraw(left - marginItem, DecorTop, maxlabelWidth + marginItem * 4 + fontHeight, fontHeight,
                        Config.decorBorderVolumeSize, Config.decorBorderVolumeType, Config.decorBorderVolumeFg,
                        Config.decorBorderVolumeBg);
    } else {
        labelPixmap->DrawText(cPoint(left, marginItem), *label, Theme.Color(clrVolumeFont), Theme.Color(clrVolumeBg),
            font, maxlabelWidth, fontHeight, taLeft);
        DecorBorderDraw(left - marginItem, DecorTop, maxlabelWidth + marginItem, fontHeight,
                        Config.decorBorderVolumeSize, Config.decorBorderVolumeType, Config.decorBorderVolumeFg,
                        Config.decorBorderVolumeBg);
    }

    ProgressBarDraw(Current, Total);

    int width = (osdWidth / 4 * 3);
    left = osdWidth - width - Config.decorBorderVolumeSize;
    left /= 2;
    DecorBorderDraw(left - marginItem, osdHeight - 50 - Config.decorProgressVolumeSize - marginItem,
                    width + marginItem * 2, Config.decorProgressVolumeSize + marginItem * 2,
                    Config.decorBorderVolumeSize, Config.decorBorderVolumeType, Theme.Color(clrTopBarBg),
                    Theme.Color(clrTopBarBg));
}

void cFlatDisplayVolume::Flush(void) {
    TopBarUpdate();
    osd->Flush();
}

void cFlatDisplayVolume::PreLoadImages(void) {
    imgLoader.LoadIcon("mute", fontHeight, fontHeight);
}
