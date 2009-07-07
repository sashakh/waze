<?php /**

bmap.php - OSM to Binary - Open Street Map to Binary Protocol converter

Released into the public domain by Adam Boardman and Paul Fox.

See 
    http://wiki.openstreetmap.org/index.php/OSM_Mobile_Binary_Protocol
for more information.

This script takes requests for OSM map information for a bounding box,
makes the request to the Open Street Map API v0.6 and reforms the reply
from XML to a simplified binary format.

eg: bmap.php?tile=tileid&ts=31&have=neighboursdownloaded

the "tileid" is a 32bit integer representing the tile to download, "ts"
indicates how many bits of the tileid are in use (this php implementation
only supports 19-31bits giving tile sizes of 0.175781x0.175781 to
0.005493x0.005493).

neighboursdownloaded = 8bits representing tiles already downloaded {nw,n,ne,
w,e,sw,s,se} eg: have downloaded ne,e,se => 00101001 = 0x29 = 41

only interesting nodes are given, and their properties are
encoded to one 8bit enum.

binid/lat/lon/time etc is 32bit integers where from lat/lon * 1000000

Binary is all little endian

To use this script you must give your web server write access to a directory 
'cache' and a file log.txt

Output format:
Node - length n id lon lat nodepropenumval {propkeyid (enumval | valsignedint | packedtime | strlen str)}
Way - length w id numberlonlat lon lat {londiff latdiff} {propkeyid (enumval | valsignedint | packedtime | strlen str)}
BlnBnBidBBloBBlaB
BlnBnBidBBloBBlaBBp
BlnBn...
BlnBwBidBBnuBBloBBatBBoBaBoBa
BlnBwBidBBnuBBloBBatBBoBa...
BlnBw...

improvement:
tile - 1031529850
osm xml: 4830
bmap.php: 1621
64%
*/



if (!function_exists("timingStart")) {
	function timingStart() {
		global $timingstarted;
		$starttime = microtime();
		$stime = explode(' ', $starttime);
		$timingstarted = $stime[0] + $stime[1];
	}

	function timingEnd() {
		global $timingstarted;
		$endtime = microtime();
		$etime = explode(' ', $endtime);
		$timingended = $etime[0] + $etime[1];
		return number_format($timingended - $timingstarted, 4);
	}
}

if (!function_exists("mkpath")) {
	function mkpath($path)
	{
	    if (@mkdir($path) or file_exists($path)) return true;
	    return (mkpath(dirname($path)) and mkdir($path));
	}
}

// args passed to reportError
global $KDownloadErrors;
global $EDownloadErrorTooBig;
global $EDownloadErrorConnect;
global $EDownloadErrorPerms;
global $EDownloadErrorXMLError;
global $EDownloadErrorNoData;

$EDownloadErrorTooBig   = 5;
$EDownloadErrorConnect  = 6;
$EDownloadErrorPerms    = 7;
$EDownloadErrorXMLError = 8;
$EDownloadErrorNoData   = 9;

// suppress php default error reporting
error_reporting(0);

//allocate values from here:
//  http://wiki.openstreetmap.org/index.php/OSM_Mobile_Binary_Protocol/Types
$KDownloadErrors = Array(
	5=>"The tile is too big, or contains too much data",
	6=>"Couldn't connect to API server", // actual message supplied in-line
	7=>"Couldn't lock data file -- check file/directory permissions",
	8=>"XML Error",  // actuall message supplied in-line
	9=>"No Data",
);

global $KMaxLon,$KMaxLat,$KMaxBits,$KLLMultiplier;
$KMaxLon = 180000000;
$KMaxLat = 90000000;
$KLLMultiplier = 1000000;
global $KNodePropertyIndex;
$KNodePropertyIndex = array (
	"continent"=>1,
	"country"=>2,
	"state"=>3,
	"region"=>4,
	"county"=>5,
	"city"=>6,
	"town"=>7,
	"village"=>8,
	"hamlet"=>9,
	"suburb"=>10,
	"island"=>11,
	"pub"=>14,
	"biergarten"=>15,
	"cafe"=>16,
	"restaurant"=>17,
	"fast_food"=>18,
	"parking"=>19,
	"bicycle_parking"=>20,
	"fuel"=>21,
	"telephone"=>22, 
	"toilets"=>23,
	"recycling"=>24,
	"public_building"=>25,
	"place_of_worship"=>26,
	"grave_yard"=>27,
	"post_office"=>28,
	"post_box"=>29,
	"school"=>30,
	"university"=>31,
	"collage"=>32,
	"pharmacy"=>33,
	"hospital"=>34,
	"library"=>35,
	"police"=>36,
	"fire_station"=>37,
	"bus_station"=>38,
	"theatre"=>39,
	"cinema"=>40,
	"arts_centre"=>41,
	"courthouse"=>42,
	"prison"=>43,
	"bank"=>44,
	"atm"=>45,
	"townhall"=>46,
	"park_ride"=>47,
	"doctors"=>48,
	"clinic"=>49,
	"first_aid"=>50,
	"bureau_de_change"=>51,
	"border_control"=>52,
	"music_venue"=>53,
	"local_government"=>54,
	"embassy"=>55,
	"carsharing"=>56,
	"car_rental"=>57,
	"potable_water"=>58,
	"dumpstation"=>59,
	"cultural_centre"=>60,
	"indoor_shopping_centre"=>61,
	"crematorium"=>62,
	"fire_hydrant"=>63,
	"bicycle_rental"=>64,
	"science_park"=>65,
	"gallery"=>66,
	"stop"=>72,
	"traffic_signals"=>73,
	"highway:crossing"=>74,
	"gate"=>75,
	"stile"=>76,
	"cattle_grid"=>77,
	"toll_booth"=>78,
	"incline"=>79,
	"incline_steep"=>80,
	"highway:viaduct"=>81,
	"motorway_junction"=>82,
	"services"=>83,
	"ford"=>84,
	"mini_roundabout"=>78,
	"bus_sluice"=>79,
	"railway:station"=>89,
	"halt"=>90,
	"railway:viaduct"=>91,
	"railway:crossing"=>92,
	"level_Crossing"=>93,
	"subway:station"=>94,
	"station_entrance"=>95,
	"lock_gate"=>98,
	"turning_point"=>99,
	"aqueduct"=>100,
	"boatyard"=>101,
	"water_point"=>102,
	"waste_disposal"=>103,
	"mooring"=>104,
	"weir"=>105,
	"waterfall"=>106,
	"sports_centre"=>109,
	"golf_course"=>110,
	"stadium"=>111,
	"marina"=>112,
	"track"=>113,
	"pitch"=>114,
	"water_park"=>115,
	"slipway"=>116,
	"fishing"=>117,
	"nature_reserve"=>118,
	"park"=>119,
	"playground"=>120,
	"garden"=>121,
	"common"=>122,
	"information"=>125,
	"tourism:camp_site"=>126,
	"caravan_site"=>127,
	"picnic_site"=>128,
	"viewpoint"=>129,
	"theme_park"=>130,
	"hotel"=>131,
	"motel"=>132,
	"guest_house"=>134,
	"hostel"=>135,
	"attraction"=>136,
	"zoo"=>137,
	"chalet"=>138,
	"rest_camp"=>138,
	"castle"=>141,
	"monument"=>142,
	"museum"=>143,
	"archaeological_site"=>144,
	"icon"=>145,
	"ruins"=>146,
	"wreck"=>147,
	"10Pin"=>150,
	"athletics"=>151,
	"baseball"=>152,
	"basketball"=>153,
	"bowls"=>154,
	"climbing"=>155,
	"cricket"=>156,
	"cricket_nets"=>157,
	"croquet"=>158,
	"cycling"=>159,
	"dog_racing"=>160,
	"equestrian"=>161,
	"football"=>162,
	"golf"=>163,
	"gymnastics"=>164,
	"hockey"=>165,
	"motor"=>166,
	"multi"=>167,
	"pelota"=>168,
	"racquet"=>169,
	"rugby"=>170,
	"skating"=>171,
	"skateboard"=>172,
	"soccer"=>173,
	"swimming"=>174,
	"skiing"=>175,
	"table_tennis"=>176,
	"tennis"=>177,
	"diving"=>178,
	"aerodrome"=>182,
	"terminal"=>183,
	"helipad"=>184,
	"power:tower"=>186,
	"works"=>188,
	"beacon"=>189,
	"survey_point"=>190,
	"power_wind"=>191,
	"power_hydro"=>192,
	"power_fossil"=>193,
	"power_nuclear"=>194,
	"manmade:tower"=>195,
	"water_tower"=>196,
	"gasometer"=>197,
	"reservoir_covered"=>198,
	"lighthouse"=>199,
	"windmill"=>200,
	"surveillance"=>201,
	"communications_tower"=>202,
	"baker"=>204,
	"butcher"=>205,
	"chandler"=>206,
	"supermarket"=>207,
	"outdoor_store"=>208,
	"doityourself"=>209,
	"convenience"=>210,
	"bicycle"=>211,
	"farm"=>215,
	"quarry"=>216,
	"landfill"=>217,
	"basin"=>218,
	"reservior"=>219,
	"forest"=>220,
	"allotments"=>221,
	"residential"=>222,
	"retail"=>223,
	"commercial"=>224,
	"industrial"=>225,
	"brownfield"=>226,
	"greenfield"=>227,
	"cemetery"=>228,
	"village_green"=>229,
	"recreation_ground"=>230,
	"landuse:camp_site"=>231,
	"airfield"=>233,
	"bunker"=>234,
	"barracks"=>235,
	"danger_area"=>236,
	"range"=>237,
	"spring"=>238,
	"peak"=>239,
	"cliff"=>240,
	"scree"=>241,
	"scrub"=>242,
	"fell"=>243,
	"heath"=>244,
	"wood"=>245,
	"marsh"=>246,
	"water"=>247,
	"mud"=>248,
	"beach"=>249,
	"bay"=>250,
	"tree"=>251,
	"life"=>252,
	"cave"=>253,
	"glacier"=>254
	);
global $KNodePropertiesKept;
$KNodePropertiesKept = array ("name","name:*","ref");
global $KWayPropertiesKeyTable;
global $KWayPropEnumMin,$KWayPropEnumMax,$KWayPropNumericalMin,$KWayPropNumericalMax,$KWayPropDateMin,$KWayPropDateMax,$KWayPropTextMin,$KWayPropTextMax;
$KWayPropEnumMin = 1;
$KWayPropEnumMax = 127;
$KWayPropNumericalMin = 128;
$KWayPropNumericalMax = 175;
$KWayPropDateMin = 176;
$KWayPropDateMax = 191;
$KWayPropTextMin = 192;
$KWayPropTextMax = 255;
$KWayPropertiesKeyTable = array (
	//enum based
	"highway"=>1,
	"cycleway"=>2,
	"tracktype"=>3,
	"waterway"=>4,
	"railway"=>5,
	"aeroway"=>6,
	"aerialway"=>7,
	"power"=>8,
	"man_made"=>9,
	"leisure"=>10,
	"amenity"=>11,
	"shop"=>12,
	"tourism"=>13,
	"historic"=>14,
	"landuse"=>15,
	"military"=>16,
	"natural"=>17,
	"route"=>18,
	"boundary"=>19,
	"sport"=>20,
	"abutters"=>21,
	"fenced"=>22,
	"lit"=>23,
	"area"=>24,
	"bridge"=>25,
	"cutting"=>26,
	"embankment"=>27,
	"surface"=>30,
	"access"=>31,
	"bicycle"=>32,
	"foot"=>33,
	"goods"=>34,
	"hgv"=>35,
	"horse"=>36,
	"motorcycle"=>37,
	"motorcar"=>38,
	"psv"=>39,
	"motorboat"=>40,
	"boat"=>41,
	"oneway"=>42,
	"noexit"=>43,
	"toll"=>44,
	"place"=>45,
	"lock"=>46,
	"attraction"=>47,
	"wheelchair"=>48,
	"junction"=>49,
	//numerical
	"lanes"=>128,
	"layer"=>129,
	"ele"=>130,
	"width"=>131,
	"est_width"=>132,
	"maxwidth"=>133,
	"maxlength"=>134,
	"maxspeed"=>135,
	"minspeed"=>136,
	"day_on"=>137,
	"day_off"=>138,
	"hour_on"=>139,
	"hour_off"=>140,
	"maxweight"=>141,
	"maxheight"=>142,
	//date based
	"date_on"=>176,
	"date_off"=>177,
	"start_date"=>178,
	"end_date"=>179,
	//text based
	"name"=>192,
	"name_left"=>193,
	"name_right"=>194,
	"name:*"=>195,
	"int_name"=>196,
	"nat_name"=>197,
	"reg_name"=>198,
	"loc_name"=>199,
	"old_name"=>200,
	"ref"=>201,
	"int_ref"=>202,
	"nat_ref"=>203,
	"reg_ref"=>204,
	"loc_ref"=>205,
	"old_ref"=>206,
	"ncn_ref"=>207,
	"rcn_ref"=>209,
	"lcn_ref"=>210,
	"icao"=>211,
	"iata"=>212,
	"place_name"=>213,
	"place_numbers"=>214,
	"postal_code"=>215,
	"is_in"=>216,
	"note"=>217,
	"description"=>218,
	"image"=>219,
	"source"=>220,
	"source_ref"=>221,
	"created_by"=>222
);
global $KTagsEnums;
$KTagsEnums["highway"] = Array(
	"motorway"=>1,
	"motorway_link"=>2,
	"trunk"=>3,
	"trunk_link"=>4,
	"primary"=>5,
	"primary_link"=>6,
	"secondary"=>7,
	"tertiary"=>8,
	"unclassified"=>9,
	"minor"=>10,
	"residential"=>11,
	"service"=>12,
	"track"=>13,
	"cycleway"=>14,
	"bridleway"=>15,
	"footway"=>16,
	"steps"=>17,
	"pedestrian"=>17
	);
$KTagsEnums["cycleway"] = Array(
	"lane"=>1,
	"track"=>2,
	"opposite_lane"=>3,
	"opposite_track"=>4,
	"opposite"=>5,
	);
$KTagsEnums["waterway"] = Array(
	"river"=>1,
	"canal"=>2,
	);
$KTagsEnums["boundary"] = Array(
	"administrative"=>1,
	"civil"=>2,
	"political"=>3,
	"national_park"=>4,
	);
$KTagsEnums["abutters"] = Array(
	"residential"=>1,
	"retail"=>2,
	"industrial"=>3,
	"commercial"=>4,
	"mixed"=>5,
	);
$KTagsEnums["railway"] = Array(
	"rail"=>1,
	"tram"=>2,
	"light_rail"=>3,
	"subway"=>4,
	"station"=>5,
	);
$KTagsEnums["natural"] = Array(
	"coastline"=>1,
	"water"=>2,
	"wood"=>3,
	"peak"=>4,
	);
$KTagsEnums["amenity"] = Array(
	"hospital"=>1,
	"pub"=>2,
	"parking"=>3,
	"post_office"=>4,
	"fuel"=>5,
	"telephone"=>6,
	"toilets"=>7,
	"post_box"=>8,
	"school"=>9,
	"supermarket"=>10,
	"library"=>11,
	"theatre"=>12,
	"cinema"=>13,
	"police"=>14,
	"fire_station"=>15,
	"restaurant"=>16,
	"fastfood"=>17,
	"bus_station"=>18,
	"place_of_worship"=>19,
	"cafe"=>20,
	"bicycle_parking"=>21,
	"public_building"=>22,
	"grave_yard"=>23,
	"universite"=>24,
	"college"=>25,
	"townhall"=>26
	);
$KTagsEnums["place"] = Array(
	"continent"=>1,
	"country"=>2,
	"state"=>3,
	"region"=>4,
	"county"=>5,
	"city"=>6,
	"town"=>7,
	"village"=>8,
	"hamlet"=>9,
	"suburb"=>10,
	);
$KTagsEnums["leisure"] = Array(
	"park"=>1,
	"common"=>2,
	"garden"=>3,
	"nature_reserve"=>4,
	"fishing"=>5,
	"slipway"=>6,
	"water_park"=>7,
	"pitch"=>8,
	"track"=>9,
	"marina"=>10,
	"stadium"=>11,
	"golf_course"=>12,
	"sports_centre"=>13,
	);
$KTagsEnums["historic"] = Array(
	"castle"=>1,
	"monument"=>2,
	"museum"=>3,
	"archaeological_site"=>4,
	"icon"=>5,
	"ruins"=>6
	);
$KTagsEnums["oneway"] = Array(
	"no"=>0,
	"false"=>0,
	"yes"=>1,
	"true"=>1,
	"1"=>1,
	"-1"=>2
	);


global $nodecount;
global $waycount;
global $nodes;
global $waysalreadydownloaded;

class Element {
	var $type;
	var $id;
	var $timestamp;
	var $properties=Array();
	function OutputAsCSV() {
	}
	function OutputProperties($node=False) {
		global $KNodePropertyIndex,$KNodePropertiesKept,$KPropertiesOmitted;
		global $KWayPropertiesKeyTable;
		$beginquote = False;
		$comma = False;
		$output = "";
		if ($node) {
			$nodepropertiesbin = 0;
			foreach ($KNodePropertyIndex as $nodeprop => $nodepropbin) {
				$valuetofind = $nodeprop;
				$keytofind = "";
				$colonpos = strpos($nodeprop,":");
				$valuetofind = $nodeprop;
				if ($colonpos !== False) {
					$valuetofind = substr($nodeprop,$colonpos+1);
					$keytofind = substr($nodeprop,0,$colonpos);
				}
				$key = array_search($valuetofind,$this->properties);
				if ($key && (!$keytofind || ($key == $keytofind))) {
					$nodepropertiesbin = $nodepropbin;
					break;
				}
			}
			$output .= pack("C",$nodepropertiesbin);
			$nodeproperties = array();
			foreach ($this->properties as $key => $value) {
				if (in_array($key,$KNodePropertiesKept) && ($value != "FIXME")) {
					$value = substr($value,0,255);
					$nodeproperties[$KWayPropertiesKeyTable[$key]] = pack("C",strlen($value)).$value;
				}
			}
			foreach ($nodeproperties as $key => $value) {
				$output .= pack("C",$key).$value;
			}
		} else {
			global $KWayPropertiesKeyTable;
			global $KTagsEnums;
			global $WayUsedKeys;
			global $KWayPropEnumMin,$KWayPropEnumMax,$KWayPropNumericalMin,$KWayPropNumericalMax,$KWayPropDateMin,$KWayPropDateMax,$KWayPropTextMin,$KWayPropTextMax;
			$wayproperties = array();
			foreach ($this->properties as $key => $value) {
				if ((in_array($key,$WayUsedKeys) !== False) && ($value != "FIXME")) {
					$waykeyid = $KWayPropertiesKeyTable[$key];
					if (($waykeyid >= $KWayPropEnumMin) && ($waykeyid < $KWayPropEnumMax)) {
						$enumvalue = $KTagsEnums[$key][$value];
						if ($enumvalue) {
							$wayproperties[$waykeyid] = pack("C",$enumvalue);
						}
					} else if (($waykeyid >= $KWayPropNumericalMin) && ($waykeyid < $KWayPropNumericalMax)) {
						$wayproperties[$waykeyid] = pack("l",$enumvalue);
					} else if (($waykeyid >= $KWayPropDateMin) && ($waykeyid < $KWayPropDateMax)) {
						$wayproperties[$waykeyid] = pack("V",myStrToTime($value));
					} else if (($waykeyid >= $KWayPropTextMin) && ($waykeyid < $KWayPropTextMax)) {
						$value = substr($value,0,255);
						$wayproperties[$waykeyid] = pack("C",strlen($value)).$value;
					}
				}
			}
			foreach ($wayproperties as $key => $value) {
				$output .= pack("C",$key).$value;
			}
		}
		return $output;
	}
};

function myStrToTime($string) {
	if (($pos=strpos($string,"+")) !== FALSE) {
		return(strtotime(substr($string,0,$pos-1)));
	}
}

function toDegrees($micros) {
	global $KLLMultiplier;
	return $micros / $KLLMultiplier;
}

// avoid rounding errors which can introduce small discrepancies in lat/lon values
function toMicroDegrees($degrees) {
	return str_replace(".", "", sprintf("%.6f", $degrees));
}

class NodeElement extends Element {
	var $lat;
	var $lon;
	function OutputAsCSV() {
		$output = "n";
		$output .= pack("V",$this->id);
		$output .= pack("VV",toMicroDegrees($this->lon),toMicroDegrees($this->lat));
		$prop = $this->OutputProperties(True);
		$output .= $prop;
		$nodeproparray = unpack('Cp',substr($prop,0,1));
		if ((strlen($prop)>1)|| ($nodeproparray["p"])) {
			echo pack("V",strlen($output));
			echo $output;
		}
	}
}

function appendNodeToList(&$nodelist,&$nodecount,&$node,&$lastlon,&$lastlat,&$first) {
	if ($first) {
		$lastlon = toMicroDegrees($node->lon);
		$lastlat = toMicroDegrees($node->lat);
		$nodelist .= pack("ll",$lastlon,$lastlat);
		$nodecount++;
		$first = False;
	} else {
		$thislon = toMicroDegrees($node->lon);
		$thislat = toMicroDegrees($node->lat);
		$lon = $thislon-$lastlon;
		$lat = $thislat-$lastlat;
		while (($lon > 32767) || ($lon < -32767)) {
			if ($lon > 32767) {
				$partlon = 32767;
			} else {
				$partlon = -32767;
			}
			$partlat = $lat/($lon/$partlon);
			if ($partlat > 32767 or $partlat < -32767) {
				if ($partlat > 32767) {
					$partlat = 32767;
				} else {
					$partlat = -32767;
				}
				$partlon = $lon/($lat/$partlat);
			}

			$lon = $lon-$partlon;
			$lat = $lat-$partlat;
			$nodelist .= pack("ss",$partlon,$partlat);

			$nodecount++;
		}
		while (($lat > 32767) || ($lat < -32767)) {
			if ($lat > 32767) {
				$partlat = 32767;
			} else {
				$partlat = -32767;
			}
			$partlon = $lon/($lat/$partlat);
			if ($partlon > 32767 or $partlon < -32767) {
				if ($partlon > 32767) {
					$partlon = 32767;
				} else {
					$partlon = -32767;
				}
				$partlat = $lat/($lon/$partlon);
			}
			$lon = $lon-$partlon;
			$lat = $lat-$partlat;
			$nodelist .= pack("ss",$partlon,$partlat);
			$nodecount++;
		}
		$nodelist .= pack("ss",$lon,$lat);
		$nodecount++;
		$lastlon = $thislon;
		$lastlat = $thislat;
	}
}

class WayElement extends Element {
	var $nodes=Array();
	function OutputAsCSV() {
		global $nodes;
		$output = "w";
		$output .= pack("V",$this->id);
		$first=True;
		$lastnodeid=0;
		$lastlat=0;
		$lastlon=0;
		$nodelist="";
		$nodecount=0;
		foreach ($this->nodes as $key => $nodeid) {
			$node = $nodes[$nodeid];
			appendNodeToList($nodelist,$nodecount,$node,$lastlon,$lastlat,$first);
		}
		$output .= pack("V",$nodecount);
		$output .= $nodelist;
		$output .= $this->OutputProperties();
		echo pack("V",strlen($output));
                echo $output;
	}
}

function startElement($parser, $name, $attrs) {
	global $current;
	switch ($name) {
		case "NODE":
		global $nodecount;
		$nodecount++;
		$current = new NodeElement();
		$current->id = $attrs["ID"];
		if ($attrs["ID"] > 4294967295) {
			doLog("Node ID too big!!");
		}
		$current->timestamp = $attrs["TIMESTAMP"];
		$current->lat = $attrs["LAT"];
		$current->lon = $attrs["LON"];
		break;
		case "WAY":
		global $waycount;
		$waycount++;
		$current = new WayElement();
		$current->id = $attrs["ID"];
		if ($attrs["ID"] > 4294967295) {
			doLog("Way ID too big!!");
		}
		$current->timestamp = $attrs["TIMESTAMP"];
		break;
		case "TAG":
		if ($attrs["K"] != "created_by") {
			$current->properties[$attrs["K"]]=$attrs["V"];
		}
		break;
		case "ND":
		$current->nodes[]=$attrs["REF"];
		break;
	}
}

function endElement($parser, $name) {
	global $current,$nodes,$ways;
	switch ($name) {
		case "NODE":
		$nodes[$current->id] = $current;
		if ($current->properties) {
			$current->OutputAsCSV();
		}
		break;
		case "WAY":
		$current->OutputAsCSV();
		break;
	}
}

function compress_output($output) {
	$compressed_out = gzencode($output);
	header("Content-Encoding: gzip");
	return $compressed_out;
}

function lonLatToTileId($lon, $lat) {
	global $KMaxLon,$KMaxLat,$KMaxBits;
	$out = 0;
	for ($index=0; $index<$KMaxBits; $index++) {
		if ($index%2) {
			if ($lat >= ($KMaxLat-($KMaxLat>>($index>>1)))) {
				$out += 1<<($KMaxBits-1-$index);
			} else {
				$lat += ($KMaxLat>>($index>>1));
			}
		} else {
			if ($lon >= ($KMaxLon-($KMaxLon>>($index>>1)))) {
				$out += 1<<($KMaxBits-1-$index);
			} else {
				$lon += ($KMaxLon>>($index>>1));
			}
		}
	}
	return $out;
}

function tileIdToBBoxArg($tileid,&$lonmin,&$latmin,&$lonmax,&$latmax) {
	global $KMaxLon,$KMaxLat,$KMaxBits;
	$lonmin = -$KMaxLon;
	$latmin = -$KMaxLat;
	for ($index=($KMaxBits-1); $index>=0; $index--) {
		$set = ((($tileid>>(($KMaxBits-1)-$index)) %2)==1);
		if ($set) {
			if ($index%2) {
				$latmin += ($KMaxLat>>($index>>1));
			} else {
				$lonmin += ($KMaxLon>>($index>>1));
			}
		}
	}
	$lonmax = $lonmin+($KMaxLon>>(($KMaxBits-1)>>1));
	$latmax = $latmin+($KMaxLat>>(($KMaxBits-2)>>1));
}

function tileIdToBBox($tileid) {
	$lonmin = 0;
	$latmin = 0;
	$lonmax = 0;
	$latmax = 0;
	tileIdToBBoxArg($tileid,$lonmin,$latmin,$lonmax,$latmax);
	$out = sprintf("%F,%F,%F,%F", toDegrees($lonmin), toDegrees($latmin),toDegrees($lonmax), toDegrees($latmax));
	return $out;
}

function report_xml_error($xml_parser) {
	global $EDownloadErrorXMLError;
	reportError($EDownloadErrorXMLError,
		sprintf("XML error: %s at line %d",
			xml_error_string( xml_get_error_code($xml_parser)),
			xml_get_current_line_number( $xml_parser)));
}

function downloadToXml($serverhost,$serverpath,$query) {
	global $phpsupportsfilter;

	$testfile = $_REQUEST["testfile"];

	$path = $serverpath . "?" . $query;
	if ($testfile) {
		$fp = fopen("test.5.osm",'r');
	} else {
		$fp = fsockopen( $serverhost, 80, &$errno, &$errstr, 120);
	}
	
	if( !$fp ) {
		global $EDownloadErrorConnect;
		reportError($EDownloadErrorConnect,
		  "Couldn't connect to API server ".$serverhost);
	}

	doLog($serverhost.$path);
	if (!$testfile) {
		fwrite($fp, "GET $path HTTP/1.1\r\n");
		fwrite($fp, "Host: $serverhost\r\n");
		if ($_SERVER['PHP_AUTH_PW']) {
			fwrite($fp, "Authorization: Basic " .
			    base64_encode($_SERVER['PHP_AUTH_USER'] .
			    ":" . $_SERVER['PHP_AUTH_PW']) . "\r\n");
		}
		fwrite($fp, "User-Agent: OSMtoCSV/1.0\r\n");
		fwrite($fp, "Accept-encoding: gzip\r\n");
		fwrite($fp, "Connection: Close\r\n");
		fwrite($fp, "\r\n");
		//doLog("wrote to socket");
	}

	$gzip = False;
	$chunked = False;
	$header200 = False;
	$gotdata = False;
	if (!$testfile) {
		stream_set_timeout($fp,60*20);
		ini_set('default_socket_timeout',  60*20);
	}

	// get first (status) line
	//doLog("get status line");
	if ($statusline = fgets($fp)) {
		$statusline = trim($statusline);
		//doLog("got statusline: ".$statusline);
		if (strpos($statusline," 200 ")!==FALSE) {
			$header200 = True;
		} else if (strpos($statusline," 401 ")!==FALSE) {
			needAuth();
		} else if (strpos($statusline," 400 ")!==FALSE) {
			global $EDownloadErrorTooBig;
			reportError($EDownloadErrorTooBig);
		} else if (strpos($statusline," 404 ")!==FALSE) {
			global $EDownloadErrorConnect;
			reportError($EDownloadErrorConnect);
		}
	}

	if ($gzip && $chunked) {
		global $EDownloadErrorNoData;
		reportError($EDownloadErrorNoData,
			"Can't handle chunked gzipped response");
	}

	// get header
	while (!feof($fp)) {

		$headerline = trim(fgets($fp));
		//doLog("got headerline, len: ".strlen($headerline));

		// headers end with a blank line
		if (!$headerline)
		    break;

		if (strncasecmp($headerline,'Location:', 9) == 0) {
			$headerline = trim(substr($headerline, 9));
			doLog("Redirecting to: " . $headerline );
			$parsed = parse_url($headerline);
			fclose($fp);
			downloadToXml(
				$parsed['host'],$parsed['path'],
				$parsed['query']);
			return;
		}
	
		if (strncasecmp($headerline,"Content-encoding: gzip",
				strlen("Content-encoding: gzip")) == 0) {
			$gzip = True;
		}

		if (strncasecmp($headerline, "Transfer-Encoding: chunked",
				strlen("Transfer-Encoding: chunked")) == 0) {
			$chunked = True;
		}

	}

	//doLog("finished header");

	if (!$header200) {
		//doLog("No 200 header: '" . $statusline . "'");
		reportError($EDownloadErrorNoData,
			"Got: " . $statusline);
	}


	if ($gzip) {
		//doLog("reading gzip header");
		$discard = fread($fp, 10); // discard gzip file header
		if ($phpsupportsfilter) {
			//doLog("append filter");
			if (!stream_filter_append($fp, 'zlib.inflate',
					STREAM_FILTER_READ)) {
				doLog("error appending zlib.inflate");
				exit();
			}
		}
	}
	//doLog("create parser");
	$xml_parser = xml_parser_create();
	xml_set_element_handler($xml_parser, "startElement", "endElement");

	//doLog("gzip: $gzip, chunked: $chunked, feof: ".feof($fp));

	/* get and parse the body */
	if ($chunked) {
		while (!feof($fp)) {

			// read a length line:  "hex[;[comment]]CRNL"
			$chunkline = preg_replace("/;.*/","",fgets($fp));
			$chunk = hexdec($chunkline);

			if ($chunk == 0)
			    break;

			$got = 0;
			while ($got < $chunk &&
					($piece = fread($fp, $chunk - $got))) {
				$got += strlen($piece);
				if (!xml_parse($xml_parser, $piece, False)) {
					report_xml_error($xml_parser);
				}
				$gotdata = True;
			}
			fgets($fp); // data is followed by CR/NL
		}
		if (!xml_parse($xml_parser, "", True)) {
			report_xml_error($xml_parser);
		}
		// consume footers, if any, and trailing blank line
		while (!feof($fp)) {
			$footer = fgets($fp);
		}

	} else {
		// not chunked -- much simpler
		$alldata = "";
		while (!feof($fp)) {
			//doLog("about to fread");
			$data = fread($fp, 8192);
			//doLog("read");
			//doLog("readdata len: ".strlen($data).", feof: ".feof($fp));
			if ($phpsupportsfilter || !$gzip) {
				//doLog("data is '" . $data . "'");
				if (!xml_parse($xml_parser, $data, feof($fp))) {
					report_xml_error($xml_parser);
				}
				$gotdata = True;
			} else {
				$alldata .= $data;
			}
		}
		if (!$phpsupportsfilter && $gzip) {
			//doLog("parse all at once after unzip");
			$alldata = gzinflate($alldata);
			if (!xml_parse($xml_parser, $alldata, feof($fp))) {
				report_xml_error($xml_parser);
			}
			$gotdata = True;
		}
	}
	xml_parser_free($xml_parser);

	fclose($fp);

	if (!$gotdata) {
		global $EDownloadErrorNoData;
		reportError($EDownloadErrorNoData);
	}

}

function old_downloadToXml($serverhost,$serverpath,$query) {

	$testfile = $_REQUEST["testfile"];

	$path = $serverpath . "?" . $query;
	if ($testfile) {
		$fp = fopen("test.5.osm",'r');
	} else {
		$fp = fsockopen( $serverhost, 80, &$errno, &$errstr, 120);
	}
	
	if( !$fp ) {
		global $EDownloadErrorConnect;
		reportError($EDownloadErrorConnect,
		  "Couldn't connect to API server ".$serverhost);
	}

	doLog($serverhost.$path);
	if (!$testfile) {
		fwrite($fp, "GET $path HTTP/1.1\r\n");
		fwrite($fp, "Host: $serverhost\r\n");
		if ($_SERVER['PHP_AUTH_PW']) {
			fwrite($fp, "Authorization: Basic " .
			    base64_encode($_SERVER['PHP_AUTH_USER'] .
			    ":" . $_SERVER['PHP_AUTH_PW']) . "\r\n");
		}
		fwrite($fp, "User-Agent: OSMtoCSV/1.0\r\n");
		fwrite($fp, "Accept-encoding: gzip\r\n");
		fwrite($fp, "Connection: Close\r\n");
		fwrite($fp, "\r\n");
	}

	$gzip = False;
	$chunked = False;
	$header200 = False;
	$gotdata = False;
	if (!$testfile) {
		stream_set_timeout($fp,60*20);
		ini_set('default_socket_timeout',  60*20);
	}

	// get first (status) line
	if ($statusline = fgets($fp)) {
		$statusline = trim($statusline);
		if (strpos($statusline," 200 ")!==FALSE) {
			$header200 = True;
		} else if (strpos($statusline," 401 ")!==FALSE) {
			needAuth();
		} else if (strpos($statusline," 400 ")!==FALSE) {
			global $EDownloadErrorTooBig;
			reportError($EDownloadErrorTooBig);
		} else if (strpos($statusline," 404 ")!==FALSE) {
			global $EDownloadErrorConnect;
			reportError($EDownloadErrorConnect);
		}
	}

	if ($gzip && $chunked) {
		global $EDownloadErrorNoData;
		reportError($EDownloadErrorNoData,
			"Can't handle chunked gzipped response");
	}

	// get header
	while (!feof($fp)) {

		$headerline = trim(fgets($fp));

		// headers end with a blank line
		if (!$headerline)
		    break;

		if (strncasecmp($headerline,'Location:', 9) == 0) {
			$headerline = trim(substr($headerline, 9));
			doLog("Redirecting to: " . $headerline );
			$parsed = parse_url($headerline);
			fclose($fp);
			downloadToXml(
				$parsed['host'],$parsed['path'],
				$parsed['query']);
			return;
		}
	
		if (strncasecmp($headerline,"Content-encoding: gzip",
				strlen("Content-encoding: gzip")) == 0) {
			$gzip = True;
		}

		if (strncasecmp($headerline, "Transfer-Encoding: chunked",
				strlen("Transfer-Encoding: chunked")) == 0) {
			$chunked = True;
		}

	}

	if (!$header200) {
		doLog("No 200 header: '" . $statusline . "'");
		reportError($EDownloadErrorNoData,
			"Got: " . $statusline);
	}


	if ($gzip) {
		$toss = fread($fp, 10); // discard gzip file header
		if (!stream_filter_append($fp, 'zlib.inflate',
						STREAM_FILTER_READ)) {
		    doLog("error appending zlib.inflate");
		    exit();
		}
	}

	$xml_parser = xml_parser_create();
	xml_set_element_handler($xml_parser, "startElement", "endElement");

	/* get and parse the body */
	if ($chunked) {
		while (!feof($fp)) {

			// read a length line:  "hex[;[comment]]CRNL"
			$chunkline = preg_replace("/;.*/","",fgets($fp));
			$chunk = hexdec($chunkline);

			if ($chunk == 0)
			    break;

			$got = 0;
			while ($got < $chunk &&
					($piece = fread($fp, $chunk - $got))) {
				$got += strlen($piece);
				if (!xml_parse($xml_parser, $piece, False)) {
					report_xml_error($xml_parser);
				}
				$gotdata = True;
			}
			fgets($fp); // data is followed by CR/NL
		}
		if (!xml_parse($xml_parser, "", True)) {
			report_xml_error($xml_parser);
		}
		// consume footers, if any, and trailing blank line
		while (!feof($fp)) {
			$footer = fgets($fp);
		}

	} else {
		// not chunked -- much simpler
		while (!feof($fp)) {
			$data = fread($fp, 8192);
			// doLog("data is '" . $data . "'");
			if (!xml_parse($xml_parser, $data, feof($fp))) {
				report_xml_error($xml_parser);
			}
			$gotdata = True;
		}
	}
	xml_parser_free($xml_parser);

	fclose($fp);

	if (!$gotdata) {
		global $EDownloadErrorNoData;
		reportError($EDownloadErrorNoData);
	}

}

function getStaticPageNameBMap($tileid) {
	global $KMaxBits, $CacheDir;
	$htmldir = $CacheDir."/".$KMaxBits."/";
	if (!@is_dir($htmldir)) {
		//echo "made: ".$htmldir;
		mkpath($htmldir,0775,True);
	}
	$pagename = $htmldir.$tileid;
	return $pagename;
}


//scan surrounding tiles for ways already known of
function findKnownWays($tileid, $have) {
	global $waysalreadydownloaded,$KMaxBits,$KMaxLat,$KMaxLon;
	$waysalreadydownloaded = Array();
	$neighbours = array(0x01=>"+-",0x02=>" -",0x04=>"--",0x08=>"+ ",0x10=>"- ",0x20=>"++",0x40=>" +",0x80=>"-+");
	$lonmin = 0;
	$latmin = 0;
	$lonmax = 0;
	$latmax = 0;
	$gridsizelon = ($KMaxLon>>(($KMaxBits-1)>>1));
	$gridsizelat = ($KMaxLat>>(($KMaxBits-2)>>1));
	//echo "gridsizelon: $gridsizelon, gridsizelat: $gridsizelat, ";
	//echo base_convert($tileid, 10, 2);
	tileIdToBBoxArg($tileid,$lonmin,$latmin,$lonmax,$latmax);
	//echo "gridsizelon1: ".($lonmax-$lonmin).", gridsizelat1: ".($latmax-$latmin).", ";
	
	foreach ($neighbours as $mask => $direction) {
		if ($have&$mask) {
			//doLog("have: $have - $direction");
			//echo "<p>have: $direction, ";
			$lon = $lonmin+$gridsizelon/2;
			$lat = $latmin+$gridsizelat/2;
			if ($direction[0]=='+')
				$lon += $gridsizelon;
			if ($direction[0]=='-')
				$lon -= $gridsizelon;
			if ($direction[1]=='+')
				$lat += $gridsizelat;
			if ($direction[1]=='-')
				$lat -= $gridsizelat;
			
			$neighbourid = lonLatToTileId($lon,$lat);
			//echo base_convert($neighbourid, 10, 2);
			//echo ", ".base_convert($neighbourid^$tileid, 10, 2);
			//doLog("neighbour: $neighbourid");
			
			$filename = getStaticPageNameBMap($neighbourid);
			if (file_exists($filename)) {
				// doLog("found: $filename, ");
				$content  = file_get_contents($filename);
				$foundwayscount = 0;
				for ($i=0; $i<(strlen($content)); ) {
					$lenarray = unpack('Vlen',substr($content,$i,4));
					$len = $lenarray["len"];
					if (($content[$i+4] == 'w')) {
						$packarray = unpack('Vid',substr($content,$i+5,4));
						//var_dump($packarray);
						//doLog("foundway(".$content[$i].$content[$i+1].$content[$i+2].$content[$i+3].$content[$i+4].")[".($foundwayscount++)."]: ".$packarray["id"]);
						$waysalreadydownloaded[]=$packarray["id"];
					}
					$i+=4+$len;
				}
			}
		}
	}
}

/*
 * start static page construction, also set to ignore abort if first
 * one to ask for this page
*/
function staticPageStartBMap($tileid) {
	global $staticpagename,$staticpagelockfilehandle;
	$staticpagename = getStaticPageNameBMap($tileid);
	// doLog("staticpagename is $staticpagename");
	$staticpagelockfilehandle = fopen($staticpagename.".lock",'w');
	if (flock($staticpagelockfilehandle, LOCK_EX)) {
		fwrite($staticpagelockfilehandle, "");
		$cachetimeout = 28800; //how long to keep cache files for
		global $nocache;
		if (!$nocache && file_exists($staticpagename) &&
			(filemtime($staticpagename)+$cachetimeout > time())) {

			fclose($staticpagelockfilehandle);
			//read and return page
			$tocutdotpos = strripos($staticpagename, ".");
			if ($tocutdotpos) {
				$extension = substr($staticpagename,$tocutdotpos+1);
			}
			outputWithoutNeighbouringWays(file_get_contents($staticpagename));
			echo pack("V",2)."#C";
			global $skipwaycount;
			doLog("Cache Hit, Skipped Ways: ".$skipwaycount);
			exit();
		} else {
			ignore_user_abort(True);
		}
	} else {
		global $EDownloadErrorPerms;
		reportError($EDownloadErrorPerms);
	}
	
	timingStart();
	ob_start();
}

function staticPageEndBMap() {
	global $staticpagename,$staticpagelockfilehandle;
	
	$output = "t";
	$currenttime = time();
	$output .= pack("V",$currenttime);
	echo pack("V",strlen($output));
	echo $output;
	doLog("Page create time: ".timingEnd());
	
	$staticpagecontent = ob_get_clean();
	outputWithoutNeighbouringWays($staticpagecontent);
	
	$f = fopen($staticpagename,'w');
	fwrite($f, $staticpagecontent);
	fclose($f);
	fclose($staticpagelockfilehandle);
	
	echo pack("V",2)."#F";
}

function reportError($error, $errorString) {
	global $KDownloadErrors;
	
	$output = "f";
	$output .= pack("C",$error);
	if (!$errorString) {
	    $errorString = $KDownloadErrors[$error];
	}
	$errorString = substr($errorString,0,255);
	$output .= pack("C", strlen($errorString)).$errorString;
	echo pack("V",strlen($output));
	echo $output;
	doLog("Reported error #".$error." '".$errorString."'");
	exit;
}

function outputWithoutNeighbouringWays($content) {
	global $waysalreadydownloaded,$skipwaycount;
	$output = True;
	$skipwaycount = 0;
	$skippedways = Array();
	for ($i=0; $i<(strlen($content)); ) {
		$lenarray = unpack('Vlen',substr($content,$i,4));
		$len = $lenarray["len"];
		if ($content[$i+4] == 'w') {
			$packarray = unpack('Vid',substr($content,$i+5,4));
			//doLog("checking way: ".$packarray['id']);
			if (in_array($packarray['id'],$waysalreadydownloaded)) {
				// doLog("skipping way: ".$packarray['id']);
				$output = False;
				$skipwaycount++;
				$skippedways[] = $packarray['id'];
			} else {
				$output = True;
			}
		} else {
			$output = True;
		}
		if ($output) {
			echo substr($content,$i,4+$len);
		}
		$i+=4+$len;
	}
	//output list of skipped way ids
	if (count($skippedways)) {
		//echo "<p>skippedways</p>";
		$output = "o";
		for ($i=0; $i<count($skippedways); $i++) {
			//doLog($skippedways[$i]);
			$output .= pack("V",$skippedways[$i]);
		}
		echo pack("V",strlen($output));
		echo $output;
	} else {
		//echo "<p>noskippedways</p>";
	}
}

function fetchTile($tileid, $bits, $have, $server, $path) {

    global $KMaxBits, $nodecount, $waycount;
    $KMaxBits = $bits;

    set_time_limit(60*20);

    $bbox = tileIdToBBox($tileid);

    findKnownWays($tileid,$have);

    staticPageStartBMap($tileid);

    downloadToXml($server, $path, "bbox=".$bbox);
    doLog("Nodes: ".$nodecount.", Ways: ".$waycount);

    staticPageEndBMap();

    global $skipwaycount;
    doLog("Skipped Ways: ".$skipwaycount);

}


?>
