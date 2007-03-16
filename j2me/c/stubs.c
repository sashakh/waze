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
#include "roadmap_path.h"
#include "roadmap_net.h"
#include "roadmap_serial.h"
#include "roadmap_spawn.h"
#include "roadmap_file.h"

int roadmap_net_receive (RoadMapSocket s, void *data, int size) { return -1; }
int roadmap_net_send    (RoadMapSocket s, const void *data, int length,
                         int wait) { return -1; }

void roadmap_net_close  (RoadMapSocket s) {}

int  roadmap_spawn_write_pipe (RoadMapPipe pipe, const void *data, int length) { return -1; }
int  roadmap_spawn_read_pipe  (RoadMapPipe pipe, void *data, int size) { return -1; }
void roadmap_spawn_close_pipe (RoadMapPipe pipe) {}

void roadmap_messagebox (const char *title, const char *text) { 
  NOPH_Alert_t msg = NOPH_Alert_new(title, text, 0, NOPH_AlertType_get(NOPH_AlertType_INFO));
  NOPH_Alert_setTimeout(msg, NOPH_Alert_FOREVER);
  NOPH_Display_setCurrent(NOPH_Display_getDisplay(NOPH_MIDlet_get()), msg);
}

void *roadmap_res_get (unsigned int type, unsigned int flags,
                       const char *name) { return NULL; }

int roadmap_preferences_use_keyboard() { return 0; }

