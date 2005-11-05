# --- Installation options -------------------------------------------------

DESTDIR=
INSTALLDIR=/usr/local
bindir=$(INSTALLDIR)/bin


# --- Tool specific options ------------------------------------------------

GUICFLAGS=`pkg-config --cflags gtk+-2.0`
GUILDFLAGS=`pkg-config --libs gtk+-2.0` $(X11LDFLAGS)


WARNFLAGS = -W -Wall -Wno-unused-parameter

RDMLIBS= ../libroadmap.a libgtkroadmap.a ../unix/libosroadmap.a ../libroadmap.a


ifeq ($(MODE),DEBUG)
   STDCFLAGS=-g $(WARNFLAGS) -DROADMAP_DEBUG_HEAP
else
ifeq ($(MODE),PROFILE)
   STDCFLAGS=-g $(WARNFLAGS) -pg -fprofile-arcs -g
   LDFLAGS=-pg $(LDFLAGS)
else
   STDCFLAGS=-O2 -ffast-math -fomit-frame-pointer $(WARNFLAGS) $(OPTIONS)
endif
endif

ifeq ($(DESKTOP),GPE)
	GPECFLAGS=-DROADMAP_USES_GPE
	GPELDFLAGS=-lgpewidget -ldisplaymigration
else
	GPECFLAGS=
	GPELDFLAGS=
endif


HOST=`uname -s`
ifeq ($(HOST),Darwin)
	ARFLAGS="r"
else
	ARFLAGS="rf"
endif


CFLAGS = $(STDCFLAGS) $(GPECFLAGS) $(GUICFLAGS) -I.. -I/usr/local/include
LIBS = -L/usr/local/lib $(RDMLIBS) $(GPELDFLAGS) $(GUILDFLAGS) -lm

# --- RoadMap sources & targets -----------------------------------

RMLIBSRCS=roadmap_keyboard.c \
          roadmap_messagebox.c \
          roadmap_dialog.c \
          roadmap_fileselection.c \
          roadmap_canvas.c

RMLIBOBJS=$(RMLIBSRCS:.c=.o)

RUNTIME=gtkroadgps gtkroadmap


# --- Conventional targets ----------------------------------------

.PHONY: all runtime clean install uninstall strip

all: runtime

runtime: $(RUNTIME)

clean:
	rm -f *.o *.a *.da $(RUNTIME)

strip:
	$(STRIP) -s $(RUNTIME)

install:
	mkdir -p $(DESTDIR)$(bindir)
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME) roadmap roadgps
	install $(RUNTIME) $(DESTDIR)$(bindir)
	ln -s gtkroadmap $(DESTDIR)$(bindir)/roadmap
	ln -s gtkroadgps $(DESTDIR)$(bindir)/roadgps

uninstall:
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME) roadmap roadgps

# --- The real targets --------------------------------------------

libgtkroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libgtkroadmap.a $(RMLIBOBJS)
	$(RANLIB) libgtkroadmap.a

gtkroadmap: roadmap_main.o ../libguiroadmap.a $(RDMLIBS)
	$(CC) $(LDFLAGS) roadmap_main.o -o gtkroadmap ../libguiroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o ../libguiroadgps.a $(RDMLIBS)
	$(CC) $(LDFLAGS) roadmap_main.o -o gtkroadgps ../libguiroadgps.a $(LIBS)

