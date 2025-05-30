/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include <getopt.h>
#include <vdr/plugin.h>
#include <Magick++.h>

#if defined(APIVERSNUM) && APIVERSNUM < 20308
#error "VDR-2.3.8 API version or greater is required!"
#endif

#include "./flat.h"
#include "./setup.h"
#include "./imageloader.h"

static const char *VERSION        = "1.1.11";
static const char *DESCRIPTION    = "Skin flatPlus";

class cPluginFlat : public cPlugin {
 public:
        cPluginFlat();
        ~cPluginFlat() override;
        const char *Version() override { return VERSION; }
        const char *Description() override { return DESCRIPTION; }
        const char *CommandLineHelp() override;
        bool ProcessArgs(int argc, char *argv[]) override;
        bool Initialize() override;
        bool Start() override;
        void Stop() override;
        void Housekeeping() override;
        // void MainThreadHook() override;  // Deprecated in VDR 2.7.4
        cString Active() override;
        time_t WakeupTime() override;
        const char *MainMenuEntry() override { return nullptr; }
        cOsdObject *MainMenuAction() override;
        cMenuSetupPage *SetupMenu() override;
        bool SetupParse(const char *Name, const char *Value) override;
        bool Service(const char *Id, void *Data = nullptr) override;
        const char **SVDRPHelpPages() override;
        cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) override;  // NOLINT

 private:
        cFlat *flat;
};

cPluginFlat::cPluginFlat() {
    flat = nullptr;
}

cPluginFlat::~cPluginFlat() {}

const char *cPluginFlat::CommandLineHelp() {
    return "  -l <LOGOPATH>, --logopath=<LOGOPATH>       Set directory where Channel Logos are stored.\n";
}

bool cPluginFlat::ProcessArgs(int argc, char *argv[]) {
    // Implement command line argument processing here if applicable.
    static const struct option long_options[] = {
        { "logopath", required_argument, NULL, 'l' },
        { NULL }
    };

    int c {0};
    while ((c = getopt_long(argc, argv, "l:", long_options, NULL)) != -1) {
        switch (c) {
            case 'l':
                Config.SetLogoPath(cString(optarg));
                break;
            default:
                return false;
        }
    }
    return true;
}

__attribute__((constructor)) static void init() {
#ifndef IMAGEMAGICK
    // From skin nopacity: Prevents *magick from occupying segfaults
    MagickLib::InitializeMagickEx(NULL, MAGICK_OPT_NO_SIGNAL_HANDER, NULL);
#else
    Magick::InitializeMagick(NULL);
#endif
}

bool cPluginFlat::Initialize() {
    Config.Init();
    return true;
}

bool cPluginFlat::Start() {
    if (!cOsdProvider::SupportsTrueColor()) {
        esyslog("flatPlus: No TrueColor OSD found! Aborting!");
        return false;
    } else {
        dsyslog("flatPlus: TrueColor OSD found");
    }

    ImgCache.Create();
    ImgCache.PreLoadImage();

    flat = new cFlat;
    return flat;
}

void cPluginFlat::Stop() {
    ImgCache.Clear();
}

void cPluginFlat::Housekeeping() {
}

// void cPluginFlat::MainThreadHook() {}  // Deprecated in VDR 2.7.4

cString cPluginFlat::Active() {
    return NULL;
}

time_t cPluginFlat::WakeupTime() {
    return 0;
}

cOsdObject *cPluginFlat::MainMenuAction() {
    return nullptr;
}

cMenuSetupPage *cPluginFlat::SetupMenu() {
    return new cFlatSetup();
}

bool cPluginFlat::SetupParse(const char *Name, const char *Value) {
    return Config.SetupParse(Name, Value);
}

bool cPluginFlat::Service(const char *Id, void *Data) {
    return false;
}

const char **cPluginFlat::SVDRPHelpPages() {
    static const char *HelpPages[] = {
        "RLFC\n"
        "    Remove Logo From Cache.\n"
        "    Specify logo filename with extension, but without path",
        NULL
    };
    return HelpPages;
}

cString cPluginFlat::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) {
    if (!strcasecmp(Command, "RLFC")) {
        if (!Option || !strcmp(Option, "")) {
            ReplyCode = 500;
            return "No logo given";
        }

        if (ImgCache.RemoveFromCache(Option)) {
            ReplyCode = 900;
            return "Successfully remove logo from cache";
        } else {
            ReplyCode = 501;
            return "Failure - Logo not found in cache";
        }
    }
    return NULL;
}

VDRPLUGINCREATOR(cPluginFlat);  // Don't touch this!
