/* buildmap_shapefile.c - a module to read shapefiles.
 *
 * LICENSE:
 *
 *   Copyright 2004 Stephen Woodbridge
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
 *   see buildmap_shapefile.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#if ROADMAP_USE_SHAPEFILES

#include <shapefil.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_path.h"

#include "buildmap.h"
#include "buildmap_zip.h"
#include "buildmap_shapefile.h"
#include "buildmap_city.h"
#include "buildmap_square.h"
#include "buildmap_point.h"
#include "buildmap_line.h"
#include "buildmap_street.h"
#include "buildmap_range.h"
#include "buildmap_area.h"
#include "buildmap_shape.h"
#include "buildmap_polygon.h"

// shapfile column names for DMTI

#define F_CARTO     "CARTO"
#define F_FRADDL    "FROMLEFT"
#define F_TOADDL    "TOLEFT"
#define F_FRADDR    "FROMRIGHT"
#define F_TOADDR    "TORIGHT"
#define F_FSAL      "LEFT_FSA"
#define F_FSAR      "RIGHT_FSA"
#define F_FEDIRP    "PREDIR"
#define F_FETYPEP   "PRETYPE"
#define F_FENAME    "STREETNAME"
#define F_FETYPES   "SUFTYPE"
#define F_FEDIRS    "SUFDIR"
#define F_LEFT_MUN  "LEFT_MUN"
#define F_RIGHT_MUN "RIGHT_MUN"
#define F_LEFT_MAF  "LEFT_MAF"
#define F_RIGHT_MAF "RIGHT_MAF"
#define F_UID       "UNIQUEID"

// shapefile colum names for DCW

#define F_EXS       "EXS"
#define F_MED       "MED"
#define F_ACC       "ACC"
#define F_RTT       "RTT"
#define F_ID        "ID"


static BuildMapDictionary DictionaryPrefix;
static BuildMapDictionary DictionaryStreet;
static BuildMapDictionary DictionaryType;
static BuildMapDictionary DictionarySuffix;
static BuildMapDictionary DictionaryCity;
static BuildMapDictionary DictionaryFSA;

static int BuildMapCanals = ROADMAP_WATER_RIVER;
static int BuildMapRivers = ROADMAP_WATER_RIVER;


static RoadMapString
str2dict (BuildMapDictionary d, const char *string) {

   if (!strlen(string)) {
      return buildmap_dictionary_add (d, "", 0);
   }

   return buildmap_dictionary_add (d, (char *) string, strlen(string));
}


/* This function returns the roadmap type for a line.
 * If the value is 0, the line will be ignored.
 */
static char shapefile2type (char cfcc, int carto) {

   switch (cfcc) {

      case 'R': /* DMTI roads. */

         switch (carto) {

            case 1:  return ROADMAP_ROAD_FREEWAY;
            case 2:  return ROADMAP_ROAD_MAIN;
            case 3:  return ROADMAP_ROAD_MAIN;
            case 4:  return ROADMAP_ROAD_MAIN;
            case 5:  return ROADMAP_ROAD_STREET;
            case 6:  return ROADMAP_ROAD_TRAIL;
            default: return ROADMAP_ROAD_RAMP;
         }
         break;

      case 'D': /* DCW Roads. */

         switch (carto) {

            case  0:  return ROADMAP_ROAD_STREET;
            case 14:  return ROADMAP_ROAD_FREEWAY;
            case 15:  return ROADMAP_ROAD_MAIN;
            default:  return ROADMAP_ROAD_TRAIL;
         }
         break;

      case 'H': /* Rivers, lakes and sea. */

         switch (carto) {

            case '0': return ROADMAP_WATER_SHORELINE;
            case '1': return BuildMapRivers;
            case '2': return BuildMapCanals;
            case '5': return ROADMAP_WATER_SEA;
         }
         break;
   }
   return 0;
}


static void shapefile_summary (int verbose, int count) {

   buildmap_summary (verbose, "%d records", count);
}


static char **buildmap_clean_split_string (const char *string) {

   const char *ss;
   const char *s;
   char **results;
   char buf[128];
   int cnt;
   int len;
   int i;

   i = 0;
   results = (char **) calloc(11, sizeof(char *));
   if (!results) {
      fprintf(stderr, "buildmap_clean_split_string(): memory allocations error\n");
      exit(1);
   }

   ss = s = string;
   while (*s != '\0' && i < 11) {
      if      (!strncasecmp(s, " and ", 5)) cnt = 5;
      else if (!strncasecmp(s, ", ", 2))    cnt = 2;
      else if (*s == '-' || *s == ',')      cnt = 1;
      else if (*(s+1) == '\0')              cnt = 0;
      else {
         s++;
	 continue;
      }
      if (!cnt) s++;
      len = (s-ss < 128)?s-ss:127;
      strncpy(buf, ss, len);
      buf[len] = '\0';
      results[i++] = strdup(buf);
      s = s + cnt;
      ss = s;
   }
   return results;
}


static void buildmap_range_add_address (

      const char *mun,
      const char *maf,
      RoadMapZip  zip,
      int line,
      int street,
      int fradd,
      int toadd) {

   RoadMapString city;
   RoadMapString place;

   char **cities, **c;
   char **places, **p;
   
   cities = buildmap_clean_split_string(mun);
   if (strcmp(mun, maf)) {
      places = buildmap_clean_split_string(maf);
      p = places;
      while (*p) {
         c = cities;
         while (*c) {
            city  = str2dict(DictionaryCity, *c);
            place = str2dict(DictionaryCity, *p);
            buildmap_range_add_place(place, city);
            city = place;

            buildmap_range_add (line, street, fradd, toadd, zip, city);
            c++;
         }
         p++;
      }
      // free up memory used by places
      p = places;
      while (*p) { free(*p); p++; }
      free(places);
   } else {
      c = cities;
      while (*c) {
         RoadMapString city;

         city  = str2dict(DictionaryCity, *c);
         buildmap_range_add (line, street, fradd, toadd, zip, city);
         c++;
      }
   }
   // free up memory used by cities
   c = cities;
   while (*c) { free(*c); c++; }
   free(cities);
}


static void buildmap_shapefile_read_rte (const char *source, int verbose) {

   int    irec;
   int    record_count;
   int    merged_range_count;

   const char  *munl, *munr, *mafl, *mafr;

   int line;
   int line_index;
   int street;
   RoadMapZip zip = 0;

   char cfcc;
   int  tlid;
   RoadMapString fedirp;
   RoadMapString fename;
   RoadMapString fetype;
   RoadMapString fedirs;
   int fraddl;
   int toaddl;
   int fraddr;
   int toaddr;
   int zipl;
   int zipr;
   int frlong;
   int frlat;
   int tolong;
   int tolat;
   int from_point;
   int to_point;
   int j, lat, lon;

   int merged_range;
   int diff_fr;
   int diff_to;
   
   int iCARTO, iFRADDL, iTOADDL, iFRADDR, iTOADDR, iZIPL, iZIPR;
   int iFEDIRP, iFETYPEP, iFENAME, iFETYPES, iFEDIRS;
   int iLEFT_MUN, iRIGHT_MUN, iLEFT_MAF, iRIGHT_MAF, iUID;
   
   DBFHandle hDBF;
   SHPHandle hSHP;
   SHPObject *shp;

   char *full_name = malloc(strlen(source) + 4);

   DictionaryPrefix = buildmap_dictionary_open ("prefix");
   DictionaryStreet = buildmap_dictionary_open ("street");
   DictionaryType   = buildmap_dictionary_open ("type");
   DictionarySuffix = buildmap_dictionary_open ("suffix");
   DictionaryCity   = buildmap_dictionary_open ("city");
   DictionaryFSA    = buildmap_dictionary_open ("fsa");

   roadmap_check_allocated(full_name);

   strcpy (full_name, source);
   strcat (full_name, "rte");

   buildmap_set_source(full_name);

   hDBF = DBFOpen(full_name, "rb");
   hSHP = SHPOpen(full_name, "rb");

   iCARTO     = DBFGetFieldIndex(hDBF, F_CARTO);
   iFRADDL    = DBFGetFieldIndex(hDBF, F_FRADDL);
   iTOADDL    = DBFGetFieldIndex(hDBF, F_TOADDL);
   iFRADDR    = DBFGetFieldIndex(hDBF, F_FRADDR);
   iTOADDR    = DBFGetFieldIndex(hDBF, F_TOADDR);
   iZIPL      = DBFGetFieldIndex(hDBF, F_FSAL);
   iZIPR      = DBFGetFieldIndex(hDBF, F_FSAR);
   iFEDIRP    = DBFGetFieldIndex(hDBF, F_FEDIRP);
   iFETYPEP   = DBFGetFieldIndex(hDBF, F_FETYPEP);
   iFENAME    = DBFGetFieldIndex(hDBF, F_FENAME);
   iFETYPES   = DBFGetFieldIndex(hDBF, F_FETYPES);
   iFEDIRS    = DBFGetFieldIndex(hDBF, F_FEDIRS);
   iLEFT_MUN  = DBFGetFieldIndex(hDBF, F_LEFT_MUN);
   iRIGHT_MUN = DBFGetFieldIndex(hDBF, F_RIGHT_MUN);
   iLEFT_MAF  = DBFGetFieldIndex(hDBF, F_LEFT_MAF);
   iRIGHT_MAF = DBFGetFieldIndex(hDBF, F_RIGHT_MAF);
   iUID       = DBFGetFieldIndex(hDBF, F_UID);
   
   
   record_count = DBFGetRecordCount(hDBF);
   merged_range_count = 0;

   for (irec=0; irec<record_count; irec++) {

      buildmap_set_line (irec);

      cfcc   = shapefile2type('R', DBFReadIntegerAttribute(hDBF, irec, iCARTO));
      tlid   = DBFReadIntegerAttribute(hDBF, irec, iUID);

      if (cfcc > 0) {

         fedirp = str2dict (DictionaryPrefix, 
           DBFReadStringAttribute(hDBF, irec, iFEDIRP));
         fename = str2dict (DictionaryStreet,
           DBFReadStringAttribute(hDBF, irec, iFENAME));
         fetype = str2dict (DictionaryType,
           DBFReadStringAttribute(hDBF, irec, iFETYPES));
         fedirs = str2dict (DictionarySuffix,
           DBFReadStringAttribute(hDBF, irec, iFEDIRS));

         fraddl = DBFReadIntegerAttribute(hDBF, irec, iFRADDL);
         toaddl = DBFReadIntegerAttribute(hDBF, irec, iTOADDL);

         fraddr = DBFReadIntegerAttribute(hDBF, irec, iFRADDR);
         toaddr = DBFReadIntegerAttribute(hDBF, irec, iTOADDR);

         zipl   = str2dict (DictionaryFSA,
           DBFReadStringAttribute(hDBF, irec, iZIPL));
         zipr   = str2dict (DictionaryFSA,
           DBFReadStringAttribute(hDBF, irec, iZIPR));

         shp = SHPReadObject(hSHP, irec);
	 
         frlong = shp->padfX[0] * 1000000.0;
         frlat  = shp->padfY[0] * 1000000.0;

         tolong = shp->padfX[shp->nVertices-1] * 1000000.0;
         tolat  = shp->padfY[shp->nVertices-1] * 1000000.0;

         from_point = buildmap_point_add (frlong, frlat);
         to_point   = buildmap_point_add (tolong, tolat);

         line = buildmap_line_add (tlid, cfcc, from_point, to_point);

         SHPDestroyObject(shp);

         street = buildmap_street_add
                        (cfcc, fedirp, fename, fetype, fedirs, line);

         /* Check if the two sides of the street can be merged into
          * a single range (same city/place, same zip, and same range
          * beside the even/odd difference).
          */
         merged_range = 0;
         diff_fr = fraddr - fraddl;
         diff_to = toaddr - toaddl;

         munl = DBFReadStringAttribute(hDBF, irec, iLEFT_MUN);
         munr = DBFReadStringAttribute(hDBF, irec, iRIGHT_MUN);
         mafl = DBFReadStringAttribute(hDBF, irec, iLEFT_MAF);
         mafr = DBFReadStringAttribute(hDBF, irec, iRIGHT_MAF);

         if ((zipl == zipr) &&
             !strcmp(munl, munr) && !strcmp(mafl, mafr) &&
             (diff_fr > -10) && (diff_fr < 10) &&
             (diff_to > -10) && (diff_to < 10)) {

            int fradd;
            int toadd;

            buildmap_range_merge (fraddl, toaddl,
                                  fraddr, toaddr,
                                  &fradd, &toadd);

            zip = buildmap_zip_add (zipl, frlong, frlat);

            if (fradd == 0 && toadd == 0)  {
                buildmap_range_add_no_address (line, street);
                continue;
            } else {
               buildmap_range_add_address
                  (munl, mafl, zip, line, street, fradd, toadd);

               merged_range = 1;
               merged_range_count += 1;
            }
         }

         if (! merged_range) {

            zip = buildmap_zip_add (zipl, frlong, frlat);

            if (fraddl == 0 && toaddl == 0) 
                buildmap_range_add_no_address (line, street);
            else
                buildmap_range_add_address
                   (munl, mafl, zip, line, street, fraddl, toaddl);

            if (zipr != zipl)
               zip = buildmap_zip_add (zipr, frlong, frlat);

            if (fraddr == 0 && toaddr == 0) {
               if (fraddl != 0 || toaddl != 0) {
                  buildmap_range_add_no_address (line, street);
               }
            } else {
                buildmap_range_add_address
                   (munr, mafr, zip, line, street, fraddr, toaddr);
            }
         }
      }

      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }
   }

   buildmap_info("loading shape info ...");

   buildmap_line_sort();

   for (irec=0; irec<record_count; irec++) {

      buildmap_set_line (irec);

      cfcc   = shapefile2type('R', DBFReadIntegerAttribute(hDBF, irec, iCARTO));

      if (cfcc > 0) {

         shp = SHPReadObject(hSHP, irec);

         tlid = DBFReadIntegerAttribute(hDBF, irec, iUID);
         line_index = buildmap_line_find_sorted(tlid);

         if (line_index >= 0) {
	  
             // Add the shape points here

             for (j=1; j<shp->nVertices-1; j++) {
                 lon = shp->padfX[j] * 1000000.0;
                 if (lon != 0) {
                     lat = shp->padfY[j] * 1000000.0;
                     buildmap_shape_add(line_index, j-1, lon, lat);
                 }
             }
         }

         SHPDestroyObject(shp);

      }

      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }
   }

   DBFClose(hDBF);
   SHPClose(hSHP);

   shapefile_summary (verbose, record_count);
   if (merged_range_count > 0) {
      buildmap_summary (verbose, "%d ranges merged", merged_range_count);
   }

   free(full_name);
}


static void buildmap_shapefile_read_hyl (const char *source, int verbose) {

}


static void buildmap_shapefile_read_hyr (const char *source, int verbose) {

}


/******************************** DCW ***************************************/

static void buildmap_shapefile_read_dcw_roads (const char *source, int verbose) {

   int    irec;
   int    record_count;

   int line;
   int line_index;
   int street;
   int tlid, cfcc, rtt, exs;

   RoadMapString fedirp;
   RoadMapString fename;
   RoadMapString fetype;
   RoadMapString fedirs;

   int frlong;
   int frlat;
   int tolong;
   int tolat;
   int from_point;
   int to_point;
   int j, lat, lon;

   int iEXS, iMED, iACC, iRTT, iUID;
   
   DBFHandle hDBF;
   SHPHandle hSHP;
   SHPObject *shp;

   DictionaryPrefix = buildmap_dictionary_open ("prefix");
   DictionaryStreet = buildmap_dictionary_open ("street");
   DictionaryType   = buildmap_dictionary_open ("type");
   DictionarySuffix = buildmap_dictionary_open ("suffix");
   DictionaryCity   = buildmap_dictionary_open ("city");
   DictionaryFSA    = buildmap_dictionary_open ("fsa");

   char *full_name = malloc(strlen(source) + 4);
   roadmap_check_allocated(full_name);

   strcpy (full_name, source);
   buildmap_set_source(full_name);

   hDBF = DBFOpen(full_name, "rb");
   hSHP = SHPOpen(full_name, "rb");

   /***************************************
    * EXS - road status
    *    2 - doubtful
    *    5 - under construction
    *   28 - operational
    *   55 - unexamined/unsurveyed
    * MED - median
    *    0 - unknown
    *    1 - with median
    *    2 - without median
    * ACC - accuracy
    *    0 - unknown
    *    1 - accurate
    *    2 - approximate
    * RTT - route type
    *    0 - unknown
    *   14 - Primary route
    *   15 - Secondary route
    ****************************************/
        
   iEXS       = DBFGetFieldIndex(hDBF, F_EXS);
   iMED       = DBFGetFieldIndex(hDBF, F_MED);
   iACC       = DBFGetFieldIndex(hDBF, F_ACC);
   iRTT       = DBFGetFieldIndex(hDBF, F_RTT);
   iUID       = DBFGetFieldIndex(hDBF, F_ID);
   
   
   record_count = DBFGetRecordCount(hDBF);

   fedirp = str2dict (DictionaryPrefix, "");
   fename = str2dict (DictionaryStreet, "unknown");
   fetype = str2dict (DictionaryType,   "");
   fedirs = str2dict (DictionarySuffix, "");

   for (irec=0; irec<record_count; irec++) {

      buildmap_set_line (irec);

      rtt    = DBFReadIntegerAttribute(hDBF, irec, iRTT);
      exs    = DBFReadIntegerAttribute(hDBF, irec, iEXS);
      if (exs < 10) rtt = exs;
      cfcc   = shapefile2type('D', rtt);
      tlid   = DBFReadIntegerAttribute(hDBF, irec, iUID);

      if (cfcc > 0) {

         shp = SHPReadObject(hSHP, irec);
   
         frlong = shp->padfX[0] * 1000000.0;
         frlat  = shp->padfY[0] * 1000000.0;

         tolong = shp->padfX[shp->nVertices-1] * 1000000.0;
         tolat  = shp->padfY[shp->nVertices-1] * 1000000.0;

         from_point = buildmap_point_add (frlong, frlat);
         to_point   = buildmap_point_add (tolong, tolat);

         line = buildmap_line_add (tlid, cfcc, from_point, to_point);

         SHPDestroyObject(shp);

         street = buildmap_street_add
                        (cfcc, fedirp, fename, fetype, fedirs, line);
         
         buildmap_range_add_no_address (line, street);
      }

      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }
   }

   buildmap_info("loading shape info ...");

   buildmap_line_sort();

   for (irec=0; irec<record_count; irec++) {

      buildmap_set_line (irec);

      rtt    = DBFReadIntegerAttribute(hDBF, irec, iRTT);
      exs    = DBFReadIntegerAttribute(hDBF, irec, iEXS);
      if (exs < 10) rtt = exs;
      cfcc   = shapefile2type('D', rtt);

      if (cfcc > 0) {

         shp = SHPReadObject(hSHP, irec);

         tlid = DBFReadIntegerAttribute(hDBF, irec, iUID);
         line_index = buildmap_line_find_sorted(tlid);

         if (line_index >= 0) {
    
             // Add the shape points here

             for (j=1; j<shp->nVertices-1; j++) {
                 lon = shp->padfX[j] * 1000000.0;
                 if (lon != 0) {
                     lat = shp->padfY[j] * 1000000.0;
                     buildmap_shape_add(line_index, j-1, lon, lat);
                 }
             }
         }

         SHPDestroyObject(shp);
      }

      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }
   }

   DBFClose(hDBF);
   SHPClose(hSHP);

   shapefile_summary (verbose, record_count);

   free(full_name);
}


static void buildmap_shapefile_read_dcw_hyl (const char *source, int verbose) {

}


static void buildmap_shapefile_read_dcw_hyr (const char *source, int verbose) {

}

#endif // ROADMAP_USE_SHAPEFILES


void buildmap_shapefile_process (const char *source,
                                 int verbose, int canals, int rivers) {

#if ROADMAP_USE_SHAPEFILES

   char *base = roadmap_path_remove_extension (source);
   char *basename = roadmap_path_skip_directories (base);

   /* Remove the "file type", if any, to keep only the province's name. */
   if (strlen(basename) > 2) basename[2] = 0;

   if (! canals) {
      BuildMapCanals = 0;
   }

   if (! rivers) {
      BuildMapRivers = 0;
   }

   buildmap_shapefile_read_rte(base, verbose);
   buildmap_shapefile_read_hyl(base, verbose);
   buildmap_shapefile_read_hyr(base, verbose);

   free (base);

#else

   fprintf (stderr,
            "cannot process %s: built with no shapefile support.\n",
            source);
   exit(1);
#endif // ROADMAP_USE_SHAPEFILES
}


void buildmap_shapefile_dcw_process (const char *source,
                                     int verbose, int canals, int rivers) {

#if ROADMAP_USE_SHAPEFILES

   char *base = roadmap_path_remove_extension (source);

   if (! canals) {
      BuildMapCanals = 0;
   }

   if (! rivers) {
      BuildMapRivers = 0;
   }

   buildmap_shapefile_read_dcw_roads(base, verbose);
   buildmap_shapefile_read_dcw_hyl(base, verbose);
   buildmap_shapefile_read_dcw_hyr(base, verbose);

   free (base);

#else

   fprintf (stderr,
            "cannot process %s: built with no shapefile support.\n",
            source);
   exit(1);
#endif // ROADMAP_USE_SHAPEFILES
}

