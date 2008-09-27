
# Those hand-made icons (the others are official road signs):
ICONSRC= rm_hold.xpm \
			rm_gps.xpm \
			rm_zoom1.xpm \
			rm_zoomin.xpm \
			rm_zoomout.xpm

ICONS=$(ICONSRC:.xpm=.png)

CONVERT=convert2

all: icons

install:
	rm -f temp.png
	cp rm_*.png $(ICONDIR)

uninstall:
	cd $(ICONDIR) && rm -f rm_*.png

icons:
	rm -f temp.png

clean: cleanone

cleanone:
	$(RM) $(ICONS)
	# Clean up CVS backup files as well.
	$(RM) .#*

%.png : %.xpm
	$(CONVERT) -transparent yellow $< temp.png
	$(CONVERT) -scale 16x16 temp.png $@
