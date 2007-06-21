
# QT makefile

TOP = ..
include $(TOP)/options.mk

# --- Installation options -------------------------------------------------

desktopdir=$(INSTALLDIR)/apps/Applications
MOC=$(QTDIR)/bin/moc

# --- QT-specific options ------------------------------------------------

ifeq ($(DESKTOP),QT4)
CFLAGS += -I$(QTDIR)/include -I$(QTDIR)/include/Qt -I$(QTDIR)/include/QtGui
CFLAGS += -fno-rtti -fno-exceptions -DQT4
LIBS += -L$(QTDIR)/lib -lQtCore -lQtGui -lpthread
#MOC=$(QTDIR)/bin/moc-qt4
endif

ifeq ($(DESKTOP),QPE4)
   CFLAGS += -I$(QTDIR)/include -I$(QTDIR)/include/Qt 
   CFLAGS += -I$(QTDIR)/include/QtGui -DQWS
   LIBS += -L$(QTDIR)/lib -lQtCore -lQtGui -lQtSql -lQtSvg -lQtNetwork -lQtXml -lts
   INSTALLDIR=$(QTDIR)
endif

# --- QT sources & targets --------------------------------------------

RMLIBSRCS = qt_main.cc \
           moc_qt_main.cc \
           qt_canvas.cc \
           moc_qt_canvas.cc \
           qt_dialog.cc \
           moc_qt_dialog.cc \
           roadmap_dialog.cc \
           roadmap_fileselection.cc \
           roadmap_messagebox.c

RMLIBOBJS1 = $(RMLIBSRCS:.cc=.o)
RMLIBOBJS = $(CANVAS_OBJS) $(RMLIBOBJS1:.c=.o)

TARGETS = qtroadgps qtroadmap


# --- Conventional targets ----------------------------------------

.PHONY: all clean strip install install-ipkg uninstall

all: $(TARGETS)

clean:
	rm -f *~ *.o *.a *.da $(TARGETS) moc_qt_*.cc

strip:
	$(STRIP) $(TARGETS)

install:
	mkdir -p $(DESTDIR)$(bindir)
	cd $(DESTDIR)$(bindir) && rm -f $(TARGETS) roadmap roadgps
	install $(TARGETS) $(DESTDIR)$(bindir)/
	ln -s qtroadmap $(DESTDIR)$(bindir)/roadmap
	ln -s qtroadgps $(DESTDIR)$(bindir)/roadgps
	cd $(DESTDIR)$(bindir) ; $(STRIP) $(TARGETS)

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
	cd $(bindir) && rm -f $(TARGETS) roadmap roadgps

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
	$(CXX) $(LDFLAGS) -o qtroadmap roadmap_main.o \
		$(TOP)/libguiroadmap.a libqtroadmap.a $(LIBS)

qtroadgps: roadmap_main.o libqtroadmap.a \
		$(TOP)/libguiroadgps.a  $(filter %.a, $(LIBS))
	$(CXX) $(LDFLAGS) -o qtroadgps roadmap_main.o \
		$(TOP)/libguiroadgps.a libqtroadmap.a $(LIBS)

moc_qt_main.cc: qt_main.h
	$(MOC) qt_main.h -o moc_qt_main.cc

moc_qt_canvas.cc: qt_canvas.h
	$(MOC) qt_canvas.h -o moc_qt_canvas.cc

moc_qt_dialog.cc: qt_dialog.h
	$(MOC) qt_dialog.h -o moc_qt_dialog.cc

