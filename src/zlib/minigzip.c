/* minigzip.c -- simulate gzip using the zlib compression library
 * Copyright (C) 1995-1998 Jean-loup Gailly.
 * Copyright (C) 2000      Tenik Co.,Ltd.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/*
 * minigzip is a minimal implementation of the gzip utility. This is
 * only an example of using zlib and isn't meant to replace the
 * full-featured gzip. No attempt is made to deal with file systems
 * limiting names to 14 or 8+3 characters, etc... Error checking is
 * very limited. So use minigzip only for testing; use gzip for the
 * real thing. On MSDOS, use only on file names without extension
 * or in pipe mode.
 */

/* @(#) $Id: minigzip.c,v 1.1 2006/08/18 18:11:56 eshabtai Exp $ */

#if defined(_WIN32_WCE)
#if _WIN32_WCE < 211
#error (f|w)printf functions is not support old WindowsCE.
#endif
#undef USE_MMAP
#include <windows.h>
#else
#include <stdio.h>
#endif
#include "zlib.h"

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#else
   extern void exit  OF((int));
#endif

#ifdef USE_MMAP
#  include <sys/types.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#endif

#if (defined(MSDOS) || defined(OS2) || defined(WIN32)) && !defined(_WIN32_WCE)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#ifdef VMS
#  define unlink delete
#  define GZ_SUFFIX "-gz"
#endif
#ifdef RISCOS
#  define unlink remove
#  define GZ_SUFFIX "-gz"
#  define fileno(file) file->__file
#endif
#if defined(__MWERKS__) && __dest_os != __be_os && __dest_os != __win32_os
#  include <unix.h> /* for fileno */
#endif

#ifndef WIN32 /* unlink already in stdio.h for WIN32 */
  extern int unlink OF((const char *));
#endif

#ifndef GZ_SUFFIX
#  define GZ_SUFFIX ".gz"
#endif
#define SUFFIX_LEN (sizeof(GZ_SUFFIX)-1)

#define BUFLEN      16384
#define MAX_NAME_LEN 1024

#ifdef MAXSEG_64K
#  define local static
   /* Needed for systems with limitation on stack size. */
#else
#  define local
#endif

#if defined(_WIN32_WCE)
#undef  stderr
#define stderr  stdout
#define F_FILE  HANDLE
#define F_NULL  INVALID_HANDLE_VALUE
#else
#define F_FILE  FILE*
#define F_NULL  NULL
#endif

char *prog;

void error            OF((const char *msg));
void gz_compress      OF((F_FILE in, gzFile out));
#ifdef USE_MMAP
int  gz_compress_mmap OF((F_FILE in, gzFile out));
#endif
void gz_uncompress    OF((gzFile in, F_FILE out));
void file_compress    OF((char  *file, char *mode));
void file_uncompress  OF((char  *file));
int  main             OF((int argc, char *argv[]));

/* ===========================================================================
 * Display error message and exit
 */
void error(msg)
    const char *msg;
{
    fprintf(stderr, "%s: %s\n", prog, msg);
    exit(1);
}

#if defined(_WIN32_WCE)
void perror(msg)
    const char *msg;
{
    DWORD dwError;
    LPVOID lpMsgBuf;

    dwError = GetLastError();
    if ( FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL) )
    {
        wprintf(TEXT("%S: %s\n"), msg, (LPTSTR)lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
    else
    {
        wprintf(TEXT("%S: Error #%d\n"), msg, dwError);
    }
}

int unlink(filename)
    const char *filename;
{
    TCHAR path[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, filename, -1, path, MAX_PATH);
    return DeleteFile(path);
}
#endif

/* ===========================================================================
 * Compress input to output then close both files.
 */

void gz_compress(in, out)
    F_FILE in;
    gzFile out;
{
    local char buf[BUFLEN];
    int len;
    int err;

#ifdef USE_MMAP
    /* Try first compressing with mmap. If mmap fails (minigzip used in a
     * pipe), use the normal fread loop.
     */
    if (gz_compress_mmap(in, out) == Z_OK) return;
#endif
    for (;;) {
#if defined(_WIN32_WCE)
        if (!ReadFile(in, buf, sizeof(buf), &len, NULL)) {
#else
        len = fread(buf, 1, sizeof(buf), in);
        if (ferror(in)) {
#endif
            perror("fread");
            exit(1);
        }
        if (len == 0) break;

        if (gzwrite(out, buf, (unsigned)len) != len) error(gzerror(out, &err));
    }
#if defined(_WIN32_WCE)
    CloseHandle(in);
#else
    fclose(in);
#endif
    if (gzclose(out) != Z_OK) error("failed gzclose");
}

#ifdef USE_MMAP /* MMAP version, Miguel Albrecht <malbrech@eso.org> */

/* Try compressing the input file at once using mmap. Return Z_OK if
 * if success, Z_ERRNO otherwise.
 */
int gz_compress_mmap(in, out)
    F_FILE in;
    gzFile out;
{
    int len;
    int err;
    int ifd = fileno(in);
    caddr_t buf;    /* mmap'ed buffer for the entire input file */
    off_t buf_len;  /* length of the input file */
    struct stat sb;

    /* Determine the size of the file, needed for mmap: */
    if (fstat(ifd, &sb) < 0) return Z_ERRNO;
    buf_len = sb.st_size;
    if (buf_len <= 0) return Z_ERRNO;

    /* Now do the actual mmap: */
    buf = mmap((caddr_t) 0, buf_len, PROT_READ, MAP_SHARED, ifd, (off_t)0); 
    if (buf == (caddr_t)(-1)) return Z_ERRNO;

    /* Compress the whole file at once: */
    len = gzwrite(out, (char *)buf, (unsigned)buf_len);

    if (len != (int)buf_len) error(gzerror(out, &err));

    munmap(buf, buf_len);
    fclose(in);
    if (gzclose(out) != Z_OK) error("failed gzclose");
    return Z_OK;
}
#endif /* USE_MMAP */

/* ===========================================================================
 * Uncompress input to output then close both files.
 */
void gz_uncompress(in, out)
    gzFile in;
    F_FILE out;
{
    local char buf[BUFLEN];
    int len;
    int err;
#if defined(_WIN32_WCE)
    int size;
#endif

    for (;;) {
        len = gzread(in, buf, sizeof(buf));
        if (len < 0) error (gzerror(in, &err));
        if (len == 0) break;

#if defined(_WIN32_WCE)
        if (!WriteFile(out, buf, (unsigned)len, &size, NULL) || size != len) {
#else
        if ((int)fwrite(buf, 1, (unsigned)len, out) != len) {
#endif
	    error("failed fwrite");
	}
    }
#if defined(_WIN32_WCE)
    if (!CloseHandle(out)) error("failed fclose");
#else
    if (fclose(out)) error("failed fclose");
#endif

    if (gzclose(in) != Z_OK) error("failed gzclose");
}


/* ===========================================================================
 * Compress the given file: create a corresponding .gz file and remove the
 * original.
 */
void file_compress(file, mode)
    char  *file;
    char  *mode;
{
    local char outfile[MAX_NAME_LEN];
    F_FILE in;
    gzFile out;
#if defined(_WIN32_WCE)
    TCHAR path[MAX_PATH];
#endif

    strcpy(outfile, file);
    strcat(outfile, GZ_SUFFIX);

#if defined(_WIN32_WCE)
    MultiByteToWideChar(CP_ACP, 0, file, -1, path, MAX_PATH);
    in = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
#else
    in = fopen(file, "rb");
#endif
    if (in == F_NULL) {
        perror(file);
        exit(1);
    }
    out = gzopen(outfile, mode);
    if (out == F_NULL) {
        fprintf(stderr, "%s: can't gzopen %s\n", prog, outfile);
        exit(1);
    }
    gz_compress(in, out);

    unlink(file);
}


/* ===========================================================================
 * Uncompress the given file and remove the original.
 */
void file_uncompress(file)
    char  *file;
{
    local char buf[MAX_NAME_LEN];
    char *infile, *outfile;
    F_FILE out;
    gzFile in;
    int len = strlen(file);
#if defined(_WIN32_WCE)
    TCHAR path[MAX_PATH];
#endif

    strcpy(buf, file);

    if (len > SUFFIX_LEN && strcmp(file+len-SUFFIX_LEN, GZ_SUFFIX) == 0) {
        infile = file;
        outfile = buf;
        outfile[len-3] = '\0';
    } else {
        outfile = file;
        infile = buf;
        strcat(infile, GZ_SUFFIX);
    }
    in = gzopen(infile, "rb");
    if (in == F_NULL) {
        fprintf(stderr, "%s: can't gzopen %s\n", prog, infile);
        exit(1);
    }
#if defined(_WIN32_WCE)
    MultiByteToWideChar(CP_ACP, 0, outfile, -1, path, MAX_PATH);
    out = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
#else
    out = fopen(outfile, "wb");
#endif
    if (out == F_NULL) {
        perror(file);
        exit(1);
    }

    gz_uncompress(in, out);

    unlink(infile);
}


/* ===========================================================================
 * Usage:  minigzip [-d] [-f] [-h] [-1 to -9] [files...]
 *   -d : decompress
 *   -f : compress with Z_FILTERED
 *   -h : compress with Z_HUFFMAN_ONLY
 *   -1 to -9 : compression level
 */

int main(argc, argv)
    int argc;
    char *argv[];
{
    int uncompr = 0;
#if !defined(_WIN32_WCE)
    gzFile file;
#endif
    char outmode[20];

    strcpy(outmode, "wb6 ");

    prog = argv[0];
    argc--, argv++;

    while (argc > 0) {
      if (strcmp(*argv, "-d") == 0)
	uncompr = 1;
      else if (strcmp(*argv, "-f") == 0)
	outmode[3] = 'f';
      else if (strcmp(*argv, "-h") == 0)
	outmode[3] = 'h';
      else if ((*argv)[0] == '-' && (*argv)[1] >= '1' && (*argv)[1] <= '9' &&
	       (*argv)[2] == 0)
	outmode[2] = (*argv)[1];
      else
	break;
      argc--, argv++;
    }
    if (argc == 0) {
#if defined(_WIN32_WCE)
        wprintf(TEXT("Usage:  minigzip [-d] [-f] [-h] [-1 to -9] [files...]\n"));
        wprintf(TEXT("  -d : decompress\n"));
        wprintf(TEXT("  -f : compress with Z_FILTERED\n"));
        wprintf(TEXT("  -h : compress with Z_HUFFMAN_ONLY\n"));
        wprintf(TEXT("  -1 to -9 : compression level\n"));
#else
        SET_BINARY_MODE(stdin);
        SET_BINARY_MODE(stdout);
        if (uncompr) {
            file = gzdopen(fileno(stdin), "rb");
            if (file == NULL) error("can't gzdopen stdin");
            gz_uncompress(file, stdout);
        } else {
            file = gzdopen(fileno(stdout), outmode);
            if (file == NULL) error("can't gzdopen stdout");
            gz_compress(stdin, file);
        }
#endif
    } else {
        do {
            if (uncompr) {
                file_uncompress(*argv);
            } else {
                file_compress(*argv, outmode);
            }
        } while (argv++, --argc);
    }
    exit(0);
    return 0; /* to avoid warning */
}
