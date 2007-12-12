<?php

// This is the CGI interface for the Mobile Binary proxy server.
//
// Released into the public domain by Adam Boardman and Paul Fox
// 

// configure the server
$server = "www.openstreetmap.org";
$server = "osmxapi.informationfreeway.org";
//$server = "openstreetmap.gryph.de";
$path = "/api/0.5/map"; 

require_once( "osmgetbmapcore.php" );
if file_exists("common/static-inc.php") {
    require_once( "common/static-inc.php" );
}
if file_exists("common/symbianmachineid.php") {
    require_once( "common/symbianmachineid.php" );
}

global $nocache, $CacheDir;
$nocache = $_REQUEST["no-cache"];
$CacheDir = "cache";   // cache is in current directory

global $KDeviceUidLookup;
$machine = $KDeviceUidLookup[$_REQUEST["m"]];
if (!$machine) {
	$machine = $_REQUEST["m"];
}

function doLog($text) {
        global $errorlogfilepointer;
        if (!$errorlogfilepointer) {
                global $config;
                $errorlogfile = "log.txt";
                $errorlogfilepointer = fopen($errorlogfile, "a+");
        }

        $logentry = date("jS M Y (H:i:s)")." : ".getenv("REMOTE_ADDR").
			" (".$text.")\r\n";
        fwrite($errorlogfilepointer, $logentry);

	if (is_array($text)) {
	        foreach ($text as $key => $value) {
			fwrite($errorlogfilepointer, "$key => $value\r\n");
		}
	}
}

global $KUserAgentToUsedKeys;
$KUserAgentToUsedKeys["WhereAmI/0.05"] = array(
	"highway",
	"railway",
	"natural",
	"amenity",
	"place",
	"leisure",
	"historic",
	"name",
	"ref",
	"oneway"
	);
$KUserAgentToUsedKeys["Mozilla"] = array(
	"highway",
	"cycleway",
	"waterway",
	"boundary",
	"railway",
	"natural",
	"amenity",
	"place",
	"leisure",
	"historic",
	"name",
	"ref",
	"oneway"
	);
global $WayUsedKeys;
foreach ($KUserAgentToUsedKeys as $key => $value) {
	if (strpos($key, $_SERVER['HTTP_USER_AGENT']) !== False) {
		$WayUsedKeys = $value;
	}
}
if (!$WayUsedKeys) {
	$WayUsedKeys = $KUserAgentToUsedKeys["Mozilla"];
}


function needAuth() {
	header('WWW-Authenticate: Basic realm="OSMtoCSV Auth"');
	header('HTTP/1.0 401 Unauthorized');
	echo 'Please enter your openstreetmap login details';
	exit;
}


$tileid = $_REQUEST["tile"];
$bits =   $_REQUEST["ts"];
$have =   $_REQUEST["have"];

if ($bits < 19 || $bits > 31) $bits = 31;

doLog("Tile: ".$tileid." Bits: ".$bits." - ".$_SERVER['HTTP_USER_AGENT'].
	" ".$_SERVER['PHP_AUTH_USER']." - ".$machine);

if (strstr($HTTP_SERVER_VARS['HTTP_ACCEPT_ENCODING'], 'gzip')) {
	ob_start("compress_output");
}

fetchTile($tileid, $bits, $have, $server, $path);

?>
