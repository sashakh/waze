# --- Tool specific options ------------------------------------------------

RANLIB = ranlib

HOST=`uname -s`
ifeq ($(HOST),Darwin)
	ARFLAGS="r"
else
	ARFLAGS="rf"
endif

# WARNFLAGS = -g
WARNFLAGS = -O2
MODECFLAGS= $(WARNFLAGS) -ffast-math -fomit-frame-pointer $(OPTIONS)
CFLAGS =  $(MODECFLAGS) $(STDCFLAGS)

# --- RoadMap sources & targets --------------------------------------------


OSLIBSRCS=\
          gpx.c \
          xmltag.c \
          waypt.c \
          route.c \
          util.c \
          queue.c \
          mkshort.c \
          formspec.c \
          vmem.c

OSLIBOBJS=$(OSLIBSRCS:.c=.o)


# --- Conventional targets ----------------------------------------

all: runtime

build:

runtime: libgpx.a

clean: cleanone

cleanone:
	rm -f *.o *.a *.da

install:

uninstall:


# --- The real targets --------------------------------------------

libgpx.a: $(OSLIBOBJS)
	$(AR) $(ARFLAGS) libgpx.a $(OSLIBOBJS)
	$(RANLIB) libgpx.a

