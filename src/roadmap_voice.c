/* roadmap_voice.c - Manage voice announcements.
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
 *   See roadmap_voice.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_message.h"
#include "roadmap_spawn.h"

#include "roadmap_voice.h"


static int   RoadMapVoiceMuted = 0;
static int   RoadMapVoiceInUse = 0;
static char *RoadMapVoiceName = NULL;
static char *RoadMapVoiceArguments = NULL;

static RoadMapFeedback RoadMapVoiceActive;


struct voice_translation {
    char *from;
    char *to;
};


/* The translations below must be sorted by decreasing size,
 * so that the longest match is always selected first.
 */
static struct voice_translation RoadMapVoiceTranslation[] = {
    {"Blvd", "boulevard"},
    {"Cir",  "circle"},
    {"St",   "street"},
    {"Rd",   "road"},
    {"Pl",   "place"},
    {"Ct",   "court"},
    {"Dr",   "drive"},
    {"Ln",   "lane"},
    {"N",    "north"},
    {"W",    "west"},
    {"S",    "south"},
    {"E",    "east"},
    {NULL, NULL}
};


static void roadmap_voice_queue (const char *name, const char *arguments) {
    
    if (RoadMapVoiceInUse) {
        
        /* Replace the previously queued message (too old now). */
        
        if (RoadMapVoiceName != NULL) {
            free(RoadMapVoiceName);
        }
        RoadMapVoiceName = strdup (name);
        
        if (RoadMapVoiceArguments != NULL) {
            free(RoadMapVoiceArguments);
        }
        RoadMapVoiceArguments = strdup (arguments);

    } else {
        
        /* Play this message now. */
        
        RoadMapVoiceInUse = 1;
        roadmap_spawn_with_feedback (name, arguments, &RoadMapVoiceActive);
    }
}


static void roadmap_voice_complete (void *data) {
    
    if (RoadMapVoiceName != NULL) {
        
        /* Play the queued message now. */
        
        RoadMapVoiceInUse = 1;
        roadmap_spawn_with_feedback
            (RoadMapVoiceName, RoadMapVoiceArguments, &RoadMapVoiceActive);
        
        free (RoadMapVoiceName);
        RoadMapVoiceName = NULL;
        
        if (RoadMapVoiceArguments != NULL) {
            free (RoadMapVoiceArguments);
            RoadMapVoiceArguments = NULL;
        }
        
    } else {
        
        /* The sound device is now available (as far as we know). */
        
        RoadMapVoiceInUse = 0;
    }
}


static int roadmap_voice_expand (const char *input, char *output, int size) {

    int length;
    int acronym_length;
    const char *acronym;
    struct voice_translation *cursor;
    const char *acronym_found;
    struct voice_translation *cursor_found;

    if (size <= 0) {
        return 0;
    }
    
    acronym = input;
    acronym_length = 0;
    acronym_found = input + strlen(input);
    cursor_found  = NULL;
    
    for (cursor = RoadMapVoiceTranslation; cursor->from != NULL; ++cursor) {
        
        acronym = strstr (input, cursor->from);
        if (acronym != NULL) {
            if (acronym < acronym_found) {
                acronym_found = acronym;
                cursor_found  = cursor;
            }
        }
    }
    
    if (cursor_found == NULL) {
        strncpy (output, input, size);
        return 1;
    }

    acronym = acronym_found;
    cursor  = cursor_found;
    acronym_length = strlen(cursor->from);
    
    length = acronym - input;
        
    if (length > size) return 0;
    
    /* Copy the unexpanded part, up to the acronym that was found. */
    strncpy (output, input, length);
    output += length;
    size -= length;

    if (size <= 0) return 0;
    
    if ((acronym_length != 0) &&
        (acronym[acronym_length] == ' ' ||
         acronym[acronym_length] == ',' ||
         acronym[acronym_length] == 0)) {
        
        /* This is a valid acronym: translate it. */
        length = strlen(cursor->to);
        strncpy (output, cursor->to, size);
        output += length;
        size   -= length;
        
        if (size <= 0) return 0;

    } else {
        /* This is not a valid acronym: leave it unchanged. */
        strncpy (output, acronym, acronym_length);
        output += acronym_length;
        size   -= acronym_length;
    }
        
        
    return roadmap_voice_expand (acronym + acronym_length, output, size);
}


static void roadmap_voice_announce (const char *name, const char *format) {

    char  text[1024];
    char  expanded[1024];
    
    const char *final;
    char *arguments;

    
    if (RoadMapVoiceMuted) return;

    RoadMapVoiceActive.handler = roadmap_voice_complete;
    
    /* remove any heading number or punctuation from the message, as
     * these are more annoying than useful.
     * We must, however, keep the streets which name is a number...
     */
    while ((*name < 'a' || *name > 'z') && (*name < 'A' || *name > 'Z')) {
        name += 1;
    }
    if (strncmp (name, "th ", 3) == 0 ||
        strncmp (name, "st ", 3) == 0 ||
        strncmp (name, "nd ", 3) == 0 ||
        strncmp (name, "rd ", 3) == 0) {
        for (name -= 1; *name >= '0' && *name <= '9'; --name) ;
        name += 1;
    }
    
    if (roadmap_voice_expand (name, expanded, sizeof(expanded))) {
        final = expanded;
    } else {
        final = name;
    }
    roadmap_message_set ('N', "%s", final);
    
    roadmap_message_format (text, sizeof(text),
                            roadmap_config_get ("Voice", format));
    
    arguments = strchr (text, ' ');
    
    if (arguments == NULL) {
        
        roadmap_voice_queue (text, "");
        
    } else {
        
        *arguments = 0;
        
        while (isspace(*(++arguments))) ;
            
        roadmap_voice_queue (text, arguments);
    }
}


void roadmap_voice_approach (const char *name) {

    roadmap_voice_announce (name, "Approach");
}

void roadmap_voice_current_street (const char *name) {

    roadmap_voice_announce (name, "Current Street");
}

void roadmap_voice_intersection (const char *name) {

    roadmap_voice_announce (name, "Next Intersection");
}

void roadmap_voice_selected (const char *name) {

    roadmap_voice_announce (name, "Selected Street");
}


void roadmap_voice_mute (void) {
    RoadMapVoiceMuted = 1;
}

void roadmap_voice_enable (void) {
    RoadMapVoiceMuted = 0;
}


void roadmap_voice_initialize (void) {
    
    roadmap_config_declare ("preferences", "Voice", "Approach", "flite -t 'Approaching %N'");
    roadmap_config_declare ("preferences", "Voice", "Current Street", "flite -t 'Current street is %N'");
    roadmap_config_declare ("preferences", "Voice", "Next Intersection", "flite -t 'Next intersection: %N'");
    roadmap_config_declare ("preferences", "Voice", "Selected Street", "flite -t 'Selected %N'");
}
