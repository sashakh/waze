# --- Tool specific options ------------------------------------------------

TOP = ..
include $(TOP)/options.mk

# --- UNIX OS sources & targets --------------------------------------------

LIBSRCS=roadmap_file.c \
          roadmap_path.c \
          roadmap_library.c \
          roadmap_net.c \
          roadmap_serial.c \
          roadmap_spawn.c \
          roadmap_time.c

LIBOBJS=$(LIBSRCS:.c=.o)


# --- Conventional targets ----------------------------------------

.PHONY: all clean install uninstall

all: libosroadmap.a

clean:
	rm -f *.o *.a *.da

install:

uninstall:

sourcelist:
	@echo Makefile $(LIBSRCS)


# --- The real targets --------------------------------------------

libosroadmap.a: $(LIBOBJS)
	$(AR) $(ARFLAGS) libosroadmap.a $(LIBOBJS)
	$(RANLIB) libosroadmap.a

