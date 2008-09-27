# --- Installation options -------------------------------------------------

DESTDIR=
INSTALLDIR=/usr/local
bindir=$(INSTALLDIR)/bin


# --- Tool specific options ------------------------------------------------

RANLIB=ranlib

GUICFLAGS=`gtk-config --cflags`
GUILDFLAGS=`gtk-config --libs`

WARNFLAGS = -W -Wall

STDCFLAGS=-O2 -ffast-math -fomit-frame-pointer -g $(WARNFLAGS)
# STDCFLAGS=-g $(WARNFLAGS) -DROADMAP_DEBUG_HEAP
# STDCFLAGS=-g $(WARNFLAGS) -pg -fprofile-arcs -g


RDMLIBS= ../libroadmap.a libgtkroadmap.a ../unix/libosroadmap.a ../libroadmap.a

HOST=`uname -s`
ifeq ($(HOST),Darwin)
	ARFLAGS="r"
else
	ARFLAGS="rf"
endif

CFLAGS=$(STDCFLAGS) $(GUICFLAGS) -I.. -I/usr/local/include
LIBS= -L/usr/local/libs $(RDMLIBS) $(GUILDFLAGS) -lm

# --- RoadMap sources & targets --------------------------------------------

RMLIBSRCS=roadmap_keyboard.c \
          roadmap_messagebox.c \
          roadmap_dialog.c \
          roadmap_fileselection.c \
          roadmap_canvas.c

RMLIBOBJS=$(RMLIBSRCS:.c=.o)

RUNTIME=gtkroadgps gtkroadmap


# --- Conventional targets -------------------------------------------------

all: runtime

others:
	$(MAKE) -C .. CFLAGS="$(STDCFLAGS)" runtime
	$(MAKE) -C .. -C unix CFLAGS="$(STDCFLAGS) -I.." runtime

build:
	$(MAKE) -C .. CFLAGS="$(STDCFLAGS)" build

runtime: others $(RUNTIME)

clean: cleanone
	$(MAKE) -C .. cleanone
	$(MAKE) -C .. -C unix cleanone

cleanone:
	rm -f *.o *.a *.da $(RUNTIME)

install: all
	$(MAKE) -C .. install
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME) roadmap roadgps
	install $(RUNTIME) $(DESTDIR)$(bindir)
	ln -s gtkroadmap $(DESTDIR)$(bindir)/roadmap
	ln -s gtkroadgps $(DESTDIR)$(bindir)/roadgps

uninstall:
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME) roadmap roadgps
	$(MAKE) -C .. uninstall

# --- The real targets --------------------------------------------

libgtkroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libgtkroadmap.a $(RMLIBOBJS)
	$(RANLIB) libgtkroadmap.a

gtkroadmap: roadmap_main.o ../libguiroadmap.a $(RDMLIBS)
	$(CC) roadmap_main.o -o gtkroadmap ../libguiroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o ../libguiroadgps.a $(RDMLIBS)
	$(CC) roadmap_main.o -o gtkroadgps ../libguiroadgps.a $(LIBS)

