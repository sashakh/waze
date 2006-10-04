/*
    Copyright (C) 2002, 2003, 2004, 2005  Robert Lipe, robertlipe@usa.net

    Modified for use with RoadMap, Paul Fox, 2005

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA

 */
#ifndef gpsbabel_defs_h_included
#define gpsbabel_defs_h_included
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include "roadmap_list.h"
#include "gbtypes.h"

/*
 * Snprintf is in SUS (so it's in most UNIX-like substance) and it's in 
 * C99 (albeit with slightly different semantics) but it isn't in C89.   
 * This tweaks allows us to use snprintf on the holdout.
 */
#if __WIN32__
#  define snprintf _snprintf
#  define vsnprintf _vsnprintf
#endif

/* Turn off numeric conversion warning */
#if __WIN32__
#pragma warning(disable:4244)
#endif

/* 
 *  Toss in some GNU C-specific voodoo for checking.
 */
#if __GNUC__ 
#  define PRINTFLIKE(x,y) __attribute__ ((__format__ (__printf__, (x), (y))))
#  define NORETURN void __attribute__ ((__noreturn__))
#else
#  define PRINTFLIKE(x,y)
#  define NORETURN void
#endif

/* Short or Long XML Times */
#define XML_SHORT_TIME 1
#define XML_LONG_TIME 2

/*
 * Common definitions.   There should be no protocol or file-specific
 * data in this file.
 */

#if ROADMAP_UNNEEDED
typedef enum {
        fix_unknown=-1,
        fix_none=0,
        fix_2d=1,       
        fix_3d,
        fix_dgps,
        fix_pps
} fix_type;

/*
 * Extended data if waypoint happens to represent a geocache.  This is 
 * totally voluntary data...
 */

typedef enum {
        gt_unknown = 0 ,
        gt_traditional,
        gt_multi,
        gt_virtual,
        gt_letterbox,
        gt_event,
        gt_suprise,
        gt_webcam,
        gt_earth,
        gt_locationless,
        gt_benchmark, /* Extension to Groundspeak for GSAK */
        gt_cito
} geocache_type;

typedef enum {
        gc_unknown = 0,
        gc_micro,
        gc_other,
        gc_regular,
        gc_large,
        gc_virtual,
        gc_small
} geocache_container;

typedef struct {
        int is_html;
        char *utfstring;
} utf_string;

typedef struct {
        int id; /* The decimal cache number */
        geocache_type type:5;
        geocache_container container:4;
        unsigned int diff:6; /* (multiplied by ten internally) */
        unsigned int terr:6; /* (likewise) */
        time_t exported;
        time_t last_found;
        char *placer; /* Placer name */
        char *hint; /* all these UTF8, XML entities removed, May be not HTML. */
        utf_string desc_short;
        utf_string desc_long; 
} geocache_data ;
#endif // ROADMAP_UNNEEDED

typedef struct xml_tag {
        char *tagname;
        char *cdata;
        int cdatalen;
        char *parentcdata;
        int parentcdatalen;
        char **attributes;
        struct xml_tag *parent;
        struct xml_tag *sibling;
        struct xml_tag *child;
} xml_tag ;

typedef void (*fs_destroy)(void *);
typedef void (*fs_copy)(void **, void *);
typedef struct format_specific_data {
        long type;
        struct format_specific_data *next;
        
        fs_destroy destroy;
        fs_copy copy;
} format_specific_data;

format_specific_data *fs_chain_copy( format_specific_data *source );
void fs_chain_destroy( format_specific_data *chain );
format_specific_data *fs_chain_find( format_specific_data *chain, long type );
void fs_chain_add( format_specific_data **chain, format_specific_data *data );

typedef struct fs_xml {
        format_specific_data fs;
        xml_tag *tag;
} fs_xml;

fs_xml *fs_xml_alloc( long type );

/* we only have one type of data, so only the first of these is
 * relevant */
#define FS_GPX 0x67707800L
#define FS_AN1W 0x616e3177L
#define FS_AN1L 0x616e316cL
#define FS_AN1V 0x616e3176L

/*
 * Misc bitfields inside struct waypoint;
 */
typedef struct {
        unsigned int shortname_is_synthetic:1;
} wp_flags;

/* this simply avoids everything in the gpx directory needing to
* include roadmap_types.h */
#ifndef ROADMAP
typedef struct {
   int longitude;
   int latitude;
} RoadMapPosition;
#endif


/*
 * This is a waypoint, as stored in the GPSR.   It tries to not 
 * cater to any specific model or protocol.  Anything that needs to
 * be truncated, edited, or otherwise trimmed should be done on the
 * way to the target.
 */

typedef struct {
        queue Q;                        /* Master waypoint q.  Not for use
                                           by modules. */
        /*
         * RoadMap's representation of lat/long position.
         */
        RoadMapPosition pos;


        /* shortname is a waypoint name as stored in receiver.  It should
         * strive to be, well, short, and unique.   Enforcing length and
         * character restrictions is the job of the output.   A typical
         * minimum length for shortname is 6 characters for NMEA units,
         * 8 for Magellan and 10 for Vista.   These are only guidelines.
         */
        char *shortname;

        time_t creation_time;   /* standardized in UTC/GMT */

        wp_flags wpt_flags;

        /* track information -- these three are usually only present
         * for data gathered "on the go".
         */
        int altitude;   /* centimeters */
        float speed;    /* Optional: meters per second. */
        float course;   /* Optional: degrees true */

        /*
         * description is typically a human readable description of the 
         * waypoint.   It may be used as a comment field in some receivers.
         * These are probably under 40 bytes, but that's only a guideline.
         * This text comes from the GPX <cmt> tag, and may be sent to
         * the GPS unit.
         */
        char *description;

        /*
         * notes are relatively long - over 100 characters - prose associated
         * with the above.   Unlike shortname and description, these are never
         * used to compute anything else and are strictly "passed through".
         * This text comes from the GPX <desc> tag, and is strictly for
         * the user.  RoadMap currently doesn't use this.
         */
        char *notes;

#if ROADMAP_UNNEEDED
        const char *icon_descr;

        char *url;
        char *url_link_text;

        /* Optional dilution of precision:  positional, horizontal, veritcal.  
         * 1 <= dop <= 50 
         */ 
        float hdop;             
        float vdop;             
        float pdop;             

        fix_type fix;   /* Optional: 3d, 2d, etc. */
        int  sat;       /* Optional: number of sats used for fix */

        geocache_data gc_data;
#endif

        format_specific_data *fs;
} waypoint;

typedef struct {
        queue Q;                /* Link onto parent list. */
        queue_head waypoint_list;    /* List of child waypoints */
        char *rte_name;
        char *rte_desc;
        int rte_num;
        int rte_waypt_ct;               /* # waypoints in waypoint list */
        int rte_is_track;       /* set externally */
        format_specific_data *fs;
} route_head;

/*
 *  Bounding box information.
 */
typedef struct {
        int max_lat;
        int max_lon;
        int min_lat;
        int min_lon;
} bounds;

typedef void (*waypt_cb) (const waypoint *);
typedef void (*route_hdr)(const route_head *);
typedef void (*route_trl)(const route_head *);
void waypt_add (queue_head *q, waypoint *);
waypoint * waypt_dupe (const waypoint *);
waypoint * waypt_new(void);
void waypt_del (waypoint *);
void waypt_free (waypoint *);
void waypt_iterator(queue_head *, waypt_cb);
void waypt_init_bounds(bounds *bounds);
int waypt_bounds_valid(bounds *bounds);
void waypt_add_to_bounds(bounds *bounds, const waypoint *waypointp);
void waypt_compute_bounds(queue_head *qh, bounds *);
void waypt_flush_queue(queue_head *);
unsigned int waypt_count(void);
void set_waypt_count(unsigned int nc);
void set_waypt_head(queue *wh);
void free_gpx_extras (xml_tag * tag);
void xcsv_setup_internal_style(const char *style_buf);
void xcsv_read_internal_style(const char *style_buf);
waypoint * waypt_find_waypt_by_name(queue_head *qh, const char *name);
void waypt_backup(unsigned int *count, queue **head_bak);
void waypt_restore(unsigned int count, queue *head_bak);

route_head *route_head_alloc(void);
void route_add_wpt(route_head *rte, waypoint *next, waypoint *wpt, int after);
void route_add_wpt_tail(route_head *rte, waypoint *wpt);
void route_del_wpt(route_head *rte, waypoint *wpt);
void route_add(queue_head *q, route_head *rte);
void route_del(route_head *rte);
void route_reverse(const route_head *rte_hd);
waypoint * route_find_waypt_by_name(route_head *rh, const char *name);
void route_waypt_iterator(const route_head *rte, waypt_cb);
void route_iterator(queue_head *qh, route_hdr rh, route_trl rt, waypt_cb wc);
void route_free (route_head *);
void route_flush_queue( queue_head *);
route_head * route_find_route_by_name(queue_head *routes, const char *name);
unsigned int route_count(void);
unsigned int track_count(void);
void route_backup(unsigned int *count, queue **head_bak);
void route_restore(unsigned int count, queue *head_bak);
void track_backup(unsigned int *count, queue **head_bak);
void track_restore(unsigned int count, queue *head_bak);

/*
 * All shortname functions take a shortname handle as the first arg.
 * This is an opaque pointer.  Callers must not fondle the contents of it.
 */
typedef struct short_handle * short_handle;
#ifndef DEBUG_MEM 
char *mkshort (short_handle,  const char *);
void *mkshort_new_handle(void);
#else
char *MKSHORT(short_handle,  const char *, DEBUG_PARAMS);
void *MKSHORT_NEW_HANDLE(DEBUG_PARAMS);
#define mkshort( a, b) MKSHORT(a,b,__FILE__, __LINE__)
#define mkshort_new_handle() MKSHORT_NEW_HANDLE(__FILE__,__LINE__)
#endif
char *mkshort_from_wpt(short_handle h, const waypoint *wpt);
void mkshort_del_handle(short_handle *h);
void setshort_length(short_handle, int n);
void setshort_badchars(short_handle,  const char *);
void setshort_goodchars(short_handle,  const char *);
void setshort_mustupper(short_handle,  int n);
void setshort_mustuniq(short_handle,  int n);
void setshort_whitespace_ok(short_handle,  int n);
void setshort_repeating_whitespace_ok(short_handle,  int n);
void setshort_defname(short_handle, const char *s);

/*
 *  Vmem flags values.
 */
#define VMFL_NOZERO (1 << 0)
typedef struct vmem {
        void *mem;              /* visible memory object */
        size_t size;            /* allocated size of object */
} vmem_t;
vmem_t  vmem_alloc(size_t, int flags);
void    vmem_free(vmem_t*);
void    vmem_realloc(vmem_t*, size_t);


#define ARGTYPE_UNKNOWN    0x00000000
#define ARGTYPE_INT        0x00000001
#define ARGTYPE_FLOAT      0x00000002
#define ARGTYPE_STRING     0x00000003
#define ARGTYPE_BOOL       0x00000004
#define ARGTYPE_FILE       0x00000005
#define ARGTYPE_OUTFILE    0x00000006

/* REQUIRED means that the option is required to be set. 
 * See also BEGIN/END_REQ */
#define ARGTYPE_REQUIRED   0x40000000

/* HIDDEN means that the option does not appear in help texts.  Useful
 * for debugging or testing options */
#define ARGTYPE_HIDDEN     0x20000000

/* BEGIN/END_EXCL mark the beginning and end of an exclusive range of
 * options. No more than one of the options in the range may be selected 
 * or set. If exactly one must be set, use with BEGIN/END_REQ
 * Both of these flags set is just like neither set, so avoid doing that. */
#define ARGTYPE_BEGIN_EXCL 0x10000000
#define ARGTYPE_END_EXCL   0x08000000

/* BEGIN/END_REQ mark the beginning and end of a required range of 
 * options.  One or more of the options in the range MUST be selected or set.
 * If exactly one must be set, use with BEGIN/END_EXCL 
 * Both of these flags set is synonymous with REQUIRED, so use that instead
 * for "groups" of exactly one option. */
#define ARGTYPE_BEGIN_REQ  0x04000000
#define ARGTYPE_END_REQ    0x02000000 

#define ARGTYPE_TYPEMASK 0x00000fff
#define ARGTYPE_FLAGMASK 0xfffff000

typedef struct arglist {
        char *argstring;
        char **argval;
        char *helpstring;
        char *defaultvalue;
        gbuint32 argtype;
        char *minvalue;         /* minimum value for numeric options */
        char *maxvalue;         /* maximum value for numeric options */
} arglist_t;


NORETURN fatal(const char *, ...) PRINTFLIKE(1, 2);
void warning(const char *, ...) PRINTFLIKE(1, 2);

int gpx_write( FILE *ofd, queue_head *w, queue_head *r, queue_head *t);
int gpx_read(FILE *ifile, queue_head *w, queue_head *r, queue_head *t);
time_t xml_parse_time( const char *cdatastr );


void write_xml_entity(FILE *ofd, const char *indent,
                const char *tag, const char *value);
void xml_fill_in_time(char *time_string, const time_t timep, 
                int long_or_short);
void xml_write_time(FILE *ofd, const time_t timep,
                const char *indent, char *elname);
void write_optional_xml_entity(FILE *ofd, const char *indent,
                const char *tag, const char *value);
        

void *xcalloc(size_t nmemb, size_t size);
void *xmalloc(size_t size);
void *xrealloc(void *p, size_t s);
void xfree(void *mem);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);
char *xstrndupt(const char *s, size_t n);
char *xstrappend(char *src, const char *addon);

void rtrim(char *s);
time_t mkgmtime(struct tm *t);
time_t current_time(void);
char * xml_entitize(const char * str);

/* 
 * Character encoding transformations.
 */

#define CET_NOT_CONVERTABLE_DEFAULT '$'
#define CET_CHARSET_ASCII       "US-ASCII"
#define CET_CHARSET_UTF8        "UTF-8"
#define CET_CHARSET_MS_ANSI     "MS-ANSI"
#define CET_CHARSET_LATIN1      "ISO-8859-1"

#define str_utf8_to_cp1252(str) cet_str_utf8_to_cp1252((str)) 
#define str_cp1252_to_utf8(str) cet_str_cp1252_to_utf8((str))

#define str_utf8_to_iso8859_1(str) cet_str_utf8_to_iso8859_1((str)) 
#define str_iso8859_1_to_utf8(str) cet_str_iso8859_1_to_utf8((str))

/*
 * A constant for unknown altitude.   It's tempting to just use zero
 * but that's not very nice for the folks near sea level.
 */
#define unknown_alt 999999999

#endif /* gpsbabel_defs_h_included */
