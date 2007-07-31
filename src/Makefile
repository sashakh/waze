
# --- Tool specific options ------------------------------------------------


TOP = .
include $(TOP)/options.mk

# --- RoadMap sources & targets --------------------------------------------

RMLIBSRCS=roadmap_log.c \
	roadmap_message.c \
	roadmap_string.c \
	roadmap_voice.c \
	roadmap_scan.c \
	roadmap_list.c \
	roadmap_config.c \
	roadmap_option.c \
	roadmap_metadata.c \
	roadmap_index.c \
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
	roadmap_plugin.c \
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
	roadmap_sunrise.c \
	roadmap_state.c \
	roadmap_gpx.c \
	roadmap_linefont.c

RMLIBOBJS=$(RMLIBSRCS:.c=.o)


RMGUISRCS=roadmap_sprite.c \
	roadmap_object.c \
	roadmap_trip.c \
	roadmap_track.c \
	roadmap_landmark.c \
	roadmap_features.c \
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
	roadmap_label.c \
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


BMLIBSRCS=buildmap_messages.c \
	buildmap_dictionary.c \
	buildmap_dbwrite.c \
	buildmap_metadata.c \
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
	buildmap_layer.c \
	buildmap_tiger.c \
	buildmap_shapefile.c \
	buildmap_empty.c \
	buildmap_place.c \
	buildmap_index.c \

BMLIBOBJS=$(BMLIBSRCS:.c=.o)

BMSRCS=buildmap_main.c

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


XCHGSRCS=rdmxchange_main.c \
	rdmxchange_dictionary.c \
	rdmxchange_index.c \
	rdmxchange_metadata.c \
	rdmxchange_point.c \
	rdmxchange_line.c \
	rdmxchange_shape.c \
	rdmxchange_polygon.c \
	rdmxchange_square.c \
	rdmxchange_street.c \
	rdmxchange_range.c \
	rdmxchange_zip.c \
	buildmap_messages.c \
	buildmap_dbwrite.c

XCHGOBJS=$(XCHGSRCS:.c=.o)


PKGDATAFILES=sprites preferences session drivers roadmap.popup
SCRIPTS=rdmdownload rdmgetall rdmgenmaps rdmgendcwmaps rdmcompare
RUNTIME=$(RDMLIBS) libguiroadgps.a libguiroadmap.a
DRIVERS=rdmkismet rdmghost rdmfriends rdmtrace rdmxchange
TOOLS=sunrise $(SQLTOOLS)
ifneq ($(strip $(POPT)),NO)
  BUILD = buildmap buildus dumpmap buildplace rdmindex 
endif


# --- Conventional targets ----------------------------------------

.PHONY: all gtk gtk2 qt3 qt4 qpe4 rebuild build \
         topclean clean strip install uninstall unix gpx

all: runtime build $(TOOLS) icons


# --- Convenience targets, to force a specific desktop build ------
#     (overrides setting in config.mk)
gtk:
	$(MAKE) DESKTOP=GTK

gtk2:
	$(MAKE) DESKTOP=GTK2

qt3:
	$(MAKE) DESKTOP=QT

qt4:
	$(MAKE) DESKTOP=QT4

qpe4:
	$(MAKE) DESKTOP=QPE4

rebuild: clean all


# ---  Sub-targets ------------------------------------------------

unix gpx:
	$(MAKE) -C $@ all


runtime: unix gpx $(RUNTIME) $(DRIVERS)
	for module in $(RDMODULES) ; \
	do \
		$(MAKE) -C $$module all || exit 1; \
	done

build: unix gpx $(BUILD)

strip:
	-$(STRIP) $(BUILD) $(DRIVERS)
	for module in $(RDMODULES) ; \
	do \
		$(MAKE) -C $$module strip || exit 1; \
	done

icons:
	$(MAKE) -C icons all

topclean:
	rm -f *.o *.a *.da $(BUILD) $(TOOLS) $(DRIVERS)
	# Clean up CVS backup files as well.
	$(RM) .#*

clean:  topclean
	for module in icons unix gpx $(RDMODULES) ; \
	do \
		$(MAKE) -C $$module clean || exit 1; \
	done
	rm -f agg_support/*.o


install: installdata installbin installicons installdesktop
	for module in $(RDMODULES) ; \
	do \
		$(MAKE) -C $$module install || exit 1; \
	done

installdata:
	mkdir -p $(DESTDIR)$(pkgmapsdir)
	chmod a+rx $(DESTDIR)$(pkgmapsdir)
	mkdir -p $(DESTDIR)$(pkgdatadir)
	chmod a+rx $(DESTDIR)$(pkgdatadir)
	mkdir -p $(DESTDIR)$(pkgdatadir)/default
	chmod a+rx $(DESTDIR)$(pkgdatadir)/default
	cd $(DESTDIR)$(pkgdatadir) && rm -f $(PKGDATAFILES)
	cp $(PKGDATAFILES) $(DESTDIR)$(pkgdatadir)/.
	cd $(DESTDIR)$(pkgdatadir) ; chmod a+r $(PKGDATAFILES)
	cp default/All $(DESTDIR)$(pkgdatadir)/default/.
	chmod a+r $(DESTDIR)$(pkgdatadir)/default/All

installbin:
	mkdir -p $(DESTDIR)$(pkgbindir)
	cd $(DESTDIR)$(pkgbindir) && rm -f $(BUILD) $(SCRIPTS)
	cp $(BUILD) $(SCRIPTS) rdmindex $(DRIVERS) $(DESTDIR)$(pkgbindir)/.
	cd $(DESTDIR)$(pkgbindir) && chmod a+x $(BUILD) $(SCRIPTS) rdmindex $(DRIVERS)
	mkdir -p $(DESTDIR)$(man1dir)
	$(INSTALL_DATA) roadgps.1 roadmap.1 $(DESTDIR)$(man1dir)/.

installdesktop:
	if [ -d $(menudir) ] ; then cp roadmap.menu $(menudir)/roadmap; fi
	mkdir -p $(DESTDIR)$(desktopdir)
	cp -f roadmap.desktop $(DESTDIR)$(desktopdir)/.

installicons:
	$(MAKE) -C icons install

installdoc: ../web/manual.html 
	cp ../web/manual.html $(DESTDIR)$(pkgdatadir)/manual.html

webdoc: ../web/manual.html ../web/makemaps.html

../web/manual.html: ../README
	aptconvert -toc -enc ASCII $@ $<

../web/makemaps.html: ../howtos/makemaps.txt
	aptconvert -toc -enc ASCII $@ $<

uninstall:
	cd $(DESTDIR)$(pkgdatadir) && rm -f $(PKGDATAFILES)
	cd $(DESTDIR)$(pkgbindir) && rm -f $(BUILD) $(SCRIPTS) $(DRIVERS)
	if [ -d $(menudir) ] ; then rm -f $(menudir)/roadmap; fi
	$(MAKE) -C icons uninstall
	cd $(DESTDIR)$(desktopdir) && rm -f roadmap.desktop
	cd $(DESTDIR)$(man1dir) && rm -f roadgps.1 roadmap.1
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module uninstall || exit 1; \
		fi ; \
	done

# --- The real targets --------------------------------------------

libbuildmap.a: $(BMLIBOBJS)
	$(AR) $(ARFLAGS) libbuildmap.a $(BMLIBOBJS)
	$(RANLIB) libbuildmap.a

libroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libroadmap.a $(RMLIBOBJS)
	$(RANLIB) libroadmap.a

libguiroadmap.a: $(RMGUIOBJS) $(RMPLUGINOBJS)
	$(AR) $(ARFLAGS) libguiroadmap.a $(RMGUIOBJS) $(RMPLUGINOBJS)
	$(RANLIB) libguiroadmap.a

libguiroadgps.a: $(RGGUIOBJS)
	$(AR) $(ARFLAGS) libguiroadgps.a $(RGGUIOBJS)
	$(RANLIB) libguiroadgps.a

sunrise: roadmap_sunrise.c
	$(CC) $(LDFLAGS) $(CFLAGS) -DSUNRISE_PROGRAM roadmap_sunrise.c -o sunrise -lm

sqliteload: sqliteload.o
	$(CC) sqliteload.o $(SQLCFLAGS) $(SQLLDFLAGS) -o sqliteload -lm

rdmindex : rdmindex_main.o libbuildmap.a $(RDMLIBS)
	$(CC) $(LDFLAGS) rdmindex_main.o -o rdmindex libbuildmap.a $(RDMLIBS) $(LIBS)

rdmxchange : $(XCHGOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) -o rdmxchange $(XCHGOBJS) $(RDMLIBS) $(LIBS)

rdmkismet: roadmap_kismet.o $(RDMLIBS)
	$(CC) $(LDFLAGS) roadmap_kismet.o -o rdmkismet $(RDMLIBS) $(RDMLIBS) -lm

rdmghost: roadmap_ghost.o $(RDMLIBS)
	$(CC) $(LDFLAGS) roadmap_ghost.o -o rdmghost $(RDMLIBS) -lm

rdmfriends: roadmap_friends.o $(RDMLIBS)
	$(CC) $(LDFLAGS) roadmap_friends.o -o rdmfriends $(RDMLIBS) $(RDMLIBS) -lm

rdmtrace: roadmap_trace.o $(RDMLIBS)
	$(CC) $(LDFLAGS) roadmap_trace.o -o rdmtrace $(RDMLIBS) -lm

dumpmap: $(DMOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(DMOBJS) -o dumpmap $(LIBS)

buildmap: $(BMOBJS) libbuildmap.a $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BMOBJS) -o buildmap libbuildmap.a $(LIBS)

# buildus to be retired soon.
buildus: $(BUOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BUOBJS) -o buildus $(LIBS)

buildplace: $(BPOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BPOBJS) -o buildplace $(LIBS)


