
TOP = ..
include $(TOP)/options.mk

# Those hand-made icons (the others are official road signs):
ICONSRC= rm_hold.xpm \
	rm_gps.xpm \
	rm_zoom1.xpm \
	rm_zoomin.xpm \
	rm_zoomout.xpm

ICONS=$(ICONSRC:.xpm=.png)

.PHONY: all install uninstall icons clean

all: icons

install: $(ICONS)
	rm -f temp.png
	cp rm_*.png $(DESTDIR)$(ICONDIR)

uninstall:
	cd $(DESTDIR)$(ICONDIR) && rm -f rm_*.png

icons: $(ICONS)
	rm -f temp.png

clean:
	$(RM) $(ICONS)
	# Clean up CVS backup files as well.
	$(RM) .#*

%.png : %.xpm
	$(CONVERT) -transparent yellow $< temp.png
	$(CONVERT) -scale 16x16 temp.png $@
