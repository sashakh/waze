/* roadmap_log.c - a module for managing uniform error & info messages.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 * SYNOPSYS:
 *
 *   #include "roadmap.h"
 *
 *   void roadmap_log (int level, char *format, ...);
 *
 * This module is used to control and manage the appearance of messages
 * printed by the roadmap program. The goals are (1) to produce a uniform
 * look, (2) have a central point of control for error management and
 * (3) have a centralized control for routing messages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "javax/microedition/lcdui.h"

#include "roadmap.h"
#include "roadmap_gps.h"
#include "roadmap_download.h"
#include "roadmap_path.h"
#include "roadmap_pointer.h"
#include "roadmap_serial.h"
#include "roadmap_spawn.h"
#include "roadmap_file.h"

int  roadmap_spawn_write_pipe (RoadMapPipe pipe, const void *data, int length) { return -1; }
int  roadmap_spawn_read_pipe  (RoadMapPipe pipe, void *data, int size) { return -1; }
void roadmap_spawn_close_pipe (RoadMapPipe pipe) {}

#include "roadmap_res.h"
#include "roadmap_canvas.h"

void *roadmap_res_get (unsigned int type, unsigned int flags,
                       const char *name) {

   char full_name[255];

   if (type != RES_BITMAP) return NULL;

   snprintf(full_name, sizeof(full_name), "/%s.png", name);

   return roadmap_canvas_load_image (NULL, full_name);
}

int roadmap_preferences_use_keyboard() { return 0; }

void roadmap_voice_initialize (void) {}
void roadmap_voice_announce (const char *title) {}
void roadmap_voice_mute   (void) {}
void roadmap_voice_enable (void) {}

time_t roadmap_sunset (const RoadMapGpsPosition *position) { return 0; }

time_t roadmap_sunrise (const RoadMapGpsPosition *position) { return 0; }

void roadmap_pointer_initialize (void) {}
void roadmap_pointer_register_short_click (RoadMapPointerHandler handler,
                                           int priority) { }


void roadmap_pointer_register_long_click (RoadMapPointerHandler handler,
                                          int priority) { }


void roadmap_pointer_register_pressed (RoadMapPointerHandler handler,
                                       int priority) { }


void roadmap_pointer_register_released (RoadMapPointerHandler handler,
                                        int priority) { }


void roadmap_pointer_register_drag_start (RoadMapPointerHandler handler,
                                          int priority) { }


void roadmap_pointer_register_drag_motion (RoadMapPointerHandler handler,
                                           int priority) { }


void roadmap_pointer_register_drag_end (RoadMapPointerHandler handler,
                                        int priority) { }


void roadmap_pointer_unregister_short_click (RoadMapPointerHandler handler) { }


void roadmap_pointer_unregister_long_click (RoadMapPointerHandler handler) { } 

void roadmap_pointer_unregister_pressed (RoadMapPointerHandler handler) { } 

void roadmap_pointer_unregister_released (RoadMapPointerHandler handler) { }


void roadmap_pointer_unregister_drag_start (RoadMapPointerHandler handler) { }


void roadmap_pointer_unregister_drag_motion (RoadMapPointerHandler handler) { }


void roadmap_pointer_unregister_drag_end (RoadMapPointerHandler handler) { } 

const RoadMapGuiPoint *roadmap_pointer_position (void) { return NULL; }

void roadmap_help_initialize (void) {}
int roadmap_help_first_topic (const char **label,
                              RoadMapCallback *callback) {

   return 0;
}

int roadmap_help_next_topic (const char **label,
                             RoadMapCallback *callback) {

   return 0;
}

void export_sync (void) {}
void editor_upload_select (void) {}

int editor_download_update_map (RoadMapDownloadCallbacks *callbacks) { return 0; }

void editor_export_reset_dirty (void) {}
void editor_export_gpx (void) {}


int roadmap_trip_load (const char *name, int silent) { return 0; }
void roadmap_trip_save (const char *name) {}
void roadmap_trip_save_screenshot (void) {}

void roadmap_download_show_space (void) {}
void roadmap_download_delete (void) {}

void roadmap_download_subscribe_protocol  (const char *prefix,
                                           RoadMapDownloadProtocol handler) {}
void roadmap_download_subscribe_when_done (RoadMapDownloadEvent handler) {}
int  roadmap_download_enabled (void) { return 0; }
void roadmap_download_initialize (void) {}
void roadmap_download_block       (int fips) {}
void roadmap_download_unblock     (int fips) {}
void roadmap_download_unblock_all (void) {}
int  roadmap_download_blocked     (int fips) { return 0; }

void roadmap_copy_init (RoadMapDownloadSubscribe subscribe) {}
void roadmap_httpcopy_init (RoadMapDownloadSubscribe subscribe) {}

void roadmap_spawn_check (void) {}
void roadmap_spawn_initialize (const char *argv0) {}
int  roadmap_spawn (const char *name, const char *command_line) { return 0; }

void roadmap_driver_initialize (void) {}
void roadmap_driver_shutdown (void) {}
void roadmap_driver_activate (void) {}

void roadmap_driver_register_link_control
        (roadmap_gps_link_control add, roadmap_gps_link_control remove) {}

void roadmap_driver_register_server_control
        (roadmap_gps_link_control add, roadmap_gps_link_control remove) {}

void roadmap_driver_input (RoadMapIO *io) {}
void roadmap_driver_accept (RoadMapIO *io) {}

void roadmap_res_shutdown (void) {}

void roadmap_preferences_edit (void) {}

void roadmap_crossing_dialog (void) {}
void roadmap_coord_dialog (void) {}
void update_range_dialog(void) {}
void edit_markers_dialog (void) {}
void editor_notes_add_quick (void) { }
void editor_notes_add_edit (void) { }
void editor_notes_add_voice (void) { }




const char *editor_main_get_version (void) { return "0.10.0 rc4"; }

