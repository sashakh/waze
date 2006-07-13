/* roadmap_linefont.h - draw text on a map.
 *
 * LICENSE:
 *
 *   Copyright 2006 Paul G. Fox
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

#ifndef ROADMAP_NO_LINEFONT

#define ROADMAP_USE_LINEFONT 1

void roadmap_linefont_extents
        (const char *text, int size, int *width, int *ascent, int *descent);

void roadmap_linefont_text
	(const char *text, int allign, RoadMapGuiPoint *center,
		int size, int theta);

#define ROADMAP_LINEFONT_LEFT     0
#define ROADMAP_LINEFONT_RIGHT    1
#define ROADMAP_LINEFONT_CENTER_X 2
#define ROADMAP_LINEFONT_UPPER    0
#define ROADMAP_LINEFONT_LOWER    4
#define ROADMAP_LINEFONT_CENTER_Y 8

#define ROADMAP_LINEFONT_CENTERED \
	(ROADMAP_LINEFONT_CENTER_X | ROADMAP_LINEFONT_CENTER_Y)
#define ROADMAP_LINEFONT_CENTERED_ABOVE \
	(ROADMAP_LINEFONT_CENTER_X | ROADMAP_LINEFONT_LOWER)

#endif
