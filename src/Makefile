
# --- Tool specific options ------------------------------------------------


TOP = .
include $(TOP)/options.mk

# --- RoadMap sources & targets --------------------------------------------

RMLIBSRC = roadmap_log.c \
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
	roadmap_osm.c \
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
	roadmap_vii.c \
	roadmap_object.c \
	roadmap_driver.c \
	roadmap_adjust.c \
	roadmap_sunrise.c \
	roadmap_state.c \
	roadmap_gpx.c \
	roadmap_linefont.c \
	roadmap_lang.c \
	roadmap_iso.c \
	roadmap_turns.c

RMLIBOBJS = $(RMLIBSRC:.c=.o)


RMGUISRC = roadmap_sprite.c \
	roadmap_object.c \
	roadmap_trip.c \
	roadmap_tripdb.c \
	roadmap_track.c \
	roadmap_landmark.c \
	roadmap_features.c \
	roadmap_layer.c \
	roadmap_fuzzy.c \
	roadmap_navigate.c \
	roadmap_pointer.c \
	roadmap_screen.c \
	roadmap_screen_obj.c \
	roadmap_display.c \
	roadmap_factory.c \
	roadmap_preferences.c \
	roadmap_address.c \
	roadmap_crossing.c \
	roadmap_coord.c \
	roadmap_download.c \
	roadmap_help.c \
	roadmap_label.c \
	roadgps_screen.c \
	roadgps_logger.c \
	roadmap_start.c

RMGUIOBJS = $(RMGUISRC:.c=.o)


RGGUISRC = roadgps_start.c \
	roadmap_factory.c \
	roadmap_help.c \
	roadgps_screen.c \
	roadgps_logger.c

RGGUIOBJS = $(RGGUISRC:.c=.o)

RMPLUGINSRC = \
	roadmap_copy.c \
	roadmap_httpcopy.c \
	$(EDITOR_PLUGINSRC) $(TRIP_PLUGINSRC) $(NAVIGATE_PLUGINSRC)

RMPLUGINOBJS=$(RMPLUGINSRC:.c=.o)


DMSRC = dumpmap_main.c

DMOBJS = $(DMSRC:.c=.o)


BMLIBSRC = buildmap_messages.c \
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
	buildmap_osm_binary.c \
	buildmap_osm_common.c \
	buildmap_osm_text.c \
	buildmap_empty.c \
	buildmap_place.c \
	buildmap_index.c \
	buildmap_opt.c \
	buildmap_turn_restrictions.c

BMLIBOBJS = $(BMLIBSRC:.c=.o)

BMSRC = buildmap_main.c

BMOBJS = $(BMSRC:.c=.o)

BMOSMSRC = buildmap_osm_main.c

BMOSMOBJS = $(BMOSMSRC:.c=.o)

BPSRC = buildmap_messages.c \
	buildmap_dictionary.c \
	buildmap_dbwrite.c \
	buildmap_square.c \
	buildmap_point.c \
	buildmap_place.c \
	buildmap_opt.c \
	buildplace_main.c

BPOBJS = $(BPSRC:.c=.o)


BUSRC = buildus_main.c \
	buildus_fips.c \
	buildus_county.c \
	buildmap_dictionary.c \
	buildmap_dbwrite.c \
	buildmap_messages.c \
	buildmap_opt.c

BUOBJS = $(BUSRC:.c=.o)


XCHGSRC = rdmxchange_main.c \
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

XCHGOBJS = $(XCHGSRC:.c=.o)


MISCSRC = \
	rdmindex_main.c \
	roadmap_kismet.c \
	roadmap_ghost.c \
	roadmap_friends.c \
	roadmap_trace.c

BMHEADERS = \
	buildmap.h \
	buildmap_area.h \
	buildmap_city.h \
	buildmap_empty.h \
	buildmap_index.h \
	buildmap_layer.h \
	buildmap_line.h \
	buildmap_metadata.h \
	buildmap_opt.h \
	buildmap_osm_binary.h \
	buildmap_osm_common.h \
	buildmap_osm_text.h \
	buildmap_place.h \
	buildmap_point.h \
	buildmap_polygon.h \
	buildmap_range.h \
	buildmap_shape.h \
	buildmap_shapefile.h \
	buildmap_square.h \
	buildmap_street.h \
	buildmap_tiger.h \
	buildmap_zip.h \
	buildus_county.h \
	roadmap_iso.h \
	buildus_fips.h \
	buildmap_turn_restrictions.h

XCHGHEADERS = \
	rdmxchange.h

RGHEADERS = \
	roadgps_logger.h \
	roadgps_screen.h

RMHEADERS = \
	roadmap.h \
	roadmap_address.h \
	roadmap_adjust.h \
	roadmap_canvas.h \
	roadmap_config.h \
	roadmap_coord.h \
	roadmap_copy.h \
	roadmap_copyright.h \
	roadmap_county.h \
	roadmap_crossing.h \
	roadmap_dialog.h \
	roadmap_dictionary.h \
	roadmap_display.h \
	roadmap_download.h \
	roadmap_driver.h \
	roadmap_factory.h \
	roadmap_features.h \
	roadmap_file.h \
	roadmap_fileselection.h \
	roadmap_fuzzy.h \
	roadmap_geocode.h \
	roadmap_gps.h \
	roadmap_gpsd2.h \
	roadmap_gpx.h \
	roadmap_gui.h \
	roadmap_hash.h \
	roadmap_help.h \
	roadmap_history.h \
	roadmap_httpcopy.h \
	roadmap_index.h \
	roadmap_input.h \
	roadmap_io.h \
	roadmap_label.h \
	roadmap_landmark.h \
	roadmap_lang.h \
	roadmap_layer.h \
	roadmap_library.h \
	roadmap_line.h \
	roadmap_linefont.h \
	roadmap_list.h \
	roadmap_locator.h \
	roadmap_main.h \
	roadmap_math.h \
	roadmap_message.h \
	roadmap_messagebox.h \
	roadmap_metadata.h \
	roadmap_navigate.h \
	roadmap_net.h \
	roadmap_nmea.h \
	roadmap_object.h \
	roadmap_osm.h \
	roadmap_path.h \
	roadmap_place.h \
	roadmap_plugin.h \
	roadmap_point.h \
	roadmap_pointer.h \
	roadmap_polygon.h \
	roadmap_preferences.h \
	roadmap_progress.h \
	roadmap_scan.h \
	roadmap_screen.h \
	roadmap_screen_obj.h \
	roadmap_serial.h \
	roadmap_shape.h \
	roadmap_spawn.h \
	roadmap_sprite.h \
	roadmap_square.h \
	roadmap_start.h \
	roadmap_state.h \
	roadmap_street.h \
	roadmap_string.h \
	roadmap_sunrise.h \
	roadmap_time.h \
	roadmap_track.h \
	roadmap_trigonometry.h \
	roadmap_trip.h \
	roadmap_tripdb.h \
	roadmap_turns.h \
	roadmap_types.h \
	roadmap_vii.h \
	roadmap_voice.h


RMDBHEADERS = \
	roadmap_db.h \
	roadmap_db_county.h \
	roadmap_db_dictionary.h \
	roadmap_db_index.h \
	roadmap_db_line.h \
	roadmap_db_metadata.h \
	roadmap_db_place.h \
	roadmap_db_point.h \
	roadmap_db_polygon.h \
	roadmap_db_range.h \
	roadmap_db_shape.h \
	roadmap_db_square.h \
	roadmap_db_street.h \
	roadmap_dbread.h \
	roadmap_db_turns.h

HEADERS = $(BMHEADERS) $(XCHGHEADERS) $(RGHEADERS) $(RMHEADERS) $(RMDBHEADERS)

# more sources
PKGDATAFILES = sprites preferences session drivers \
		roadmap.popup roadmap.screenobjects
MANPAGES = roadmap.1 roadgps.1
MISCFILES = roadmap.desktop roadmap.menu default/All \
		roadmap.toolbar.ex roadmap.actionlabels.ex \
		app_a02.txt usstates.txt roadmap.doxy
SCRIPTS = rdmdownload rdmcompare \
		rdmgenmaps rdmgenshpmaps rdmgenstatemaps rdmgendcwmaps
MKFILES = Makefile config.mk options.mk

# this font is GPL, and comes from RedHat's "liberation-fonts-ttf-3.tar.gz"
FONT = LiberationSans-Bold.ttf

# the manual is a post-processed version of README.  the conversion tool
# isn't common, so we distribute both.  (this version is slightly
# different than that built in the web subdirectory.)
MANUAL = usermanual.html

# more products
ifeq ($(DESKTOP),WINCE)
BUILD = # buildus
else
BUILD = buildmap buildmap_osm buildus buildplace dumpmap
# rdmindex -- removed from default build.  it's unfinished s/w, and
# its presence is confusing.
endif

RUNTIME = $(RDMLIBS) libguiroadmap.a
ifneq ($(DESKTOP),WINCE)
RUNTIME += libguiroadgps.a
DRIVERS = rdmkismet rdmghost rdmfriends rdmtrace rdmxchange
TOOLS = sunrise
endif

# the "ALL_..." variables are important in that they drive the
# creation of the release tarballs.
C_SRC = $(RMLIBSRC) $(RMGUISRC) $(RGGUISRC) \
	$(RMPLUGINSRC) $(DMSRC) $(BMLIBSRC) $(BMSRC) \
	$(BMOSMSRC) \
	$(BPSRC) $(BUSRC) $(XCHGSRC) $(MISCSRC)

ALL_SRC = $(MKFILES) $(C_SRC) $(HEADERS) \
	$(PKGDATAFILES) $(MISCFILES) $(MANPAGES) $(SCRIPTS) $(FONT) $(MANUAL)

ALL_SUBDIRS = unix gpx gtk gtk2 qt qt4 icons ipkg agg_support php win32

# --- Additions for Trip plugin -------------------------------

ifeq ($(USE_TRIP_PLUGIN),yes)
TRIP_PLUGINSRC = \
	trip/trip_bar.c \
	trip/trip_cost.c \
	trip/trip_instr.c \
	trip/trip.c \
	trip/trip_plugin.c \
	trip/trip_traffic.c

CFLAGS += -DHAVE_TRIP_PLUGIN

RMLIBSRC += roadmap_skin.c roadmap_res.c
# RMLIBSRC += roadmap_turns.c

RMHEADERS += roadmap_skin.h roadmap_res.h
#	roadmap_db_turns.h roadmap_turns.h

PLUGIN_CLEAN += ${TRIP_PLUGINSRC:.c=.o} ${RMLIBSRC:.c=.o}

RMGUISRC +=

ALL_SUBDIRS +=

else
TRIP_PLUGINSRC =
endif

# --- Additions for Navigate plugin -------------------------------

ifeq ($(USE_NAVIGATE_PLUGIN),yes)
NAVIGATE_PLUGINSRC = \
	navigate/navigate.c \
	navigate/navigate_bar.c \
	navigate/navigate_plugin.c \
	navigate/navigate_visual.c \
	navigate/navigate_cost.c \
	navigate/navigate_simple.c \
	navigate/navigate_route.c

NAVIGATE_PLUGIN_HDR = \
	navigate/navigate_bar.h \
	navigate/navigate_cost.h \
	navigate/navigate.h \
	navigate/navigate_plugin.h \
	navigate/navigate_route.h \
	navigate/navigate_simple.h \
	navigate/navigate_visual.h

CFLAGS += -DHAVE_NAVIGATE_PLUGIN

PLUGIN_CLEAN += ${NAVIGATE_PLUGINSRC:.c=.o}
RMLIBSRC += roadmap_skin.c roadmap_res.c
#	roadmap_turns.c

RMHEADERS += ${NAVIGATE_PLUGIN_HDR} \
	roadmap_skin.h roadmap_res.h
#       roadmap_db_turns.h roadmap_turns.h

RMGUISRC +=

ALL_SUBDIRS +=

else
NAVIGATE_PLUGINSRC =
endif

# --- Additions for Editor plugin -------------------------------

ifeq ($(USE_EDITOR_PLUGIN),yes)
EDITOR_PLUGINSRC = \
	editor/editor_plugin.c \
	editor/editor_main.c \
	editor/editor_screen.c \
	editor/static/editor_dialog.c \
	editor/static/update_range.c \
	editor/static/edit_marker.c \
	editor/static/notes.c \
	editor/db/editor_street.c \
	editor/db/editor_point.c \
	editor/db/editor_shape.c \
	editor/db/editor_line.c \
	editor/db/editor_square.c \
	editor/db/editor_route.c \
	editor/db/editor_marker.c \
	editor/db/editor_dictionary.c \
	editor/db/editor_override.c \
	editor/export/editor_export.c \
	editor/export/editor_download.c \
	editor/export/editor_upload.c \
	editor/export/editor_sync.c \
	editor/db/editor_trkseg.c \
	editor/track/editor_track_filter.c \
	editor/track/editor_track_util.c \
	editor/track/editor_track_main.c \
	editor/track/editor_track_unknown.c \
	editor/track/editor_track_known.c \
	editor/track/editor_gps_data.c \
	editor/db/editor_db.c \
	buildmap_dbwrite.c \
	buildmap_metadata.c \
	buildmap_messages.c

CFLAGS += -DHAVE_EDITOR_PLUGIN

RMLIBSRC += \

RMGUISRC +=

ALL_SUBDIRS +=

else
EDITOR_PLUGINSRC =
endif

# --- Conventional targets ----------------------------------------

.PHONY: all gtk gtk2 qt3 qt4 qpe4 rebuild build \
         topclean clean strip install uninstall unix gpx

all: runtime build $(TOOLS) icons $(MANUAL)


# --- Convenience targets, to force a specific desktop build ------
# (These targets override the default setting in config.mk, but
# you may need other settings from there anyway, so they may not work.)
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

wince:
	$(MAKE) DESKTOP=WINCE

rebuild: clean all


# ---  Sub-targets ------------------------------------------------

$(OSDIR) gpx:
	$(MAKE) -C $@ all


runtime: $(OSDIR) gpx $(RUNTIME) $(DRIVERS)
	for module in $(RDMODULES) ; \
	do \
		$(MAKE) -C $$module all || exit 1; \
	done

build: $(OSDIR) gpx $(BUILD)

strip:
	-$(STRIP) $(BUILD) $(DRIVERS)
	for module in $(RDMODULES) ; \
	do \
		$(MAKE) -C $$module strip || exit 1; \
	done

icons:
	$(MAKE) -C icons all

topclean:
	rm -f *.o *.a *.da .depends.mk $(BUILD) $(TOOLS) $(DRIVERS)
	# Clean up CVS backup files as well.
	$(RM) .#*

clean:  topclean
	for module in icons $(OSDIR) gpx $(RDMODULES) ; \
	do \
		$(MAKE) -C $$module clean || exit 1; \
	done
	rm -f agg_support/*.o agg_support/.depends.mk ${PLUGIN_CLEAN}


install: all installdata installbin installicons \
		installdesktop installdoc installman
	for module in $(RDMODULES) ; \
	do \
		$(MAKE) -C $$module install || exit 1; \
	done

installdata:
	mkdir -p $(pkgmapsdir)
	chmod a+rx $(pkgmapsdir)
	mkdir -p $(pkgdatadir)
	chmod a+rx $(pkgdatadir)
	mkdir -p $(pkgdatadir)/default
	chmod a+rx $(pkgdatadir)/default
	cd $(pkgdatadir) && rm -f $(PKGDATAFILES)
	cp $(PKGDATAFILES) $(pkgdatadir)/.
	cd $(pkgdatadir) ; chmod a+r $(PKGDATAFILES)
	cp default/All $(pkgdatadir)/default/.
	chmod a+r $(pkgdatadir)/default/All
ifneq ($(strip $(AGG)),NO)
	cp $(FONT) $(pkgdatadir)/font.ttf
endif
ifeq ($(strip $(LANGS)),YES)
	mkdir -p $(pkgdatadir)/langs
	chmod a+rx $(pkgdatadir)/langs
	cp langs/*_* $(pkgdatadir)/langs/.
	chmod a+r $(pkgdatadir)/langs/*
endif


installbin:
	mkdir -p $(pkgbindir)
	cd $(pkgbindir) && rm -f $(BUILD) $(SCRIPTS)
	cp $(BUILD) $(SCRIPTS) $(DRIVERS) $(pkgbindir)/.
	cp php/osmgetbmap php/osmgetbmapcore.php $(pkgbindir)/.
	cd $(pkgbindir) && \
		chmod a+x $(BUILD) $(SCRIPTS) $(DRIVERS) osmgetbmap

installdesktop:
	-test -d $(menudir) && cp roadmap.menu $(menudir)/roadmap
	mkdir -p $(desktopdir)
	cp -f roadmap.desktop $(desktopdir)/.

installicons:
	$(MAKE) -C icons install

installman: $(MANPAGES)
	mkdir -p $(man1dir)
	$(INSTALL_DATA) $(MANPAGES) $(man1dir)/.

installdoc: $(MANUAL) 
	-cp $(MANUAL) $(pkgdatadir)

$(MANUAL): ../README
	expand $< | \
	(cd ../doc; txt2tags --target html \
	    --toc --infile - --outfile - ) >$@
	 sed -i \
	    -e 's/@@.*runtime-help-link \([^ ]*\) *$$/<a name="\1"><\/a>/' \
	    -e '/"#toc1"/s/^/<a name="TableOfContents">/' \
	    $@

uninstall:
	cd $(pkgdatadir) && rm -f $(PKGDATAFILES)
	cd $(pkgbindir) && rm -f $(BUILD) $(SCRIPTS) $(DRIVERS)
	-test -d $(menudir) && rm -f $(menudir)/roadmap
	$(MAKE) -C icons uninstall
	cd $(desktopdir) && rm -f roadmap.desktop
	cd $(man1dir) && rm -f $(MANPAGES)
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

dumpmap: $(DMOBJS) libbuildmap.a $(RDMLIBS)
	$(CC) $(LDFLAGS) $(DMOBJS) -o dumpmap libbuildmap.a $(LIBS)

buildmap: $(BMOBJS) libbuildmap.a $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BMOBJS) -o buildmap libbuildmap.a $(LIBS)

buildmap_osm: $(BMOSMOBJS) libbuildmap.a $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BMOSMOBJS) -o buildmap_osm libbuildmap.a $(LIBS)

# buildus to be retired soon.
buildus: $(BUOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BUOBJS) -o buildus $(LIBS)

buildplace: $(BPOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BPOBJS) -o buildplace $(LIBS)


# --- distribution preparation targets

# pull the version indicator from roadmap_copyright.h
DOTVERSION=$(shell sed -n \
	-e '/ROADMAP_VERSION/s/.*"\(.*\)"/\1/p' roadmap_copyright.h)

RELNAME = roadmap-$(DOTVERSION)

.PHONY: checkdotversion sourcelist linkdir tarball webdoc

# source files from "..", i.e. the real top-level.  these go into tarball.
UPPER_SRC = README COPYING 
DOC_SRC = Bugs ChangeLog Coding Configuration Developer-Guide \
	Installation Map-Building Map-Designing Map-Format \
	OpenStreetMap Platforms Quickstart Status ToDo Usage

tarball: $(RELNAME)-src.tar.gz

# the contents of the tarball are listed explicitly, using the UPPER_SRC
# variable and the output of "make sourcelist".  an appropriate path
# prefix is added to make all relative paths correct.
$(RELNAME)-src.tar.gz: linkdir $(ALL_SRC)
	@tar --dereference -czvf $(RELNAME)-src.tar.gz \
	    $(addprefix $(RELNAME)/, $(UPPER_SRC)) \
	    $(addprefix $(RELNAME)/doc/, $(DOC_SRC)) \
	    $(addprefix $(RELNAME)/src/, $(shell \
		  $(MAKE) -C $(RELNAME)/src --no-print-directory sourcelist) \
	    )
	rm $(RELNAME)
	
# create a symlink to our own top-level directory, for the purposes of tarring
# up a tree with a reasonable name.
linkdir: checkdotversion
	rm -f $(RELNAME)
	ln -sf $(shell cd ..; pwd) $(RELNAME)

# force failure if the DOTVERSION environment variable is not set
checkdotversion:
	@echo Checking that \$$DOTVERSION is set
	test "$(DOTVERSION)"

# emit a list of the source at this and all lower levels
sourcelist:
	@echo $(ALL_SRC)
	@echo $(foreach dir, $(ALL_SUBDIRS), \
	    $(addprefix $(dir)/, \
		$(shell $(MAKE) -C $(dir) --no-print-directory sourcelist)))


depends::
	$(MAKEDEPS) -Y -f - $(C_SRC) > .depends.mk 2>/dev/null ; \
	for dir in $(ALL_SUBDIRS) ; \
	do \
		test -f $$dir/Makefile && \
			$(MAKE) -C $$dir depends || exit 1; \
	done

depends::
	awk '/\%\!include/ {printf("usermanual.html: ../doc/%s\n", $$2);}' ../README >>.depends.mk

.depends.mk:
	@echo "Note: run 'make depends' to create dependency files" >&2

-include .depends.mk

#
# Build API documentation based on doxygen
#
apidocs:	roadmap.doxy
	doxygen roadmap.doxy
#
# This special dependency makes sure you pick up changes to the Makefile.
# Psst: mention all the sources that have #ifdef HAVE_TRIP_PLUGIN or so.
#
roadmap_start.o:	Makefile config.mk
roadmap_line.o:		Makefile config.mk
roadmap_math.o:		Makefile config.mk
roadmap_point.o:	Makefile config.mk
roadmap_screen.o:	Makefile config.mk
roadmap_plugin.o:	Makefile config.mk
roadmap_trip.o:		Makefile config.mk
