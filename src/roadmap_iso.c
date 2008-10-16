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
 * @brief List of country codes from ISO 3166/MA standard, and conversion functions.
 *
 * List obtained from http://en.wikipedia.org/wiki/ISO_3166-1 .
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "buildmap.h"
#include "buildus_county.h"
#include "roadmap.h"
#include "roadmap_iso.h"

/**
 * @brief table for iso 3166-1 country codes
 *
 * This would be static but we need to access it from roadmap_iso_create_all_countries(),
 * and that function calls a bunch of buildmap_* functions so it would cause link "issues"
 * in RoadMap if it would be in this source file.
 */
struct iso_country IsoCountryCodeTable[] = {
 { "Afghanistan",					4, 	"AFG", "AF" },
 { "Åland Islands",					248, 	"ALA", "AX" },
 { "Albania",						8, 	"ALB", "AL" },
 { "Algeria",						12, 	"DZA", "DZ" },
 { "American Samoa",					16, 	"ASM", "AS" },
 { "Andorra",						20, 	"AND", "AD" },
 { "Angola",						24, 	"AGO", "AO" },
 { "Anguilla",						660, 	"AIA", "AI" },
 { "Antarctica",					10, 	"ATA", "AQ" },
 { "Antigua and Barbuda",				28, 	"ATG", "AG" },
 { "Argentina",						32, 	"ARG", "AR" },
 { "Armenia",						51, 	"ARM", "AM" },
 { "Aruba",						533, 	"ABW", "AW" },
 { "Australia",						36, 	"AUS", "AU" },
 { "Austria",						40, 	"AUT", "AT" },
 { "Azerbaijan",					31, 	"AZE", "AZ" },
 { "Bahamas",						44, 	"BHS", "BS" },
 { "Bahrain",						48, 	"BHR", "BH" },
 { "Bangladesh",					50, 	"BGD", "BD" },
 { "Barbados",						52, 	"BRB", "BB" },
 { "Belarus",						112, 	"BLR", "BY" },
 { "Belgium",						56, 	"BEL", "BE" },
 { "Belize",						84, 	"BLZ", "BZ" },
 { "Benin",						204, 	"BEN", "BJ" },
 { "Bermuda",						60, 	"BMU", "BM" },
 { "Bhutan",						64, 	"BTN", "BT" },
 { "Bolivia",						68, 	"BOL", "BO" },
 { "Bosnia and Herzegovina",				70, 	"BIH", "BA" },
 { "Botswana",						72, 	"BWA", "BW" },
 { "Norway Bouvet Island",				74, 	"BVT", "BV" },
 { "Brazil",						76, 	"BRA", "BR" },
 { "British Indian Ocean Territory",			86, 	"IOT", "IO" },
 { "Brunei Darussalam",					96, 	"BRN", "BN" },
 { "Bulgaria",						100, 	"BGR", "BG" },
 { "Burkina Faso",					854, 	"BFA", "BF" },
 { "Burundi",						108, 	"BDI", "BI" },
 { "Cambodia",						116, 	"KHM", "KH" },
 { "Cameroon",						120, 	"CMR", "CM" },
 { "Canada",						124, 	"CAN", "CA" },
 { "Cape Verde",					132, 	"CPV", "CV" },
 { "Cayman Islands",					136, 	"CYM", "KY" },
 { "Central African Republic",				140, 	"CAF", "CF" },
 { "Chad",						148, 	"TCD", "TD" },
 { "Chile",						152, 	"CHL", "CL" },
 { "China",						156, 	"CHN", "CN" },
 { "Christmas Island",					162, 	"CXR", "CX" },
 { "Cocos (Keeling) Islands",				166, 	"CCK", "CC" },
 { "Colombia",						170, 	"COL", "CO" },
 { "Comoros",						174, 	"COM", "KM" },
 { "Congo",						178, 	"COG", "CG" },
 { "Democratic Republic of the Congo",			180, 	"COD", "CD" },
 { "Cook Islands",					184, 	"COK", "CK" },
 { "Costa Rica",					188, 	"CRI", "CR" },
 { "Côte d'Ivoire",					384, 	"CIV", "CI" },
 { "Croatia",						191, 	"HRV", "HR" },
 { "Cuba",						192, 	"CUB", "CU" },
 { "Cyprus",						196, 	"CYP", "CY" },
 { "Czech Republic",					203, 	"CZE", "CZ" },
 { "Denmark",						208, 	"DNK", "DK" },
 { "Djibouti",						262, 	"DJI", "DJ" },
 { "Dominica",						212, 	"DMA", "DM" },
 { "Dominican Republic",				214, 	"DOM", "DO" },
 { "Ecuador",						218, 	"ECU", "EC" },
 { "Egypt",						818, 	"EGY", "EG" },
 { "El Salvador",					222, 	"SLV", "SV" },
 { "Equatorial Guinea",					226, 	"GNQ", "GQ" },
 { "Eritrea",						232, 	"ERI", "ER" },
 { "Estonia",						233, 	"EST", "EE" },
 { "Ethiopia",						231, 	"ETH", "ET" },
 { "Falkland Islands (Malvinas)",			238, 	"FLK", "FK" },
 { "Faroe Islands",					234, 	"FRO", "FO" },
 { "Fiji",						242, 	"FJI", "FJ" },
 { "Finland",						246, 	"FIN", "FI" },
 { "France",						250, 	"FRA", "FR" },
 { "French Guiana",					254, 	"GUF", "GF" },
 { "French Polynesia",					258, 	"PYF", "PF" },
 { "French Southern Territories",			260, 	"ATF", "TF" },
 { "Gabon",						266, 	"GAB", "GA" },
 { "Gambia",						270, 	"GMB", "GM" },
 { "Georgia",						268, 	"GEO", "GE" },
 { "Germany",						276, 	"DEU", "DE" },
 { "Ghana",						288, 	"GHA", "GH" },
 { "Gibraltar",						292, 	"GIB", "GI" },
 { "Greece",						300, 	"GRC", "GR" },
 { "Greenland",						304, 	"GRL", "GL" },
 { "Grenada",						308, 	"GRD", "GD" },
 { "Guadeloupe",					312, 	"GLP", "GP" },
 { "Guam",						316, 	"GUM", "GU" },
 { "Guatemala",						320, 	"GTM", "GT" },
 { "Guernsey",						831, 	"GGY", "GG" },
 { "Guinea",						324, 	"GIN", "GN" },
 { "Guinea-Bissau",					624, 	"GNB", "GW" },
 { "Guyana",						328, 	"GUY", "GY" },
 { "Haiti",						332, 	"HTI", "HT" },
 { "Australia Heard Island and McDonald Islands",	334, 	"HMD", "HM" },
 { "Vatican City State",				336, 	"VAT", "VA" },
 { "Honduras",						340, 	"HND", "HN" },
 { "Hong Kong",						344, 	"HKG", "HK" },
 { "Hungary",						348, 	"HUN", "HU" },
 { "Iceland",						352, 	"ISL", "IS" },
 { "India",						356, 	"IND", "IN" },
 { "Indonesia",						360, 	"IDN", "ID" },
 { "Islamic Republic of Iran",				364, 	"IRN", "IR" },
 { "Iraq",						368, 	"IRQ", "IQ" },
 { "Ireland",						372, 	"IRL", "IE" },
 { "Isle of Man",					833, 	"IMN", "IM" },
 { "Israel",						376, 	"ISR", "IL" },
 { "Italy",						380, 	"ITA", "IT" },
 { "Jamaica",						388, 	"JAM", "JM" },
 { "Japan",						392, 	"JPN", "JP" },
 { "Jersey",						832, 	"JEY", "JE" },
 { "Jordan",						400, 	"JOR", "JO" },
 { "Kazakhstan",					398, 	"KAZ", "KZ" },
 { "Kenya",						404, 	"KEN", "KE" },
 { "Kiribati",						296, 	"KIR", "KI" },
 { "Democratic People's Republic of Korea",		408, 	"PRK", "KP" },
 { "Republic of Korea",					410, 	"KOR", "KR" },
 { "Kuwait",						414, 	"KWT", "KW" },
 { "Kyrgyzstan",					417, 	"KGZ", "KG" },
 { "Lao People's Democratic Republic",			418, 	"LAO", "LA" },
 { "Latvia",						428, 	"LVA", "LV" },
 { "Lebanon",						422, 	"LBN", "LB" },
 { "Lesotho",						426, 	"LSO", "LS" },
 { "Liberia",						430, 	"LBR", "LR" },
 { "Libyan Arab Jamahiriya",				434, 	"LBY", "LY" },
 { "Liechtenstein",					438, 	"LIE", "LI" },
 { "Lithuania",						440, 	"LTU", "LT" },
 { "Luxembourg",					442, 	"LUX", "LU" },
 { "Macao",						446, 	"MAC", "MO" },
 { "Macedonia, the former Yugoslav Republic of", 	807, 	"MKD", "MK" },
 { "Madagascar",					450, 	"MDG", "MG" },
 { "Malawi",						454, 	"MWI", "MW" },
 { "Malaysia",						458, 	"MYS", "MY" },
 { "Maldives",						462, 	"MDV", "MV" },
 { "Mali",						466, 	"MLI", "ML" },
 { "Malta",						470, 	"MLT", "MT" },
 { "Marshall Islands",					584, 	"MHL", "MH" },
 { "Martinique",					474, 	"MTQ", "MQ" },
 { "Mauritania",					478, 	"MRT", "MR" },
 { "Mauritius",						480, 	"MUS", "MU" },
 { "Mayotte",						175, 	"MYT", "YT" },
 { "Mexico",						484, 	"MEX", "MX" },
 { "Federated States of Micronesia",			583, 	"FSM", "FM" },
 { "Moldova",						498, 	"MDA", "MD" },
 { "Monaco",						492, 	"MCO", "MC" },
 { "Mongolia",						496, 	"MNG", "MN" },
 { "Montenegro",					499, 	"MNE", "ME" },
 { "Montserrat",					500, 	"MSR", "MS" },
 { "Morocco",						504, 	"MAR", "MA" },
 { "Mozambique",					508, 	"MOZ", "MZ" },
 { "Myanmar",						104, 	"MMR", "MM" },
 { "Namibia",						516, 	"NAM", "NA" },
 { "Nauru",						520, 	"NRU", "NR" },
 { "Nepal",						524, 	"NPL", "NP" },
 { "Netherlands",					528, 	"NLD", "NL" },
 { "Netherlands Antilles",				530, 	"ANT", "AN" },
 { "New Caledonia",					540, 	"NCL", "NC" },
 { "New Zealand",					554, 	"NZL", "NZ" },
 { "Nicaragua",						558, 	"NIC", "NI" },
 { "Niger",						562, 	"NER", "NE" },
 { "Nigeria",						566, 	"NGA", "NG" },
 { "Niue",						570, 	"NIU", "NU" },
 { "Norfolk Island",					574, 	"NFK", "NF" },
 { "Northern Mariana Islands",				580, 	"MNP", "MP" },
 { "Norway",						578, 	"NOR", "NO" },
 { "Oman",						512, 	"OMN", "OM" },
 { "Pakistan",						586, 	"PAK", "PK" },
 { "Palau",						585, 	"PLW", "PW" },
 { "Occupied Palestinian Territory",			275, 	"PSE", "PS" },
 { "Panama",						591, 	"PAN", "PA" },
 { "Papua New Guinea",					598, 	"PNG", "PG" },
 { "Paraguay",						600, 	"PRY", "PY" },
 { "Peru",						604, 	"PER", "PE" },
 { "Philippines",					608, 	"PHL", "PH" },
 { "Pitcairn",						612, 	"PCN", "PN" },
 { "Poland",						616, 	"POL", "PL" },
 { "Portugal",						620, 	"PRT", "PT" },
 { "Puerto Rico",					630, 	"PRI", "PR" },
 { "Qatar",						634, 	"QAT", "QA" },
 { "Réunion",						638, 	"REU", "RE" },
 { "Romania",						642, 	"ROU", "RO" },
 { "Russian Federation",				643, 	"RUS", "RU" },
 { "Rwanda",						646, 	"RWA", "RW" },
 { "Saint Barthélemy",					652, 	"BLM", "BL" },
 { "Saint Helena",					654, 	"SHN", "SH" },
 { "Saint Kitts and Nevis",				659, 	"KNA", "KN" },
 { "Saint Lucia",					662, 	"LCA", "LC" },
 { "Saint Martin (French part)",			663, 	"MAF", "MF" },
 { "Saint Pierre and Miquelon",				666, 	"SPM", "PM" },
 { "Saint Vincent and the Grenadines",			670, 	"VCT", "VC" },
 { "Samoa",						882, 	"WSM", "WS" },
 { "San Marino",					674, 	"SMR", "SM" },
 { "Sao Tome and Principe",				678, 	"STP", "ST" },
 { "Saudi Arabia",					682, 	"SAU", "SA" },
 { "Senegal",						686, 	"SEN", "SN" },
 { "Serbia",						688, 	"SRB", "RS" },
 { "Seychelles",					690, 	"SYC", "SC" },
 { "Sierra Leone",					694, 	"SLE", "SL" },
 { "Singapore",						702, 	"SGP", "SG" },
 { "Slovakia",						703, 	"SVK", "SK" },
 { "Slovenia",						705, 	"SVN", "SI" },
 { "Solomon Islands",					90, 	"SLB", "SB" },
 { "Somalia",						706, 	"SOM", "SO" },
 { "South Africa",					710, 	"ZAF", "ZA" },
 { "South Georgia and the South Sandwich Islands",	239, 	"SGS", "GS" },
 { "Spain",						724, 	"ESP", "ES" },
 { "Sri Lanka",						144, 	"LKA", "LK" },
 { "Sudan",						736, 	"SDN", "SD" },
 { "Suriname",						740, 	"SUR", "SR" },
 { "Norway Svalbard and Jan Mayen",			744, 	"SJM", "SJ" },
 { "Swaziland",						748, 	"SWZ", "SZ" },
 { "Sweden",						752, 	"SWE", "SE" },
 { "Switzerland",					756, 	"CHE", "CH" },
 { "Syrian Arab Republic",				760, 	"SYR", "SY" },
 { "Taiwan, Province of China", 			158, 	"TWN", "TW" },
 { "Tajikistan",					762, 	"TJK", "TJ" },
 { "United Republic of Tanzania",			834, 	"TZA", "TZ" },
 { "Thailand",						764, 	"THA", "TH" },
 { "Timor-Leste",					626, 	"TLS", "TL" },
 { "Togo",						768, 	"TGO", "TG" },
 { "Tokelau",						772, 	"TKL", "TK" },
 { "Tonga",						776, 	"TON", "TO" },
 { "Trinidad and Tobago",				780, 	"TTO", "TT" },
 { "Tunisia",						788, 	"TUN", "TN" },
 { "Turkey",						792, 	"TUR", "TR" },
 { "Turkmenistan",					795, 	"TKM", "TM" },
 { "Turks and Caicos Islands",				796, 	"TCA", "TC" },
 { "Tuvalu",						798, 	"TUV", "TV" },
 { "Uganda",						800, 	"UGA", "UG" },
 { "Ukraine",						804, 	"UKR", "UA" },
 { "United Arab Emirates",				784, 	"ARE", "AE" },
 { "United Kingdom",					826, 	"GBR", "GB" },
 { "United States",					840, 	"USA", "US" },
 { "United States Minor Outlying Islands",		581, 	"UMI", "UM" },
 { "Uruguay",						858, 	"URY", "UY" },
 { "Uzbekistan",					860, 	"UZB", "UZ" },
 { "Vanuatu",						548, 	"VUT", "VU" },
 { "Venezuela",						862,	"VEN", "VE" },
 { "Viet Nam",						704, 	"VNM", "VN" },
 { "British Virgin Islands",				 92, 	"VGB", "VG" },
 { "Virgin Islands, U.S.",			 	850,	"VIR", "VI" },
 { "Wallis and Futuna",					876,	"WLF", "WF" },
 { "Western Sahara",					732,	"ESH", "EH" },
 { "Yemen",						887,	"YEM", "YE" },
 { "Zambia",						894,	"ZMB", "ZM" },
 { "Zimbabwe",						716,	"ZWE", "ZW" },
 { 0,							0,	"   ", "  " },
};

/**
 * @brief form a file name for the map for a given fips (actually an encoded country code)
 * @param buf is the buffer to put the file name in
 * @param fips the input parameter
 *
 * FIX ME maybe this function should build a list of possible file names instead of just one
 */
void roadmap_iso_mapfile_from_fips(char *buf, int fips)
{
	int	i, j;
	int	country = (fips / 1000) % 1000;

	for (i=0; IsoCountryCodeTable[i].name; i++)
		if (IsoCountryCodeTable[i].numeric == country) {
			sprintf(buf, "iso-%s.rdm", IsoCountryCodeTable[i].alpha2);
			for (j=0; buf[j]; j++)
				buf[j] = tolower(buf[j]);
			return;
		}
}

/**
 * @brief decode an ISO map file name, figure out whether it's valid
 * @param fn a string containing the file name
 * @param country the country code is returned in this variable
 * @param division the country division code is returned in this variable
 * @param suffix the suffix (e.g. ".osm" for the file name)
 * @return whether this is a valid file name
 *
 * The return parameters must be allocated by the caller.
 *
 * Note : http://en.wikipedia.org/wiki/ISO_3166-2 describes country division codes.
 * Summary : ISO 3166-2 codes consist of two parts, separated by a hyphen.
 * The first part is the ISO 3166-1 alpha-2 code element, the second is
 * alphabetic or numeric and has one, two or three characters.
 * The second part often is based on national standards.
 */
int buildmap_osm_filename_iso(char *fn, char *country, char *division, char *suffix)
{
	int	n;
	char	pattern[32];

	sprintf(pattern, "iso-%%[a-zA-Z]-%%[a-zA-Z0-9]%s", suffix);
	n = sscanf(fn, pattern, country, division);
	if (strlen(country) != 2)
		return 0;
	if (n == 2) {
		if (strlen(division) > 3) {
			division[0] = '\0';
			return 1;
		}
		return 2;
	}
	division[0] = '\0';
	if (n == 1)
		return 1;
	country[0] = '\0';
	return 0;
}

/**
 * @brief Translate a division code into a number in a simplistic way
 * @param country the country code is used to read a file containing divisions
 * @param division this string is looked for in the country division file
 * @return the numeric representation of this country division
 *
 * This function looks up the "division" in a file that is supposed to
 * contain stuff about the "country".
 * The file is named "iso-division-%s [.txt]", and contains one line
 * per entry, each line has a number (this is what we look for),
 * whitespace, and the division code.
 * Example :
 * 	1 bru
 *
 * Note : http://en.wikipedia.org/wiki/ISO_3166-2 describes country division codes.
 * Summary : ISO 3166-2 codes consist of two parts, separated by a hyphen.
 * The first part is the ISO 3166-1 alpha-2 code element, the second is
 * alphabetic or numeric and has one, two or three characters.
 * The second part often is based on national standards.
 */
int roadmap_iso_division_to_num(char *country, char *division)
{
	char	mfn[20], div[16];
	FILE	*f;
	int	num;

	sprintf(mfn, "iso-division-%s" _TXT, country);
	f = fopen(mfn, "r");
	if (! f) {
		/* How to report errors here ? */
		roadmap_log(ROADMAP_WARNING, "Cannot open %s", mfn);
		return 0;
	}

	while (! feof(f)) {
		if (fscanf(f, "%d %s", &num, div) != 2)
			continue;
		if (strcmp(div, division) == 0) {
			fclose(f);
			return num;
		}
	}
	fclose(f);
	return 0;
}

/**
 * @brief interpret a USC style file name for a RoadMap map
 * @param fn this string contains the map filename
 * @param fips return the FIPS number in this variable
 * @return whether this is a valid file name
 */
int buildmap_osm_filename_usc(char *fn, int *fips)
{
	int	n, f;

	n = sscanf(fn, "usc%d.rdm", &f);
	if (n != 1)
		return 0;
	if (fips)
		*fips = f;
	return 1;
}

/**
 * @brief convert 2 or 3 character ISO country code to the numeric code
 * @param alpha the ISO country code (either 2 or 3 character form)
 * @return the numeric ISO country code
 */
int roadmap_iso_alpha_to_num(char *alpha)
{
	int	i;

	for (i=0; IsoCountryCodeTable[i].numeric; i++) {
		if (strcasecmp(alpha, IsoCountryCodeTable[i].alpha2) == 0) {
			return IsoCountryCodeTable[i].numeric;
		}
		if (strcasecmp(alpha, IsoCountryCodeTable[i].alpha3) == 0) {
			return IsoCountryCodeTable[i].numeric;
		}
	}
	return 0;
}
