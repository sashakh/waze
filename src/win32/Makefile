# --- Installation options -------------------------------------------------

DESTDIR=
INSTALLDIR=/usr/local
bindir=$(INSTALLDIR)/bin


# --- Tool specific options ------------------------------------------------

RANLIB=ranlib
STRIP=strip

WARNFLAGS = -W -Wall -Wno-unused-parameter

RDMLIBS= ../libroadmap.a libwin32roadmap.a libosroadmap.a ../libroadmap.a libwin32roadmap.a


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

HOST=`uname -s`
ifeq ($(HOST),Darwin)
	ARFLAGS="r"
else
	ARFLAGS="rf"
endif


CFLAGS = $(STDCFLAGS) $(GPECFLAGS) $(GUICFLAGS) -I.. -I/usr/local/include -DNDEBUG -w -fno-underscoring 
CPPFLAGS = -DUNICODE -D_UNICODE
LIBS = $(RDMLIBS) $(GPELDFLAGS) $(GUILDFLAGS) -lm -L ../dglib -ldgl -L../agg -lagg_w32 -L../freetype -lfreetype_w32 -lwsock32 -lgdi32 -lcomdlg32

ifeq ($(BIDI),YES)
LIBS += -lfribidi
CFLAGS += -DUSE_FRIBIDI
endif

CXXFLAGS = $(CFLAGS) -I../agg/include -I/usr/include/freetype2 -I/usr/include/fribidi
# --- RoadMap sources & targets -----------------------------------

RMLIBSRCS=roadmap_messagebox.c \
          roadmap_dialog.c \
          roadmap_fileselection.c 

RMLIBOBJS=$(RMLIBSRCS:.c=.o) roadmap_canvas_agg.o ../agg/roadmap_canvas.o ../agg/font_freetype/agg_font_freetype.o

RUNTIME=gtkroadmap


# --- Conventional targets ----------------------------------------

.PHONY: all others build runtime clean cleanone install uninstall

all: runtime

others:
	$(MAKE) -f Makefile.win32 -C .. CFLAGS="$(STDCFLAGS)" runtime
	$(MAKE) -f Makefile.win32 -C ../agg CFLAGS="$(STDCFLAGS)" runtime
	$(MAKE) -f Makefile.win32 -C ../freetype CFLAGS="$(STDCFLAGS)" runtime

build:
	$(MAKE) -C .. CFLAGS="$(STDCFLAGS)" build

runtime: others $(RUNTIME)

clean: cleanone
	$(MAKE) -C .. cleanone
	$(MAKE) -C .. -C win32 cleanone

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

libwin32roadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libwin32roadmap.a $(RMLIBOBJS)
	$(RANLIB) libwin32roadmap.a

RES = /opt/xmingw/bin/i386-mingw32msvc-windres
roadmap_rc.o: roadmap.rc
	$(RES) --language=0 $(RESFLAGS) $(AM_RESFLAGS) -o $@ $<

gtkroadmap: roadmap_main.o roadmap_rc.o ../libguiroadmap.a $(RDMLIBS)
	$(CXX) $(LDFLAGS) roadmap_main.o -o gtkroadmap ../libguiroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o roadmap_rc.o ../libguiroadgps.a $(RDMLIBS)
	$(CXX) $(LDFLAGS) roadmap_main.o -o gtkroadgps ../libguiroadgps.a $(LIBS)

