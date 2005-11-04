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


# --- Tool specific options ------------------------------------------------

RANLIB = ranlib

WARNFLAGS = -W -Wall -Wno-unused-parameter


ifeq ($(MODE),DEBUG)
# Memory leak detection using mtrace:
# Do not forget to set the trace file using the env. variable MALLOC_TRACE,
# then use the mtrace tool to analyze the output.
   MODECFLAGS=-g $(WARNFLAGS) -DROADMAP_DEBUG_HEAP
   MODELDFLAGS=
else
ifeq ($(MODE),PROFILE)
   MODECFLAGS=-g $(WARNFLAGS) -pg -fprofile-arcs -g
   MODELDFLAGS=-pg
else
   MODECFLAGS=-O2 -ffast-math -fomit-frame-pointer $(WARNFLAGS) $(OPTIONS)
   MODELDFLAGS=
endif
endif

ifeq ($(SQL),SQLITE)
   SQLCFLAGS=-DROADMAP_USES_SQLITE
	SQLLDFLAGS=-lsqlite
	SQLTOOLS=sqliteload
else
   SQLCFLAGS=
	SQLLDFLAGS=
	SQLTOOLS=
endif

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

HOST=`uname -s`
ifeq ($(HOST),Darwin)
	ARFLAGS="r"
else
	ARFLAGS="rf"
endif

ifeq ($(SHAPEFILES),NO)
   ROADMAP_USE_SHAPEFILES=
   SHAPELDFLAGS=
else
   ROADMAP_USE_SHAPEFILES=-DROADMAP_USE_SHAPEFILES
	SHAPELDFLAGS=-lshp
endif


CFLAGS=$(MODECFLAGS) $(SQLCFLAGS) $(ROADMAP_USE_SHAPEFILES) -I/usr/local/include
LDFLAGS=$(MODELDFLAGS) $(SQLLDFLAGS) $(SHAPELDFLAGS)

RDMLIBS=libroadmap.a unix/libosroadmap.a
LIBS=$(RDMLIBS) -lpopt -lm


# --- RoadMap sources & targets --------------------------------------------

RMLIBSRCS=roadmap_log.c \
          roadmap_message.c \
          roadmap_string.c \
          roadmap_voice.c \
          roadmap_list.c \
          roadmap_config.c \
          roadmap_option.c \
          roadmap_county.c \
          roadmap_locator.c \
          roadmap_math.c \
          roadmap_hash.c \
          roadmap_dbread.c \
          roadmap_dictionary.c \
          roadmap_square.c \
          roadmap_point.c \
          roadmap_line.c \
          roadmap_shape.c \
          roadmap_place.c \
          roadmap_polygon.c \
          roadmap_street.c \
          roadmap_geocode.c \
          roadmap_history.c \
          roadmap_input.c \
          roadmap_nmea.c \
          roadmap_gpsd2.c \
          roadmap_io.c \
          roadmap_gps.c \
          roadmap_object.c \
          roadmap_driver.c \
          roadmap_adjust.c \
          roadmap_sunrise.c

RMLIBOBJS=$(RMLIBSRCS:.c=.o)


RMGUISRCS=roadmap_sprite.c \
          roadmap_object.c \
          roadmap_trip.c \
          roadmap_layer.c \
          roadmap_fuzzy.c \
          roadmap_navigate.c \
			 roadmap_pointer.c \
          roadmap_screen.c \
          roadmap_display.c \
          roadmap_factory.c \
          roadmap_preferences.c \
          roadmap_address.c \
          roadmap_crossing.c \
          roadmap_coord.c \
          roadmap_download.c \
          roadmap_help.c \
          roadmap_start.c

RMGUIOBJS=$(RMGUISRCS:.c=.o)


RGGUISRCS=roadgps_start.c \
          roadmap_factory.c \
          roadmap_help.c \
          roadgps_screen.c \
          roadgps_logger.c

RGGUIOBJS=$(RGGUISRCS:.c=.o)


RMPLUGINSRCS=roadmap_copy.c roadmap_httpcopy.c

RMPLUGINOBJS=$(RMPLUGINSRCS:.c=.o)


DMSRCS=dumpmap_main.c

DMOBJS=$(DMSRCS:.c=.o)


BMSRCS=buildmap_messages.c \
       buildmap_dictionary.c \
       buildmap_dbwrite.c \
       buildmap_city.c \
       buildmap_square.c \
       buildmap_point.c \
       buildmap_street.c \
       buildmap_area.c \
       buildmap_line.c \
       buildmap_shape.c \
       buildmap_polygon.c \
       buildmap_range.c \
       buildmap_zip.c \
       buildmap_tiger.c \
       buildmap_shapefile.c \
       buildmap_place.c \
       buildmap_main.c

BMOBJS=$(BMSRCS:.c=.o)


BPSRCS=buildmap_messages.c \
       buildmap_dictionary.c \
       buildmap_dbwrite.c \
       buildmap_square.c \
       buildmap_point.c \
       buildmap_place.c \
       buildplace_main.c

BPOBJS=$(BPSRCS:.c=.o)


BUSRCS=buildus_main.c \
       buildus_fips.c \
       buildus_county.c \
       buildmap_dictionary.c \
       buildmap_dbwrite.c \
       buildmap_messages.c

BUOBJS=$(BUSRCS:.c=.o)


PKGDATAFILES=sprites preferences schema session drivers
BUILD=buildmap buildus dumpmap buildplace
SCRIPTS=rdmdownload rdmgetall rdmgenmaps rdmgendcwmaps rdmcompare
RUNTIME=$(RDMLIBS) libguiroadgps.a libguiroadmap.a
DRIVERS=rdmkismet rdmghost rdmfriends rdmtrace
TOOLS=sunrise $(SQLTOOLS)


# --- Conventional targets ----------------------------------------

.PHONY: all gtk gtk2 qt rebuild build everything clean strip install uninstall

all: everything

gtk:
	$(MAKE) DESKTOP=GTK

gtk2:
	$(MAKE) DESKTOP=GTK2

qt:
	$(MAKE) DESKTOP=QT

rebuild: clean everything

everything: runtime build $(TOOLS) icons

runtime: $(RUNTIME) $(DRIVERS)
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module STDCFLAGS="$(CFLAGS)" runtime || exit 1; \
		fi ; \
	done

build: $(BUILD)

strip:
	strip -s $(RUNTIME)
	strip -s $(BUILD)
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module strip || exit 1; \
		fi ; \
	done

clean:
	rm -f *.o *.a *.da $(BUILD) $(TOOLS) $(DRIVERS)
	# Clean up CVS backup files as well.
	$(RM) .#*
	for module in icons $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module clean || exit 1; \
		fi ; \
	done
	if [ -d unix ] ; then $(MAKE) -C unix clean ; fi


install: everything
	mkdir -p $(DESTDIR)$(pkgmapsdir)
	chmod a+rx $(DESTDIR)$(pkgmapsdir)
	mkdir -p $(DESTDIR)$(pkgdatadir)
	chmod a+rx $(DESTDIR)$(pkgdatadir)
	cd $(DESTDIR)$(pkgdatadir) && rm -f $(PKGDATAFILES)
	cp $(PKGDATAFILES) $(DESTDIR)$(pkgdatadir)/.
	cd $(DESTDIR)$(pkgdatadir) ; chmod a+r $(PKGDATAFILES)
	mkdir -p $(DESTDIR)$(pkgbindir)
	cd $(DESTDIR)$(pkgbindir) && rm -f $(BUILD) $(SCRIPTS)
	cp $(BUILD) $(SCRIPTS) $(DRIVERS) $(DESTDIR)$(pkgbindir)/.
	cd $(DESTDIR)$(pkgbindir) && chmod a+x $(BUILD) $(SCRIPTS) $(DRIVERS)
	if [ -d $(menudir) ] ; then cp roadmap.menu $(menudir)/roadmap; fi
	mkdir -p $(DESTDIR)$(ICONDIR)
	cd icons && $(MAKE) ICONDIR="$(DESTDIR)$(ICONDIR)" install
	cp -f roadmap*.png $(DESTDIR)$(ICONDIR)/.
	mkdir -p $(DESTDIR)$(desktopdir)
	cp -f roadmap.desktop $(DESTDIR)$(desktopdir)/.
	mkdir -p $(DESTDIR)$(man1dir)
	$(INSTALL_DATA) roadgps.1 roadmap.1 $(DESTDIR)$(man1dir)/.
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module install || exit 1; \
		fi ; \
	done

installdoc: webdoc
	copy ../web/manual.html $(DESTDIR)$(pkgdatadir)/manual.html

webdoc:
	aptconvert -toc -enc ASCII ../web/manual.html ../README

uninstall:
	cd $(DESTDIR)$(pkgdatadir) && rm -f $(PKGDATAFILES)
	cd $(DESTDIR)$(pkgbindir) && rm -f $(BUILD) $(SCRIPTS) $(DRIVERS)
	if [ -d $(menudir) ] ; then rm -f $(menudir)/roadmap; fi
	cd icons && $(MAKE) ICONDIR="$(DESTDIR)$(ICONDIR)" uninstall
	cd $(DESTDIR)$(ICONDIR) && rm -f roadmap*.png
	cd $(DESTDIR)$(desktopdir) && rm -f roadmap.desktop
	cd $(DESTDIR)$(man1dir) && rm -f roadgps.1 roadmap.1
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module uninstall || exit 1; \
		fi ; \
	done

icons:
	$(MAKE) -C icons STDCFLAGS="$(CFLAGS)" all

sunrise: roadmap_sunrise.c
	$(CC) -g -DSUNRISE_PROGRAM roadmap_sunrise.c -o sunrise -lm

sqliteload: sqliteload.c
	$(CC) -g sqliteload.c $(SQLCFLAGS) $(SQLLDFLAGS) -o sqliteload -lm

rdmkismet: roadmap_kismet.c $(RDMLIBS)
	$(CC) -g roadmap_kismet.c -o rdmkismet $(RDMLIBS) $(RDMLIBS) -lm

rdmghost: roadmap_ghost.c $(RDMLIBS)
	$(CC) -g roadmap_ghost.c -o rdmghost $(RDMLIBS) -lm

rdmfriends: roadmap_friends.c $(RDMLIBS)
	$(CC) -g roadmap_friends.c -o rdmfriends $(RDMLIBS) $(RDMLIBS) -lm

rdmtrace: roadmap_trace.c $(RDMLIBS)
	$(CC) -g roadmap_trace.c -o rdmtrace $(RDMLIBS) -lm

# --- The real targets --------------------------------------------

libroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libroadmap.a $(RMLIBOBJS)
	$(RANLIB) libroadmap.a

libguiroadmap.a: $(RMGUIOBJS) $(RMPLUGINOBJS)
	$(AR) $(ARFLAGS) libguiroadmap.a $(RMGUIOBJS) $(RMPLUGINOBJS)
	$(RANLIB) libguiroadmap.a

libguiroadgps.a: $(RGGUIOBJS)
	$(AR) $(ARFLAGS) libguiroadgps.a $(RGGUIOBJS)
	$(RANLIB) libguiroadgps.a

unix/libosroadmap.a:
	if [ -d unix ] ; then $(MAKE) -C unix "CFLAGS=$(CFLAGS) -I.." ; fi

dumpmap: $(DMOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(DMOBJS) -o dumpmap $(LIBS)

buildmap: $(BMOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BMOBJS) -o buildmap $(LIBS)

buildus: $(BUOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BUOBJS) -o buildus $(LIBS)

buildplace: $(BPOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BPOBJS) -o buildplace $(LIBS)

