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

#include "buildmap_layer.h"

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

// shapefile column names for RNF

#define F_FID           "FID"
#define F_NGD_ID        "NGD_ID"
#define F_CLASS         "CLASS"
#define F_NAME          "NAME"
#define F_TYPE          "TYPE"
#define F_DIRECTION     "DIRECTION"
#define F_ADDR_FM_LE    "ADDR_FM_LE"
#define F_ADDR_TO_LE    "ADDR_TO_LE"
#define F_ADDR_FM_RG    "ADDR_FM_RG"
#define F_ADDR_TO_RG    "ADDR_TO_RG"

// shapefile colum names for DCW

#define F_EXS       "EXS"
#define F_MED       "MED"
#define F_ACC       "ACC"
#define F_RTT       "RTT"
#define F_ID        "ID"

// shapefile column names for US state boundaries
#define F_AREA         "AREA"
#define F_PERIMETER    "PERIMETER"
#define F_ST99_D00_    "ST99_D00_"
#define F_ST99_D00_ID  "ST99_D00_I"
#define F_STATE        "STATE"
#define F_NAME         "NAME"
#define F_LSAD         "LSAD"
#define F_REGION       "REGION"
#define F_DIVISION     "DIVISION"
#define F_LSAD_TRANS   "LSAD_TRANS"   

// shapefile column names for Canadian provincial boundaries
#define F_UUID         "UUID"
#define F_TYPE_E       "TYPE_E"
#define F_NAME         "NAME"
#define F_SRC_AGENCY   "SRC_AGENCY"
#define F_L_UPD_DATE   "L_UPD_DATE"
#define F_L_UPD_TYPE   "L_UPD_TYPE"
#define F_P_UPD_DATE   "P_UPD_DATE"

static BuildMapDictionary DictionaryPrefix;
static BuildMapDictionary DictionaryStreet;
static BuildMapDictionary DictionaryType;
static BuildMapDictionary DictionarySuffix;
static BuildMapDictionary DictionaryCity;
static BuildMapDictionary DictionaryFSA;

static BuildMapDictionary DictionaryBounds;


/* Road layers. */

static int BuildMapLayerFreeway = 0;
static int BuildMapLayerRamp = 0;
static int BuildMapLayerMain = 0;
static int BuildMapLayerStreet = 0;
static int BuildMapLayerTrail = 0;

/* Water layers. */

static int BuildMapLayerShoreline = 0;
static int BuildMapLayerRiver = 0;
static int BuildMapLayerCanal = 0;
static int BuildMapLayerLake = 0;
static int BuildMapLayerSea = 0;

static int BuildMapLayerBoundary = 0;

static void buildmap_shapefile_find_layers (void) {

   BuildMapLayerFreeway   = buildmap_layer_get ("freeways");
   BuildMapLayerRamp      = buildmap_layer_get ("ramps");
   BuildMapLayerMain      = buildmap_layer_get ("highways");
   BuildMapLayerStreet    = buildmap_layer_get ("streets");
   BuildMapLayerTrail     = buildmap_layer_get ("trails");

   BuildMapLayerShoreline = buildmap_layer_get ("shore");
   BuildMapLayerRiver     = buildmap_layer_get ("rivers");
   BuildMapLayerCanal     = buildmap_layer_get ("canals");
   BuildMapLayerLake      = buildmap_layer_get ("lakes");
   BuildMapLayerSea       = buildmap_layer_get ("sea");

   BuildMapLayerBoundary = buildmap_layer_get ("boundaries");
}


static RoadMapString str2dict (BuildMapDictionary d, const char *string) {

   if (!strlen(string)) {
      return buildmap_dictionary_add (d, "", 0);
   }

   return buildmap_dictionary_add (d, (char *) string, strlen(string));
}


/* This function returns the roadmap type for a line.
 * If the value is 0, the line will be ignored.
 */
static char shapefile2type_dmti (char cfcc, int carto) {

   switch (cfcc) {

      case 'R': /* DMTI roads. */

         switch (carto) {

            case 1:  return BuildMapLayerFreeway;
            case 2:  return BuildMapLayerMain;
            case 3:  return BuildMapLayerMain;
            case 4:  return BuildMapLayerMain;
            case 5:  return BuildMapLayerStreet;
            case 6:  return BuildMapLayerTrail;
            default: return BuildMapLayerRamp;
         }
         break;

      case 'H': /* Rivers, lakes and sea. */

         switch (carto) {

            case '0': return BuildMapLayerShoreline;
            case '1': return BuildMapLayerRiver;
            case '2': return BuildMapLayerCanal;
            case '5': return BuildMapLayerSea;
         }
         break;
   }
   return 0;
}

static char shapefile2type_rnf (char cfcc, const char *class) {

   if (!strcmp(class, "UTR")) { //  Utility roads
      return BuildMapLayerTrail;
   } else if (!strcmp(class, "CON")) { //  Connector roads
      return BuildMapLayerMain;
   } else if (!strcmp(class, "UR")) { //  Unclassified roads
      return BuildMapLayerStreet;
   } else if (!strcmp(class, "ST")) { //  Streets
      return BuildMapLayerStreet;
   } else if (!strcmp(class, "HI")) { //  Highways
      return BuildMapLayerFreeway;
   } else if (!strcmp(class, "BT")) { //  Bridges and tunnels
      return BuildMapLayerMain;
   }

   return 0;
}

static char shapefile2type_dcw (char cfcc, int carto) {

   switch (cfcc) {

      case 'D': /* DCW Roads. */

         switch (carto) {

            case  0:  return BuildMapLayerStreet;
            case 14:  return BuildMapLayerFreeway;
            case 15:  return BuildMapLayerMain;
            default:  return BuildMapLayerTrail;
         }
         break;

      case 'H': /* Rivers, lakes and sea. */

         switch (carto) {

            case '0': return BuildMapLayerShoreline;
            case '1': return BuildMapLayerRiver;
            case '2': return BuildMapLayerCanal;
            case '5': return BuildMapLayerSea;
         }
         break;
   }
   return 0;
}

static char shapefile2type_states (char cfcc) {

   return BuildMapLayerBoundary;
}

static char shapefile2type_provinces (char cfcc) {

   return BuildMapLayerBoundary;
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
      unsigned int fradd,
      unsigned int toadd) {

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
      while (*p) free(*p++);
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
   while (*c) free(*c++);
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
   unsigned int fraddl;
   unsigned int toaddl;
   unsigned int fraddr;
   unsigned int toaddr;
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

      cfcc   = shapefile2type_dmti
                ('R', DBFReadIntegerAttribute(hDBF, irec, iCARTO));
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

         fraddl = (unsigned int) DBFReadIntegerAttribute(hDBF, irec, iFRADDL);
         toaddl = (unsigned int) DBFReadIntegerAttribute(hDBF, irec, iTOADDL);

         fraddr = (unsigned int) DBFReadIntegerAttribute(hDBF, irec, iFRADDR);
         toaddr = (unsigned int) DBFReadIntegerAttribute(hDBF, irec, iTOADDR);

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

         line = buildmap_line_add (tlid, cfcc, from_point, to_point,
			 ROADMAP_LINE_DIRECTION_BOTH);

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

            unsigned int fradd;
            unsigned int toadd;

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

      cfcc   = shapefile2type_dmti
                ('R', DBFReadIntegerAttribute(hDBF, irec, iCARTO));

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
                     buildmap_shape_add(line_index, irec, tlid, j-1, lon, lat);
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

/******************************** RNF ***************************************/

static void buildmap_shapefile_read_rnf_rte (const char *source, int verbose) {

   int    irec;
   int    record_count;
   int    merged_range_count;

   const char  *munl, *munr, *mafl, *mafr;

   int line;
   int line_index;
   int street;

   char cfcc;
   int  tlid;
   RoadMapString dirp;
   RoadMapString dirs;
   RoadMapString name;
   RoadMapString type;
   unsigned int fraddl;
   unsigned int toaddl;
   unsigned int fraddr;
   unsigned int toaddr;
   int frlong;
   int frlat;
   int tolong;
   int tolat;
   int from_point;
   int to_point;
   int j, lat, lon;

   RoadMapZip zip = 0;
   int zipl, zipr;
   int merged_range;
   int diff_fr;
   int diff_to;

   int iCLASS, iFRADDL, iTOADDL, iFRADDR, iTOADDR;
   int iDIR, iTYPE, iNAME, iUID;

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
   // strcat (full_name, "rte");

   buildmap_set_source(full_name);

   hDBF = DBFOpen(full_name, "rb");
   hSHP = SHPOpen(full_name, "rb");

   iCLASS     = DBFGetFieldIndex(hDBF, F_CLASS);
   iFRADDL    = DBFGetFieldIndex(hDBF, F_ADDR_FM_LE);
   iTOADDL    = DBFGetFieldIndex(hDBF, F_ADDR_TO_LE);
   iFRADDR    = DBFGetFieldIndex(hDBF, F_ADDR_FM_RG);
   iTOADDR    = DBFGetFieldIndex(hDBF, F_ADDR_TO_RG);
   iDIR       = DBFGetFieldIndex(hDBF, F_DIRECTION);
   iTYPE      = DBFGetFieldIndex(hDBF, F_TYPE);
   iNAME     = DBFGetFieldIndex(hDBF, F_NAME);
   iUID       = DBFGetFieldIndex(hDBF, F_NGD_ID);


   record_count = DBFGetRecordCount(hDBF);
   merged_range_count = 0;

   dirp = str2dict (DictionaryPrefix, "");

   for (irec=0; irec<record_count; irec++) {

      buildmap_set_line (irec);

      cfcc   = shapefile2type_rnf('R', DBFReadStringAttribute(hDBF, irec, iCLASS));
      tlid   = DBFReadIntegerAttribute(hDBF, irec, iUID);

      if (cfcc > 0) {

         dirs = str2dict (DictionarySuffix,
           DBFReadStringAttribute(hDBF, irec, iDIR));
         name = str2dict (DictionaryStreet,
           DBFReadStringAttribute(hDBF, irec, iNAME));
         type = str2dict (DictionaryType,
           DBFReadStringAttribute(hDBF, irec, iTYPE));

         fraddl = (unsigned int) DBFReadIntegerAttribute(hDBF, irec, iFRADDL);
         toaddl = (unsigned int) DBFReadIntegerAttribute(hDBF, irec, iTOADDL);

         fraddr = (unsigned int) DBFReadIntegerAttribute(hDBF, irec, iFRADDR);
         toaddr = (unsigned int) DBFReadIntegerAttribute(hDBF, irec, iTOADDR);

         shp = SHPReadObject(hSHP, irec);

         frlong = shp->padfX[0] * 1000000.0;
         frlat  = shp->padfY[0] * 1000000.0;

         tolong = shp->padfX[shp->nVertices-1] * 1000000.0;
         tolat  = shp->padfY[shp->nVertices-1] * 1000000.0;

         from_point = buildmap_point_add (frlong, frlat);
         to_point   = buildmap_point_add (tolong, tolat);

         line = buildmap_line_add (tlid, cfcc, from_point, to_point,
			 ROADMAP_LINE_DIRECTION_BOTH);

         SHPDestroyObject(shp);

         street = buildmap_street_add
                        (cfcc, dirp, name, type, dirs, line);

         /* Check if the two sides of the street can be merged into
          * a single range (same city/place, same zip, and same range
          * beside the even/odd difference).
          */
         zipl = zipr = 1;  // no zips in RNF file
         munl = munr = "?";  // no zips in RNF file
         mafl = mafr = "?";  // no zips in RNF file
         merged_range = 0;
         diff_fr = fraddr - fraddl;
         diff_to = toaddr - toaddl;

         if ((zipl == zipr) &&
             !strcmp(munl, munr) && !strcmp(mafl, mafr) &&
             (abs(diff_fr) < 10) &&
             (abs(diff_to) < 10) &&
             (fraddl != 0 || toaddl != 0 || fraddr != 0 || toaddr != 0)) {

            unsigned int fradd;
            unsigned int toadd;

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

      cfcc   = shapefile2type_rnf
                ('R', DBFReadStringAttribute(hDBF, irec, iCLASS));

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
                     buildmap_shape_add(line_index, irec, tlid, j-1, lon, lat);
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
   char *full_name;

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

   full_name = malloc(strlen(source) + 4);
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
      cfcc   = shapefile2type_dcw('D', rtt);
      tlid   = DBFReadIntegerAttribute(hDBF, irec, iUID);

      if (cfcc > 0) {

         shp = SHPReadObject(hSHP, irec);

         frlong = shp->padfX[0] * 1000000.0;
         frlat  = shp->padfY[0] * 1000000.0;

         tolong = shp->padfX[shp->nVertices-1] * 1000000.0;
         tolat  = shp->padfY[shp->nVertices-1] * 1000000.0;

         from_point = buildmap_point_add (frlong, frlat);
         to_point   = buildmap_point_add (tolong, tolat);

         line = buildmap_line_add (tlid, cfcc, from_point, to_point,
			 ROADMAP_LINE_DIRECTION_BOTH);

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
      cfcc   = shapefile2type_dcw('D', rtt);

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
                     buildmap_shape_add(line_index, irec, tlid, j-1, lon, lat);
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

/******************************** state boundaries **************************/

/* 0 == all */
int which_states;

/* fips codes for alaska, hawaii, and wyoming (the last continental) */
#define AK 02
#define HI 15
#define WY 56

void buildmap_shapefile_set_states(int which) {

    which_states = which;

}

static int exclude_state(int state) {
      if (which_states == JUST_CONTINENTAL) {
         if (state == AK || state == HI || state > WY)
            return 1;
      } else if (which_states == JUST_AK) {
         if (state != AK)
            return 1;
      } else if (which_states == JUST_HI) {
         if (state != HI)
            return 1;
      }
      return 0;
}

static void buildmap_shapefile_read_state (const char *source, int verbose) {

   int    irec;
   int    record_count;

   int line;
   int line_index;
   int tlid, cfcc;

   int frlong;
   int frlat;
   int tolong;
   int tolat;
   int from_point;
   int to_point;
   int j, lat, lon;
   char *full_name;
   int state;

   int iUID;
   int iSTATE;

   DBFHandle hDBF;
   SHPHandle hSHP;
   SHPObject *shp;

   DictionaryBounds = buildmap_dictionary_open ("boundaries");

   full_name = malloc(strlen(source) + 4);
   roadmap_check_allocated(full_name);

   strcpy (full_name, source);
   buildmap_set_source(full_name);

   hDBF = DBFOpen(full_name, "rb");
   hSHP = SHPOpen(full_name, "rb");

   iUID       = DBFGetFieldIndex(hDBF, F_ST99_D00_ID);
   iSTATE     = DBFGetFieldIndex(hDBF, F_STATE);

   record_count = DBFGetRecordCount(hDBF);

   for (irec=0; irec<record_count; irec++) {

      buildmap_set_line (irec);

      state = DBFReadIntegerAttribute(hDBF, irec, iSTATE);
      if (exclude_state(state))
         continue;
       
      cfcc   = shapefile2type_states('D');
      tlid   = DBFReadIntegerAttribute(hDBF, irec, iUID);

      if (cfcc > 0) {

         shp = SHPReadObject(hSHP, irec);

         if (shp->padfX && shp->padfY) {
             frlong = shp->padfX[0] * 1000000.0;
             frlat  = shp->padfY[0] * 1000000.0;

             tolong = shp->padfX[shp->nVertices-1] * 1000000.0;
             tolat  = shp->padfY[shp->nVertices-1] * 1000000.0;

             from_point = buildmap_point_add (frlong, frlat);
             to_point   = buildmap_point_add (tolong, tolat);

             line = buildmap_line_add (tlid, cfcc, from_point, to_point,
			     ROADMAP_LINE_DIRECTION_BOTH);
         }

         SHPDestroyObject(shp);

      }

      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }
   }

   buildmap_info("loading shape info ...");

   for (irec=0; irec<record_count; irec++) {

      buildmap_set_line (irec);

      state = DBFReadIntegerAttribute(hDBF, irec, iSTATE);
      if (exclude_state(state))
         continue;
       
      shp = SHPReadObject(hSHP, irec);

      for (j=1; j<shp->nVertices-1; j++) {
         if (shp->padfX && shp->padfY) {
          lon = shp->padfX[j] * 1000000.0;
          if (lon != 0) {
              lat = shp->padfY[j] * 1000000.0;
              buildmap_square_adjust_limits(lon, lat);
          }
         }
      }

      SHPDestroyObject(shp);


      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }
   }

   buildmap_line_sort();

   for (irec=0; irec<record_count; irec++) {

      buildmap_set_line (irec);

      state = DBFReadIntegerAttribute(hDBF, irec, iSTATE);
      if (exclude_state(state))
         continue;
       
      shp = SHPReadObject(hSHP, irec);

      tlid = DBFReadIntegerAttribute(hDBF, irec, iUID);
      line_index = buildmap_line_find_sorted(tlid);
 
      if (line_index >= 0) {

         // Add the shape points here

         for (j=1; j<shp->nVertices-1; j++) {
             if (shp->padfX && shp->padfY) {
                 lon = shp->padfX[j] * 1000000.0;
                 if (lon != 0) {
                     lat = shp->padfY[j] * 1000000.0;
                     buildmap_shape_add(line_index, irec, tlid, j-1, lon, lat);
                 }
             }
         }
      }

      SHPDestroyObject(shp);

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

/******************************** provincial boundaries **********************/

static void buildmap_shapefile_read_province (const char *source, int verbose) {

   int    irec;
   int    record_count;

   int line;
   int line_index;
   int tlid, cfcc;

   int frlong;
   int frlat;
   int tolong;
   int tolat;
   int from_point;
   int to_point;
   int j, lat, lon;
   char *full_name;
   const char *type;

   int iUID, iTYPE;

   DBFHandle hDBF;
   SHPHandle hSHP;
   SHPObject *shp;

   DictionaryBounds = buildmap_dictionary_open ("boundaries");

   full_name = malloc(strlen(source) + 4);
   roadmap_check_allocated(full_name);

   strcpy (full_name, source);
   buildmap_set_source(full_name);

   hDBF = DBFOpen(full_name, "rb");
   hSHP = SHPOpen(full_name, "rb");

   iUID       = DBFGetFieldIndex(hDBF, F_UUID);
   iTYPE      = DBFGetFieldIndex(hDBF, F_TYPE_E);

   record_count = DBFGetRecordCount(hDBF);

   for (irec=0; irec<record_count; irec++) {

      buildmap_set_line (irec);

      cfcc   = shapefile2type_provinces('D');
      type = DBFReadStringAttribute(hDBF, irec, iTYPE);

      if (strcmp(type, "TERR") == 0)
         continue;  /* provinces only.  no roads up there anyways.  ;-) */

      tlid   = DBFReadIntegerAttribute(hDBF, irec, iUID);

      if (cfcc > 0) {

         shp = SHPReadObject(hSHP, irec);

         if (shp->padfX && shp->padfY) {
             frlong = shp->padfX[0] * 1000000.0;
             frlat  = shp->padfY[0] * 1000000.0;

             tolong = shp->padfX[shp->nVertices-1] * 1000000.0;
             tolat  = shp->padfY[shp->nVertices-1] * 1000000.0;

             from_point = buildmap_point_add (frlong, frlat);
             to_point   = buildmap_point_add (tolong, tolat);

             line = buildmap_line_add (tlid, cfcc, from_point, to_point,
			     ROADMAP_LINE_DIRECTION_BOTH);
         }

         SHPDestroyObject(shp);

      }

      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }
   }

   buildmap_info("loading shape info ...");

   for (irec=0; irec<record_count; irec++) {

      type = DBFReadStringAttribute(hDBF, irec, iTYPE);

      if (strcmp(type, "TERR") == 0)
         continue;  /* provinces only.  no roads up there anyways.  ;-) */

      buildmap_set_line (irec);

      shp = SHPReadObject(hSHP, irec);

      for (j=1; j<shp->nVertices-1; j++) {
         if (shp->padfX && shp->padfY) {
            lon = shp->padfX[j] * 1000000.0;
            if (lon != 0) {
               lat = shp->padfY[j] * 1000000.0;
               buildmap_square_adjust_limits(lon, lat);
            }
         }
      }

      SHPDestroyObject(shp);


      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }
   }

   buildmap_line_sort();

   for (irec=0; irec<record_count; irec++) {

      type = DBFReadStringAttribute(hDBF, irec, iTYPE);

      if (strcmp(type, "TERR") == 0)
         continue;  /* provinces only.  no roads up there anyways.  ;-) */

      buildmap_set_line (irec);

      shp = SHPReadObject(hSHP, irec);

      tlid = DBFReadIntegerAttribute(hDBF, irec, iUID);
      line_index = buildmap_line_find_sorted(tlid);
 
      if (line_index >= 0) {

         // Add the shape points here

         for (j=1; j<shp->nVertices-1; j++) {
             if (shp->padfX && shp->padfY) {
                 lon = shp->padfX[j] * 1000000.0;
                 if (lon != 0) {
                     lat = shp->padfY[j] * 1000000.0;
                     buildmap_shape_add(line_index, irec, tlid, j-1, lon, lat);
                 }
             }
         }
      }

      SHPDestroyObject(shp);

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


void buildmap_shapefile_dmti_process (const char *source,
                                 const char *county,
                                 int verbose) {

   char *base = roadmap_path_remove_extension (source);

   buildmap_shapefile_find_layers ();

   buildmap_shapefile_read_rte(base, verbose);
   buildmap_shapefile_read_hyl(base, verbose);
   buildmap_shapefile_read_hyr(base, verbose);

   free (base);

}

/* Canadian Road Network File */
void buildmap_shapefile_rnf_process (const char *source,
                                 const char *county,
                                 int verbose) {

   char *base = roadmap_path_remove_extension (source);

   buildmap_shapefile_find_layers ();

   buildmap_shapefile_read_rnf_rte(base, verbose);
   buildmap_shapefile_read_hyl(base, verbose);
   buildmap_shapefile_read_hyr(base, verbose);

   free (base);

}

/* Digital Charts of the World */
void buildmap_shapefile_dcw_process (const char *source,
                                     const char *county,
                                     int verbose) {

   char *base = roadmap_path_remove_extension (source);

   buildmap_shapefile_find_layers ();

   buildmap_shapefile_read_dcw_roads(base, verbose);
   buildmap_shapefile_read_dcw_hyl(base, verbose);
   buildmap_shapefile_read_dcw_hyr(base, verbose);

   free (base);

}

/* US State boundaries */
void buildmap_shapefile_state_process (const char *source,
                                     const char *county,
                                     int verbose) {

   char *base = roadmap_path_remove_extension (source);

   buildmap_shapefile_find_layers ();

   buildmap_shapefile_read_state(base, verbose);

   free (base);

}

/* Canada Provincial boundaries */
void buildmap_shapefile_province_process (const char *source,
                                     const char *county,
                                     int verbose) {

   char *base = roadmap_path_remove_extension (source);

   buildmap_shapefile_find_layers ();

   buildmap_shapefile_read_province(base, verbose);

   free (base);

}

#endif // ROADMAP_USE_SHAPEFILES

