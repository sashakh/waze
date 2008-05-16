/* roadmap_path.c - a module to handle file path in an OS independent way.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright 2008 Danny Backx
 *
 *   Based on an implementation by Pascal F. Martin.
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
 *   See roadmap_path.h.
 */

#include <windows.h>

#include "../roadmap.h"
#include "../roadmap_file.h"
#include "../roadmap_path.h"

const char *RoadMapPathCurrentDirectory = ".";

typedef struct RoadMapPathRecord *RoadMapPathList;

struct RoadMapPathRecord {
	RoadMapPathList next;

	char  *name;
	int    count;
	char **items;
	char  *preferred;
};

static RoadMapPathList RoadMapPaths = NULL;

/*
 * There were a couple of hardcoded paths here, they're now mostly removed
 * to be able to cope with storage cards named differently from
 * "/storage card".
 *
 * The default paths were
 * 	/Program Files/roadmap
 *	/Storage Card/roadmap
 * and something similar for /.../roadmap/maps .
 *
 * This is now changed. If your card is called /Storage card then the
 * result is the same.
 */

/* The number of storage cards supported */
#define	ROADMAP_MAX_CARDS	8

/* The path for configuration files (the "config" path). */ 
static char **RoadMapPathConfig = 0;
static const char *RoadMapPathConfigSuffix = "roadmap";

static const char *RoadMapPathConfigPreferred = "/Storage Card/roadmap";

/* The default path for the map files (the "maps" path): */
static char **RoadMapPathMaps = 0;
static const char *RoadMapPathMapsSuffix = "roadmap/maps";

static const char *RoadMapPathMapsPreferred = "/Storage Card/roadmap/maps";

/* We don't have a user directory in wince so we'll leave this one empty */
static const char *RoadMapPathUser[] = {
	NULL
};

static void roadmap_path_insert(const char *dir)
{
	static int i = 0;
	int l = strlen(dir);

	if (i == ROADMAP_MAX_CARDS)
		return;
	if (RoadMapPathConfig == 0)
		RoadMapPathConfig = (char **)calloc(ROADMAP_MAX_CARDS + 1,
				sizeof(char *));
	if (RoadMapPathMaps == 0)
		RoadMapPathMaps = (char **)calloc(ROADMAP_MAX_CARDS + 1,
				sizeof(char *));

	RoadMapPathConfig[i] = malloc(l + strlen(RoadMapPathConfigSuffix) + 4);
	sprintf(RoadMapPathConfig[i], "/%s/%s", dir, RoadMapPathConfigSuffix);

	RoadMapPathMaps[i] = malloc(l + strlen(RoadMapPathMapsSuffix) + 4);
	sprintf(RoadMapPathMaps[i], "/%s/%s", dir, RoadMapPathMapsSuffix);

	i++;

	/* Make sure the lists are null-terminated */
	RoadMapPathConfig[i] = 0;
	RoadMapPathMaps[i] = 0;
}


static void roadmap_path_get_cards()
{
	WIN32_FIND_DATA	ffd;
	HANDLE		h;
	int		i;
	static int	done = 0;

	/* Run this only once */
	if (done != 0)
		return;
	done++;

	roadmap_path_insert("Program Files");
	h = FindFirstFlashCard(&ffd);
	if (h == INVALID_HANDLE_VALUE) {
		roadmap_path_insert("Storage card");	/* Do this anyway */
		return;
	}
	roadmap_path_insert(ConvertToANSI(ffd.cFileName, CP_ACP));
	while (FindNextFlashCard(h, &ffd) == TRUE) {
		roadmap_path_insert(ConvertToANSI(ffd.cFileName, CP_ACP));
	}
	FindClose(h);
}


static void roadmap_path_list_create(const char *name,
		const char *items[],
		const char *preferred)
{
	int i;
	int count;
	RoadMapPathList new_path;

	for (count = 0; items && items[count] != NULL; ++count) ;

	new_path = malloc (sizeof(struct RoadMapPathRecord));
	roadmap_check_allocated(new_path);

	new_path->next  = RoadMapPaths;
	new_path->name  = strdup(name);
	new_path->count = count;
	new_path->items = NULL;
	new_path->preferred = NULL;

	if (count) {
		new_path->items = calloc (count, sizeof(char *));
		roadmap_check_allocated(new_path->items);

		for (i = 0; i < count; ++i) {
			new_path->items[i] = strdup(items[i]);
		}
	}
	if (preferred) {
		new_path->preferred  = strdup(preferred);
	}

	RoadMapPaths = new_path;
}

static RoadMapPathList roadmap_path_find (const char *name)
{
	RoadMapPathList cursor;

	roadmap_path_get_cards();

	if (RoadMapPaths == NULL) {

		/* Add the hardcoded configuration. */
		roadmap_path_list_create ("config", RoadMapPathConfig,
				RoadMapPathConfigPreferred);
		roadmap_path_list_create ("maps", RoadMapPathMaps,
				RoadMapPathMapsPreferred);
		roadmap_path_list_create ("user", RoadMapPathUser, "/");

		roadmap_path_list_create ("features",    NULL, NULL);
	}

	for (cursor = RoadMapPaths; cursor != NULL; cursor = cursor->next) {
		if (strcasecmp(cursor->name, name) == 0) break;
	}
	return cursor;
}


/* Directory path strings operations. -------------------------------------- */

static char *roadmap_path_cat (const char *s1, const char *s2)
{ 
	char *result = malloc (strlen(s1) + strlen(s2) + 4);

	roadmap_check_allocated (result);

	strcpy (result, s1);
	strcat (result, "/");
	strcat (result, s2);

	return result;
}


char *roadmap_path_join (const char *path, const char *name)
{
	if (path == NULL || path[0] == 0) {
		return strdup (name);
	}
	return roadmap_path_cat (path, name);
}


char *roadmap_path_parent (const char *path, const char *name)
{
	char *separator;
	char *full_name = roadmap_path_join (path, name);

	separator = strrchr (full_name, '/');
	if (separator == NULL)
		separator = strrchr (full_name, '\\');

	if (separator == NULL) {
		roadmap_path_free (full_name);
		return strdup(RoadMapPathCurrentDirectory);
	}

	*separator = 0;

	return full_name;
}


char *roadmap_path_skip_directories (const char *name)
{
	char *result = strrchr (name, '/');

	if (result == NULL)
		result = strrchr (name, '\\');
	if (result == NULL)
		return (char *)name;

	return result + 1;
}


char *roadmap_path_remove_extension (const char *name)
{
	char *result;
	char *p;


	result = strdup(name);
	roadmap_check_allocated(result);

	p = roadmap_path_skip_directories (result);
	p = strrchr (p, '.');
	if (p != NULL) *p = 0;

	return result;
}


const char *roadmap_path_user (void)
{
	static char *RoadMapUser = NULL;

	if (RoadMapUser == NULL) {
		WCHAR path_unicode[MAX_PATH];
		char *path;
		char *tmp;
		/* We don't have a user directory so we'll use the executable path */
		GetModuleFileName(NULL, path_unicode,
			sizeof(path_unicode)/sizeof(path_unicode[0]));
		path = ConvertToANSI(path_unicode, CP_ACP);
		tmp = strrchr (path, '/');
		if (tmp == NULL)
			tmp = strrchr (path, '\\');
		if (tmp != NULL) {
			*tmp = '\0';
		}
		RoadMapUser = path;
	}
	return RoadMapUser;
}


const char *roadmap_path_trips (void)
{   
	static char  RoadMapDefaultTrips[] = "trips";
	static char *RoadMapTrips = NULL;

	if (RoadMapTrips == NULL) {

		RoadMapTrips =
			roadmap_path_cat (roadmap_path_user(), RoadMapDefaultTrips);

		roadmap_path_create(RoadMapTrips);
	}
	return RoadMapTrips;
}


/* Path lists operations. -------------------------------------------------- */

void roadmap_path_set (const char *name, const char *path)
{
	int i;
	int count;
	int length;
	int expand_length;
	const char *item;
	const char *expand;
	const char *next_item;

	RoadMapPathList path_list = roadmap_path_find (name);


	if (path_list == NULL) {
		roadmap_log(ROADMAP_FATAL, "unknown path set '%s'", name);
	}

	while (*path == ',') path += 1;
	if (*path == 0) return; /* Ignore empty path: current is better. */


	if (path_list->items != NULL) {

		/* This replaces a path that was already set. */

		for (i = path_list->count-1; i >= 0; --i) {
			free (path_list->items[i]);
		}
		free (path_list->items);
	}


	/* Count the number of items in this path string. */

	count = 0;
	item = path;
	while (item != NULL) {
		item = strchr(item, ',');
		if (item) {
			item++;
			if (!*item) item = NULL;
		}
		count += 1;
	}

	path_list->items = calloc (count, sizeof(char *));
	roadmap_check_allocated(path_list->items);


	/* Extract and expand each item of the path.
	*/
	for (i = 0, item = path; item != NULL; item = next_item) {

		next_item = strchr (item, ',');

		expand = "";
		expand_length = strlen(expand);

		if (next_item == NULL) {
			length = strlen(item);
		} else {
			length = next_item - item;
		}

		path_list->items[i] = malloc (length + expand_length + 1);
		roadmap_check_allocated(path_list->items[i]);

		strcpy (path_list->items[i], expand);
		strncat (path_list->items[i], item, length);

		(path_list->items[i])[length+expand_length] = 0;

		++i;

		if (next_item) {
			next_item++;
			if (!*next_item) next_item = NULL;
		}
	}
	path_list->count = i;
}


const char *roadmap_path_first (const char *name)
{
	RoadMapPathList path_list = roadmap_path_find (name);

	if (path_list == NULL) {
		roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
	}

	if (path_list->count > 0) {
		return path_list->items[0];
	}

	return NULL;
}


const char *roadmap_path_next  (const char *name, const char *current)
{
	int i;
	RoadMapPathList path_list = roadmap_path_find (name);


	for (i = 0; i < path_list->count-1; ++i) {

		if (path_list->items[i] == current) {
			return path_list->items[i+1];
		}
	}

	return NULL;
}


const char *roadmap_path_last (const char *name)
{
	RoadMapPathList path_list = roadmap_path_find (name);

	if (path_list == NULL) {
		roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
	}

	if (path_list->count > 0) {
		return path_list->items[path_list->count-1];
	}
	return NULL;
}


const char *roadmap_path_previous (const char *name, const char *current)
{
	int i;
	RoadMapPathList path_list = roadmap_path_find (name);

	for (i = path_list->count-1; i > 0; --i) {

		if (path_list->items[i] == current) {
			return path_list->items[i-1];
		}
	}
	return NULL;
}

/* This function always return a hardcoded default location,
 * which is the recommended location for these objects.
 */
const char *roadmap_path_preferred (const char *name)
{
   RoadMapPathList path_list = roadmap_path_find (name);

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   return path_list->preferred;
}

void roadmap_path_create (const char *path)
{
	LPWSTR path_unicode = ConvertToUNICODE(path);
	CreateDirectory(path_unicode, NULL);
	free(path_unicode);
}


static char *RoadMapPathEmptyList = NULL;

char **roadmap_path_list (const char *path, const char *extension)
{
	WIN32_FIND_DATA wfd;
	WCHAR strPath[MAX_PATH];
	HANDLE hFound;
	LPWSTR path_unicode = ConvertToUNICODE(path);
	LPWSTR ext_unicode = ConvertToUNICODE(extension);
	int   count;
	char **result;
	char **cursor;

	_snwprintf(strPath, MAX_PATH, TEXT("%s/*%s"), path_unicode, ext_unicode);

	free(path_unicode);
	free(ext_unicode);

	hFound = FindFirstFile(strPath, &wfd);
	if (hFound == INVALID_HANDLE_VALUE) return &RoadMapPathEmptyList;

	count = 1;
	while(FindNextFile(hFound, &wfd)) ++count;
	FindClose(hFound);

	cursor = result = calloc (count+1, sizeof(char *));
	roadmap_check_allocated (result);

	hFound = FindFirstFile(strPath, &wfd);

	if (hFound == INVALID_HANDLE_VALUE) return &RoadMapPathEmptyList;

	do {
		if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			char *name = ConvertToANSI(wfd.cFileName, CP_ACP);
			*(cursor++) = name;
		}
	} while (FindNextFile(hFound, &wfd));

	*cursor = NULL;

	return result;
}


void roadmap_path_list_free (char **list)
{
	char **cursor;

	if ((list == NULL) || (list == &RoadMapPathEmptyList)) return;

	for (cursor = list; *cursor != NULL; ++cursor) {
		free (*cursor);
	}
	free (list);
}


void roadmap_path_free (const char *path)
{
	free ((char *) path);
}


const char *roadmap_path_search_icon (const char *name)
{
	static char result[256];

	sprintf (result, "%s/icons/rm_%s.png", roadmap_path_user(), name);
	if (roadmap_file_exists(NULL, result)) return result;

	sprintf (result, "/Storage Card/Roadmap/icons/rm_%s.png", name);
	if (roadmap_file_exists(NULL, result)) return result;

	return NULL; /* Not found. */
}


int roadmap_path_is_full_path (const char *name)
{
	return name[0] == '/' || name[0] == '\\';
}


int roadmap_path_is_directory (const char *name)
{
   LPWSTR path_unicode = ConvertToUNICODE(name);
   DWORD fa = GetFileAttributes(path_unicode);
   free(path_unicode);
   return fa && FILE_ATTRIBUTE_DIRECTORY;
}

const char *roadmap_path_skip_separator (const char *name)
{

   if ((*name == '/') || (*name == '\\')) {
      return name + 1;
   }
   return NULL; // Not a valid path separator.
}

const char *roadmap_path_temporary (void)
{
	return roadmap_path_user();
}

