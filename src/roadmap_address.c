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
 *   See roadmap_address.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_gui.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include "roadmap_messagebox.h"
#include "roadmap_dialog.h"
#include "roadmap_display.h"
#include "roadmap_preferences.h"
#include "roadmap_geocode.h"

#include "roadmap_address.h"


typedef struct {

    const char *title;
    
    int   use_zip;

    RoadMapGeocode *selections;

    void *history;

} RoadMapAddressDialog;


static void roadmap_address_done (RoadMapGeocode *selected) {

    PluginStreet street;
    PluginLine line;

    roadmap_locator_activate (selected->fips);

    roadmap_log (ROADMAP_DEBUG, "selected address at %d.%06d %c, %d.%06d %c",
                 abs(selected->position.longitude)/1000000,
                 abs(selected->position.longitude)%1000000,
                 selected->position.longitude >= 0 ? 'E' : 'W',
                 abs(selected->position.latitude)/1000000,
                 abs(selected->position.latitude)%1000000,
                 selected->position.latitude >= 0 ? 'N' : 'S');

    roadmap_plugin_set_line
       (&line, ROADMAP_PLUGIN_ID, selected->line, -1, selected->fips);

    roadmap_display_activate
       ("Selected Street", &line, &selected->position, &street);

    roadmap_trip_set_point ("Selection", &selected->position);
    roadmap_trip_set_point ("Address", &selected->position);
    roadmap_trip_set_focus ("Address");

    roadmap_screen_refresh ();
}


static void roadmap_address_selected (const char *name, void *data) {

   RoadMapGeocode *selected;

   selected =
      (RoadMapGeocode *) roadmap_dialog_get_data ("List", ".Streets");

   if (selected != NULL) {
      roadmap_address_done (selected);
   }
}


static void roadmap_address_selection_ok (const char *name, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   roadmap_dialog_hide (name);
   roadmap_address_selected (name, data);

   if (context->selections != NULL) {
      free (context->selections);
      context->selections = NULL;
   }
}


static void roadmap_address_selection (void  *data,
                                       int    count,
                                       RoadMapGeocode *selections) {

   int i;
   char **names = calloc (count, sizeof(char *));
   void **list  = calloc (count, sizeof(void *));


   roadmap_check_allocated(list);
   roadmap_check_allocated(names);

   for (i = count-1; i >= 0; --i) {
      names[i] = selections[i].name;
      list[i] = selections + i;
   }


   if (roadmap_dialog_activate ("Street Select", data)) {

      roadmap_dialog_new_list ("List", ".Streets");

      roadmap_dialog_add_button ("OK", roadmap_address_selection_ok);

      roadmap_dialog_complete (0); /* No need for a keyboard. */
   }

   roadmap_dialog_show_list
      ("List", ".Streets", count, names, list, roadmap_address_selected);

   free (names);
   free (list);
}


static void roadmap_address_set (RoadMapAddressDialog *context) {

   char *argv[4];

   roadmap_history_get ('A', context->history, argv);

   roadmap_dialog_set_data ("Address", "Number:", argv[0]);
   roadmap_dialog_set_data ("Address", "Street:", argv[1]);
   roadmap_dialog_set_data ("Address", "City:",   argv[2]);
   roadmap_dialog_set_data ("Address", "State:",  argv[3]);
}


static void roadmap_address_before (const char *name, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   context->history = roadmap_history_before ('A', context->history);

   roadmap_address_set (context);
}


static void roadmap_address_after (const char *name, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   context->history = roadmap_history_after ('A', context->history);

   roadmap_address_set (context);
}


static void roadmap_address_ok (const char *name, void *data) {

   int i;
   int count;

   RoadMapGeocode *selections;

   char *street_number_image;
   char *street_name;
   char *city;
   char *state;
   const char *argv[4];

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;


   street_number_image =
      (char *) roadmap_dialog_get_data ("Address", "Number:");

   street_name   = (char *) roadmap_dialog_get_data ("Address", "Street:");
   state         = (char *) roadmap_dialog_get_data ("Address", "State:");

   if (context->use_zip) {
      return; /* TBD: how to select by ZIP ? Need one more table in usdir. */
   }

   city = (char *) roadmap_dialog_get_data ("Address", "City:");

   count = roadmap_geocode_address (&selections,
                                    street_number_image,
                                    street_name,
                                    city,
                                    state);
   if (count <= 0) {
      roadmap_messagebox ("Warning", roadmap_geocode_last_error());
      free (selections);
      return;
   }

   argv[0] = street_number_image;
   argv[1] = street_name;
   argv[2] = city;
   argv[3] = state;

   roadmap_history_add ('A', argv);

   roadmap_dialog_hide (name);

   if (count > 1) {

      /* Open a selection dialog to let the user choose the right block. */

      roadmap_address_selection (context, count, selections);

      for (i = 0; i < count; ++i) {
         free (selections[i].name);
         selections[i].name = NULL;
      }

      context->selections = selections; /* Free these only when done. */

   } else {

      roadmap_address_done (selections);

      free (selections[0].name);
      free (selections);
   }
}


static void roadmap_address_cancel (const char *name, void *data) {

   roadmap_dialog_hide (name);
}


static void roadmap_address_dialog (RoadMapAddressDialog *context) {

   if (roadmap_dialog_activate (context->title, context)) {

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

      roadmap_dialog_complete (roadmap_preferences_use_keyboard());

      roadmap_history_declare ('A', 4);
   }

   context->history = roadmap_history_latest ('A');

   roadmap_address_set (context);
}


void roadmap_address_location_by_city (void) {

   static RoadMapAddressDialog context = {"Location", 0, NULL, NULL};

   roadmap_address_dialog (&context);
}

void roadmap_address_location_by_zip (void) {

   static RoadMapAddressDialog context = {"Location by ZIP", 1, NULL, NULL};

   roadmap_address_dialog (&context);
}

