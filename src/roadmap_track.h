/* roadmap_track.h - Keep track of where we've been.
 *
 * LICENSE:
 *
 *   Copyright 2005 Paul G. Fox
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

#ifndef INCLUDE__ROADMAP_TRACK__H
#define INCLUDE__ROADMAP_TRACK__H


void  roadmap_track_display (void);
void  roadmap_track_reset (void);
void  roadmap_track_initialize (void);
void  roadmap_track_activate (void);
void  roadmap_track_save (void);
void  roadmap_track_autowrite (void);
void  roadmap_track_autoload(void);
void  roadmap_track_toggle_display(void);
int roadmap_track_is_refresh_needed (void);

#endif // INCLUDE__ROADMAP_TRACK__H

