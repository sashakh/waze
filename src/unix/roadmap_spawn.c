/* roadmap_spawn.c - Process control interface for the RoadMap application.
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_spawn.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "roadmap.h"
#include "roadmap_spawn.h"


static char *RoadMapSpawnPath = NULL;

static void (*RoadMapSpawnNextHandler) (int signal) = NULL;


static void roadmap_spawn_set_arguments
               (int argc, char *argv[], const char *command_line) {

   int   i = 0;
   char *p = strdup (command_line);

   while ((i < argc) && (*p)) {

      while (isspace(*p)) ++p;

      if (*p == 0) break;

      argv[++i] = p;

      while ((! isspace(*p)) && (*p)) ++p;

      if (*p == 0) break;

      *p = 0;

      p += 1;
   }

   argv[++i] = NULL;
}


static void roadmap_spawn_child_exit_handler (int signal);

static void roadmap_spawn_set_handler (void) {

   RoadMapSpawnNextHandler =
      signal (SIGCHLD, roadmap_spawn_child_exit_handler);

   if ((RoadMapSpawnNextHandler == SIG_ERR) ||
       (RoadMapSpawnNextHandler == roadmap_spawn_child_exit_handler)) {
      RoadMapSpawnNextHandler = NULL;
   }
}


static void roadmap_spawn_child_exit_handler (int signal) {

   int status;

   waitpid (-1, &status, WNOHANG);

   if (RoadMapSpawnNextHandler != NULL) {
      (*RoadMapSpawnNextHandler) (signal);
   }
   roadmap_spawn_set_handler ();
}


void roadmap_spawn_initialize (const char *argv0) {

   if ((argv0[0] == '/') || (strncmp (argv0, "../", 3) == 0)) {

       char *last_slash;

       if (RoadMapSpawnPath != NULL) {
          free (RoadMapSpawnPath);
       }
       RoadMapSpawnPath = strdup (argv0);
       if (RoadMapSpawnPath == NULL) {
          roadmap_log (ROADMAP_FATAL, "no more memory");
       }
       last_slash = strrchr (RoadMapSpawnPath, '/');
       last_slash[1] = 0; /* remove the current's program name. */
   }
}


int  roadmap_spawn (const char *name, const char *command_line) {

   char *argv[16];
   pid_t child = fork();

   if (child < 0) {
      roadmap_log (ROADMAP_ERROR, "cannot fork(), error %d", errno);
   }

   if (child == 0) {

      argv[0] = (char *)name;
      roadmap_spawn_set_arguments (15, argv, command_line);

      if (RoadMapSpawnPath == NULL) {

         execvp (name, argv);
         roadmap_log (ROADMAP_FATAL, "execvp(%s) failed", name);

      } else {

         char *fullname = malloc (strlen(RoadMapSpawnPath) + strlen(name) + 4);

         strcpy (fullname, RoadMapSpawnPath);
         strcat (fullname, name);
         execv  (fullname, argv);
         roadmap_log (ROADMAP_FATAL, "execv(%s) failed", fullname);
      }
   }

#ifdef QWS
   /* Why not using roadmap_spawn_set_handler()? Ask Latchesar Ionkov. */
#else
   roadmap_spawn_set_handler ();
#endif

   return child;
}

