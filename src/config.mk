#
# config.mk
#
# This file controls how RoadMap gets built (at least on unix
# "Makefile-based" platforms).  Try and keep your customizations
# here.  You can override these settings from the commandline
# if you wish, e.g. "make MODE=DEBUG".

# If you change settings in this file, it's quite possible that
# you'll need to do a "make clean" before trying your build again.
# (In other words, it may appear to build correctly, and in some
# cases might, but there are no guarantees.)

# Basic building mode
MODE =		# blank for "normal" build
# MODE=DEBUG	# enables -g, disables -O, turns on mtrace leak detection
# MODE=PROFILE	# sets up for profiling with gprof

# Add any other special local CFLAGS values here
CFLAGS =

# if you know you need other libraries for your build, you can
# add those here as well.  for example:
# LIBS += -L$(QTDIR)/lib64
LIBS =

# Choose a desktop
# DESKTOP = GTK
DESKTOP = GTK2
# DESKTOP = GPE
# DESKTOP = QT
# DESKTOP = QPE

# if you select QT or QPE above, you might also want to set
# QTDIR here:
QTDIR = /usr
# if you are using a build of QT without the QPainter::rotate()
# call builtin (i.e., built with QT_NO_TRANSFORMATIONS, as in
# some Familiar builds), set QT_NO_ROTATE
# QT_NO_ROTATE = YES

# RoadMap uses the "expat" library in order to read and write xml
# for the GPX format route/track/waypoint files.  If you don't
# libexpat.a on your system, you can still use RoadMap, but you
# won't be able to import or export your route and waypoint data.
EXPAT = YES
# EXPAT = NO

# RoadMap users in the USA will probably use the Tiger maps from
# the US Census bureau.  These maps do not requre "shapefile"
# support when building the rdm format of the maps.  Users in
# other areas will likely be building maps from other sources,
# and will probably need shapefile support.
SHAPEFILES = NO
# SHAPEFILES = YES

# In order to properly label streets on the map, it's necessary
# to be able to draw text at arbitrary angles.  RoadMap uses the
# "Anti-Grain Geometry" library, libagg.a, in order to do this. 
# If you don't have this library, RoadMap can still do street
# labels, but they will all be horizontal, and won't look as
# nice.  (for more information on AGG, see http://antigrain.com)
# [ Note -- currently, only GTK2 supports AGG builds.  ]
# AGG = YES
AGG = NO

# Some languages need to present text that flows from right to to
# left.  RoadMap supports this via the "Free Implementation of
# the BiDirectional" algorithm library, (aka "FriBidi" -- for
# more information, see http://fribidi.org )  (AGG is also
# required.)
BIDI = NO
# BIDI = YES

# RoadMap internal profiling -- you probably don't want this.
# DBG_TIME = YES
