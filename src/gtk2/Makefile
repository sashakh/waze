# GTK2 makefile

TOP = ..
include $(TOP)/options.mk

# --- GTK2-specific options ---------------------------------------

LIBS += `pkg-config --libs gtk+-2.0`
CFLAGS += `pkg-config --cflags gtk+-2.0`

ifeq ($(DESKTOP),GPE)
	CFLAGS +=-DROADMAP_USES_GPE
	LIBS += -lgpewidget -ldisplaymigration
endif


# --- GTK2 sources & targets -----------------------------------

SOURCE = roadmap_main.c \
	    roadmap_canvas.c roadmap_canvas_agg.cpp

RMLIBSRCS = roadmap_keyboard.c \
          roadmap_messagebox.c \
          roadmap_dialog.c \
          roadmap_fileselection.c \
          roadmap_signals.c \
          roadmap_progress.c

OBJS = $(CANVAS_OBJS) $(RMLIBSRCS:.c=.o)

TARGETS = gtkroadgps gtkroadmap

HEADERS = \
	roadmap_gtkcanvas.h \
	roadmap_gtkmain.h \
	roadmap_keyboard.h

# --- Conventional targets ----------------------------------------

.PHONY: all strip clean install uninstall

all: $(TARGETS)

strip:
	$(STRIP) $(TARGETS)

clean:
	rm -f *.o *.a *.da $(TARGETS) .depends.mk

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
	@echo Makefile roadmap_main.c \
	    roadmap_canvas.c roadmap_canvas_agg.cpp \
	    $(RMLIBSRCS) $(HEADERS)

# --- The real targets --------------------------------------------

libgtkroadmap.a: $(OBJS)
	$(AR) $(ARFLAGS) libgtkroadmap.a $(OBJS)
	$(RANLIB) libgtkroadmap.a

gtkroadmap: roadmap_main.o libgtkroadmap.a \
		$(TOP)/libguiroadmap.a $(filter %.a, $(LIBS))
	$(GTK2CC) $(LDFLAGS) -o gtkroadmap roadmap_main.o \
		$(TOP)/libguiroadmap.a libgtkroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o libgtkroadmap.a \
		$(TOP)/libguiroadgps.a  $(filter %.a, $(LIBS))
	$(GTK2CC) $(LDFLAGS) -o gtkroadgps roadmap_main.o \
		$(TOP)/libguiroadgps.a libgtkroadmap.a $(LIBS)


depends:
	$(MAKEDEPS) -Y -f - $(SOURCE) $(RMLIBSRCS) > .depends.mk 2>/dev/null

-include .depends.mk
