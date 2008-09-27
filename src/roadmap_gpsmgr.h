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
 *  Right now this is real basic. After the dialogs are cleaned up I'll add bluetooth functions...
 */
#ifndef ROADMAP_GPSMGR_H
#define ROADMAP_GPSMGR_H

extern int GpsEnabled;

void roadmap_gpsmgr_initialize();
void roadmap_gpsmgr_start();
void roadmap_gpsmgr_release();

#endif /* ifndef ROADMAP_GPSMGR_H */
