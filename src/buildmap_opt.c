/*
 * This version of Stig Brautaset's command options parser is part
 * of RoadMap.  The code has been modified somewhat from the original.
 * The original is available here:
 *    http://code.brautaset.org/options/
 * The RoadMap version merges the opt_init() and opt_parse() routines
 * into one API call from the client, and moves the declaration of
 * the option type from the call to opt_parse() into the structure of
 * option declarations.
 * Modifications by Paul Fox, November 2007
 */
/**
 * @file
 * @brief `options' library source
 * @author Stig Brautaset
 *
 * Copyright (C) 2003,2004 Stig Brautaset
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/**
 * @defgroup options The `options' library
 * @brief An simple-to-use options-parsing library
 *
 * `options' is a small and easy to use option-parsing library. At the
 * same time, it is fairly powerful. 
 *
 * @{
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "buildmap_opt.h"


/** Determine max value */
#define max(a, b) (a) > (b) ? a : b
/** Detect end of properly declared opt_defs array */
#define isopt(o) (o && ((o->lopt && *o->lopt) || (o->sopt && *o->sopt)))

/**
 * @brief Internal representation of options
 */
struct opt {
	struct opt *next;	/**< Pointer to next in list */
	char *lopt;		/**< Long option */
	char *sopt;		/**< Short option */
	char type;		/**< Option type */
	
	/** Option value can have several types */ 
	union {			
		int ival;	/**< Value if type == 0 */
		char *str;	/**< Value if type == 1 */
		char **array;	/**< Value if type == 2 */
	} u;
};

/* Declaration of Internal functions */
static char *opt_shift(int *argc, char **argv);
static void opt_shift_short(int *argc, char **argv);
static struct opt *opt_find(struct opt *opts, char *opt, int islong, char **eq);

struct opt *global_opts;

/**
 * Return pointer to error string based on error code returned.
 * @param error the error code to decode
 * @return A pointer to a `friendly' error message string is returned.
 */
const char *opt_strerror(int error)
{ 
	const char *str;
	switch (error) {
		case opt_wantarg:
			str = "option wants argument";
			break;
		case opt_badarg:
			str = "improper format for argument";
			break;
		case opt_dontexist:
			str = "option does not exist";
			break;
		case opt_memfail:
			str = "memory allocation failure";
			break;
		case opt_utype:
			str = "underlying type not supported";
			break;
		case opt_type:
			str = "requested type not supported";
			break;
		default:
			str = "unknown error code";
			break;
	}
	return str;
}

#if NEEDED
/**
 * Duplicate a string.
 * @param src the string to be duplicated
 * @return Pointer to the duplicated string (or NULL on error)
 */
char *opt_strdup(char *src)
{
	int len;
	char *dst;

	len = strlen(src);
	dst = malloc(len + 1);

	if (dst) 
		(void)memcpy(dst, src, len + 1);
	return dst;
}
#endif

/**
 * Free an option structure.
 * @param opts pointer to structure to free
 */
void opt_free(struct opt *opts)
{
	struct opt *opt;
	while (opts) {
		opt = opts->next;
		free(opts);
		opts = opt;
	}
}

/**
 * Retrieve the value of an option.
 * @param opts internal state
 * @param opt option to retrieve value for
 * @param fmt format of returned option (currently "int" or "str")
 * @param val pointer to variable of correct type to hold value
 * @return Zero on success, non-zero on error.
 *
 * @note This function is slightly asymetric in that it returns (through
 * @p val) _values_ for non-strings but a pointer to the string for
 * strings. If a copy of the string is desired, strdup() can be used.
 */
int opt_val(char *opt, void *val)
{
	struct opt *o;
	int retval;
	int *ival = val;
	char **str = val;
	char *end;
	
	/* a matching long option takes presedense over a short */
	o = opt_find(global_opts, opt, 1, NULL);
	if (!o)
		o = opt_find(global_opts, opt, 0, NULL);

	if (!o) {
		retval = opt_dontexist;
		if (o->type == opt_string) {
		    *str = NULL;
		}
	} else {
		retval = 0;
		switch (o->type) {
		case opt_flag:
		    *ival = o->u.ival;
		    break;
		case opt_int:
		    *ival = strtol(o->u.str, &end, 0);
		    if (*end)
			retval = opt_badarg;
		    break;
		case opt_string:
		    *str = strdup(o->u.str);
		    if (!*str)
			    retval = opt_memfail;
		    break;
		default:
		    retval = opt_type;
		}
	}
	return retval;
}

/**
 * Print options and help messages.
 *
 * @param options structure holding options to describe
 * @param defs whether or not to print default values
 *
 * @return Zero on success, a non-zero error code otherwise.
 *
 * @todo Find out if this function instead should take a `struct opt *'.
 * Then we can print the current values as well as the default values.
 */
int opt_desc(struct opt_defs *options, int defs)
{
	int retval, longest;
	struct opt_defs *o;
	enum {PADDING = 5};

	longest = retval = 0;
	for (o = options; isopt(o); o++) {
		int llen = o->lopt ? strlen(o->lopt) : 0;
 		int slen = o->sopt ? strlen(o->sopt) : 0;
	
		longest = max(longest, llen + slen + PADDING);
	}

	for (o = options; isopt(o); o++) {
		char s[100];
		char *defval = "";

		if (o->lopt && *o->lopt && o->sopt && *o->sopt) {
			sprintf(s, "--%s, -%.1s", o->lopt, o->sopt);
		} else if (o->lopt && *o->lopt){
			sprintf(s, "--%s", o->lopt);
		} else if (o->sopt && *o->sopt){
			sprintf(s, " -%.1s", o->sopt);
		} else {
			s[0] = '\0';
			return opt_deferror;
		}

		switch (o->type) {
		case opt_flag:
			defval = "0";
			break;
		case opt_int:
		case opt_string:
			defval = o->deflt;
			break;
		default:
			return opt_utype;
		}

		printf(" %-*s \t%s\n", longest, s, o->help);
		if (defs) {
			printf(" %*s \tdefault: ", longest, "");
			switch (o->type) {
			case opt_flag:
			case opt_int:
				printf("%s\n", defval);
				break;
			case opt_string:
				printf("\"%s\"\n", defval);
				break;
			}
		}
				
	}

	return retval;
}


/**
 * Initialise an opt structure.
 * @param options definition of options 
 * @return boolean for error
 */
static int opt_init(struct opt_defs *options)
{
	struct opt *opt;
	struct opt_defs *o;

	if (global_opts) {
		opt_free(global_opts);
	}

	global_opts = NULL;
	for (o = options; isopt(o); o++) {
		opt = malloc(sizeof *opt);
		if (!opt) {
			break;
		}
		opt->next = NULL;
		opt->lopt = o->lopt;
		opt->sopt = o->sopt;
		opt->type = o->type;

		switch (opt->type) {
			case opt_flag:
				opt->u.ival = 0;
				break;
			case opt_int:
			case opt_string:
				opt->u.str = o->deflt;
				break;
			default:
				goto errout;
		}
		opt->next = global_opts;
		global_opts = opt;

	}

    errout:
	if (isopt(o)) {
		opt_free(global_opts);
		global_opts = NULL;
	}

	return global_opts != 0;
}

/**
 * Parse argv and extract options.
 *
 * @param opts pointer to structure to populate with options
 * @param argc number of elements in argv -- changes on return
 * @param argv argv array as passed to main() -- changes on return
 * @param mixed can options and positional arguments be intermixed?
 *
 * @return Zero on success, errorcode otherwise.
 */
// int opt_parse(struct opt *opts, int *argc, char **argv, int mixed)
int opt_parse(struct opt_defs *options, int *argc, char **argv, int mixed)
{
	int i;

	if (!opt_init(options))
		return opt_deferror;

	i = 1;
	while (i < *argc) {
		if (argv[i][0] != '-' || (argv[i][0] == '-' && !argv[i][1])) {
			/* not an option (no leading '-' or '-' on its own) */
			if (!mixed) {
				return opt_ok;
			} else {
				/* skip */
				i++;
			}
		} else if (argv[i][1] == '-' && !argv[i][2]) {
			/* let '--' by itself terminate option processing */
			(void)opt_shift(argc, &argv[i]);
			return opt_ok;
		} else {
			int l;
			struct opt *o;
			char *eq;

			l = argv[i][1] == '-';

			o = opt_find(global_opts, argv[i] + 1 + l, l, &eq);
			if (!o)
				return opt_dontexist;

			if (l) {
				if (eq)
				    argv[i] = eq + 1;
				else
				    (void)opt_shift(argc, &argv[i]);
				switch(o->type) {
				case opt_flag:
					o->u.ival++;
					break;
				case opt_int:
				case opt_string:
					if (i < *argc) {
					    o->u.str = opt_shift(argc, &argv[i]);
					} else {
					    return opt_wantarg;
					}
					break;
				}
			} else {
				int c;

				c = *argc;
				opt_shift_short(argc, &argv[i]);

				switch(o->type) {
				case opt_flag:
					o->u.ival++;
					break;
				case opt_int:
				case opt_string:
					if (i < *argc) {
						if (c == *argc)
							o->u.str = 1 + opt_shift(argc, &argv[i]);
						else
							o->u.str = opt_shift(argc, &argv[i]);
					} else {
						return opt_wantarg;
					}
					break;
				}
			}
		}
	}

	return opt_ok;
}

/**
 * Find option description for current option.
 *
 */
static struct opt *opt_find(struct opt *opts, char *opt, int islong, char **eq)
{
	struct opt *o;
	int found;
	char *e;

	e = NULL;
	found = 0;
	for (o = opts; isopt(o); o = o->next) {
		if (islong && o->lopt) {
			found = !strcmp(o->lopt, opt);
			if (!found) {
				e = strchr(opt, '=');
				if (e)
					found = !strncmp(o->lopt, opt, e - opt);
			}
		} else if (!islong && o->sopt) {
			found = o->sopt[0] == opt[0];
		}
		if (found)
			break;
	}
	if (eq) *eq = e;
	return found ? o : NULL; 
}

/**
 * Return current array member and shift left to fill its position. 
 * @param argc current number of elements in argv
 * @param argv pointer to position in argv array to shift into
 * @return pointer to member removed from argv array
 */
static char *opt_shift(int *argc, char **argv)
{
	int i;
	char *opt;

	opt = *argv;
	(*argc)--;

	for (i = 0; argv[i] && i < *argc; i++) {
		argv[i] = argv[i + 1];
	}
	argv[i] = NULL;

	return opt;
}

/**
 * Shift short option left one place.
 * @param argc current number of elements in argv
 * @param argv pointer to position in argv array to shift into
 */
static void opt_shift_short(int *argc, char **argv)
{
	if (!argv[0][2])
		(void)opt_shift(argc, argv);
	else
		memmove(argv[0] + 1, argv[0] + 2, strlen(argv[0] + 1));
}

/* @} */
