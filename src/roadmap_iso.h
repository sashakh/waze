/*
 * LICENSE:
 *
 *   Copyright (c) 2008, Danny Backx
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

/**
 * @file
 * @brief Includes for list of country codes from ISO 3166/MA standard, and conversion functions.
 */

int roadmap_iso_alpha_to_num(char *alpha);
void roadmap_iso_create_all_countries(void);
void roadmap_iso_mapfile_from_fips(char *buf, int fips);
int buildmap_osm_filename_iso(char *fn, char *country, char *division, char *suffix);
int roadmap_iso_division_to_num(char *country, char *division);
int buildmap_osm_filename_usc(char *fn, int *fips);

/**
 * @brief structure for iso 3166-1 country codes
 */
typedef struct iso_country {
	char	*name;		/**< official country names */
	int	numeric;	/**< numeric form of iso code */
	char	alpha3[4];	/**< alpha 3 form of iso code */
	char	alpha2[3];	/**< alpha 2 form of iso code */
} iso_country;

extern struct iso_country IsoCountryCodeTable[];
