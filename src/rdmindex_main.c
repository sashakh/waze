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
#include "buildmap_opt.h"


static roadmap_db_model *RoadMapIndexModel;

static const char *RdmIndexName = "index.rdm";


static int   RdmIndexVerbose = 0;
static int   RdmIndexSilent = 0;
static int   RdmIndexGrand = 0;
static int   RdmIndexRecursive = 0;
static char *RdmIndexPath = ".";

struct opt_defs options[] = {
   {"path", "p", opt_string, ".",
        "Location of the generated index file"},
   {"grand", "g", opt_flag, "0",
        "Build a grand index of index files"},
   {"recursive", "r", opt_flag, "0",
        "Search the map files recursively"},
   {"verbose", "v", opt_flag, "0",
        "Show progress information"},
   {"silent", "s", opt_flag, "0",
        "Show nothing"},
   OPT_DEFS_END
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


void usage(char *progpath, const char *msg) {

   char *prog = strrchr(progpath, '/');

   if (prog)
       prog++;
   else
       prog = progpath;

   if (msg)
       fprintf(stderr, "%s: %s\n", prog, msg);
   fprintf(stderr,
       "usage: %s [options] [directory...]\n", prog);
   opt_desc(options, 1);
   exit(1);
}

int main (int argc, char **argv) {

   int i, error;

   /* parse the options */
   error = opt_parse(options, &argc, argv, 0);
   if (error) usage(argv[0], opt_strerror(error));

   /* then, fetch the option values */
   error = opt_val("verbose", &RdmIndexVerbose) ||
           opt_val("silent", &RdmIndexSilent) ||
           opt_val("grand", &RdmIndexGrand) ||
           opt_val("recursive", &RdmIndexRecursive) ||
           opt_val("path", &RdmIndexPath);
   if (error)
      usage(argv[0], opt_strerror(error));


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

   } else {

      RoadMapIndexModel =
         roadmap_db_register
            (RoadMapIndexModel, "metadata", &RoadMapMetadataHandler);
      RoadMapIndexModel =
         roadmap_db_register
            (RoadMapIndexModel, "square", &RoadMapSquareHandler);
      // Unused at this time.
      // RoadMapIndexModel =
      //    roadmap_db_register
      //       (RoadMapIndexModel, "zip", &RoadMapZipHandler);
   }

   if (argc == 1) {
      static char *RdmIndexDefault[] = {NULL, ".", NULL};
      RdmIndexDefault[0] = argv[0];
      argv = RdmIndexDefault;
      argc = 2;
   }

   RoadMapIndexModel =
      roadmap_db_register
         (RoadMapIndexModel, "string", &RoadMapDictionaryHandler);

   if (RdmIndexRecursive) {

      for (i = 1; i < argc; ++i) {
         rdmindex_recurse (argv[i]);
      }

   } else {

      for (i = 1; i < argc; ++i) {
         if (RdmIndexGrand) {
            rdmindex_scan_indexes (argv[i]);
         } else {
            rdmindex_scan_maps (argv[i]);
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

