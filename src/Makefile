

CFLAGS=-O2 -ffast-math -fomit-frame-pointer -Wall
# CFLAGS=-g -Wall -DROADMAP_DEBUG_HEAP
# CFLAGS=-g -Wall -pg -fprofile-arcs -g

RDMLIBS=libroadmap.a unix/libosroadmap.a
LIBS=$(RDMLIBS) -lpopt -lm

# LDFLAGS=-pg $(LDFLAGS)


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
          roadmap_history.c \
          roadmap_nmea.c \
          roadmap_gps.c

RMLIBOBJS=$(RMLIBSRCS:.c=.o)


RMGUISRCS=roadmap_sprite.c \
          roadmap_trip.c \
          roadmap_layer.c \
          roadmap_navigate.c \
          roadmap_screen.c \
          roadmap_display.c \
          roadmap_factory.c \
          roadmap_preferences.c \
          roadmap_address.c \
          roadmap_start.c

RMGUIOBJS=$(RMGUISRCS:.c=.o)


RGGUISRCS=roadgps_start.c \
          roadmap_factory.c \
          roadgps_screen.c \
          roadgps_logger.c

RGGUIOBJS=$(RGGUISRCS:.c=.o)


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
RUNTIME=libroadmap.a libguiroadgps.a libguiroadmap.a

INSTALLDIR=/usr/local


# --- Conventional targets ----------------------------------------

all: everything

runtime: $(RUNTIME)

build: $(BUILD)

strip:
	strip -s $(RUNTIME)
	strip -s $(BUILD)

clean: cleanone
	make -C qt cleanone
	make -C gtk cleanone
	make -C gtk2 cleanone
	make -C unix cleanone

cleanone:
	rm -f *.o *.a *.da $(BUILD)

install: all
	mkdir -p $(INSTALLDIR)/share/roadmap
	cd $(INSTALLDIR)/share/roadmap ; rm -f sprites preferences
	cp sprites preferences schema $(INSTALLDIR)/share/roadmap
	cd $(INSTALLDIR)/share/roadmap ; chmod a+r sprites preferences
	cd $(INSTALLDIR)/bin ; rm -f $(BUILD) $(SCRIPTS)
	cp $(BUILD) $(SCRIPTS) $(INSTALLDIR)/bin
	cd $(INSTALLDIR)/bin ; chmod a+x $(BUILD) $(SCRIPTS)
	if [ -d /usr/lib/menu ] ; then cp roadmap.menu /usr/lib/menu/roadmap; fi

uninstall:
	cd $(INSTALLDIR)/bin ; rm -f $(BUILD) $(SCRIPTS)
	cd $(INSTALLDIR) ; rm -rf share/roadmap
	rm -f /usr/lib/menu/roadmap

everything: build
	cd gtk ; make STDCFLAGS="$(CFLAGS)" all
	cd gtk2 ; make STDCFLAGS="$(CFLAGS)" all
	cd qt ; make STDCFLAGS="$(CFLAGS)" all

cleanall:
	cd gtk ; make clean
	cd gtk2 ; make clean
	cd qt ; make clean

rebuild: cleanall everything

# --- The real targets --------------------------------------------

libroadmap.a: $(RMLIBOBJS)
	ar rf libroadmap.a $(RMLIBOBJS)

libguiroadmap.a: $(RMGUIOBJS)
	ar rf libguiroadmap.a $(RMGUIOBJS)

libguiroadgps.a: $(RGGUIOBJS)
	ar rf libguiroadgps.a $(RGGUIOBJS)

unix/libosroadmap.a:
	cd unix; make "CFLAGS=$(CFLAGS) -I.."

dumpmap: $(DMOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(DMOBJS) -o dumpmap $(LIBS)

buildmap: $(BMOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BMOBJS) -o buildmap $(LIBS)

buildus: $(BUOBJS) $(RDMLIBS)
	$(CC) $(LDFLAGS) $(BUOBJS) -o buildus $(LIBS)

