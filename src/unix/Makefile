

OSLIBSRCS=roadmap_file.c \
          roadmap_path.c \
          roadmap_library.c \
          roadmap_net.c \
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
	ar rf libosroadmap.a $(OSLIBOBJS)

