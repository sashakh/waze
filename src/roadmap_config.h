/* roadmap_config.h - A module to handle all RoadMap configuration issues.
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
 */

#ifndef _ROADMAP_CONFIG__H_
#define _ROADMAP_CONFIG__H_

#include <stdio.h>

#include "roadmap_types.h"


#define ROADMAP_CONFIG_STRING   0
#define ROADMAP_CONFIG_ENUM     1
#define ROADMAP_CONFIG_COLOR    2


void roadmap_config_declare
        (const char *config,
         const char *category,
         const char *name,
         const char *default_value);

void roadmap_config_declare_enumeration
        (const char *config,
         const char *category,
         const char *name,
         const char *enumeration_value, ...);

void roadmap_config_declare_color
        (const char *config,
         const char *category,
         const char *name,
         const char *default_value);


void *roadmap_config_first (char *config);
int   roadmap_config_get_type (void *cursor);
void *roadmap_config_scan
         (void *cursor, char **category, char **name, char **value);

void *roadmp_config_get_enumeration (void *cursor);
char *roadmp_config_get_enumeration_value (void *enumeration);
void *roadmp_config_get_enumeration_next (void *enumeration);

void  roadmap_config_initialize (void);
void  roadmap_config_save       (int force);

char *roadmap_config_get (char *category, char *name);
void  roadmap_config_set (char *category, char *name, char *value);

int   roadmap_config_get_integer (char *category, char *name);

void  roadmap_config_get_position (char *name, RoadMapPosition *position);
void  roadmap_config_set_position (char *name, RoadMapPosition *position);

#endif // _ROADMAP_CONFIG__H_
