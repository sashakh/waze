
# Those hand-made icons (the others are official road signs):
ICONSRC= rm_hold.xpm \
			rm_gps.xpm \
			rm_zoom1.xpm \
			rm_zoomin.xpm \
			rm_zoomout.xpm

ICONS=$(ICONSRC:.xpm=.png)


install: $(ICONS)
	rm -f temp.png
	cp rm_*.png $(ICONDIR)

icons: $(ICONS)
	rm -f temp.png

%.png : %.xpm
	convert -transparent yellow $< temp.png
	convert -scale 16x16 temp.png $@
