/* roadmap_address_ssd.c - manage the roadmap address dialogs for small
 *                         screen devices.
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_geocode.h"
#include "roadmap_trip.h"
#include "roadmap_county.h"
#include "roadmap_display.h"
#include "ssd/ssd_keyboard.h"
#include "ssd/ssd_list.h"

#include "roadmap_address.h"

#define MAX_LIST_RESULTS 10

#define MAX_NAMES 100
static int RoadMapAddressSearchCount;
static char *RoadMapAddressSearchNames[MAX_NAMES];
static RoadMapAddressNav RoadMapAddressNavigate;

typedef struct {

    const char *title;
    
    int   use_zip;
    int   navigate;

    RoadMapGeocode *selections;

    void *history;

    char *city_name;
    char *street_name;
    int   range;

} RoadMapAddressDialog;


typedef struct {

   RoadMapAddressSearchCB callback;
   void *data;
   const char *city;
} RoadMapAddressSearch;


static void roadmap_address_done (RoadMapGeocode *selected,
                                  RoadMapAddressDialog *context) {

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

    roadmap_trip_set_point ("Selection", &selected->position);
    roadmap_trip_set_point ("Address", &selected->position);

    if (!context->navigate || !RoadMapAddressNavigate) {
       
       roadmap_trip_set_focus ("Address");

       roadmap_display_activate
          ("Selected Street", &line, &selected->position, &street);
       
       roadmap_screen_refresh ();
    } else {
       if ((*RoadMapAddressNavigate) (&selected->position, &line, 0) != -1) {
       }
    }
}


static void roadmap_address_show (const char *city,
                                  const char *street_name,
                                  const char *street_number_image,
                                  RoadMapAddressDialog *context) {

   int i;
   int count;

   RoadMapGeocode *selections;

   char *state;
   const char *argv[4];

   state         = "IL";

   if (context->use_zip) {
      return; /* TBD: how to select by ZIP ? Need one more table in usdir. */
   }

   count = roadmap_geocode_address (&selections,
                                    street_number_image,
                                    street_name,
                                    city,
                                    state);
   if (count <= 0) {
      roadmap_messagebox (roadmap_lang_get ("Warning"),
                          roadmap_geocode_last_error());
      free (selections);
      return;
   }

   argv[0] = street_number_image;
   argv[1] = street_name;
   argv[2] = city;
   argv[3] = state;

   //roadmap_history_add ('A', argv);

   roadmap_address_done (selections, context);

   for (i = 0; i < count; ++i) {
      free (selections[i].name);
      selections[i].name = NULL;
   }

   free (selections);
}


/*** city / street search ***/
static int roadmap_address_populate_list (RoadMapString index,
                                          const char *string,
                                          void *context) {

   if (RoadMapAddressSearchCount == MAX_NAMES) return 0;

   RoadMapAddressSearchNames[RoadMapAddressSearchCount++] = (char *)string;

   return 1;
}


static void roadmap_address_search_populate (const char *str,
                                             RoadMapAddressSearch *search,
                                             int force) {

   RoadMapAddressSearchCount = 0;

   if (force || strlen(str)) {

      if (search->city) {
         roadmap_street_search (search->city,
                                str,
                                roadmap_address_populate_list,
                                NULL);
      } else {
         roadmap_locator_search_city (str, roadmap_address_populate_list, NULL);
      }
   }
}


static int list_callback (int type, const char *new_value, void *context) {

   RoadMapAddressSearch *search = (RoadMapAddressSearch *)context;

   ssd_keyboard_hide (SSD_KEYBOARD_LETTERS);
   ssd_list_hide ();

   search->callback (new_value, search->data);
   free (search);

   return 1;
}


static int keyboard_callback (int type, const char *new_value, void *context) {
   RoadMapAddressSearch *search = (RoadMapAddressSearch *)context;

   const char *title;

   if (!search->city) {
      title = roadmap_lang_get ("City");
   } else {
      title = roadmap_lang_get ("Street");
   }

   roadmap_address_search_populate (new_value, search,
                                    type == SSD_KEYBOARD_OK);

   if (*new_value && !RoadMapAddressSearchCount) return 0;
   if (!RoadMapAddressSearchCount) return 1;

   if ((RoadMapAddressSearchCount <= MAX_LIST_RESULTS) || 
       (type == SSD_KEYBOARD_OK)) {

      ssd_list_show (title,
                     RoadMapAddressSearchCount,
                     (const char **)RoadMapAddressSearchNames,
                     NULL,
                     list_callback,
                     search);
   }


   return 1;
}


static void roadmap_address_street_result (const char *result, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *)data;
   char name[255];
   char *tmp;

   if ((result == NULL) || !strlen (result)) return;

   strncpy (name, result, sizeof(name));
   name [sizeof(name) - 1] = 0;

   tmp = strrchr (name, ',');
   if (!tmp) return;

   *tmp = 0;
   tmp += 2;

   roadmap_address_show (tmp, name, "", context);
}


static void roadmap_address_city_result (const char *result, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *)data;
   context->city_name = strdup(result);

   roadmap_address_search_dialog
      (context->city_name, roadmap_address_street_result, context);
}


static void roadmap_address_dialog (RoadMapAddressDialog *context) {

   roadmap_address_search_dialog
      (NULL, roadmap_address_city_result, context);
}


void roadmap_address_search_dialog (const char *city,
                                    RoadMapAddressSearchCB callback,
                                    void *data) {

   const char *title;
   RoadMapAddressSearch *context =
      (RoadMapAddressSearch *) malloc (sizeof(*context));

   context->callback = callback;
   context->data = data;
   context->city = city;

   if (!context->city) {
      title = roadmap_lang_get ("City");
   } else {
      title = roadmap_lang_get ("Street");
   }

   ssd_keyboard_show (SSD_KEYBOARD_LETTERS,
                      title,
                      "",
                      keyboard_callback,
                      context);

#if 0
   if (roadmap_dialog_activate ("Search Address", context)) {

      roadmap_dialog_new_entry  (".search", "Name",
                                 roadmap_address_search_populate);
      roadmap_dialog_new_label  (".search", "found");

      roadmap_dialog_new_list   (".search", ".results");

      roadmap_dialog_add_button ("Cancel", roadmap_address_search_cancel);
      roadmap_dialog_add_button ("Done", roadmap_address_search_done);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard()); /* No need for a keyboard. */
   }

   roadmap_dialog_set_data  (".search", "Name", "");
   roadmap_dialog_set_data  (".search", "found", "");
   roadmap_dialog_set_focus (".search", "Name");

   roadmap_address_search_populate ("Search Address", context);
#endif   
}


void roadmap_address_location_by_city (void) {

   static RoadMapAddressDialog context = {"Location", 0, 0, NULL, NULL,
                                           NULL, NULL, 0};

   roadmap_address_dialog (&context);
}

void roadmap_address_location_by_zip (void) {

   static RoadMapAddressDialog context = {"Location by ZIP", 1, 0,
                                           NULL, NULL, NULL, NULL, 0};

   roadmap_address_dialog (&context);
}

void roadmap_address_register_nav (RoadMapAddressNav navigate) {
   RoadMapAddressNavigate = navigate;
}

