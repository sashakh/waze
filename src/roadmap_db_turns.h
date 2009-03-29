/*
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright (c) 2009, Danny Backx.
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

/**
 * @file
 * @brief the format of the turn restrictions table used by RoadMap.
 */

#ifndef _ROADMAP_DB_TURNS_H_
#define _ROADMAP_DB_TURNS_H_

#include "roadmap_types.h"

typedef struct {
   int from_line;
   int to_line;
} RoadMapTurns;

typedef struct {
   int node;
   int first;
   int count;
} RoadMapTurnsByNode;

typedef struct {
   int first;
   int count;
} RoadMapTurnsBySquare;

/**
 * Modelling one way streets in this database would be very expensive.
 * Instead, this is in the line/data2 table, together with layer info.
 */
#endif // _ROADMAP_DB_TURNS_H_
