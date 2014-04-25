#include "setup.h"

cStringList Bordertypes;
cStringList DiskUsages;
cStringList Progresstypes;
cStringList MenuChannelViews;
cStringList MenuTimerViews;
cStringList MenuEventViews;
cStringList MenuRecordingViews;
cStringList DecorDescriptions;

cFlatSetup::cFlatSetup(void) {
    SetupConfig = Config;
    ItemLastSel = -1;
    Setup();
}

cFlatSetup::~cFlatSetup() {

}

void cFlatSetup::Setup(void) {
    Clear();
    Bordertypes.Clear();
    Progresstypes.Clear();

    Bordertypes.Clear();
    Bordertypes.Append( strdup(tr("none")) );
    Bordertypes.Append( strdup(tr("rect")) );
    Bordertypes.Append( strdup(tr("round")) );
    Bordertypes.Append( strdup(tr("invert round")) );
    Bordertypes.Append( strdup(tr("rect + alpha blend")) );
    Bordertypes.Append( strdup(tr("round + alpha blend")) );
    Bordertypes.Append( strdup(tr("invert round + alpha blend")) );

    Progresstypes.Clear();
    Progresstypes.Append( strdup(tr("small line + big line")) );
    Progresstypes.Append( strdup(tr("big line")) );
    Progresstypes.Append( strdup(tr("big line + outline")) );
    Progresstypes.Append( strdup(tr("small line + big line + dot")) );
    Progresstypes.Append( strdup(tr("big line + dot")) );
    Progresstypes.Append( strdup(tr("big line + outline + dot")) );
    Progresstypes.Append( strdup(tr("small line + dot")) );
    Progresstypes.Append( strdup(tr("outline + dot")) );
    Progresstypes.Append( strdup(tr("small line + big line + alpha blend")) );
    Progresstypes.Append( strdup(tr("big line + alpha blend")) );

    DiskUsages.Clear();
    DiskUsages.Append( strdup(tr("do not show")) );
    DiskUsages.Append( strdup(tr("timer & recording menu")) );
    DiskUsages.Append( strdup(tr("always on the menu")) );
    DiskUsages.Append( strdup(tr("always show")) );

    MenuChannelViews.Clear();
    MenuChannelViews.Append( strdup(tr("VDR default")) );
    MenuChannelViews.Append( strdup(tr("flatPlus long")) );
    MenuChannelViews.Append( strdup(tr("flatPlus long + EPG")) );
    MenuChannelViews.Append( strdup(tr("flatPlus short")) );
    MenuChannelViews.Append( strdup(tr("flatPlus short + EPG")) );

    MenuTimerViews.Clear();
    MenuTimerViews.Append( strdup(tr("VDR default")) );
    MenuTimerViews.Append( strdup(tr("flatPlus long")) );
    MenuTimerViews.Append( strdup(tr("flatPlus short")) );
    MenuTimerViews.Append( strdup(tr("flatPlus short + EPG")) );

    MenuEventViews.Clear();
    MenuEventViews.Append( strdup(tr("VDR default")) );
    MenuEventViews.Append( strdup(tr("flatPlus long")) );
    MenuEventViews.Append( strdup(tr("flatPlus short")) );
    MenuEventViews.Append( strdup(tr("flatPlus short + EPG")) );

    MenuRecordingViews.Clear();
    MenuRecordingViews.Append( strdup(tr("VDR default")) );
    MenuRecordingViews.Append( strdup(tr("flatPlus long")) );
    MenuRecordingViews.Append( strdup(tr("flatPlus short")) );
    MenuRecordingViews.Append( strdup(tr("flatPlus short + Info")) );

    Add(new cOsdItem(tr("General settings"), osUnknown, true));
    Add(new cOsdItem(tr("Channelinfo settings"), osUnknown, true));
    Add(new cOsdItem(tr("Menu settings"), osUnknown, true));
    Add(new cOsdItem(tr("Replay settings"), osUnknown, true));
    Add(new cOsdItem(tr("Volume settings"), osUnknown, true));
    Add(new cOsdItem(tr("Tracks settings"), osUnknown, true));

    static cPlugin *pTVScraper = cPluginManager::GetPlugin("tvscraper");
    static cPlugin *pScraper2Vdr = cPluginManager::GetPlugin("scraper2vdr");
    if( pTVScraper || pScraper2Vdr )
        Add(new cOsdItem(tr("TVScraper settings"), osUnknown, true));
    else
        Add(new cOsdItem(tr("TVScraper not installed"), osUnknown, false));

    if( ItemLastSel >= 0 ) {
        SetCurrent(Get(ItemLastSel));
        ItemLastSel = -1;
    }

    Display();
}

eOSState cFlatSetup::ProcessKey(eKeys Key) {
    bool hadSubMenu = HasSubMenu();
    eOSState state = cMenuSetupPage::ProcessKey(Key);
    if (hadSubMenu && Key == kOk)
        Store();
    if (!hadSubMenu && (state == osUnknown || Key == kOk)) {
        if ((Key == kOk && !hadSubMenu)) {
            const char* ItemText = Get(Current())->Text();
            if (strcmp(ItemText, tr("General settings")) == 0)
                state = AddSubMenu(new cFlatSetupGeneral(&SetupConfig));
            if (strcmp(ItemText, tr("Channelinfo settings")) == 0)
                state = AddSubMenu(new cFlatSetupChannelInfo(&SetupConfig));
            if (strcmp(ItemText, tr("Menu settings")) == 0)
                state = AddSubMenu(new cFlatSetupMenu(&SetupConfig));
            if (strcmp(ItemText, tr("Replay settings")) == 0)
                state = AddSubMenu(new cFlatSetupReplay(&SetupConfig));
            if (strcmp(ItemText, tr("Volume settings")) == 0)
                state = AddSubMenu(new cFlatSetupVolume(&SetupConfig));
            if (strcmp(ItemText, tr("Tracks settings")) == 0)
                state = AddSubMenu(new cFlatSetupTracks(&SetupConfig));
            if (strcmp(ItemText, tr("TVScraper settings")) == 0)
                state = AddSubMenu(new cFlatSetupTvsraper(&SetupConfig));
        }
    }
    return state;
}

void cFlatSetup::Store(void) {
    Config = SetupConfig;
    SetupStore("decorBorderChannelByTheme", Config.decorBorderChannelByTheme);
    SetupStore("decorBorderChannelTypeUser", Config.decorBorderChannelTypeUser);
    SetupStore("decorBorderChannelSizeUser", Config.decorBorderChannelSizeUser);
    SetupStore("decorBorderTopBarByTheme", Config.decorBorderTopBarByTheme);
    SetupStore("decorBorderTopBarTypeUser", Config.decorBorderTopBarTypeUser);
    SetupStore("decorBorderTopBarSizeUser", Config.decorBorderTopBarSizeUser);
    SetupStore("decorBorderMessageByTheme", Config.decorBorderMessageByTheme);
    SetupStore("decorBorderMessageTypeUser", Config.decorBorderMessageTypeUser);
    SetupStore("decorBorderMessageSizeUser", Config.decorBorderMessageSizeUser);
    SetupStore("decorBorderVolumeByTheme", Config.decorBorderVolumeByTheme);
    SetupStore("decorBorderVolumeTypeUser", Config.decorBorderVolumeTypeUser);
    SetupStore("decorBorderVolumeSizeUser", Config.decorBorderVolumeSizeUser);
    SetupStore("decorBorderTrackByTheme", Config.decorBorderTrackByTheme);
    SetupStore("decorBorderTrackTypeUser", Config.decorBorderTrackTypeUser);
    SetupStore("decorBorderTrackSizeUser", Config.decorBorderTrackSizeUser);
    SetupStore("decorBorderReplayByTheme", Config.decorBorderReplayByTheme);
    SetupStore("decorBorderReplayTypeUser", Config.decorBorderReplayTypeUser);
    SetupStore("decorBorderReplaySizeUser", Config.decorBorderReplaySizeUser);
    SetupStore("decorBorderMenuItemByTheme", Config.decorBorderMenuItemByTheme);
    SetupStore("decorBorderMenuItemTypeUser", Config.decorBorderMenuItemTypeUser);
    SetupStore("decorBorderMenuItemSizeUser", Config.decorBorderMenuItemSizeUser);
    SetupStore("decorBorderMenuContentHeadByTheme", Config.decorBorderMenuContentHeadByTheme);
    SetupStore("decorBorderMenuContentHeadTypeUser", Config.decorBorderMenuContentHeadTypeUser);
    SetupStore("decorBorderMenuContentHeadSizeUser", Config.decorBorderMenuContentHeadSizeUser);
    SetupStore("decorBorderMenuContentByTheme", Config.decorBorderMenuContentByTheme);
    SetupStore("decorBorderMenuContentTypeUser", Config.decorBorderMenuContentTypeUser);
    SetupStore("decorBorderMenuContentSizeUser", Config.decorBorderMenuContentSizeUser);
    SetupStore("decorBorderButtonByTheme", Config.decorBorderButtonByTheme);
    SetupStore("decorBorderButtonTypeUser", Config.decorBorderButtonTypeUser);
    SetupStore("decorBorderButtonSizeUser", Config.decorBorderButtonSizeUser);
    SetupStore("decorProgressChannelByTheme", Config.decorProgressChannelByTheme);
    SetupStore("decorProgressChannelTypeUser", Config.decorProgressChannelTypeUser);
    SetupStore("decorProgressChannelSizeUser", Config.decorProgressChannelSizeUser);
    SetupStore("decorProgressVolumeByTheme", Config.decorProgressVolumeByTheme);
    SetupStore("decorProgressVolumeTypeUser", Config.decorProgressVolumeTypeUser);
    SetupStore("decorProgressVolumeSizeUser", Config.decorProgressVolumeSizeUser);
    SetupStore("decorProgressMenuItemByTheme", Config.decorProgressMenuItemByTheme);
    SetupStore("decorProgressMenuItemTypeUser", Config.decorProgressMenuItemTypeUser);
    SetupStore("decorProgressMenuItemSizeUser", Config.decorProgressMenuItemSizeUser);
    SetupStore("decorProgressReplayByTheme", Config.decorProgressReplayByTheme);
    SetupStore("decorProgressReplayTypeUser", Config.decorProgressReplayTypeUser);
    SetupStore("decorProgressReplaySizeUser", Config.decorProgressReplaySizeUser);
    SetupStore("decorProgressSignalByTheme", Config.decorProgressSignalByTheme);
    SetupStore("decorProgressSignalTypeUser", Config.decorProgressSignalTypeUser);
    SetupStore("decorProgressSignalSizeUser", Config.decorProgressSignalSizeUser);
    SetupStore("ButtonsShowEmpty", Config.ButtonsShowEmpty);
    SetupStore("ChannelIconsShow", Config.ChannelIconsShow);
    SetupStore("SignalQualityShow", Config.SignalQualityShow);
    SetupStore("DiskUsageShow", Config.DiskUsageShow);
    SetupStore("MenuItemPadding", Config.MenuItemPadding);
    SetupStore("marginOsdVer", Config.marginOsdVer);
    SetupStore("marginOsdHor", Config.marginOsdHor);
    SetupStore("TopBarFontSize", dtoa(Config.TopBarFontSize));
    SetupStore("MessageOffset", Config.MessageOffset);
    SetupStore("MenuContentFullSize", Config.MenuContentFullSize);
    SetupStore("ChannelResolutionAspectShow", Config.ChannelResolutionAspectShow);
    SetupStore("ChannelFormatShow", Config.ChannelFormatShow);
    SetupStore("RecordingResolutionAspectShow", Config.RecordingResolutionAspectShow);
    SetupStore("RecordingFormatShow", Config.RecordingFormatShow);
    SetupStore("RecordingAdditionalInfoShow", Config.RecordingAdditionalInfoShow);
    SetupStore("EpgAdditionalInfoShow", Config.EpgAdditionalInfoShow);
    SetupStore("TopBarRecordingShow", Config.TopBarRecordingShow);
    SetupStore("MenuItemIconsShow", Config.MenuItemIconsShow);
    SetupStore("TopBarMenuIconShow", Config.TopBarMenuIconShow);
    SetupStore("DecorIndex", Config.DecorIndex);
    SetupStore("MainMenuItemScale", dtoa(Config.MainMenuItemScale));
    SetupStore("MenuChannelView", Config.MenuChannelView);
    SetupStore("MenuTimerView", Config.MenuTimerView);
    SetupStore("MenuEventView", Config.MenuEventView);
    SetupStore("MenuRecordingView", Config.MenuRecordingView);
    SetupStore("ChannelSimpleAspectFormat", Config.ChannelSimpleAspectFormat);
    SetupStore("RecordingSimpleAspectFormat", Config.RecordingSimpleAspectFormat);
    SetupStore("MenuItemRecordingClearPercent", Config.MenuItemRecordingClearPercent);
    SetupStore("MenuItemRecordingShowFolderDate", Config.MenuItemRecordingShowFolderDate);
    SetupStore("MenuItemParseTilde", Config.MenuItemParseTilde);
    SetupStore("TopBarRecConflictsShow", Config.TopBarRecConflictsShow);
    SetupStore("TopBarRecConflictsHigh", Config.TopBarRecConflictsHigh);
    SetupStore("SignalQualityUseColors", Config.SignalQualityUseColors);
    SetupStore("TVScraperChanInfoShowPoster", Config.TVScraperChanInfoShowPoster);
    SetupStore("TVScraperChanInfoPosterSize", dtoa(Config.TVScraperChanInfoPosterSize));
    SetupStore("TVScraperEPGInfoShowPoster", Config.TVScraperEPGInfoShowPoster);
    SetupStore("TVScraperRecInfoShowPoster", Config.TVScraperEPGInfoShowPoster);
    SetupStore("EpgRerunsShow", Config.EpgRerunsShow);
    SetupStore("TVScraperEPGInfoShowActors", Config.TVScraperEPGInfoShowActors);
    SetupStore("TVScraperRecInfoShowActors", Config.TVScraperRecInfoShowActors);

    Config.Init();
}

//------------------------------------------------------------------------------------------------------------------

cMenuSetupSubMenu::cMenuSetupSubMenu(const char* Title, cFlatConfig* data) : cOsdMenu(Title, 30) {
    SetupConfig = data;
    ItemLastSel = -1;
}

cOsdItem *cMenuSetupSubMenu::InfoItem(const char *label, const char *value) {
    cOsdItem *item;
    item = new cOsdItem(cString::sprintf("%s: %s", label, value));
    item->SetSelectable(false);
    return item;
}

// General Settings
cFlatSetupGeneral::cFlatSetupGeneral(cFlatConfig* data)  : cMenuSetupSubMenu(tr("General settings"), data) {
    Setup();
}

void cFlatSetupGeneral::Setup(void) {
    Clear();

    DecorDescriptions.Clear();
    SetupConfig->DecorDescriptions( DecorDescriptions );
    if( SetupConfig->DecorIndex < 0 || SetupConfig->DecorIndex > DecorDescriptions.Size() )
        SetupConfig->DecorIndex = 0;

    Add(new cMenuEditStraItem(tr("Decorfile"), &SetupConfig->DecorIndex, DecorDescriptions.Size(), &DecorDescriptions[0]));

    Add(new cMenuEditBoolItem(tr("Show empty color-buttons"), &SetupConfig->ButtonsShowEmpty));
    Add(new cMenuEditBoolItem(tr("Show TopBar menu icons"), &SetupConfig->TopBarMenuIconShow));
    Add(new cMenuEditStraItem(tr("Show Diskusage stats"), &SetupConfig->DiskUsageShow, DiskUsages.Size(), &DiskUsages[0]));
    Add(new cMenuEditIntItem(tr("OSD vertical margin"), &SetupConfig->marginOsdVer));
    Add(new cMenuEditIntItem(tr("OSD horizontal margin"), &SetupConfig->marginOsdHor));
    Add(new cMenuEditPrcItem(tr("TopBar font size"), &SetupConfig->TopBarFontSize, 0.01, 0.2, 1));
    Add(new cMenuEditBoolItem(tr("TopBar show recording"), &SetupConfig->TopBarRecordingShow));
    Add(new cMenuEditBoolItem(tr("TopBar show conflicts"), &SetupConfig->TopBarRecConflictsShow));
    Add(new cMenuEditIntItem(tr("Conflicts min value for red"), &SetupConfig->TopBarRecConflictsHigh));
    Add(new cMenuEditIntItem(tr("Message bottom offset"), &SetupConfig->MessageOffset));


    Add(new cMenuEditBoolItem(tr("TopBar border by decor-file?"), &SetupConfig->decorBorderTopBarByTheme));
    if( SetupConfig->decorBorderTopBarByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("TopBar border type"), Bordertypes[SetupConfig->decorBorderTopBarTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("TopBar border size"), SetupConfig->decorBorderTopBarSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("TopBar border type"), &SetupConfig->decorBorderTopBarTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("TopBar border size"), &SetupConfig->decorBorderTopBarSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Message border by decor-file?"), &SetupConfig->decorBorderMessageByTheme));
    if( SetupConfig->decorBorderMessageByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Message border type"), Bordertypes[SetupConfig->decorBorderMessageTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Message border size"), SetupConfig->decorBorderMessageSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Message border type"), &SetupConfig->decorBorderMessageTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Message border size"), &SetupConfig->decorBorderMessageSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Button border by decor-file?"), &SetupConfig->decorBorderButtonByTheme));
    if( SetupConfig->decorBorderButtonByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Button border type"), Bordertypes[SetupConfig->decorBorderButtonTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Button border size"), SetupConfig->decorBorderButtonSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Button border type"), &SetupConfig->decorBorderButtonTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Button border size"), &SetupConfig->decorBorderButtonSizeUser));
    }

    cString ImageCache = cString::sprintf("%s:\t%d / %d", tr("Imagecache entries"), imgCache.getCacheCount(), MAX_IMAGE_CACHE);
    Add(new cOsdItem(ImageCache, osUnknown, true));

    if( ItemLastSel >= 0 ) {
        SetCurrent(Get(ItemLastSel));
        ItemLastSel = -1;
    }

    Display();
}

eOSState cFlatSetupGeneral::ProcessKey(eKeys Key) {
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
            case kOk:
                return osBack;
            default:
                break;
        }
    }
    if( Key == kLeft || Key == kRight ) {
        const char* ItemText = Get(Current())->Text();
        if( strstr(ItemText, tr("TopBar border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Message border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Button border by decor-file?")) != NULL
        ) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// Channel Info Settings
cFlatSetupChannelInfo::cFlatSetupChannelInfo(cFlatConfig* data)  : cMenuSetupSubMenu(tr("Channelinfo settings"), data) {
    Setup();
}

void cFlatSetupChannelInfo::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Show Channelinfo icons"), &SetupConfig->ChannelIconsShow));
    Add(new cMenuEditBoolItem(tr("Show signal quality"), &SetupConfig->SignalQualityShow));
    Add(new cMenuEditBoolItem(tr("Colors for signal quality"), &SetupConfig->SignalQualityUseColors));
    Add(new cMenuEditBoolItem(tr("Show resolution & aspect"), &SetupConfig->ChannelResolutionAspectShow));
    Add(new cMenuEditBoolItem(tr("Show format (hd/sd)"), &SetupConfig->ChannelFormatShow));
    Add(new cMenuEditBoolItem(tr("Simple aspect & format"), &SetupConfig->ChannelSimpleAspectFormat));

    Add(new cMenuEditBoolItem(tr("Channelinfo border by decor-file?"), &SetupConfig->decorBorderChannelByTheme));
    if( SetupConfig->decorBorderChannelByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Channelinfo border type"), Bordertypes[SetupConfig->decorBorderChannelTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Channelinfo border size"), SetupConfig->decorBorderChannelSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Channelinfo border type"), &SetupConfig->decorBorderChannelTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Channelinfo border size"), &SetupConfig->decorBorderChannelSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Channelinfo progress by decor-file?"), &SetupConfig->decorProgressChannelByTheme));
    if( SetupConfig->decorProgressChannelByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Channelinfo progress type"), Progresstypes[SetupConfig->decorProgressChannelTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Channelinfo progress size"), SetupConfig->decorProgressChannelSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Channelinfo progress type"), &SetupConfig->decorProgressChannelTypeUser, Progresstypes.Size(), &Progresstypes[0]));
        Add(new cMenuEditIntItem(tr("Channelinfo progress size"), &SetupConfig->decorProgressChannelSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Signalquality progress by decor-file?"), &SetupConfig->decorProgressSignalByTheme));
    if( SetupConfig->decorProgressSignalByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Signalquality progress type"), Progresstypes[SetupConfig->decorProgressSignalTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Signalquality progress size"), SetupConfig->decorProgressSignalSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Signalquality progress type"), &SetupConfig->decorProgressSignalTypeUser, Progresstypes.Size(), &Progresstypes[0]));
        Add(new cMenuEditIntItem(tr("Signalquality progress size"), &SetupConfig->decorProgressSignalSizeUser));
    }

    if( ItemLastSel >= 0 ) {
        SetCurrent(Get(ItemLastSel));
        ItemLastSel = -1;
    }

    Display();
}

eOSState cFlatSetupChannelInfo::ProcessKey(eKeys Key) {
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
            case kOk:
                return osBack;
            default:
                break;
        }
    }
    if( Key == kLeft || Key == kRight ) {
        const char* ItemText = Get(Current())->Text();
        if( strstr(ItemText, tr("Channelinfo border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Channelinfo progress by decor-file?")) != NULL ||
            strstr(ItemText, tr("Signalquality progress by decor-file?")) != NULL
        ) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// Menu Settings
cFlatSetupMenu::cFlatSetupMenu(cFlatConfig* data)  : cMenuSetupSubMenu(tr("Menu settings"), data) {
    Setup();
}

void cFlatSetupMenu::Setup(void) {
    Clear();

    Add(new cMenuEditIntItem(tr("Menuitem padding"), &SetupConfig->MenuItemPadding));
    Add(new cMenuEditBoolItem(tr("Show mainmenu item icons"), &SetupConfig->MenuItemIconsShow));
    Add(new cMenuEditBoolItem(tr("Menu content fullsize background"), &SetupConfig->MenuContentFullSize));
    Add(new cMenuEditBoolItem(tr("Show additional recording info"), &SetupConfig->RecordingAdditionalInfoShow));
    Add(new cMenuEditBoolItem(tr("Show additional EPG info"), &SetupConfig->EpgAdditionalInfoShow));
    Add(new cMenuEditBoolItem(tr("Show reruns in EPG info"), &SetupConfig->EpgRerunsShow));
    Add(new cMenuEditPrcItem(tr("Main menuitem scale"), &SetupConfig->MainMenuItemScale, 0.2, 1, 0));

    Add(new cMenuEditStraItem(tr("Menu channel view"), &SetupConfig->MenuChannelView, MenuChannelViews.Size(), &MenuChannelViews[0]));
    Add(new cMenuEditStraItem(tr("Menu timer view"), &SetupConfig->MenuTimerView, MenuTimerViews.Size(), &MenuTimerViews[0]));
    Add(new cMenuEditStraItem(tr("Menu event view"), &SetupConfig->MenuEventView, MenuEventViews.Size(), &MenuEventViews[0]));
    Add(new cMenuEditStraItem(tr("Menu recording view"), &SetupConfig->MenuRecordingView, MenuRecordingViews.Size(), &MenuRecordingViews[0]));

    Add(new cMenuEditBoolItem(tr("Menuitem border by decor-file?"), &SetupConfig->decorBorderMenuItemByTheme));
    if( SetupConfig->decorBorderMenuItemByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Menuitem border type"), Bordertypes[SetupConfig->decorBorderMenuItemTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Menuitem border size"), SetupConfig->decorBorderMenuItemSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Menuitem border type"), &SetupConfig->decorBorderMenuItemTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Menuitem border size"), &SetupConfig->decorBorderMenuItemSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Menucont. border by decor-file?"), &SetupConfig->decorBorderMenuContentByTheme));
    if( SetupConfig->decorBorderMenuContentByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Menucont. border type"), Bordertypes[SetupConfig->decorBorderMenuContentTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Menucont. border size"), SetupConfig->decorBorderMenuContentSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Menucont. border type"), &SetupConfig->decorBorderMenuContentTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Menucont. border size"), &SetupConfig->decorBorderMenuContentSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Menucont. head border by decor-file?"), &SetupConfig->decorBorderMenuContentHeadByTheme));
    if( SetupConfig->decorBorderMenuContentHeadByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Menucont. head border type"), Bordertypes[SetupConfig->decorBorderMenuContentHeadTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Menucont. head border size"), SetupConfig->decorBorderMenuContentHeadSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Menucont. head border type"), &SetupConfig->decorBorderMenuContentHeadTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Menucont. head border size"), &SetupConfig->decorBorderMenuContentHeadSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Menuitem progress by decor-file?"), &SetupConfig->decorProgressMenuItemByTheme));
    if( SetupConfig->decorProgressMenuItemByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Menuitem progress type"), Progresstypes[SetupConfig->decorProgressMenuItemTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Menuitem progress size"), SetupConfig->decorProgressMenuItemSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Menuitem progress type"), &SetupConfig->decorProgressMenuItemTypeUser, Progresstypes.Size(), &Progresstypes[0]));
        Add(new cMenuEditIntItem(tr("Menuitem progress size"), &SetupConfig->decorProgressMenuItemSizeUser));
    }

    if( ItemLastSel >= 0 ) {
        SetCurrent(Get(ItemLastSel));
        ItemLastSel = -1;
    }

    Display();
}

eOSState cFlatSetupMenu::ProcessKey(eKeys Key) {
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
            case kOk:
                return osBack;
            default:
                break;
        }
    }
    if( Key == kLeft || Key == kRight ) {
        const char* ItemText = Get(Current())->Text();
        if( strstr(ItemText, tr("Menuitem border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Menucont. border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Menucont. head border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Menuitem progress by decor-file?")) != NULL
        ) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// Replay Settings
cFlatSetupReplay::cFlatSetupReplay(cFlatConfig* data)  : cMenuSetupSubMenu(tr("Replay settings"), data) {
    Setup();
}

void cFlatSetupReplay::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Replay border by decor-file?"), &SetupConfig->decorBorderReplayByTheme));
    Add(new cMenuEditBoolItem(tr("Show resolution & aspect"), &SetupConfig->RecordingResolutionAspectShow));
    Add(new cMenuEditBoolItem(tr("Show format (hd/sd)"), &SetupConfig->RecordingFormatShow));
    Add(new cMenuEditBoolItem(tr("Simple aspect & format"), &SetupConfig->RecordingSimpleAspectFormat));

    if( SetupConfig->decorBorderReplayByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Replay border type"), Bordertypes[SetupConfig->decorBorderReplayTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Replay border size"), SetupConfig->decorBorderReplaySizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Replay border type"), &SetupConfig->decorBorderReplayTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Replay border size"), &SetupConfig->decorBorderReplaySizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Replay progress by decor-file?"), &SetupConfig->decorProgressReplayByTheme));
    if( SetupConfig->decorProgressReplayByTheme ) {
        cString size = cString::sprintf("%s:\t%d", tr("Replay progress size"), SetupConfig->decorProgressReplaySizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditIntItem(tr("Replay progress size"), &SetupConfig->decorProgressReplaySizeUser));
    }

    if( ItemLastSel >= 0 ) {
        SetCurrent(Get(ItemLastSel));
        ItemLastSel = -1;
    }

    Display();
}

eOSState cFlatSetupReplay::ProcessKey(eKeys Key) {
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
            case kOk:
                return osBack;
            default:
                break;
        }
    }
    if( Key == kLeft || Key == kRight ) {
        const char* ItemText = Get(Current())->Text();
        if( strstr(ItemText, tr("Replay border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Replay progress by decor-file?")) != NULL
        ) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// Volume Settings
cFlatSetupVolume::cFlatSetupVolume(cFlatConfig* data)  : cMenuSetupSubMenu(tr("Volume settings"), data) {
    Setup();
}

void cFlatSetupVolume::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Volume border by decor-file?"), &SetupConfig->decorBorderVolumeByTheme));
    if( SetupConfig->decorBorderVolumeByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Volume border type"), Bordertypes[SetupConfig->decorBorderVolumeTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Volume border size"), SetupConfig->decorBorderVolumeSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Volume border type"), &SetupConfig->decorBorderVolumeTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Volume border size"), &SetupConfig->decorBorderVolumeSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Volume progress by decor-file?"), &SetupConfig->decorProgressVolumeByTheme));
    if( SetupConfig->decorProgressVolumeByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Volume progress type"), Progresstypes[SetupConfig->decorProgressVolumeTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Volume progress size"), SetupConfig->decorProgressVolumeSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Volume progress type"), &SetupConfig->decorProgressVolumeTypeUser, Progresstypes.Size(), &Progresstypes[0]));
        Add(new cMenuEditIntItem(tr("Volume progress size"), &SetupConfig->decorProgressVolumeSizeUser));
    }

    if( ItemLastSel >= 0 ) {
        SetCurrent(Get(ItemLastSel));
        ItemLastSel = -1;
    }

    Display();
}

eOSState cFlatSetupVolume::ProcessKey(eKeys Key) {
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
            case kOk:
                return osBack;
            default:
                break;
        }
    }
    if( Key == kLeft || Key == kRight ) {
        const char* ItemText = Get(Current())->Text();
        if( strstr(ItemText, tr("Volume border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Volume progress by decor-file?")) != NULL
        ) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// Tracks Settings
cFlatSetupTracks::cFlatSetupTracks(cFlatConfig* data)  : cMenuSetupSubMenu(tr("Tracks settings"), data) {
    Setup();
}

void cFlatSetupTracks::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Tracks border by decor-file?"), &SetupConfig->decorBorderTrackByTheme));
    if( SetupConfig->decorBorderTrackByTheme ) {
        cString type = cString::sprintf("%s:\t%s", tr("Tracks border type"), Bordertypes[SetupConfig->decorBorderTrackTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Tracks border size"), SetupConfig->decorBorderTrackSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Tracks border type"), &SetupConfig->decorBorderTrackTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Tracks border size"), &SetupConfig->decorBorderTrackSizeUser));
    }

    if( ItemLastSel >= 0 ) {
        SetCurrent(Get(ItemLastSel));
        ItemLastSel = -1;
    }

    Display();
}

eOSState cFlatSetupTracks::ProcessKey(eKeys Key) {
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
            case kOk:
                return osBack;
            default:
                break;
        }
    }
    if( Key == kLeft || Key == kRight ) {
        const char* ItemText = Get(Current())->Text();
        if( strstr(ItemText, tr("Tracks border by decor-file?")) != NULL
        ) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// TVScraper Settings
cFlatSetupTvsraper::cFlatSetupTvsraper(cFlatConfig* data)  : cMenuSetupSubMenu(tr("Tracks settings"), data) {
    Setup();
}

void cFlatSetupTvsraper::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Channelinfo show poster?"), &SetupConfig->TVScraperChanInfoShowPoster));
    Add(new cMenuEditPrcItem(tr("Channelinfo poster size"), &SetupConfig->TVScraperChanInfoPosterSize, 0.004, 0.015, 2));
    Add(new cMenuEditBoolItem(tr("EPG info show poster?"), &SetupConfig->TVScraperEPGInfoShowPoster));
    Add(new cMenuEditBoolItem(tr("recording info show poster?"), &SetupConfig->TVScraperRecInfoShowPoster));
    Add(new cMenuEditBoolItem(tr("EPG info show actors?"), &SetupConfig->TVScraperEPGInfoShowActors));
    Add(new cMenuEditBoolItem(tr("recording info show actors?"), &SetupConfig->TVScraperRecInfoShowActors));

    if( ItemLastSel >= 0 ) {
        SetCurrent(Get(ItemLastSel));
        ItemLastSel = -1;
    }

    Display();
}

eOSState cFlatSetupTvsraper::ProcessKey(eKeys Key) {
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
            case kOk:
                return osBack;
            default:
                break;
        }
    }
    /*
    if( Key == kLeft || Key == kRight ) {
        const char* ItemText = Get(Current())->Text();
        if( strstr(ItemText, tr("Tracks border by decor-file?")) != NULL
        ) {
            ItemLastSel = Current();
            Setup();
        }
    }
    */
    return state;
}
