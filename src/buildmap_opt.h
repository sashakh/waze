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
 * @brief `options' library header 
 * @author Stig Brautaset
 *
 * `options' is a small and easy to use option-parsing library.
 * At the same time, it is fairly powerful. 
 *
 * Copyright (C) 2003,2004 Stig Brautaset
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef options__opt_h
#define options__opt_h

#include <stddef.h>	/* for NULL */

struct opt;		/* Forward declaration for prototypes */

/**
 * Error codes returned by various functions.
 */
enum {
	opt_ok = 0,		/**< No error */
	opt_wantarg,		/**< Option wants arg but gets none */
	opt_badarg,		/**< Option wants arg but wrong format */
	opt_dontexist,		/**< Option does not exist */
	opt_deferror,		/**< Error in option definition */
	opt_memfail,		/**< Memory allocation failure */
	opt_utype,		/**< Underlying type not supported */
	opt_type,		/**< Requested type not supported */
};

/** 
 * @brief Option definitions
 *
 * An array of structures of this type is used to tell `options'
 * what options (and their types) to expect.
 *
 * The type should be 0 if the option doesn't take an argument, 1
 * if it does. 
 *
 * Example:
 * @code
 * struct opt_defs options[] = {
 *	{"help", "h", opt_flag, "",	"Print this help"},
 *	{"name", "", opt_string, "lusr","Name of user"},
 *	{"", "d", opt_flag, "",		"Increment debug level"},
 *	{"num", "n", opt_int, "10",	"Integer arg with 10 as	default"},
 *	OPT_DEFS_END
 * };
 * @endcode
 */ 

typedef enum {
	opt_flag = 1,
	opt_int,
	opt_string,
} opt_types;

struct opt_defs {
	char *lopt;		/**< Long option */
	char *sopt;		/**< Short option */
	opt_types type;		/**< Takes argument? (0=no, 1=yes) */
	char *deflt;		/**< Default value */
	char *help;		/**< Help message */
};

/**
 * Must be used as last option in opt_defs array. 
 * @see opt_defs
 */
#define OPT_DEFS_END {NULL, NULL, 0, NULL, NULL}

/* automatically generated protypes */
/* opt.c */
const char *opt_strerror(int error);
// char *opt_strdup(char *src);
void opt_free(struct opt *opts);
int opt_val(char *opt, void *val);
int opt_desc(struct opt_defs *options, int defs);
// struct opt *opt_init(struct opt_defs *options);
int opt_parse(struct opt_defs *options, int *argc, char **argv, int mixed);

#endif /* options__opt_h */

