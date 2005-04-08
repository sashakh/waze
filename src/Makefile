
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

ifeq ($(SHAPEFILES),NO)
   ROADMAP_USE_SHAPEFILES=
   SHAPELDFLAGS=
else
   ROADMAP_USE_SHAPEFILES=-DROADMAP_USE_SHAPEFILES
	SHAPELDFLAGS=-lshp
endif

CFLAGS=$(MODECFLAGS) $(SQLCFLAGS) $(ROADMAP_USE_SHAPEFILES)
LDFLAGS=$(MODELDFLAGS) $(SQLLDFLAGS) $(SHAPELDFLAGS)
TOOLS=sunrise $(SQLTOOLS) rdmkismet rdmghost

RDMLIBS=libroadmap.a unix/libosroadmap.a
LIBS=$(RDMLIBS) -lpopt -lm



RMLIBSRCS=roadmap_log.c \
          roadmap_message.c \
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
          roadmap_nmea.c \
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


PKGDATAFILES=sprites preferences schema session
BUILD=buildmap buildus dumpmap buildplace
SCRIPTS=rdmdownload rdmgetall rdmgenmaps rdmgendcwmaps rdmcompare
RUNTIME=$(RDMLIBS) libguiroadgps.a libguiroadmap.a

DESTDIR=
INSTALLDIR=/usr/local
desktopdir=$(INSTALLDIR)/applications
pkgdatadir=$(INSTALLDIR)/share/roadmap
bindir=$(INSTALLDIR)/bin
pkgbindir=$(bindir)
menudir=$(DESTDIR)/usr/lib/menu
ICONDIR=$(INSTALLDIR)/share/pixmaps
mandir=$(INSTALLDIR)/share/man
man1dir=$(mandir)/man1

INSTALL      = install
INSTALL_DATA = install -m644


# --- Conventional targets ----------------------------------------

all: everything

runtime: $(RUNTIME)

build: $(BUILD)

strip:
	strip -s $(RUNTIME)
	strip -s $(BUILD)

clean: cleanone
	for module in icons $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module cleanone || exit 1; \
		fi ; \
	done
	if [ -d unix ] ; then $(MAKE) -C unix cleanone ; fi

cleanone:
	rm -f *.o *.a *.da $(BUILD) $(TOOLS)
	# Clean up CVS backup files as well.
	$(RM) .#*

install: everything
	mkdir -p $(DESTDIR)$(pkgdatadir)
	cd $(DESTDIR)$(pkgdatadir) && rm -f $(PKGDATAFILES)
	cp $(PKGDATAFILES) $(DESTDIR)$(pkgdatadir)/.
	cd $(DESTDIR)$(pkgdatadir) ; chmod a+r $(PKGDATAFILES)
	mkdir -p $(DESTDIR)$(pkgbindir)
	cd $(DESTDIR)$(pkgbindir) && rm -f $(BUILD) $(SCRIPTS)
	cp $(BUILD) $(SCRIPTS) $(DESTDIR)$(pkgbindir)/.
	cd $(DESTDIR)$(pkgbindir) && chmod a+x $(BUILD) $(SCRIPTS)
	if [ -d $(menudir) ] ; then cp roadmap.menu $(menudir)/roadmap; fi
	mkdir -p $(DESTDIR)$(ICONDIR)
	cd icons && $(MAKE) ICONDIR="$(DESTDIR)$(ICONDIR)" install
	cp -f roadmap*.png $(DESTDIR)$(ICONDIR)/.
	mkdir -p $(DESTDIR)$(desktopdir)
	cp -f roadmap.desktop $(DESTDIR)$(desktopdir)/.
	mkdir -p $(DESTDIR)$(man1dir)
	$(INSTALL_DATA) roadgps.1 roadmap.1 $(DESTDIR)$(man1dir)/.

installdoc:
	aptconvert -toc -enc ASCII $(DESTDIR)$(pkgdatadir)/manual.html ../README

webdoc:
	aptconvert -toc -enc ASCII ../web/manual.html ../README

uninstall:
	cd $(DESTDIR)$(pkgbindir) && rm -f $(BUILD) $(SCRIPTS)
	cd $(DESTDIR)$(INSTALLDIR) && rm -rf share/roadmap
	rm -f $(DESTDIR)$(menudir)/roadmap

everything: modules runtime build $(TOOLS)

modules:
	for module in icons $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module STDCFLAGS="$(CFLAGS)" all || exit 1; \
		fi ; \
	done

cleanall:
	for module in icons $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module clean ; \
		fi ; \
	done

rebuild: cleanall everything

sunrise: roadmap_sunrise.c
	$(CC) -g -DSUNRISE_PROGRAM roadmap_sunrise.c -o sunrise -lm

sqliteload: sqliteload.c
	$(CC) -g sqliteload.c $(SQLCFLAGS) $(SQLLDFLAGS) -o sqliteload -lm

rdmkismet: roadmap_kismet.c $(RDMLIBS)
	$(CC) -g roadmap_kismet.c -o rdmkismet $(RDMLIBS) $(RDMLIBS) -lm

rdmghost: roadmap_ghost.c $(RDMLIBS)
	$(CC) -g roadmap_ghost.c -o rdmghost $(RDMLIBS) -lm

# --- The real targets --------------------------------------------

libroadmap.a: $(RMLIBOBJS)
	$(AR) rf libroadmap.a $(RMLIBOBJS)

libguiroadmap.a: $(RMGUIOBJS) $(RMPLUGINOBJS)
	$(AR) rf libguiroadmap.a $(RMGUIOBJS) $(RMPLUGINOBJS)

libguiroadgps.a: $(RGGUIOBJS)
	$(AR) rf libguiroadgps.a $(RGGUIOBJS)

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

