/*
GPS daemon manager API. The API is used by those applications that
wish to use services provided by gps daemon i.e., they wish to receive
GPS data from the daemon. See README file for more details.

Copyright (C) 2006 Nokia Corporation. All rights reserved.

Author: Jukka Rissanen <jukka.rissanen@nokia.com>

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/* $Id:$ */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#ifndef gpsmgr_included
#define gpsmgr_included

/* gpsd pid is stored to this file, also tells if the gpsd
 * is running or not (if not locked, then gpsd is not running)
 */
#define GPSMGR_GPSD_LOCK "/tmp/.gpsmgr_gpsd"

/* Lock file that manages reference count of the clients */
#define GPSMGR_LOCK      "/tmp/.gpsmgr_lock"

/* Lock file that controls access to GPSMGR_LOCK file */
#define GPSMGR_LOCK2     "/tmp/.gpsmgr_lock2"

/* Value of mode parameter in gpsmgr_is_gpsd_running() */
#define GPSMGR_MODE_JUST_CHECK 1
#define GPSMGR_MODE_LOCK_IF_POSSIBLE (!GPSMGR_MODE_JUST_CHECK)


#define GPSMGR_MAX_APPLICATIONS 16

/* Internal context information */
typedef struct {
  pid_t pid;           /* gpsd process pid (if we started it) */
  pid_t mgr_pid;       /* manager pid */
  char *file;          /* name of the locking file */
  int lock_file_desc;  /* only one lock file / application */
  int locking_slot;    /* what is our position in locking file, this
			* is used as a reference count so that we 
			* know when there is no more clients needing
			* gpsd service
			*/

  int already_locked_by_us;
                        /* we already have the lock but gpsd is not yet
			 * running (used together with gpsmgr_is_gpsd_running()
			 * to atomically start gpsd)
			 */
} gpsmgr_t;


/* Start function starts the gps daemon if it is not running.
 *
 * Parameters:
 *    path : name of the program (gpsd) to start (if the path does not start
 *           with / then PATH is checked when starting the program)
 *    gps_device : list of pointers to device name of the GPS device (like
 *                 /dev/rfcomm0), so there can be multiple GPS devices
 *                 configured in the system. The list is null terminated
 *                 list of null terminated strings.
 *    ctrl_sock : control socket location
 *    debug_level : gpsd debug level (set to 0 to use the default)
 *    port : gpsd port (set to 0 to use the default (2947))
 *    ctx : caller must allocate this
 *
 * Returns:
 *    <0 : error, check errno for more details
 *     0 : ok
 *
 */
extern int gpsmgr_start(char *path, char **gps_devices, char *ctrl_sock, 
			int debug_level, short port, gpsmgr_t *ctx);


/* Stop function stops the gps daemon if this is the only user of the gpsd
 *
 * Parameters:
 *    ctx : context returned by gpsmgr_start()
 *
 * Returns:
 *    <0 : error, check errno for more details
 *     0 : ok, we were the only one using gpsd
 *     1 : ok, there are other users of gpsd that have a master lock
 *     2 : ok, there are other users of gpsd but we have a master lock
 *
 */
extern int gpsmgr_stop(gpsmgr_t *ctx);


/* Check if the gpsd is running and optionally return number of clients
 * using this library. The function is optional and client application
 * does not have to use it.
 *
 * Parameter:
 *    ctx : context returned by gpsmgr_start(), optional. If the param
 *          is not given, then num_of_clients cannot be returned.
 *    num_of_clients: if user supplies this then the API will place
 *                    number of library users to it (only when mode is
 *                    set to GPSMGR_MODE_JUST_CHECK)
 *    mode : if set to GPSMGR_MODE_JUST_CHECK, then the library just checks
 *           if the gpsd is running. The value GPSMGR_MODE_LOCK_IF_POSSIBLE
 *           means that the library locks the gpsd lockfile (which means
 *           that gpsd is running) if the gpsd is not yet running. In this case
 *           the caller should call gpsmgr_start() to really start the gpsd.
 *
 * Returns:
 *    0 : not running or an error happened
 *    1 : running
 *    2 : was not running, but is now ready to be run
 *
 */
extern int gpsmgr_is_gpsd_running(gpsmgr_t *ctx, int *num_of_clients, int mode);


/* Set the debug mode. Debug mode can be activated if the library
 * is compiled with DEBUG defined.
 *
 * Parameters:
 *    mode : new mode (0=no debugging, >0 activate debugging)
 *
 * Returns:
 *    -1 : the program was not compiled with debugging mode
 *  other: previous debugging mode value
 *
 */
extern int gpsmgr_set_debug_mode(int mode);


#endif /* gpsmgr_included */
