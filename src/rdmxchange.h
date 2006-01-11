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

#include "roadmap_dbread.h"


typedef void (*RdmXchangeExporter) (FILE *output);

typedef struct rdmxchange_export_record {

   const char *type;

   RdmXchangeExporter head;
   RdmXchangeExporter data;

} RdmXchangeExport;


void rdmxchange_main_register_export (RdmXchangeExport *export);


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

#endif // INCLUDED__RDMXCHANGE__H

