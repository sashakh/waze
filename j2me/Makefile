######################################################################
##
## Copyright (C) 2005,  Blekinge Institute of Technology
##
## Filename:      Makefile
## Author:        Simon Kagstrom <ska@bth.se>
## Description:   Generic Makefile
##
## $Id: Makefile,v 1.1.2.2 2007/03/16 16:09:38 eshabtai Exp $
##
######################################################################
SUBDIRS = c
RESOURCES=res/program.data.bin res/lang res/preferences res/schema res/sprites res/objects res/objects_wide res/roadmap-24.png res/welcome.png res/200.mp3 res/800.mp3 res/Arrive.mp3 res/KeepLeft.mp3 res/KeepRight.mp3 res/m.mp3 res/rec_end.mp3 res/rec_start.mp3 res/Straight.mp3 res/TurnLeft.mp3 res/TurnRight.mp3 res/within.mp3 \
#res/usc77001.rdm
#RESOURCES=res/program.data.bin res/preferences res/schema res/sprites res/objects res/objects_wide res/usdir.rdm res/usc77001.rdm
TARGET   =src/CompiledProgram.class FreeMap.jar FreeMap.jad

all: include/ $(SUBDIRS)

# This is actually not needed since we include Rules-j2me, but
# we keep it here to show how new syscall sets are added. Further,
# ansi is implicitly implied by j2me, so that can be removed as well
CIBYL_SYSCALL_SETS ?= javax.microedition ansi resource_manager softfloat command_mgr gps_manager jmicropolygon sound_mgr timer_mgr sony_ericsson

CIBYL_GENERATE_JAVA_WRAPPERS_OPTS=-I include -S syscalls -D JSR075

CIBYL_CLASSPATH = $(WTK_PATH)/lib/jsr082.jar:$(WTK_PATH)/lib/jsr75.jar:../nokiaext.jar
EMULATOR_OPTIONS = -Xheapsize:10M

include $(CIBYL_BASE)/build/Rules-j2me.mk

src/CibylConfig.java: CibylConfig.java
	cp $< $@

include/: FORCE
	$(CIBYL_BASE)/tools/cibyl-generate-c-header -o $@ syscalls/ command_mgr gps_manager sound_mgr timer_mgr sony_ericsson
FORCE:
