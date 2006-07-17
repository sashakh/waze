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


# --- Conventional targets ----------------------------------------

all: runtime

runtime: libgpx.a

clean:
	rm -f *.o *.a *.da

install:

uninstall:


# --- The real targets --------------------------------------------

libgpx.a: $(LIBOBJS)
	$(AR) $(ARFLAGS) libgpx.a $(LIBOBJS)
	$(RANLIB) libgpx.a

