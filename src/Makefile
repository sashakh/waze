
ifeq ($(MODE),DEBUG)
# Memory leak detection using mtrace:
# Do not forget to set the trace file using the env. variable MALLOC_TRACE,
# then use the mtrace tool to analyze the output.
   MODECFLAGS=-g -Wall -DROADMAP_DEBUG_HEAP
   MODELDFLAGS=
else
ifeq ($(MODE),PROFILE)
   MODECFLAGS=-g -Wall -pg -fprofile-arcs -g
   MODELDFLAGS=-pg
else
   MODECFLAGS=-O2 -ffast-math -fomit-frame-pointer -Wall $(OPTIONS)
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

CFLAGS=$(MODECFLAGS) $(SQLCFLAGS)
LDFLAGS=$(MODELDFLAGS) $(SQLLDFLAGS)
TOOLS=sunrise $(SQLTOOLS)

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
          roadmap_polygon.c \
          roadmap_street.c \
          roadmap_geocode.c \
          roadmap_history.c \
          roadmap_nmea.c \
          roadmap_gps.c \
          roadmap_adjust.c \
          roadmap_sunrise.c

RMLIBOBJS=$(RMLIBSRCS:.c=.o)


RMGUISRCS=roadmap_sprite.c \
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
       buildmap_main.c

BMOBJS=$(BMSRCS:.c=.o)


BUSRCS=buildus_main.c \
       buildus_fips.c \
       buildus_county.c \
       buildmap_dictionary.c \
       buildmap_dbwrite.c \
       buildmap_messages.c

BUOBJS=$(BUSRCS:.c=.o)


BUILD=buildmap buildus dumpmap
SCRIPTS=rdmdownload rdmgetall rdmgenmaps rdmcompare
RUNTIME=$(RDMLIBS) libguiroadgps.a libguiroadmap.a

INSTALLDIR=/usr/local


# --- Conventional targets ----------------------------------------

all: everything

runtime: $(RUNTIME)

build: $(BUILD)

strip:
	strip -s $(RUNTIME)
	strip -s $(BUILD)

clean: cleanone
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			make -C $$module cleanone ; \
		fi ; \
	done
	if [ -d unix ] ; then make -C unix cleanone ; fi

cleanone:
	rm -f *.o *.a *.da $(BUILD)

install: build runtime icons
	mkdir -p $(INSTALLDIR)/share/roadmap
	cd $(INSTALLDIR)/share/roadmap ; rm -f sprites preferences
	cp sprites preferences schema $(INSTALLDIR)/share/roadmap
	cd $(INSTALLDIR)/share/roadmap ; chmod a+r sprites preferences
	aptconvert -toc $(INSTALLDIR)/share/roadmap/manual.html ../README
	cd $(INSTALLDIR)/bin ; rm -f $(BUILD) $(SCRIPTS)
	cp $(BUILD) $(SCRIPTS) $(INSTALLDIR)/bin
	cd $(INSTALLDIR)/bin ; chmod a+x $(BUILD) $(SCRIPTS)
	if [ -d /usr/lib/menu ] ; then cp roadmap.menu /usr/lib/menu/roadmap; fi
	mkdir -p $(INSTALLDIR)/share/pixmaps
	cd icons ; make ICONDIR="$(INSTALLDIR)/share/pixmaps" install
	cp -f roadmap*.png $(INSTALLDIR)/share/pixmaps
	cp -f roadmap.desktop $(INSTALLDIR)/applications

uninstall:
	cd $(INSTALLDIR)/bin ; rm -f $(BUILD) $(SCRIPTS)
	cd $(INSTALLDIR) ; rm -rf share/roadmap
	rm -f /usr/lib/menu/roadmap

everything: runtime build icons $(TOOLS)
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			make -C $$module STDCFLAGS="$(CFLAGS)" all ; \
		fi ; \
	done

cleanall:
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			make -C $$module clean ; \
		fi ; \
	done

rebuild: cleanall everything

sunrise: roadmap_sunrise.c
	$(CC) -g -DSUNRISE_PROGRAM roadmap_sunrise.c -o sunrise -lm

sqliteload: sqliteload.c
	$(CC) -g sqliteload.c $(SQLCFLAGS) $(SQLLDFLAGS) -o sqliteload -lm

# --- The real targets --------------------------------------------

libroadmap.a: $(RMLIBOBJS)
	$(AR) rf libroadmap.a $(RMLIBOBJS)

libguiroadmap.a: $(RMGUIOBJS) $(RMPLUGINOBJS)
	$(AR) rf libguiroadmap.a $(RMGUIOBJS) $(RMPLUGINOBJS)

libguiroadgps.a: $(RGGUIOBJS)
	$(AR) rf libguiroadgps.a $(RGGUIOBJS)

unix/libosroadmap.a:
	if [ -d unix ] ; then make -C unix "CFLAGS=$(CFLAGS) -I.." ; fi

dumpmap: $(DMOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(DMOBJS) -o dumpmap $(LIBS)

buildmap: $(BMOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BMOBJS) -o buildmap $(LIBS)

buildus: $(BUOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BUOBJS) -o buildus $(LIBS)

