/*\
 *
 *  % MIT License
 *  %
 *  % Copyright (c) 1990 W. Kirk Snyder
 *  %
 *  % Permission is hereby granted, free of charge, to any person obtaining a copy
 *  % of this software and associated documentation files (the "Software"), to deal
 *  % in the Software without restriction, including without limitation the rights
 *  % to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  % copies of the Software, and to permit persons to whom the Software is
 *  % furnished to do so, subject to the following conditions:
 *  % 
 *  % The above copyright notice and this permission notice shall be included in all
 *  % copies or substantial portions of the Software.
 *  %
 *  % THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  % IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  % FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  % AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  % LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  % OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  % SOFTWARE.
 *  %
 *
 *  \package{System Definitions}
 *
 *  This header is included in every source file in the \setl\ system.  It
 *  contains a few declarations common to both the compiler and
 *  interpreter, but it's primary reason for being is to try to isolate
 *  system dependencies, to the extent that is possible.
 *
 *  There seem to be five sources of system dependencies:
 *
 *  \begin{enumerate}
 *  \item
 *  Operating system.  There are differences in file name handling.
 *
 *  \item
 *  Compiler.  There are relatively few differences here, since I tried
 *  to use ANSI C throughout.  For micros, differences in libraries are
 *  generally linked to differences in compilers.
 *
 *  \item
 *  Floating point format.  We need the bit field layout for hashing
 *  floating point numbers and to convert them to integers.
 *
 *  \item
 *  Processor.  So far there are no processor dependencies, but we might
 *  find some in future ports.
 *
 *  \item
 *  Site dependencies.  There are differences in the availability,
 *  content, and location of header files and libraries.  These are
 *  particularly annoying, since I can't predict what future sites will
 *  have.
 *
 *  \end{enumerate}
 *
 *  There are a number of preprocessor symbols used to control these
 *  annoyances.  See the appropriate makefiles for information on them.
\*/

#ifndef SYSTEM_LOADED

#ifdef __alpha
#define int32 int
#else
#define int32 long
#endif

#ifndef WIN32
#ifdef _WIN32
#define WIN32 1
#endif
#endif


#ifndef UNIX
#ifdef unix
#define UNIX 1
#endif
#ifdef __APPLE__ 
#ifdef __MACH__
#define UNIX 1
#endif
#endif
#endif

#ifdef WIN32
#define WATCOM 1
#include "config.h.win" 
#endif

#ifdef HAVE_CONFIG_H
#include "config.h" 
#endif

#ifdef HAVE_MPATROL
#include "mpatrol.h"
#endif

#ifdef MACINTOSH
#include "setlvers.h"
#endif

#define NATIVE_INIT  "__INIT"

#ifndef YES
#define YES    1                       /* true constant                     */
#endif

#ifndef NO
#define NO     0                       /* false constant                    */
#endif

/*\
 *  \function{MS-DOS}
 *
 *  I'm consolodating my MS-DOS implementations.  From now on I will only
 *  support 386 systems, and will use Watcom as my C compiler vendor.
\*/

#ifdef WATCOM
#define DOS 1

/*\
 *  Compiler options.  Putting them here preserves precious command line
 *  space.
\*/

#define FARDATA
#define IEEE_LEND 1

/* Intel C standard header files */

#include <stdlib.h>                    /* Lattice standard header           */
#include <stdio.h>                     /* standard I/O header               */
#include <string.h>                    /* string header                     */
#include <time.h>                      /* time structures                   */


/* file name constants */

#define PATH_LENGTH  128               /* maximum length of a path name     */
#define PATH_SEP     '\\'              /* node separator in paths           */
#define EOFCHAR      0x1a              /* end of file character             */

/* file modes */

#define BINARY_RD          "rb"        /* input binary file                 */
#define BINARY_WR          "wb"        /* output binary file                */
#define BINARY_RDWR        "rb+"       /* I/O binary file                   */
#define BINARY_CREATE_RDWR "wb+"       /* new binary file (I/O)             */

/* operating system specific access() */

#define os_access(f,m) access(f,m)

/* exit codes */

#define SUCCESS_EXIT          0        /* program completed successfully    */
#define GIVEUP_EXIT          10        /* severe error                      */
#define TRAP_EXIT            20        /* program trap - internal error     */
#define COMPILE_ERROR_EXIT    1        /* compilation had errors            */
#define ABEND_EXIT            2        /* program aborted                   */
#define ASSERT_EXIT           1        /* assertion failed                  */

/* min and max macros */

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

#endif

/*\
 *  \function{Unix}
 *
 *  The current Unix implementations all use the Gnu C compiler.  This is
 *  my preferred compiler for two reasons:  it recognizes the ANSI
 *  standard and it is available on many machines.
\*/

#if UNIX

/* some OS dependent symbols */

#define FARDATA

/* Gnu C standard header files */

#include <sys/types.h>                 /* standard type definitions         */
#include <stdio.h>                     /* standard I/O library              */
#include <string.h>                    /* string library                    */
#include <time.h>                      /* time structures                   */

/* min and max macros */

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

/* some convenient Microsoft symbols */

#ifndef SEEK_CUR
#define  SEEK_CUR    1                 /* fseek relative to current pos     */
#endif
#ifndef SEEK_END
#define  SEEK_END    2                 /* fseek relative to eof             */
#endif
#ifndef SEEK_SET
#define  SEEK_SET    0                 /* fseek relative to beginning       */
#endif
#ifndef SIG_ERR
#define  SIG_ERR     (int (*)())-1     /* error return from signal          */
#endif

#ifndef labs
#define labs(x) (((x) < 0) ? -(x) : (x))
                                       /* absolute value for longs?         */
#endif

/* definitions Microsoft puts in stdlib.h */
/* These definitions are also used on Windows */

#ifndef AIX
char *getenv(const char *);            /* get environment string            */
#ifndef HAVE_MPATROL
void *malloc(size_t);                  /* memory allocation                 */
#endif
char *getcwd(char *, size_t);             /* get current working directory     */
#endif

/* one of our sites uses getwd rather than getcwd! */

#ifdef GETWD
#define getcwd(a,b) getwd(a)
#endif

/* file name constants */

#define PATH_LENGTH  1024              /* maximum length of a path name     */
#define PATH_SEP     '/'               /* node separator in paths           */
#define EOFCHAR      0x04              /* end of file character             */

/* file modes */

#define BINARY_RD          "r"         /* input binary file                 */
#define BINARY_WR          "w"         /* output binary file                */
#define BINARY_RDWR        "r+"        /* I/O binary file                   */
#define BINARY_CREATE_RDWR "w+"        /* new binary file (I/O)             */

/* operating system specific access() */

#define os_access(f,m) access(f,m)

/* exit codes */

#define SUCCESS_EXIT          0        /* program completed successfully    */
#define GIVEUP_EXIT          10        /* severe error                      */
#define TRAP_EXIT            20        /* program trap - internal error     */
#define COMPILE_ERROR_EXIT    1        /* compilation had errors            */
#define ABEND_EXIT            2        /* program aborted                   */
#define ASSERT_EXIT           1        /* assertion failed                  */

#endif

/*\
 *  \function{VMS}
 *
 *  I used the standard DEC C compiler for the VMS implementation.  There
 *  is a version of Gnu C for VMS, but for some reason it is not
 *  installed at NYU and I was too lazy to install it.
\*/

#if VMS

/* some OS dependent symbols */

#define FARDATA

/* VMS C standard header files */

#include <types.h>                     /* standard type definitions         */
#include <stdio.h>                     /* standard I/O library              */
#include <string.h>                    /* string library                    */
#include <time.h>                      /* time structures                   */

/* min and max macros */

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

/* some convenient Microsoft symbols */

#define  SEEK_CUR    1                 /* fseek relative to current pos     */
#define  SEEK_END    2                 /* fseek relative to eof             */
#define  SEEK_SET    0                 /* fseek relative to beginning       */
#define  SIG_ERR     (int (*)())-1     /* error return from signal          */

#define labs(x) (((x) < 0) ? -(x) : (x))
                                       /* absolute value for longs?         */

/* definitions Microsoft puts in stdlib.h */

char *getenv(char *);                  /* get environment string            */

/* VMS doesn't have unlink */

#define unlink(x) delete(x)

/* file name constants */

#define PATH_LENGTH  256               /* maximum length of a path name     */
#define PATH_SEP     ']'               /* node separator in paths           */
#define EOFCHAR      0x1a              /* end of file character             */

/* file modes */

#define BINARY_RD          "r"         /* input binary file                 */
#define BINARY_WR          "w"         /* output binary file                */
#define BINARY_RDWR        "r+"        /* I/O binary file                   */
#define BINARY_CREATE_RDWR "w+"        /* new binary file (I/O)             */

/*
 *  For some strange reason, the access() function in VMS C only works if
 *  the rights given is 0 (test for existence).  This isn't as strong a
 *  test as I'd like, but the result is just a somewhat less intelligible
 *  error message, so I'll live with it for now.
 */

#define os_access(f,m) access(f,0)

/*
 *  VMS reserves the exit codes for it's own errors, so we always exit
 *  with a code of zero.
 */

#define SUCCESS_EXIT          0        /* program completed successfully    */
#define GIVEUP_EXIT           0        /* severe error                      */
#define TRAP_EXIT             0        /* program trap - internal error     */
#define COMPILE_ERROR_EXIT    0        /* compilation had errors            */
#define ABEND_EXIT            0        /* program aborted                   */
#define ASSERT_EXIT           0        /* assertion failed                  */

#endif

/*\
 *  \function{Apple Macintosh}
 *
 *  I use MPW (Macintosh Programmer's Workbench) C for the Apple
 *  Macintosh.  I tried Think C, but the version I have will not allow
 *  more than 32K of global data, and I need more than that.
\*/

#ifdef MACINTOSH

/* some OS dependent symbols */

#define FARDATA

/* MPW C standard header files */

#include <stdlib.h>                    /* standard C library                */
#include <stdio.h>                     /* standard I/O library              */
#include <string.h>                    /* string library                    */
#include <time.h>                      /* time structures                   */
#if MPWC
#include <CursorCtl.h>                 /* MPW cursor control                */
#endif

unsigned char *Pstring(char *str);               /* conversion string utility        */

/* min and max macros */

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

#ifndef __MWERKS__ /* GDM */
#define labs(x) (((x) < 0) ? -(x) : (x))
                                       /* absolute value for longs?         */
#endif

/* file name constants */

#define PATH_LENGTH  256               /* maximum length of a path name     */
#define PATH_SEP     ':'               /* node separator in paths           */
#define EOFCHAR      0xff              /* end of file character             */

/* file modes */

#define BINARY_RD          "rb"        /* input binary file                 */
#define BINARY_WR          "wb"        /* output binary file                */
#define BINARY_RDWR        "r+b"       /* I/O binary file                   */
#define BINARY_CREATE_RDWR "w+b"       /* new binary file (I/O)             */

/* operating system specific access() */

#define os_access(f,m) access(f,m)

/* definitions Microsoft puts in stdlib.h */

#ifdef HAS_SOCKETS
#ifdef MACINTOSH
/* definition for this case is in GUSI */
#else
int access(char *,int);                /* check file availability           */
#endif
#else
int access(char *,int);                /* check file availability           */
#endif

#ifndef __MWERKS__ /* GDM */
int getpid(void);                      /* process identification            */
#else
#include <ctime>
#endif
int unlink(const char *);                        /* delete file                       */

/* exit codes */

#define SUCCESS_EXIT          0        /* program completed successfully    */
#define GIVEUP_EXIT          10        /* severe error                      */
#define TRAP_EXIT            20        /* program trap - internal error     */
#define COMPILE_ERROR_EXIT    1        /* compilation had errors            */
#define ABEND_EXIT            2        /* program aborted                   */
#define ASSERT_EXIT           1        /* assertion failed                  */

#endif

/*\
 *  \function{Floating Point Declarations}
 *
 *  Floating point format is a big headache, since the supposed
 *  `standards' (IEEE) are not universally adopted.  Even when they are,
 *  we have to worry about big-endian vs.\ little-endian for bit-field
 *  layouts.  To make things worse, compilers which support IEEE format
 *  sometimes simply return NAN (Not a Number) or infinity as the result
 *  of an invalid operation, and do not raise the math error signal. The
 *  following macro will test for an invalid value.  So far, we only need
 *  the macro for IEEE, but it should be easy to modify for others if
 *  that proves necessary.
 *
 *  Note:  The following depend on long's being 32 bits.  That's the case
 *  on most machines these days, but it's something to beware of.
\*/
#if WORDS_BIGENDIAN
#define IEEE_BEND 1
#else
#define IEEE_LEND 1
#endif

#if HAVE_ISNAN
#define INFNAN 1
#endif

#if INFNAN
#if IEEE_BEND
#define NANorINF(X) ((*((int32 *)&X) & 0x7ff00000) == 0x7ff00000)
#endif

#if IEEE_LEND
#define NANorINF(X) ((*(((int32 *)&X) + 1) & 0x7ff00000) == 0x7ff00000)
#endif
#endif

/*\
 *  \function{Global Declarations}
 *
 *  The following declarations are used in all major components of the
 *  \setl\ system.
\*/


#define MAX_TOK_LEN           256      /* maximum token length              */
#define MAX_UNIT_NAME          64      /* maximum length of a unit name     */

/* file position structure */

struct file_pos_item {
   int fp_line;                        /* line number                       */
   int fp_column;                      /* column number                     */
};

typedef struct file_pos_item file_pos_type;
                                       /* file position type                */

/* Structure used to find and store all the global variables                */

struct global_item {
   int gl_number;                      /* position in eval_vars package     */
   int gl_offset;                      /* position in symbol table          */
   char *gl_name;                      /* name lexeme                       */
   int gl_global;                      /* YES if symb. is a global var      */
   int gl_present;                     /* YES if symb. already defined      */
   struct global_item *gl_next_ptr;    /* list of symbols                   */
   unsigned gl_type : 4;               /* symbol class (id or procedure     */
};

typedef struct global_item *global_ptr_type;

/* macro to copy file positions */

#define copy_file_pos(t,s) { \
   (t)->fp_line = (s)->fp_line; \
   (t)->fp_column = (s)->fp_column; \
}

/* environment keys */

#define LIB_KEY             "SETL2_LIB"
                                       /* default library                   */
#define LIBPATH_KEY         "SETL2_LIBPATH"
                                       /* library path                      */
#define COMP_OPTIONS_KEY    "STLC_OPTIONS"
                                       /* compiler options                  */
#define TEMP_PATH_KEY       "SETL2_TMP"
                                       /* temporary file path               */
#define INTERP_OPTIONS_KEY  "STLX_OPTIONS"
                                       /* interpreter options               */

/* set switches for library manager */

#ifdef COMPILER
#define LIBWRITE 1
#endif
#ifdef LIBUTIL
#define LIBWRITE 1
#endif

#ifdef DYNAMIC_COMP
#define LIBWRITE 1
#endif

#define JAVASCRIPT_PREFIX "javascript:"

/* Shared library stuff... */


#define SO_EXTENSION ""

#if HAVE_DLFCN_H
#undef SO_EXTENSION
#define SO_EXTENSION ".so"
#endif

#ifdef WIN32
#undef SO_EXTENSION
#ifdef _DEBUG
#define SO_EXTENSION "d.dll"
#else
#define SO_EXTENSION ".dll"
#endif
#define SETL_API __declspec(dllexport)
#else
#define SETL_API
#endif

#define EXTERNAL extern

#ifdef TSAFE
#define SETL_SYSTEM_PROTO plugin_item_ptr_type plugin_instance,
#define SETL_SYSTEM_PROTO_VOID plugin_item_ptr_type plugin_instance
#define SETL_SYSTEM plugin_instance,
#define SETL_SYSTEM_VOID plugin_instance
#else
#define SETL_SYSTEM_PROTO 
#define SETL_SYSTEM_PROTO_VOID void
#define SETL_SYSTEM 
#define SETL_SYSTEM_VOID 
#endif

int plugin_printf(const char *format, ...);
int plugin_fprintf(FILE *file, const char *format, ...);
int plugin_fputs(const char *string, FILE *file);

#define SYSTEM_LOADED 1


#endif
