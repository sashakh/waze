
GUICFLAGS=-I/usr/include/qt
GUILDFLAGS=-lqt
MOC=/usr/bin/moc
INSTALLDIR=/usr/local

ifeq ($(DESKTOP),QPE)
   QPE: GUICFLAGS=-I$(QTDIR)/include -DQWS
   QPE: GUILDFLAGS=-L$(QTDIR)/lib -lqte -lqpe
   QPE: MOC=$(QTDIR)/bin/moc
   QPE: LDFLAGS=-L$(QTDIR)/lib
   INSTALLDIR=$(QTDIR)
endif

ifeq ($(MODE),DEBUG)
   STDCFLAGS=-g -Wall -DROADMAP_DEBUG_HEAP
else
ifeq ($(MODE),PROFILE)
   STDCFLAGS=-g -Wall -pg -fprofile-arcs -g
   LDFLAGS=-pg $(LDFLAGS)
else
   STDCFLAGS=-O2 -ffast-math -fomit-frame-pointer -g -Wall $(OPTIONS)
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
	install qtroadmap $(INSTALLDIR)/bin/roadmap
	install qtroadgps $(INSTALLDIR)/bin/roadgps
	install ipkg/zroadmap.desktop $(INSTALLDIR)/apps/Applications
	install ipkg/zroadgps.desktop $(INSTALLDIR)/apps/Applications
	install ipkg/zroadmap.png $(INSTALLDIR)/pics
	install ipkg/zroadgps.png $(INSTALLDIR)/pics
	install ipkg/preferences $(INSTALLDIR)/share/roadmap

uninstall:
	cd $(INSTALLDIR)/bin ; rm -f $(RUNTIME) roadmap roadgps
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

