/* rdmxchange.h - general definitions use by the rdmxchange program.
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
 */

#ifndef INCLUDED__RDMXCHANGE__H
#define INCLUDED__RDMXCHANGE__H

#include <stdio.h>

#include "roadmap.h"
#include "roadmap_dbread.h"

#include "buildmap.h"


/* The map export side. ----------------------------------------------- */

typedef void (*RdmXchangeExporter) (FILE *output);

typedef struct rdmxchange_export_record {

   const char *type;

   RdmXchangeExporter head;
   RdmXchangeExporter data;

} RdmXchangeExport;


void rdmxchange_main_register_export (const RdmXchangeExport *export);


extern roadmap_db_handler RoadMapDictionaryExport;
extern roadmap_db_handler RoadMapIndexExport;
extern roadmap_db_handler RoadMapLineExport;
extern roadmap_db_handler RoadMapMetadataExport;
extern roadmap_db_handler RoadMapPointExport;
extern roadmap_db_handler RoadMapPolygonExport;
extern roadmap_db_handler RoadMapShapeExport;
extern roadmap_db_handler RoadMapSquareExport;
extern roadmap_db_handler RoadMapStreetExport;
extern roadmap_db_handler RoadMapZipExport;
extern roadmap_db_handler RoadMapRangeExport;


/* The map import side. ----------------------------------------------- */

short rdmxchange_import_short (const char *data);
int   rdmxchange_import_int   (const char *data);


typedef void (*RdmXchangeTableImporter)  (const char *table, int count);
typedef int  (*RdmXchangeSchemaImporter) (const char *table,
                                          char *fields[], int count);
typedef void (*RdmXchangeDataImporter)   (int table,
                                          char *fields[], int count);

typedef struct {

   const char *type;

   RdmXchangeTableImporter  table;
   RdmXchangeSchemaImporter schema;
   RdmXchangeDataImporter   data;

} RdmXchangeImport;


extern RdmXchangeImport RdmXchangeZipImport;
extern RdmXchangeImport RdmXchangeStreetImport;
extern RdmXchangeImport RdmXchangeRangeImport;
extern RdmXchangeImport RdmXchangePolygonImport;
extern RdmXchangeImport RdmXchangeShapeImport;
extern RdmXchangeImport RdmXchangeLineImport;
extern RdmXchangeImport RdmXchangePointImport;
extern RdmXchangeImport RdmXchangeSquareImport;
extern RdmXchangeImport RdmXchangeIndexImport;
extern RdmXchangeImport RdmXchangeMetadataImport;
extern RdmXchangeImport RdmXchangeDictionaryImport;

#endif // INCLUDED__RDMXCHANGE__H

