/* rdmindex_main.c - The main function of the map index builder tool.
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
#include <stdlib.h>
#include <stdio.h>

#include <popt.h>

#include "roadmap_dbread.h"
#include "roadmap_dictionary.h"
#include "roadmap_types.h"
#include "roadmap_hash.h"
#include "roadmap_path.h"

#include "roadmap_index.h"
#include "roadmap_square.h"
#include "roadmap_metadata.h"
#include "roadmap_file.h"

#include "buildmap.h"
#include "buildmap_index.h"


static roadmap_db_model *RoadMapIndexModel;

static const char *RdmIndexName = "index.rdm";


static int   RdmIndexVerbose = 0;
static int   RdmIndexSilent = 0;
static int   RdmIndexGrand = 0;
static int   RdmIndexRecursive = 0;
static char *RdmIndexPath = NULL;

static struct poptOption RdmIndexOptions[] = {

   POPT_AUTOHELP

   {"grand", 'g',
      POPT_ARG_NONE, &RdmIndexGrand, 0,
      "Build a grand index of index files", NULL},

   {"recursive", 'r',
      POPT_ARG_NONE, &RdmIndexRecursive, 0,
      "Search the map files recursively", NULL},

   {"verbose", 'v',
      POPT_ARG_NONE, &RdmIndexVerbose, 0, "Show progress information", NULL},

   {"silent", 's',
      POPT_ARG_NONE, &RdmIndexSilent, 0, "Show nothing", NULL},

   {"path", 'p',
      POPT_ARG_STRING, &RdmIndexPath, 0,
      "Location of the generated index file", "PATH"},

   {NULL, 0, 0, NULL, 0, NULL, NULL}
};


static void rdmindex_save (void) {

   buildmap_set_source (RdmIndexName);
   buildmap_db_open (RdmIndexPath, RdmIndexName);

   buildmap_db_save ();

   buildmap_db_close ();
}


static void rdmindex_scan_cities (void) {

   int i;
   char *name;
   RoadMapDictionary cities;


   cities = roadmap_dictionary_open ("city");

   if (! cities) return; /* May not exist is all map files. */

   for (i = 1, name = roadmap_dictionary_get (cities, 1);
        name != NULL;
        name = roadmap_dictionary_get (cities, ++i)) {

      buildmap_index_add_city (name);
   }
}


static void rdmindex_scan_indexes (const char *path) {

}


static void rdmindex_scan_maps (const char *path) {

   int i;

   char **files;
   char **cursor;

   char *name;
   char *fullname;

   int   wtid;
   const char *class;
   const char *attribute;

   RoadMapArea edges;


   roadmap_path_set ("maps", path);

   files = roadmap_path_list (path, ".rdm");

   for (cursor = files; *cursor != NULL; ++cursor) {

      name = *cursor;

      if (strcmp (name, RdmIndexName) == 0) continue;

      /* Compatibility with old versions. */
      if (strcmp (name, "usdir.rdm") == 0) continue;

      fullname = roadmap_path_join (path, name);

      buildmap_set_source (fullname);
      if (! RdmIndexSilent) buildmap_info ("indexing...");

      if (! roadmap_db_open (path, name, RoadMapIndexModel)) {
         buildmap_fatal (0, "cannot open the map database");
      }
      roadmap_db_activate (path, name);

      wtid = atoi(roadmap_metadata_get_attribute ("Territory", "Id"));
      class = roadmap_metadata_get_attribute ("Class", "Name");

      if (wtid == 0 || class[0] == 0) {

         buildmap_error
            (0, "incorrect map identity: ID is '%s', class is '%s'",
             roadmap_metadata_get_attribute ("Territory", "Id"),
             class);

      } else {

         attribute = roadmap_metadata_get_attribute ("Territory", "Parent");

         buildmap_index_add_map (wtid, class, attribute, fullname);

         roadmap_square_edges (ROADMAP_SQUARE_GLOBAL, &edges);
         buildmap_index_set_map_edges (&edges);

         for (i = 1; ; ++i) {

            attribute =
               roadmap_metadata_get_attribute_next ("Territory", "Parent", i);

            if ((attribute == NULL)  || (attribute[0] == 0)) break;

            buildmap_index_add_authority_name (attribute);
         }

         attribute = roadmap_metadata_get_attribute ("Territory", "Name");
         if (attribute[0] != 0) {
            buildmap_index_set_territory_name (attribute);
         }

         rdmindex_scan_cities ();

         // TBD: Postal codes.
      }

      roadmap_db_close (path, name);
      roadmap_path_free(fullname);
   }

   roadmap_path_list_free (files);
}


static const char **rdmindex_subdirectories (const char *path) {

   char **files = roadmap_path_list (path, "");
   char **cursor1;
   char **cursor2;

   for (cursor1 = cursor2 = files; *cursor1 != NULL; ++cursor1) {

      char *filename = roadmap_path_join (path, *cursor1);

      if (roadmap_path_is_directory (filename)) {

         if (cursor2 != cursor1) {
            *cursor2 = *cursor1;
         }
         ++cursor2;
      }
      roadmap_path_free (filename);
   }
   *cursor2 = NULL;

   return (const char **)files;
}


static void rdmindex_recurse (const char *path) {

   const char **files;
   const char **cursor;

   if (RdmIndexGrand) {
      rdmindex_scan_indexes (path);
   } else {
      rdmindex_scan_maps (path);
   }

   files = rdmindex_subdirectories (path);

   for (cursor = files; *cursor != NULL; ++cursor) {

      char *filename = roadmap_path_join (path, *cursor);

      rdmindex_recurse (filename);
      roadmap_path_free (filename);
   }

   roadmap_path_list_free ((char **)files);
}


int main (int argc, const char **argv) {

   int i;
   poptContext decoder;
   const char **leftovers;


   decoder = poptGetContext ("rdmindex", argc, argv, RdmIndexOptions, 0);

   while (poptGetNextOpt(decoder) > 0) ;

   leftovers = poptGetArgs(decoder);


   /* The index path is set to the one directory provided as a "smart"
    * default that is assumed to match the intend in most cases.
    */
   if ((RdmIndexPath == NULL) && (leftovers[0] != NULL)) {
      if ((leftovers[1] == NULL) && roadmap_path_is_directory(leftovers[0])) {
         RdmIndexPath = strdup(leftovers[0]);
      }
   }

   /* Nothing really fits: use the ultimate default. */

   if (RdmIndexPath == NULL) {
      RdmIndexPath = strdup(roadmap_path_preferred("maps")); /* default. */
   }

   /* Make sure we have a directory there. */

   if (! roadmap_path_is_directory (RdmIndexPath)) {
      if (roadmap_file_exists (NULL, RdmIndexPath)) {
         buildmap_fatal (-1, "path %s is not a directory", RdmIndexPath);
      } else {
         buildmap_fatal (-1, "path %s does not exist", RdmIndexPath);
      }
   }

   buildmap_index_initialize (RdmIndexPath);


   if (RdmIndexGrand) {

      RoadMapIndexModel =
         roadmap_db_register
            (RoadMapIndexModel, "index", &RoadMapIndexHandler);

      /* By default, we scan the local subdirectories. */

      if (leftovers == NULL || leftovers[0] == NULL) {
         leftovers = rdmindex_subdirectories (".");
      }

   } else {

      RoadMapIndexModel =
         roadmap_db_register
            (RoadMapIndexModel, "metadata", &RoadMapMetadataHandler);
      RoadMapIndexModel =
         roadmap_db_register
            (RoadMapIndexModel, "square", &RoadMapSquareHandler);
      // Unused at that time.
      // RoadMapIndexModel =
      //    roadmap_db_register
      //       (RoadMapIndexModel, "zip", &RoadMapZipHandler);

      /* By default we scan the local directory. */

      if (leftovers == NULL || leftovers[0] == NULL) {
         static const char *RdmIndexDefault[] = {".", NULL};
         leftovers = RdmIndexDefault;
      }
   }

   if (leftovers == NULL || leftovers[0] == NULL) {
      poptPrintUsage (decoder, stderr, 0); /* No argument? */
      return 1;
   }

   RoadMapIndexModel =
      roadmap_db_register
         (RoadMapIndexModel, "string", &RoadMapDictionaryHandler);



   if (RdmIndexRecursive) {

      for (i = 0; leftovers[i] != NULL; ++i) {
         rdmindex_recurse (leftovers[i]);
      }

   } else {

      for (i = 0; leftovers[i] != NULL; ++i) {
         if (RdmIndexGrand) {
            rdmindex_scan_indexes (leftovers[i]);
         } else {
            rdmindex_scan_maps (leftovers[i]);
         }
      }
   }

   buildmap_set_source (RdmIndexName);

   buildmap_db_sort();

   if (RdmIndexVerbose) {

      roadmap_hash_summary ();
      buildmap_db_summary ();
   }

   rdmindex_save ();

   return 0;
}

