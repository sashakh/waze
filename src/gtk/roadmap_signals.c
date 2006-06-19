/*
 * This code appears, in its original form, as a sample program written
 * by Hendrik Tews, as a demonstration of how to catch signals in an
 * addendum he wrote for the gtk+ 2.0 tutorial.  His original page is
 * here:   http://wwwtcs.inf.tu-dresden.de/~tews/Gtk/a2955.html
 *
 * Adapted (backported to gtk 1.2) for RoadMap by Paul Fox.
 */


/* set up for catching signals */

#include <stdio.h>      /* for printf and friends */
#include <stdlib.h>     /* for exit */
#include <unistd.h>     /* for write, pipe */
#include <fcntl.h>      /* for fcntl, O_NONBLOCK */
#include <signal.h>     /* for signal */
#include <gtk/gtk.h>        /* well guess for what ... */

#include "roadmap.h"

void roadmap_main_exit (void);

/* 
 * Unix signals that are caught are written to a pipe. The pipe connects 
 * the unix signal handler with GTK's event loop. The array signal_pipe will 
 * hold the file descriptors for the two ends of the pipe (index 0 for 
 * reading, 1 for writing).
 * As reaction to a unix signal we change the text of a label, hence the
 * label must be global.
 */
int signal_pipe[2];

/* 
 * The unix signal handler.
 * Write any unix signal into the pipe. The writing end of the pipe is in 
 * non-blocking mode. If it is full (which can only happen when the 
 * event loop stops working) signals will be dropped.
 */
void roadmap_signal_handler(int signal) {

    if(write(signal_pipe[1], &signal, sizeof(int)) != sizeof(int)) {
        roadmap_log(ROADMAP_WARNING, "unix signal %d lost", signal);
    }
}
    
/* 
 * The event loop callback that handles the unix signals. Must be a GIOFunc.
 * The source is the reading end of our pipe, cond is one of 
 *   G_IO_IN or G_IO_PRI (I don't know what could lead to G_IO_PRI)
 * the pointer d is always NULL
 */
static gboolean roadmap_deliver_signal
    (GIOChannel *source, GIOCondition cond, gpointer d) {

    /* 
     * There is no g_io_channel_read or g_io_channel_read_int, so we read
     * char's and use a union to recover the unix signal number.
     */
    union {
        gchar chars[sizeof(int)];
        int signal;
    } buf;
    GIOError status;       /* save the reading status */
    gsize bytes_read;       /* save the number of chars read */
    

    /* 
     * Read from the pipe as long as data is available. The reading end is 
     * also in non-blocking mode, so if we have consumed all unix signals, 
     * the read returns G_IO_STATUS_AGAIN. 
     */
    while((status = g_io_channel_read(source, buf.chars, 
                 sizeof(int), &bytes_read)) == G_IO_ERROR_NONE &&
                 bytes_read > 0) {

        if(bytes_read == 0) break;

        /* 
         * There might be some problem resulting in too few char's read.
         * Check it.
         */
        if(bytes_read != sizeof(int)){
            roadmap_log(ROADMAP_WARNING,
                    "lost data in signal pipe (expected %d, received %d)",
                    sizeof(int), bytes_read);
            continue;               /* discard the garbage and keep fingers crossed */
        }
        roadmap_log(ROADMAP_WARNING, "received signal %d", buf.signal);
        roadmap_main_exit();
        break;

    }
    
    if (status == G_IO_ERROR_AGAIN || status == G_IO_ERROR_NONE){
        return (TRUE);      /* keep the event source */
    }

    /* 
     * Reading from the pipe has not returned with normal status. Check for 
     * potential errors and return from the callback.
     */
    roadmap_log (ROADMAP_WARNING, "signal pipe has been closed");

    return (FALSE);      /* don't keep the event source */

}



void
roadmap_signals_init(void) {

    /* 
     * In order to register the reading end of the pipe with the event loop 
     * we must convert it into a GIOChannel.
     */
    GIOChannel *g_signal_in; 

    long fd_flags;   /* used to change the pipe into non-blocking mode */
        

    /* 
     * Set the unix signal handling up.
     * First create a pipe.
     */
    if(pipe(signal_pipe)) {
        perror("pipe");
        exit(1);
    }

    /* 
     * put the write end of the pipe into nonblocking mode,
     * need to read the flags first, otherwise we would clear other flags too.
     */
    fd_flags = fcntl(signal_pipe[1], F_GETFL);
    if(fd_flags == -1) {
            perror("read descriptor flags");
            exit(1);
    }
    if(fcntl(signal_pipe[1], F_SETFL, fd_flags | O_NONBLOCK) == -1) {
            perror("write descriptor flags");
            exit(1);
    }

    /* Install the unix signal handler roadmap_signal_handler for
     * the signals of interest */
    signal(SIGHUP, roadmap_signal_handler);
    signal(SIGINT, roadmap_signal_handler);
    signal(SIGQUIT, roadmap_signal_handler);
    signal(SIGTERM, roadmap_signal_handler);

    /* convert the reading end of the pipe into a GIOChannel */
    g_signal_in = g_io_channel_unix_new (signal_pipe[0]);

    /* register the reading end with the event loop */
    g_io_add_watch
        (g_signal_in, G_IO_IN | G_IO_PRI, roadmap_deliver_signal, NULL);
}
