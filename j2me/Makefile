######################################################################
##
## Copyright (C) 2005,  Blekinge Institute of Technology
##
## Filename:      Makefile
## Author:        Simon Kagstrom <ska@bth.se>
## Description:   Generic Makefile
##
## $Id: Makefile,v 1.6 2008/02/04 10:09:47 eshabtai Exp $
##
######################################################################
SUBDIRS = c
RESOURCES=res/program.data.bin res/lang res/preferences res/schema res/sprites res/objects res/objects_wide res/roadmap-24.png res/welcome.png res/200.mp3 res/800.mp3 res/Arrive.mp3 res/KeepLeft.mp3 res/KeepRight.mp3 res/m.mp3 res/rec_end.mp3 res/rec_start.mp3 res/Straight.mp3 res/TurnLeft.mp3 res/TurnRight.mp3 res/within.mp3 \
res/nav_approaching.png res/nav_exit_right.png res/nav_panel.png res/nav_turn_right.png res/nav_continue.png res/nav_keep_left.png res/nav_panel_wide.png res/nav_u_turn.png res/nav_exit_left.png res/nav_keep_right.png res/nav_turn_left.png res/nav_panel_small.png \
res/usc77001.rdm
#RESOURCES=res/program.data.bin res/preferences res/schema res/sprites res/objects res/objects_wide res/usdir.rdm res/usc77001.rdm
#TARGET   =src/CompiledProgram.class FreeMap.jar FreeMap.jad
TARGET   =FreeMap.jar FreeMap.jad

all: include/ $(SUBDIRS)

# This is actually not needed since we include Rules-j2me, but
# we keep it here to show how new syscall sets are added. Further,
# ansi is implicitly implied by j2me, so that can be removed as well
CIBYL_SYSCALL_SETS ?= javax.microedition ansi resource_manager softfloat command_mgr gps_manager jmicropolygon sound_mgr timer_mgr sony_ericsson 
#CIBYL_SYSCALL_SETS ?= javax.microedition ansi resource_manager softfloat command_mgr gps_manager nokiapolygon sound_mgr timer_mgr sony_ericsson

CIBYL_GENERATE_JAVA_WRAPPERS_OPTS=-I include -S syscalls -D JSR075

#CIBYL_MIPS2JAVA_OPTS ?=-Iinclude -O --generate-java-call-table --call-table-hierarchy=10 --class-size-limit=5000 --optimize-indirect-calls --optimize-inline-all-builtins --optimize-register-scheduling --optimize-colocate-functions=roadmap_screen_repaint_now,roadmap_screen_repaint_square,roadmap_screen_draw_square 
#CIBYL_MIPS2JAVA_OPTS ?=-Iinclude -O --call-table-hierarchy=3 --single-class --optimize-indirect-calls --optimize-inline-all-builtins --optimize-colocate-functions=roadmap_screen_repaint_now,roadmap_screen_repaint_square,roadmap_screen_draw_square --save-temps --use-direct-memory-references
#--trace-range=0x010242ac,0x01024400

CIBYL_MIPS2JAVA_OPTS ?=-Iinclude -O --call-table-hierarchy=3 --optimize-indirect-calls --optimize-inline-all-builtins --optimize-colocate-functions=roadmap_screen_repaint_now,roadmap_screen_repaint_square,roadmap_screen_draw_square --use-direct-memory-references

#CIBYL_MIPS2JAVA_OPTS ?=-Iinclude --call-table-hierarchy=3 --single-class  --single-class --save-temps
CIBYL_CLASSPATH = $(WTK_PATH)/lib/jsr082.jar:$(WTK_PATH)/lib/jsr75.jar:../nokiaext.jar:../jsr179.jar:jsr179.jar
EMULATOR_OPTIONS = -Xheapsize:10M

include $(CIBYL_BASE)/build/Rules-j2me.mk

src/CibylConfig.java: CibylConfig.java
	cp $< $@

include/: FORCE
	$(CIBYL_BASE)/tools/cibyl-generate-c-header -o $@ syscalls/ command_mgr gps_manager sound_mgr timer_mgr sony_ericsson 
FORCE:
