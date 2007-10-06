# GTK makefile

TOP = ..
include $(TOP)/options.mk

# --- GTK-specific options ---------------------------------------

LIBS += `gtk-config --libs`
CFLAGS += `gtk-config --cflags`

# --- GTK sources & targets -----------------------------------

RMLIBSRCS=roadmap_keyboard.c \
          roadmap_messagebox.c \
          roadmap_dialog.c \
          roadmap_fileselection.c \
          roadmap_signals.c

RMLIBOBJS = $(CANVAS_OBJS) $(RMLIBSRCS:.c=.o)

HEADERS = \
	roadmap_gtkcanvas.h \
	roadmap_gtkmain.h \
	roadmap_keyboard.h

TARGETS = gtkroadgps gtkroadmap

# --- Conventional targets ----------------------------------------

.PHONY: all strip clean install uninstall

all: $(TARGETS)

strip:
	$(STRIP) $(TARGETS)

clean:
	rm -f *.o *.a *.da $(TARGETS)

install:
	mkdir -p $(bindir)
	cd $(bindir) && rm -f $(TARGETS) roadmap roadgps
	install $(TARGETS) $(bindir)
	ln -s gtkroadmap $(bindir)/roadmap
	ln -s gtkroadgps $(bindir)/roadgps
	cd $(bindir) ; $(STRIP) $(TARGETS)

uninstall:
	cd $(bindir) && rm -f $(TARGETS) roadmap roadgps

sourcelist:
	@echo Makefile roadmap_main.c roadmap_canvas.c $(RMLIBSRCS) $(HEADERS)

# hopefully temporary
roadmap_canvas_agg.o:
	@echo
	@echo "*** Sorry!  We still need a roadmap_canvas_agg.cpp for GTK."
	@echo
	false

# --- The real targets --------------------------------------------

libgtkroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libgtkroadmap.a $(RMLIBOBJS)
	$(RANLIB) libgtkroadmap.a

gtkroadmap: roadmap_main.o libgtkroadmap.a \
		$(TOP)/libguiroadmap.a $(filter %.a, $(LIBS))
	$(CC) $(LDFLAGS) -o gtkroadmap roadmap_main.o \
		$(TOP)/libguiroadmap.a libgtkroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o libgtkroadmap.a \
		$(TOP)/libguiroadgps.a  $(filter %.a, $(LIBS))
	$(CC) $(LDFLAGS) -o gtkroadgps roadmap_main.o \
		$(TOP)/libguiroadgps.a libgtkroadmap.a $(LIBS)

