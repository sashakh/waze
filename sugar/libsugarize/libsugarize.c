//////////////////////////////////////////
//
// gcc -W -Wall -I/usr/X11R6/include -L/usr/X11R6/lib -Os -shared -fpic -Wl,-soname,libsugarize.so -Wl,-z,initfirst -nostartfiles -o libsugarize.so libsugarize.c -lX11 -ldl -lc
//
// code from Albert Cahalan:
//   git://dev.laptop.org/users/albert/sugarize
// 

#define _GNU_SOURCE		// this is Linux source
#include <dlfcn.h>

#define XChangeProperty go_away
#include<X11/Xlib.h>
#undef XChangeProperty

#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <unistd.h>


static int (*xcp) (Display * dpy, Window win, Atom a_prop, Atom a_type,
		   int format, int mode, void *data, int num);

static Display *dpy2;
static Window win2 = -1;

static Atom a_activity;
static Atom a_bundle;
static Atom a_cardinal;
static Atom a_string;
static Atom a_pid;
static Atom a_name;
static Atom a_utf8;
static int pid;

static const char *sugar_activity;
static const char *sugar_bundle;
static const char *sugar_object;
static const char *sugar_uri;
static const char *net_wm_name;

int
XChangeProperty(Display * dpy, Window win, Atom a_prop, Atom a_type,
		int format, int mode, unsigned char *data, int num)
{
    // printf("0x%08x %d %d\n",win,!!sugar_activity,!!sugar_bundle);
    if (!xcp) {
	xcp = dlsym(RTLD_NEXT, "XChangeProperty");
	a_activity = XInternAtom(dpy, "_SUGAR_ACTIVITY_ID", 0);
	a_bundle = XInternAtom(dpy, "_SUGAR_BUNDLE_ID", 0);
	a_cardinal = XInternAtom(dpy, "CARDINAL", 0);
	a_string = XInternAtom(dpy, "STRING", 0);
	a_pid = XInternAtom(dpy, "_NET_WM_PID", 0);
	a_name = XInternAtom(dpy, "_NET_WM_NAME", 0);
	a_utf8 = XInternAtom(dpy, "UTF8_STRING", 0);
	pid = getpid();
    }
    int ret = xcp(dpy, win, a_prop, a_type, format, mode, data, num);
    if (dpy == dpy2 && win == win2)
	return ret;
    dpy2 = dpy;
    win2 = win;
    if (net_wm_name && *net_wm_name)
	xcp(dpy, win, a_name, a_utf8, 8, PropModeReplace, (void *)net_wm_name,
	    strlen(net_wm_name));
    xcp(dpy, win, a_pid, a_cardinal, 32, PropModeReplace,
	(unsigned char *) &pid, 1);
    if (sugar_activity)
	xcp(dpy, win, a_activity, a_string, 8, PropModeReplace, (void *)sugar_activity,
	    strlen(sugar_activity));
    if (sugar_bundle)
	xcp(dpy, win, a_bundle, a_string, 8, PropModeReplace, (void *)sugar_bundle,
	    strlen(sugar_bundle));
    return ret;
}

void
_init(int argc, char *argv[], char *envp[])
{
    (void)argc;
    (void)argv;
    int pos = 0;
    while (envp[pos]) {
	if (!memcmp
	    ("SUGAR_BUNDLE_ID=", envp[pos], sizeof("SUGAR_BUNDLE_ID=") - 1))
	    sugar_bundle = envp[pos] + sizeof("SUGAR_BUNDLE_ID=") - 1;
	if (!memcmp
	    ("SUGAR_ACTIVITY_ID=", envp[pos], sizeof("SUGAR_ACTIVITY_ID=") - 1))
	    sugar_activity = envp[pos] + sizeof("SUGAR_ACTIVITY_ID=") - 1;
	if (!memcmp
	    ("SUGAR_OBJECT_ID=", envp[pos], sizeof("SUGAR_OBJECT_ID=") - 1))
	    sugar_object = envp[pos] + sizeof("SUGAR_OBJECT_ID=") - 1;
	if (!memcmp("SUGAR_URI=", envp[pos], sizeof("SUGAR_URI=") - 1))
	    sugar_uri = envp[pos] + sizeof("SUGAR_URI=") - 1;
	if (!memcmp("NET_WM_NAME=", envp[pos], sizeof("NET_WM_NAME=") - 1))
	    net_wm_name = envp[pos] + sizeof("NET_WM_NAME=") - 1;
	pos++;
    }
}

/////////////////
