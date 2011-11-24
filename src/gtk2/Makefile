# --- Installation options -------------------------------------------------

DESTDIR=
INSTALLDIR=/usr/local
bindir=$(INSTALLDIR)/bin


# --- Tool specific options ------------------------------------------------

RANLIB=ranlib
STRIP=strip

GUICFLAGS=`pkg-config --cflags gtk+-2.0`
GUILDFLAGS=`pkg-config --libs gtk+-2.0` $(X11LDFLAGS)


WARNFLAGS = -W -Wall -Wno-unused-parameter

ifeq ($(MODE),DEBUG)
   STDCFLAGS=-g $(WARNFLAGS) -DROADMAP_DEBUG_HEAP
else
ifeq ($(MODE),PROFILE)
   STDCFLAGS=-g $(WARNFLAGS) -pg -fprofile-arcs -g
   LDFLAGS=-pg $(LDFLAGS)
else
   STDCFLAGS=-O2 -ffast-math -fomit-frame-pointer $(WARNFLAGS) $(OPTIONS)
endif
endif

ifeq ($(DESKTOP),GPE)
	GPECFLAGS=-DROADMAP_USES_GPE
	GPELDFLAGS=-lgpewidget -ldisplaymigration
else
	GPECFLAGS=
	GPELDFLAGS=
endif

ifeq ($(SSD),YES)
CFLAGS += -DSSD
SSDLIB=../libssd_widgets.a
endif

RDMLIBS= ../libroadmap.a libgtkroadmap.a ../unix/libosroadmap.a ../libroadmap.a libgtkroadmap.a


HOST=`uname -s`
ifeq ($(HOST),Darwin)
	ARFLAGS="r"
else
	ARFLAGS="rf"
endif


CFLAGS = $(STDCFLAGS) $(GPECFLAGS) $(GUICFLAGS) -I.. -I/usr/local/include -DNDEBUG -w
LIBS = -L/usr/local/lib $(RDMLIBS) $(GPELDFLAGS) $(GUILDFLAGS) -lm -lagg -lfreetype -lpng

ifeq ($(BIDI),YES)
LIBS += -lfribidi
CFLAGS += -DUSE_FRIBIDI
endif

RUNTIME=gtkroadmap gtkroadgps

ifeq ($(FREEMAP_IL),YES)
CFLAGS += -DFREEMAP_IL
RUNTIME=gtkroadmap
endif

CXXFLAGS = $(CFLAGS) -I/usr/include/agg2 -I/usr/include/freetype2 -I/usr/include/fribidi
# --- RoadMap sources & targets -----------------------------------

RMLIBSRCS=roadmap_keyboard.c \
          roadmap_dialog.c \
          roadmap_fileselection.c

ifneq ($(SSD),YES)
RMLIBSRCS += roadmap_messagebox.c
endif


RMLIBOBJS=$(RMLIBSRCS:.c=.o) roadmap_canvas_agg.o ../agg/roadmap_canvas.o ../agg/font_freetype/agg_font_freetype.o roadmap_libpng.o


# --- Conventional targets ----------------------------------------

.PHONY: all others build runtime clean cleanone install uninstall ../libssd_widgets.a

all: runtime

others:
	$(MAKE) -C .. CFLAGS="$(STDCFLAGS)" runtime
	$(MAKE) -C .. -C unix CFLAGS="$(STDCFLAGS) -I.." runtime

build:
	$(MAKE) -C .. CFLAGS="$(STDCFLAGS)" build

runtime: others $(RUNTIME)

clean: cleanone
	$(MAKE) -C .. cleanone
	$(MAKE) -C .. -C unix cleanone

cleanone:
	rm -f *.o *.a *.da $(RUNTIME)

install: all
	$(MAKE) -C .. install
	$(STRIP) $(RUNTIME)
	mkdir -p $(DESTDIR)$(bindir)
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME) roadmap roadgps
	install $(RUNTIME) $(DESTDIR)$(bindir)
	ln -s gtkroadmap $(DESTDIR)$(bindir)/roadmap
	ln -s gtkroadgps $(DESTDIR)$(bindir)/roadgps

uninstall:
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME) roadmap roadgps
	$(MAKE) -C .. uninstall

# --- The real targets --------------------------------------------

libgtkroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libgtkroadmap.a $(RMLIBOBJS)
	$(RANLIB) libgtkroadmap.a

gtkroadmap: roadmap_main.o $(SSDLIB) ../libguiroadmap.a $(RDMLIBS)
	$(CXX) $(LDFLAGS) roadmap_main.o -o gtkroadmap $(SSDLIB) ../libguiroadmap.a $(LIBS) $(SSDLIB) ../libguiroadmap.a

gtkroadgps: roadmap_main.o ../libguiroadgps.a $(RDMLIBS)
	$(CXX) $(LDFLAGS) roadmap_main.o -o gtkroadgps $(SSDLIB) ../libguiroadgps.a $(LIBS) $(SSDLIB) ../libguiroadgps.a $(RDMLIBS)

../libssd_widgets.a:
	$(MAKE) -C .. libssd_widgets.a
