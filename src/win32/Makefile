
# WinCE Makefile, based around the arm-wince-mingw32ce cross toolchain,
# which comes from http://cegcc.sourceforge.net/
# Code built this way *DOES NOT WORK* as of july 2007.
# The code may work if built with microsoft tools, in which case this
# makefile isn't used, and the project files under wince_env/ may
# need updating.
#
# support for win32 and wince is better in the roadmap_editor
# branch of RoadMap CVS, so it's probably worth looking there for
# fixes if things are really obviously broken.  that branch only
# uses native windows tools, though -- not mingw32ce.
#


TOP = ..
include $(TOP)/options.mk

RMLIBSRC= \
	roadmap_messagebox.c \
	roadmap_dialog.c \
	roadmap_file.c \
	roadmap_library.c \
	roadmap_main.cpp \
	roadmap_fileselection.c \
	roadmap_net.c \
	roadmap_path.c \
	roadmap_serial.c \
	roadmap_spawn.c \
	roadmap_time.c \
	roadmap_win32.c \
	time.c \
	wince_input_mon.c \

#	roadmap_keyboard.c \
#	roadmap_signals.c \

RMLIBOBJ1 = $(RMLIBSRC:.cpp=.o)
RMLIBOBJ = $(CANVAS_OBJS) $(RMLIBOBJ1:.c=.o)

HEADERS = \
	resource.h \
	newres.h \
	colors.h \
	time.h \
	roadmap_win32.h \
	roadmap_wincecanvas.h \
	wince_input_mon.h

TARGETS = wroadgps wroadmap

# --- The real targets --------------------------------------------

all: $(TARGETS)

strip:
	$(STRIP) $(TARGETS)

clean:
	rm -f *.o *.a *.da $(TARGETS)

libwroadmap.a: $(RMLIBOBJ)
	$(AR) $(ARFLAGS) libwroadmap.a $(RMLIBOBJ)
	$(RANLIB) libwroadmap.a

wroadmap: roadmap_main.o libwroadmap.a \
		$(TOP)/libguiroadmap.a $(filter %.a, $(LIBS))
	$(CC) $(LDFLAGS) -o wroadmap roadmap_main.o \
		$(TOP)/libguiroadmap.a libwroadmap.a $(LIBS) \
		libwroadmap.a -lwinsock -lcommctrl -laygshell -lws2

wroadgps: roadmap_main.o libwroadmap.a \
		$(TOP)/libguiroadgps.a  $(filter %.a, $(LIBS))
	$(CC) $(LDFLAGS) -o wroadgps roadmap_main.o \
		$(TOP)/libguiroadgps.a libwroadmap.a $(LIBS) \
		libwroadmap.a -lwinsock -lcommctrl -laygshell -lws2


# for win32 we aren't as rigorous about producing
# this list of source as we are for the other RoadMap
# directories.  (using wildcards will do for now.)
sourcelist:
	@ls Makefile \
	    *.[ch] *.cpp *.ico \
	    roadmap.rc preferences \
	    install.inf \
	    wince_env/Roadmap.vcw \
	    wince_env/*/*.vcp \
	    icons/rm_*.bmp
