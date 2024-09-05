/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#include <vdr/menu.h>
#include <vdr/tools.h>
#include "./config.h"
#include "./services/scraper2vdr.h"

class cFlatSetup : public cMenuSetupPage {
 public:
        cFlatSetup();
        virtual ~cFlatSetup();

 protected:
        virtual void Store();
        virtual eOSState ProcessKey(eKeys Key);

 private:
        cFlatConfig SetupConfig;

        int ItemLastSel;
        void Setup();
};

class cMenuSetupSubMenu : public cOsdMenu {
 public:
        cMenuSetupSubMenu(const char *Title, cFlatConfig *data);

 protected:
        cFlatConfig *SetupConfig;
        virtual void Setup() = 0;
        cOsdItem *InfoItem(const char *label, const char *value);
        int ItemLastSel;
};

class cFlatSetupGeneral : public cMenuSetupSubMenu {
 public:
        explicit cFlatSetupGeneral(cFlatConfig *data);
        virtual eOSState ProcessKey(eKeys Key);

 protected:
        void Setup();
        void SaveCurrentSettings();
        void LoadConfigFile();
        bool SetupParse(const char *Name, const char *Value);

 private:
};

class cFlatSetupChannelInfo : public cMenuSetupSubMenu {
 public:
        explicit cFlatSetupChannelInfo(cFlatConfig *data);
        virtual eOSState ProcessKey(eKeys Key);
 
 protected:
        void Setup();
};

class cFlatSetupMenu : public cMenuSetupSubMenu {
 public:
        explicit cFlatSetupMenu(cFlatConfig *data);
        virtual eOSState ProcessKey(eKeys Key);

 protected:
        void Setup();
};

class cFlatSetupReplay : public cMenuSetupSubMenu {
 public:
        explicit cFlatSetupReplay(cFlatConfig *data);
        virtual eOSState ProcessKey(eKeys Key);

 protected:
        void Setup();
};

class cFlatSetupVolume : public cMenuSetupSubMenu {
 public:
        explicit cFlatSetupVolume(cFlatConfig *data);
        virtual eOSState ProcessKey(eKeys Key);

 protected:
        void Setup();
};

class cFlatSetupTracks : public cMenuSetupSubMenu {
 public:
        explicit cFlatSetupTracks(cFlatConfig *data);
        virtual eOSState ProcessKey(eKeys Key);

 protected:
        void Setup();
};

class cFlatSetupTVScraper : public cMenuSetupSubMenu {
 public:
        explicit cFlatSetupTVScraper(cFlatConfig *data);
        virtual eOSState ProcessKey(eKeys Key);

 protected:
        void Setup();
};

class cFlatSetupMMWidget : public cMenuSetupSubMenu {
 public:
        explicit cFlatSetupMMWidget(cFlatConfig *data);
        virtual eOSState ProcessKey(eKeys Key);

 protected:
        void Setup();
};
