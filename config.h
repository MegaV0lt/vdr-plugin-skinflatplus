/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/plugin.h>
#include <vdr/tools.h>

#include <string>
#include <vector>

#define PLUGINCONFIGPATH (cPlugin::ConfigDirectory(PLUGIN_NAME_I18N))
#define PLUGINRESOURCEPATH (cPlugin::ResourceDirectory(PLUGIN_NAME_I18N))
#define WIDGETOUTPUTPATH "/tmp/skinflatplus/widgets"

bool CompareNumStrings(const cString &left, const cString &right);
bool StringCompare(const std::string &left, const std::string &right);
bool PairCompareTimeString(const std::pair<time_t, cString>&i, const std::pair<time_t, cString>&j);
bool PairCompareIntString(const std::pair<int, std::string>&i, const std::pair<int, std::string>&j);  // NOLINT
int RoundUp(int NumToRound, int multiple);

class cFlatConfig {
 public:
        cFlatConfig();
        ~cFlatConfig();

        void Init();
        void SetLogoPath(cString path);
        bool SetupParse(const char *Name, const char *Value);

        void ThemeCheckAndInit();
        void ThemeInit();
        void DecorDescriptions(cStringList &Decors);  // NOLINT
        cString DecorDescription(cString File);
        void DecorLoadFile(cString File);
        void DecorLoadCurrent();
        void DecorCheckAndInit();

        void GetConfigFiles(cStringList &Files);  // NOLINT

        void RecordingOldLoadConfig();
        int GetRecordingOldValue(const std::string &folder);

        cString ThemeCurrent {""};
        cString LogoPath {""};
        cString IconPath {""};
        cString RecordingOldConfigFile {""};

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

        int decorBorderChannelByTheme {1};
        int decorBorderChannelTypeTheme, decorBorderChannelSizeTheme;
        int decorBorderChannelTypeUser {0}, decorBorderChannelSizeUser {0};
        int decorBorderChannelType, decorBorderChannelSize;
        tColor decorBorderChannelFg, decorBorderChannelBg;

        int decorBorderChannelEPGByTheme {1};
        int decorBorderChannelEPGTypeTheme, decorBorderChannelEPGSizeTheme;
        int decorBorderChannelEPGTypeUser {0}, decorBorderChannelEPGSizeUser {0};
        int decorBorderChannelEPGType, decorBorderChannelEPGSize;
        tColor decorBorderChannelEPGFg, decorBorderChannelEPGBg;

        int decorBorderTopBarByTheme {1};
        int decorBorderTopBarTypeTheme, decorBorderTopBarSizeTheme;
        int decorBorderTopBarTypeUser {0}, decorBorderTopBarSizeUser {0};
        int decorBorderTopBarType, decorBorderTopBarSize;
        tColor decorBorderTopBarFg, decorBorderTopBarBg;

        int decorBorderMessageByTheme {1};
        int decorBorderMessageTypeTheme, decorBorderMessageSizeTheme;
        int decorBorderMessageTypeUser {0}, decorBorderMessageSizeUser {0};
        int decorBorderMessageType, decorBorderMessageSize;
        tColor decorBorderMessageFg, decorBorderMessageBg;

        int decorBorderVolumeByTheme {1};
        int decorBorderVolumeTypeTheme, decorBorderVolumeSizeTheme;
        int decorBorderVolumeTypeUser {0}, decorBorderVolumeSizeUser {0};
        int decorBorderVolumeType, decorBorderVolumeSize;
        tColor decorBorderVolumeFg, decorBorderVolumeBg;

        int decorBorderTrackByTheme {1};
        int decorBorderTrackTypeTheme, decorBorderTrackSizeTheme;
        int decorBorderTrackTypeUser {0}, decorBorderTrackSizeUser {0};
        int decorBorderTrackType, decorBorderTrackSize;
        tColor decorBorderTrackFg, decorBorderTrackBg;
        tColor decorBorderTrackSelFg, decorBorderTrackSelBg;
        tColor decorBorderTrackCurFg, decorBorderTrackCurBg;

        int decorBorderReplayByTheme {1};
        int decorBorderReplayTypeTheme, decorBorderReplaySizeTheme;
        int decorBorderReplayTypeUser {0}, decorBorderReplaySizeUser {0};
        int decorBorderReplayType, decorBorderReplaySize;
        tColor decorBorderReplayFg, decorBorderReplayBg;

        int decorBorderMenuItemByTheme {1};
        int decorBorderMenuItemTypeTheme, decorBorderMenuItemSizeTheme;
        int decorBorderMenuItemTypeUser {0}, decorBorderMenuItemSizeUser {0};
        int decorBorderMenuItemType, decorBorderMenuItemSize;
        tColor decorBorderMenuItemFg, decorBorderMenuItemBg;
        tColor decorBorderMenuItemSelFg, decorBorderMenuItemSelBg;
        tColor decorBorderMenuItemCurFg, decorBorderMenuItemCurBg;

        int decorBorderMenuContentHeadByTheme {1};
        int decorBorderMenuContentHeadTypeTheme, decorBorderMenuContentHeadSizeTheme;
        int decorBorderMenuContentHeadTypeUser {0}, decorBorderMenuContentHeadSizeUser {0};
        int decorBorderMenuContentHeadType, decorBorderMenuContentHeadSize;
        tColor decorBorderMenuContentHeadFg, decorBorderMenuContentHeadBg;

        int decorBorderMenuContentByTheme {1};
        int decorBorderMenuContentTypeTheme, decorBorderMenuContentSizeTheme;
        int decorBorderMenuContentTypeUser {0}, decorBorderMenuContentSizeUser {0};
        int decorBorderMenuContentType, decorBorderMenuContentSize;
        tColor decorBorderMenuContentFg, decorBorderMenuContentBg;

        int decorBorderButtonByTheme {1};
        int decorBorderButtonTypeTheme, decorBorderButtonSizeTheme;
        int decorBorderButtonTypeUser {0}, decorBorderButtonSizeUser {0};
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

        int decorProgressChannelByTheme {1};
        int decorProgressChannelTypeTheme, decorProgressChannelSizeTheme;
        int decorProgressChannelTypeUser {0}, decorProgressChannelSizeUser {20};
        int decorProgressChannelType, decorProgressChannelSize;
        tColor decorProgressChannelFg, decorProgressChannelBarFg, decorProgressChannelBg;

        int decorProgressVolumeByTheme {1};
        int decorProgressVolumeTypeTheme, decorProgressVolumeSizeTheme;
        int decorProgressVolumeTypeUser {0}, decorProgressVolumeSizeUser {20};
        int decorProgressVolumeType, decorProgressVolumeSize;
        tColor decorProgressVolumeFg, decorProgressVolumeBarFg, decorProgressVolumeBg;

        int decorProgressMenuItemByTheme {1};
        int decorProgressMenuItemTypeTheme, decorProgressMenuItemSizeTheme;
        int decorProgressMenuItemTypeUser {0}, decorProgressMenuItemSizeUser {20};
        int decorProgressMenuItemType, decorProgressMenuItemSize;
        tColor decorProgressMenuItemFg, decorProgressMenuItemBarFg, decorProgressMenuItemBg;
        tColor decorProgressMenuItemCurFg, decorProgressMenuItemCurBarFg, decorProgressMenuItemCurBg;

        int decorProgressReplayByTheme {1};
        int decorProgressReplayTypeTheme, decorProgressReplaySizeTheme;
        int decorProgressReplayTypeUser {0}, decorProgressReplaySizeUser {40};
        int decorProgressReplayType, decorProgressReplaySize;
        tColor decorProgressReplayFg, decorProgressReplayBarFg, decorProgressReplayBg;

        int decorProgressSignalByTheme {1};
        int decorProgressSignalTypeTheme, decorProgressSignalSizeTheme;
        int decorProgressSignalTypeUser {0}, decorProgressSignalSizeUser {20};
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
        int decorScrollBarByTheme {0};
        int decorScrollBarTypeTheme {0}, decorScrollBarSizeTheme {20};
        int decorScrollBarTypeUser {0}, decorScrollBarSizeUser {20};
        int decorScrollBarType, decorScrollBarSize;
        tColor decorScrollBarFg, decorScrollBarBarFg, decorScrollBarBg;

        // General Config
        int ButtonsShowEmpty {true};
        int ChannelIconsShow {true};  // Show format and aspect icons in channel status bar

        int SignalQualityShow {true};
        int SignalQualityUseColors {false};

        int DiskUsageShow {1};  // 0 = disabled, 1 = Timer and recording menu, 2 = Always in menu, 3 = Always
        int DiskUsageShort {false};
        int DiskUsageFree {1};  // 0 = occupied, 1 = free, 2 = special mode free time instead of used

        int MenuContentFullSize {true};

        int MenuItemPadding {3};
        int marginOsdVer {5}, marginOsdHor {5};
        int MessageOffset {50};

        double TopBarFontSize {5.0 / 100};
        double TopBarFontClockScale {1.0 / 100};

        int ChannelResolutionAspectShow {true};
        int ChannelFormatShow {true};
        int ChannelSimpleAspectFormat {true};
        int ChannelTimeLeft {0};
        int ChannelDvbapiInfoShow {1};
        int ChannelShowNameWithShadow {false};  // Show channel name and number with shadow instead of background
        int ChannelShowStartTime {true};

        int ChannelWeatherShow {1};
        int PlaybackWeatherShow {1};
        double WeatherFontSize {5.0 / 100};

        int RecordingResolutionAspectShow {true};
        int RecordingFormatShow {true};
        int RecordingSimpleAspectFormat {true};
        int RecordingAdditionalInfoShow {true};
        double TimeSecsScale {1.0};

        int RecordingDimmOnPause {true};
        int RecordingDimmOnPauseDelay {20};
        int RecordingDimmOnPauseOpaque {240};

        double EpgFskGenreIconSize {0.5 / 100};  // Percent of content head height
        int EpgRerunsShow {true};
        int EpgAdditionalInfoShow {true};

        int TopBarRecordingShow {true};
        int TopBarRecConflictsShow {true};
        int TopBarRecConflictsHigh {2};

        int MenuItemIconsShow {true};
        int TopBarMenuIconShow {true};
        int TopBarHideClockText {0};

        int MenuChannelView {1};  // 0 = disable (vdr), 1 = long, 2 = long + epg, 3 = short, 4 = short +epg
        int MenuTimerView {1};  // 0 = disable (vdr), 1 = long, 2 = short, 3 = short +epg
        int MenuEventView {1};  // 0 = disable (vdr), 1 = long, 2 = short, 3 = short +epg
        int MenuEventRecordingViewJustify {0};  // 0 = disable, 1 = Justify wrapped text
        int MenuRecordingView {1};  // 0 = disable (vdr), 1 = long, 2 = short, 3 = short +epg
        int MenuFullOsd {0};
        int MenuEventViewAlwaysWithDate {1};

        int MenuRecordingShowCount {1};  // Show number of recordings in menu recordings
        int MenuTimerShowCount {1};      // Show number of timers in menu timers
        int MenuChannelShowCount {1};    // Show number of channels in menu channels

        double MenuItemRecordingSeenThreshold {0.98 / 100.0};
        int MenuItemRecordingDefaultOldDays {-1};

        int MessageColorPosition {1};  // 0 = vertical, 1 = horizontal

        //* Hidden configs (only in setup.conf, no osd menu)
        int MenuItemRecordingClearPercent {1};    // Remove % sign in front of a edited recording
        int MenuItemRecordingShowFolderDate {1};  // 0 = disable, 1 = newest recording date, 2 = oldest recording date
        int MenuItemParseTilde {1};               // Display string after ~ in different color and remove ~
        int ShortRecordingCount {0};              // Simpler display of counts in menu recordings
        int MainMenuWidgetActiveTimerShowRemoteRefreshTime {60 * 30};  // Every 30 minutes
        //* Hidden configs (only in setup.conf, no osd menu)

        int MenuItemRecordingShowRecordingErrors {1};  // 0 = disable, 1 = show recording error icons
        int PlaybackShowRecordingErrors {true};        // Show error marks in replay progressbar
        int PlaybackShowErrorMarks {1};                // Types: '|' (1, 2), 'I' (3, 4) and '+' (5, 6) small/big
        int PlaybackShowRecordingDate {true};          // Show date and time with short text
        int PlaybackShowEndTime {0};                   // Show end time of recording
        int MenuItemRecordingShowRecordingErrorsThreshold {1000};  // Threshold for displaying error instead of warning
        int MenuItemRecordingShowFormatIcons {1};      // Show format icons (sd/hd/uhd) in menu recordings

        // Text scroller
        int ScrollerEnable {1};
        int ScrollerStep {2};
        int ScrollerDelay {25};  // In ms
        int ScrollerType {0};

        // Main menu widgets
        int MainMenuWidgetsEnable {1};
        double MainMenuItemScale {0.5};

        int MainMenuWidgetWeatherShow {true};
        int MainMenuWidgetWeatherPosition {1};
        int MainMenuWidgetWeatherType {0};
        int MainMenuWidgetWeatherDays {5};

        int MainMenuWidgetDVBDevicesShow {true};
        int MainMenuWidgetDVBDevicesPosition {2};
        int MainMenuWidgetDVBDevicesDiscardUnknown {true};
        int MainMenuWidgetDVBDevicesDiscardNotUsed {false};
        int MainMenuWidgetDVBDevicesNativeNumbering {false};  // Display device numbers from 1..x

        int MainMenuWidgetActiveTimerShow {true};
        int MainMenuWidgetActiveTimerPosition {3};
        int MainMenuWidgetActiveTimerMaxCount {2};
        int MainMenuWidgetActiveTimerShowActive {true};
        int MainMenuWidgetActiveTimerShowRecording {true};
        int MainMenuWidgetActiveTimerShowRemoteActive {false};
        int MainMenuWidgetActiveTimerShowRemoteRecording {false};
        int MainMenuWidgetActiveTimerHideEmpty {false};

        int MainMenuWidgetLastRecShow {false};
        int MainMenuWidgetLastRecPosition {4};
        int MainMenuWidgetLastRecMaxCount {3};

        int MainMenuWidgetTimerConflictsShow {false};
        int MainMenuWidgetTimerConflictsPosition {5};
        int MainMenuWidgetTimerConflictsHideEmpty {false};

        int MainMenuWidgetSystemInfoShow {true};
        int MainMenuWidgetSystemInfoPosition {6};

        int MainMenuWidgetSystemUpdatesShow {true};
        int MainMenuWidgetSystemUpdatesPosition {7};
        int MainMenuWidgetSystemUpdatesHideIfZero {0};

        int MainMenuWidgetTemperaturesShow {true};
        int MainMenuWidgetTemperaturesPosition {8};

        int MainMenuWidgetCommandShow {true};
        int MainMenuWidgetCommandPosition {9};

        // TVScraper
        int TVScraperChanInfoShowPoster {1};
        double TVScraperChanInfoPosterSize {1.0 / 100};

        int TVScraperReplayInfoShowPoster {1};
        double TVScraperReplayInfoPosterSize {1.0 / 100};
        double TVScraperPosterOpacity {0.8 / 100};  // Opacitiy of poster in replay info and channel info (80%)

        int TVScraperEPGInfoShowPoster {1};
        int TVScraperRecInfoShowPoster {1};

        int TVScraperEPGInfoShowActors {1};
        int TVScraperRecInfoShowActors {1};
        int TVScraperShowMaxActors {-1};  // -1 Show all actors, 0 = disable, 1+ = max actors to show

        int DecorIndex {0};

        void Store(const char *Name, const char *Value, const char *Filename);
        void Store(const char *Name, int Value, const char *Filename);
        void Store(const char *Name, double &Value, const char *Filename);  // NOLINT

 private:
        cString CheckSlashAtEnd(std::string path);

        int m_DecorCurrent {-1};
};
