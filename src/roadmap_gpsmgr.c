/* roadmap_gpsmgr.c - Roadmap for Maemo specific gpsd startup function
 *  (c) Copyright 2008 Charles Werbick
 *  
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  
 */
#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>

#include "roadmap.h"
#include "roadmap_config.h"

#define LEGACY 1 
#include "gpsbt.h"

static RoadMapConfigDescriptor RoadMapConfigGPSSource =
                        ROADMAP_CONFIG_ITEM("GPS", "Source");

static RoadMapConfigDescriptor RoadMapConfigBluetoothMAC =
			ROADMAP_CONFIG_ITEM("Bluetooth", "Bluetooth Address");

static RoadMapConfigDescriptor RoadMapConfigBluetoothEnabled =
			ROADMAP_CONFIG_ITEM("Bluetooth", "Bluetooth Enabled");

static RoadMapConfigDescriptor RoadMapConfigGPSEnabled =
                        ROADMAP_CONFIG_ITEM("GPS", "Enabled");

gint GpsEnabled; 
static gboolean we_own_gpsd = FALSE;

static GThread *_gpsd_thread = NULL;
static GMutex *_gpsd_init_mutex = NULL;

static gpsbt_t *gpsd_context;
/* Right now this is real basic. we use gpsbt because roadmap uses gpsd and so that
 * we can add bluetooth gps support for n800 userslater. After the dialogs are cleaned up,
 * I'll add bluetooth functions...
*/

void roadmap_gpsmgr_initialize()
{
  if (!g_thread_supported ()) g_thread_init (NULL);

  _gpsd_init_mutex = g_mutex_new();
  gpsd_context = g_new(gpsbt_t, sizeof(gpsbt_t));
/*
  roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGPSEnabled, "yes", "no", NULL);
  roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigBluetoothEnabled, "yes", "no", NULL);
  roadmap_config_declare("preferences", &RoadMapConfigBluetoothMAC, "");
  if(roadmap_config_match(&RoadMapConfigGPSEnabled, "yes")){
*/
      GpsEnabled = 1;
//  }

}

void roadmap_gpsmgr_start_gpsd()
{
  gchar errstr[2048] = "";
  const gchar *bt_address = roadmap_config_get(&RoadMapConfigBluetoothMAC);
     if(gpsbt_start((gchar*)bt_address, 0, 0, 0, errstr, sizeof(errstr),
                  0, gpsd_context) < 0)
     {
         g_printerr("Error connecting to GPS receiver: (%d) %s (%s)\n",
                 errno, strerror(errno), errstr);
 
     }


}

void roadmap_gpsmgr_start()
{
   g_mutex_lock(_gpsd_init_mutex);
   if (_gpsd_thread) g_thread_join(_gpsd_thread);
   _gpsd_thread = g_thread_create((GThreadFunc)roadmap_gpsmgr_start_gpsd, NULL, TRUE, NULL);
   g_mutex_unlock(_gpsd_init_mutex);

}
/* gpsbt.c already wraps libgpsmgr for us. If another program is using gps, we're fine. But we'll check anyway...
*/

void roadmap_gpsmgr_release()
{
   if(_gpsd_thread){
      g_mutex_lock(_gpsd_init_mutex);
      g_thread_join(_gpsd_thread);
      g_mutex_unlock(_gpsd_init_mutex);
   }

   gpsbt_stop(gpsd_context);
   _gpsd_thread = NULL;
   GpsEnabled = 0;
}

void roadmap_gpsmgr_shutdown()
{
   roadmap_gpsmgr_release();
   g_free (_gpsd_init_mutex);
   g_free (gpsd_context);
   gpsd_context = NULL;
}
