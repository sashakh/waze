
# all this does is make it easier to keep the website in sync

all: webdoc


# These are the NON-txt2tags pages -- i.e., they're currently maintained
# as HTML by hand.
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

# Most of the pages are are derived from the files in the doc
# directory.  We combine the available chapters of doc text into
# some standalone pages as well as some compilations.
T2T_WEBDOCS = \
	usermanual.html \
	devguide.html \
	buglist.html \
	status.html \
	todolist.html \
	changelog.html \
	installation.html \
	quickstart.html \
	platforms.html


# should match the list of %!includes in README
USER_GUIDE_SECTIONS = \
	Quickstart \
	Installation \
	Usage \
	Configuration \
	OpenStreetMap \
	Map-Building
USER_GUIDE_SECTION_PATHS = $(addprefix ../doc/, $(USER_GUIDE_SECTIONS))


# should match the list of %!includes in Developer-Guide
DEVGUIDE_SECTIONS = \
	Coding \
	Map-Building \
	Map-Format \
	Map-Designing
DEVGUIDE_SECTION_PATHS = $(addprefix ../doc/, $(DEVGUIDE_SECTIONS))


SUPPORT = \
	background2.jpg \
	roadmap-webicon.png \
	sitestandard.css

# expanded locally, not installed:
#	navbar.html \
#	sitesponsor.html \


# everything we'll install on the website
SITESOURCE = $(SCREENSHOTS) $(T2T_WEBDOCS) $(PAGES) $(SUPPORT)

# flag files that say we did the copy
SITESOURCE_COPIED = $(addprefix .copied$(SF)-, $(SITESOURCE))

# Support both Paul and Danny
ifeq ($(USER),danny)
SF_UID=dannybackx
else
SF_UID=pgf
endif
# use 'make SF=1 install' to send to SF
ifeq ($(strip $(SF)),)
OUR_SITE_REFERENCE = roadmap.sourceforge.net
OUR_SITE_ACCESS = mulch:/var/web/home/tmp/nroadmap
else
OUR_SITE_REFERENCE = roadmap.sourceforge.net
OUR_SITE_ACCESS = $(SF_UID),roadmap@web.sourceforge.net:htdocs
endif

# copy the html, and touch a local flag file.  before copying,
# locally expand our SSI #include directive.  this removes the
# need for SSI on the host site (which may require .shtml extension).
.copied$(SF)-%.html : %.html
	sed -e '/include virtual="navbar.html"/r navbar.html' \
	    -e '/include virtual="navbar.html"/d' \
	    -e '/INCLUDE_SITE_SPONSOR_HERE/r sitesponsor.html' \
	    -e '/INCLUDE_SITE_SPONSOR_HERE/d' \
	    $< >foo.html && \
	scp foo.html $(OUR_SITE_ACCESS)/$<
	# rm -f foo.html
	touch $@

.copied$(SF)-% : %
	scp $< $(OUR_SITE_ACCESS)/$<
	touch $@

roadmap-webicon.png: ../src/icons/roadmap-64.png
	cp $< $@

# several parts of the RoadMap website are produced directly from
# text files in the CVS tree, converted to html using txt2tags.
# (http://txt2tags.sourceforge.net)

# use GNU make's target-specific variables to name the sourcefile
# for each constructed web page.  the names are somewhat random,
# so we can't simply use a pattern rule.
usermanual.html: DOC_SOURCE = ../README
usermanual.html: ../README $(USER_GUIDE_SECTION_PATHS)

devguide.html: DOC_SOURCE = ../doc/Developer-Guide
devguide.html: ../doc/Developer-Guide $(DEVGUIDE_SECTION_PATHS)

buglist.html: DOC_SOURCE = ../doc/Bugs
buglist.html: ../doc/Bugs

changelog.html: DOC_SOURCE = ../doc/ChangeLog
changelog.html: ../doc/ChangeLog

todolist.html: DOC_SOURCE = ../doc/ToDo
todolist.html: ../doc/ToDo

status.html: DOC_SOURCE = ../doc/Status
status.html: ../doc/Status

installation.html: DOC_SOURCE = ../doc/Installation
installation.html: ../doc/Installation

quickstart.html: DOC_SOURCE = ../doc/Quickstart
quickstart.html: ../doc/Quickstart

platforms.html: DOC_SOURCE = ../doc/Platforms
platforms.html: ../doc/Platforms


# here's the rule for building webpages from our
# txt2tags-compatible text files.  we expand tabs (since they can
# confuse txt2tags) and post-process, to add some extra html at
# the top, to make the docs fit in with the rest of the site.  we
# also remove absolute references to our own website, so that the
# links all work properly when installed in a test tree.
$(T2T_WEBDOCS):
	expand $(DOC_SOURCE) | \
	(cd ../doc; txt2tags --target html \
	    --toc --infile - --outfile - ) >$@
	sed -i \
	    -e '/<HEAD>/s/$$/<link href="sitestandard.css" type=text\/css rel=stylesheet>/' \
	    -e '/<BODY .*>/a \
	    <!--#include virtual="navbar.html" -->' \
	    -e 's/<BODY .*>/<body class="roadmap-doc">/' \
	    -e 's;http://'$(OUR_SITE_REFERENCE)'/;;' \
	    -e 's/@@.*runtime-help-link \(.*\) *$$/<a name="\1"><\/a>/' $@

favicon.ico: ../src/icons/roadmap-64.png 
	mkdir -p favicon.tmpdir
	cd favicon.tmpdir ;\
	pngtopnm ../../src/icons/roadmap-64.png >roadmap-64.pnm ;\
	for size in 16 24 32 48 ;\
	do \
	  for color in 16 256 ;\
	  do \
	    pnmscale -xsize $$size -ysize $$size roadmap-64.pnm >favicon-$$size-.ppm ;\
	    pnmcolormap $$color favicon-$$size-.ppm >favicon--$$color.ppm ;\
	    pnmremap -mapfile=favicon--$$color.ppm favicon-$$size-.ppm >favicon-$$size-$$color.ppm ;\
	    rm -f favicon-$$size-.ppm favicon--$$color.ppm ;\
	    ff="$$ff favicon-$$size-$$color.ppm" ;\
	  done ;\
	done ;\
	ppmtowinicon $$ff >../favicon.ico
	rm -rf favicon.tmpdir


webdoc: $(T2T_WEBDOCS)

install:	webdoc $(SITESOURCE_COPIED)

clean-webdoc:
	rm -f $(T2T_WEBDOCS)


clean: clean-webdoc
	rm -f .copied*


clean-html:
	rm -f .copied*.html
	rm -f .copied*.css
