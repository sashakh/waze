
GUICFLAGS=`gtk-config --cflags`
GUILDFLAGS=`gtk-config --libs`

WARNFLAGS = -W -Wall

STDCFLAGS=-O2 -ffast-math -fomit-frame-pointer -g $(WARNFLAGS)
# STDCFLAGS=-g $(WARNFLAGS) -DROADMAP_DEBUG_HEAP
# STDCFLAGS=-g $(WARNFLAGS) -pg -fprofile-arcs -g

CFLAGS=$(STDCFLAGS) $(GUICFLAGS) -I..

RDMLIBS= ../libroadmap.a libgtkroadmap.a ../unix/libosroadmap.a ../libroadmap.a
LIBS=$(RDMLIBS) $(GUILDFLAGS) -lm

# LDFLAGS=-pg $(LDFLAGS)


RMLIBSRCS=roadmap_keyboard.c \
          roadmap_messagebox.c \
          roadmap_dialog.c \
          roadmap_fileselection.c \
          roadmap_canvas.c

RMLIBOBJS=$(RMLIBSRCS:.c=.o)

RUNTIME=gtkroadgps gtkroadmap

INSTALLDIR=/usr/local


# --- Conventional targets ----------------------------------------

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
	$(AR) rf libgtkroadmap.a $(RMLIBOBJS)

gtkroadmap: roadmap_main.o ../libguiroadmap.a $(RDMLIBS)
	$(CC) roadmap_main.o -o gtkroadmap ../libguiroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o ../libguiroadgps.a $(RDMLIBS)
	$(CC) roadmap_main.o -o gtkroadgps ../libguiroadgps.a $(LIBS)

