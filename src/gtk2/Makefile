
GUICFLAGS=`pkg-config --cflags gtk+-2.0`
GUILDFLAGS=`pkg-config --libs gtk+-2.0` $(X11LDFLAGS)

GPECFLAGS=-DROADMAP_USES_GPE -I/usr/local/include
GPELDFLAGS=-L/usr/local/lib -lgpewidget -ldisplaymigration

RDMLIBS= ../libroadmap.a libgtkroadmap.a ../unix/libosroadmap.a ../libroadmap.a
STRIP=strip

WARNFLAGS = -W -Wall

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
   CFLAGS=$(STDCFLAGS) $(GPECFLAGS) $(GUICFLAGS) -I..
   LIBS=$(RDMLIBS) $(GPELDFLAGS) $(GUILDFLAGS) -lm
else
   CFLAGS=$(STDCFLAGS) $(GUICFLAGS) -I..
   LIBS=$(RDMLIBS) $(GUILDFLAGS) -lm
endif


RMLIBSRCS=roadmap_keyboard.c \
          roadmap_messagebox.c \
          roadmap_dialog.c \
          roadmap_fileselection.c \
          roadmap_canvas.c

RMLIBOBJS=$(RMLIBSRCS:.c=.o)

RUNTIME=gtkroadgps gtkroadmap

DESTDIR=
INSTALLDIR=/usr/local
bindir=$(INSTALLDIR)/bin

# --- Conventional targets ----------------------------------------

.PHONY: all others build runtime clean cleanone install uninstall

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
	$(STRIP) $(RUNTIME)
	mkdir -p $(DESTDIR)$(bindir)
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME) roadmap roadgps
	install $(RUNTIME) $(DESTDIR)$(bindir)
	ln -s gtkroadmap $(DESTDIR)$(bindir)/roadmap
	ln -s gtkroadgps $(DESTDIR)$(bindir)/roadgps

uninstall:
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME) roadmap roadgps
	$(MAKE) -C .. uninstall

# --- The real targets --------------------------------------------

libgtkroadmap.a: $(RMLIBOBJS)
	$(AR) rf libgtkroadmap.a $(RMLIBOBJS)

gtkroadmap: roadmap_main.o ../libguiroadmap.a $(RDMLIBS)
	$(CC) $(LDFLAGS) roadmap_main.o -o gtkroadmap ../libguiroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o ../libguiroadgps.a $(RDMLIBS)
	$(CC) $(LDFLAGS) roadmap_main.o -o gtkroadgps ../libguiroadgps.a $(LIBS)

