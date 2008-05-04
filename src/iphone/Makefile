# iPhone makefile

TOP = ..
include $(TOP)/options.mk

# --- iPhone-specific options ---------------------------------------

ROADMAP_BUNDLE=RoadMap.app
ROADGPS_BUNDLE=RoadGps.app
LIBS += 
LDFLAGS += -L/home/ditlemor/usr/local/lib -lobjc -ObjC \
	-framework CoreFoundation -framework Foundation \
	-framework UIKit -framework LayerKit -framework CoreGraphics \
	-framework GraphicsServices

# --- iPhone sources & targets -----------------------------------

# note that source is Objective C, hence .m suffix

RMLIBSRCS=roadmap_messagebox.m \
          roadmap_dialog.m \
          roadmap_fileselection.m \
		  roadmap_progress.m

RMLIBOBJS = $(CANVAS_OBJS) $(RMLIBSRCS:.m=.o)

HEADERS = \
	roadmap_iphonecanvas.h \
	roadmap_iphonemain.h \
	roadmap_iphonemessagebox.h \
	colors.h

TARGETS = iphoneroadgps iphoneroadmap

# --- Conventional targets ----------------------------------------

.PHONY: all strip clean install uninstall

all: $(TARGETS)

strip:
	$(STRIP) $(TARGETS)

clean:
	rm -f *.o *.a *.da $(TARGETS) 
	rm -rf $(ROADMAP_BUNDLE) $(ROADGPS_BUNDLE)

install:
	mkdir -p $(ROADMAP_BUNDLE)/resources/default
	cp iphoneroadmap $(INSTALLDIR)/RoadMap
	cp Info.plist.roadmap $(INSTALLDIR)/Info.plist
	cp ../icons/iphone_default.png $(INSTALLDIR)/icon.png

uninstall:
	cd $(bindir) && rm -f $(TARGETS) iphoneroadmap iphoneroadgps

sourcelist:
	@echo Makefile roadmap_main.m \
	    roadmap_canvas.m \
	    $(RMLIBSRCS) $(HEADERS)

# --- The real targets --------------------------------------------

libiphoneroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libiphoneroadmap.a $(RMLIBOBJS)
	$(RANLIB) libiphoneroadmap.a

%.o: %.m
	$(CC) $(CFLAGS) -c $< -o $@

iphoneroadmap: roadmap_main.o libiphoneroadmap.a \
		$(TOP)/libguiroadmap.a $(filter %.a, $(LIBS))
	$(CXX) $(LDFLAGS) -o iphoneroadmap roadmap_main.o \
		$(TOP)/libguiroadmap.a libiphoneroadmap.a $(LIBS)

iphoneroadgps: roadmap_main.o libiphoneroadmap.a \
		$(TOP)/libguiroadgps.a  $(filter %.a, $(LIBS))
	$(CXX) $(LDFLAGS) -o iphoneroadgps roadmap_main.o \
		$(TOP)/libguiroadgps.a libiphoneroadmap.a $(LIBS)

