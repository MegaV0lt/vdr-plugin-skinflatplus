#include "setup.h"

cStringList Bordertypes;
cStringList DiskUsages;
cStringList Progresstypes;
cStringList MenuChannelViews;
cStringList MenuTimerViews;
cStringList MenuEventViews;
cStringList MenuRecordingViews;
cStringList DecorDescriptions;
cStringList ConfigFiles;
cStringList MessageColorPositions;
cStringList ScrollerTypes;
cStringList ScrollBarTypes;
cStringList DiskUsageFree;
cStringList ChannelTimeLefts;
cStringList WeatherTypes;

int ConfigFileSelection;

cFlatSetup::cFlatSetup(void) {
    SetupConfig = Config;
    ItemLastSel = -1;
    Setup();
}

cFlatSetup::~cFlatSetup() {

}

void cFlatSetup::Setup(void) {
    Clear();
    //Bordertypes.Clear();  // Double .Clear()?
    //Progresstypes.Clear();

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

    DiskUsageFree.Clear();
    DiskUsageFree.Append( strdup(tr("occupied")) );
    DiskUsageFree.Append( strdup(tr("free")) );

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

    MessageColorPositions.Clear();
    MessageColorPositions.Append( strdup(tr("vertical")) );
    MessageColorPositions.Append( strdup(tr("hoizontal")) );

    ScrollerTypes.Clear();
    ScrollerTypes.Append( strdup( tr("carriage return")) );
    ScrollerTypes.Append( strdup( tr("left-right-left")) );

    ScrollBarTypes.Clear();
    ScrollBarTypes.Append( strdup( tr("left line + rect bar")) );
    ScrollBarTypes.Append( strdup( tr("left line + round bar")) );
    ScrollBarTypes.Append( strdup( tr("middle line + rect bar")) );
    ScrollBarTypes.Append( strdup( tr("middle line + round bar")) );
    ScrollBarTypes.Append( strdup( tr("outline + rect bar")) );
    ScrollBarTypes.Append( strdup( tr("outline + round bar")) );
    ScrollBarTypes.Append( strdup( tr("rect bar")) );
    ScrollBarTypes.Append( strdup( tr("round bar")) );

    ChannelTimeLefts.Clear();
    ChannelTimeLefts.Append( strdup( tr("past / remaining")) );
    ChannelTimeLefts.Append( strdup( tr("past")) );
    ChannelTimeLefts.Append( strdup( tr("remaining")) );

    WeatherTypes.Clear();
    WeatherTypes.Append( strdup( tr("short")) );
    WeatherTypes.Append( strdup( tr("long")) );

    Add(new cOsdItem(tr("General settings"), osUnknown, true));
    Add(new cOsdItem(tr("Channelinfo settings"), osUnknown, true));
    Add(new cOsdItem(tr("Menu settings"), osUnknown, true));
    Add(new cOsdItem(tr("Replay settings"), osUnknown, true));
    Add(new cOsdItem(tr("Volume settings"), osUnknown, true));
    Add(new cOsdItem(tr("Tracks settings"), osUnknown, true));

    static cPlugin *pScraper = GetScraperPlugin();
    if (pScraper)
        Add(new cOsdItem(tr("TVScraper / scraper2vdr settings"), osUnknown, true));
    else
        Add(new cOsdItem(tr("TVScraper / scraper2vdr not installed"), osUnknown, false));

    Add(new cOsdItem(tr("Main menu widgets settings"), osUnknown, true));

    if (ItemLastSel >= 0) {
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
            if (strcmp(ItemText, tr("TVScraper / scraper2vdr settings")) == 0)
                state = AddSubMenu(new cFlatSetupTvsraper(&SetupConfig));
            if (strcmp(ItemText, tr("Main menu widgets settings")) == 0)
                state = AddSubMenu(new cFlatSetupMMWidget(&SetupConfig));
        }
    }
    return state;
}

void cFlatSetup::Store(void) {
    Config = SetupConfig;
    SetupStore("decorBorderChannelByTheme", Config.decorBorderChannelByTheme);
    SetupStore("decorBorderChannelTypeUser", Config.decorBorderChannelTypeUser);
    SetupStore("decorBorderChannelSizeUser", Config.decorBorderChannelSizeUser);
    SetupStore("decorBorderChannelEPGByTheme", Config.decorBorderChannelEPGByTheme);
    SetupStore("decorBorderChannelEPGTypeUser", Config.decorBorderChannelEPGTypeUser);
    SetupStore("decorBorderChannelEPGSizeUser", Config.decorBorderChannelEPGSizeUser);
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
    SetupStore("decorScrollBarByTheme", Config.decorScrollBarByTheme);
    SetupStore("decorScrollBarTypeUser", Config.decorScrollBarTypeUser);
    SetupStore("decorScrollBarSizeUser", Config.decorScrollBarSizeUser);

    SetupStore("ButtonsShowEmpty", Config.ButtonsShowEmpty);
    SetupStore("ChannelDvbapiInfoShow", Config.ChannelDvbapiInfoShow);
    SetupStore("ChannelFormatShow", Config.ChannelFormatShow);
    SetupStore("ChannelIconsShow", Config.ChannelIconsShow);
    SetupStore("ChannelResolutionAspectShow", Config.ChannelResolutionAspectShow);
    SetupStore("ChannelShowStartTime", Config.ChannelShowStartTime);
    SetupStore("ChannelSimpleAspectFormat", Config.ChannelSimpleAspectFormat);
    SetupStore("ChannelTimeLeft", Config.ChannelTimeLeft);
    SetupStore("ChannelWeatherShow", Config.ChannelWeatherShow);
    SetupStore("DecorIndex", Config.DecorIndex);
    SetupStore("DiskUsageFree", Config.DiskUsageFree);
    SetupStore("DiskUsageShort", Config.DiskUsageShort);
    SetupStore("DiskUsageShow", Config.DiskUsageShow);
    SetupStore("EpgAdditionalInfoShow", Config.EpgAdditionalInfoShow);
    SetupStore("EpgRerunsShow", Config.EpgRerunsShow);
    SetupStore("MainMenuItemScale", dtoa(Config.MainMenuItemScale));
    SetupStore("MainMenuWidgetActiveTimerHideEmpty", Config.MainMenuWidgetActiveTimerHideEmpty);
    SetupStore("MainMenuWidgetActiveTimerMaxCount", Config.MainMenuWidgetActiveTimerMaxCount);
    SetupStore("MainMenuWidgetActiveTimerPosition", Config.MainMenuWidgetActiveTimerPosition);
    SetupStore("MainMenuWidgetActiveTimerShowActive", Config.MainMenuWidgetActiveTimerShowActive);
    SetupStore("MainMenuWidgetActiveTimerShow", Config.MainMenuWidgetActiveTimerShow);
    SetupStore("MainMenuWidgetActiveTimerShowRecording", Config.MainMenuWidgetActiveTimerShowRecording);
    SetupStore("MainMenuWidgetActiveTimerShowRemoteActive", Config.MainMenuWidgetActiveTimerShowRemoteActive);
    SetupStore("MainMenuWidgetActiveTimerShowRemoteRecording", Config.MainMenuWidgetActiveTimerShowRemoteRecording);
    SetupStore("MainMenuWidgetActiveTimerShowRemoteRefreshTime", Config.MainMenuWidgetActiveTimerShowRemoteRefreshTime);
    SetupStore("MainMenuWidgetCommandPosition", Config.MainMenuWidgetCommandPosition);
    SetupStore("MainMenuWidgetCommandShow", Config.MainMenuWidgetCommandShow);
    SetupStore("MainMenuWidgetDVBDevicesDiscardNotUsed", Config.MainMenuWidgetDVBDevicesDiscardNotUsed);
    SetupStore("MainMenuWidgetDVBDevicesDiscardUnknown", Config.MainMenuWidgetDVBDevicesDiscardUnknown);
    SetupStore("MainMenuWidgetDVBDevicesNativeNumbering", Config.MainMenuWidgetDVBDevicesNativeNumbering);
    SetupStore("MainMenuWidgetDVBDevicesPosition", Config.MainMenuWidgetDVBDevicesPosition);
    SetupStore("MainMenuWidgetDVBDevicesShow", Config.MainMenuWidgetDVBDevicesShow);
    SetupStore("MainMenuWidgetLastRecMaxCount", Config.MainMenuWidgetLastRecMaxCount);
    SetupStore("MainMenuWidgetLastRecPosition", Config.MainMenuWidgetLastRecPosition);
    SetupStore("MainMenuWidgetLastRecShow", Config.MainMenuWidgetLastRecShow);
    SetupStore("MainMenuWidgetsEnable", Config.MainMenuWidgetsEnable);
    SetupStore("MainMenuWidgetSystemInfoPosition", Config.MainMenuWidgetSystemInfoPosition);
    SetupStore("MainMenuWidgetSystemInfoShow", Config.MainMenuWidgetSystemInfoShow);
    SetupStore("MainMenuWidgetSystemUpdatesHideIfZero", Config.MainMenuWidgetSystemUpdatesHideIfZero);
    SetupStore("MainMenuWidgetSystemUpdatesPosition", Config.MainMenuWidgetSystemUpdatesPosition);
    SetupStore("MainMenuWidgetSystemUpdatesShow", Config.MainMenuWidgetSystemUpdatesShow);
    SetupStore("MainMenuWidgetTemperaturesPosition", Config.MainMenuWidgetTemperaturesPosition);
    SetupStore("MainMenuWidgetTemperaturesShow", Config.MainMenuWidgetTemperaturesShow);
    SetupStore("MainMenuWidgetTimerConflictsHideEmpty", Config.MainMenuWidgetTimerConflictsHideEmpty);
    SetupStore("MainMenuWidgetTimerConflictsPosition", Config.MainMenuWidgetTimerConflictsPosition);
    SetupStore("MainMenuWidgetTimerConflictsShow", Config.MainMenuWidgetTimerConflictsShow);
    SetupStore("MainMenuWidgetWeatherDays", Config.MainMenuWidgetWeatherDays);
    SetupStore("MainMenuWidgetWeatherPosition", Config.MainMenuWidgetWeatherPosition);
    SetupStore("MainMenuWidgetWeatherShow", Config.MainMenuWidgetWeatherShow);
    SetupStore("MainMenuWidgetWeatherType", Config.MainMenuWidgetWeatherType);
    SetupStore("marginOsdHor", Config.marginOsdHor);
    SetupStore("marginOsdVer", Config.marginOsdVer);
    SetupStore("MenuChannelShowCount", Config.MenuChannelShowCount);
    SetupStore("MenuChannelView", Config.MenuChannelView);
    SetupStore("MenuContentFullSize", Config.MenuContentFullSize);
    SetupStore("MenuEventViewAllwaysWithDate", Config.MenuEventViewAllwaysWithDate);
    SetupStore("MenuEventView", Config.MenuEventView);
    SetupStore("MenuFullOsd", Config.MenuFullOsd);
    SetupStore("MenuItemIconsShow", Config.MenuItemIconsShow);
    SetupStore("MenuItemPadding", Config.MenuItemPadding);
    SetupStore("MenuItemParseTilde", Config.MenuItemParseTilde);
    SetupStore("MenuItemRecordingClearPercent", Config.MenuItemRecordingClearPercent);
    SetupStore("MenuItemRecordingDefaultOldDays", Config.MenuItemRecordingDefaultOldDays);
    SetupStore("MenuItemRecordingSeenThreshold", dtoa(Config.MenuItemRecordingSeenThreshold));
    SetupStore("MenuItemRecordingShowFolderDate", Config.MenuItemRecordingShowFolderDate);
    SetupStore("MenuItemRecordingShowRecordingErrors", Config.MenuItemRecordingShowRecordingErrors);
    SetupStore("MenuItemRecordingShowRecordingErrorsThreshold", Config.MenuItemRecordingShowRecordingErrorsThreshold);
    SetupStore("MenuRecordingShowCount", Config.MenuRecordingShowCount);
    SetupStore("MenuRecordingView", Config.MenuRecordingView);
    SetupStore("MenuTimerShowCount", Config.MenuTimerShowCount);
    SetupStore("MenuTimerView", Config.MenuTimerView);
    SetupStore("MessageColorPosition", Config.MessageColorPosition);
    SetupStore("MessageOffset", Config.MessageOffset);
    SetupStore("PlaybackWeatherShow", Config.PlaybackWeatherShow);
    SetupStore("RecordingAdditionalInfoShow", Config.RecordingAdditionalInfoShow);
    SetupStore("RecordingDimmOnPause", Config.RecordingDimmOnPause);
    SetupStore("RecordingDimmOnPauseDelay", Config.RecordingDimmOnPauseDelay);
    SetupStore("RecordingDimmOnPauseOpaque", Config.RecordingDimmOnPauseOpaque);
    SetupStore("RecordingFormatShow", Config.RecordingFormatShow);
    SetupStore("RecordingResolutionAspectShow", Config.RecordingResolutionAspectShow);
    SetupStore("RecordingSimpleAspectFormat", Config.RecordingSimpleAspectFormat);
    SetupStore("ScrollerDelay", Config.ScrollerDelay);
    SetupStore("ScrollerEnable", Config.ScrollerEnable);
    SetupStore("ScrollerStep", Config.ScrollerStep);
    SetupStore("ScrollerType", Config.ScrollerType);
    SetupStore("ShortRecordingCount", Config.ShortRecordingCount);
    SetupStore("SignalQualityShow", Config.SignalQualityShow);
    SetupStore("SignalQualityUseColors", Config.SignalQualityUseColors);
    SetupStore("TimeSecsScale", dtoa(Config.TimeSecsScale));
    SetupStore("TopBarFontClockScale", dtoa(Config.TopBarFontClockScale));
    SetupStore("TopBarFontSize", dtoa(Config.TopBarFontSize));
    SetupStore("TopBarHideClockText", Config.TopBarHideClockText);
    SetupStore("TopBarMenuIconShow", Config.TopBarMenuIconShow);
    SetupStore("TopBarRecConflictsHigh", Config.TopBarRecConflictsHigh);
    SetupStore("TopBarRecConflictsShow", Config.TopBarRecConflictsShow);
    SetupStore("TopBarRecordingShow", Config.TopBarRecordingShow);
    SetupStore("TVScraperChanInfoPosterSize", dtoa(Config.TVScraperChanInfoPosterSize));
    SetupStore("TVScraperChanInfoShowPoster", Config.TVScraperChanInfoShowPoster);
    SetupStore("TVScraperEPGInfoShowActors", Config.TVScraperEPGInfoShowActors);
    SetupStore("TVScraperEPGInfoShowPoster", Config.TVScraperEPGInfoShowPoster);
    SetupStore("TVScraperRecInfoShowActors", Config.TVScraperRecInfoShowActors);
    SetupStore("TVScraperRecInfoShowPoster", Config.TVScraperRecInfoShowPoster);
    SetupStore("TVScraperReplayInfoPosterSize", dtoa(Config.TVScraperReplayInfoPosterSize));
    SetupStore("TVScraperReplayInfoShowPoster", Config.TVScraperReplayInfoShowPoster);
    SetupStore("WeatherFontSize", dtoa(Config.WeatherFontSize));

    Config.Init();
}

void cFlatSetupGeneral::LoadConfigFile(void) {
    cString Filename = cString::sprintf("%s/configs/%s", cPlugin::ConfigDirectory(PLUGIN_NAME_I18N), ConfigFiles[ConfigFileSelection]);

    FILE *f = fopen(Filename, "r");
    if (f) {
        int line = 0;
        char *s;
        cReadLine ReadLine;
        while ((s = ReadLine.Read(f)) != NULL) {
            line++;
            char *p = strchr(s, '#');
            if (p)
                *p = 0;
            s = stripspace(skipspace(s));
            if (!isempty(s)) {
                char *n = s;
                char *v = strchr(s, '=');
                if (v) {
                    *v++ = 0;
                    n = stripspace(skipspace(n));
                    v = stripspace(skipspace(v));
                    bool success = SetupParse(n, v);
                    if (!success)
                        dsyslog("skin flatPlus: Failed to load config: %s with value: %s", n, v);
                }
            }
        }
    } else
        dsyslog("skin flatPlus: Failed to load config: file <%s> not found", *Filename);
    cString msg = cString::sprintf("%s %s %s", tr("configfile"), ConfigFiles[ConfigFileSelection], tr("loaded"));
    Skins.Message(mtInfo, msg);
}

bool cFlatSetupGeneral::SetupParse(const char *Name, const char *Value) {
    if      (strcmp(Name, "decorBorderChannelByTheme") == 0)            SetupConfig->decorBorderChannelByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelTypeUser") == 0)           SetupConfig->decorBorderChannelTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelSizeUser") == 0)           SetupConfig->decorBorderChannelSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelEPGByTheme") == 0)         SetupConfig->decorBorderChannelEPGByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelEPGTypeUser") == 0)        SetupConfig->decorBorderChannelEPGTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelEPGSizeUser") == 0)        SetupConfig->decorBorderChannelEPGSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderTopBarByTheme") == 0)             SetupConfig->decorBorderTopBarByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderTopBarTypeUser") == 0)            SetupConfig->decorBorderTopBarTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderTopBarSizeUser") == 0)            SetupConfig->decorBorderTopBarSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMessageByTheme") == 0)            SetupConfig->decorBorderMessageByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderMessageTypeUser") == 0)           SetupConfig->decorBorderMessageTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMessageSizeUser") == 0)           SetupConfig->decorBorderMessageSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderVolumeByTheme") == 0)             SetupConfig->decorBorderVolumeByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderVolumeTypeUser") == 0)            SetupConfig->decorBorderVolumeTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderVolumeSizeUser") == 0)            SetupConfig->decorBorderVolumeSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderTrackByTheme") == 0)              SetupConfig->decorBorderTrackByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderTrackTypeUser") == 0)             SetupConfig->decorBorderTrackTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderTrackSizeUser") == 0)             SetupConfig->decorBorderTrackSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderReplayByTheme") == 0)             SetupConfig->decorBorderReplayByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderReplayTypeUser") == 0)            SetupConfig->decorBorderReplayTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderReplaySizeUser") == 0)            SetupConfig->decorBorderReplaySizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuItemByTheme") == 0)           SetupConfig->decorBorderMenuItemByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuItemTypeUser") == 0)          SetupConfig->decorBorderMenuItemTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuItemSizeUser") == 0)          SetupConfig->decorBorderMenuItemSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentHeadByTheme") == 0)    SetupConfig->decorBorderMenuContentHeadByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentHeadTypeUser") == 0)   SetupConfig->decorBorderMenuContentHeadTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentHeadSizeUser") == 0)   SetupConfig->decorBorderMenuContentHeadSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentByTheme") == 0)        SetupConfig->decorBorderMenuContentByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentTypeUser") == 0)       SetupConfig->decorBorderMenuContentTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentSizeUser") == 0)       SetupConfig->decorBorderMenuContentSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderButtonByTheme") == 0)             SetupConfig->decorBorderButtonByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderButtonTypeUser") == 0)            SetupConfig->decorBorderButtonTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderButtonSizeUser") == 0)            SetupConfig->decorBorderButtonSizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressChannelByTheme") == 0)          SetupConfig->decorProgressChannelByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressChannelTypeUser") == 0)         SetupConfig->decorProgressChannelTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressChannelSizeUser") == 0)         SetupConfig->decorProgressChannelSizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressVolumeByTheme") == 0)           SetupConfig->decorProgressVolumeByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressVolumeTypeUser") == 0)          SetupConfig->decorProgressVolumeTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressVolumeSizeUser") == 0)          SetupConfig->decorProgressVolumeSizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressMenuItemByTheme") == 0)         SetupConfig->decorProgressMenuItemByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressMenuItemTypeUser") == 0)        SetupConfig->decorProgressMenuItemTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressMenuItemSizeUser") == 0)        SetupConfig->decorProgressMenuItemSizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressReplayByTheme") == 0)           SetupConfig->decorProgressReplayByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressReplayTypeUser") == 0)          SetupConfig->decorProgressReplayTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressReplaySizeUser") == 0)          SetupConfig->decorProgressReplaySizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressSignalByTheme") == 0)           SetupConfig->decorProgressSignalByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressSignalTypeUser") == 0)          SetupConfig->decorProgressSignalTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressSignalSizeUser") == 0)          SetupConfig->decorProgressSignalSizeUser = atoi(Value);
    else if (strcmp(Name, "decorScrollBarByTheme") == 0)                SetupConfig->decorScrollBarByTheme = atoi(Value);
    else if (strcmp(Name, "decorScrollBarTypeUser") == 0)               SetupConfig->decorScrollBarTypeUser = atoi(Value);
    else if (strcmp(Name, "decorScrollBarSizeUser") == 0)               SetupConfig->decorScrollBarSizeUser = atoi(Value);

    else if (strcmp(Name, "ButtonsShowEmpty") == 0)                     SetupConfig->ButtonsShowEmpty = atoi(Value);
    else if (strcmp(Name, "ChannelDvbapiInfoShow") == 0)         SetupConfig->ChannelDvbapiInfoShow = atoi(Value);
    else if (strcmp(Name, "ChannelFormatShow") == 0)                    SetupConfig->ChannelFormatShow = atoi(Value);
    else if (strcmp(Name, "ChannelIconsShow") == 0)                     SetupConfig->ChannelIconsShow = atoi(Value);
    else if (strcmp(Name, "ChannelResolutionAspectShow") == 0)          SetupConfig->ChannelResolutionAspectShow = atoi(Value);
    else if (strcmp(Name, "ChannelShowStartTime") == 0)                 SetupConfig->ChannelShowStartTime = atoi(Value);
    else if (strcmp(Name, "ChannelSimpleAspectFormat") == 0)            SetupConfig->ChannelSimpleAspectFormat = atoi(Value);
    else if (strcmp(Name, "ChannelTimeLeft") == 0)                      SetupConfig->ChannelTimeLeft = atoi(Value);
    else if (strcmp(Name, "ChannelWeatherShow") == 0)                   SetupConfig->ChannelWeatherShow = atoi(Value);
    else if (strcmp(Name, "DecorIndex") == 0)                           SetupConfig->DecorIndex = atoi(Value);
    else if (strcmp(Name, "DiskUsageFree") == 0)                        SetupConfig->DiskUsageFree = atoi(Value);
    else if (strcmp(Name, "DiskUsageShort") == 0)                       SetupConfig->DiskUsageShort = atoi(Value);
    else if (strcmp(Name, "DiskUsageShow") == 0)                        SetupConfig->DiskUsageShow = atoi(Value);
    else if (strcmp(Name, "EpgAdditionalInfoShow") == 0)                SetupConfig->EpgAdditionalInfoShow = atoi(Value);
    else if (strcmp(Name, "EpgRerunsShow") == 0)                        SetupConfig->EpgRerunsShow = atoi(Value);
    else if (strcmp(Name, "MainMenuItemScale") == 0)                    SetupConfig->MainMenuItemScale = atod(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerHideEmpty") == 0)   SetupConfig->MainMenuWidgetActiveTimerHideEmpty = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerMaxCount") == 0)    SetupConfig->MainMenuWidgetActiveTimerMaxCount = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerPosition") == 0)    SetupConfig->MainMenuWidgetActiveTimerPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShow") == 0)        SetupConfig->MainMenuWidgetActiveTimerShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowActive") == 0)  SetupConfig->MainMenuWidgetActiveTimerShowActive = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowRecording") == 0) SetupConfig->MainMenuWidgetActiveTimerShowRecording = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowRemoteActive") == 0)         SetupConfig->MainMenuWidgetActiveTimerShowRemoteActive = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowRemoteRecording") == 0)         SetupConfig->MainMenuWidgetActiveTimerShowRemoteRecording = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowRemoteRefreshTime") == 0)         SetupConfig->MainMenuWidgetActiveTimerShowRemoteRefreshTime = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetCommandPosition") == 0)        SetupConfig->MainMenuWidgetCommandPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetCommandShow") == 0)            SetupConfig->MainMenuWidgetCommandShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesDiscardNotUsed") == 0) SetupConfig->MainMenuWidgetDVBDevicesDiscardNotUsed = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesDiscardUnknown") == 0) SetupConfig->MainMenuWidgetDVBDevicesDiscardUnknown = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesNativeNumbering") == 0) SetupConfig->MainMenuWidgetDVBDevicesNativeNumbering = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesPosition") == 0)     SetupConfig->MainMenuWidgetDVBDevicesPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesShow") == 0)         SetupConfig->MainMenuWidgetDVBDevicesShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetLastRecMaxCount") == 0)        SetupConfig->MainMenuWidgetLastRecMaxCount = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetLastRecPosition") == 0)        SetupConfig->MainMenuWidgetLastRecPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetLastRecShow") == 0)            SetupConfig->MainMenuWidgetLastRecShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetsEnable") == 0)                SetupConfig->MainMenuWidgetsEnable = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemInfoPosition") == 0)     SetupConfig->MainMenuWidgetSystemInfoPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemInfoShow") == 0)         SetupConfig->MainMenuWidgetSystemInfoShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemUpdatesHideIfZero") == 0)SetupConfig->MainMenuWidgetSystemUpdatesHideIfZero = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemUpdatesPosition") == 0)  SetupConfig->MainMenuWidgetSystemUpdatesPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemUpdatesShow") == 0)      SetupConfig->MainMenuWidgetSystemUpdatesShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTemperaturesPosition") == 0)   SetupConfig->MainMenuWidgetTemperaturesPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTemperaturesShow") == 0)       SetupConfig->MainMenuWidgetTemperaturesShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTimerConflictsHideEmpty") == 0)SetupConfig->MainMenuWidgetTimerConflictsHideEmpty = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTimerConflictsPosition") == 0) SetupConfig->MainMenuWidgetTimerConflictsPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTimerConflictsShow") == 0)     SetupConfig->MainMenuWidgetTimerConflictsShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetWeatherDays") == 0)            SetupConfig->MainMenuWidgetWeatherDays = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetWeatherPosition") == 0)        SetupConfig->MainMenuWidgetWeatherPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetWeatherShow") == 0)            SetupConfig->MainMenuWidgetWeatherShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetWeatherType") == 0)            SetupConfig->MainMenuWidgetWeatherType = atoi(Value);
    else if (strcmp(Name, "marginOsdHor") == 0)                         SetupConfig->marginOsdHor = atoi(Value);
    else if (strcmp(Name, "marginOsdVer") == 0)                         SetupConfig->marginOsdVer = atoi(Value);
    else if (strcmp(Name, "MenuChannelShowCount") == 0)                 SetupConfig->MenuChannelShowCount = atoi(Value);
    else if (strcmp(Name, "MenuChannelView") == 0)                      SetupConfig->MenuChannelView = atoi(Value);
    else if (strcmp(Name, "MenuContentFullSize") == 0)                  SetupConfig->MenuContentFullSize = atoi(Value);
    else if (strcmp(Name, "MenuEventView") == 0)                        SetupConfig->MenuEventView = atoi(Value);
    else if (strcmp(Name, "MenuEventViewAllwaysWithDate") == 0)         SetupConfig->MenuEventViewAllwaysWithDate = atoi(Value);
    else if (strcmp(Name, "MenuFullOsd") == 0)                          SetupConfig->MenuFullOsd = atoi(Value);
    else if (strcmp(Name, "MenuItemIconsShow") == 0)                    SetupConfig->MenuItemIconsShow = atoi(Value);
    else if (strcmp(Name, "MenuItemPadding") == 0)                      SetupConfig->MenuItemPadding = atoi(Value);
    else if (strcmp(Name, "MenuItemParseTilde") == 0)                   SetupConfig->MenuItemParseTilde = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingClearPercent") == 0)        SetupConfig->MenuItemRecordingClearPercent = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingDefaultOldDays") == 0)         SetupConfig->MenuItemRecordingDefaultOldDays = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingSeenThreshold") == 0)        SetupConfig->MenuItemRecordingSeenThreshold = atod(Value);
    else if (strcmp(Name, "MenuItemRecordingShowFolderDate") == 0)      SetupConfig->MenuItemRecordingShowFolderDate = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingShowRecordingErrors") == 0)         SetupConfig->MenuItemRecordingShowRecordingErrors = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingShowRecordingErrorsThreshold") == 0)         SetupConfig->MenuItemRecordingShowRecordingErrorsThreshold = atoi(Value);
    else if (strcmp(Name, "MenuRecordingShowCount") == 0)               SetupConfig->MenuRecordingShowCount = atoi(Value);
    else if (strcmp(Name, "MenuRecordingView") == 0)                    SetupConfig->MenuRecordingView = atoi(Value);
    else if (strcmp(Name, "MenuTimerShowCount") == 0)                   SetupConfig->MenuTimerShowCount = atoi(Value);
    else if (strcmp(Name, "MenuTimerView") == 0)                        SetupConfig->MenuTimerView = atoi(Value);
    else if (strcmp(Name, "MessageColorPosition") == 0)                 SetupConfig->MessageColorPosition = atoi(Value);
    else if (strcmp(Name, "MessageOffset") == 0)                        SetupConfig->MessageOffset = atoi(Value);
    else if (strcmp(Name, "PlaybackWeatherShow") == 0)                  SetupConfig->PlaybackWeatherShow = atoi(Value);
    else if (strcmp(Name, "RecordingAdditionalInfoShow") == 0)          SetupConfig->RecordingAdditionalInfoShow = atoi(Value);
    else if (strcmp(Name, "RecordingDimmOnPause") == 0)                 SetupConfig->RecordingDimmOnPause = atoi(Value);
    else if (strcmp(Name, "RecordingDimmOnPauseDelay") == 0)            SetupConfig->RecordingDimmOnPauseDelay = atoi(Value);
    else if (strcmp(Name, "RecordingDimmOnPauseOpaque") == 0)           SetupConfig->RecordingDimmOnPauseOpaque = atoi(Value);
    else if (strcmp(Name, "RecordingFormatShow") == 0)                  SetupConfig->RecordingFormatShow = atoi(Value);
    else if (strcmp(Name, "RecordingResolutionAspectShow") == 0)        SetupConfig->RecordingResolutionAspectShow = atoi(Value);
    else if (strcmp(Name, "RecordingSimpleAspectFormat") == 0)          SetupConfig->RecordingSimpleAspectFormat = atoi(Value);
    else if (strcmp(Name, "ScrollerDelay") == 0)                        SetupConfig->ScrollerDelay = atoi(Value);
    else if (strcmp(Name, "ScrollerEnable") == 0)                       SetupConfig->ScrollerEnable = atoi(Value);
    else if (strcmp(Name, "ScrollerStep") == 0)                         SetupConfig->ScrollerStep = atoi(Value);
    else if (strcmp(Name, "ScrollerType") == 0)                         SetupConfig->ScrollerType = atoi(Value);
    else if (strcmp(Name, "ShortRecordingCount") == 0)                  SetupConfig->ShortRecordingCount = atoi(Value);
    else if (strcmp(Name, "SignalQualityShow") == 0)                    SetupConfig->SignalQualityShow = atoi(Value);
    else if (strcmp(Name, "SignalQualityUseColors") == 0)               SetupConfig->SignalQualityUseColors = atoi(Value);
    else if (strcmp(Name, "TimeSecsScale") == 0)                        SetupConfig->TimeSecsScale = atod(Value);
    else if (strcmp(Name, "TopBarFontClockScale") == 0)                 SetupConfig->TopBarFontClockScale = atod(Value);
    else if (strcmp(Name, "TopBarFontSize") == 0)                       SetupConfig->TopBarFontSize = atod(Value);
    else if (strcmp(Name, "TopBarHideClockText") == 0)                  SetupConfig->TopBarHideClockText = atoi(Value);
    else if (strcmp(Name, "TopBarMenuIconShow") == 0)                   SetupConfig->TopBarMenuIconShow = atoi(Value);
    else if (strcmp(Name, "TopBarRecConflictsHigh") == 0)               SetupConfig->TopBarRecConflictsHigh = atoi(Value);
    else if (strcmp(Name, "TopBarRecConflictsShow") == 0)               SetupConfig->TopBarRecConflictsShow = atoi(Value);
    else if (strcmp(Name, "TopBarRecordingShow") == 0)                  SetupConfig->TopBarRecordingShow = atoi(Value);
    else if (strcmp(Name, "TVScraperChanInfoPosterSize") == 0)          SetupConfig->TVScraperChanInfoPosterSize = atod(Value);
    else if (strcmp(Name, "TVScraperChanInfoShowPoster") == 0)          SetupConfig->TVScraperChanInfoShowPoster = atoi(Value);
    else if (strcmp(Name, "TVScraperEPGInfoShowActors") == 0)           SetupConfig->TVScraperEPGInfoShowActors = atoi(Value);
    else if (strcmp(Name, "TVScraperEPGInfoShowPoster") == 0)           SetupConfig->TVScraperEPGInfoShowPoster = atoi(Value);
    else if (strcmp(Name, "TVScraperRecInfoShowActors") == 0)           SetupConfig->TVScraperRecInfoShowActors = atoi(Value);
    else if (strcmp(Name, "TVScraperRecInfoShowPoster") == 0)           SetupConfig->TVScraperRecInfoShowPoster = atoi(Value);
    else if (strcmp(Name, "TVScraperReplayInfoPosterSize") == 0)        SetupConfig->TVScraperReplayInfoPosterSize = atod(Value);
    else if (strcmp(Name, "TVScraperReplayInfoShowPoster") == 0)        SetupConfig->TVScraperReplayInfoShowPoster = atoi(Value);
    else if (strcmp(Name, "WeatherFontSize") == 0)                      SetupConfig->WeatherFontSize = atod(Value);
    else return false;

    return true;
}

void cFlatSetupGeneral::SaveCurrentSettings(void) {
    time_t t;
    time(&t);
    tm *tm = localtime(&t);
    char time[32];
    strftime(time, sizeof(time)-1, "%d.%m.%Y_%H:%M", tm);
    cString File = time;
    cString Filename = cString::sprintf("%s/configs/%s", PLUGINRESOURCEPATH, *File);

    // if file exist remove it
    if (access(Filename, F_OK) != -1)
        remove(Filename);

    Config.Store("decorBorderChannelByTheme", SetupConfig->decorBorderChannelByTheme, *Filename);
    Config.Store("decorBorderChannelTypeUser", SetupConfig->decorBorderChannelTypeUser, *Filename);
    Config.Store("decorBorderChannelSizeUser", SetupConfig->decorBorderChannelSizeUser, *Filename);
    Config.Store("decorBorderChannelEPGByTheme", SetupConfig->decorBorderChannelEPGByTheme, *Filename);
    Config.Store("decorBorderChannelEPGTypeUser", SetupConfig->decorBorderChannelEPGTypeUser, *Filename);
    Config.Store("decorBorderChannelEPGSizeUser", SetupConfig->decorBorderChannelEPGSizeUser, *Filename);
    Config.Store("decorBorderTopBarByTheme", SetupConfig->decorBorderTopBarByTheme, *Filename);
    Config.Store("decorBorderTopBarTypeUser", SetupConfig->decorBorderTopBarTypeUser, *Filename);
    Config.Store("decorBorderTopBarSizeUser", SetupConfig->decorBorderTopBarSizeUser, *Filename);
    Config.Store("decorBorderMessageByTheme", SetupConfig->decorBorderMessageByTheme, *Filename);
    Config.Store("decorBorderMessageTypeUser", SetupConfig->decorBorderMessageTypeUser, *Filename);
    Config.Store("decorBorderMessageSizeUser", SetupConfig->decorBorderMessageSizeUser, *Filename);
    Config.Store("decorBorderVolumeByTheme", SetupConfig->decorBorderVolumeByTheme, *Filename);
    Config.Store("decorBorderVolumeTypeUser", SetupConfig->decorBorderVolumeTypeUser, *Filename);
    Config.Store("decorBorderVolumeSizeUser", SetupConfig->decorBorderVolumeSizeUser, *Filename);
    Config.Store("decorBorderTrackByTheme", SetupConfig->decorBorderTrackByTheme, *Filename);
    Config.Store("decorBorderTrackTypeUser", SetupConfig->decorBorderTrackTypeUser, *Filename);
    Config.Store("decorBorderTrackSizeUser", SetupConfig->decorBorderTrackSizeUser, *Filename);
    Config.Store("decorBorderReplayByTheme", SetupConfig->decorBorderReplayByTheme, *Filename);
    Config.Store("decorBorderReplayTypeUser", SetupConfig->decorBorderReplayTypeUser, *Filename);
    Config.Store("decorBorderReplaySizeUser", SetupConfig->decorBorderReplaySizeUser, *Filename);
    Config.Store("decorBorderMenuItemByTheme", SetupConfig->decorBorderMenuItemByTheme, *Filename);
    Config.Store("decorBorderMenuItemTypeUser", SetupConfig->decorBorderMenuItemTypeUser, *Filename);
    Config.Store("decorBorderMenuItemSizeUser", SetupConfig->decorBorderMenuItemSizeUser, *Filename);
    Config.Store("decorBorderMenuContentHeadByTheme", SetupConfig->decorBorderMenuContentHeadByTheme, *Filename);
    Config.Store("decorBorderMenuContentHeadTypeUser", SetupConfig->decorBorderMenuContentHeadTypeUser, *Filename);
    Config.Store("decorBorderMenuContentHeadSizeUser", SetupConfig->decorBorderMenuContentHeadSizeUser, *Filename);
    Config.Store("decorBorderMenuContentByTheme", SetupConfig->decorBorderMenuContentByTheme, *Filename);
    Config.Store("decorBorderMenuContentTypeUser", SetupConfig->decorBorderMenuContentTypeUser, *Filename);
    Config.Store("decorBorderMenuContentSizeUser", SetupConfig->decorBorderMenuContentSizeUser, *Filename);
    Config.Store("decorBorderButtonByTheme", SetupConfig->decorBorderButtonByTheme, *Filename);
    Config.Store("decorBorderButtonTypeUser", SetupConfig->decorBorderButtonTypeUser, *Filename);
    Config.Store("decorBorderButtonSizeUser", SetupConfig->decorBorderButtonSizeUser, *Filename);
    Config.Store("decorProgressChannelByTheme", SetupConfig->decorProgressChannelByTheme, *Filename);
    Config.Store("decorProgressChannelTypeUser", SetupConfig->decorProgressChannelTypeUser, *Filename);
    Config.Store("decorProgressChannelSizeUser", SetupConfig->decorProgressChannelSizeUser, *Filename);
    Config.Store("decorProgressVolumeByTheme", SetupConfig->decorProgressVolumeByTheme, *Filename);
    Config.Store("decorProgressVolumeTypeUser", SetupConfig->decorProgressVolumeTypeUser, *Filename);
    Config.Store("decorProgressVolumeSizeUser", SetupConfig->decorProgressVolumeSizeUser, *Filename);
    Config.Store("decorProgressMenuItemByTheme", SetupConfig->decorProgressMenuItemByTheme, *Filename);
    Config.Store("decorProgressMenuItemTypeUser", SetupConfig->decorProgressMenuItemTypeUser, *Filename);
    Config.Store("decorProgressMenuItemSizeUser", SetupConfig->decorProgressMenuItemSizeUser, *Filename);
    Config.Store("decorProgressReplayByTheme", SetupConfig->decorProgressReplayByTheme, *Filename);
    Config.Store("decorProgressReplayTypeUser", SetupConfig->decorProgressReplayTypeUser, *Filename);
    Config.Store("decorProgressReplaySizeUser", SetupConfig->decorProgressReplaySizeUser, *Filename);
    Config.Store("decorProgressSignalByTheme", SetupConfig->decorProgressSignalByTheme, *Filename);
    Config.Store("decorProgressSignalTypeUser", SetupConfig->decorProgressSignalTypeUser, *Filename);
    Config.Store("decorProgressSignalSizeUser", SetupConfig->decorProgressSignalSizeUser, *Filename);
    Config.Store("decorScrollBarByTheme", SetupConfig->decorScrollBarByTheme, *Filename);
    Config.Store("decorScrollBarTypeUser", SetupConfig->decorScrollBarTypeUser, *Filename);
    Config.Store("decorScrollBarSizeUser", SetupConfig->decorScrollBarSizeUser, *Filename);

    Config.Store("ButtonsShowEmpty", SetupConfig->ButtonsShowEmpty, *Filename);
    Config.Store("ChannelDvbapiInfoShow", SetupConfig->ChannelDvbapiInfoShow, *Filename);
    Config.Store("ChannelFormatShow", SetupConfig->ChannelFormatShow, *Filename);
    Config.Store("ChannelIconsShow", SetupConfig->ChannelIconsShow, *Filename);
    Config.Store("ChannelResolutionAspectShow", SetupConfig->ChannelResolutionAspectShow, *Filename);
    Config.Store("ChannelShowStartTime", SetupConfig->ChannelShowStartTime, *Filename);
    Config.Store("ChannelSimpleAspectFormat", SetupConfig->ChannelSimpleAspectFormat, *Filename);
    Config.Store("ChannelTimeLeft", SetupConfig->ChannelTimeLeft, *Filename);
    Config.Store("ChannelWeatherShow", SetupConfig->ChannelWeatherShow, *Filename);
    Config.Store("DecorIndex", SetupConfig->DecorIndex, *Filename);
    Config.Store("DiskUsageFree", SetupConfig->DiskUsageFree, *Filename);
    Config.Store("DiskUsageShort", SetupConfig->DiskUsageShort, *Filename);
    Config.Store("DiskUsageShow", SetupConfig->DiskUsageShow, *Filename);
    Config.Store("EpgAdditionalInfoShow", SetupConfig->EpgAdditionalInfoShow, *Filename);
    Config.Store("EpgRerunsShow", SetupConfig->EpgRerunsShow, *Filename);
    Config.Store("MainMenuItemScale", dtoa(Config.MainMenuItemScale), *Filename);
    Config.Store("MainMenuWidgetActiveTimerHideEmpty", SetupConfig->MainMenuWidgetActiveTimerHideEmpty, *Filename);
    Config.Store("MainMenuWidgetActiveTimerMaxCount", SetupConfig->MainMenuWidgetActiveTimerMaxCount, *Filename);
    Config.Store("MainMenuWidgetActiveTimerPosition", SetupConfig->MainMenuWidgetActiveTimerPosition, *Filename);
    Config.Store("MainMenuWidgetActiveTimerShowActive", SetupConfig->MainMenuWidgetActiveTimerShowActive, *Filename);
    Config.Store("MainMenuWidgetActiveTimerShowRecording", SetupConfig->MainMenuWidgetActiveTimerShowRecording, *Filename);
    Config.Store("MainMenuWidgetActiveTimerShowRemoteActive", SetupConfig->MainMenuWidgetActiveTimerShowRemoteActive, *Filename);
    Config.Store("MainMenuWidgetActiveTimerShowRemoteRecording", SetupConfig->MainMenuWidgetActiveTimerShowRemoteRecording, *Filename);
    Config.Store("MainMenuWidgetActiveTimerShowRemoteRefreshTime", SetupConfig->MainMenuWidgetActiveTimerShowRemoteRefreshTime, *Filename);
    Config.Store("MainMenuWidgetActiveTimerShow", SetupConfig->MainMenuWidgetActiveTimerShow, *Filename);
    Config.Store("MainMenuWidgetCommandPosition", SetupConfig->MainMenuWidgetCommandPosition, *Filename);
    Config.Store("MainMenuWidgetCommandShow", SetupConfig->MainMenuWidgetCommandShow, *Filename);
    Config.Store("MainMenuWidgetDVBDevicesDiscardNotUsed", SetupConfig->MainMenuWidgetDVBDevicesDiscardNotUsed, *Filename);
    Config.Store("MainMenuWidgetDVBDevicesDiscardUnknown", SetupConfig->MainMenuWidgetDVBDevicesDiscardUnknown, *Filename);
    Config.Store("MainMenuWidgetDVBDevicesNativeNumbering", SetupConfig->MainMenuWidgetDVBDevicesNativeNumbering, *Filename);
    Config.Store("MainMenuWidgetDVBDevicesPosition", SetupConfig->MainMenuWidgetDVBDevicesPosition, *Filename);
    Config.Store("MainMenuWidgetDVBDevicesShow", SetupConfig->MainMenuWidgetDVBDevicesShow, *Filename);
    Config.Store("MainMenuWidgetLastRecMaxCount", SetupConfig->MainMenuWidgetLastRecMaxCount, *Filename);
    Config.Store("MainMenuWidgetLastRecPosition", SetupConfig->MainMenuWidgetLastRecPosition, *Filename);
    Config.Store("MainMenuWidgetLastRecShow", SetupConfig->MainMenuWidgetLastRecShow, *Filename);
    Config.Store("MainMenuWidgetsEnable", SetupConfig->MainMenuWidgetsEnable, *Filename);
    Config.Store("MainMenuWidgetSystemInfoPosition", SetupConfig->MainMenuWidgetSystemInfoPosition, *Filename);
    Config.Store("MainMenuWidgetSystemInfoShow", SetupConfig->MainMenuWidgetSystemInfoShow, *Filename);
    Config.Store("MainMenuWidgetSystemUpdatesHideIfZero", SetupConfig->MainMenuWidgetSystemUpdatesHideIfZero, *Filename);
    Config.Store("MainMenuWidgetSystemUpdatesPosition", SetupConfig->MainMenuWidgetSystemUpdatesPosition, *Filename);
    Config.Store("MainMenuWidgetSystemUpdatesShow", SetupConfig->MainMenuWidgetSystemUpdatesShow, *Filename);
    Config.Store("MainMenuWidgetTemperaturesPosition", SetupConfig->MainMenuWidgetTemperaturesPosition, *Filename);
    Config.Store("MainMenuWidgetTemperaturesShow", SetupConfig->MainMenuWidgetTemperaturesShow, *Filename);
    Config.Store("MainMenuWidgetTimerConflictsHideEmpty", SetupConfig->MainMenuWidgetTimerConflictsHideEmpty, *Filename);
    Config.Store("MainMenuWidgetTimerConflictsPosition", SetupConfig->MainMenuWidgetTimerConflictsPosition, *Filename);
    Config.Store("MainMenuWidgetTimerConflictsShow", SetupConfig->MainMenuWidgetTimerConflictsShow, *Filename);
    Config.Store("MainMenuWidgetWeatherDays", SetupConfig->MainMenuWidgetWeatherDays, *Filename);
    Config.Store("MainMenuWidgetWeatherPosition", SetupConfig->MainMenuWidgetWeatherPosition, *Filename);
    Config.Store("MainMenuWidgetWeatherShow", SetupConfig->MainMenuWidgetWeatherShow, *Filename);
    Config.Store("MainMenuWidgetWeatherType", SetupConfig->MainMenuWidgetWeatherType, *Filename);
    Config.Store("marginOsdHor", SetupConfig->marginOsdHor, *Filename);
    Config.Store("marginOsdVer", SetupConfig->marginOsdVer, *Filename);
    Config.Store("MenuChannelShowCount", SetupConfig->MenuChannelShowCount, *Filename);
    Config.Store("MenuChannelView", SetupConfig->MenuChannelView, *Filename);
    Config.Store("MenuContentFullSize", SetupConfig->MenuContentFullSize, *Filename);
    Config.Store("MenuEventViewAllwaysWithDate", SetupConfig->MenuEventViewAllwaysWithDate, *Filename);
    Config.Store("MenuEventView", SetupConfig->MenuEventView, *Filename);
    Config.Store("MenuFullOsd", SetupConfig->MenuFullOsd, *Filename);
    Config.Store("MenuItemIconsShow", SetupConfig->MenuItemIconsShow, *Filename);
    Config.Store("MenuItemPadding", SetupConfig->MenuItemPadding, *Filename);
    Config.Store("MenuItemParseTilde", SetupConfig->MenuItemParseTilde, *Filename);
    Config.Store("MenuItemRecordingClearPercent", SetupConfig->MenuItemRecordingClearPercent, *Filename);
    Config.Store("MenuItemRecordingDefaultOldDays", SetupConfig->MenuItemRecordingDefaultOldDays, *Filename);
    Config.Store("MenuItemRecordingSeenThreshold", dtoa(Config.MenuItemRecordingSeenThreshold), *Filename);
    Config.Store("MenuItemRecordingShowFolderDate", SetupConfig->MenuItemRecordingShowFolderDate, *Filename);
    Config.Store("MenuItemRecordingShowRecordingErrors", SetupConfig->MenuItemRecordingShowRecordingErrors, *Filename);
    Config.Store("MenuItemRecordingShowRecordingErrorsThreshold", dtoa(Config.MenuItemRecordingShowRecordingErrorsThreshold), *Filename);
    Config.Store("MenuRecordingShowCount", SetupConfig->MenuRecordingShowCount, *Filename);
    Config.Store("MenuRecordingView", SetupConfig->MenuRecordingView, *Filename);
    Config.Store("MenuTimerShowCount", SetupConfig->MenuTimerShowCount, *Filename);
    Config.Store("MenuTimerView", SetupConfig->MenuTimerView, *Filename);
    Config.Store("MessageColorPosition", SetupConfig->MessageColorPosition, *Filename);
    Config.Store("MessageOffset", SetupConfig->MessageOffset, *Filename);
    Config.Store("PlaybackWeatherShow", SetupConfig->PlaybackWeatherShow, *Filename);
    Config.Store("RecordingAdditionalInfoShow", SetupConfig->RecordingAdditionalInfoShow, *Filename);
    Config.Store("RecordingDimmOnPauseDelay", SetupConfig->RecordingDimmOnPauseDelay, *Filename);
    Config.Store("RecordingDimmOnPauseOpaque", SetupConfig->RecordingDimmOnPauseOpaque, *Filename);
    Config.Store("RecordingDimmOnPause", SetupConfig->RecordingDimmOnPause, *Filename);
    Config.Store("RecordingFormatShow", SetupConfig->RecordingFormatShow, *Filename);
    Config.Store("RecordingResolutionAspectShow", SetupConfig->RecordingResolutionAspectShow, *Filename);
    Config.Store("RecordingSimpleAspectFormat", SetupConfig->RecordingSimpleAspectFormat, *Filename);
    Config.Store("ScrollerDelay", SetupConfig->ScrollerDelay, *Filename);
    Config.Store("ScrollerEnable", SetupConfig->ScrollerEnable, *Filename);
    Config.Store("ScrollerStep", SetupConfig->ScrollerStep, *Filename);
    Config.Store("ScrollerType", SetupConfig->ScrollerType, *Filename);
    Config.Store("ShortRecordingCount", SetupConfig->ShortRecordingCount, *Filename);
    Config.Store("SignalQualityShow", SetupConfig->SignalQualityShow, *Filename);
    Config.Store("SignalQualityUseColors", SetupConfig->SignalQualityUseColors, *Filename);
    Config.Store("TimeSecsScale", dtoa(Config.TimeSecsScale), *Filename);
    Config.Store("TopBarFontClockScale", dtoa(Config.TopBarFontClockScale), *Filename);
    Config.Store("TopBarFontSize", dtoa(Config.TopBarFontSize), *Filename);
    Config.Store("TopBarHideClockText", SetupConfig->TopBarHideClockText, *Filename);
    Config.Store("TopBarMenuIconShow", SetupConfig->TopBarMenuIconShow, *Filename);
    Config.Store("TopBarRecConflictsHigh", SetupConfig->TopBarRecConflictsHigh, *Filename);
    Config.Store("TopBarRecConflictsShow", SetupConfig->TopBarRecConflictsShow, *Filename);
    Config.Store("TopBarRecordingShow", SetupConfig->TopBarRecordingShow, *Filename);
    Config.Store("TVScraperChanInfoPosterSize", dtoa(Config.TVScraperChanInfoPosterSize), *Filename);
    Config.Store("TVScraperChanInfoShowPoster", SetupConfig->TVScraperChanInfoShowPoster, *Filename);
    Config.Store("TVScraperEPGInfoShowActors", SetupConfig->TVScraperEPGInfoShowActors, *Filename);
    Config.Store("TVScraperEPGInfoShowPoster", SetupConfig->TVScraperEPGInfoShowPoster, *Filename);
    Config.Store("TVScraperRecInfoShowActors", SetupConfig->TVScraperRecInfoShowActors, *Filename);
    Config.Store("TVScraperRecInfoShowPoster", SetupConfig->TVScraperRecInfoShowPoster, *Filename);
    Config.Store("TVScraperReplayInfoPosterSize", dtoa(Config.TVScraperReplayInfoPosterSize), *Filename);
    Config.Store("TVScraperReplayInfoShowPoster", SetupConfig->TVScraperReplayInfoShowPoster, *Filename);
    Config.Store("WeatherFontSize", dtoa(Config.WeatherFontSize), *Filename);

    cString msg = cString::sprintf("%s %s", tr("saved settings in file:"), *File);
    Skins.Message(mtInfo, msg);
}
//------------------------------------------------------------------------------------------------------------------

cMenuSetupSubMenu::cMenuSetupSubMenu(const char* Title, cFlatConfig* data) : cOsdMenu(Title, 35) {
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
cFlatSetupGeneral::cFlatSetupGeneral(cFlatConfig* data) : cMenuSetupSubMenu(tr("General settings"), data) {
    Setup();
}

void cFlatSetupGeneral::Setup(void) {
    Clear();

    DecorDescriptions.Clear();
    SetupConfig->DecorDescriptions( DecorDescriptions );
    if (SetupConfig->DecorIndex < 0 || SetupConfig->DecorIndex > DecorDescriptions.Size())
        SetupConfig->DecorIndex = 0;

    if (DecorDescriptions.Size() == 0) {
        esyslog("skin flatPlus: No decor files found, please check your installation!");
        Add(new cOsdItem(tr("no decorfiles found, check install"), osUnknown, false));
    } else
        Add(new cMenuEditStraItem(tr("Decorfile"), &SetupConfig->DecorIndex, DecorDescriptions.Size(), &DecorDescriptions[0]));

    ConfigFiles.Clear();
    SetupConfig->GetConfigFiles(ConfigFiles);
    for (int i = 0; i < ConfigFiles.Size(); i++)
        dsyslog("ConfigFile: %s", ConfigFiles[i]);

    ConfigFileSelection = 0;
    if (ConfigFiles.Size() == 0) {
        esyslog("skin flatPlus: No config files found, please check your installation!");
        Add(new cOsdItem(tr("no config-files found, check install"), osUnknown, false));
    } else
        Add(new cMenuEditStraItem(tr("Press ok to load config file"), &ConfigFileSelection, ConfigFiles.Size(), &ConfigFiles[0]));

    cString saveSettings = cString::sprintf("%s:\t%s", tr("Save current settings"), tr("press ok to save current settings"));
    Add(new cOsdItem(saveSettings, osUnknown, true));

    Add(new cMenuEditBoolItem(tr("Show empty color-buttons"), &SetupConfig->ButtonsShowEmpty));
    Add(new cMenuEditBoolItem(tr("Show TopBar menu icons"), &SetupConfig->TopBarMenuIconShow));
    Add(new cMenuEditStraItem(tr("Diskusage show"), &SetupConfig->DiskUsageShow, DiskUsages.Size(), &DiskUsages[0]));
    Add(new cMenuEditBoolItem(tr("Diskusage short display"), &SetupConfig->DiskUsageShort));
    Add(new cMenuEditStraItem(tr("Diskusage free/occupied"), &SetupConfig->DiskUsageFree, DiskUsageFree.Size(), &DiskUsageFree[0]));
    Add(new cMenuEditIntItem(tr("OSD vertical margin"), &SetupConfig->marginOsdVer));
    Add(new cMenuEditIntItem(tr("OSD horizontal margin"), &SetupConfig->marginOsdHor));
    Add(new cMenuEditPrcItem(tr("TopBar font size"), &SetupConfig->TopBarFontSize, 0.01, 0.2, 1));
    Add(new cMenuEditBoolItem(tr("TopBar show recording"), &SetupConfig->TopBarRecordingShow));
    Add(new cMenuEditBoolItem(tr("TopBar show conflicts"), &SetupConfig->TopBarRecConflictsShow));
    Add(new cMenuEditBoolItem(tr("TopBar hide clock text"), &SetupConfig->TopBarHideClockText));
    Add(new cMenuEditPrcItem(tr("TopBar clock font scale"), &SetupConfig->TopBarFontClockScale, 0.005, 0.02, 1));
    Add(new cMenuEditIntItem(tr("Conflicts min value for red"), &SetupConfig->TopBarRecConflictsHigh));
    Add(new cMenuEditIntItem(tr("Message bottom offset"), &SetupConfig->MessageOffset));
    Add(new cMenuEditStraItem(tr("Message color position"), &SetupConfig->MessageColorPosition, MessageColorPositions.Size(), &MessageColorPositions[0]));

    Add(new cMenuEditBoolItem(tr("Use Textscroller?"), &SetupConfig->ScrollerEnable));
    if (SetupConfig->ScrollerEnable) {
        Add(new cMenuEditIntItem(tr("Scroller step (in pixel)"), &SetupConfig->ScrollerStep));
        Add(new cMenuEditIntItem(tr("Scroller delay (in ms)"), &SetupConfig->ScrollerDelay));
        Add(new cMenuEditStraItem(tr("Scroller type"), &SetupConfig->ScrollerType, ScrollerTypes.Size(), &ScrollerTypes[0]));
    } else {
        cString step = cString::sprintf("%s:\t%d", tr("Scroller step (in pixel)"), SetupConfig->ScrollerStep);
        Add(new cOsdItem(step, osUnknown, false));
        cString delay = cString::sprintf("%s:\t%d", tr("Scroller delay (in ms)"), SetupConfig->ScrollerDelay);
        Add(new cOsdItem(delay, osUnknown, false));
        cString type = cString::sprintf("%s:\t%s", tr("Scroller type"), ScrollerTypes[SetupConfig->ScrollerType]);
        Add(new cOsdItem(type, osUnknown, false));
    }

    Add(new cMenuEditBoolItem(tr("TopBar border by decor-file?"), &SetupConfig->decorBorderTopBarByTheme));
    if (SetupConfig->decorBorderTopBarByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("TopBar border type"), Bordertypes[SetupConfig->decorBorderTopBarTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("TopBar border size"), SetupConfig->decorBorderTopBarSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("TopBar border type"), &SetupConfig->decorBorderTopBarTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("TopBar border size"), &SetupConfig->decorBorderTopBarSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Message border by decor-file?"), &SetupConfig->decorBorderMessageByTheme));
    if (SetupConfig->decorBorderMessageByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Message border type"), Bordertypes[SetupConfig->decorBorderMessageTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Message border size"), SetupConfig->decorBorderMessageSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Message border type"), &SetupConfig->decorBorderMessageTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Message border size"), &SetupConfig->decorBorderMessageSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Button border by decor-file?"), &SetupConfig->decorBorderButtonByTheme));
    if (SetupConfig->decorBorderButtonByTheme) {
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

    if (ItemLastSel >= 0) {
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
            {
                const char* ItemText = Get(Current())->Text();
                if (strstr(ItemText, tr("Save current settings")) != NULL) {
                    SaveCurrentSettings();
                    return osUnknown;
                } else if (strstr(ItemText, tr("Press ok to load config file")) != NULL) {
                    LoadConfigFile();
                    return osBack;
                } else {
                    return osBack;
                }
            }
            default:
                break;
        }
    }
    if (Key == kLeft || Key == kRight) {
        const char* ItemText = Get(Current())->Text();
        if (strstr(ItemText, tr("TopBar border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Message border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Use Textscroller?")) != NULL ||
            strstr(ItemText, tr("Button border by decor-file?")) != NULL) {
              ItemLastSel = Current();
              Setup();
        }
    }
    return state;
}

// Channel Info Settings
cFlatSetupChannelInfo::cFlatSetupChannelInfo(cFlatConfig* data) : cMenuSetupSubMenu(tr("Channelinfo settings"), data) {
    Setup();
}

void cFlatSetupChannelInfo::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Show Channelinfo icons"), &SetupConfig->ChannelIconsShow));
    Add(new cMenuEditBoolItem(tr("Show event start time left"), &SetupConfig->ChannelShowStartTime));
    Add(new cMenuEditBoolItem(tr("Show signal quality"), &SetupConfig->SignalQualityShow));
    Add(new cMenuEditBoolItem(tr("Show weather widget"), &SetupConfig->ChannelWeatherShow));
    Add(new cMenuEditPrcItem(tr("Weather widget font size"), &SetupConfig->WeatherFontSize, 0.01, 0.2, 1));
    Add(new cMenuEditBoolItem(tr("Colors for signal quality"), &SetupConfig->SignalQualityUseColors));
    Add(new cMenuEditBoolItem(tr("Show resolution & aspect"), &SetupConfig->ChannelResolutionAspectShow));
    Add(new cMenuEditBoolItem(tr("Show format (hd/sd)"), &SetupConfig->ChannelFormatShow));
    Add(new cMenuEditBoolItem(tr("Simple aspect & format"), &SetupConfig->ChannelSimpleAspectFormat));
    static cPlugin *pDVBApi = cPluginManager::GetPlugin("dvbapi");
    if (!pDVBApi) {
        Add(new cOsdItem(tr("dvbapi plugin not installed"), osUnknown, false));
    } else {
        Add(new cMenuEditBoolItem(tr("Show dvbapi informations"), &SetupConfig->ChannelDvbapiInfoShow));
    }
    Add(new cMenuEditStraItem(tr("program past/remaining time format"), &SetupConfig->ChannelTimeLeft, ChannelTimeLefts.Size(), &ChannelTimeLefts[0]));

    Add(new cMenuEditBoolItem(tr("Channelinfo border by decor-file?"), &SetupConfig->decorBorderChannelByTheme));
    if (SetupConfig->decorBorderChannelByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Channelinfo border type"), Bordertypes[SetupConfig->decorBorderChannelTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Channelinfo border size"), SetupConfig->decorBorderChannelSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Channelinfo border type"), &SetupConfig->decorBorderChannelTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Channelinfo border size"), &SetupConfig->decorBorderChannelSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Channelinfo EPG border by decor-file?"), &SetupConfig->decorBorderChannelEPGByTheme));
    if (SetupConfig->decorBorderChannelEPGByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Channelinfo EPG border type"), Bordertypes[SetupConfig->decorBorderChannelEPGTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Channelinfo EPG border size"), SetupConfig->decorBorderChannelEPGSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Channelinfo EPG border type"), &SetupConfig->decorBorderChannelEPGTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Channelinfo EPG border size"), &SetupConfig->decorBorderChannelEPGSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Channelinfo progress by decor-file?"), &SetupConfig->decorProgressChannelByTheme));
    if (SetupConfig->decorProgressChannelByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Channelinfo progress type"), Progresstypes[SetupConfig->decorProgressChannelTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Channelinfo progress size"), SetupConfig->decorProgressChannelSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Channelinfo progress type"), &SetupConfig->decorProgressChannelTypeUser, Progresstypes.Size(), &Progresstypes[0]));
        Add(new cMenuEditIntItem(tr("Channelinfo progress size"), &SetupConfig->decorProgressChannelSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Signalquality progress by decor-file?"), &SetupConfig->decorProgressSignalByTheme));
    if (SetupConfig->decorProgressSignalByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Signalquality progress type"), Progresstypes[SetupConfig->decorProgressSignalTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Signalquality progress size"), SetupConfig->decorProgressSignalSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Signalquality progress type"), &SetupConfig->decorProgressSignalTypeUser, Progresstypes.Size(), &Progresstypes[0]));
        Add(new cMenuEditIntItem(tr("Signalquality progress size"), &SetupConfig->decorProgressSignalSizeUser));
    }

    if (ItemLastSel >= 0) {
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
    if (Key == kLeft || Key == kRight) {
        const char* ItemText = Get(Current())->Text();
        if (strstr(ItemText, tr("Channelinfo border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Channelinfo EPG border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Channelinfo progress by decor-file?")) != NULL ||
            strstr(ItemText, tr("Signalquality progress by decor-file?")) != NULL) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// Menu Settings
cFlatSetupMenu::cFlatSetupMenu(cFlatConfig* data) : cMenuSetupSubMenu(tr("Menu settings"), data) {
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
    Add(new cMenuEditBoolItem(tr("Menu draw background over the entire height"), &SetupConfig->MenuFullOsd));

    Add(new cMenuEditBoolItem(tr("Recording menu show recording count in title"), &SetupConfig->MenuRecordingShowCount));
    Add(new cMenuEditPrcItem(tr("Recording menu recording seen threshold"), &SetupConfig->MenuItemRecordingSeenThreshold, 0.008, 0.01, 2));
    Add(new cMenuEditIntItem(tr("Recording menu default value - old folder in days"), &SetupConfig->MenuItemRecordingDefaultOldDays, -1));
    Add(new cMenuEditBoolItem(tr("Recording menu show recerrors icon"), &SetupConfig->MenuItemRecordingShowRecordingErrors));
    Add(new cMenuEditIntItem(tr("Recording recerrors icon threshold"), &SetupConfig->MenuItemRecordingShowRecordingErrorsThreshold, 1, 999999));

    Add(new cMenuEditBoolItem(tr("Timer menu show timer count in title"), &SetupConfig->MenuTimerShowCount));
    Add(new cMenuEditBoolItem(tr("Channel menu show channel count in title"), &SetupConfig->MenuChannelShowCount));

    Add(new cMenuEditStraItem(tr("Menu channel view"), &SetupConfig->MenuChannelView, MenuChannelViews.Size(), &MenuChannelViews[0]));
    Add(new cMenuEditStraItem(tr("Menu timer view"), &SetupConfig->MenuTimerView, MenuTimerViews.Size(), &MenuTimerViews[0]));
    Add(new cMenuEditStraItem(tr("Menu event view"), &SetupConfig->MenuEventView, MenuEventViews.Size(), &MenuEventViews[0]));
    Add(new cMenuEditBoolItem(tr("Menu event view allways with date"), &SetupConfig->MenuEventViewAllwaysWithDate));
    Add(new cMenuEditStraItem(tr("Menu recording view"), &SetupConfig->MenuRecordingView, MenuRecordingViews.Size(), &MenuRecordingViews[0]));

    Add(new cMenuEditBoolItem(tr("Scrollbar by decor-file?"), &SetupConfig->decorScrollBarByTheme));
    if (SetupConfig->decorScrollBarByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Scrollbar type"), ScrollBarTypes[SetupConfig->decorScrollBarTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Scrollbar size"), SetupConfig->decorScrollBarSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Scrollbar type"), &SetupConfig->decorScrollBarTypeUser, ScrollBarTypes.Size(), &ScrollBarTypes[0]));
        Add(new cMenuEditIntItem(tr("Scrollbar size"), &SetupConfig->decorScrollBarSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Menuitem border by decor-file?"), &SetupConfig->decorBorderMenuItemByTheme));
    if (SetupConfig->decorBorderMenuItemByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Menuitem border type"), Bordertypes[SetupConfig->decorBorderMenuItemTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Menuitem border size"), SetupConfig->decorBorderMenuItemSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Menuitem border type"), &SetupConfig->decorBorderMenuItemTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Menuitem border size"), &SetupConfig->decorBorderMenuItemSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Menucont. border by decor-file?"), &SetupConfig->decorBorderMenuContentByTheme));
    if (SetupConfig->decorBorderMenuContentByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Menucont. border type"), Bordertypes[SetupConfig->decorBorderMenuContentTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Menucont. border size"), SetupConfig->decorBorderMenuContentSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Menucont. border type"), &SetupConfig->decorBorderMenuContentTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Menucont. border size"), &SetupConfig->decorBorderMenuContentSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Menucont. head border by decor-file?"), &SetupConfig->decorBorderMenuContentHeadByTheme));
    if (SetupConfig->decorBorderMenuContentHeadByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Menucont. head border type"), Bordertypes[SetupConfig->decorBorderMenuContentHeadTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Menucont. head border size"), SetupConfig->decorBorderMenuContentHeadSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Menucont. head border type"), &SetupConfig->decorBorderMenuContentHeadTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Menucont. head border size"), &SetupConfig->decorBorderMenuContentHeadSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Menuitem progress by decor-file?"), &SetupConfig->decorProgressMenuItemByTheme));
    if (SetupConfig->decorProgressMenuItemByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Menuitem progress type"), Progresstypes[SetupConfig->decorProgressMenuItemTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Menuitem progress size"), SetupConfig->decorProgressMenuItemSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Menuitem progress type"), &SetupConfig->decorProgressMenuItemTypeUser, Progresstypes.Size(), &Progresstypes[0]));
        Add(new cMenuEditIntItem(tr("Menuitem progress size"), &SetupConfig->decorProgressMenuItemSizeUser));
    }

    if (ItemLastSel >= 0) {
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
    if (Key == kLeft || Key == kRight) {
        const char* ItemText = Get(Current())->Text();
        if (strstr(ItemText, tr("Menuitem border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Menucont. border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Menucont. head border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Menuitem progress by decor-file?")) != NULL ||
            strstr(ItemText, tr("Scrollbar by decor-file?")) != NULL) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// Replay Settings
cFlatSetupReplay::cFlatSetupReplay(cFlatConfig* data) : cMenuSetupSubMenu(tr("Replay settings"), data) {
    Setup();
}

void cFlatSetupReplay::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Show resolution & aspect"), &SetupConfig->RecordingResolutionAspectShow));
    Add(new cMenuEditBoolItem(tr("Show format (hd/sd)"), &SetupConfig->RecordingFormatShow));
    Add(new cMenuEditBoolItem(tr("Simple aspect & format"), &SetupConfig->RecordingSimpleAspectFormat));
    Add(new cMenuEditPrcItem(tr("Time seconds font scale"), &SetupConfig->TimeSecsScale, 0.003, 0.01, 1));
    Add(new cMenuEditBoolItem(tr("Show weather widget"), &SetupConfig->PlaybackWeatherShow));

    Add(new cMenuEditBoolItem(tr("Dimm on pause?"), &SetupConfig->RecordingDimmOnPause));
    if (SetupConfig->RecordingDimmOnPause) {
        Add(new cMenuEditIntItem(tr("Dimm on pause delay"), &SetupConfig->RecordingDimmOnPauseDelay));
        Add(new cMenuEditIntItem(tr("Dimm on pause opaque"), &SetupConfig->RecordingDimmOnPauseOpaque));
    } else {
        cString type = cString::sprintf("%s:\t%d", tr("Dimm on pause delay"), SetupConfig->RecordingDimmOnPauseDelay);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Dimm on pause opaque"), SetupConfig->RecordingDimmOnPauseOpaque);
        Add(new cOsdItem(size, osUnknown, false));
    }
    Add(new cMenuEditBoolItem(tr("Replay border by decor-file?"), &SetupConfig->decorBorderReplayByTheme));
    if (SetupConfig->decorBorderReplayByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Replay border type"), Bordertypes[SetupConfig->decorBorderReplayTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Replay border size"), SetupConfig->decorBorderReplaySizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Replay border type"), &SetupConfig->decorBorderReplayTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Replay border size"), &SetupConfig->decorBorderReplaySizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Replay progress by decor-file?"), &SetupConfig->decorProgressReplayByTheme));
    if (SetupConfig->decorProgressReplayByTheme) {
        cString size = cString::sprintf("%s:\t%d", tr("Replay progress size"), SetupConfig->decorProgressReplaySizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditIntItem(tr("Replay progress size"), &SetupConfig->decorProgressReplaySizeUser));
    }

    if (ItemLastSel >= 0) {
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
        if (strstr(ItemText, tr("Replay border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Replay progress by decor-file?")) != NULL ||
            strstr(ItemText, tr("Dimm on pause?")) != NULL) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// Volume Settings
cFlatSetupVolume::cFlatSetupVolume(cFlatConfig* data) : cMenuSetupSubMenu(tr("Volume settings"), data) {
    Setup();
}

void cFlatSetupVolume::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Volume border by decor-file?"), &SetupConfig->decorBorderVolumeByTheme));
    if (SetupConfig->decorBorderVolumeByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Volume border type"), Bordertypes[SetupConfig->decorBorderVolumeTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Volume border size"), SetupConfig->decorBorderVolumeSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Volume border type"), &SetupConfig->decorBorderVolumeTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Volume border size"), &SetupConfig->decorBorderVolumeSizeUser));
    }

    Add(new cMenuEditBoolItem(tr("Volume progress by decor-file?"), &SetupConfig->decorProgressVolumeByTheme));
    if (SetupConfig->decorProgressVolumeByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Volume progress type"), Progresstypes[SetupConfig->decorProgressVolumeTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Volume progress size"), SetupConfig->decorProgressVolumeSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Volume progress type"), &SetupConfig->decorProgressVolumeTypeUser, Progresstypes.Size(), &Progresstypes[0]));
        Add(new cMenuEditIntItem(tr("Volume progress size"), &SetupConfig->decorProgressVolumeSizeUser));
    }

    if (ItemLastSel >= 0) {
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
    if (Key == kLeft || Key == kRight) {
        const char* ItemText = Get(Current())->Text();
        if (strstr(ItemText, tr("Volume border by decor-file?")) != NULL ||
            strstr(ItemText, tr("Volume progress by decor-file?")) != NULL) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// Tracks Settings
cFlatSetupTracks::cFlatSetupTracks(cFlatConfig* data) : cMenuSetupSubMenu(tr("Tracks settings"), data) {
    Setup();
}

void cFlatSetupTracks::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Tracks border by decor-file?"), &SetupConfig->decorBorderTrackByTheme));
    if (SetupConfig->decorBorderTrackByTheme) {
        cString type = cString::sprintf("%s:\t%s", tr("Tracks border type"), Bordertypes[SetupConfig->decorBorderTrackTypeTheme]);
        Add(new cOsdItem(type, osUnknown, false));
        cString size = cString::sprintf("%s:\t%d", tr("Tracks border size"), SetupConfig->decorBorderTrackSizeTheme);
        Add(new cOsdItem(size, osUnknown, false));
    } else {
        Add(new cMenuEditStraItem(tr("Tracks border type"), &SetupConfig->decorBorderTrackTypeUser, Bordertypes.Size(), &Bordertypes[0]));
        Add(new cMenuEditIntItem(tr("Tracks border size"), &SetupConfig->decorBorderTrackSizeUser));
    }

    if (ItemLastSel >= 0) {
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
    if (Key == kLeft || Key == kRight) {
        const char* ItemText = Get(Current())->Text();
        if (strstr(ItemText, tr("Tracks border by decor-file?")) != NULL) {
            ItemLastSel = Current();
            Setup();
        }
    }
    return state;
}

// TVScraper Settings
cFlatSetupTvsraper::cFlatSetupTvsraper(cFlatConfig* data) : cMenuSetupSubMenu(tr("TVScraper settings"), data) {
    Setup();
}

void cFlatSetupTvsraper::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Channelinfo show poster?"), &SetupConfig->TVScraperChanInfoShowPoster));
    Add(new cMenuEditPrcItem(tr("Channelinfo poster size"), &SetupConfig->TVScraperChanInfoPosterSize, 0.004, 0.015, 2));
    Add(new cMenuEditBoolItem(tr("Replayinfo show poster?"), &SetupConfig->TVScraperReplayInfoShowPoster));
    Add(new cMenuEditPrcItem(tr("Replayinfo poster size"), &SetupConfig->TVScraperReplayInfoPosterSize, 0.004, 0.015, 2));
    Add(new cMenuEditBoolItem(tr("EPG info show poster?"), &SetupConfig->TVScraperEPGInfoShowPoster));
    Add(new cMenuEditBoolItem(tr("EPG info show actors?"), &SetupConfig->TVScraperEPGInfoShowActors));
    Add(new cMenuEditBoolItem(tr("recording info show poster?"), &SetupConfig->TVScraperRecInfoShowPoster));
    Add(new cMenuEditBoolItem(tr("recording info show actors?"), &SetupConfig->TVScraperRecInfoShowActors));

    if (ItemLastSel >= 0) {
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
    return state;
}

// Main Menu Widgets Settings
cFlatSetupMMWidget::cFlatSetupMMWidget(cFlatConfig* data) : cMenuSetupSubMenu(tr("Main menu widgets settings"), data) {
    Setup();
}

void cFlatSetupMMWidget::Setup(void) {
    Clear();

    Add(new cMenuEditBoolItem(tr("Enable main menu widgets"), &SetupConfig->MainMenuWidgetsEnable));
    if (SetupConfig->MainMenuWidgetsEnable) {
        Add(new cMenuEditPrcItem(tr("Main menu item scale"), &SetupConfig->MainMenuItemScale, 0.3, 0.7, 0));

        Add(new cOsdItem(tr("Widget weather"), osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Widget weather: enable"), &SetupConfig->MainMenuWidgetWeatherShow));
        if (SetupConfig->MainMenuWidgetWeatherShow) {
            Add(new cMenuEditIntItem(tr("Widget weather: position"), &SetupConfig->MainMenuWidgetWeatherPosition));
            Add(new cMenuEditIntItem(tr("Widget weather: show days"), &SetupConfig->MainMenuWidgetWeatherDays));
            Add(new cMenuEditStraItem(tr("Widget weather: type"), &SetupConfig->MainMenuWidgetWeatherType, WeatherTypes.Size(), &WeatherTypes[0]));
        }

        Add(new cOsdItem(tr("Widget DVB devices"), osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Widget DVB devices: enable"), &SetupConfig->MainMenuWidgetDVBDevicesShow));
        if (SetupConfig->MainMenuWidgetDVBDevicesShow) {
            Add(new cMenuEditIntItem(tr("Widget DVB devices: position"), &SetupConfig->MainMenuWidgetDVBDevicesPosition));
            Add(new cMenuEditBoolItem(tr("Widget DVB devices: don't show unknown"), &SetupConfig->MainMenuWidgetDVBDevicesDiscardUnknown));
            Add(new cMenuEditBoolItem(tr("Widget DVB devices: don't show not used"), &SetupConfig->MainMenuWidgetDVBDevicesDiscardNotUsed));
            Add(new cMenuEditBoolItem(tr("Widget DVB devices: native numbering (0..)"), &SetupConfig->MainMenuWidgetDVBDevicesNativeNumbering));
        }

        Add(new cOsdItem(tr("Widget timer"), osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Widget timer: enable"), &SetupConfig->MainMenuWidgetActiveTimerShow));
        if (SetupConfig->MainMenuWidgetActiveTimerShow) {
            Add(new cMenuEditIntItem(tr("Widget timer: position"), &SetupConfig->MainMenuWidgetActiveTimerPosition));
            Add(new cMenuEditBoolItem(tr("Widget timer: show recording timer"), &SetupConfig->MainMenuWidgetActiveTimerShowRecording));
            Add(new cMenuEditBoolItem(tr("Widget timer: show active timer"), &SetupConfig->MainMenuWidgetActiveTimerShowActive));
            Add(new cMenuEditBoolItem(tr("Widget timer: show remote recording timer"), &SetupConfig->MainMenuWidgetActiveTimerShowRemoteRecording));
            Add(new cMenuEditBoolItem(tr("Widget timer: show remote active timer"), &SetupConfig->MainMenuWidgetActiveTimerShowRemoteActive));
            Add(new cMenuEditIntItem(tr("Widget timer: max show"), &SetupConfig->MainMenuWidgetActiveTimerMaxCount));
            Add(new cMenuEditBoolItem(tr("Widget timer: hide if empty"), &SetupConfig->MainMenuWidgetActiveTimerHideEmpty));
        }

        Add(new cOsdItem(tr("Widget last recordings"), osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Widget last recordings: enable"), &SetupConfig->MainMenuWidgetLastRecShow));
        if (SetupConfig->MainMenuWidgetLastRecShow) {
            Add(new cMenuEditIntItem(tr("Widget last recordings: position"), &SetupConfig->MainMenuWidgetLastRecPosition));
            Add(new cMenuEditIntItem(tr("Widget last recordings: max show"), &SetupConfig->MainMenuWidgetLastRecMaxCount));
        }

        Add(new cOsdItem(tr("Widget timer conflicts"), osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Widget timer conflicts: enable"), &SetupConfig->MainMenuWidgetTimerConflictsShow));
        if (SetupConfig->MainMenuWidgetTimerConflictsShow) {
            Add(new cMenuEditIntItem(tr("Widget timer conflicts: position"), &SetupConfig->MainMenuWidgetTimerConflictsPosition));
            Add(new cMenuEditBoolItem(tr("Widget timer conflicts: hide if empty"), &SetupConfig->MainMenuWidgetTimerConflictsHideEmpty));
        }

        Add(new cOsdItem(tr("Widget system information"), osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Widget system information: enable"), &SetupConfig->MainMenuWidgetSystemInfoShow));
        if (SetupConfig->MainMenuWidgetSystemInfoShow) {
            Add(new cMenuEditIntItem(tr("Widget system information: position"), &SetupConfig->MainMenuWidgetSystemInfoPosition));
        }

        Add(new cOsdItem(tr("Widget system updates"), osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Widget system updates: enable"), &SetupConfig->MainMenuWidgetSystemUpdatesShow));
        if (SetupConfig->MainMenuWidgetSystemUpdatesShow) {
            Add(new cMenuEditIntItem(tr("Widget system updates: position"), &SetupConfig->MainMenuWidgetSystemUpdatesPosition));
            Add(new cMenuEditBoolItem(tr("Widget system updates: hide if empty"), &SetupConfig->MainMenuWidgetSystemUpdatesHideIfZero));
        }

        Add(new cOsdItem(tr("Widget temperatures"), osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Widget temperatures: enable"), &SetupConfig->MainMenuWidgetTemperaturesShow));
        if (SetupConfig->MainMenuWidgetTemperaturesShow) {
            Add(new cMenuEditIntItem(tr("Widget temperatures: position"), &SetupConfig->MainMenuWidgetTemperaturesPosition));
        }

        Add(new cOsdItem(tr("Widget custom commands"), osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Widget custom commands: enable"), &SetupConfig->MainMenuWidgetCommandShow));
        if (SetupConfig->MainMenuWidgetCommandShow) {
            Add(new cMenuEditIntItem(tr("Widget custom commands: position"), &SetupConfig->MainMenuWidgetCommandPosition));
        }
    }

    if (ItemLastSel >= 0) {
        SetCurrent(Get(ItemLastSel));
        ItemLastSel = -1;
    }

    Display();
}

eOSState cFlatSetupMMWidget::ProcessKey(eKeys Key) {
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown) {
        switch (Key) {
            case kOk:
                return osBack;
            default:
                break;
        }
    }
    if (Key == kLeft || Key == kRight) {
        const char* ItemText = Get(Current())->Text();
        if (strstr(ItemText, tr("Enable main menu widgets")) != NULL ||
            strstr(ItemText, tr("Widget weather: enable")) != NULL ||
            strstr(ItemText, tr("Widget DVB devices: enable")) != NULL ||
            strstr(ItemText, tr("Widget timer: enable")) != NULL ||
            strstr(ItemText, tr("Widget last recordings: enable")) != NULL ||
            strstr(ItemText, tr("Widget timer conflicts: enable")) != NULL ||
            strstr(ItemText, tr("Widget system information: enable")) != NULL ||
            strstr(ItemText, tr("Widget system updates: enable")) != NULL ||
            strstr(ItemText, tr("Widget temperatures: enable")) != NULL ||
            strstr(ItemText, tr("Widget custom commands: enable")) != NULL) {
              ItemLastSel = Current();
              Setup();
        }
    }
    return state;
}
