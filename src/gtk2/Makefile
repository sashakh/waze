
GUICFLAGS=`pkg-config --cflags gtk+-2.0` -DGTK_DISABLE_DEPRECATED=1
GUILDFLAGS=`pkg-config --libs gtk+-2.0`

GPECFLAGS=-DROADMAP_USES_GPE -I/usr/local/include
GPELDFLAGS=-L/usr/local/lib -lgpewidget -ldisplaymigration

RDMLIBS= ../libroadmap.a libgtkroadmap.a ../unix/libosroadmap.a

ifeq ($(MODE),DEBUG)
   STDCFLAGS=-g -Wall -DROADMAP_DEBUG_HEAP
else
ifeq ($(MODE),PROFILE)
   STDCFLAGS=-g -Wall -pg -fprofile-arcs -g
   LDFLAGS=-pg $(LDFLAGS)
else
   STDCFLAGS=-O2 -ffast-math -fomit-frame-pointer -Wall
endif
endif

ifeq ($(TARGET),GPE)
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

INSTALLDIR=/usr/local

# --- Conventional targets ----------------------------------------

.PHONY: all others build runtime clean cleanone install uninstall

all: runtime

others:
	make -C .. CFLAGS="$(STDCFLAGS)" runtime
	make -C .. -C unix CFLAGS="$(STDCFLAGS) -I.." runtime

build:
	make -C .. CFLAGS="$(STDCFLAGS)" build

runtime: others $(RUNTIME)

clean: cleanone
	make -C .. cleanone
	make -C .. -C unix cleanone

cleanone:
	rm -f *.o *.a *.da $(RUNTIME)

install: all
	make -C .. install
	cd $(INSTALLDIR)/bin; rm -f $(RUNTIME) roadmap roadgps
	cp $(RUNTIME) $(INSTALLDIR)/bin
	ln -s $(INSTALLDIR)/bin/gtkroadmap $(INSTALLDIR)/bin/roadmap
	ln -s $(INSTALLDIR)/bin/gtkroadgps $(INSTALLDIR)/bin/roadgps
	cd $(INSTALLDIR)/bin; chmod a+x $(RUNTIME)

uninstall:
	cd $(INSTALLDIR)/bin ; rm -f $(RUNTIME) roadmap roadgps
	make -C .. uninstall

# --- The real targets --------------------------------------------

libgtkroadmap.a: $(RMLIBOBJS)
	ar rf libgtkroadmap.a $(RMLIBOBJS)

gtkroadmap: roadmap_main.o ../libguiroadmap.a $(RDMLIBS)
	$(CC) roadmap_main.o -o gtkroadmap ../libguiroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o ../libguiroadgps.a $(RDMLIBS)
	$(CC) roadmap_main.o -o gtkroadgps ../libguiroadgps.a $(LIBS)

