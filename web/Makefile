
default: install

APT_WEBDOCS = \
	manual.html \
	buglist.html \
	changelog.html \
	familiar.html \
	todolist.html

PAGES = \
	index.html \
	documentation.html \
	download.html \
	list.html \
	makemaps.html \
	maps.html \
	screenshots.html \

SCREENSHOTS = \
	roadmap-gpe.jpg \
	stamp-gpe.jpg \
	screen-3d.png \
	stamp-3d.png \
	screen-ellis.jpg \
	stamp-ellis.jpg \
	screen-labels.png \
	stamp-labels.png \
	screen-market.jpg \
	stamp-market.jpg \
	screen-onscreen.gif \
	stamp-onscreen.gif \
	screen-preferences.jpg \
	stamp-preferences.jpg \
	screen-roadgps.jpg \
	stamp-roadgps.jpg \
	screen-route.png \
	stamp-route.png \
	screen-wince.jpg \
	stamp-wince.jpg

SUPPORT = \
	apt_toc.gif \
	background2.jpg \
	navbar.html \
	sitestandard.css \


SITESOURCE = $(SCREENSHOTS) $(APT_WEBDOCS) $(PAGES) $(SUPPORT)

SITESOURCE_COPIED = $(addprefix .copied-, $(SITESOURCE))

full-install:
	rm -f .copied-*
	$(MAKE) install

install: webdoc execbits $(SITESOURCE_COPIED)

.copied-% : %
	scp $< roadmap.digitalomaha.net:public_html/new
	touch $@

webdoc:
	$(MAKE) -C ../src webdoc

# we use the Apache XBitHack option in order to enable server-side
# includes for our .html pages.  this lets us easily include the navbar
# in all of them.  but it means they all have to have the user/group execute
# bits set.
execbits:
	chmod ug+x $(PAGES) $(APT_WEBDOCS)

