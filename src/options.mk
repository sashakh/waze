
# Build options for RoadMap.  Mostly this file shouldn't need
# to be edited.  Personal changes can probably be made in config.mk

include $(TOP)/config.mk

# --- Installation options -------------------------------------------------

ifeq ($(strip $(INSTALLDIR)),)
# most things go into /usr/local by default
INSTALLDIR = /usr/local
endif

# Just two things don't go under INSTALLDIR -- and these can be
# affected by changing DESTDIR.  The things unaffected by
# INSTALLDIR are the traditional /var/lib/roadmap location for
# maps, and the menu entry installation in /usr/share/menu. 
# Changing DESTDIR will affect INSTALLDIR, and also the
# ROADMAP_CONFIG_DIR and ROADMAP_MAP_DIR variables, if they're in
# use.  ("DESTDIR" should probably be renamed "ROOTDIR".)
# DESTDIR = 

ifneq ($(strip $(DESTDIR)),)
override INSTALLDIR := $(DESTDIR)/$(INSTALLDIR)
endif

desktopdir = $(INSTALLDIR)/applications
bindir = $(INSTALLDIR)/bin
pkgbindir = $(bindir)

# if the user compiled in a new config dir, install to it
ifneq ($(strip $(ROADMAP_CONFIG_DIR)),)
pkgdatadir = $(DESTDIR)/$(ROADMAP_CONFIG_DIR)
else
pkgdatadir = $(INSTALLDIR)/share/roadmap
endif

# if the user compiled in a new map dir, create it here
ifneq ($(strip $(ROADMAP_MAP_DIR)),)
pkgmapsdir = $(DESTDIR)/$(ROADMAP_MAP_DIR)
else
pkgmapsdir = $(DESTDIR)/var/lib/roadmap
endif

menudir = $(DESTDIR)/usr/share/menu
icondir = $(INSTALLDIR)/share/pixmaps
man1dir = $(INSTALLDIR)/share/man/man1

INSTALL      = install
INSTALL_DATA = install -m644
# the image conversion tool "convert" comes with ImageMagick.
# on debian or ubuntu:  "apt-get install imagemagick"
CONVERT      = convert

# if you want to cross-compile, define CROSS in config.mk.  you
# may also have to add paths to libraries (with -L) in LDFLAGS.
CC =    $(CROSS)gcc
CXX =   $(CROSS)g++
AS =    $(CROSS)as
AR =    $(CROSS)ar
LD =    $(CROSS)ld
STRIP = $(CROSS)strip
RANLIB = $(CROSS)ranlib


# --- Build options ------------------------------------------------

ALL_RDMODULES = gtk gtk2 qt qt4 wince

ifeq ($(DESKTOP),GTK2)
	RDMODULES=gtk2
else
ifeq ($(DESKTOP),GPE)
	RDMODULES=gtk2
else
ifeq ($(DESKTOP),GTK)
	RDMODULES=gtk
else
ifeq ($(DESKTOP),QT)
	RDMODULES=qt
else
ifeq ($(DESKTOP),QT4)
	RDMODULES=qt4
else
ifeq ($(DESKTOP),QPE)
	RDMODULES=qt
else
ifeq ($(DESKTOP),QPE4)
	RDMODULES=qt4
else
ifeq ($(DESKTOP),WINCE)
	RDMODULES=win32
else
ifeq ($(DESKTOP),IPHONE)
	RDMODULES=iphone
else
	RDMODULES=$(ALL_RDMODULES)
endif
endif
endif
endif
endif
endif
endif
endif
endif

ifeq ($(DESKTOP),WINCE)
	OSDIR=win32
else
	LIBOS = $(TOP)/unix/libosroadmap.a
	OSDIR = unix
endif

ifeq ($(strip $(LANGS)),YES)
	CFLAGS += -DLANG_SUPPORT
endif

ifeq ($(strip $(MODE)),DEBUG)
	CFLAGS += -g

	# ROADMAP_MTRACE turns on leak detection using mtrace:
	# Do not forget to set the trace file using the env. 
	# variable MALLOC_TRACE, then use the mtrace tool to
	# analyze the output.
	#
	# (another excellent tool for this purpose, by the way,
	#  is "valgrind", which needs no special compilation --
	#  it works by replacing shared libraries.)
	#
	CFLAGS += -DROADMAP_MTRACE

	# ROADMAP_LISTS_TYPESAFE forces "type safety" for the
	# list manipulation code, which can help catch bugs earlier.
	CFLAGS += -DROADMAP_LISTS_TYPESAFE

	# ROADMAP_INDEX_DEBUG does some sanity checking on various
	# table indices before using them.
	CFLAGS += -DROADMAP_INDEX_DEBUG
else
ifeq ($(strip $(MODE)),PROFILE)
	CFLAGS += -g -pg -fprofile-arcs -g
	LIBS += -pg
else
	CFLAGS += -O -DNDEBUG
ifneq ($(DESKTOP),WINCE)
	CFLAGS += -fomit-frame-pointer
endif
endif
endif

WARNFLAGS = -W \
	-Wall \
	-Wno-unused-parameter \
	-Wcast-align \
	-Wreturn-type \
	-Wsign-compare

#	 -Wpointer-arith \

CFLAGS += $(WARNFLAGS) 


RDMLIBS= $(TOP)/libroadmap.a \
	$(LIBOS) \
	$(TOP)/gpx/libgpx.a \
	$(TOP)/libroadmap.a 

LIBS += $(RDMLIBS)

ifneq ($(strip $(ROADMAP_CONFIG_DIR)),)
	CFLAGS += -DROADMAP_CONFIG_DIR=\"$(ROADMAP_CONFIG_DIR)\"
endif
ifneq ($(strip $(ROADMAP_MAP_DIR)),)
	CFLAGS += -DROADMAP_MAP_DIR=\"$(ROADMAP_MAP_DIR)\"
endif

# expat library, for xml GPX format
ifneq ($(strip $(EXPAT)),NO)
	LIBS += -lexpat 
	CFLAGS += -DROADMAP_USES_EXPAT
endif

# shapefile support needed for building some mapsets
ifneq ($(strip $(SHAPEFILES)),NO)
	CFLAGS += -DROADMAP_USE_SHAPEFILES
	LIBS += -lshp
endif

# rotation support in QT/QPE?
ifeq ($(strip $(QT_NO_ROTATE)),YES)
	CFLAGS += -DQT_NO_ROTATE
endif


# each DESKTOP version has a fully-"native" canvas
# implementation, as well as a possible agg-based implementation.
AGG_CHOICES = rgb565 rgba32 bgra32 rgb24 bgr24

ifeq ($(strip $(AGG)),NO)
	CANVAS_OBJS = roadmap_canvas.o
	GTK2CC = $(CC)
else

ifeq ($(findstring $(strip $(AGG)), $(AGG_CHOICES)),)
$(info Valid AGG values: $(AGG_CHOICES), or NO)
$(error Sorry, 'AGG = $(strip $(AGG))' isn't valid)
 else
	LIBS += -laggfontfreetype -lagg -lfreetype
	CFLAGS += -DAGG_PIXFMT=pixfmt_$(AGG) \
		-I$(TOP)/agg_support \
		-I/usr/include/agg2 \
		-I/usr/local/include/agg2 \
		-I/usr/include/freetype2
	CANVAS_OBJS = roadmap_canvas_agg.o \
		$(TOP)/agg_support/roadmap_canvas.o
	GTK2CC = $(CXX)
 endif

endif

# bidirectional text lib
ifneq ($(strip $(BIDI)),NO)
	LIBS += -lfribidi
	CFLAGS += -DUSE_FRIBIDI -I/usr/include/fribidi
endif

# RoadMap internal profiling
ifeq ($(strip $(DBG_TIME)),YES)
	CFLAGS += -DROADMAP_DBG_TIME
endif


HOST=`uname -s`
ifeq ($(HOST),Darwin)
	ARFLAGS="r"
else
ifeq ($(DESKTOP),IPHONE)
	ARFLAGS="r"
else
	ARFLAGS="rf"
endif
endif


ifeq ($(DESKTOP),WINCE)
	CFLAGS += -I$(TOP) -I$(TOP)/win32 \
		-DNDEBUG -D_WIN32_WCE=0x0300 -D_WIN32_IE=0x0300 \
		-D_TXT=\".txt\" -D_EXE=\".exe\"
	LIBS := $(LIBS) -lm
else
	CFLAGS += -I$(TOP) -I/usr/local/include \
		  -D_TXT=\"\" -D_EXE=\"\"
	LIBS := -L/usr/local/lib $(LIBS) -lm
endif

CXXFLAGS = $(CFLAGS)
