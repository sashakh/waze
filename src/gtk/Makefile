
GUICFLAGS=`gtk-config --cflags`
GUILDFLAGS=`gtk-config --libs`

STDCFLAGS=-O2 -ffast-math -fomit-frame-pointer -Wall
# STDCFLAGS=-g -Wall -DROADMAP_DEBUG_HEAP
# STDCFLAGS=-g -Wall -pg -fprofile-arcs -g

CFLAGS=$(STDCFLAGS) $(GUICFLAGS) -I..

RDMLIBS= ../libroadmap.a libgtkroadmap.a ../unix/libosroadmap.a
LIBS=$(RDMLIBS) $(GUILDFLAGS) -lm

# LDFLAGS=-pg $(LDFLAGS)


RMLIBSRCS=roadmap_keyboard.c \
          roadmap_messagebox.c \
          roadmap_dialog.c \
          roadmap_canvas.c

RMLIBOBJS=$(RMLIBSRCS:.c=.o)

RUNTIME=gtkroadgps gtkroadmap

INSTALLDIR=/usr/local


# --- Conventional targets ----------------------------------------

all: build runtime

build:
	cd ..; make CFLAGS="$(STDCFLAGS)" build

runtime: $(RUNTIME)

clean:
	rm -f *.o *.a *.da $(RUNTIME)
	cd ..; make clean
	cd ../unix; make clean

install: all
	cd .. ; make install
	cd $(INSTALLDIR)/bin; rm -f $(RUNTIME) roadmap roadgps
	cp $(RUNTIME) $(INSTALLDIR)/bin
	ln -s $(INSTALLDIR)/bin/gtkroadmap $(INSTALLDIR)/bin/roadmap
	ln -s $(INSTALLDIR)/bin/gtkroadgps $(INSTALLDIR)/bin/roadgps
	cd $(INSTALLDIR)/bin; chmod a+x $(RUNTIME)

uninstall:
	cd $(INSTALLDIR)/bin ; rm -f $(RUNTIME) roadmap roadgps
	cd .. ; make uninstall

# --- The real targets --------------------------------------------

../libroadmap.a:
	cd ..; make CFLAGS="$(STDCFLAGS)" runtime

../libguiroadmap.a:
	cd ..; make CFLAGS="$(STDCFLAGS)" runtime

../libguiroadgps.a:
	cd ..; make CFLAGS="$(STDCFLAGS)" runtime

../unix/libosroadmap.a:
	cd ../unix; make CFLAGS="$(STDCFLAGS) -I.." all

libgtkroadmap.a: $(RMLIBOBJS)
	ar rf libgtkroadmap.a $(RMLIBOBJS)

gtkroadmap: roadmap_main.o $(RDMLIBS) ../libguiroadmap.a
	$(CC) roadmap_main.o -o gtkroadmap ../libguiroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o $(RDMLIBS) ../libguiroadgps.a
	$(CC) roadmap_main.o -o gtkroadgps ../libguiroadgps.a $(LIBS)

