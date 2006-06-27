
# Build options for RoadMap.  Mostly this file shouldn't need
# to be edited.  Personal changes can probably be made in config.mk

include $(TOP)/config.mk

# --- Installation options -------------------------------------------------

DESTDIR=
INSTALLDIR=/usr/local
desktopdir=$(INSTALLDIR)/applications
pkgdatadir=$(INSTALLDIR)/share/roadmap
pkgmapsdir=/var/lib/roadmap
bindir=$(INSTALLDIR)/bin
pkgbindir=$(bindir)
menudir=$(DESTDIR)/usr/lib/menu
ICONDIR=$(INSTALLDIR)/share/pixmaps
mandir=$(INSTALLDIR)/share/man
man1dir=$(mandir)/man1

INSTALL      = install
INSTALL_DATA = install -m644


# --- Build options ------------------------------------------------

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
ifeq ($(DESKTOP),QPE)
	RDMODULES=qt
else
	RDMODULES=gtk gtk2 qt
endif
endif
endif
endif
endif


RANLIB=ranlib
STRIP=strip

ifeq ($(strip $(MODE)),DEBUG)
	# Memory leak detection using mtrace:
	# Do not forget to set the trace file using the env. 
	# variable MALLOC_TRACE, then use the mtrace tool to
	# analyze the output.
	CFLAGS += -g -DROADMAP_DEBUG_HEAP
else
ifeq ($(strip $(MODE)),PROFILE)
	CFLAGS += -g -pg -fprofile-arcs -g
	LIBS += -pg
else
	CFLAGS += -O2 -ffast-math -fomit-frame-pointer
endif
endif

WARNFLAGS = -W \
	-Wall \
	-Wno-unused-parameter \
	-Wcast-align \
	-Wpointer-arith \
	-Wreturn-type \
	-Wsign-compare

CFLAGS += $(WARNFLAGS) 


RDMLIBS= $(TOP)/libroadmap.a \
	$(TOP)/unix/libosroadmap.a  \
	$(TOP)/gpx/libgpx.a \
	$(TOP)/libroadmap.a 

LIBS += $(RDMLIBS)

# expat library, for xml GPX format
ifneq ($(strip $(EXPAT)),NO)
	LIBS += -lexpat 
	CFLAGS += -DROADMAP_USES_EXPAT
endif

# shapefiles for some mapsets
ifneq ($(strip $(SHAPEFILES)),NO)
	CFLAGS += -DROADMAP_USE_SHAPEFILES
	LIBS += -lshp
endif

# rotation support in QT/QPE
ifeq ($(strip $(DESKTOP)),QT)
	CFLAGS += -DANGLED_LABELS=1
endif
ifeq ($(strip $(DESKTOP)),QPE)
	CFLAGS += -DANGLED_LABELS=1
endif


# later...
# ifneq ($(NAVIGATE),NO)
# 	LIBS += -L$(TOP)/dglib -ldgl
# endif

# each DESKTOP version has a fully-"native" canvas
# implementation, as well as an agg-based implementation.
ifeq ($(strip $(AGG)),NO)
	CANVAS_OBJS = roadmap_canvas.o
else
	LIBS += -lagg -lfreetype
	CFLAGS += -DANGLED_LABELS=1 \
		-I$(TOP)/agg_support \
		-I/usr/include/agg2 \
		-I/usr/local/include/agg2 \
		-I/usr/include/freetype2
	CANVAS_OBJS = roadmap_canvas_agg.o \
		$(TOP)/agg_support/roadmap_canvas.o \
		$(TOP)/agg_support/agg_font_freetype.o
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
	ARFLAGS="rf"
endif


CFLAGS += -I$(TOP) -I/usr/local/include -DNDEBUG

LIBS := -lpopt -L/usr/local/lib $(LIBS) -lm

CXXFLAGS = $(CFLAGS)
