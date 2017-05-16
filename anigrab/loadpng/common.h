/*
**  bmp2png --- conversion from (Windows or OS/2 style) BMP to PNG
**  png2bmp --- conversion from PNG to (Windows style) BMP
**
**  Copyright (C) 1999-2000 MIYASAKA Masaru <alkaid@coral.ocn.ne.jp>
**
**  Permission to use, copy, modify, and distribute this software and
**  its documentation for any purpose and without fee is hereby granted,
**  provided that the above copyright notice appear in all copies and
**  that both that copyright notice and this permission notice appear
**  in supporting documentation. This software is provided "as is"
**  without express or implied warranty.
**
**  NOTE: Comments are partly written in Japanese. Sorry.
*/

#ifndef COMMON_H
#define COMMON_H

#if defined(__RSXNT__) && defined(__CRTRSXNT__)
# include <crtrsxnt.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
//#include <setjmp.h>

	/* for stat() */
#include <sys/types.h>
#include <sys/stat.h>

	/* for utime() */
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__LCC__)
# include <sys/utime.h>
# if defined(__LCC__)
   int utime(const char *, struct _utimbuf *);
# endif
#else
# include <utime.h>
#endif
	/* for isatty() */
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__LCC__)
# include <io.h>
#else
# include <unistd.h>
#endif
	/* for mkdir() */
#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW32__) || \
    defined(__LCC__)
# include <direct.h>
# define MKDIR(d,m)	mkdir(d)
#else
# if defined(__GO32__) && !defined(__DJGPP__)	/* DJGPP v.1 */
#  include <osfcn.h>
# else
#  include <sys/stat.h>
# endif
# define MKDIR(d,m)	mkdir(d,m)
#endif

#if !defined(USE_FDOPEN) && !defined(USE_SETMODE)
# if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__RSXNT__) || \
     defined(_MSC_VER) || defined(__BORLANDC__) || defined(__LCC__) || \
     defined(__DJGPP__) || defined(__GO32__)
#  define USE_SETMODE
# endif
# if 0 /* defined(__YOUR_COMPLIER_MACRO__) */
#  define USE_FDOPEN
# endif
#endif
	/* for setmode() */
#ifdef USE_SETMODE
# include <io.h>
# include <fcntl.h>
#endif

#include "png.h"

#if (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32)
# define WIN32
#endif
#if defined(__MSDOS__) && !defined(MSDOS)
# define MSDOS
#endif

#if defined(WIN32) || defined(__DJGPP__)
#define WIN32_LFN		/* Win32-style long filename */
#endif

#if defined(WIN32) || defined(MSDOS)
# define PDELIM_PRIMARY		'\\'
# define PDELIM_SECONDARY	'/'
# define PDELIM_DRIVESPEC	':'
# define IsPathDelim(c)		((c)==PDELIM_PRIMARY || (c)==PDELIM_SECONDARY)
# define isoptchar(c)		((c)=='-' || (c)=='/')
# ifdef JAPANESE
#  define DBCS_PATH
#  define IsDBCSLead(c)		(('\x81'<=(c)&&(c)<='\x9f')||('\xe0'<=(c)&&(c)<='\xfc'))
# else
#  undef  DBCS_PATH
#  define IsDBCSLead(c)		(0)
# endif
#else	/* UNIX, etc */
# define PDELIM_PRIMARY		'/'
# define IsPathDelim(c)		((c)==PDELIM_PRIMARY)
# define isoptchar(c)		((c)=='-')
# undef  DBCS_PATH
# define IsDBCSLead(c)		(0)
#endif

typedef char           CHAR;
typedef unsigned char  BYTE;
typedef short          SHORT;
typedef unsigned short WORD;
typedef int            INT;
typedef unsigned int   UINT;
typedef long           LONG;
typedef unsigned long  DWORD;
typedef enum { FALSE=0,TRUE=1 } BOOL;

typedef png_color PALETTE;
typedef struct tagIMAGE {
	LONG    width;
	LONG    height;
	UINT    pixdepth;
	UINT    palnum;
	BOOL    topdown;
	/* ----------- */
	DWORD   rowbytes;
	DWORD   imgbytes;
	PALETTE *palette;
	BYTE    **rowptr;
	BYTE    *bmpbits;
} IMAGE;

extern int quietmode;

void xprintf( const char *, ... );
void set_status( const char *, ... );
void feed_line( void );
void init_row_callback( png_structp, png_uint_32, png_uint_32 );
void row_callback( png_structp, png_uint_32, int );
void png_my_error( png_structp, png_const_charp );
void png_my_warning( png_structp, png_const_charp );
BOOL imgalloc( IMAGE * );
void imgfree( IMAGE * );
int parsearg( int *, char **, int, char **, char * );
char **envargv( int *, char ***, const char * );
int tokenize( char *, const char * );
int makedir( const char * );
int renbak( const char * );
int filetype( const char * );
int cpyftime( const char *, const char * );
char *suffix( const char * );
char *bname( const char * );
char *addslash( char * );
char *delslash( char * );
char *path_skiproot( const char * );
char *path_nextslash( const char * );
#ifdef WIN32_LFN
int is_dos_extension( const char * );
#endif

#endif /* COMMON_H */
