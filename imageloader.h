/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#pragma once

#define X_DISPLAY_MISSING

#include <vdr/osd.h>
#include <vdr/skins.h>
#include <vdr/tools.h>

#include <string>

#include <Magick++.h>

// Must be included after Magick++ (which includes <ft2build.h>)
#include "./imagemagickwrapper.h"

// using namespace Magick;
using Magick::Image;  // Import only the Image class from the Magick namespace
using Magick::Color;  // Import only the Color class from the Magick namespace

class cImageLoader : public cImageMagickWrapper {
 public:
    cImageLoader();
    ~cImageLoader();

    cImage* GetLogo(const char *logo, int width, int height, bool MissingOk = false);
    cImage* GetIcon(const char *cIcon, int width, int height);
    cImage* GetFile(const char *cFile, int width, int height);
    bool SearchRecordingPoster(const cString &RecPath, cString &found);  // NOLINT

 private:
    const cString m_LogoExtension {"png"};
    void ToLowerCase(std::string &str);  // NOLINT
    bool CheckImageExistence(const cString &RecPath, const cString &Image, cString &found);  // NOLINT
};
