/* roadmap_address.c - manage the roadmap address dialogs.
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
 *   void roadmap_dialog_destination_address (void);
 *   void roadmap_dialog_location_address (void);
 *   void roadmap_dialog_about (void);
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_gui.h"
#include "roadmap_street.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include "roadmap_messagebox.h"
#include "roadmap_dialog.h"


#define ROADMAP_MAX_STREETS  256


typedef struct {

    const char *title;
    const char *name;
    
    int   use_zip;

    RoadMapPosition *positions;

    void *history;

} RoadMapAddressDialog;



static void roadmap_address_done (const char *name, RoadMapPosition *position) {

    roadmap_trip_set_point (name, position);
    roadmap_trip_set_focus (name, 0);
    roadmap_screen_refresh ();
}


static void roadmap_address_selected (const char *name, void *data) {

   RoadMapPosition *position;
   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   position = (RoadMapPosition *) roadmap_dialog_get_data ("List", ".Streets");

   if (position != NULL) {
      roadmap_address_done (context->name, position);
   }
}


static void roadmap_address_selection_ok (const char *name, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   roadmap_dialog_hide (name);
   roadmap_address_selected (name, data);

   if (context->positions != NULL) {
      free (context->positions);
   }
}


static void roadmap_address_selection (void  *data,
                                       int    count,
                                       char **names,
                                       RoadMapPosition *positions) {

   int i;
   void *list[ROADMAP_MAX_STREETS];


   for (i = 0; i < count; ++i) {
      list[i] = positions + i;
   }

   if (roadmap_dialog_activate ("RoadMap Street Select", data)) {

      roadmap_dialog_new_list ("List", ".Streets");

      roadmap_dialog_add_button ("OK", roadmap_address_selection_ok);

      roadmap_dialog_complete (0); /* No need for a keyboard. */
   }

   roadmap_dialog_show_list
      ("List", ".Streets", count, names, list, roadmap_address_selected);
}


static void roadmap_address_before (const char *name, void *data) {

   char *number;
   char *street;
   char *city;
   char *state;
   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   context->history =
      roadmap_history_get_before
         (context->history, &number, &street, &city, &state);

   roadmap_dialog_set_data ("Address", "Number:", number);
   roadmap_dialog_set_data ("Address", "Street:", street);
   roadmap_dialog_set_data ("Address", "City:",   city);
   roadmap_dialog_set_data ("Address", "State:",  state);
}


static void roadmap_address_after (const char *name, void *data) {

   char *number;
   char *street;
   char *city;
   char *state;
   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   context->history =
      roadmap_history_get_after
         (context->history, &number, &street, &city, &state);

   roadmap_dialog_set_data ("Address", "Number:", number);
   roadmap_dialog_set_data ("Address", "Street:", street);
   roadmap_dialog_set_data ("Address", "City:",   city);
   roadmap_dialog_set_data ("Address", "State:",  state);
}


static void roadmap_address_ok (const char *name, void *data) {

   int i, j, k;
   int count;

   int   lines[ROADMAP_MAX_STREETS];
   char *names[ROADMAP_MAX_STREETS];
   int   street_number[ROADMAP_MAX_STREETS];
   RoadMapBlocks blocks[ROADMAP_MAX_STREETS];

   RoadMapPosition *positions;

   char *street_number_image;
   char *street_name;
   char *city;
   char *state;
   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;


   street_number_image =
      (char *) roadmap_dialog_get_data ("Address", "Number:");

   street_name   = (char *) roadmap_dialog_get_data ("Address", "Street:");
   state         = (char *) roadmap_dialog_get_data ("Address", "State:");

   if (context->use_zip) {
      // TBD: how to select by ZIP ? Need one more table in usdir.
      // int zip =  atoi(roadmap_dialog_get_data ("Address", "Zip:");
      // count = roadmap_street_blocks_by_zip (street_name, zip, blocks, 256);
      return;
   }

   city = (char *) roadmap_dialog_get_data ("Address", "City:");

   roadmap_history_add (street_number_image, street_name, city, state);

   switch (roadmap_locator_by_city (city, state)) {

   case ROADMAP_US_OK:

      count = roadmap_street_blocks_by_city (street_name, city, blocks, 256);
      break;

   case ROADMAP_US_NOSTATE:

      roadmap_messagebox ("RoadMap Warning",
                         "No state with that name could be found");
      return;

   case ROADMAP_US_NOCITY:

      roadmap_messagebox ("RoadMap Warning", 
                          "No city with that name could be found");
      return;

   default:

      roadmap_messagebox ("RoadMap Warning", 
                          "No related map could not be found");
      return;
   }

   if (count <= 0) {
      switch (count) {
      case ROADMAP_STREET_NOADDRESS:
         roadmap_messagebox ("RoadMap Warning", 
                             "The street address could not be found");
         break;
      case ROADMAP_STREET_NOCITY:
         roadmap_messagebox ("RoadMap Warning", 
                             "No city with that name could be found");
         break;
      case ROADMAP_STREET_NOSTREET:
         roadmap_messagebox ("RoadMap Warning", 
                             "No street with that name could be found");
         break;
      default:
         roadmap_messagebox ("RoadMap Warning", 
                             "The address could not be found");
      }
      return;
   }

   if (count > ROADMAP_MAX_STREETS) {
      roadmap_log (ROADMAP_ERROR, "too many blocks");
      count = ROADMAP_MAX_STREETS;
   }

   /* If the street number was not provided, retrieve all street blocks to
    * expand the match list, else decode that street number and update the
    * match list.
    */
   if (street_number_image[0] == 0) {

      int range_count;
      RoadMapStreetRange ranges[ROADMAP_MAX_STREETS];

      for (i = 0, j = 0; i < count; ++i) {
         street_number[i] = -1;
      }

      for (i = 0, j = 0; i < count && street_number[i] < 0; ++i) {

         range_count =
            roadmap_street_get_ranges (blocks+i, ROADMAP_MAX_STREETS, ranges);

         if (range_count > 0) {

            street_number[i] = (ranges[0].min + ranges[0].max) / 2;

            for (k = 1; k < range_count; ++k) {

               if (count >= ROADMAP_MAX_STREETS) {
                  roadmap_log (ROADMAP_ERROR, "too many blocks, cannot expand");
                  break;
               }
               blocks[count] = blocks[i];
               ranges[count] = ranges[k];
               street_number[count] =
                  (ranges[count].min + ranges[count].max) / 2;
               count += 1;
            }
         }
      }

   } else {

      int number;

      number = roadmap_math_street_address
                  (street_number_image, strlen(street_number_image));

      for (i = 0, j = 0; i < count; ++i) {
         street_number[i] = number;
      }
   }

   positions = (RoadMapPosition *) calloc (count, sizeof(RoadMapPosition));

   for (i = 0, j = 0; i< count; ++i) {

      int line;

      line = roadmap_street_get_position
                    (blocks+i, street_number[i], positions+j);

      for (k = 0; k < j; ++k) {
         if (lines[k] == line) {
            line = -1;
            break;
         }
      }

      if (line >= 0) {
        RoadMapStreetProperties properties;
          
        roadmap_street_get_properties (line, &properties);
        names[j] = strdup (roadmap_street_get_full_name (&properties));
        lines[j] = line;
        j += 1;
      }
   }

   if (j <= 0) {

      roadmap_messagebox ("RoadMap Warning", "No valid street was found");
      return;
   }

   roadmap_dialog_hide (name);

   if (j > 1) {

      /* Open a selection dialog to let the user choose the right block. */

      roadmap_address_selection (context, j, names, positions);

      for (i = 0; i < j; ++i) {
         free (names[i]);
      }

      context->positions = positions; /* Free these only when done. */

   } else {

      roadmap_address_done (context->name, positions);

      free (names[0]);
      free (positions);
   }
}


static void roadmap_address_cancel (const char *name, void *data) {

   roadmap_dialog_hide (name);
}


static void roadmap_address_dialog (RoadMapAddressDialog *context) {

   char *street_number_image;
   char *street_name;
   char *city;
   char *state;


   if (roadmap_dialog_activate (context->title, context)) {

      int use_keyboard;

      use_keyboard = (strcasecmp (roadmap_config_get ("General", "Keyboard"),
                                  "yes") == 0);

      roadmap_dialog_new_entry ("Address", "Number:");
      roadmap_dialog_new_entry ("Address", "Street:");
      if (context->use_zip) {
         roadmap_dialog_new_entry ("Address", "Zip:");
      } else {
         roadmap_dialog_new_entry ("Address", "City:");
      }
      roadmap_dialog_new_entry ("Address", "State:");

      roadmap_dialog_add_button ("Back", roadmap_address_before);
      roadmap_dialog_add_button ("Next", roadmap_address_after);
      roadmap_dialog_add_button ("OK", roadmap_address_ok);
      roadmap_dialog_add_button ("Cancel", roadmap_address_cancel);

      roadmap_dialog_complete (use_keyboard);
   }

   context->history =
      roadmap_history_get_latest
         (&street_number_image, &street_name, &city, &state);

   roadmap_dialog_set_data ("Address", "Number:", street_number_image);
   roadmap_dialog_set_data ("Address", "Street:", street_name);
   roadmap_dialog_set_data ("Address", "City:", city);
   roadmap_dialog_set_data ("Address", "State:", state);
}


void roadmap_address_destination_by_city (void) {

   static RoadMapAddressDialog context = {
      "RoadMap Destination",
       "Destination",
      0
   };

   roadmap_address_dialog (&context);
}


void roadmap_address_destination_by_zip (void) {

   static RoadMapAddressDialog context = {
       "RoadMap Destination",
       "Destination",
       1
   };

   roadmap_address_dialog (&context);
}


void roadmap_address_location_by_city (void) {

   static RoadMapAddressDialog context = {
       "RoadMap Location",
       "Location",
       0
   };

   roadmap_address_dialog (&context);
}

void roadmap_address_location_by_zip (void) {

   static RoadMapAddressDialog context = {
       "RoadMap Location",
       "Location",
       1
   };

   roadmap_address_dialog (&context);
}

