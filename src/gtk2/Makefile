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

RMLIBSRCS=roadmap_keyboard.c \
          roadmap_messagebox.c \
          roadmap_dialog.c \
          roadmap_fileselection.c \
          roadmap_signals.c

RMLIBOBJS = $(CANVAS_OBJS) $(RMLIBSRCS:.c=.o)

TARGETS = gtkroadgps gtkroadmap

# --- Conventional targets ----------------------------------------

.PHONY: all strip clean install uninstall

all: $(TARGETS)

strip:
	$(STRIP) $(TARGETS)

clean:
	rm -f *.o *.a *.da $(TARGETS)

install:
	$(MAKE) -C $(TOP) install
	$(STRIP) $(TARGETS)
	mkdir -p $(DESTDIR)$(bindir)
	cd $(DESTDIR)$(bindir) && rm -f $(TARGETS) roadmap roadgps
	install $(TARGETS) $(DESTDIR)$(bindir)
	ln -s gtkroadmap $(DESTDIR)$(bindir)/roadmap
	ln -s gtkroadgps $(DESTDIR)$(bindir)/roadgps

uninstall:
	cd $(DESTDIR)$(bindir) && rm -f $(TARGETS) roadmap roadgps
	$(MAKE) -C $(TOP) uninstall

# --- The real targets --------------------------------------------

libgtkroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libgtkroadmap.a $(RMLIBOBJS)
	$(RANLIB) libgtkroadmap.a

gtkroadmap: roadmap_main.o libgtkroadmap.a \
		$(TOP)/libguiroadmap.a $(filter %.a, $(LIBS))
	$(CXX) $(LDFLAGS) -o gtkroadmap roadmap_main.o \
		$(TOP)/libguiroadmap.a libgtkroadmap.a $(LIBS)

gtkroadgps: roadmap_main.o libgtkroadmap.a \
		$(TOP)/libguiroadgps.a  $(filter %.a, $(LIBS))
	$(CXX) $(LDFLAGS) -o gtkroadgps roadmap_main.o \
		$(TOP)/libguiroadgps.a libgtkroadmap.a $(LIBS)

