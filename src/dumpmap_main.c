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

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_dictionary.h"
#include "roadmap_metadata.h"
#include "roadmap_index.h"
#include "buildmap_opt.h"


roadmap_db_model *RoadMapModel = NULL;

static int   DumpMapVerbose = 0;
static int   DumpMapShowStrings = 0;
static int   DumpMapShowAttributes = 0;
static int   DumpMapShowIndex = 0;
static char *DumpMapShowDump = "";
static char *DumpMapShowVolume = "";
static char *DumpMapSearchStringOption = "";


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

   if (*DumpMapShowVolume) {
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


/* Index print module ------------------------------------------------------
 *
 * This is a simple module as it does not care about keeping any
 * context, it does its jobs once the database has been mapped, and
 * then it forgets about the database.
 */

static void *dumpmap_index_map (roadmap_db *root) {

   return (*RoadMapIndexHandler.map) (root);
}

static void roadmap_index_print_maps (int wtid) {

   int i;
   int max;
   int count;
   char **classes;
   char **files;

   max = roadmap_index_get_map_count ();
   classes = calloc (max, sizeof(char *));
   roadmap_check_allocated(classes);
   files = calloc (max, sizeof(char *));
   roadmap_check_allocated(files);

   count = roadmap_index_by_territory (wtid, classes, files, max);

   for (i = 0; i < count; ++i) {
      printf ("         map %s (%s)\n", files[i], classes[i]);
   }
}

static void roadmap_index_print_territories (const char *authority) {

   int i;
   int max;
   int count;
   int *list;

   max = roadmap_index_get_territory_count ();
   list = calloc (max, sizeof(int));
   roadmap_check_allocated(list);

   count = roadmap_index_by_authority (authority, list, max);

   for (i = 0; i < count; ++i) {

      const char *path = roadmap_index_get_territory_path (list[i]);

      if (path == NULL || path[0] == 0) path = ".";

      printf ("      Territory %09d (%s) at %s\n",
              list[i], roadmap_index_get_territory_name (list[i]), path);
      roadmap_index_print_maps (list[i]);
   }
}

static void roadmap_index_print_authorities (void) {

   char **names;
   char **symbols;
   int authority_count;
   int authority;


   printf ("RoadMap index tables:\n");

   authority_count = roadmap_index_list_authorities (&symbols, &names);

   for (authority = 0; authority < authority_count; ++authority) {

       printf ("   Authority %s (%s)\n", symbols[authority], names[authority]);

       roadmap_index_print_territories (symbols[authority]);
   }
}

static void dumpmap_index_activate (void *context) {

   (*RoadMapIndexHandler.activate) (context);

   if (DumpMapShowIndex != 0) {
      roadmap_index_print_authorities ();
   } else {
      roadmap_dictionary_dump ();
   }
}

roadmap_db_handler DumpMapPrintIndex =
    {"index",
     dumpmap_index_map, dumpmap_index_activate, NULL};


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
struct opt_defs options[] = {
   {"strings", "", opt_flag, "0",
        "Show a dictionary's content"},
   {"volume", "", opt_string, "",
        "Show a specific volume"},
   {"search", "", opt_string, "",
        "Search through a specific volume"},
   {"dump", "d", opt_string, "",
        "Dump a specific table"},
   {"attributes", "a", opt_flag, "0",
        "Show map attributes"},
   {"index", "i", opt_flag, "0",
        "Show map index"},
   {"verbose", "v", opt_flag, "0",
        "Show more progress information"},
   OPT_DEFS_END
};

void usage(char *progpath, const char *msg) {

   char *prog = strrchr(progpath, '/');

   if (prog)
       prog++;
   else
       prog = progpath;

   if (msg)
       fprintf(stderr, "%s: %s\n", prog, msg);
   fprintf(stderr,
       "usage: %s [options] mapfile [mapfile...]\n", prog);
   opt_desc(options, 1);
   exit(1);
}

int main (int argc, char **argv) {

   int i, error;

   /* parse the options */
   error = opt_parse(options, &argc, argv, 0);
   if (error) usage(argv[0], opt_strerror(error));

   /* then, fetch the option values */
   error = opt_val("verbose", &DumpMapVerbose) ||
           opt_val("strings", &DumpMapShowStrings) ||
           opt_val("volume", &DumpMapShowVolume) ||
           opt_val("search", &DumpMapSearchStringOption) ||
           opt_val("dump", &DumpMapShowDump) ||
           opt_val("attributes", &DumpMapShowAttributes) ||
           opt_val("index", &DumpMapShowIndex);
   if (error)
      usage(argv[0], opt_strerror(error));


   if (DumpMapShowAttributes) {

      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "metadata", &DumpMapPrintAttributes);
      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "string", &RoadMapDictionaryHandler);

   } else if (DumpMapShowIndex) {

      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "index", &DumpMapPrintIndex);
      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "string", &RoadMapDictionaryHandler);

   } else if (DumpMapShowStrings || *DumpMapShowVolume) {

      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "string", &DumpMapPrintString);

   } else if (*DumpMapSearchStringOption) {

      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "string", &RoadMapDictionaryHandler);
      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, "/", &DumpMapSearchString);

   } else if (*DumpMapShowDump) {

      RoadMapModel =
         roadmap_db_register
            (RoadMapModel, DumpMapShowDump, &DumpMapHexaDump);

   } else {

      RoadMapModel =
         roadmap_db_register (RoadMapModel, "/", &DumpMapPrintTree);
   }


   if (argc < 2)
      usage(argv[0], "missing map file\n");


   for (i = 1; i < argc; ++i) {

      if (argc >= 3) printf ("%s\n", argv[i]);

      if (! roadmap_db_open ("", argv[i], RoadMapModel)) {
         roadmap_log (ROADMAP_FATAL, "cannot open map database %s", argv[i]);
      }
      roadmap_db_close ("", argv[i]);
   }

   roadmap_db_end ();

   return 0;
}

