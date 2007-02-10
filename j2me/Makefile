######################################################################
##
## Copyright (C) 2005,  Blekinge Institute of Technology
##
## Filename:      Makefile
## Author:        Simon Kagstrom <ska@bth.se>
## Description:   Generic Makefile
##
## $Id: Makefile,v 1.1.2.1 2007/02/10 16:09:36 eshabtai Exp $
##
######################################################################
SUBDIRS = c
RESOURCES=res/program.data.bin res/preferences res/schema res/sprites res/objects res/objects_wide res/usdir.rdm res/usc77001.rdm
TARGET   =src/CompiledProgram.class roadmap.jar roadmap.jad

all: include/ $(SUBDIRS)

# This is actually not needed since we include Rules-j2me, but
# we keep it here to show how new syscall sets are added. Further,
# ansi is implicitly implied by j2me, so that can be removed as well
CIBYL_SYSCALL_SETS ?= javax.microedition ansi resource_manager softfloat command_mgr gps_manager jmicropolygon

CIBYL_GENERATE_JAVA_WRAPPERS_OPTS=-I include -S syscalls

CIBYL_CLASSPATH = $(WTK_PATH)/lib/jsr082.jar
EMULATOR_OPTIONS = -Xheapsize:10M

include $(CIBYL_BASE)/build/Rules-j2me.mk

src/CibylConfig.java: CibylConfig.java
	cp $< $@

include/: FORCE
	$(CIBYL_BASE)/tools/cibyl-generate-c-header -o $@ syscalls/ command_mgr gps_manager
FORCE:
