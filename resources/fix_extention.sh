#!/bin/sh

echo=

d=$1

test -z "$d" && exit 1

for f in `find $d/skins -type f -name '*.bin'` ; do
	${echo} mv $f `echo $f | sed -e 's/\.bin$/.png/'`
done

for f in `find $d/sound -type f -name '*.bin'` ; do
	${echo} mv $f `echo $f | sed -e 's/\.bin$/.mp3/'`
done
