/* colors.h - colors table, used to translate names into RGB values
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright 2008 Danny Backx - separate table from declaration.
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
 */

#ifndef _COLORS_H_
#define _COLORS_H_

struct ColorEntry {
	char *name;
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

extern const struct ColorEntry color_table[];
extern const int roadmap_ncolors;

#endif /* _COLORS_H_ */
