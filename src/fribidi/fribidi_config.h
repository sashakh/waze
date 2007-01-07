
#define FRIBIDI_PACKAGE "fribidi"

#define FRIBIDI_VERSION "0.10.7"
#define FRIBIDI_MAJOR_VERSION 0
#define FRIBIDI_MINOR_VERSION 10
#define FRIBIDI_MICRO_VERSION 7
#define FRIBIDI_INTERFACE_VERSION 2

#if 1 /* FRIBIDI_NO_CHARSETS */
#define FRIBIDI_NO_CHARSETS 1
#else /* NOT FRIBIDI_NO_CHARSETS */
#undef FRIBIDI_NO_CHARSETS
#endif /* FRIBIDI_NO_CHARSETS */

#define TOSTR(x) #x

#ifdef WIN32

#define MEM_OPTIMIZED

#define HAS_FRIBIDI_TAB_CHAR_TYPE_2_I 1
#define HAS_FRIBIDI_TAB_CHAR_TYPE_9_I 1

#define FRIBIDI_API

#define snprintf _snprintf

#else /* NOT WIN32 */

#define FRIBIDI_API

#endif /* WIN32 */

#define FRIBIDI_TRUE    1
#define FRIBIDI_FALSE   0

#ifndef TRUE
#define TRUE FRIBIDI_TRUE
#endif /* TRUE */
#ifndef FALSE
#define FALSE FRIBIDI_FALSE
#endif /* FALSE */

#define FRIBIDI_SIZEOF_SHORT 2
#define FRIBIDI_SIZEOF_INT 4
#define FRIBIDI_SIZEOF_LONG 4

