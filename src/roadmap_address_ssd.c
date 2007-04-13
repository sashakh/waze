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
   char *prefix;
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


static int roadmap_address_show (const char *city,
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
      return 0; /* TBD: how to select by ZIP ? Need one more table in usdir. */
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
      return 0;
   }

   argv[0] = street_number_image;
   argv[1] = street_name;
   argv[2] = city;
   argv[3] = state;

   roadmap_history_add ('A', argv);

   roadmap_address_done (selections, context);

   for (i = 0; i < count; ++i) {
      free (selections[i].name);
      selections[i].name = NULL;
   }

   free (selections);

   return 1;
}


/*** city / street search ***/
static int roadmap_address_populate_list (RoadMapString index,
                                          const char *string,
                                          void *context) {

   if (RoadMapAddressSearchCount == MAX_NAMES) return 0;

   RoadMapAddressSearchNames[RoadMapAddressSearchCount++] = (char *)string;

   return 1;
}


static int roadmap_address_search_count (const char *str,
                                         RoadMapAddressSearch *search) {
                                         
   int count;

   if (search->city) {
      count = roadmap_street_search (search->city, str, NULL, NULL);
   } else {
      count = roadmap_locator_search_city (str, NULL, NULL);
   }

   return count;
}


static void roadmap_address_search_populate (const char *str,
                                             RoadMapAddressSearch *search) {

   RoadMapAddressSearchCount = 0;

   if (search->city) {
           roadmap_street_search (search->city,
                           str,
                           roadmap_address_populate_list,
                           NULL);
   } else {
           roadmap_locator_search_city (str, roadmap_address_populate_list, NULL);
   }
}


static int list_callback (int type, const char *new_value, void *context) {

   RoadMapAddressSearch *search = (RoadMapAddressSearch *)context;

   if ((strlen(new_value) > 3) &&
       !strcmp(new_value + strlen(new_value) - 3, " - ")) {

       char str[255];

       ssd_list_hide ();

       strcpy(str, new_value);
       str[strlen(str) - 2] = '\0';

       ssd_dialog_set_value ("input", str);

       str[strlen(str) - 1] = '\0';
       search->prefix = strdup(str);

       return 1;
   }

   ssd_keyboard_hide ();

   search->callback (new_value, search->data);
   free (search);

   ssd_list_hide ();

   return 1;
}


static int cmpstring(const void *p1, const void *p2) {
   return strcmp(* (char * const *) p1, * (char * const *) p2);
}

static int keyboard_callback (int type, const char *new_value, void *context) {
   RoadMapAddressSearch *search = (RoadMapAddressSearch *)context;

   const char *title;
   int count;

   if (!search->city) {
      if (!*new_value && (type == SSD_KEYBOARD_OK)) {
         search->callback ("", search->data);
         free (search);
         return 1;
      }
      title = roadmap_lang_get ("City");
   } else {
      title = roadmap_lang_get ("Street");
      if (search->prefix && !strcmp(search->prefix, new_value)) {
         ssd_dialog_set_value ("input", "");

         free (search->prefix);
         search->prefix = NULL;
         return 0;
      }
   }

   count = roadmap_address_search_count (new_value, search);

   if (*new_value && !count) return 0;
   if (!count) return 1;

   if ((count <= MAX_LIST_RESULTS) || 
       (type == SSD_KEYBOARD_OK)) {

      roadmap_address_search_populate (new_value, search);

      qsort (RoadMapAddressSearchNames, RoadMapAddressSearchCount,
             sizeof(RoadMapAddressSearchNames[0]), cmpstring);

      ssd_list_show (title,
                     RoadMapAddressSearchCount,
                     (const char **)RoadMapAddressSearchNames,
                     NULL,
                     list_callback,
                     search);
   }


   return 1;
}


static int house_keyboard_callback (int type, const char *new_value,
                                    void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *)data;

   if (type == SSD_KEYBOARD_EXTRA) {
      context->navigate = 1;
      if (roadmap_address_show (context->city_name, context->street_name,
                                new_value, context)) {
         ssd_keyboard_hide ();
      }
      context->navigate = 0;
   }

   if (type == SSD_KEYBOARD_OK) {
      if (roadmap_address_show (context->city_name, context->street_name,
                                new_value, context)) {
         ssd_keyboard_hide ();
      }
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
   if (tmp) {
      *tmp = 0;
      tmp += 2;

      if (*tmp && !*context->city_name) {
         free (context->city_name);
         context->city_name = strdup (tmp);
      }
   }

   if (context->street_name) free(context->street_name);
   context->street_name = strdup(name);

   ssd_keyboard_show (SSD_KEYBOARD_DIGITS,
                      roadmap_lang_get ("House number"),
                      "",
                      roadmap_lang_get ("Navigate"),
                      house_keyboard_callback,
                      context);
}


static void roadmap_address_city_result (const char *result, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *)data;
   if (context->city_name) free(context->city_name);
   context->city_name = strdup(result);

   roadmap_address_search_dialog
      (context->city_name, roadmap_address_street_result, context);
}


static void roadmap_address_dialog (RoadMapAddressDialog *context) {

   roadmap_address_search_dialog
      (NULL, roadmap_address_city_result, context);
}


static int history_callback (int type, const char *new_value, void *data) {
   RoadMapAddressDialog *context = (RoadMapAddressDialog *)data;

   if (!new_value[0] || !strcmp(new_value, roadmap_lang_get ("New address"))) {
      roadmap_address_search_dialog
         (NULL, roadmap_address_city_result, data);

   } else if (!strchr(new_value, ',')) {
      /* We only got a city */
      if (context->city_name) free(context->city_name);
      context->city_name = strdup (new_value);
      roadmap_address_search_dialog
         (context->city_name, roadmap_address_street_result, data);
   } else {
      /* We got a full address */
      char *argv[4];

      roadmap_history_get ('A', (void *) ssd_dialog_get_data ("list"), argv);

      context->navigate = 1;
      if (context->city_name) free(context->city_name);
      if (context->street_name) free(context->street_name);
      context->city_name = strdup (argv[2]);
      context->street_name = strdup (argv[1]);

      if (roadmap_address_show (context->city_name, context->street_name,
                                argv[0], context)) {
         ssd_list_hide ();
      }
      context->navigate = 0;

      return 1;
   }

   ssd_list_hide ();

   return 1;
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
   context->prefix = NULL;

   if (!context->city) {
      title = roadmap_lang_get ("City");
   } else {
      title = roadmap_lang_get ("Street");
   }

   ssd_keyboard_show (SSD_KEYBOARD_LETTERS,
                      title,
                      "",
                      NULL,
                      keyboard_callback,
                      context);
}


void roadmap_address_history (void) {

#define MAX_HISTORY_ENTRIES 100
   static char *labels[MAX_HISTORY_ENTRIES];
   static void *values[MAX_HISTORY_ENTRIES];
   static int count = -1;
   void *history;
   static RoadMapAddressDialog context = {"Location", 0, 0, NULL, NULL,
                                           NULL, NULL, 0};

   if (count == -1) {
      roadmap_history_declare ('A', 4);
      labels[0] = (char *)roadmap_lang_get ("New address");
   }

   history = roadmap_history_latest ('A');

   count = 1;

   while (history && (count < MAX_HISTORY_ENTRIES)) {
      void *prev = history;

      char *argv[4];
      char str[100];

      roadmap_history_get ('A', history, argv);

      if (count == 1) {
         /* Add special city entry */
         if (labels[1]) free (labels[1]);
         labels[1] = strdup (argv[2]);
         count++;
      }

      snprintf (str, sizeof(str), "%s %s, %s", argv[1], argv[0], argv[2]);

      if (labels[count]) free (labels[count]);
      labels[count] = strdup(str);

      values[count] = history;

      count++;
      history = roadmap_history_before ('A', history);
      if (history == prev) break;
   }
      
   ssd_list_show (roadmap_lang_get ("History"),
                  count,
                  (const char **)labels,
                  (const void **)values,
                  history_callback,
                  &context);
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

