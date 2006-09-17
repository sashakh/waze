/* roadmap_start.h - The interface for the RoadMap main module.
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

#ifndef INCLUDE__ROADMAP_START__H
#define INCLUDE__ROADMAP_START__H

#include "roadmap_factory.h"

/* The two following functions are used to freeze all RoadMap function
 * in cases when the context does not allow for RoadMap to function in
 * a normal fashion. The single example is when downloading maps:
 * RoadMap should not try access the maps, as there is at least one
 * map file that is incomplete.
 * There ought to be a better way, such using a temporary file name...
 */
void roadmap_start_freeze   (void);
void roadmap_start_unfreeze (void);
int roadmap_start_is_frozen (void);

void roadmap_start      (int argc, char **argv);
void roadmap_start_exit (void);

const char *roadmap_start_get_title (const char *name);
void roadmap_start_add_long_click_item (const char *name,
                                        const char *description,
                                        RoadMapCallback callback);

const RoadMapAction *roadmap_start_find_action (const char *name);
void roadmap_start_set_title (const char *title);

#endif /* INCLUDE__ROADMAP_START__H */

