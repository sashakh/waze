PREFIX := /usr/local
INSTALL:= install

export DESKTOP:=GTK2
export MODE:=DEBUG
export SSD:=YES
export TOUCH:=YES
export HI_RES_SCREEN:=YES
export FREEMAP_IL:=YES
export BIDI:=YES
export SHAPEFILES:=NO
export J2ME:=NO
#export RENDERING:=OPENGL
export TILESTORAGE:=SQLITE
export TTS:=YES
export LIBGPS:=YES
## export this to support Maemo5
#export NO_MENU:=YES
#export MAEMO5:=YES

all clean:
	$(MAKE) PROJ_NAME=$(PWD) -C src $@

maemo:
	$(MAKE) PROJ_NAME=$(PWD) MAEMO5=YES -C src all

install: install-waze

install-waze:
	$(INSTALL) -s -T src/gtk2/gtkroadmap $(PREFIX)/bin/waze

install install-user:
	$(MAKE) PREFIX=$(PREFIX) -C resources $@
