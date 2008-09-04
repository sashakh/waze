
# i'm pretty sure this libsugarize thing is by Albert Cahalan.

.PHONY: clean install xo

CFLAGS:=-s -W -Wall -Os

libsugarize.so:
	gcc $(CFLAGS) -I/usr/X11R6/include -L/usr/X11R6/lib \
	    -shared -fpic -Wl,-soname,libsugarize.so \
	    -Wl,-z,initfirst -nostartfiles -o libsugarize.so \
	    libsugarize.c -lX11 -ldl -lc

clean:
	rm -rf *.o core libsugarize.so

