/* dumpmap_main.c - a tool to dump information from a RoadMap map file.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <popt.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_dictionary.h"
#include "roadmap_metadata.h"


roadmap_db_model *RoadMapModel = NULL;

static int   DumpMapVerbose = 0;
static int   DumpMapShowStrings = 0;
static int   DumpMapShowAttributes = 0;
static char *DumpMapShowDump = NULL;
static char *DumpMapShowVolume = NULL;
static char *DumpMapSearchStringOption = NULL;


/* Tree print module -------------------------------------------------------
 *
 * This is a simple module as it does not care about keeping any
 * context, it does its jobs once the database has been mapped, and
 * then it forgets about the database.
 */

static void dumpmap_show_size (int size) {

   if (size > 1024) {
      if (size > (1024 * 1024)) {
         printf ("%d.%d Mbytes",
                 size / (1024 * 1024),
                 ((10 * size) / (1024 * 1024)) % 10);
      } else {
         printf ("%d Kbytes", size / 1024);
      }
   } else {
      printf ("%d bytes", size);
   }
}


static void dumpmap_printtree (roadmap_db *parent, roadmap_db *root) {

   int size;
   int count;
   int level;
   roadmap_db *child;

   level = 2 * (roadmap_db_get_level (parent) - roadmap_db_get_level (root));

   for (child = roadmap_db_get_first (parent);
        child != NULL;
        child = roadmap_db_get_next (child)) {

      printf ("%*.*s %s (", level, level, "", roadmap_db_get_name (child));

      size = roadmap_db_get_size (child);

      if (roadmap_db_get_first (child) != NULL) {

         dumpmap_show_size (size);
         printf (") -->\n");
         dumpmap_printtree (child, root);

      } else {

         count = roadmap_db_get_count (child);

         if (count != 0) {
            printf ("%d records, ", count);
         }

         dumpmap_show_size (size);
         printf (")\n");
      }
   }
}

static void *dumpmap_printtree_map (roadmap_db *root) {

   dumpmap_printtree (root, root);
   return "OK";
}

roadmap_db_handler DumpMapPrintTree =
    {"printtree", dumpmap_printtree_map, NULL, NULL};


/* Dictionary print module -------------------------------------------------
 *
 * This is a simple module as it does not care about keeping any
 * context, it does its jobs once the database has been mapped, and
 * then it forgets about the database.
 */

static void *dumpmap_printstring_map (roadmap_db *root) {

   return (*RoadMapDictionaryHandler.map) (root);
}

static void dumpmap_printstring_activate (void *context) {

   (*RoadMapDictionaryHandler.activate) (context);

   if (DumpMapShowVolume != NULL) {
      roadmap_dictionary_dump_volume (DumpMapShowVolume);
   } else {
      roadmap_dictionary_dump ();
   }
}

roadmap_db_handler DumpMapPrintString =
    {"printstring",
     dumpmap_printstring_map, dumpmap_printstring_activate, NULL};


/* Dictionary search module ------------------------------------------------
 *
 * This is a simple module as it does not care about keeping any
 * context, it does its jobs once the database has been mapped, and
 * then it forgets about the database.
 */

static void *dumpmap_searchstring_map (roadmap_db *root) {

   return "OK"; /* Must not be null. */
}

static void dumpmap_searchstring_activate (void *context) {

   char data[512];
   char choices[128];
   char input;
   RoadMapString result;

   RoadMapDictionary       streets;
   RoadMapDictionaryCursor cursor;


   streets = roadmap_dictionary_open (DumpMapSearchStringOption);
   if (streets == NULL) {
      fprintf (stderr,
               "Could not open the dictionary volume %s\n",
               DumpMapSearchStringOption);
      exit (1);
   }

   cursor = roadmap_dictionary_new_cursor (streets);

   roadmap_dictionary_get_next (cursor, choices);

   while (choices[0]) {

      if (roadmap_dictionary_completable (cursor)) {
         result = roadmap_dictionary_get_result (cursor);
         printf ("A street of that name exists at index %d.\n", result);
      }

      roadmap_dictionary_completion (cursor, data);
      printf ("Current street name: %s\n", data);

      input = ' ';
      do {
         if (input != '\n') {
            printf ("Please select one of these characters: '%s'\n", choices);
         }
         scanf ("%c", &input);

      } while (strchr (choices, input) == NULL);

      roadmap_dictionary_move_cursor (cursor, input);
      roadmap_dictionary_get_next (cursor, choices);
   }

   result = roadmap_dictionary_get_result (cursor);
   printf ("Street index & name are: %d, %s\n",
           result, roadmap_dictionary_get (streets, result));
   
   roadmap_dictionary_free_cursor (cursor);
}

roadmap_db_handler DumpMapSearchString =
    {"searchstring",
     dumpmap_searchstring_map, dumpmap_searchstring_activate, NULL};


/* Attributes print module -------------------------------------------------
 *
 * This is a simple module as it does not care about keeping any
 * context, it does its jobs once the database has been mapped, and
 * then it forgets about the database.
 */

static void *dumpmap_attributes_map (roadmap_db *root) {

   return (*RoadMapMetadataHandler.map) (root);
}

static void dumpmap_attributes_print (const char *category,
                                      const char *name,
                                      const char *value) {

   printf ("   %s.%s: %s\n", category, name, value);
}

static void dumpmap_attributes_activate (void *context) {

   (*RoadMapMetadataHandler.activate) (context);

   if (DumpMapShowAttributes != 0) {
      roadmap_metadata_scan_attributes (dumpmap_attributes_print);
   } else {
      roadmap_dictionary_dump ();
   }
}

roadmap_db_handler DumpMapPrintAttributes =
    {"attributes",
     dumpmap_attributes_map, dumpmap_attributes_activate, NULL};


/* Section dump module -----------------------------------------------------
 *
 * This is a simple module as it does not care about keeping any
 * context, it does its jobs once the database has been mapped, and
 * then it forgets about the database.
 */

static void *dumpmap_hexadump_map (roadmap_db *root) {

   int i;
   int size;
   const char *data;

   size = roadmap_db_get_size (root);
   data = roadmap_db_get_data (root);

   for (i = 0; i <= size - 16; i += 16) {

      int j;

      printf ("%08d  ", i);

      for (j = 0; j < 16; ++j) {
         printf (" %02x", 0xff & ((int)data[i+j]));
      }
      printf ("\n");
   }

   if (i < size) {
      printf ("%08d  ", i);
      for (; i < size; ++i) {
         printf (" %02x", 0xff & ((int)data[i]));
      }
      printf ("\n");
   }

   return "OK";
}


roadmap_db_handler DumpMapHexaDump =
    {"hexadump", dumpmap_hexadump_map, NULL, NULL};


/* Main program. -----------------------------------------------------------
 */
static struct poptOption DumpMapDictionaryOptions[] = {

   {"strings", 0,
      POPT_ARG_NONE, &DumpMapShowStrings, 0, "Show dictionary's content", NULL},

   {"volume", 0,
      POPT_ARG_STRING, &DumpMapShowVolume, 0, "Show a specific volume", "NAME"},

   {"search", 0,
      POPT_ARG_STRING, &DumpMapSearchStringOption, 0, "Search through a specific volume", "NAME"},

   {NULL, 0, 0, NULL, 0, NULL, NULL}

};

static struct poptOption DumpMapOptions[] = {

   POPT_AUTOHELP

   {"verbose", 'v',
      POPT_ARG_NONE, &DumpMapVerbose, 0, "Show additional information", NULL},

   {"dump", 'd',
      POPT_ARG_STRING, &DumpMapShowDump, 0, "Dump a specific table", "TABLE"},

   {"attributes", 'a',
      POPT_ARG_NONE, &DumpMapShowAttributes, 0, "Show map attributes", NULL},

   {NULL, 0,
        POPT_ARG_INCLUDE_TABLE, DumpMapDictionaryOptions, 0, "Dictionary options", NULL},

   {NULL, 0, 0, NULL, 0, NULL, NULL}
};


int main (int argc, const char **argv) {

   int i;
   const char **leftovers;


   poptContext decoder =
      poptGetContext ("dumpmap", argc, argv, DumpMapOptions, 0);


   while (poptGetNextOpt(decoder) > 0) ;


   if (DumpMapShowAttributes) {

      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "metadata", &DumpMapPrintAttributes);
      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "string", &RoadMapDictionaryHandler);

   } else if (DumpMapShowStrings || (DumpMapShowVolume != NULL)) {

      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "string", &DumpMapPrintString);

   } else if (DumpMapSearchStringOption != NULL) {

      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "string", &RoadMapDictionaryHandler);
      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "/", &DumpMapSearchString);

   } else if (DumpMapShowDump != NULL) {

      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, DumpMapShowDump, &DumpMapHexaDump);

   } else {

      RoadMapModel =
         roadmap_db_register (RoadMapModel, "/", &DumpMapPrintTree);
   }


   leftovers = poptGetArgs(decoder);

   if (leftovers == NULL || leftovers[0] == NULL) {
      fprintf (stderr, "Please provide the name of a map file\n");
      poptPrintUsage (decoder, stderr, 0);
      exit (1);
   }


   for (i = 0; leftovers[i] != NULL; ++i) {

      if (leftovers[1] != NULL) printf ("%s\n", leftovers[i]);

      if (! roadmap_db_open ("", leftovers[i], RoadMapModel)) {
         roadmap_log (ROADMAP_FATAL,
                      "cannot open map database %s", leftovers[i]);
      }
      roadmap_db_close ("", leftovers[i]);
   }

   roadmap_db_end ();

   return 0;
}

