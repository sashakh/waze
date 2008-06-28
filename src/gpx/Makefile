# --- Tool specific options ------------------------------------------------

TOP = ..
include $(TOP)/options.mk

# --- RoadMap sources & targets --------------------------------------------


LIBSRCS=\
          gpx.c \
          xmltag.c \
          waypt.c \
          route.c \
          util.c \
          mkshort.c \
          formspec.c \
          vmem.c

LIBOBJS=$(LIBSRCS:.c=.o)

HEADERS = \
	defs.h \
	gbtypes.h

# --- Conventional targets ----------------------------------------

all: runtime

runtime: libgpx.a

clean:
	rm -f *.o *.a *.da .depends.mk

install:

uninstall:

sourcelist:
	@echo README Makefile $(LIBSRCS) $(HEADERS)

# --- The real targets --------------------------------------------

libgpx.a: $(LIBOBJS)
	$(AR) $(ARFLAGS) libgpx.a $(LIBOBJS)
	$(RANLIB) libgpx.a


depends:
	$(MAKEDEPS) -Y -f - $(LIBSRCS) > .depends.mk 2>/dev/null

-include .depends.mk
