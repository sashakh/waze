/*
 * Copyright 2008 Charles Werbick
 * This file is part of Roadmap for Maemo (among others, see below ;-)
 *
 * GPS Bluetooth code from Nokia added to Roadmap for Maemo to facilitate gps management.
 * (Same  code as Maemo-Mapper uses... Why argue with success?)
 *
 *
 */

#ifndef LEGACY
#  include "/usr/include/gpsbt.h"
#else

/*
GPS BT management API. The API is used by those applications that
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

#include <gpsmgr.h>

#ifndef gpsbt_included
#define gpsbt_included

/* Internal context information */
typedef struct {
  gpsmgr_t mgr;
  char **rfcomms; /* what devices where found (null terminated array),
                   * not used if compiled with USE_AUTOMATIC_DISCONNECT
                   * (see gpsbt.c for details)
                   */
  int timeout; /* timeout for dbus messages */
} gpsbt_t;


/* Start function finds out the available GPS BT devices and starts
 * the gpsd if it is not running. If no GPS BT devices are found, then
 * an error is returned.
 *
 * Parameters:
 *    bda 
 *       BT address of the GPS device, normally this is left
 *       to null but if this API cannot find a suitable GPS
 *       BT device, the application can ask the BT device
 *       from the user and supply the address here.
 *
 *    debug_level
 *       debug level (set to 0 to use the default)
 *
 *    gpsd_debug_level
 *       gpsd debug level (set to 0 to use the default)
 *
 *    port
 *       gpsd port (set to 0 to use the default (2947))
 *
 *    error_buf
 *       user supplied error buffer (optional), if there is an error
 *       the API puts error message to this buffer
 *
 *    error_buf_max_len
 *       max length of the error buffer, set to 0 if error
 *       message is not needed in caller program
 *
 *    timeout_ms
 *       timeout (in ms) when waiting dbus replies, set to 0 to use
 *       the library default (5 seconds), if set to <0 then use
 *       the dbus default (whatever that is).
 *
 *    ctx
 *       caller must allocate and clear this struct before call
 *
 * Returns:
 *    <0 : error, check errno for more details
 *     0 : ok
 *
 * Example:
 *    The default call would be
 *         gpsbt_start(NULL, 0, 0, 0, &ctx);
 *
 * If the GPSD_PROG environment variable is defined, then it is used
 * when starting the program, otherwise "gpsd" will be used as a program
 * name. This way you can override the program name and/or path if necessary.
 *
 * If the GPSD_CTRL_SOCK environment variable is defined, then it is used
 * as a location to gpsd control socket, otherwise the default control socket
 * is /tmp/.gpsd_ctrl_sock
 */

extern int gpsbt_start(char *bda,
                       int debug_level,
                       int gpsd_debug_level,
                       short port,
                       char *error_buf,
                       int error_buf_max_len,
                       int timeout_ms,
                       gpsbt_t *ctx);


/* Stop function stops the gpsd if it was running and nobody
 * was using it.
 *
 * Parameters:
 *    ctx : context returned by gpsbt_start()
 *
 * Returns:
 *    <0 : error, check errno for more details
 *     0 : ok
 *
 */

extern int gpsbt_stop(gpsbt_t *ctx);

#endif /* gpsbt_included */

#endif
