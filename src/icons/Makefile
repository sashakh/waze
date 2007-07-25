
TOP = ..
include $(TOP)/options.mk

# hand-made icons (the other rm_*.png icons are official road signs):
# the source for these is .xpm
RM_ICONS_XPM= rm_hold.xpm \
	rm_gps.xpm \
	rm_zoom1.xpm \
	rm_zoomin.xpm \
	rm_zoomout.xpm

RM_ICONS_PNG_FROM_XPM=$(RM_ICONS_XPM:.xpm=.png)

# the source for these is .png
RM_ICONS_PNG_FROM_CVS = \
	rm_clockwise.png \
	rm_counterclockwise.png \
	rm_destination.png \
	rm_down.png \
	rm_full.png \
	rm_left.png \
	rm_location.png \
	rm_quit.png \
	rm_record.png \
	rm_right.png \
	rm_stop.png \
	rm_up.png

# the rm_* icons we install are all .png
RM_ICONS_PNG = $(RM_ICONS_PNG_FROM_CVS) $(RM_ICONS_PNG_FROM_XPM) 

# the source for the roadmap* icons is all .png
ROADMAP_ICONS_PNG = \
	roadmap-16.png \
	roadmap-32.png \
	roadmap-48.png \
	roadmap-64.png \
	roadmap-right.png \
	roadmap-small.png \
	roadmap.png

ROADMAP_ICONS_XPM=$(ROADMAP_ICONS_PNG:.png=.xpm)

# the roadmap* icons we install are both the .png and .xpm variants
ROADMAP_ICONS_ALL = $(ROADMAP_ICONS_PNG) $(ROADMAP_ICONS_XPM)


.PHONY: all install uninstall icons clean

all: icons

icons: $(RM_ICONS_PNG_FROM_XPM) $(ROADMAP_ICONS_XPM)

install: $(RM_ICONS_PNG_FROM_XPM)
	mkdir -p $(DESTDIR)$(ICONDIR)
	cp -f $(RM_ICONS_PNG) $(ROADMAP_ICONS_ALL) $(DESTDIR)$(ICONDIR)

uninstall:
	cd $(DESTDIR)$(ICONDIR) && rm -f $(RM_ICONS_PNG) $(ROADMAP_ICONS_ALL)


clean:
	$(RM) $(RM_ICONS_PNG_FROM_XPM) $(ROADMAP_ICONS_XPM)
	# Clean up CVS backup files as well.
	$(RM) .#*

# icon rules -- use "convert" from imagemagick
%.png : %.xpm
	$(CONVERT) -transparent yellow $< temp.png
	$(CONVERT) -scale 16x16 temp.png $@
	rm -f temp.png

%.xpm : %.png
	$(CONVERT) $< $@

