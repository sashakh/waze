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

void roadmap_linefont_extents
        (const char *text, int size,
         int *width, int *ascent, int *descent, int *can_tilt);

void roadmap_linefont_text
        ( RoadMapGuiPoint *center, int where, int size, const char *text);

void roadmap_linefont_text_angle
        ( RoadMapGuiPoint *center,
                int size, int theta, const char *text);
