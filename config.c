/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./config.h"

#include <vdr/tools.h>

#include <dirent.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <utility>
#include <vector>

#include "./flat.h"

cFlatConfig::cFlatConfig() {
    RecordingOldFolder.reserve(64);  // Set to at least 64 entry's
    RecordingOldValue.reserve(64);
}

cFlatConfig::~cFlatConfig() {
}

bool cFlatConfig::SetupParse(const char *Name, const char *Value) {
    if      (strcmp(Name, "decorBorderChannelByTheme") == 0)            decorBorderChannelByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelTypeUser") == 0)           decorBorderChannelTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelSizeUser") == 0)           decorBorderChannelSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelEPGByTheme") == 0)         decorBorderChannelEPGByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelEPGTypeUser") == 0)        decorBorderChannelEPGTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderChannelEPGSizeUser") == 0)        decorBorderChannelEPGSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderTopBarByTheme") == 0)             decorBorderTopBarByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderTopBarTypeUser") == 0)            decorBorderTopBarTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderTopBarSizeUser") == 0)            decorBorderTopBarSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMessageByTheme") == 0)            decorBorderMessageByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderMessageTypeUser") == 0)           decorBorderMessageTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMessageSizeUser") == 0)           decorBorderMessageSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderVolumeByTheme") == 0)             decorBorderVolumeByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderVolumeTypeUser") == 0)            decorBorderVolumeTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderVolumeSizeUser") == 0)            decorBorderVolumeSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderTrackByTheme") == 0)              decorBorderTrackByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderTrackTypeUser") == 0)             decorBorderTrackTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderTrackSizeUser") == 0)             decorBorderTrackSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderReplayByTheme") == 0)             decorBorderReplayByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderReplayTypeUser") == 0)            decorBorderReplayTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderReplaySizeUser") == 0)            decorBorderReplaySizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuItemByTheme") == 0)           decorBorderMenuItemByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuItemTypeUser") == 0)          decorBorderMenuItemTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuItemSizeUser") == 0)          decorBorderMenuItemSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentHeadByTheme") == 0)    decorBorderMenuContentHeadByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentHeadTypeUser") == 0)   decorBorderMenuContentHeadTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentHeadSizeUser") == 0)   decorBorderMenuContentHeadSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentByTheme") == 0)        decorBorderMenuContentByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentTypeUser") == 0)       decorBorderMenuContentTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderMenuContentSizeUser") == 0)       decorBorderMenuContentSizeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderButtonByTheme") == 0)             decorBorderButtonByTheme = atoi(Value);
    else if (strcmp(Name, "decorBorderButtonTypeUser") == 0)            decorBorderButtonTypeUser = atoi(Value);
    else if (strcmp(Name, "decorBorderButtonSizeUser") == 0)            decorBorderButtonSizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressChannelByTheme") == 0)          decorProgressChannelByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressChannelTypeUser") == 0)         decorProgressChannelTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressChannelSizeUser") == 0)         decorProgressChannelSizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressVolumeByTheme") == 0)           decorProgressVolumeByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressVolumeTypeUser") == 0)          decorProgressVolumeTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressVolumeSizeUser") == 0)          decorProgressVolumeSizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressMenuItemByTheme") == 0)         decorProgressMenuItemByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressMenuItemTypeUser") == 0)        decorProgressMenuItemTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressMenuItemSizeUser") == 0)        decorProgressMenuItemSizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressReplayByTheme") == 0)           decorProgressReplayByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressReplayTypeUser") == 0)          decorProgressReplayTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressReplaySizeUser") == 0)          decorProgressReplaySizeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressSignalByTheme") == 0)           decorProgressSignalByTheme = atoi(Value);
    else if (strcmp(Name, "decorProgressSignalTypeUser") == 0)          decorProgressSignalTypeUser = atoi(Value);
    else if (strcmp(Name, "decorProgressSignalSizeUser") == 0)          decorProgressSignalSizeUser = atoi(Value);
    else if (strcmp(Name, "decorScrollBarByTheme") == 0)                decorScrollBarByTheme = atoi(Value);
    else if (strcmp(Name, "decorScrollBarTypeUser") == 0)               decorScrollBarTypeUser = atoi(Value);
    else if (strcmp(Name, "decorScrollBarSizeUser") == 0)               decorScrollBarSizeUser = atoi(Value);

    else if (strcmp(Name, "ButtonsShowEmpty") == 0)                     ButtonsShowEmpty = atoi(Value);
    else if (strcmp(Name, "ChannelDvbapiInfoShow") == 0)                ChannelDvbapiInfoShow = atoi(Value);
    else if (strcmp(Name, "ChannelFormatShow") == 0)                    ChannelFormatShow = atoi(Value);
    else if (strcmp(Name, "ChannelIconsShow") == 0)                     ChannelIconsShow = atoi(Value);
    else if (strcmp(Name, "ChannelResolutionAspectShow") == 0)          ChannelResolutionAspectShow = atoi(Value);
    else if (strcmp(Name, "ChannelShowNameWithShadow") == 0)            ChannelShowNameWithShadow = atoi(Value);
    else if (strcmp(Name, "ChannelShowStartTime") == 0)                 ChannelShowStartTime = atoi(Value);
    else if (strcmp(Name, "ChannelSimpleAspectFormat") == 0)            ChannelSimpleAspectFormat = atoi(Value);
    else if (strcmp(Name, "ChannelTimeLeft") == 0)                      ChannelTimeLeft = atoi(Value);
    else if (strcmp(Name, "ChannelWeatherShow") == 0)                   ChannelWeatherShow = atoi(Value);
    else if (strcmp(Name, "DecorIndex") == 0)                           DecorIndex = atoi(Value);
    else if (strcmp(Name, "DiskUsageFree") == 0)                        DiskUsageFree = atoi(Value);
    else if (strcmp(Name, "DiskUsageShort") == 0)                       DiskUsageShort = atoi(Value);
    else if (strcmp(Name, "DiskUsageShow") == 0)                        DiskUsageShow = atoi(Value);
    else if (strcmp(Name, "EpgAdditionalInfoShow") == 0)                EpgAdditionalInfoShow = atoi(Value);
    else if (strcmp(Name, "EpgRerunsShow") == 0)                        EpgRerunsShow = atoi(Value);
    else if (strcmp(Name, "EpgFskGenreIconSize") == 0)                  EpgFskGenreIconSize = atod(Value);
    else if (strcmp(Name, "MainMenuItemScale") == 0)                    MainMenuItemScale = atod(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerHideEmpty") == 0)   MainMenuWidgetActiveTimerHideEmpty = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerMaxCount") == 0)    MainMenuWidgetActiveTimerMaxCount = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerPosition") == 0)    MainMenuWidgetActiveTimerPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShow") == 0)        MainMenuWidgetActiveTimerShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowActive") == 0)  MainMenuWidgetActiveTimerShowActive = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowRecording") == 0) MainMenuWidgetActiveTimerShowRecording = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowRemoteActive") == 0)         MainMenuWidgetActiveTimerShowRemoteActive = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowRemoteRecording") == 0)         MainMenuWidgetActiveTimerShowRemoteRecording = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetActiveTimerShowRemoteRefreshTime") == 0)         MainMenuWidgetActiveTimerShowRemoteRefreshTime = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetCommandPosition") == 0)        MainMenuWidgetCommandPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetCommandShow") == 0)            MainMenuWidgetCommandShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesDiscardNotUsed") == 0)  MainMenuWidgetDVBDevicesDiscardNotUsed = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesDiscardUnknown") == 0)  MainMenuWidgetDVBDevicesDiscardUnknown = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesNativeNumbering") == 0)  MainMenuWidgetDVBDevicesNativeNumbering = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesPosition") == 0)     MainMenuWidgetDVBDevicesPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetDVBDevicesShow") == 0)         MainMenuWidgetDVBDevicesShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetLastRecMaxCount") == 0)        MainMenuWidgetLastRecMaxCount = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetLastRecPosition") == 0)        MainMenuWidgetLastRecPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetLastRecShow") == 0)            MainMenuWidgetLastRecShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetsEnable") == 0)                MainMenuWidgetsEnable = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemInfoPosition") == 0)     MainMenuWidgetSystemInfoPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemInfoShow") == 0)         MainMenuWidgetSystemInfoShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemUpdatesHideIfZero") == 0)MainMenuWidgetSystemUpdatesHideIfZero = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemUpdatesPosition") == 0)  MainMenuWidgetSystemUpdatesPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetSystemUpdatesShow") == 0)      MainMenuWidgetSystemUpdatesShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTemperaturesPosition") == 0)   MainMenuWidgetTemperaturesPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTemperaturesShow") == 0)       MainMenuWidgetTemperaturesShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTimerConflictsHideEmpty") == 0)MainMenuWidgetTimerConflictsHideEmpty = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTimerConflictsPosition") == 0) MainMenuWidgetTimerConflictsPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetTimerConflictsShow") == 0)     MainMenuWidgetTimerConflictsShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetWeatherDays") == 0)            MainMenuWidgetWeatherDays = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetWeatherPosition") == 0)        MainMenuWidgetWeatherPosition = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetWeatherShow") == 0)            MainMenuWidgetWeatherShow = atoi(Value);
    else if (strcmp(Name, "MainMenuWidgetWeatherType") == 0)            MainMenuWidgetWeatherType = atoi(Value);
    else if (strcmp(Name, "marginOsdHor") == 0)                         marginOsdHor = atoi(Value);
    else if (strcmp(Name, "marginOsdVer") == 0)                         marginOsdVer = atoi(Value);
    else if (strcmp(Name, "MenuChannelShowCount") == 0)                 MenuChannelShowCount = atoi(Value);
    else if (strcmp(Name, "MenuChannelView") == 0)                      MenuChannelView = atoi(Value);
    else if (strcmp(Name, "MenuContentFullSize") == 0)                  MenuContentFullSize = atoi(Value);
    else if (strcmp(Name, "MenuEventView") == 0)                        MenuEventView = atoi(Value);
    else if (strcmp(Name, "MenuEventViewAlwaysWithDate") == 0)          MenuEventViewAlwaysWithDate = atoi(Value);
    else if (strcmp(Name, "MenuFullOsd") == 0)                          MenuFullOsd = atoi(Value);
    else if (strcmp(Name, "MenuItemIconsShow") == 0)                    MenuItemIconsShow = atoi(Value);
    else if (strcmp(Name, "MenuItemPadding") == 0)                      MenuItemPadding = atoi(Value);
    else if (strcmp(Name, "MenuItemParseTilde") == 0)                   MenuItemParseTilde = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingClearPercent") == 0)        MenuItemRecordingClearPercent = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingDefaultOldDays") == 0)      MenuItemRecordingDefaultOldDays = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingSeenThreshold") == 0)       MenuItemRecordingSeenThreshold = atod(Value);
    else if (strcmp(Name, "MenuItemRecordingShowFolderDate") == 0)      MenuItemRecordingShowFolderDate = atoi(Value);
    else if (strcmp(Name, "MenuEventRecordingViewJustify") == 0)        MenuEventRecordingViewJustify = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingShowRecordingErrors") == 0)      MenuItemRecordingShowRecordingErrors = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingShowRecordingErrorsThreshold") == 0)      MenuItemRecordingShowRecordingErrorsThreshold = atoi(Value);
    else if (strcmp(Name, "MenuItemRecordingShowFormatIcons") == 0)     MenuItemRecordingShowFormatIcons = atoi(Value);
    else if (strcmp(Name, "MenuRecordingShowCount") == 0)               MenuRecordingShowCount = atoi(Value);
    else if (strcmp(Name, "MenuRecordingView") == 0)                    MenuRecordingView = atoi(Value);
    else if (strcmp(Name, "MenuTimerShowCount") == 0)                   MenuTimerShowCount = atoi(Value);
    else if (strcmp(Name, "MenuTimerView") == 0)                        MenuTimerView = atoi(Value);
    else if (strcmp(Name, "MessageColorPosition") == 0)                 MessageColorPosition = atoi(Value);
    else if (strcmp(Name, "MessageOffset") == 0)                        MessageOffset = atoi(Value);
    else if (strcmp(Name, "PlaybackShowRecordingErrors") == 0)          PlaybackShowRecordingErrors = atoi(Value);
    else if (strcmp(Name, "PlaybackShowErrorMarks") == 0)               PlaybackShowErrorMarks = atoi(Value);
    else if (strcmp(Name, "PlaybackShowRecordingDate") == 0)            PlaybackShowRecordingDate = atoi(Value);
    else if (strcmp(Name, "PlaybackShowEndTime") == 0)                  PlaybackShowEndTime = atoi(Value);
    else if (strcmp(Name, "PlaybackWeatherShow") == 0)                  PlaybackWeatherShow = atoi(Value);
    else if (strcmp(Name, "RecordingAdditionalInfoShow") == 0)          RecordingAdditionalInfoShow = atoi(Value);
    else if (strcmp(Name, "RecordingDimmOnPause") == 0)                 RecordingDimmOnPause = atoi(Value);
    else if (strcmp(Name, "RecordingDimmOnPauseDelay") == 0)            RecordingDimmOnPauseDelay = atoi(Value);
    else if (strcmp(Name, "RecordingDimmOnPauseOpaque") == 0)           RecordingDimmOnPauseOpaque = atoi(Value);
    else if (strcmp(Name, "RecordingFormatShow") == 0)                  RecordingFormatShow = atoi(Value);
    else if (strcmp(Name, "RecordingResolutionAspectShow") == 0)        RecordingResolutionAspectShow = atoi(Value);
    else if (strcmp(Name, "RecordingSimpleAspectFormat") == 0)          RecordingSimpleAspectFormat = atoi(Value);
    else if (strcmp(Name, "ScrollerDelay") == 0)                        ScrollerDelay = atoi(Value);
    else if (strcmp(Name, "ScrollerEnable") == 0)                       ScrollerEnable = atoi(Value);
    else if (strcmp(Name, "ScrollerStep") == 0)                         ScrollerStep = atoi(Value);
    else if (strcmp(Name, "ScrollerType") == 0)                         ScrollerType = atoi(Value);
    else if (strcmp(Name, "ShortRecordingCount") == 0)                  ShortRecordingCount = atoi(Value);
    else if (strcmp(Name, "SignalQualityShow") == 0)                    SignalQualityShow = atoi(Value);
    else if (strcmp(Name, "SignalQualityUseColors") == 0)               SignalQualityUseColors = atoi(Value);
    else if (strcmp(Name, "TimeSecsScale") == 0)                        TimeSecsScale = atod(Value);
    else if (strcmp(Name, "TopBarFontClockScale") == 0)                 TopBarFontClockScale = atod(Value);
    else if (strcmp(Name, "TopBarFontSize") == 0)                       TopBarFontSize = atod(Value);
    else if (strcmp(Name, "TopBarHideClockText") == 0)                  TopBarHideClockText = atoi(Value);
    else if (strcmp(Name, "TopBarMenuIconShow") == 0)                   TopBarMenuIconShow = atoi(Value);
    else if (strcmp(Name, "TopBarRecConflictsHigh") == 0)               TopBarRecConflictsHigh = atoi(Value);
    else if (strcmp(Name, "TopBarRecConflictsShow") == 0)               TopBarRecConflictsShow = atoi(Value);
    else if (strcmp(Name, "TopBarRecordingShow") == 0)                  TopBarRecordingShow = atoi(Value);
    else if (strcmp(Name, "TVScraperChanInfoPosterSize") == 0)          TVScraperChanInfoPosterSize = atod(Value);
    else if (strcmp(Name, "TVScraperChanInfoShowPoster") == 0)          TVScraperChanInfoShowPoster = atoi(Value);
    else if (strcmp(Name, "TVScraperEPGInfoShowActors") == 0)           TVScraperEPGInfoShowActors = atoi(Value);
    else if (strcmp(Name, "TVScraperEPGInfoShowPoster") == 0)           TVScraperEPGInfoShowPoster = atoi(Value);
    else if (strcmp(Name, "TVScraperRecInfoShowActors") == 0)           TVScraperRecInfoShowActors = atoi(Value);
    else if (strcmp(Name, "TVScraperShowMaxActors") == 0)               TVScraperShowMaxActors = atoi(Value);
    else if (strcmp(Name, "TVScraperRecInfoShowPoster") == 0)           TVScraperRecInfoShowPoster = atoi(Value);
    else if (strcmp(Name, "TVScraperReplayInfoPosterSize") == 0)        TVScraperReplayInfoPosterSize = atod(Value);
    else if (strcmp(Name, "TVScraperReplayInfoShowPoster") == 0)        TVScraperReplayInfoShowPoster = atoi(Value);
    else if (strcmp(Name, "TVScraperPosterOpacity") == 0)               TVScraperPosterOpacity = atod(Value);
    else if (strcmp(Name, "WeatherFontSize") == 0)                      WeatherFontSize = atod(Value);
    else
        return false;

    return true;
}

void cFlatConfig::ThemeCheckAndInit() {
    if (strcmp(Theme.Name(), *ThemeCurrent) != 0) {
        dsyslog("flatPlus: Load theme: %s", *ThemeCurrent);
        ThemeCurrent = Theme.Name();
        ThemeInit();
    }
}

void cFlatConfig::DecorCheckAndInit() {
    if (m_DecorCurrent != DecorIndex) {
        m_DecorCurrent = DecorIndex;
        DecorLoadCurrent();
    }
    if (decorBorderChannelByTheme) {
        decorBorderChannelType = decorBorderChannelTypeTheme;
        decorBorderChannelSize = decorBorderChannelSizeTheme;
    } else {
        decorBorderChannelType = decorBorderChannelTypeUser;
        decorBorderChannelSize = decorBorderChannelSizeUser;
    }

    if (decorBorderChannelEPGByTheme) {
        decorBorderChannelEPGType = decorBorderChannelEPGTypeTheme;
        decorBorderChannelEPGSize = decorBorderChannelEPGSizeTheme;
    } else {
        decorBorderChannelEPGType = decorBorderChannelEPGTypeUser;
        decorBorderChannelEPGSize = decorBorderChannelEPGSizeUser;
    }

    if (decorBorderTopBarByTheme) {
        decorBorderTopBarType = decorBorderTopBarTypeTheme;
        decorBorderTopBarSize = decorBorderTopBarSizeTheme;
    } else {
        decorBorderTopBarType = decorBorderTopBarTypeUser;
        decorBorderTopBarSize = decorBorderTopBarSizeUser;
    }

    if (decorBorderMessageByTheme) {
        decorBorderMessageType = decorBorderMessageTypeTheme;
        decorBorderMessageSize = decorBorderMessageSizeTheme;
    } else {
        decorBorderMessageType = decorBorderMessageTypeUser;
        decorBorderMessageSize = decorBorderMessageSizeUser;
    }

    if (decorBorderVolumeByTheme) {
        decorBorderVolumeType = decorBorderVolumeTypeTheme;
        decorBorderVolumeSize = decorBorderVolumeSizeTheme;
    } else {
        decorBorderVolumeType = decorBorderVolumeTypeUser;
        decorBorderVolumeSize = decorBorderVolumeSizeUser;
    }

    if (decorBorderTrackByTheme) {
        decorBorderTrackType = decorBorderTrackTypeTheme;
        decorBorderTrackSize = decorBorderTrackSizeTheme;
    } else {
        decorBorderTrackType = decorBorderTrackTypeUser;
        decorBorderTrackSize = decorBorderTrackSizeUser;
    }

    if (decorBorderReplayByTheme) {
        decorBorderReplayType = decorBorderReplayTypeTheme;
        decorBorderReplaySize = decorBorderReplaySizeTheme;
    } else {
        decorBorderReplayType = decorBorderReplayTypeUser;
        decorBorderReplaySize = decorBorderReplaySizeUser;
    }

    if (decorBorderMenuItemByTheme) {
        decorBorderMenuItemType = decorBorderMenuItemTypeTheme;
        decorBorderMenuItemSize = decorBorderMenuItemSizeTheme;
    } else {
        decorBorderMenuItemType = decorBorderMenuItemTypeUser;
        decorBorderMenuItemSize = decorBorderMenuItemSizeUser;
    }

    if (decorBorderMenuContentHeadByTheme) {
        decorBorderMenuContentHeadType = decorBorderMenuContentHeadTypeTheme;
        decorBorderMenuContentHeadSize = decorBorderMenuContentHeadSizeTheme;
    } else {
        decorBorderMenuContentHeadType = decorBorderMenuContentHeadTypeUser;
        decorBorderMenuContentHeadSize = decorBorderMenuContentHeadSizeUser;
    }

    if (decorBorderMenuContentByTheme) {
        decorBorderMenuContentType = decorBorderMenuContentTypeTheme;
        decorBorderMenuContentSize = decorBorderMenuContentSizeTheme;
    } else {
        decorBorderMenuContentType = decorBorderMenuContentTypeUser;
        decorBorderMenuContentSize = decorBorderMenuContentSizeUser;
    }

    if (decorBorderButtonByTheme) {
        decorBorderButtonType = decorBorderButtonTypeTheme;
        decorBorderButtonSize = decorBorderButtonSizeTheme;
    } else {
        decorBorderButtonType = decorBorderButtonTypeUser;
        decorBorderButtonSize = decorBorderButtonSizeUser;
    }

    if (decorProgressChannelByTheme) {
        decorProgressChannelType = decorProgressChannelTypeTheme;
        decorProgressChannelSize = decorProgressChannelSizeTheme;
    } else {
        decorProgressChannelType = decorProgressChannelTypeUser;
        decorProgressChannelSize = decorProgressChannelSizeUser;
    }

    if (decorProgressVolumeByTheme) {
        decorProgressVolumeType = decorProgressVolumeTypeTheme;
        decorProgressVolumeSize = decorProgressVolumeSizeTheme;
    } else {
        decorProgressVolumeType = decorProgressVolumeTypeUser;
        decorProgressVolumeSize = decorProgressVolumeSizeUser;
    }

    if (decorProgressMenuItemByTheme) {
        decorProgressMenuItemType = decorProgressMenuItemTypeTheme;
        decorProgressMenuItemSize = decorProgressMenuItemSizeTheme;
    } else {
        decorProgressMenuItemType = decorProgressMenuItemTypeUser;
        decorProgressMenuItemSize = decorProgressMenuItemSizeUser;
    }

    if (decorProgressReplayByTheme) {
        decorProgressReplayType = decorProgressReplayTypeTheme;
        decorProgressReplaySize = decorProgressReplaySizeTheme;
    } else {
        decorProgressReplayType = decorProgressReplayTypeUser;
        decorProgressReplaySize = decorProgressReplaySizeUser;
    }

    if (decorProgressSignalByTheme) {
        decorProgressSignalType = decorProgressSignalTypeTheme;
        decorProgressSignalSize = decorProgressSignalSizeTheme;
    } else {
        decorProgressSignalType = decorProgressSignalTypeUser;
        decorProgressSignalSize = decorProgressSignalSizeUser;
    }

    if (decorScrollBarByTheme) {
        decorScrollBarType = decorScrollBarTypeTheme;
        decorScrollBarSize = decorScrollBarSizeTheme;
    } else {
        decorScrollBarType = decorScrollBarTypeUser;
        decorScrollBarSize = decorScrollBarSizeUser;
    }

    if (decorBorderChannelType == 0) decorBorderChannelSize = 0;
    if (decorBorderTopBarType == 0) decorBorderTopBarSize = 0;
    if (decorBorderMessageType == 0) decorBorderMessageSize = 0;
    if (decorBorderVolumeType == 0) decorBorderVolumeSize = 0;
    if (decorBorderTrackType == 0) decorBorderTrackSize = 0;
    if (decorBorderReplayType == 0) decorBorderReplaySize = 0;
    if (decorBorderMenuItemType == 0) decorBorderMenuItemSize = 0;
    if (decorBorderMenuContentHeadType == 0) decorBorderMenuContentHeadSize = 0;
    if (decorBorderMenuContentType == 0) decorBorderMenuContentSize = 0;
    if (decorBorderButtonType == 0) decorBorderButtonSize = 0;
}

void cFlatConfig::ThemeInit() {
    decorBorderChannelFg = Theme.Color(clrChannelBorderFg);
    decorBorderChannelBg = Theme.Color(clrChannelBorderBg);

    decorBorderChannelEPGFg = Theme.Color(clrChannelEPGBorderFg);
    decorBorderChannelEPGBg = Theme.Color(clrChannelEPGBorderBg);

    decorBorderTopBarFg = Theme.Color(clrTopBarBorderFg);
    decorBorderTopBarBg = Theme.Color(clrTopBarBorderBg);

    decorBorderMessageFg = Theme.Color(clrMessageBorderFg);
    decorBorderMessageBg = Theme.Color(clrMessageBorderBg);

    decorBorderVolumeFg = Theme.Color(clrVolumeBorderFg);
    decorBorderVolumeBg = Theme.Color(clrVolumeBorderBg);

    decorBorderTrackFg = Theme.Color(clrTrackItemBorderFg);
    decorBorderTrackBg = Theme.Color(clrTrackItemBorderBg);
    decorBorderTrackCurFg = Theme.Color(clrTrackItemCurrentBorderFg);
    decorBorderTrackCurBg = Theme.Color(clrTrackItemCurrentBorderBg);
    decorBorderTrackSelFg = Theme.Color(clrTrackItemSelableBorderFg);
    decorBorderTrackSelBg = Theme.Color(clrTrackItemSelableBorderBg);

    decorBorderReplayFg = Theme.Color(clrReplayBorderFg);
    decorBorderReplayBg = Theme.Color(clrReplayBorderBg);

    decorBorderMenuItemFg = Theme.Color(clrMenuItemBorderFg);
    decorBorderMenuItemBg = Theme.Color(clrMenuItemBorderBg);
    decorBorderMenuItemSelFg = Theme.Color(clrMenuItemSelableBorderFg);
    decorBorderMenuItemSelBg = Theme.Color(clrMenuItemSelableBorderBg);
    decorBorderMenuItemCurFg = Theme.Color(clrMenuItemCurrentBorderFg);
    decorBorderMenuItemCurBg = Theme.Color(clrMenuItemCurrentBorderBg);

    decorBorderMenuContentHeadFg = Theme.Color(clrMenuContentHeadBorderFg);
    decorBorderMenuContentHeadBg = Theme.Color(clrMenuContentHeadBorderBg);

    decorBorderMenuContentFg = Theme.Color(clrMenuContentBorderFg);
    decorBorderMenuContentBg = Theme.Color(clrMenuContentBorderBg);

    decorBorderButtonFg = Theme.Color(clrButtonBorderFg);
    decorBorderButtonBg = Theme.Color(clrButtonBorderBg);

    decorProgressChannelFg = Theme.Color(clrChannelProgressFg);
    decorProgressChannelBarFg = Theme.Color(clrChannelProgressBarFg);
    decorProgressChannelBg = Theme.Color(clrChannelProgressBg);

    decorProgressVolumeFg = Theme.Color(clrVolumeProgressFg);
    decorProgressVolumeBarFg = Theme.Color(clrVolumeProgressBarFg);
    decorProgressVolumeBg = Theme.Color(clrVolumeProgressBg);

    decorProgressMenuItemFg = Theme.Color(clrMenuItemProgressFg);
    decorProgressMenuItemBarFg = Theme.Color(clrMenuItemProgressBarFg);
    decorProgressMenuItemBg = Theme.Color(clrMenuItemProgressBg);
    decorProgressMenuItemCurFg = Theme.Color(clrMenuItemCurProgressFg);
    decorProgressMenuItemCurBarFg = Theme.Color(clrMenuItemCurProgressBarFg);
    decorProgressMenuItemCurBg = Theme.Color(clrMenuItemCurProgressBg);

    decorProgressReplayFg = Theme.Color(clrReplayProgressFg);
    decorProgressReplayBarFg = Theme.Color(clrReplayProgressBarFg);
    decorProgressReplayBg = Theme.Color(clrReplayProgressBg);

    decorProgressSignalFg = Theme.Color(clrChannelSignalProgressFg);
    decorProgressSignalBarFg = Theme.Color(clrChannelSignalProgressBarFg);
    decorProgressSignalBg = Theme.Color(clrChannelSignalProgressBg);

    decorScrollBarFg = Theme.Color(clrScrollbarFg);
    decorScrollBarBarFg = Theme.Color(clrScrollbarBarFg);
    decorScrollBarBg = Theme.Color(clrScrollbarBg);
}

void cFlatConfig::Init() {
    if (!strcmp(LogoPath, "")) {
        LogoPath = cString::sprintf("%s/logos", PLUGINRESOURCEPATH);
        dsyslog("flatPlus: LogoPath: %s", *LogoPath);
    }
    if (!strcmp(IconPath, "")) {
        IconPath = cString::sprintf("%s/icons", PLUGINRESOURCEPATH);
        dsyslog("flatPlus: IconPath: %s", *IconPath);
    }
    if (!strcmp(RecordingOldConfigFile, "")) {
        dsyslog("flatPlus: PLUGINCONFIGPATH: %s", PLUGINCONFIGPATH);
        RecordingOldConfigFile = cString::sprintf("%s/recordings_old.cfg", PLUGINCONFIGPATH);
        dsyslog("flatPlus: RecordingOldConfigFile: %s", *RecordingOldConfigFile);
        RecordingOldLoadConfig();
    }
    ThemeInit();
    DecorCheckAndInit();
}

// Sort strings by number in name
bool CompareNumStrings(const cString &a, const cString &b) {
    std::string_view sa = *a;
    std::string_view sb = *b;
    auto get_num = [](std::string_view s) {
        size_t pos = s.find('_');
        if (pos != std::string_view::npos)
            return atoi(std::string(s.substr(0, pos)).c_str());
        return 0;
    };
    return get_num(sa) < get_num(sb);
}

bool StringCompare(const std::string &left, const std::string &right) {
    auto len = std::min(left.size(), right.size());
    auto lit = left.cbegin(), rit = right.cbegin();
    while (len-- > 0) {
        if (tolower(*lit) != tolower(*rit)) return tolower(*lit) < tolower(*rit);
        ++lit;
        ++rit;
    }
    return left.size() < right.size();
}

bool PairCompareTimeString(const std::pair<time_t, cString> &i, const std::pair<time_t, cString> &j) {
    return i.first < j.first;
}

bool PairCompareIntString(const std::pair<int, std::string> &i, const std::pair<int, std::string> &j) {
    return i.first < j.first;  // ascending
}

/**
 * Round up a number to the nearest multiple of another number.
 *
 * @param[in] NumToRound The number to round up.
 * @param[in] multiple The number to round up to. If zero, return NumToRound.
 * @return the rounded up number.
 */
int RoundUp(int NumToRound, int multiple) {
    if (multiple == 0) return NumToRound;

    return (NumToRound + multiple - 1) / multiple * multiple;
}

void cFlatConfig::DecorDescriptions(cStringList &Decors) {
    cString DecorPath = cString::sprintf("%s/decors", PLUGINRESOURCEPATH);
    std::vector<std::string> files;
    files.reserve(64);  // Set to at least 64 entry's
    Decors.Clear();

    cString FileName {""};
    cReadDir d(*DecorPath);
    struct dirent *e;
    while ((e = d.Next()) != nullptr) {
        FileName = AddDirectory(*DecorPath, e->d_name);
        files.emplace_back(*FileName);
    }

    std::sort(files.begin(), files.end(), StringCompare);
    std::string File_Name {""};
    cString Desc {""};
    const std::size_t FilesSize {files.size()};
    for (uint i {0}; i < FilesSize; ++i) {
        File_Name = files.at(i);
        Desc = DecorDescription(File_Name.c_str());
        Decors.Append(strdup(*Desc));
    }
}

cString cFlatConfig::DecorDescription(cString File) {
    cString description {""};
    FILE *f = fopen(File, "r");
    if (f) {
        int line {0};
        char *s {nullptr}, *p {nullptr}, *n {nullptr}, *v {nullptr};
        cReadLine ReadLine;
        while ((s = ReadLine.Read(f)) != nullptr) {
            ++line;
            p = strchr(s, '#');
            if (p) *p = 0;
            s = stripspace(skipspace(s));
            if (!isempty(s)) {
                n = s;
                v = strchr(s, '=');
                if (v) {
                    *v++ = 0;
                    n = stripspace(skipspace(n));
                    v = stripspace(skipspace(v));
                    if (strstr(n, "Description") == n) {
                        description = strdup(v);
                        break;
                    }
                }
            }
        }  // while
        fclose(f);
    }
    return description;
}

void cFlatConfig::DecorLoadCurrent() {
    cString DecorPath = cString::sprintf("%s/decors", PLUGINRESOURCEPATH);
    std::vector<std::string> files;
    files.reserve(64);  // Set to at least 64 entry's

    cString FileName {""};
    cReadDir d(*DecorPath);
    struct dirent *e;
    while ((e = d.Next()) != nullptr) {
        FileName = AddDirectory(*DecorPath, e->d_name);
        files.emplace_back(*FileName);
    }

    std::string FileName2 {""};
    std::sort(files.begin(), files.end(), StringCompare);
    if (DecorIndex >= 0 && DecorIndex < static_cast<int>(files.size())) {
        FileName2 = files.at(DecorIndex);
        DecorLoadFile(FileName2.c_str());
    }
}

void cFlatConfig::DecorLoadFile(cString File) {
    dsyslog("flatPlus: Load decor file: %s", *File);

    FILE *f = fopen(File, "r");
    if (f) {
        int line {0}, value {0};
        char *s {nullptr}, *p {nullptr}, *n {nullptr}, *v {nullptr};
        cReadLine ReadLine;
        while ((s = ReadLine.Read(f)) != nullptr) {
            ++line;
            p = strchr(s, '#');
            if (p)
                *p = 0;
            s = stripspace(skipspace(s));
            if (!isempty(s)) {
                n = s;
                v = strchr(s, '=');
                if (v) {
                    *v++ = 0;
                    n = stripspace(skipspace(n));
                    v = stripspace(skipspace(v));
                    value = atoi(v);
                    if (strstr(n, "ChannelBorderType") == n) {
                        decorBorderChannelTypeTheme = value; continue; }
                    if (strstr(n, "ChannelBorderSize") == n) {
                        decorBorderChannelSizeTheme = value; continue; }
                    if (strstr(n, "ChannelEPGBorderType") == n) {
                        decorBorderChannelEPGTypeTheme = value; continue; }
                    if (strstr(n, "ChannelEPGBorderSize") == n) {
                        decorBorderChannelEPGSizeTheme = value; continue; }
                    if (strstr(n, "TopBarBorderType") == n) {
                        decorBorderTopBarTypeTheme = value; continue; }
                    if (strstr(n, "TopBarBorderSize") == n) {
                        decorBorderTopBarSizeTheme = value; continue; }
                    if (strstr(n, "MessageBorderType") == n) {
                        decorBorderMessageTypeTheme = value; continue; }
                    if (strstr(n, "MessageBorderSize") == n) {
                        decorBorderMessageSizeTheme = value; continue; }
                    if (strstr(n, "VolumeBorderType") == n) {
                        decorBorderVolumeTypeTheme = value; continue; }
                    if (strstr(n, "VolumeBorderSize") == n) {
                        decorBorderVolumeSizeTheme = value; continue; }
                    if (strstr(n, "TrackItemBorderType") == n) {
                        decorBorderTrackTypeTheme = value; continue; }
                    if (strstr(n, "TrackItemBorderSize") == n) {
                        decorBorderTrackSizeTheme = value; continue; }
                    if (strstr(n, "ReplayBorderType") == n) {
                        decorBorderReplayTypeTheme = value; continue; }
                    if (strstr(n, "ReplayBorderSize") == n) {
                        decorBorderReplaySizeTheme = value; continue; }
                    if (strstr(n, "MenuItemBorderType") == n) {
                        decorBorderMenuItemTypeTheme = value; continue; }
                    if (strstr(n, "MenuItemBorderSize") == n) {
                        decorBorderMenuItemSizeTheme = value; continue; }
                    if (strstr(n, "MenuContentHeadBorderType") == n) {
                        decorBorderMenuContentHeadTypeTheme = value; continue; }
                    if (strstr(n, "MenuContentHeadBorderSize") == n) {
                        decorBorderMenuContentHeadSizeTheme = value; continue; }
                    if (strstr(n, "MenuContentBorderType") == n) {
                        decorBorderMenuContentTypeTheme = value; continue; }
                    if (strstr(n, "MenuContentBorderSize") == n) {
                        decorBorderMenuContentSizeTheme = value; continue; }
                    if (strstr(n, "ButtonBorderType") == n) {
                        decorBorderButtonTypeTheme = value; continue; }
                    if (strstr(n, "ButtonBorderSize") == n) {
                        decorBorderButtonSizeTheme = value; continue; }
                    if (strstr(n, "ChannelProgressType") == n) {
                        decorProgressChannelTypeTheme = value; continue; }
                    if (strstr(n, "ChannelProgressSize") == n) {
                        decorProgressChannelSizeTheme = value; continue; }
                    if (strstr(n, "VolumeProgressType") == n) {
                        decorProgressVolumeTypeTheme = value; continue; }
                    if (strstr(n, "VolumeProgressSize") == n) {
                        decorProgressVolumeSizeTheme = value; continue; }
                    if (strstr(n, "MenuItemProgressType") == n) {
                        decorProgressMenuItemTypeTheme = value; continue; }
                    if (strstr(n, "MenuItemProgressSize") == n) {
                        decorProgressMenuItemSizeTheme = value; continue; }
                    if (strstr(n, "ReplayProgressType") == n) {
                        decorProgressReplayTypeTheme = value; continue; }
                    if (strstr(n, "ReplayProgressSize") == n) {
                        decorProgressReplaySizeTheme = value; continue; }
                    if (strstr(n, "ChannelSignalProgressType") == n) {
                        decorProgressSignalTypeTheme = value; continue; }
                    if (strstr(n, "ChannelSignalProgressSize") == n) {
                        decorProgressSignalSizeTheme = value; continue; }
                    if (strstr(n, "ScrollBarType") == n) {
                        decorScrollBarTypeTheme = value; continue; }
                    if (strstr(n, "ScrollBarSize") == n) {
                        decorScrollBarSizeTheme = value; continue; }
                }
            }
        }  // while
        fclose(f);
    }
}

void cFlatConfig::RecordingOldLoadConfig() {
    dsyslog("flatPlus: Load recording old config file: %s", *RecordingOldConfigFile);
    RecordingOldFolder.clear();
    RecordingOldValue.clear();

    FILE *f = fopen(RecordingOldConfigFile, "r");
    if (f) {
        int line {0}, value {0};
        char *s {nullptr}, *p {nullptr}, *n {nullptr}, *v {nullptr};
        cReadLine ReadLine;
        while ((s = ReadLine.Read(f)) != nullptr) {
            ++line;
            p = strchr(s, '#');
            if (p) *p = 0;
            s = stripspace(skipspace(s));
            if (!isempty(s)) {
                n = s;
                v = strchr(s, '=');
                if (v) {
                    *v++ = 0;
                    n = stripspace(skipspace(n));
                    v = stripspace(skipspace(v));
                    value = atoi(v);
                    dsyslog("flatPlus: Recording old config - folder: %s value: %d", n, value);
                    RecordingOldFolder.emplace_back(n);
                    RecordingOldValue.emplace_back(value);
                }
            }
        }  // while
        fclose(f);
    }
}

int cFlatConfig::GetRecordingOldValue(const std::string &folder) {
    std::vector<std::string>::size_type sz = RecordingOldFolder.size();
    for (std::size_t i {0}; i < sz; ++i) {
        if (RecordingOldFolder[i] == folder)
            return RecordingOldValue[i];
    }
    return -1;
}

void cFlatConfig::SetLogoPath(cString path) {
    LogoPath = CheckSlashAtEnd(*path);
}

cString cFlatConfig::CheckSlashAtEnd(std::string path) {
    if (!path.empty() && path.back() == '/') {
        path.pop_back();  // Use pop_back for efficiency
    }
    return path.c_str();
}

void cFlatConfig::Store(const char *Name, int Value, const char *Filename) {
    Store(Name, cString::sprintf("%d", Value), Filename);
}

void cFlatConfig::Store(const char *Name, double &Value, const char *Filename) {
    Store(Name, dtoa(Value), Filename);
}

void cFlatConfig::Store(const char *Name, const char *Value, const char *Filename) {
    FILE *f = fopen(Filename, "a");
    if (!f) return;

    fprintf(f, "%s = %s\n", Name, Value);
    fclose(f);
}

void cFlatConfig::GetConfigFiles(cStringList &Files) {
    cString ConfigsPath = cString::sprintf("%s/configs", cPlugin::ConfigDirectory(PLUGIN_NAME_I18N));
    std::vector<std::string> files;
    files.reserve(64);  // Set to at least 64 entry's
    Files.Clear();

    cReadDir d(*ConfigsPath);
    struct dirent *e;
    while ((e = d.Next()) != nullptr) {
        files.emplace_back(e->d_name);
    }

    std::sort(files.begin(), files.end(), StringCompare);
    std::string FileName {""};
    const std::size_t FilesSize {files.size()};
    for (std::size_t i {0}; i < FilesSize; ++i) {
        FileName = files.at(i);
        Files.Append(strdup(FileName.c_str()));
    }
}
