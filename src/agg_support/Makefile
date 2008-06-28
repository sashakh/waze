
TOP = ..
include $(TOP)/options.mk

# stub makefile -- sole purpose is to assist building tarball distributions

SOURCE = \
	roadmap_canvas.cpp

HEADERS = \
	roadmap_canvas_agg.h

sourcelist:
	@echo Makefile $(SOURCE) $(HEADERS)

depends:
	$(MAKEDEPS) -Y -f - $(SOURCE) > .depends.mk 2>/dev/null

-include .depends.mk
