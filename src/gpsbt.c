/*
 * Copyright 2008 Charles Werbick
 * This file is part of Roadmap for Maemo (among others, see below ;-)
 *
 * GPS Bluetooth code from Nokia added to Roadmap for Maemo to facilitate gps management.
 * (Same  code as Maemo-Mapper uses... Why argue with success?)
 *
 *

GPS BT management API. The API is used by those applications that
wish to use services provided by gps daemon i.e., they wish to receive
GPS data from the daemon. See README file for more details.

Copyright (C) 2006 Nokia Corporation. All rights reserved.

Author: Jukka Rissanen <jukka.rissanen@nokia.com>

Redistribution and use in source and binary forms, with or without modification, are permitted   provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/* $Id:$ */

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#define _GNU_SOURCE

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

#include "gpsbt.h"

#ifdef LEGACY

/* Set following #if to 1 if you want to use automatic bt dev disconnect.
 * This does not seem to work correctly currently (disconnects too fast)
 */
#if 0
#define USE_AUTOMATIC_DISCONNECT
#else
#undef USE_AUTOMATIC_DISCONNECT
#endif

/* default control socket (not used by default) */
#define CTRL_SOCK "/tmp/.gpsd_ctrl_sock"

/* Note that 10 second timeout should not be lowered, because
 * then there might be GPS device connection errors if there
 * are multiple GPS devices that the user is using.
 * E.g., if the timeout is set to 5 secs and there are three
 * GPS devices that the user is using, it is possible that
 * if the 1st device fails, the other devices might also fail
 * if the timeout is lower than 10 secs.
 */
#if 0
#define DEFAULT_TIMEOUT (1000*10) /* 10 second timeout (in ms) */
#else
#define DEFAULT_TIMEOUT (1000*60) /* 60 second timeout (in ms) */
#endif

/* BT dbus service location */
#define BASE_PATH                "/org/bluez"
#define BASE_INTERFACE           "org.bluez"
#define ADAPTER_PATH             BASE_PATH
#define ADAPTER_INTERFACE        BASE_INTERFACE ".Adapter"
#define MANAGER_PATH             BASE_PATH
#define MANAGER_INTERFACE        BASE_INTERFACE ".Manager"
#define ERROR_INTERFACE          BASE_INTERFACE ".Error"
#define SECURITY_INTERFACE       BASE_INTERFACE ".Security"
#define RFCOMM_INTERFACE         BASE_INTERFACE ".RFCOMM"
#define BLUEZ_DBUS               BASE_INTERFACE

#define LIST_ADAPTERS            "ListAdapters"
#define LIST_BONDINGS            "ListBondings"
#define CREATE_BONDING           "CreateBonding"
#define GET_REMOTE_NAME          "GetRemoteName"
#define GET_REMOTE_SERVICE_CLASSES "GetRemoteServiceClasses"

#define BTCOND_PATH              "/com/nokia/btcond/request"
#define BTCOND_BASE              "com.nokia.btcond"
#define BTCOND_INTERFACE         BTCOND_BASE ".request"
#define BTCOND_REQUEST           BTCOND_INTERFACE
#define BTCOND_CONNECT           "rfcomm_connect"
#define BTCOND_DISCONNECT        "rfcomm_disconnect"
#define BTCOND_DBUS              BTCOND_BASE

#define GPS_SERVICE_CLASS_STR "positioning"

typedef struct {
  char *adapter;  /* do not free this, it is freed somewhere else */
  char *bonding;  /* allocated from heap, you must free this */
} bonding_t;


/* Not all gps bt devices set the positioning bit in service class to 1,
 * so we need a way to figure out whether the device supports positioning
 * or not. The array contains sub-string for devices that we know to
 * support positioning. The sub-string will be matched against device
 * remote name that we get using dbus GetRemoteName call. The name is
 * fetched only if we have no BT device in the system that has the
 * positioning bit activated. Note that the listed string is case
 * sensitive.
 */
static const char const *gps_device_names[] = {
  "HOLUX GR-",   /* like Holux GR-231 or similar */
  "Nokia LD-",   /* for Nokia LD-1W, note that LD-3W has positioning bit set */
  "GPS",         /* Insmat BT-GPS-3244C8 or TomTom Wireless GPS MkII */
  "i-Blue",      /* i-Blue line of GPS receivers. */
  0  /* must be at the end */
};


/* ----------------------------------------------------------------------- */
static int debug_level;

#ifdef DEBUG
#if (__GNUC__ > 2) && ((__GNUC__ > 3) || (__GNUC_MINOR__ > 2))
#define PDEBUG(fmt...) do {                                                \
    if (debug_level) {                                                     \
      struct timeval tv;                                                   \
      gettimeofday(&tv, 0);                                                \
      printf("DEBUG[%d]:%ld.%ld:%s:%s():%d: ",                             \
             getpid(),                                                     \
             tv.tv_sec, tv.tv_usec,                                        \
             __FILE__, __FUNCTION__, __LINE__);                            \
      printf(fmt);                                                         \
      fflush(stdout);                                                      \
    }                                                                      \
  }while(0)
#else
#define PDEBUG(fmt...) do {                                                \
    if (debug_level) {                                                     \
      struct timeval tv;                                                   \
      gettimeofday(&tv, 0);                                                \
      printf("DEBUG[%d]:%ld.%ld:%s:%s():%d: ",                             \
             getpid(),                                                     \
             tv.tv_sec, tv.tv_usec,                                        \
             __FILE__, __FUNCTION__, __LINE__);                            \
      printf(##fmt);                                                       \
      fflush(stdout);                                                      \
    }                                                                      \
  }while(0)
#endif
#else
#define PDEBUG(fmt...)
#endif


/* ----------------------------------------------------------------------- */
static int check_device_name(char *name)
{
  int i=0, st=0;

  while (gps_device_names[i]) {
    if (strstr(name, gps_device_names[i])) {
      st = 1;
      break;
    }
    i++;
  }

  return st;
}


/* ----------------------------------------------------------------------- */
static inline DBusGConnection *get_dbus_gconn(GError **error)
{
  DBusGConnection *conn;

  conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, error);
  return conn;
}


/* ----------------------------------------------------------------------- */
static int set_error_msg(char *errbuf,
                         int errbuf_max_len,
                         char *msg,
                         ...)
{
  int st, len;
  va_list args;

  if (!errbuf) {
    errno = EINVAL;
    return -1;
  }

  va_start(args, msg);
  st = vsnprintf(errbuf, errbuf_max_len, msg, args);
  va_end(args);

  /* Remove \n if it is at the end of the line (so that caller can
   * print the string as it wishes)
   */
  len = strlen(errbuf);
  if (len>0 && errbuf[len-1]=='\n')
    errbuf[len-1]='\0';

  return st;
}


/* ----------------------------------------------------------------------- */
extern int gpsbt_start(char *bda,
                       int our_debug_level,
                       int gpsd_debug_level,
                       short port,
                       char *error_buf,
                       int error_buf_max_len,
                       int timeout_ms,
                       gpsbt_t *ctx)
{
  int i, j, k, st, num_bondings = 0,
    bonding_cnt = 0, num_classes = 0, num_rfcomms = 0,
    num_posdev = 0;
  GError *error = NULL;
  DBusGConnection *bus = NULL;
  DBusGProxy *proxy = NULL;
  char **str_iter = NULL;
  char **tmp_bondings = 0, **tmp_classes = 0;
  char **adapters = 0;
  char **rfcomms = 0;
  bonding_t *bondings = 0; /* points to array of bonding_t */
  bonding_t *posdev = 0; /* bondings with positioning bit on */
  char *tmp;
  char *onoff;
  const char const *spp="SPP";
  int timeout;
  char *gpsd_prog;
  char *gpsd_ctrl_sock;


#if (__GNUC__ > 2) && ((__GNUC__ > 3) || (__GNUC_MINOR__ > 2))
#define ERRSTR(fmt, args...)                                     \
  if (error_buf && error_buf_max_len>0) {                        \
    set_error_msg(error_buf, error_buf_max_len, fmt, args);      \
  } else {                                                       \
    PDEBUG(fmt, args);                                           \
  }
#else
#define ERRSTR(fmt, args...)                                     \
  if (error_buf && error_buf_max_len>0) {                        \
    set_error_msg(error_buf, error_buf_max_len, fmt, ##args);    \
  } else {                                                       \
    PDEBUG(fmt, ##args);                                         \
  }
#endif  

  if (!ctx) {
    errno = EINVAL;
    return -1;
  }

  debug_level = our_debug_level;

  /* context & error needs to be clean */
  memset(ctx, 0, sizeof(gpsmgr_t));

  if (timeout_ms==0)
    timeout = DEFAULT_TIMEOUT;
  else if (timeout_ms < 0)
    timeout = -1;
  else
    timeout = timeout_ms;

  ctx->timeout = timeout;

  /* Caller can override the name of the gpsd program and
   * the used control socket.
   */
  gpsd_prog = getenv("GPSD_PROG");
  gpsd_ctrl_sock = getenv("GPSD_CTRL_SOCK");

  if (!gpsd_prog)
    gpsd_prog = "gpsd";

  if (!gpsd_ctrl_sock)
    gpsd_ctrl_sock = CTRL_SOCK;

  /* First find out what BT devices are paired with us, then figure out
   * which one of those are GPS devices (positioning bit in class is
   * set). If found, create a rfcommX device for GPS BT and pass that
   * as a parameter when starting gpsd.
   */

  /* But first things first, if gpsd is already running then it is
   * no point trying to find out the GPS BT devices because some program
   * has already figured it out.
   */
  st = gpsmgr_is_gpsd_running(&ctx->mgr, NULL, GPSMGR_MODE_LOCK_IF_POSSIBLE);
  if (st) {

    if (st!=2) { /* gpsd is running, value 2 would mean that gpsd is not
                  * running but we have a lock acquired
                  */

      st = gpsmgr_start(gpsd_prog, NULL, gpsd_ctrl_sock,
                        gpsd_debug_level, port, &ctx->mgr);
      if (!st) {
        /* everything is ok */
        PDEBUG("%s already running, doing nothing\n",gpsd_prog);
        goto OUT;
      }

      PDEBUG("gpsmgr_start() returned %d [%s, %d]\n",st,strerror(errno),errno);
      /* Note, no exit or return here, let the API do its magic */
    }
  }


  /* Use the dbus interface to get the BT information */

  error = NULL;
  bus = get_dbus_gconn(&error);
  if (error) {
    st = -1;
    errno = ECONNREFUSED; /* close enough :) */
    ERRSTR("%s", error->message);
    PDEBUG("Cannot get reply message [%s]\n", error->message);
    goto OUT;
  }

#define CHECK_ERROR(s,o,i,m,t)                                             \
  if (st<0) {                                                              \
    ERRSTR("Cannot send msg (service=%s, object=%s, interface=%s, "        \
           "method=%s) [%s]\n", s, o, i, m,                                \
           error.message ? error.message : "<no error msg>");              \
    goto OUT;                                                              \
  }


  /* We need BT information only if the caller does not specify
   * the BT address. If address is defined, it is assumed that
   * it is already bonded and we just create RFCOMM connection
   * to it.
   */
  if (!bda) {
    proxy = dbus_g_proxy_new_for_name(bus,
        BLUEZ_DBUS, MANAGER_PATH, MANAGER_INTERFACE);

    error = NULL;
    if(!dbus_g_proxy_call(proxy, LIST_ADAPTERS, &error, G_TYPE_INVALID,
          G_TYPE_STRV, &adapters, G_TYPE_INVALID)
        || error || !adapters || !*adapters || !**adapters) {
      PDEBUG("No adapters found.\n");
      st = -1;
      goto OUT;
    }
    g_object_unref(proxy);

    /* For each adapter, get bondings */
    i = 0;
    while (adapters[i]) {
      proxy = dbus_g_proxy_new_for_name(bus,
          BLUEZ_DBUS, adapters[i], ADAPTER_INTERFACE);

      error = NULL;
      if(!dbus_g_proxy_call(proxy, LIST_BONDINGS, &error, G_TYPE_INVALID,
            G_TYPE_STRV, &tmp_bondings, G_TYPE_INVALID)
          || error || !tmp_bondings || !*tmp_bondings || !**tmp_bondings) {
        PDEBUG("Call to LIST_BONDINGS failed (error=%s, tmp_bondings[0]=%s\n",
            error ? error->message : "<null>",
            tmp_bondings ? tmp_bondings[0] : "<null>");
      } else {
        /* Count the bondings. */
        for(num_bondings = 0, str_iter = tmp_bondings; *str_iter;
            str_iter++, num_bondings++);

        /* Allocate bondings array, note that we DO allocate one extra array
         * element for marking end of array.
         */
        bondings = (bonding_t *)realloc(bondings,
                (bonding_cnt+num_bondings+1)*sizeof(bonding_t));
        if (!bondings) {
          st = -1;
          errno = ENOMEM;
          goto OUT;
        }
        bondings[bonding_cnt+num_bondings].bonding=
          bondings[bonding_cnt+num_bondings].adapter=NULL; /* just in case */


        j = 0;
        while (j<num_bondings && tmp_bondings[j]) {
          bondings[bonding_cnt].bonding = strdup(tmp_bondings[j]);
          free(tmp_bondings[j]);

          bondings[bonding_cnt].adapter = adapters[i]; /* no allocation! */
          
          PDEBUG("Bondings[%d]=%s (adapter=%s)\n", bonding_cnt,
                 bondings[bonding_cnt].bonding, bondings[bonding_cnt].adapter);

          /* Get all remote service classes for this bonding. */
          error = NULL;
          if(dbus_g_proxy_call(proxy, GET_REMOTE_SERVICE_CLASSES, &error,
                G_TYPE_STRING, bondings[bonding_cnt].bonding, G_TYPE_INVALID,
                G_TYPE_STRV, &tmp_classes, G_TYPE_INVALID)
              && !error && tmp_classes && *tmp_classes && **tmp_classes) {

            k = 0;
            while (tmp_classes[k]) {
              if (!strcasecmp(tmp_classes[k], GPS_SERVICE_CLASS_STR)) {
                /* match found, this device claims to be able to provide
                 * positioning data
                 */
                posdev = (bonding_t *)realloc(posdev,
                    (num_posdev+1)*sizeof(bonding_t));
                if (!posdev) {
                  st = -1;
                  errno = ENOMEM;
                  goto OUT;
                }
              
                posdev[num_posdev].bonding=strdup(
                    bondings[bonding_cnt].bonding);
                posdev[num_posdev].adapter=bondings[bonding_cnt].adapter;
                num_posdev++;
                onoff = "ON";
              } else {
                onoff = "OFF";
              }
              PDEBUG("Addr=%s, Class[%d]=%s (adapter=%s), "
                      "positioning bit %s\n",
                     bondings[bonding_cnt].bonding, k, tmp_classes[k],
                     bondings[bonding_cnt].adapter, onoff);
              free(tmp_classes[k]);
              
              k++;
            }

            free(tmp_classes);
            tmp_classes=0;
          }

          num_classes = 0;
          bonding_cnt++;
          j++;
        }
      }

      free(tmp_bondings);
      tmp_bondings=0;
      g_object_unref(proxy);
      i++;
    }

#if 0
#ifdef DEBUG
    if (debug_level) {
      int i=0;
      if (bonding_cnt) {
        PDEBUG("Bondings [%d]:\n", bonding_cnt);
        while (bondings[i].bonding && i<bonding_cnt) {
          PDEBUG("\t%s\n", bondings[i].bonding);
          i++;
        }
      } else {
        PDEBUG("No bondings exists.\n");
      }
    }
#endif
#endif

    /* We have bonded devices but none has positioning bit set. Try
     * to find out if any of the devices is known to be a BT device.
     */
    if (bonding_cnt>0 && num_posdev == 0) {

      /* For each bonded device, get its name */
      i = 0;
      while (i<bonding_cnt && bondings[i].bonding) {
        proxy = dbus_g_proxy_new_for_name(bus,
            BLUEZ_DBUS, bondings[i].adapter, ADAPTER_INTERFACE);

        tmp = NULL;
        error = NULL;
        if(dbus_g_proxy_call(proxy, GET_REMOTE_NAME, &error,
              G_TYPE_STRING, bondings[i].bonding, G_TYPE_INVALID,
              G_TYPE_STRING, &tmp, G_TYPE_INVALID)
            && !error && tmp && *tmp) {
          PDEBUG("Checking device name: %s\n", tmp);
          if (check_device_name(tmp)) {

            /* Found a GPS device */
            posdev = (bonding_t *)realloc(posdev,
                    (num_posdev+1)*sizeof(bonding_t));
            if (!posdev) {
              st = -1;
              errno = ENOMEM;
              goto OUT;
            }
        
            posdev[num_posdev].bonding=strdup(bondings[i].bonding);
            posdev[num_posdev].adapter=bondings[i].adapter;
            num_posdev++;

            PDEBUG("Addr=%s, (adapter=%s), Name=\"%s\"\n",
                   bondings[i].bonding, bondings[i].adapter, tmp);
          }
        }
        g_object_unref(proxy);
        i++;
      }
    }


  } else {  /* if (!bda) */

    /* Caller supplied BT address so use it */
    num_posdev = 1;
    posdev = calloc(1, sizeof(bonding_t *));
    if (!posdev) {
      st = -1;
      errno = ENOMEM;
      goto OUT;
    }
    posdev[0].bonding = strdup(bda);

    /* Adapter information is not needed */
    posdev[0].adapter = "<not avail>";
  }


  /* For each bondend BT GPS device, try to create rfcomm */
  for (i=0; i<num_posdev; i++) {
    /* Note that bluez does not provide this interface (its defined but not
     * yet implemented) so we use btcond for creating rfcomm device(s)
     */
    proxy = dbus_g_proxy_new_for_name(bus,
        BTCOND_DBUS, BTCOND_PATH, BTCOND_INTERFACE);

    error = NULL;
    tmp = NULL;
    if(!dbus_g_proxy_call(proxy, BTCOND_CONNECT, &error,
          G_TYPE_STRING, posdev[i].bonding,
          G_TYPE_STRING, spp,
#ifdef USE_AUTOMATIC_DISCONNECT
                       G_TYPE_BOOLEAN, TRUE,
#else
                       G_TYPE_BOOLEAN, FALSE,
#endif
          G_TYPE_INVALID,
          G_TYPE_STRING, &tmp,
          G_TYPE_INVALID)
        || error || !tmp || !*tmp) {
      PDEBUG("dbus_g_proxy_call returned an error: (error=(%d,%s), tmp=%s\n",
          error ? error->code : -1,
          error ? error->message : "<null>",
          tmp ? tmp : "<null>");

      /* No error if already connected */
      if (error && !strstr(error->message,
            "com.nokia.btcond.error.connected")) {

      ERROR:
        ERRSTR("Cannot send msg (service=%s, object=%s, interface=%s, "
               "method=%s) [%s]\n",
               BTCOND_DBUS,
               BTCOND_PATH,
               BTCOND_INTERFACE,
               BTCOND_CONNECT,
               error->message ? error->message : "<no error msg>");
        continue;

      } else if(!tmp || !*tmp) {

        /* hack: rfcommX device name is at the end of error message */
        char *last_space = strstr(error->message, " rfcomm");
        if (!last_space) {
          goto ERROR;
        }

        g_free(tmp);
        tmp = g_strdup_printf("/dev/%s", last_space+1);
      }
    }
    g_object_unref(proxy);

    if (tmp && tmp[0]) {
      rfcomms = (char **)realloc(rfcomms, (num_rfcomms+1)*sizeof(char *));
      if (!rfcomms) {
        st = -1;
        errno = ENOMEM;
        goto OUT;
      }
      rfcomms[num_rfcomms] = tmp;
      num_rfcomms++;

      PDEBUG("BT addr=%s, RFCOMM %s now exists (adapter=%s)\n",
              posdev[i].bonding, tmp, posdev[i].adapter);

      tmp = NULL;
    }
    else {
      g_free(tmp);
    }
  }

  if (num_rfcomms==0) {
    /* serial device creation failed */
    ERRSTR("No rfcomm %s\n", "created");
    st = -1;
    errno = EINVAL;

  } else {

    /* Add null at the end */
    rfcomms = (char **)realloc(rfcomms, (num_rfcomms+1)*sizeof(char *));
    if (!rfcomms) {
      st = -1;
      errno = ENOMEM;

    } else {

      rfcomms[num_rfcomms] = NULL;

#ifndef USE_AUTOMATIC_DISCONNECT
      ctx->rfcomms = rfcomms; /* freed in gpsbt_stop() */
#endif

      /* Just start the beast (to be done if everything is ok) */
      st = gpsmgr_start(gpsd_prog, rfcomms, gpsd_ctrl_sock,
              gpsd_debug_level, port, &ctx->mgr);
      if (!st) {
        /* everything is ok */
        goto OUT;
      }
    }
  }

 OUT:
  if (adapters) {
    g_strfreev(adapters);
  }

  if (posdev) {
    for (i=0; i<num_posdev; i++) {
      if (posdev[i].bonding) {
        free(posdev[i].bonding);
        memset(&posdev[i], 0, sizeof(bonding_t)); /* just in case */
      }
    }
    free(posdev);
    posdev = 0;
  }

  if (bondings) {
    for (i=0; i<num_bondings; i++) {
      if (bondings[i].bonding) {
        free(bondings[i].bonding);
        memset(&bondings[i], 0, sizeof(bonding_t)); /* just in case */
      }
    }
    free(bondings);
    bondings = 0;
  }

#ifdef USE_AUTOMATIC_DISCONNECT
  if (rfcomms) {
    for (i=0; i<num_rfcomms; i++) {
      if (rfcomms[i]) {
        free(rfcomms[i]);
        rfcomms[i]=0;
      }
    }
    free(rfcomms);
    rfcomms = 0;
  }
#endif

  if (bus) {
    dbus_g_connection_unref(bus);
  }

  return st;
}


/* ----------------------------------------------------------------------- */
extern int gpsbt_stop(gpsbt_t *ctx)
{
  int st;

  if (!ctx) {
    errno = EINVAL;
    return -1;
  }

  st = gpsmgr_stop(&ctx->mgr);


#ifndef USE_AUTOMATIC_DISCONNECT
  /* We need to disconnect from rfcomm device */
  if (ctx->rfcomms) {
    int i = 0;
    int skip_dbus = 0;
    DBusGConnection *bus = NULL;
    DBusGProxy *proxy = NULL;
    GError *error = NULL;

    bus = get_dbus_gconn(&error);
    if (!bus) {
      errno = ECONNREFUSED; /* close enough :) */
      PDEBUG("Cannot get reply message [%s]\n", error->message);
      skip_dbus = 1;
    }

    if (!skip_dbus) {
      /* Make sure there is no other user for gpsd, if there is then
       * we must not delete rfcomm devices. The st==0 would mean that
       * we are the only one using the dev.
       */
      if (st>0) {
        skip_dbus = 1;
        PDEBUG("Skipping rfcomm device deletion as we are "
                "not the only location user\n");
      }
    }

    while (ctx->rfcomms[i]) {

      if (!skip_dbus) {
        /* Disconnect the device */
        proxy = dbus_g_proxy_new_for_name(bus,
            BTCOND_DBUS, BTCOND_PATH, BTCOND_INTERFACE);
        error = NULL;
        if(!dbus_g_proxy_call(proxy, BTCOND_DISCONNECT, &error,
              G_TYPE_STRING, ctx->rfcomms[i], G_TYPE_INVALID, G_TYPE_INVALID)
            || error){
          PDEBUG("Cannot send msg (service=%s, object=%s, interface=%s, "
           "method=%s) [%s]\n",
                 BTCOND_DBUS,
                 BTCOND_PATH,
                 BTCOND_INTERFACE,
                 BTCOND_DISCONNECT,
                 error->message ? error->message : "<no error msg>");
        }
        g_object_unref(proxy);
      }

      free(ctx->rfcomms[i]);
      ctx->rfcomms[i]=0;
      i++;
    }

    if (bus) {
      dbus_g_connection_unref(bus);
    }

    free(ctx->rfcomms);
    ctx->rfcomms = 0;
  }
#endif /* automatic disconnect */

  return st;
}

#endif
