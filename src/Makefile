
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

RMLIBOBJS = $(RMLIBSRC:.c=.o)


RMGUISRC = roadmap_sprite.c \
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
	roadmap_start.c

RMGUIOBJS = $(RMGUISRC:.c=.o)


RGGUISRC = roadgps_start.c \
	roadmap_factory.c \
	roadmap_help.c \
	roadgps_screen.c \
	roadgps_logger.c

RGGUIOBJS = $(RGGUISRC:.c=.o)


RMPLUGINSRC = roadmap_copy.c roadmap_httpcopy.c

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
	buildmap_empty.c \
	buildmap_place.c \
	buildmap_index.c \

BMLIBOBJS = $(BMLIBSRC:.c=.o)

BMSRC = buildmap_main.c

BMOBJS = $(BMSRC:.c=.o)


BPSRC = buildmap_messages.c \
	buildmap_dictionary.c \
	buildmap_dbwrite.c \
	buildmap_square.c \
	buildmap_point.c \
	buildmap_place.c \
	buildplace_main.c

BPOBJS = $(BPSRC:.c=.o)


BUSRC = buildus_main.c \
	buildus_fips.c \
	buildus_county.c \
	buildmap_dictionary.c \
	buildmap_dbwrite.c \
	buildmap_messages.c

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
	buildus_fips.h

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
	roadmap_path.h \
	roadmap_place.h \
	roadmap_plugin.h \
	roadmap_point.h \
	roadmap_pointer.h \
	roadmap_polygon.h \
	roadmap_preferences.h \
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
	roadmap_types.h \
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
	roadmap_dbread.h

HEADERS = $(BMHEADERS) $(XCHGHEADERS) $(RGHEADERS) $(RMHEADERS) $(RMDBHEADERS)

# more sources
PKGDATAFILES = sprites preferences session drivers \
		roadmap.popup roadmap.screenobjects
MISCFILES = roadmap.desktop roadmap.menu roadmap.1 roadgps.1 default/All \
		roadmap.toolbar.ex roadmap.actionlabels.ex \
		app_a02.txt usstates.txt
SCRIPTS = rdmdownload rdmcompare \
		rdmgenmaps rdmgenshpmaps rdmgenstatemaps rdmgendcwmaps
MKFILES = Makefile config.mk options.mk

# this font is GPL, and comes from RedHat's "liberation-fonts-ttf-3.tar.gz"
FONT = LiberationSans-Bold.ttf

# the manual is a post-processed version of README.  the conversion tool
# isn't common, so we distribute both.
MANUAL = manual.html

# more products
ifneq ($(strip $(POPT)),NO)
  # these depend on libpopt
  BUILD = buildmap buildus dumpmap buildplace rdmindex 
endif

RUNTIME = $(RDMLIBS) libguiroadgps.a libguiroadmap.a
ifneq ($(DESKTOP),WINCE)
DRIVERS = rdmkismet rdmghost rdmfriends rdmtrace rdmxchange
TOOLS = sunrise
endif

# the "ALL_..." variables are important in that they drive the
# creation of the release tarballs.
ALL_SRC = $(MKFILES) $(RMLIBSRC) $(RMGUISRC) $(RGGUISRC) \
	$(RMPLUGINSRC) $(DMSRC) $(BMLIBSRC) $(BMSRC) \
	$(BPSRC) $(BUSRC) $(XCHGSRC) $(MISCSRC) $(HEADERS) \
	$(PKGDATAFILES) $(MISCFILES) $(SCRIPTS) $(FONT) $(MANUAL)

ALL_SUBDIRS = unix gpx gtk gtk2 qt qt4 icons ipkg agg_support win32

# --- Conventional targets ----------------------------------------

.PHONY: all gtk gtk2 qt3 qt4 qpe4 rebuild build \
         topclean clean strip install uninstall unix gpx

all: runtime build $(TOOLS) icons


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
	rm -f *.o *.a *.da $(BUILD) $(TOOLS) $(DRIVERS)
	# Clean up CVS backup files as well.
	$(RM) .#*

clean:  topclean
	for module in icons $(OSDIR) gpx $(RDMODULES) ; \
	do \
		$(MAKE) -C $$module clean || exit 1; \
	done
	rm -f agg_support/*.o


install: installdata installbin installicons installdesktop installdoc
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

installbin:
	mkdir -p $(pkgbindir)
	cd $(pkgbindir) && rm -f $(BUILD) $(SCRIPTS)
	cp $(BUILD) $(SCRIPTS) $(DRIVERS) $(pkgbindir)/.
	cd $(pkgbindir) && chmod a+x $(BUILD) $(SCRIPTS) $(DRIVERS)
	mkdir -p $(man1dir)
	$(INSTALL_DATA) roadgps.1 roadmap.1 $(man1dir)/.

installdesktop:
	-test -d $(menudir) && cp roadmap.menu $(menudir)/roadmap
	mkdir -p $(desktopdir)
	cp -f roadmap.desktop $(desktopdir)/.

installicons:
	$(MAKE) -C icons install

installdoc: manual.html 
	-cp manual.html $(pkgdatadir)/manual.html

manual.html: ../README
	-aptconvert -pi html showTitle yes -toc -enc ASCII $@ $<

uninstall:
	cd $(pkgdatadir) && rm -f $(PKGDATAFILES)
	cd $(pkgbindir) && rm -f $(BUILD) $(SCRIPTS) $(DRIVERS)
	-test -d $(menudir) && rm -f $(menudir)/roadmap
	$(MAKE) -C icons uninstall
	cd $(desktopdir) && rm -f roadmap.desktop
	cd $(man1dir) && rm -f roadgps.1 roadmap.1
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

dumpmap: $(DMOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(DMOBJS) -o dumpmap $(LIBS)

buildmap: $(BMOBJS) libbuildmap.a $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BMOBJS) -o buildmap libbuildmap.a $(LIBS)

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
UPPER_SRC = README \
    AUTHORS BUGS TODO CHANGES COPYING INSTALL howtos/makemaps.txt 

# the contents of the tarball are listed explicitly, using the UPPER_SRC
# variable and the output of "make sourcelist".  an appropriate path
# prefix is added to make all relative paths correct.
tarball: linkdir 
	@tar --dereference -czvf $(RELNAME).tar.gz \
	    $(addprefix $(RELNAME)/, $(UPPER_SRC)) \
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
