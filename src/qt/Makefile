
GUICFLAGS=-I/usr/include/qt3
GUILDFLAGS=-lqt
MOC=/usr/bin/moc

DESTDIR=
INSTALLDIR=/usr/local
bindir=$(INSTALLDIR)/bin
desktopdir=$(INSTALLDIR)/apps/Applications
pkgdatadir=$(INSTALLDIR)/share/roadmap

ifeq ($(DESKTOP),QPE)
   GUICFLAGS=-I$(QTDIR)/include -DQWS
   GUILDFLAGS=-L$(QTDIR)/lib -lqte -lqpe
   MOC=$(QTDIR)/bin/moc
   LDFLAGS=-L$(QTDIR)/lib
   INSTALLDIR=$(QTDIR)
endif

ifeq ($(MODE),DEBUG)
   STDCFLAGS=-g -W -Wall -DROADMAP_DEBUG_HEAP
else
ifeq ($(MODE),PROFILE)
   STDCFLAGS=-g -W -Wall -pg -fprofile-arcs -g
   LDFLAGS=-pg $(LDFLAGS)
else
   STDCFLAGS=-O2 -ffast-math -fomit-frame-pointer -g -W -Wall $(OPTIONS)
endif
endif

CFLAGS=$(STDCFLAGS) $(GUICFLAGS) -I..
CPPFLAGS=-I.. $(GUICFLAGS) $(STDCFLAGS) -fno-rtti -fno-exceptions -pipe

RDMLIBS= ../libroadmap.a libqtroadmap.a ../unix/libosroadmap.a ../libroadmap.a
LIBS=$(RDMLIBS) $(GUILDFLAGS) -lm
STRIP=strip



RMLIBSRCS= \
	  qt_main.cc \
	  moc_qt_main.cc \
	  qt_canvas.cc \
	  moc_qt_canvas.cc \
          roadmap_canvas.cc \
	  qt_dialog.cc \
	  moc_qt_dialog.cc \
          roadmap_dialog.cc \
	  qt_fileselection.cc \
	  moc_qt_fileselection.cc \
	  roadmap_fileselection.cc \
          roadmap_messagebox.c \

RMLIBOBJS1=$(RMLIBSRCS:.c=.o)
RMLIBOBJS=$(RMLIBOBJS1:.cc=.o)

RUNTIME=qtroadmap qtroadgps



# --- Conventional targets ----------------------------------------

all: runtime

others:
	make -C .. CFLAGS="$(STDCFLAGS)" LDFLAGS=$(LDFLAGS) runtime
	make -C ../unix CFLAGS="$(STDCFLAGS) -I.." LDFLAGS=$(LDFLAGS) all

build:
	make -C .. CFLAGS="$(STDCFLAGS)" LDFLAGS=$(LDFLAGS) build

runtime: others $(RUNTIME)

clean: cleanone
	make -C .. cleanone
	make -C .. -C unix cleanone

cleanone:
	rm -f *.o *.a *.da $(RUNTIME) moc_qt_*.cc

install: all
	make -C .. install INSTALLDIR=$(INSTALLDIR)
	$(STRIP) $(RUNTIME)
	mkdir -p $(DESTDIR)$(bindir)
	install qtroadmap $(DESTDIR)$(bindir)/roadmap
	install qtroadgps $(DESTDIR)$(bindir)/roadgps

install-ipkg: install
	mkdir -p $(DESTDIR)$(desktopdir)
	install ipkg/zroadmap.desktop $(DESTDIR)$(desktopdir)
	install ipkg/zroadgps.desktop $(DESTDIR)$(desktopdir)
# These no longer exists.  Do not try to install them.  It will fail
#	mkdir -p $(DESTDIR)$(INSTALLDIR)/pics
	mkdir -p $(DESTDIR)$(pkgdatadir)
#	install ipkg/zroadmap.png $(DESTDIR)$(INSTALLDIR)/pics
#	install ipkg/zroadgps.png $(DESTDIR)$(INSTALLDIR)/pics
#	install ipkg/preferences $(DESTDIR)$(pkgdatadir)

uninstall:
	cd $(bindir) && rm -f $(RUNTIME) roadmap roadgps
	make -C .. uninstall

# --- The real targets --------------------------------------------

libqtroadmap.a: $(RMLIBOBJS)
	$(AR) rf libqtroadmap.a $(RMLIBOBJS)

qtroadmap: roadmap_main.o ../libguiroadmap.a $(RDMLIBS)
	$(CXX)  -o qtroadmap roadmap_main.o ../libguiroadmap.a $(LIBS)

qtroadgps: roadmap_main.o ../libguiroadgps.a $(RDMLIBS)
	$(CC) roadmap_main.o -o qtroadgps ../libguiroadgps.a $(LIBS)

moc_qt_main.cc: qt_main.h
	$(MOC) qt_main.h -o moc_qt_main.cc

moc_qt_canvas.cc: qt_canvas.h
	$(MOC) qt_canvas.h -o moc_qt_canvas.cc

moc_qt_dialog.cc: qt_dialog.h
	$(MOC) qt_dialog.h -o moc_qt_dialog.cc

moc_qt_fileselection.cc: qt_fileselection.h
	$(MOC) qt_fileselection.h -o moc_qt_fileselection.cc

