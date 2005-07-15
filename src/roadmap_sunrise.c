/* roadmap_sunrise.c - calculate sunrise/sunset time
 *
 * license:
 *
 *   copyright 2003 eric domazlicky
 *   copyright 2003 pascal martin (changed module interface)
 *
 *   this file is part of roadmap.
 *
 *   roadmap is free software; you can redistribute it and/or modify
 *   it under the terms of the gnu general public license as published by
 *   the free software foundation; either version 2 of the license, or
 *   (at your option) any later version.
 *
 *   roadmap is distributed in the hope that it will be useful,
 *   but without any warranty; without even the implied warranty of
 *   merchantability or fitness for a particular purpose.  see the
 *   gnu general public license for more details.
 *
 *   you should have received a copy of the gnu general public license
 *   along with roadmap; if not, write to the free software
 *   foundation, inc., 59 temple place, suite 330, boston, ma  02111-1307  usa
 *
 * synopsys:
 *
 *   see roadmap_sunrise.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "roadmap_sunrise.h"

#define PI                  3.14159265358979323846
#define DEGREES             (180 / PI)
#define RADIANS             (PI / 180)
#define LU_PER_DEG          1000000     /* file lat/lon units per degree */
#define DEG_TO_LU(deg)      ( (long) ((deg) * LU_PER_DEG) )
#define LU_TO_DEG(lu)       ( (float) (lu) / LU_PER_DEG )

#define ROADMAP_SUNRISE 1
#define ROADMAP_SUNSET -1


/* returns an angle in range of 0 to (2 * PI) */

static double roadmap_sunrise_getrange(double x) {

   double temp1,temp2;

   temp1 = x / (2 * PI);
   temp2 = (2 * PI) * (temp1 - floor(temp1));
   if(temp2<0) {
       temp2 = (2*PI) + temp2;
   }

   return(temp2);
}


static time_t roadmap_sunrise_get_gmt(double decimaltime,
                                      struct tm *curtime) {

   time_t gmt;
#ifndef _WIN32
   char   *tz;
#endif

   double temp1;
   int temp2;

   if (decimaltime < 0)  {
      decimaltime += 24;
   }
   if (decimaltime > 24) {
      decimaltime = decimaltime - 24;
   }
   temp1 = fabs(decimaltime);
   temp2 = (int)(floor(temp1));
   temp1 = 60 * (temp1 - floor(temp1));

   /* fill in the tm structure */
   curtime->tm_hour = temp2;
   curtime->tm_min = (int) temp1;

#ifndef _WIN32
   tz = getenv("TZ");
   if (tz == NULL || tz[0] != 0) {
      setenv("TZ", "", 1);
      tzset();
   }
#endif

   gmt = mktime(curtime);

#ifndef _WIN32
   if (tz != NULL) {
      if (tz[0] != 0) {
         setenv("TZ", tz, 1);
      }
   } else {
      unsetenv("TZ");
   }
   tzset();
#endif

   return gmt;
}


/* Calling parameters:
 * A roadmapposition for the point you'd like to get the rise or set time for
 * A timezone/gmt offset in integer form. example: cst is -6. some provisions
 * need to be made for daylight savings time.
 * riseorset: use the constants ROADMAP_SUNRISE or ROADMAP_SUNSET.
 * the caller must have set at least the current date in the tm structure:
 * - on success this structure's members tm_hour and tm_min will be set to
 *   the time of the sunrise or sunset.
 * - on failure the function returns 0.
 *
 * Notes: this function won't work for latitudes at 63 and above and -63 and
 * below
 */
static time_t roadmap_sunrise_getriseorset
                 (const RoadMapGpsPosition *position, int riseorset) {

   time_t gmt;
   time_t result;
   struct tm curtime;
   struct tm curtime_gmt;

   double sinalt,sinphi,cosphi;
   double longitude,l,gx,gha,correction,ec,lambda,delta,e,obl,cosc;
   double utold,utnew;
   double days,t;
   int count = 0;
   int ephem2000day;
   double roadmap_lat;

   /* not accurate for places too close to the poles */
   if(abs(position->latitude) > 63000000) {
       return -1;
   }

   /* check for valid riseorset parameter */
   if((riseorset!=1)&&(riseorset!=-1)) {
       return -1;
   }

   time(&gmt);
   curtime = *(localtime(&gmt));
   curtime_gmt = *(gmtime(&gmt));


   ephem2000day = 367 * (curtime.tm_year+1900)
                      - 7 * ((curtime.tm_year+1900)
                                  + ((curtime.tm_mon+1) + 9) / 12) / 4
                      + 275 * (curtime.tm_mon+1) / 9
                      + curtime.tm_mday - 730531;
   utold = PI;
   utnew = 0;
   
   sinalt = -0.0174456; /* tbd: sets according to position->altitude. */

   roadmap_lat  = LU_TO_DEG(position->latitude);
   sinphi = sin(roadmap_lat * RADIANS);
   cosphi = cos(roadmap_lat * RADIANS);
   longitude = LU_TO_DEG(position->longitude) * RADIANS;

   while((fabs(utold-utnew) > 0.001) && (count < 35))  {

      count++;
      utold=utnew;
      days = (1.0 * ephem2000day - 0.5) + utold / (2 * PI);
      t = days / 36525;
      l  = roadmap_sunrise_getrange(4.8949504201433 + 628.331969753199 * t);
      gx = roadmap_sunrise_getrange(6.2400408 + 628.3019501 * t);
      ec = .033423 * sin(gx) + .00034907 * sin(2. * gx);
      lambda = l + ec;
      e = -1 * ec + .0430398 * sin(2. * lambda) - .00092502 * sin(4. * lambda);
      obl = .409093 - .0002269 * t;

      delta = sin(obl) * sin(lambda);
      delta = atan(delta / (sqrt(1 - delta * delta)));

      gha = utold - PI + e;
      cosc = (sinalt - sinphi * sin(delta)) / (cosphi * cos(delta));
      if(cosc > 1) {
         correction = 0;
      } else if(cosc < -1) {
         correction = PI;
      } else {
         correction = atan((sqrt(1 - cosc * cosc)) / cosc);
      }

      utnew = roadmap_sunrise_getrange
                  (utold - (gha + longitude + riseorset * correction));
   }

   /* utnew must now be converted into gmt. */

   result = roadmap_sunrise_get_gmt (utnew * DEGREES / 15, &curtime_gmt);

   /* I don't know why, but the time seems shifted by 12 hours.
    * This is not a fix: this is a quick & dirty hack.
    */
   result -= (12 * 3600);

   if (result < gmt) {
      result += (24 * 3600);
   }

   return result;
}


time_t roadmap_sunrise (const RoadMapGpsPosition *position) {

   static time_t validity = 0;
   static time_t sunrise = 0;

   time_t now = time(NULL);

   if (validity < now) {

      sunrise = roadmap_sunrise_getriseorset (position, ROADMAP_SUNRISE);

      if (sunrise < now) {

         /* This happened already: wait for the next sunrise. */

         validity = roadmap_sunrise_getriseorset (position, ROADMAP_SUNSET);

         if (validity < now) {
            /* We want the next sunset, not the last one. */
            validity += (24 * 3600);
         }

      } else if (now + 3600 < sunrise) {

         /* The next sunrise is in more than one hour: recompute every 15mn. */
         validity = now + 900;

      } else {

         /* The next sunrise is quite soon: recompute it more frequently as
          * we get closer to the deadline.
          */
         validity = (sunrise + now) / 4;

         if (validity < now + 60) {
            /* Do not recompute the sunrise every second. */
            validity = now + 60;
         }
      }
   }

   return sunrise;
}

time_t roadmap_sunset (const RoadMapGpsPosition *position) {

   static time_t validity = 0;
   static time_t sunset = 0;

   time_t now = time(NULL);

   if (now > validity) {

      sunset = roadmap_sunrise_getriseorset (position, ROADMAP_SUNSET);

      if (sunset < now) {

         /* This happened already: wait for the next sunset. */

         validity = roadmap_sunrise_getriseorset (position, ROADMAP_SUNRISE);

         if (validity < now) {
            /* We want the next sunrise, not the last one. */
            validity += (24 * 3600);
         }

      } else if (now + 3600 < sunset) {

         /* The next sunset is in more than one hour: recompute every 15mn. */
         validity = now + 900;

      } else {

         /* The next sunset is quite soon: recompute it more frequently as
          * we get closer to the deadline.
          */
         validity = (sunset + now) / 4;

         if (validity < now + 60) {
            /* Do not recompute the sunset every second. */
            validity = now + 60;
         }
      }
   }

   return sunset;
}


#ifdef SUNRISE_PROGRAM

int main(int argc, char **argv) {

   time_t rawtime;
   struct tm *realtime;
   RoadMapGpsPosition position;

   if (argc > 3) {
      setenv("TZ", argv[3], 1);
      tzset();
   } else if (argc != 3) {
      fprintf (stderr, "Usage: %s longitude latitude [timezone]\n"
                       "missing position\n", argv[0]);
      exit(1);
   }
   position.longitude = atoi(argv[1]);
   position.latitude = atoi(argv[2]);

   time(&rawtime);
   realtime = localtime(&rawtime);

   printf("current date/time: %s", asctime(realtime));

   rawtime = roadmap_sunrise (&position);
   realtime = localtime(&rawtime);

   printf("sunrise: %d:%d\n", realtime->tm_hour, realtime->tm_min);

   rawtime = roadmap_sunset (&position);
   realtime = localtime(&rawtime);

   printf("sunset:  %d:%d\n", realtime->tm_hour, realtime->tm_min);
   return 0;
}
#endif /* SUNRISE_PROGRAM */

