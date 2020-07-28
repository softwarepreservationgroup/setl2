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
 *  \package{Giveup}
 *
 *  This package contains functions which handle severe problems, where we
 *  have decided we simply can not continue.  They clean up anything
 *  necessary (work files \& such) and exit.  There is never a return from
 *  this package.
 *
 *  \texify{giveup.h}
 *
 *  \packagebody{Giveup}
\*/




/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */

#ifdef INTERP
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "builtins.h"                  /* built-in symbols                  */
#endif

#ifdef COMPILER
#ifndef VMS
#include <fcntl.h>                     /* file functions                    */
#endif
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "libman.h"                    /* library manager                   */
#endif

#include "chartab.h"                   /* character type table              */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
/* standard C header files */


#if UNIX_VARARGS
#include <varargs.h>                   /* variable argument macros          */
#else
#include <stdarg.h>                    /* variable argument macros          */
#endif

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define exit plugin_exit
#endif

/*\
 *  \function{giveup()}
 *
 *  This function is called in the event of a calamity.  Some examples
 *  are:
 *
 *  \begin{itemize}
 *  \item
 *  After a call to \verb"malloc()", where \verb"malloc()" fails.
 *  \item
 *  After a low level DOS service call, where the call fails.
 *  \item
 *  When we find something seriously wrong in the command line, and we
 *  don't know how to proceed.
 *  \end{itemize}
 *
 *  We never return from this function, but exit with a high
 *  error code.
 *
 *  It is written to accept a variable number of arguments and pass those
 *  on to \verb"vfprintf()".  The arguments are exactly the same as
 *  \verb"printf()".
\*/

#ifndef PLUGIN

#if UNIX_VARARGS
void giveup(message,va_alist)
   char *message;                      /* first argument of message         */
   va_dcl                              /* any other arguments               */
#else
void giveup(
   SETL_SYSTEM_PROTO
   char *message,                      /* first argument of message         */
   ...)                                /* any other arguments               */
#endif

{
va_list arg_pointer;                   /* argument list pointer             */
static int already_called = NO;        /* forbid recursive calls            */
#if UNIX_VARARGS
static char vp_buffer[8000];           /* vsprintf buffer area              */
char vp_pattern[20];                   /* saved vsprintf string             */
char vp_size;                          /* saved vp_size field               */
char vp_c_arg;                         /* hold character arg                */
char *vp_s_arg;                        /* hold string                       */
int vp_i_arg;                          /* hold integer arg                  */
int *vp_n_arg;                         /* hold integer pointer              */
void *vp_p_arg;                        /* hold void pointer                 */
long vp_l_arg;                         /* hold long arg                     */
double vp_f_arg;                       /* hold float arg                    */
char *vp_p, *vp_s, *vp_t;              /* used to traverse buffer           */
#endif

   if (already_called)
      return;
   already_called = YES;

   /*
    *  The following ugliness accommodates MIPS brain damage.  First, the
    *  stdarg.h macros in gcc don't work.  To make matters worse, the
    *  varargs.h macros don't work with the v?printf functions.  So, I
    *  have to do this futzing myself.
    */

#if UNIX_VARARGS

   va_start(arg_pointer);
   vp_s = message;
   vp_t = vp_buffer;
   while (*vp_s) {

      /* find the next format string */

      while (*vp_s && *vp_s != '%')  
         *vp_t++ = *vp_s++;

      if (!*vp_s) {
         *vp_t = '\0';
         break;
      }
      if (*(vp_s+1) == '%') {
         *vp_t++ = '%';
         vp_s += 2;
         continue;
      }
      vp_p = vp_pattern;
      *vp_p++ = *vp_s++;
      
      /* skip flags */

      while (*vp_s == '+' ||
             *vp_s == '-' ||
             *vp_s == '#' ||
             *vp_s == ' ')
         *vp_p++ = *vp_s++;

      /* skip width, precision */

      if (is_digit(*vp_s,10)) {
         while (is_digit(*vp_s,10))
            *vp_p++ = *vp_s++;
         if (*vp_s == '.') {
            *vp_p++ = *vp_s++;
            while (is_digit(*vp_s,10))
               *vp_p++ = *vp_s++;
         }
      }

      /* skip vp_size */

      if (*vp_s == 'h' ||
          *vp_s == 'l' ||
          *vp_s == 'L') {

         vp_size = *vp_s;
         *vp_p++ = *vp_s++;

      }
      else {
       
         vp_size = 0;

      }

      /* finally, we should have the type field */
     
      *vp_p = *vp_s++;
      *(vp_p+1) = '\0';
      switch (*vp_p) {

         case 'c' :

            vp_c_arg = va_arg(arg_pointer,char);
            sprintf(vp_t,vp_pattern,vp_c_arg);
            break;

         case 'd' :
         case 'i' :
         case 'o' :
         case 'u' :
         case 'x' :
         case 'X' :

            if (vp_size == 'l') { 
               vp_l_arg = va_arg(arg_pointer,long);
               sprintf(vp_t,vp_pattern,vp_l_arg);
            }
            else {
               vp_i_arg = va_arg(arg_pointer,int);
               sprintf(vp_t,vp_pattern,vp_i_arg);
            }
            break;

         case 'e' :
         case 'E' :
         case 'f' :
         case 'g' :
         case 'G' :
          
            vp_f_arg = va_arg(arg_pointer,double);
            sprintf(vp_t,vp_pattern,vp_f_arg);
            break;

         case 's' :
          
            vp_s_arg = va_arg(arg_pointer,char *);
            sprintf(vp_t,vp_pattern,vp_s_arg);
            break;

         case 'n' :

            vp_n_arg = va_arg(arg_pointer,int *);
            sprintf(vp_t,vp_pattern,vp_n_arg);
            break;

         case 'p' :

            vp_p_arg = va_arg(arg_pointer,void *);
            sprintf(vp_t,vp_pattern,vp_p_arg);
            break;

         default :
            strcpy(vp_t,vp_pattern);

      }

      /* skip past the appended string */

      while (*vp_t)
         vp_t++;

   }

   fflush(stdout);
   fputs(vp_buffer,stderr)
   va_end(arg_pointer);

#else

#ifdef INTERP
#ifdef WINX
   vsprintf(giveup_message, message, arg_pointer);
   MessageBox(NULL, giveup_message, "SETL2 Abort", MB_OK);
#else
#ifdef PANEL
   vsprintf(giveup_message, message, arg_pointer);
   MessageBox(NULL, giveup_message, "SETL2 Abort", MB_OK);
#else
   fflush(stdout);
   va_start(arg_pointer,message);
   vfprintf(stderr,message,arg_pointer);
   va_end(arg_pointer);
#endif
#endif

#else /* INTERP */
   fflush(stdout);
   va_start(arg_pointer,message);
   vfprintf(stderr,message,arg_pointer);
   va_end(arg_pointer);

#endif

#endif

   fprintf(stderr,"\n");
   fflush(stderr);

   /* purge temporary files */

#ifdef INTERP
   close_io(SETL_SYSTEM_VOID);
#endif

#ifdef COMPILER
   if (I2_FILE != NULL)
      close_lib();
   if (os_access(I2_FNAME,0) == 0)
      unlink(I2_FNAME);
#ifndef DYNAMIC_COMP
   if (i1_file != NULL)
      fclose(i1_file);
   if (os_access(i1_fname,0) == 0)
      unlink(i1_fname);
#endif
#endif

   exit(GIVEUP_EXIT);

}

#endif /* PLUGIN */

/*\
 *  \function{user\_interrupt()}
 *
 *  This function is installed as the user interrupt handler.  We want to
 *  call \verb"giveup()" in that event since we may have to clean up some
 *  work files.
\*/

void user_interrupt(
   int interrupt_num)

{

#ifdef TSAFE
   giveup(NULL,"\n*** Interrupted ***");
#else
   giveup("\n*** Interrupted ***");
#endif

}

/*\
 *  \function{segment\_error()}
 *
 *  This function is installed as the segmentation error handler.  We
 *  want to call abend in that case to give some clue as to the location
 *  of the error.
\*/

#if UNIX | VMS
#ifdef DEBUG

#ifdef INTERP
int i_segment_error()

{

   fflush(stdout);
#ifdef TSAFE
   abend(NULL,msg_segment_error);
#else
   abend(msg_segment_error);
#endif

}
#endif
#ifdef COMPILER
int c_segment_error()

{

   fflush(stdout);
#ifdef TSAFE
   giveup(NULL,"Segmentation error");
#else
   giveup("Segmentation error");
#endif

}
#endif

#endif
#endif

/*\
 *  \function{trap()}
 *
 *  This function is called when the program discovers an internal bug.
 *  It will probably be used only during debugging, but we will use
 *  conditional compilation to leave it in the source.
\*/

#ifdef TRAPS

#if UNIX_VARARGS
void trap(file_name,line_number,message,va_alist)
   char *file_name;                    /* source file name                  */
   int line_number;                    /* line number where trap tripped    */
   char *message;                      /* first argument of message         */
   va_dcl                              /* any other arguments               */
#else
void trap(
   char *file_name,                    /* source file name                  */
   int line_number,                    /* line number where trap tripped    */
   char *message,                      /* first argument of message         */
   ...)                                /* any other arguments               */
#endif

{
va_list arg_pointer;                   /* argument list pointer             */
#if UNIX_VARARGS
static char vp_buffer[1024];           /* vsprintf buffer area              */
char vp_pattern[20];                   /* saved vsprintf string             */
char vp_size;                          /* saved vp_size field               */
char vp_c_arg;                         /* hold character arg                */
char *vp_s_arg;                        /* hold string                       */
int vp_i_arg;                          /* hold integer arg                  */
int *vp_n_arg;                         /* hold integer pointer              */
void *vp_p_arg;                        /* hold void pointer                 */
long vp_l_arg;                         /* hold long arg                     */
double vp_f_arg;                       /* hold float arg                    */
char *vp_p, *vp_s, *vp_t;              /* used to traverse buffer           */
#endif

   fflush(stdout);
   fprintf(stderr,
           "*** Program trap in %s, at line %d ***\n",
           file_name,
           line_number);
   /*
    *  The following ugliness accommodates MIPS brain damage.  First, the
    *  stdarg.h macros in gcc don't work.  To make matters worse, the
    *  varargs.h macros don't work with the v?printf functions.  So, I
    *  have to do this futzing myself.
    */

#if UNIX_VARARGS

   va_start(arg_pointer);
   vp_s = message;
   vp_t = vp_buffer;
   while (*vp_s) {

      /* find the next format string */

      while (*vp_s && *vp_s != '%')  
         *vp_t++ = *vp_s++;

      if (!*vp_s) {
         *vp_t = '\0';
         break;
      }
      if (*(vp_s+1) == '%') {
         *vp_t++ = '%';
         vp_s += 2;
         continue;
      }
      vp_p = vp_pattern;
      *vp_p++ = *vp_s++;
      
      /* skip flags */

      while (*vp_s == '+' ||
             *vp_s == '-' ||
             *vp_s == '#' ||
             *vp_s == ' ')
         *vp_p++ = *vp_s++;

      /* skip width, precision */

      if (is_digit(*vp_s,10)) {
         while (is_digit(*vp_s,10))
            *vp_p++ = *vp_s++;
         if (*vp_s == '.') {
            *vp_p++ = *vp_s++;
            while (is_digit(*vp_s,10))
               *vp_p++ = *vp_s++;
         }
      }

      /* skip vp_size */

      if (*vp_s == 'h' ||
          *vp_s == 'l' ||
          *vp_s == 'L') {

         vp_size = *vp_s;
         *vp_p++ = *vp_s++;

      }
      else {
       
         vp_size = 0;

      }

      /* finally, we should have the type field */
     
      *vp_p = *vp_s++;
      *(vp_p+1) = '\0';
      switch (*vp_p) {

         case 'c' :

            vp_c_arg = va_arg(arg_pointer,char);
            sprintf(vp_t,vp_pattern,vp_c_arg);
            break;

         case 'd' :
         case 'i' :
         case 'o' :
         case 'u' :
         case 'x' :
         case 'X' :

            if (vp_size == 'l') { 
               vp_l_arg = va_arg(arg_pointer,long);
               sprintf(vp_t,vp_pattern,vp_l_arg);
            }
            else {
               vp_i_arg = va_arg(arg_pointer,int);
               sprintf(vp_t,vp_pattern,vp_i_arg);
            }
            break;

         case 'e' :
         case 'E' :
         case 'f' :
         case 'g' :
         case 'G' :
          
            vp_f_arg = va_arg(arg_pointer,double);
            sprintf(vp_t,vp_pattern,vp_f_arg);
            break;

         case 's' :
          
            vp_s_arg = va_arg(arg_pointer,char *);
            sprintf(vp_t,vp_pattern,vp_s_arg);
            break;

         case 'n' :

            vp_n_arg = va_arg(arg_pointer,int *);
            sprintf(vp_t,vp_pattern,vp_n_arg);
            break;

         case 'p' :

            vp_p_arg = va_arg(arg_pointer,void *);
            sprintf(vp_t,vp_pattern,vp_p_arg);
            break;

         default :
            strcpy(vp_t,vp_pattern);

      }

      /* skip past the appended string */

      while (*vp_t)
         vp_t++;

   }

   fputs(vp_buffer,stderr);
   va_end(arg_pointer);

#else

   va_start(arg_pointer,message);
   vfprintf(stderr,message,arg_pointer);
   va_end(arg_pointer);

#endif

   fprintf(stderr,"\n");
   fflush(stderr);

   /* purge temporary files */

#ifdef COMPILER
   if (I2_FILE != NULL)
      close_lib();
   if (os_access(I2_FNAME,0) == 0)
      unlink(I2_FNAME);
#endif

   exit(TRAP_EXIT);

}

#endif
