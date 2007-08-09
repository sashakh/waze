
TOP = ..
include $(TOP)/options.mk

# the rm_*.png icons are used by roadmap itself.  these must be
# in PNG format.  the original source for some of them is in XPM
# format, but we keep the PNG results around as well.  (so that
# the "convert" program can be optional.)

# the source for the hand-made icons is .xpm
RM_ICONS_PNG_BUILDABLE = \
	rm_hold.png \
	rm_gps.png \
	rm_zoom1.png \
	rm_zoomin.png \
	rm_zoomout.png

# the "official road signs" are .png
RM_ICONS_PNG_CVS_ONLY = \
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

RM_ICONS_PNG = $(RM_ICONS_PNG_CVS_ONLY) $(RM_ICONS_PNG_BUILDABLE) 


# the roadmap_* icons are for "system" use -- for the menu system,
# etc.  the source for the roadmap* icons is all .png, but we try
# to install XPM versions as well, if "convert" is available.
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
ROADMAP_ICONS_ALL = $(RM_ICONS_PNG) $(ROADMAP_ICONS_PNG)

# the only pieces we try to build are the roadmap*.xpm files, and
# these are optional if the "convert" program is missing.
all:
	@echo
	@echo "If the $(CONVERT) program is missing, this next step(s)"
	@echo "  may fail.  These errors can be ignored."
	@echo
	-$(MAKE) $(ROADMAP_ICONS_XPM)

clean:
	$(RM) $(ROADMAP_ICONS_XPM)

# manually invoking this target will cause "convert" to be
# used to bring the .png versions of the rm_*.xpm files up-to-date.
rm_icons: $(RM_ICONS_PNG_BUILDABLE)
rm_icons_clean:
	$(RM) $(RM_ICONS_PNG_BUILDABLE)


install: $(RM_ICONS_PNG)
	mkdir -p $(DESTDIR)$(icondir)
	cp -f $(RM_ICONS_PNG) 	    $(DESTDIR)$(icondir)
	cp -f $(ROADMAP_ICONS_PNG)  $(DESTDIR)$(icondir)
	@echo
	@echo "The next copy command may fail."
	@echo
	-cp -f $(ROADMAP_ICONS_XPM) $(DESTDIR)$(icondir)

uninstall:
	cd $(DESTDIR)$(icondir) && rm -f $(RM_ICONS_PNG) $(ROADMAP_ICONS_ALL)

sourcelist:
	@echo Makefile $(ROADMAP_ICONS_ALL)

# create .png from our hand-made .xpm sources
rm_%.png : rm_%.xpm
	$(CONVERT) -transparent yellow $< temp.png
	$(CONVERT) -scale 16x16 temp.png $@
	rm -f temp.png

road%.xpm : road%.png
	$(CONVERT) $< $@

.PHONY: all install uninstall icons clean rm_icons

