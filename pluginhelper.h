/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#pragma once

// Forward declaration to reduce dependencies
class cPlugin;

class cFlatPluginHelper {
 private:
    static cFlatPluginHelper *s_instance;
    cPlugin *m_pEpgSearchPlugin;
    bool m_bEpgSearchPluginChecked;

    cFlatPluginHelper() : m_pEpgSearchPlugin(nullptr), m_bEpgSearchPluginChecked(false) {}

 public:
    static cFlatPluginHelper* Instance() {
        if (!s_instance) s_instance = new cFlatPluginHelper();
        return s_instance;
    }

    cPlugin* GetEpgSearchPlugin();
};
