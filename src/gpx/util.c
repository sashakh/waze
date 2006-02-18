/*
    Misc utilities.

    Copyright (C) 2002-2005 Robert Lipe, robertlipe@usa.net

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

#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>


void *
xmalloc(size_t size)
{
        void *obj = malloc(size);

        if (!obj) {
                fatal("gpsbabel: Unable to allocate %d bytes of memory.\n", (int)size);
        }

        return obj;
}

void *
xcalloc(size_t nmemb, size_t size)
{
        void *obj = calloc(nmemb, size);

        if (!obj) {
                fatal("gpsbabel: Unable to allocate %d bytes of memory.\n", (int)size);
        }

        return obj;
}

void
xfree( void *mem )
{
        free(mem);
}

char *
xstrdup(const char *s)
{
        char *o = s ? strdup(s) : strdup("");

        if (!o) {
                fatal("gpsbabel: Unable to allocate %d bytes of memory.\n", (int)strlen(s));
        }

        return o;
}

/*
 * Duplicate at most sz bytes in str.
 */
char *
xstrndup(const char *str, size_t sz)
{
        size_t newlen;
        char *newstr;

        newlen = strlen(str);
        if (newlen > sz) {
                newlen = sz;
        }

        newstr = (char *) xmalloc(newlen + 1);
        memcpy(newstr, str, newlen);    
        newstr[newlen] = 0;

        return newstr;
}

/*
 * Lazily trim whitespace (though not from allocated version) 
 * while copying.
 */
char *
xstrndupt(const char *str, size_t sz)
{
        size_t newlen;
        char *newstr;

        newlen = strlen(str);
        if (newlen > sz) {
                newlen = sz;
        }

        newstr = (char *) xmalloc(newlen + 1);
        memcpy(newstr, str, newlen);
        newstr[newlen] = '\0';
        rtrim(newstr);

        return newstr;
}

void *
xrealloc(void *p, size_t s)
{
        char *o = (char *) realloc(p,s);

        if (!o) {
                fatal("gpsbabel: Unable to realloc %d bytes of memory.\n", (int)s);
        }

        return o;
}

/*
* For an allocated string, realloc it and append 's'
*/
char *
xstrappend(char *src, const char *newd)
{
        size_t newsz;

        if (!src) {
                return xstrdup(newd);
        }

        newsz = strlen(src) + strlen(newd) + 1;
        src = xrealloc(src, newsz);
        strcat(src, newd);

        return src;
}

void 
rtrim(char *s)
{
        char *t = s;

        if (!s || !*s) {
                return;
        }

        while (*s) {
                s++;
        }

        s--;
        while ((s >= t) && isspace (*s)) {
                *s = 0;
                s--;
        }
}


void
printposn(const int c, int is_lat)
{
        char d;
        if (is_lat) {
                if (c < 0) d = 'S'; else d = 'N';
        } else {
                if (c < 0) d = 'W'; else d = 'E';
        }
        printf("%f%c ", (double)abs(c)/1000000.0, d);
}


/*
        mkgmtime -- convert tm struct in UTC to time_t

        works just like mktime but without all the mucking
        around with timezones and daylight savings

        obsoletes get_tz_offset()

        Borrowed from lynx GPL source code
        http://lynx.isc.org/release/lynx2-8-5/src/mktime.c

        Written by Philippe De Muyter <phdm@macqel.be>.
*/

time_t
mkgmtime(struct tm *t)
{
        short  month, year;
        time_t result;
        static int      m_to_d[12] =
                {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

        month = t->tm_mon;
        year = t->tm_year + month / 12 + 1900;
        month %= 12;
        if (month < 0)
        {
                year -= 1;
                month += 12;
        }
        result = (year - 1970) * 365 + m_to_d[month];
        if (month <= 1)
                year -= 1;
        result += (year - 1968) / 4;
        result -= (year - 1900) / 100;
        result += (year - 1600) / 400;
        result += t->tm_mday;
        result -= 1;
        result *= 24;
        result += t->tm_hour;
        result *= 60;
        result += t->tm_min;
        result *= 60;
        result += t->tm_sec;
        return(result);
}


/*
 * A wrapper for time(2) that allows us to "freeze" time for testing.
 */
time_t
current_time(void)
{
        if (getenv("GPSBABEL_FREEZE_TIME")) {
                return 0;
        }

        return time(NULL);
}



typedef struct {
        const char * text;
        const char * entity;
        int  not_html;
} entity_types;

static 
entity_types stdentities[] =  {
        { "&",  "&amp;", 0 },
        { "'",  "&apos;", 1 },
        { "<",  "&lt;", 0 },
        { ">",  "&gt;", 0 },
        { "\"", "&quot;", 0 },
        { NULL, NULL, 0 }
};


void utf8_to_int( const char *cp, int *bytes, int *value ) 
{
        if ( (*cp & 0xe0) == 0xc0 ) {
                if ( (*(cp+1) & 0xc0) != 0x80 ) goto dodefault;
                *bytes = 2;
                *value = ((*cp & 0x1f) << 6) | 
                        (*(cp+1) & 0x3f); 
        }
        else if ( (*cp & 0xf0) == 0xe0 ) {
                if ( (*(cp+1) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+2) & 0xc0) != 0x80 ) goto dodefault;
                *bytes = 3;
                *value = ((*cp & 0x0f) << 12) | 
                        ((*(cp+1) & 0x3f) << 6) | 
                        (*(cp+2) & 0x3f); 
        }
        else if ( (*cp & 0xf8) == 0xf0 ) {
                if ( (*(cp+1) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+2) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+3) & 0xc0) != 0x80 ) goto dodefault;
                *bytes = 4;
                *value = ((*cp & 0x07) << 18) | 
                        ((*(cp+1) & 0x3f) << 12) | 
                        ((*(cp+2) & 0x3f) << 6) | 
                        (*(cp+3) & 0x3f); 
        }
        else if ( (*cp & 0xfc) == 0xf8 ) {
                if ( (*(cp+1) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+2) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+3) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+4) & 0xc0) != 0x80 ) goto dodefault;
                *bytes = 5;
                *value = ((*cp & 0x03) << 24) | 
                        ((*(cp+1) & 0x3f) << 18) | 
                        ((*(cp+2) & 0x3f) << 12) | 
                        ((*(cp+3) & 0x3f) << 6) |
                        (*(cp+4) & 0x3f); 
        }
        else if ( (*cp & 0xfe) == 0xfc ) {
                if ( (*(cp+1) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+2) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+3) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+4) & 0xc0) != 0x80 ) goto dodefault;
                if ( (*(cp+5) & 0xc0) != 0x80 ) goto dodefault;
                *bytes = 6;
                *value = ((*cp & 0x01) << 30) | 
                        ((*(cp+1) & 0x3f) << 24) | 
                        ((*(cp+2) & 0x3f) << 18) | 
                        ((*(cp+3) & 0x3f) << 12) |
                        ((*(cp+4) & 0x3f) << 6) |
                        (*(cp+5) & 0x3f); 
        }
        else {
dodefault:
                *bytes = 1;
                *value = (unsigned char)*cp;
        }
}


static 
char * 
entitize(const char * str, int is_html) 
{
        int elen, ecount, nsecount;
        entity_types *ep;
        const char * cp;
        char * p, * tmp, * xstr;
        char tmpsub[20];

        int bytes = 0;
        int value = 0;
        ep = stdentities;
        elen = ecount = nsecount = 0;

        /* figure # of entity replacements and additional size. */
        while (ep->text) {
                cp = str;
                while ((cp = strstr(cp, ep->text)) != NULL) {
                        elen += strlen(ep->entity) - strlen(ep->text);
                        ecount++;
                        cp += strlen(ep->text);
                }
                ep++;
        }
        
        /* figure the same for other than standard entities (i.e. anything
         * that isn't in the range U+0000 to U+007F */
        for ( cp = str; *cp; cp++ ) {
                if ( *cp & 0x80 ) {
                        utf8_to_int( cp, &bytes, &value );
                        cp += bytes-1;
                        elen += sprintf( tmpsub, "&#x%x;", value ) - bytes;
                        nsecount++;     
                }
        }

        /* enough space for the whole string plus entity replacements, if any */
        tmp = xcalloc((strlen(str) + elen + 1), 1);
        strcpy(tmp, str);

        /* no entity replacements */
        if (ecount == 0 && nsecount == 0)
                return (tmp);

        if ( ecount != 0 ) {    
                for (ep = stdentities; ep->text; ep++) {
                        p = tmp;
                        if (is_html && ep->not_html)  {
                                continue;
                        }
                        while ((p = strstr(p, ep->text)) != NULL) {
                                elen = strlen(ep->entity);

                                xstr = xstrdup(p + strlen(ep->text));

                                strcpy(p, ep->entity);
                                strcpy(p + elen, xstr);

                                xfree(xstr);

                                p += elen;
                        }  
                }
        }

        if ( nsecount != 0 ) {
                p = tmp;
                while (*p) {
                        if ( *p & 0x80 ) {
                                utf8_to_int( p, &bytes, &value );
                                if ( p[bytes] ) {
                                        xstr = xstrdup( p + bytes );
                                }
                                else {
                                        xstr = NULL;
                                }
                                sprintf( p, "&#x%x;", value );
                                p = p+strlen(p);
                                if ( xstr ) {
                                        strcpy( p, xstr );
                                        xfree(xstr);
                                }
                        }
                        else {
                                p++;
                        }
                }
        }       
        return (tmp);
}

/*
 * Public callers for the above to hide the absence of &apos from HTML
 */

char * xml_entitize(const char * str) 
{
        return entitize(str, 0);
}

void
write_xml_entity(FILE *ofd, const char *indent,
                 const char *tag, const char *value)
{
        char *tmp_ent = xml_entitize(value);
        fprintf(ofd, "%s<%s>%s</%s>\n", indent, tag, tmp_ent, tag);
        xfree(tmp_ent);
}

void
write_optional_xml_entity(FILE *ofd, const char *indent,
                          const char *tag, const char *value)
{
        if (value && *value)
                write_xml_entity(ofd, indent, tag, value);
}


void
xml_fill_in_time(char *time_string, const time_t timep, int long_or_short)
{
        struct tm *tm = gmtime(&timep);
        char *format;
        
        if (!tm) {
		*time_string = 0;
                return;
	}
        
        if (long_or_short == XML_LONG_TIME)
                format = "%02d-%02d-%02dT%02d:%02d:%02dZ";
        else
                format = "%02d%02d%02dT%02d%02d%02dZ";
        sprintf(time_string, format,
                tm->tm_year+1900, 
                tm->tm_mon+1, 
                tm->tm_mday, 
                tm->tm_hour, 
                tm->tm_min, 
                tm->tm_sec);
}

void
xml_write_time(FILE *ofd, const time_t timep, const char *indent, char *elname)
{
        char time_string[64];
        xml_fill_in_time(time_string, timep, XML_LONG_TIME);
	if (*time_string) {
		fprintf(ofd, "%s<%s>%s</%s>\n",
			indent,
			elname,
			time_string,
			elname
		);
	}

}


