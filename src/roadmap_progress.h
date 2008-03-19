/*
 * RoadMap's progress bar implementation.  GTK version originally
 * from "Yattm", by Torrey Searle
 *
 * Copyright (C) 1999, Torrey Searle <tsearle@uci.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef INCLUDED__ROADMAP_PROGRESS__H
#define INCLUDED__ROADMAP_PROGRESS__H

#ifdef __cplusplus
extern "C" {
#endif

// int progress_window_new( char * filename, unsigned long size );
int roadmap_progress_new( void );
void roadmap_progress_update(int tag, int total, int progress);
void roadmap_progress_close(int tag);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // INCLUDED__ROADMAP_PROGRESS__H
