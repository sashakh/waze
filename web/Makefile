
# all this does is make it easier to keep the website in sync

default: all

APT_WEBDOCS = \
	manual.html \
	buglist.html \
	changelog.html \
	installation.html \
	todolist.html \
	makemaps.html

PAGES = \
	index.html \
	documentation.html \
	download.html \
	list.html \
	maps.html \
	screenshots.html

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
	roadmap-right.png \
	sitestandard.css

# expanded locally, not installed:
#	navbar.html \
#	sitesponsor.html \


SITESOURCE = $(SCREENSHOTS) $(APT_WEBDOCS) $(PAGES) $(SUPPORT)

all: webdoc


SITESOURCE_COPIED = $(addprefix .copied-, $(SITESOURCE))
install: all $(SITESOURCE_COPIED)

#OUR_SITE_REFERENCE = www.foxharp.boston.ma.us/tmp/nroadmap
#OUR_SITE_ACCESS = mulch:/var/web/home/tmp/nroadmap
OUR_SITE_REFERENCE = roadmap.sourceforge.net
OUR_SITE_ACCESS = roadmap.sourceforge.net:/home/groups/r/ro/roadmap/htdocs

# copy the html, and touch a local flag file.  before copying,
# locally expand our SSI #include directive.  this removes the
# need for SSI on the host site (which may require .shtml extension).
.copied-%.html : %.html
	sed -e '/include virtual="navbar.html"/r navbar.html' \
	    -e '/include virtual="navbar.html"/d' \
	    -e '/INCLUDE_SITE_SPONSOR_HERE/r sitesponsor.html' \
	    -e '/INCLUDE_SITE_SPONSOR_HERE/d' \
	    $< >foo.html && \
	scp foo.html $(OUR_SITE_ACCESS)/$<
	rm -f foo.html
	touch $@

.copied-% : %
	scp $< $(OUR_SITE_ACCESS)/$<
	touch $@

roadmap-right.png: ../src/icons/roadmap-right.png
	cp $< $@

# several parts of the RoadMap website are produced directly from
# text files in the CVS tree, converted to html using aptconvert.
# (http://www.xmlmind.com/aptconvert.html)

webdoc: $(APT_WEBDOCS)

clean-webdoc:
	rm -f $(APT_WEBDOCS)

# use GNU make's target-specific variables to name the sourcefile
# for each constructed web page.  the names are somewhat random,
# so we can't simply use a pattern rule.
manual.html: APT_SOURCE=../README
manual.html: ../README

makemaps.html: APT_SOURCE=../howtos/makemaps.txt
makemaps.html: ../howtos/makemaps.txt

buglist.html: APT_SOURCE=../BUGS
buglist.html: ../BUGS

changelog.html: APT_SOURCE=../CHANGES
changelog.html: ../CHANGES

todolist.html: APT_SOURCE=../TODO
todolist.html: ../TODO

installation.html: APT_SOURCE=../INSTALL
installation.html: ../INSTALL

# here's the rule for building webpages from our
# aptconvert-compatible text files.  we post-process, to add some
# extra html at the top, to make the docs fit in with the rest of
# the site.  we also remove absolute references to our own website,
# so that the links all work properly when installed in a test tree.

$(APT_WEBDOCS):
	aptconvert -pi html showTitle yes -toc -enc ASCII $@ $(APT_SOURCE)
	sed -i \
	    -e '/<head>/s/$$/<link href="sitestandard.css" type=text\/css rel=stylesheet>/' \
	    -e '/<body>/a \
	    <!--#include virtual="navbar.html" -->' \
	    -e 's/<body>/<body class="roadmap-doc">/' \
	    -e 's;http://'$(OUR_SITE_REFERENCE)'/;;' $@ 

favicon.ico:
	mkdir -p favicon.tmpdir
	cd favicon.tmpdir ;\
	pngtopnm ../../src/icons/roadmap-32.png >roadmap-32.pnm ;\
	for size in 16 24 32 48 ;\
	do \
	  for color in 16 256 ;\
	  do \
	    pnmscale -xsize $$size -ysize $$size roadmap-32.pnm >favicon-$$size-.ppm ;\
	    pnmcolormap $$color favicon-$$size-.ppm >favicon--$$color.ppm ;\
	    pnmremap -mapfile=favicon--$$color.ppm favicon-$$size-.ppm >favicon-$$size-$$color.ppm ;\
	    rm -f favicon-$$size-.ppm favicon--$$color.ppm ;\
	    ff="$$ff favicon-$$size-$$color.ppm" ;\
	  done ;\
	done ;\
	ppmtowinicon $$ff >../favicon.ico
	rm -rf favicon.tmpdir



clean: clean-webdoc
	rm -f .copied-*


clean-html:
	rm -f .copied-*.html
	rm -f .copied-*.css
