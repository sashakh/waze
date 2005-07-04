# --- Tool specific options ------------------------------------------------

RANLIB = ranlib

HOST=`uname -s`
ifeq ($(HOST),Darwin)
	ARFLAGS="r"
else
	ARFLAGS="rf"
endif


# --- RoadMap sources & targets --------------------------------------------

OSLIBSRCS=roadmap_file.c \
          roadmap_path.c \
          roadmap_library.c \
          roadmap_net.c \
          roadmap_serial.c \
          roadmap_spawn.c \
          roadmap_time.c

OSLIBOBJS=$(OSLIBSRCS:.c=.o)


# --- Conventional targets ----------------------------------------

all: runtime

build:

runtime: libosroadmap.a

clean: cleanone

cleanone:
	rm -f *.o *.a *.da

install:

uninstall:


# --- The real targets --------------------------------------------

libosroadmap.a: $(OSLIBOBJS)
	$(AR) $(ARFLAGS) libosroadmap.a $(OSLIBOBJS)
	$(RANLIB) libosroadmap.a

