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
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "roadmap.h"
#include "roadmap_list.h"
#include "roadmap_spawn.h"


static char *RoadMapSpawnPath = NULL;

static RoadMapList RoadMapSpawnActive = ROADMAP_LIST_EMPTY;


static void (*RoadMapSpawnNextHandler) (int signal) = NULL;


static void roadmap_spawn_set_arguments
               (int argc, char *argv[], const char *command_line) {

    int   i = 0;
    char  quoted = 0;
    char *p;

    if (command_line == NULL || command_line[0] == 0) {
        goto end_of_string;
    }
    
    p = strdup (command_line);

    while ((i < argc) && (*p)) {

        while (isspace(*p)) ++p;

        if (*p == 0) break;

        argv[++i] = p;

        while ((! isspace(*p)) || quoted) {
            switch (*p)
            {
            case '"':
            case '\'':
                if (quoted == 0) {
                    quoted = *p;
                } else if (quoted == *p) {
                    quoted = 0;
                }
                break;
                
            case 0: goto end_of_string;
            }
            ++p;
        }

        if (*p == 0) break;

        *p = 0;

        p += 1;
   }

end_of_string:
   argv[++i] = NULL;
}


#ifdef ROADMAP_USES_SIGCHLD
static void roadmap_spawn_child_exit_handler (int signal);

static void roadmap_spawn_set_handler (void) {

   RoadMapSpawnNextHandler =
      signal (SIGCHLD, roadmap_spawn_child_exit_handler);

   if ((RoadMapSpawnNextHandler == SIG_ERR) ||
       (RoadMapSpawnNextHandler == SIG_DFL) ||
       (RoadMapSpawnNextHandler == SIG_IGN) ||
       (RoadMapSpawnNextHandler == roadmap_spawn_child_exit_handler)) {
      RoadMapSpawnNextHandler = NULL;
   }
}


static void roadmap_spawn_child_exit_handler (int signal) {

    if (signal != SIGCHLD) return; /* Should never happen. */

    roadmap_spawn_check ();

    if (RoadMapSpawnNextHandler != NULL) {
        (*RoadMapSpawnNextHandler) (signal);
    }
    roadmap_spawn_set_handler ();
}
#endif


void roadmap_spawn_initialize (const char *argv0) {

   if ((argv0[0] == '/') || (strncmp (argv0, "../", 3) == 0)) {

       char *last_slash;

       if (RoadMapSpawnPath != NULL) {
          free (RoadMapSpawnPath);
       }
       RoadMapSpawnPath = strdup (argv0);
       roadmap_check_allocated(RoadMapSpawnPath);

       last_slash = strrchr (RoadMapSpawnPath, '/');
       last_slash[1] = 0; /* remove the current's program name. */
   }
}


int roadmap_spawn (const char *name, const char *command_line) {

   char *argv[16];
   pid_t child;

#ifdef ROADMAP_USES_SIGCHLD
   roadmap_spawn_set_handler ();
#endif

   child = fork();

   if (child < 0) {
      roadmap_log (ROADMAP_ERROR, "cannot fork(), error %d", errno);
   }

   if (child == 0) {

      /* This is the child process.
       * A few words of caution here: the GUI toolkit library might have
       * registered an exit callback (see atexit(3)), which should not be
       * called here (the child created no ressource that needs to be
       * deallocated). This is why we call _exit(2) and do not use
       * ROADMAP_FATAL or exit(3).
       */

      argv[0] = (char *)name;
      roadmap_spawn_set_arguments (15, argv, command_line);

      if (RoadMapSpawnPath != NULL) {

         struct stat stat_buffer;
         char *fullname;

         fullname = malloc (strlen(RoadMapSpawnPath) + strlen(name) + 4);
         strcpy (fullname, RoadMapSpawnPath);
         strcat (fullname, name);

         if (stat (fullname, &stat_buffer) == 0) {
            execv  (fullname, argv);
            roadmap_log (ROADMAP_ERROR, "execv(\"%s\") failed", fullname);
            _exit(1);
         }
      }
      execvp (name, argv);
      roadmap_log (ROADMAP_ERROR, "execvp(%s) failed", name);
      _exit(1);
   }

   return child;
}


int  roadmap_spawn_with_feedback
         (const char *name,
          const char *command_line,
          RoadMapFeedback *feedback) {
    
    roadmap_list_append (&RoadMapSpawnActive, &feedback->link);
    feedback->child = roadmap_spawn (name, command_line);
              
    return feedback->child;
}


void roadmap_spawn_check (void) {

    int pid;
    int status;
    RoadMapFeedback *item;

    pid = waitpid (-1, &status, WNOHANG);

    if (pid <= 0) return;
        
    for (item = (RoadMapFeedback *)RoadMapSpawnActive.first;
         item != NULL;
         item = (RoadMapFeedback *)item->link.next) {

        if (item->child == pid) {
            item->handler (item->data);
            roadmap_list_remove (&RoadMapSpawnActive, &item->link);
            break;
        }
    }
}

