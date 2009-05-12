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
 * @brief buildmap_turn_restrictions.c - Build turn restrictions table & index for RoadMap.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_db_turns.h"

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_turn_restrictions.h"
#include "buildmap_square.h"
#include "buildmap_point.h"


/**
 * @brief
 */
typedef struct {
   int node;
   int from_line;
   int to_line;
} BuildMapTurns;

static int TurnsCount = 0;
static int TurnsNodeCount = 0;

static int TurnsMaxNode = 0;

static BuildMapTurns *Turns[BUILDMAP_BLOCK] = {NULL};

static RoadMapHash *TurnsByNode   = NULL;

static int TurnsAddCount = 0;

static int *SortedTurns = NULL;

/**
 * @brief
 */
void buildmap_turn_restrictions_initialize (void) {

   TurnsByNode = roadmap_hash_new ("TurnsByNode", BUILDMAP_BLOCK);

   TurnsMaxNode = 0;

   TurnsAddCount = 0;
   TurnsCount = 0;
}

/**
 * @brief
 * @param node
 * @param from_line
 * @param to_line
 * @return
 */
int buildmap_turn_restrictions_add
       (int node, int from_line, int to_line) {

   int index;
   int node_exists;
   int block;
   int offset;
   BuildMapTurns *this_turn;

   TurnsAddCount += 1;

   /* First search if that shape is not known yet. */

   node_exists = 0;

   for (index = roadmap_hash_get_first (TurnsByNode, node);
        index >= 0;
        index = roadmap_hash_get_next (TurnsByNode, index)) {

      this_turn = Turns[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      if (this_turn->node == node) {

         if ((this_turn->from_line == from_line) &&
             (this_turn->to_line  == to_line )) {
            buildmap_error (0, "duplicated turn_restriction: %d %d %d", node, from_line, to_line);
         }

         node_exists = 1;
      }
   }

   /* This shape was not known yet: create a new one. */

   block = TurnsCount / BUILDMAP_BLOCK;
   offset = TurnsCount % BUILDMAP_BLOCK;

   if (block >= BUILDMAP_BLOCK) {
      buildmap_fatal (0,
         "Underdimensioned turns table (block %d, BUILDMAP_BLOCK %d)",
	 block, BUILDMAP_BLOCK);
   }

   if (block >= BUILDMAP_BLOCK) {
      buildmap_fatal (0, "too many shape records");
   }

   if (Turns[block] == NULL) {

      /* We need to add a new block to the table. */

      Turns[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapTurns));
      if (Turns[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
      roadmap_hash_resize (TurnsByNode, (block+1) * BUILDMAP_BLOCK);
   }

   this_turn = Turns[block] + offset;

   this_turn->node = node;
   this_turn->from_line = from_line;
   this_turn->to_line  = to_line;

   if (! node_exists) {

      TurnsNodeCount += 1;

      if (node > TurnsMaxNode) {
         TurnsMaxNode = node;
      }

      if (node < 0) {
         buildmap_fatal (0, "negative node index: %d", node);
      }
   }

   roadmap_hash_add (TurnsByNode, node, TurnsCount);

   return TurnsCount++;
}


int buildmap_turn_restrictions_exists (int node, int from_line, int to_line) {
   int index;
   BuildMapTurns *this_turn;

   for (index = roadmap_hash_get_first (TurnsByNode, node);
        index >= 0;
        index = roadmap_hash_get_next (TurnsByNode, index)) {

      this_turn = Turns[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      if (this_turn->node == node) {

         if ((this_turn->from_line == from_line) &&
             (this_turn->to_line  == to_line )) {
            return 1;
         }
      }
   }

   return 0;
}

/**
 * @brief support function for sorting the turns
 * @param r1
 * @param r2
 * @return
 */
static int buildmap_turns_compare (const void *r1, const void *r2) {

   int index1 = *((int *)r1);
   int index2 = *((int *)r2);

   BuildMapTurns *record1;
   BuildMapTurns *record2;

   record1 = Turns[index1/BUILDMAP_BLOCK] + (index1 % BUILDMAP_BLOCK);
   record2 = Turns[index2/BUILDMAP_BLOCK] + (index2 % BUILDMAP_BLOCK);

   if (record1->node != record2->node) {
      return record1->node - record2->node;
   }

   if (record1->from_line != record2->from_line) {
      return record1->from_line - record2->from_line;
   }

   return record1->to_line - record2->to_line;
}


/**
 * @brief Sort turns by node id, then by line ids.
 */
void buildmap_turn_restrictions_sort (void) {

   int i;

   if (SortedTurns != NULL) return; /* Sort was already performed. */

   buildmap_info ("sorting turn restrictions...");

   SortedTurns = malloc (TurnsCount * sizeof(int));
   if (SortedTurns == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   for (i = 0; i < TurnsCount; i++) {
      SortedTurns[i] = i;
   }

   qsort (SortedTurns, TurnsCount, sizeof(int), buildmap_turns_compare);
}

/**
 * @brief Save the turn restrictions database
 */
void buildmap_turn_restrictions_save (void) {

   int i;
   int j;
   int square;
   int last_node = -1;
   int node_index = -1;
   int turn_index;
   int last_square = -1;
   int square_count;

   int longitude = 0;
   int latitude = 0;

   RoadMapTurns *db_turns;
   BuildMapTurns *one_turn;
   RoadMapTurnsByNode *db_by_node;
   RoadMapTurnsBySquare *db_bysquare;

   buildmap_db *root;
   buildmap_db *table_square;
   buildmap_db *table_node;
   buildmap_db *table_data;

   return;

   buildmap_info ("saving turn restrictions...");

   root  = buildmap_db_add_section (NULL, "turns");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   square_count = buildmap_square_get_count();

   /* Create the database space. */

   table_square = buildmap_db_add_child
               (root, "bysquare", square_count, sizeof(RoadMapTurnsBySquare));

   table_node = buildmap_db_add_child
                  (root, "bynode", TurnsNodeCount, sizeof(RoadMapTurnsByNode));

   table_data = buildmap_db_add_child
                  (root, "data", TurnsCount, sizeof(RoadMapTurns));

   db_bysquare = (RoadMapTurnsBySquare *) buildmap_db_get_data (table_square);
   db_by_node = (RoadMapTurnsByNode *) buildmap_db_get_data (table_node);
   db_turns  = (RoadMapTurns *) buildmap_db_get_data (table_data);

   last_node = -1;

   for (i = 0, turn_index = 0; i < TurnsCount; i++, turn_index++) {

      j = SortedTurns[i];

      one_turn = Turns[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      if (one_turn->node != last_node) {

         if (last_node > one_turn->node) {
            buildmap_fatal (0, "decreasing node order in turn restrictions table");
         }

         node_index += 1;
         db_by_node[node_index].node = one_turn->node;
         db_by_node[node_index].first = turn_index;
         db_by_node[node_index].count = 0;

         last_node = one_turn->node;

         longitude = buildmap_point_get_longitude_sorted (last_node);
         latitude = buildmap_point_get_latitude_sorted (last_node);

         square = buildmap_point_get_square_sorted (last_node);

         if (square != last_square) {

            if (square < last_square) {
               buildmap_fatal (0, "decreasing square order in turns table");
            }
            while (last_square < square) {
               last_square += 1;
               db_bysquare[last_square].first = node_index;
               db_bysquare[last_square].count = 0;
            }
         }
         db_bysquare[last_square].count += 1;
      }

      db_by_node[node_index].count += 1;

      db_turns[turn_index].from_line = one_turn->from_line;
      db_turns[turn_index].to_line = one_turn->to_line;
   }

   if (turn_index != TurnsCount) {
      buildmap_fatal (0, "inconsistent count of turn restrictions: "
                            "total = %d, saved = %d",
                         TurnsCount, turn_index+1);
   }

   if (last_square >= square_count) {
      buildmap_fatal (0, "inconsistent count of squares: "
                            "total = %d, saved = %d",
                         square_count, last_square+1);
   }

   for (last_square += 1; last_square < square_count; last_square += 1) {
      db_bysquare[last_square].first = node_index;
      db_bysquare[last_square].count = 0;
   }

   if (node_index+1 != TurnsNodeCount) {
      buildmap_fatal (0, "inconsistent count of nodes: "
                            "total = %d, saved = %d",
                         TurnsNodeCount, node_index+1);
   }

#if 0
   if (switch_endian) {
      int i;

      for (i=0; i<square_count; i++) {
         switch_endian_int(&db_bysquare[i].first);
         switch_endian_int(&db_bysquare[i].count);
      }

      for (i=0; i<TurnsNodeCount; i++) {
         switch_endian_int(&db_by_node[i].node);
         switch_endian_int(&db_by_node[i].first);
         switch_endian_int(&db_by_node[i].count);
      }

      for (i=0; i<TurnsCount; i++) {
         switch_endian_int(&db_turns[i].from_line);
         switch_endian_int(&db_turns[i].to_line);
      }
   }
#endif
}

/**
 * @brief
 */
void buildmap_turn_restrictions_summary (void) {

   fprintf (stderr,
            "-- turn restrictions table: %d items, %d add, %d bytes used\n"
            "                            %d lines (range %d), max %d points per line\n",
            TurnsCount, 0,
            TurnsCount * sizeof(RoadMapTurns)
               + (TurnsMaxNode + 1) * sizeof(RoadMapTurnsByNode),
            TurnsNodeCount, TurnsMaxNode, 0);
}

/**
 * @brief
 */
void  buildmap_turn_restrictions_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; i++) {
      if (Turns[i] != NULL) {
         free(Turns[i]);
         Turns[i] = NULL;
      }
   }

   free (SortedTurns);
   SortedTurns = NULL;

   TurnsCount = 0;
   TurnsNodeCount = 0;

   TurnsMaxNode = 0;

   TurnsByNode = NULL;
}

