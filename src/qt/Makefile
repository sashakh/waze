
# QT makefile

TOP = ..
include $(TOP)/options.mk

# --- Installation options -------------------------------------------------

desktopdir=$(INSTALLDIR)/apps/Applications
MOC=$(QTDIR)/bin/moc

# --- QT-specific options ------------------------------------------------

ifeq ($(DESKTOP),QT)
CFLAGS += -I$(QTDIR)/include
CFLAGS += -fno-rtti -fno-exceptions
LIBS += -L$(QTDIR)/lib -lqt-mt
endif

ifeq ($(DESKTOP),QPE)
   CFLAGS += -I$(QTDIR)/include -DQWS
   LIBS += -lqte -lqpe
   INSTALLDIR=$(QTDIR)
endif



# --- QT sources & targets --------------------------------------------

MOC_RMLIBSRCS = \
	qt_main.cc \
	qt_canvas.cc \
	qt_dialog.cc

NO_MOC_RMLIBSRCS = \
	qt_fileselection.cc \
	roadmap_fileselection.cc \
	roadmap_dialog.cc \
	roadmap_messagebox.cc \
	roadmap_progress.cc

SOURCE = $(MOC_RMLIBSRCS) $(NO_MOC_RMLIBSRCS)

RMLIBSRCS = \
	$(addprefix moc_, $(MOC_RMLIBSRCS)) \
	$(MOC_RMLIBSRCS) \
	$(NO_MOC_RMLIBSRCS)

RMLIBOBJS1 = $(RMLIBSRCS:.cc=.o)
RMLIBOBJS = $(CANVAS_OBJS) $(RMLIBOBJS1:.c=.o)

HEADERS = \
	qt_canvas.h \
	qt_dialog.h \
	qt_fileselection.h \
	qt_main.h

TARGETS = qtroadgps qtroadmap


# --- Conventional targets ----------------------------------------

.PHONY: all clean strip install install-ipkg uninstall

all: $(TARGETS)

clean:
	rm -f *.o *.a *.da $(TARGETS) moc_qt_*.cc

strip:
	$(STRIP) $(TARGETS)

install:
	mkdir -p $(bindir)
	cd $(bindir) && rm -f $(TARGETS) roadmap roadgps
	install $(TARGETS) $(bindir)/
	ln -s qtroadmap $(bindir)/roadmap
	ln -s qtroadgps $(bindir)/roadgps
	cd $(bindir) ; $(STRIP) $(TARGETS)

install-ipkg: install
	mkdir -p $(desktopdir)
	install ipkg/zroadmap.desktop $(desktopdir)
	install ipkg/zroadgps.desktop $(desktopdir)
# These no longer exists.  Do not try to install them.  It will fail
#	mkdir -p $(INSTALLDIR)/pics
	mkdir -p $(pkgdatadir)
#	install ipkg/zroadmap.png $(INSTALLDIR)/pics
#	install ipkg/zroadgps.png $(INSTALLDIR)/pics
#	install ipkg/preferences $(pkgdatadir)

uninstall:
	cd $(bindir) && rm -f $(TARGETS) roadmap roadgps

sourcelist:
	@echo Makefile roadmap_main.cc roadmap_canvas.cc $(SOURCE) $(HEADERS)

# temporary
roadmap_canvas_agg.o:
	@echo
	@echo "*** Sorry!  We still need a roadmap_canvas_agg.cc for QT."
	@echo
	false


# --- The real targets --------------------------------------------

libqtroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libqtroadmap.a $(RMLIBOBJS)
	$(RANLIB) libqtroadmap.a

qtroadmap: roadmap_main.o libqtroadmap.a \
		$(TOP)/libguiroadmap.a $(filter %.a, $(LIBS))
	$(CC) $(LDFLAGS) -o qtroadmap roadmap_main.o \
		$(TOP)/libguiroadmap.a libqtroadmap.a $(LIBS)

qtroadgps: roadmap_main.o libqtroadmap.a \
		$(TOP)/libguiroadgps.a  $(filter %.a, $(LIBS))
	$(CC) $(LDFLAGS) -o qtroadgps roadmap_main.o \
		$(TOP)/libguiroadgps.a libqtroadmap.a $(LIBS)

moc_qt_main.cc: qt_main.h
	$(MOC) qt_main.h -o moc_qt_main.cc

moc_qt_canvas.cc: qt_canvas.h
	$(MOC) qt_canvas.h -o moc_qt_canvas.cc

moc_qt_dialog.cc: qt_dialog.h
	$(MOC) qt_dialog.h -o moc_qt_dialog.cc

moc_qt_fileselection.cc: qt_fileselection.h
	$(MOC) qt_fileselection.h -o moc_qt_fileselection.cc

