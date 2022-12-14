#pragma once

#include <string>
#include <vector>

#include <vdr/plugin.h>
#include "flat.h"

#define PLUGINCONFIGPATH (cPlugin::ConfigDirectory(PLUGIN_NAME_I18N))
#define PLUGINRESOURCEPATH (cPlugin::ResourceDirectory(PLUGIN_NAME_I18N))
#define WIDGETOUTPUTPATH "/tmp/skinflatplus/widgets"


bool stringCompare( const std::string &left, const std::string &right );
bool pairCompareTimeStringDesc(const std::pair<time_t, std::string>&i, const std::pair<time_t, std::string>&j);
bool pairCompareIntString(const std::pair<int, std::string>&i, const std::pair<int, std::string>&j);
int roundUp(int numToRound, int multiple);

class cFlatConfig
{
    private:
        cString checkSlashAtEnd(std::string path);

        int DecorCurrent;
    public:
        cFlatConfig(void);
        ~cFlatConfig(void);

        void Init(void);
        void SetLogoPath(cString path);
        bool SetupParse(const char *Name, const char *Value);

        void ThemeCheckAndInit(void);
        void ThemeInit(void);
        void DecorDescriptions(cStringList &Decors);
        cString DecorDescription(cString File);
        void DecorLoadFile(cString File);
        void DecorLoadCurrent(void);
        void DecorCheckAndInit(void);

        void GetConfigFiles(cStringList &Files);

        void RecordingOldLoadConfig(void);
        int GetRecordingOldValue(std::string folder);
    public:

        cString ThemeCurrent;
        cString logoPath;
        cString iconPath;
        cString RecordingOldConfigFile;

        std::vector<std::string> RecordingOldFolder;
        std::vector<int> RecordingOldValue;
        // BORDER TYPES
        // 0 = none
        // 1 = rect
        // 2 = round
        // 3 = invert round
        // 4 = rect + alpha blend
        // 5 = round + alpha blend
        // 6 = invert round + alpha blend

        int decorBorderChannelByTheme;
        int decorBorderChannelTypeTheme, decorBorderChannelSizeTheme;
        int decorBorderChannelTypeUser, decorBorderChannelSizeUser;
        int decorBorderChannelType, decorBorderChannelSize;
        tColor decorBorderChannelFg, decorBorderChannelBg;

        int decorBorderChannelEPGByTheme;
        int decorBorderChannelEPGTypeTheme, decorBorderChannelEPGSizeTheme;
        int decorBorderChannelEPGTypeUser, decorBorderChannelEPGSizeUser;
        int decorBorderChannelEPGType, decorBorderChannelEPGSize;
        tColor decorBorderChannelEPGFg, decorBorderChannelEPGBg;

        int decorBorderTopBarByTheme;
        int decorBorderTopBarTypeTheme, decorBorderTopBarSizeTheme;
        int decorBorderTopBarTypeUser, decorBorderTopBarSizeUser;
        int decorBorderTopBarType, decorBorderTopBarSize;
        tColor decorBorderTopBarFg, decorBorderTopBarBg;

        int decorBorderMessageByTheme;
        int decorBorderMessageTypeTheme, decorBorderMessageSizeTheme;
        int decorBorderMessageTypeUser, decorBorderMessageSizeUser;
        int decorBorderMessageType, decorBorderMessageSize;
        tColor decorBorderMessageFg, decorBorderMessageBg;

        int decorBorderVolumeByTheme;
        int decorBorderVolumeTypeTheme, decorBorderVolumeSizeTheme;
        int decorBorderVolumeTypeUser, decorBorderVolumeSizeUser;
        int decorBorderVolumeType, decorBorderVolumeSize;
        tColor decorBorderVolumeFg, decorBorderVolumeBg;

        int decorBorderTrackByTheme;
        int decorBorderTrackTypeTheme, decorBorderTrackSizeTheme;
        int decorBorderTrackTypeUser, decorBorderTrackSizeUser;
        int decorBorderTrackType, decorBorderTrackSize;
        tColor decorBorderTrackFg, decorBorderTrackBg;
        tColor decorBorderTrackSelFg, decorBorderTrackSelBg;
        tColor decorBorderTrackCurFg, decorBorderTrackCurBg;

        int decorBorderReplayByTheme;
        int decorBorderReplayTypeTheme, decorBorderReplaySizeTheme;
        int decorBorderReplayTypeUser, decorBorderReplaySizeUser;
        int decorBorderReplayType, decorBorderReplaySize;
        tColor decorBorderReplayFg, decorBorderReplayBg;

        int decorBorderMenuItemByTheme;
        int decorBorderMenuItemTypeTheme, decorBorderMenuItemSizeTheme;
        int decorBorderMenuItemTypeUser, decorBorderMenuItemSizeUser;
        int decorBorderMenuItemType, decorBorderMenuItemSize;
        tColor decorBorderMenuItemFg, decorBorderMenuItemBg;
        tColor decorBorderMenuItemSelFg, decorBorderMenuItemSelBg;
        tColor decorBorderMenuItemCurFg, decorBorderMenuItemCurBg;

        int decorBorderMenuContentHeadByTheme;
        int decorBorderMenuContentHeadTypeTheme, decorBorderMenuContentHeadSizeTheme;
        int decorBorderMenuContentHeadTypeUser, decorBorderMenuContentHeadSizeUser;
        int decorBorderMenuContentHeadType, decorBorderMenuContentHeadSize;
        tColor decorBorderMenuContentHeadFg, decorBorderMenuContentHeadBg;

        int decorBorderMenuContentByTheme;
        int decorBorderMenuContentTypeTheme, decorBorderMenuContentSizeTheme;
        int decorBorderMenuContentTypeUser, decorBorderMenuContentSizeUser;
        int decorBorderMenuContentType, decorBorderMenuContentSize;
        tColor decorBorderMenuContentFg, decorBorderMenuContentBg;

        int decorBorderButtonByTheme;
        int decorBorderButtonTypeTheme, decorBorderButtonSizeTheme;
        int decorBorderButtonTypeUser, decorBorderButtonSizeUser;
        int decorBorderButtonType, decorBorderButtonSize;
        tColor decorBorderButtonFg, decorBorderButtonBg;

        // PROGRESS TYPES
        // 0 = small line + big line
        // 1 = big line
        // 2 = big line + outline
        // 3 = small line + big line + dot
        // 4 = big line + dot
        // 5 = big line + outline + dot
        // 6 = small line + dot
        // 7 = outline + dot
        // 8 = small line + big line + alpha blend
        // 9 = big line + alpha blend
        int decorProgressChannelByTheme;
        int decorProgressChannelTypeTheme, decorProgressChannelSizeTheme;
        int decorProgressChannelTypeUser, decorProgressChannelSizeUser;
        int decorProgressChannelType, decorProgressChannelSize;
        tColor decorProgressChannelFg, decorProgressChannelBarFg, decorProgressChannelBg;

        int decorProgressVolumeByTheme;
        int decorProgressVolumeTypeTheme, decorProgressVolumeSizeTheme;
        int decorProgressVolumeTypeUser, decorProgressVolumeSizeUser;
        int decorProgressVolumeType, decorProgressVolumeSize;
        tColor decorProgressVolumeFg, decorProgressVolumeBarFg, decorProgressVolumeBg;

        int decorProgressMenuItemByTheme;
        int decorProgressMenuItemTypeTheme, decorProgressMenuItemSizeTheme;
        int decorProgressMenuItemTypeUser, decorProgressMenuItemSizeUser;
        int decorProgressMenuItemType, decorProgressMenuItemSize;
        tColor decorProgressMenuItemFg, decorProgressMenuItemBarFg, decorProgressMenuItemBg;
        tColor decorProgressMenuItemCurFg, decorProgressMenuItemCurBarFg, decorProgressMenuItemCurBg;

        int decorProgressReplayByTheme;
        int decorProgressReplayTypeTheme, decorProgressReplaySizeTheme;
        int decorProgressReplayTypeUser, decorProgressReplaySizeUser;
        int decorProgressReplayType, decorProgressReplaySize;
        tColor decorProgressReplayFg, decorProgressReplayBarFg, decorProgressReplayBg;

        int decorProgressSignalByTheme;
        int decorProgressSignalTypeTheme, decorProgressSignalSizeTheme;
        int decorProgressSignalTypeUser, decorProgressSignalSizeUser;
        int decorProgressSignalType, decorProgressSignalSize;
        tColor decorProgressSignalFg, decorProgressSignalBarFg, decorProgressSignalBg;

        /* Types
        * 0 = left line + rect bar
        * 1 = left line + round bar
        * 2 = middle line + rect bar
        * 3 = middle line + round bar
        * 4 = outline + rect bar
        * 5 = outline + round bar
        * 6 = rect bar
        * 7 = round bar
        */
        int decorScrollBarByTheme;
        int decorScrollBarTypeTheme, decorScrollBarSizeTheme;
        int decorScrollBarTypeUser, decorScrollBarSizeUser;
        int decorScrollBarType, decorScrollBarSize;
        tColor decorScrollBarFg, decorScrollBarBarFg, decorScrollBarBg;

        // General Config
        int ButtonsShowEmpty;
        int ChannelIconsShow;
        int SignalQualityShow;
        int SignalQualityUseColors;
        int DiskUsageShow;
        int DiskUsageShort;
        // 0 = occupied
        // 1 = free
        int DiskUsageFree;

        int MenuItemPadding;
        int marginOsdVer, marginOsdHor;
        int MessageOffset;

        int MenuContentFullSize;
        double TopBarFontSize;
        double TopBarFontClockScale;

        int ChannelResolutionAspectShow;
        int ChannelFormatShow;
        int ChannelSimpleAspectFormat;
        int ChannelTimeLeft;
        int ChannelDvbapiInfoShow;
        int ChannelShowStartTime;

        int ChannelWeatherShow;
        int PlaybackWeatherShow;
        double WeatherFontSize;

        int RecordingResolutionAspectShow;
        int RecordingFormatShow;
        int RecordingSimpleAspectFormat;
        int RecordingAdditionalInfoShow;
        double TimeSecsScale;

        int RecordingDimmOnPause;
        int RecordingDimmOnPauseDelay;
        int RecordingDimmOnPauseOpaque;

        int EpgRerunsShow;
        int EpgAdditionalInfoShow;
        int TopBarRecordingShow;
        int TopBarRecConflictsShow;
        int TopBarRecConflictsHigh;
        int MenuItemIconsShow;
        int TopBarMenuIconShow;
        int TopBarHideClockText;

        int MenuChannelView;
        int MenuTimerView;
        int MenuEventView;
        int MenuRecordingView;
        int MenuFullOsd;
        int MenuEventViewAllwaysWithDate;

        int MenuRecordingShowCount;
        int MenuTimerShowCount;
        int MenuChannelShowCount;

        double MenuItemRecordingSeenTreshold;
        int MenuItemRecordingDefaultOldDays;

        // 0 = vertikal
        // 1 = horizontal
        int MessageColorPosition;

        /* hidden configs (only in setup.conf, no osd menu) */
        int MenuItemRecordingClearPercent;
        int MenuItemRecordingShowFolderDate;  // 0 = disable, 1 = newest recording date, 2 = oldest recording date
        int MenuItemParseTilde;
        int ShortRecordingCount;
        int MainMenuWidgetActiveTimerShowRemoteRefreshTime; // in seconds
        /* hidden configs (only in setup.conf, no osd menu) */

        int MenuItemRecordingShowRecordingErrors;  // 0 = disable, 1 = show recordingerror icons
        int MenuItemRecordingShowRecordingErrorsTreshold;

        // Text Scroller
        int ScrollerEnable;
        int ScrollerStep;
        int ScrollerDelay;
        int ScrollerType;

        // mainmenu widgets
        int MainMenuWidgetsEnable;
        double MainMenuItemScale;

        int MainMenuWidgetDVBDevicesShow;
        int MainMenuWidgetDVBDevicesPosition;
        int MainMenuWidgetDVBDevicesDiscardUnknown;
        int MainMenuWidgetDVBDevicesDiscardNotUsed;
        int MainMenuWidgetDVBDevicesNativeNumbering;

        int MainMenuWidgetActiveTimerShow;
        int MainMenuWidgetActiveTimerPosition;
        int MainMenuWidgetActiveTimerMaxCount;
        int MainMenuWidgetActiveTimerShowActive;
        int MainMenuWidgetActiveTimerShowRecording;
        int MainMenuWidgetActiveTimerShowRemoteActive;
        int MainMenuWidgetActiveTimerShowRemoteRecording;
        int MainMenuWidgetActiveTimerHideEmpty;

        int MainMenuWidgetLastRecShow;
        int MainMenuWidgetLastRecPosition;
        int MainMenuWidgetLastRecMaxCount;

        int MainMenuWidgetTimerConflictsShow;
        int MainMenuWidgetTimerConflictsPosition;
        int MainMenuWidgetTimerConflictsHideEmpty;

        int MainMenuWidgetSystemInfoShow;
        int MainMenuWidgetSystemInfoPosition;

        int MainMenuWidgetSystemUpdatesShow;
        int MainMenuWidgetSystemUpdatesPosition;
        int MainMenuWidgetSystemUpdatesHideIfZero;

        int MainMenuWidgetTemperaturesShow;
        int MainMenuWidgetTemperaturesPosition;

        int MainMenuWidgetCommandShow;
        int MainMenuWidgetCommandPosition;

        int MainMenuWidgetWeatherShow;
        int MainMenuWidgetWeatherPosition;
        int MainMenuWidgetWeatherType;
        int MainMenuWidgetWeatherDays;

        // TVScraper
        int TVScraperChanInfoShowPoster;
        double TVScraperChanInfoPosterSize;

        int TVScraperReplayInfoShowPoster;
        double TVScraperReplayInfoPosterSize;

        int TVScraperEPGInfoShowPoster;
        int TVScraperRecInfoShowPoster;

        int TVScraperEPGInfoShowActors;
        int TVScraperRecInfoShowActors;

        int DecorIndex;

        void Store(const char *Name, const char *Value, const char *Filename);
        void Store(const char *Name, int Value, const char *Filename);
        void Store(const char *Name, double &Value, const char *Filename);
};
