#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
# IMPORTANT: the presence of this macro is important for the Make.config
# file. So it must be defined, even if it is not used here!
#
PLUGIN = skinflatplus

# define vdrlogo for menu topbar
# if not defined vdrlogo_default will be used
# define here or use make VDRLOGO=\"vdrlogo_default\"
# available logos are
#   vdrlogo_debian
#   vdrlogo_default
#   vdrlogo_easyvdr
#   vdrlogo_gen2vdr
#   vdrlogo_shine
#   vdrlogo_xubuntu
#   vdrlogo_xubuntu2
#   vdrlogo_yavdr
SKINFLATPLUS_VDRLOGO = vdrlogo_default

#DEFINES += -DDEBUGIMAGELOADTIME
#DEFINES += -DDEBUGEPGTIME

# External image lib to use: imagemagick, graphicsmagick
IMAGELIB ?= graphicsmagick

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG  = $(call PKGCFG,plgcfg)
VDRCONFDIR = $(call PKGCFG,configdir)
PLGRESDIR = $(call PKGCFG,resdir)/plugins/$(PLUGIN)
TMPDIR ?= /tmp
SKINFLATPLUS_WIDGETDIR ?= $(LIBDIR)/$(PLUGIN)/widgets

### The compiler options:
export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags)

CXXFLAGS += -std=c++11

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The version number of VDR's plugin API:
APIVERSION = $(call PKGCFG,apiversion)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines and Dependencies (add further entries here):

ifeq ($(IMAGELIB), imagemagick)
  INCLUDES += $(shell pkg-config --cflags Magick++ freetype2 fontconfig)
  LIBS += $(shell pkg-config --libs Magick++)
  DEFINES += -DIMAGEMAGICK
else ifeq ($(IMAGELIB), graphicsmagick)
  INCLUDES += $(shell pkg-config --cflags GraphicsMagick++ freetype2 fontconfig)
  LIBS += $(shell pkg-config --libs GraphicsMagick++)
else
  $(error ERROR: IMAGELIB must be either imagemagick or graphicsmagick)
endif

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"' -DVDRLOGO=\"$(SKINFLATPLUS_VDRLOGO)\"
DEFINES += -DWIDGETFOLDER='"$(SKINFLATPLUS_WIDGETDIR)"'

### The object files (add further files here):

OBJS = config.o setup.o imagecache.o imagescaler.o imagemagickwrapper.o imageloader.o baserender.o complexcontent.o textscroller.o displaychannel.o displaymenu.o displaymessage.o \
	   displayreplay.o displaytracks.o displayvolume.o flat.o $(PLUGIN).o

### The main target:

all: $(SOFILE) i18n

### Implicit rules:

%.o: %.c
	$(CXX) -Wall $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

$(SOFILE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install-themes:
	mkdir -p $(DESTDIR)$(VDRCONFDIR)/themes
	cp themes/* $(DESTDIR)$(VDRCONFDIR)/themes

install-decors:
	mkdir -p $(DESTDIR)$(PLGRESDIR)/decors
	cp decors/* $(DESTDIR)$(PLGRESDIR)/decors

install-icons:
	mkdir -p $(DESTDIR)$(PLGRESDIR)/icons
	cp -r --remove-destination icons/* $(DESTDIR)$(PLGRESDIR)/icons

install-configs:
	mkdir -p $(DESTDIR)$(VDRCONFDIR)/plugins/$(PLUGIN)/configs
	cp configs/* $(DESTDIR)$(VDRCONFDIR)/plugins/$(PLUGIN)/configs

install-widgets:
	mkdir -p $(DESTDIR)$(SKINFLATPLUS_WIDGETDIR)
	cp -r widgets/* $(DESTDIR)$(SKINFLATPLUS_WIDGETDIR)

install: install-lib install-i18n install-themes install-icons install-decors install-configs install-widgets

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude .git* --exclude *.o --exclude *.rej --exclude *.orig -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~
