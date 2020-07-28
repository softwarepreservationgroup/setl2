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
 *  \package{Source and Error Listings}
 *
 *  This file contains functions and data which produce program and error
 *  listings.
 *
 *  \texify{listing.h}
 *
 *  \packagebody{Listing}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "chartab.h"                   /* character type table              */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "listing.h"                   /* source and error listings         */

/* standard C header files */

#if UNIX_VARARGS
#include <varargs.h>                   /* variable argument macros          */
#else
#include <stdarg.h>                    /* variable argument macros          */
#endif

#ifdef PLUGIN

#define fprintf plugin_fprintf
#define fputs   plugin_fputs
#endif

/* constants */

#define WARNING 0                      /* warning message                   */
#define SETL_ERROR 1                        /* error message                     */
#define INFO 2                         /* information message               */
#define ERR_BLOCK_SIZE     50          /* error table block size            */

/* error message structure */

typedef struct {
   file_pos_type em_file_pos;          /* file position                     */
   int em_msg_type;                    /* message type                      */
   char *em_msg_text;                  /* message text                      */
} err_msg;

/* error message table */

static err_msg *err_table = NULL;      /* error message table               */
static err_top = 0;                    /* top of error message table        */
static err_max = 0;                    /* size of error message table       */

/* forward declarations */

static void expand_err_table(SETL_SYSTEM_PROTO_VOID);
                                       /* expand the error message table    */
static int err_msg_cmp(void *, void *);
                                       /* compare for qsort                 */

/*\
 *  \function{error\_message()}
 *
 *  This function saves an error message for printing later.  We keep a
 *  table of error messages, rather than just printing them.  Due to the
 *  syntax of SETL2, we can not find all possible semantic errors in the
 *  first pass over the program.  This means that if we print errors as
 *  we find them, we will print them out of sequence, which is confusing
 *  to the programmer.  To avoid that, we store error messages in a
 *  table, then after the program has been completely parsed, we sort the
 *  error messages and print them out.
\*/

#if UNIX_VARARGS
void error_message(SETL_SYSTEM_PROTO err_file_pos,message,va_alist)
   struct file_pos_item *err_file_pos;
                                       /* error's file position             */
   char *message;                      /* first argument of message         */
   va_dcl                              /* other message arguments           */
#else
void error_message(
   SETL_SYSTEM_PROTO
   struct file_pos_item *err_file_pos,
                                       /* error's file position             */
   char *message,                      /* first argument of message         */
   ...)                                /* other message arguments           */
#endif

{
char errmess[300];                     /* buffer to expand error message    */
va_list arg_pointer;                   /* argument list pointer             */
#if UNIX_VARARGS
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

   /* expand error table if necessary */

   if (err_top == err_max)
      expand_err_table(SETL_SYSTEM_VOID);

   /* copy the file position */

   if (err_file_pos == NULL) {
      err_table[err_top].em_file_pos.fp_line = 0;
      err_table[err_top].em_file_pos.fp_column = 0;
   }
   else {
      err_table[err_top].em_file_pos.fp_line = err_file_pos->fp_line;
      err_table[err_top].em_file_pos.fp_column = err_file_pos->fp_column;
   }

   /* expand the error message text */

   /*
    *  The following ugliness accommodates MIPS brain damage.  First, the
    *  stdarg.h macros in gcc don't work.  To make matters worse, the
    *  varargs.h macros don't work with the v?printf functions.  So, I
    *  have to do this futzing myself.
    */

#if UNIX_VARARGS

   va_start(arg_pointer);
   vp_s = message;
   vp_t = errmess;
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

   va_end(arg_pointer);

#else

   va_start(arg_pointer,message);
   vsprintf(errmess,message,arg_pointer);
   va_end(arg_pointer);

#endif

   err_table[err_top].em_msg_text =
                            (char *)malloc((size_t)(strlen(errmess) + 1));
   if (err_table[err_top].em_msg_text == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   strcpy(err_table[err_top].em_msg_text,errmess);

   err_table[err_top].em_msg_type = SETL_ERROR;

   err_top++;

   UNIT_ERROR_COUNT++;
   return;

}

/*\
 *  \function{warning\_message()}
 *
 *  This function saves a warning message for printing later.  We keep a
 *  table of error messages, rather than just printing them.  Due to the
 *  syntax of SETL2, we can not find all possible semantic errors in the
 *  first pass over the program.  This means that if we print errors as
 *  we find them, we will print them out of sequence, which is confusing
 *  to the programmer.  To avoid that, we store error messages in a
 *  table, then after the program has been completely parsed, we sort the
 *  error messages and print them out.
\*/

#if UNIX_VARARGS
void warning_message(SETL_SYSTEM_PROTO err_file_pos,message,va_alist)
   struct file_pos_item *err_file_pos;
                                       /* error's file position             */
   char *message;                      /* first argument of message         */
   va_dcl                              /* other message arguments           */
#else
void warning_message(
   SETL_SYSTEM_PROTO
   struct file_pos_item *err_file_pos,
                                       /* error's file position             */
   char *message,                      /* first argument of message         */
   ...)                                /* other message arguments           */
#endif

{
char errmess[300];                     /* buffer to expand error message    */
va_list arg_pointer;                   /* argument list pointer             */
#if UNIX_VARARGS
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

   /* expand error table if necessary */

   if (err_top == err_max)
      expand_err_table(SETL_SYSTEM_VOID);

   /* copy the file position */

   if (err_file_pos == NULL) {
      err_table[err_top].em_file_pos.fp_line = 0;
      err_table[err_top].em_file_pos.fp_column = 0;
   }
   else {
      err_table[err_top].em_file_pos.fp_line = err_file_pos->fp_line;
      err_table[err_top].em_file_pos.fp_column = err_file_pos->fp_column;
   }

   /* expand the warning message text */

   /*
    *  The following ugliness accommodates MIPS brain damage.  First, the
    *  stdarg.h macros in gcc don't work.  To make matters worse, the
    *  varargs.h macros don't work with the v?printf functions.  So, I
    *  have to do this futzing myself.
    */

#if UNIX_VARARGS

   va_start(arg_pointer);
   vp_s = message;
   vp_t = errmess;
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

   va_end(arg_pointer);

#else

   va_start(arg_pointer,message);
   vsprintf(errmess,message,arg_pointer);
   va_end(arg_pointer);

#endif

   err_table[err_top].em_msg_text =
                            (char *)malloc((size_t)(strlen(errmess) + 1));
   if (err_table[err_top].em_msg_text == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   strcpy(err_table[err_top].em_msg_text,errmess);

   err_table[err_top].em_msg_type = SETL_ERROR;

   err_top++;

   UNIT_WARNING_COUNT++;
   return;

}

/*\
 *  \function{info\_message()}
 *
 *  This function saves an information message for printing later.  We
 *  keep a table of error messages, rather than just printing them.  Due
 *  to the syntax of SETL2, we can not find all possible semantic errors in
 *  the first pass over the program.  This means that if we print errors
 *  as we find them, we will print them out of sequence, which is
 *  confusing to the programmer.  To avoid that, we store error messages
 *  in a table, then after the program has been completely parsed, we
 *  sort the error messages and print them out.
\*/

#if UNIX_VARARGS
void info_message(SETL_SYSTEM_PROTO err_file_pos,message,va_alist)
   struct file_pos_item *err_file_pos;
                                       /* error's file position             */
   char *message;                      /* first argument of message         */
   va_dcl                              /* other message arguments           */
#else
void info_message(
   SETL_SYSTEM_PROTO
   struct file_pos_item *err_file_pos,
                                       /* error's file position             */
   char *message,                      /* first argument of message         */
   ...)                                /* other message arguments           */
#endif

{
char errmess[300];                     /* buffer to expand error message    */
va_list arg_pointer;                   /* argument list pointer             */
#if UNIX_VARARGS
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

   /* expand error table if necessary */

   if (err_top == err_max)
      expand_err_table(SETL_SYSTEM_VOID);

   /* copy the file position */

   if (err_file_pos == NULL) {
      err_table[err_top].em_file_pos.fp_line = 0;
      err_table[err_top].em_file_pos.fp_column = 0;
   }
   else {
      err_table[err_top].em_file_pos.fp_line = err_file_pos->fp_line;
      err_table[err_top].em_file_pos.fp_column = err_file_pos->fp_column;
   }

   /* expand the info message text */

   /*
    *  The following ugliness accommodates MIPS brain damage.  First, the
    *  stdarg.h macros in gcc don't work.  To make matters worse, the
    *  varargs.h macros don't work with the v?printf functions.  So, I
    *  have to do this futzing myself.
    */

#if UNIX_VARARGS

   va_start(arg_pointer);
   vp_s = message;
   vp_t = errmess;
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

   va_end(arg_pointer);

#else

   va_start(arg_pointer,message);
   vsprintf(errmess,message,arg_pointer);
   va_end(arg_pointer);

#endif

   err_table[err_top].em_msg_text =
                            (char *)malloc((size_t)(strlen(errmess) + 1));
   if (err_table[err_top].em_msg_text == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   strcpy(err_table[err_top].em_msg_text,errmess);

   err_table[err_top].em_msg_type = SETL_ERROR;

   err_top++;

   return;

}

/*\
 *  \function{expand\_err\_table()}
 *
 *  This function expands the error table.  We store the error table as
 *  an expanding array.  We do not expect the error table to get very
 *  big, so we don't mind some inefficiency here, and since we will
 *  eventually have to sort the error table, it is convenient to store it
 *  as an array.
\*/


static void expand_err_table(SETL_SYSTEM_PROTO_VOID)

{
err_msg *temp_err_table;               /* temporary error table             */
int i;                                 /* temporary looping variable        */

   /* allocate a bigger array */

   temp_err_table = err_table;
   err_table = (err_msg *)malloc(
                  (size_t)((err_max + ERR_BLOCK_SIZE) * sizeof(err_msg)));
   if (err_table == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* copy the old table to the new one */

   for (i = 0; i < err_top; i++)
      memcpy((void *)(err_table + i),
             (void *)(temp_err_table + i),
             sizeof(err_msg));

   err_max += ERR_BLOCK_SIZE;

}


/*\
 *  \function{err\_msg\_cmp}
 *
 *  This function compares two error message structures by file position
 *  and message type.  It returns -1 if the first came before the second,
 *  1 if the second came before the first, and 0 if they are the same.
\*/

static int err_msg_cmp(elem1,elem2)
void *elem1, *elem2;

{

   /* check line */

   if (((err_msg *)elem1)->em_file_pos.fp_line <
       ((err_msg *)elem2)->em_file_pos.fp_line)
      return -1;
   else if (((err_msg *)elem1)->em_file_pos.fp_line >
           ((err_msg *)elem2)->em_file_pos.fp_line)
      return 1;

   /* check column */

   else if (((err_msg *)elem1)->em_file_pos.fp_column <
           ((err_msg *)elem2)->em_file_pos.fp_column)
      return -1;
   else if (((err_msg *)elem1)->em_file_pos.fp_column >
           ((err_msg *)elem2)->em_file_pos.fp_column)
      return 1;

   /* check type */

   else if (((err_msg *)elem1)->em_msg_type <
           ((err_msg *)elem2)->em_msg_type)
      return -1;
   else if (((err_msg *)elem1)->em_msg_type >
           ((err_msg *)elem2)->em_msg_type)
      return 1;

   /* must be equal */

   else
      return 0;

}


/*\
 *  \function{print\_errors()}
 *
 *  This function sorts and prints the error message table.
\*/

void print_errors(SETL_SYSTEM_PROTO_VOID)

{
int i;                                 /* temporary looping variable        */
char position[15];                     /* error position (display form)     */
char *p;                               /* temporary looping variable        */

   /* if there were no errors, don't bother */

   if (err_top == 0)
      return;

   /* sort the table */

   qsort((void *)err_table,
         (size_t)err_top,
         sizeof(err_msg),(int (*) (const void *,const void *))err_msg_cmp);


   /* now print it */

   for (i = 0; i < err_top; i++) {

 
      if ((COMPILER_OPTIONS & VERBOSE_FILES)>0) {
        fprintf(stdout," File : %s\n",C_SOURCE_NAME);
      }
      sprintf(position,"[%d:%d]",err_table[i].em_file_pos.fp_line,
                                 err_table[i].em_file_pos.fp_column);

      for (p = position+strlen(position); p < position+15; *p++ = ' ');
      position[14] = '\0';

      if (err_table[i].em_msg_type == WARNING) {

         fprintf(stdout,
                 "%s WARNING => %s\n",
                 position,
                 err_table[i].em_msg_text);

      }

      else if (err_table[i].em_msg_type == SETL_ERROR) {

         fprintf(stdout,
                 "%s *ERROR* => %s\n",
                 position,
                 err_table[i].em_msg_text);

      }

      else if (err_table[i].em_msg_type == INFO) {

         fprintf(stdout,
                 "                           %s\n",
                 err_table[i].em_msg_text);

      }

   }

}

/*\
 *  \function{print\_listing}
 *
 *  This function prints a source listing.
\*/

void print_listing(SETL_SYSTEM_PROTO_VOID)

{
FILE *src_file,*list_file;
char buffer[200];
int line_num;
int i;                                 /* temporary looping variable        */

   list_file = fopen(LIST_FNAME,"w");
   if (list_file == NULL)
      giveup(SETL_SYSTEM "Unable to open listing file => %s",LIST_FNAME);

   src_file = fopen(C_SOURCE_NAME,"r");
   if (src_file == NULL)
      giveup(SETL_SYSTEM msg_missing_source_file,C_SOURCE_NAME);

   fputs("\f",list_file);
   line_num = 1;
   i = 0;
   for (;;) {

      if (fgets(buffer,200,src_file) == NULL)
         break;

      fprintf(list_file,"%5d  %s",line_num,buffer);

      while (i < err_top && err_table[i].em_file_pos.fp_line == line_num) {

         if (err_table[i].em_msg_type == WARNING) {

            fprintf(list_file,
                    "%d : WARNING => %s\n",
                    err_table[i].em_file_pos.fp_line,
                    err_table[i].em_msg_text);

         }

         else if (err_table[i].em_msg_type == SETL_ERROR) {

            fprintf(list_file,
                    "%d : *ERROR* => %s\n",
                    err_table[i].em_file_pos.fp_line,
                    err_table[i].em_msg_text);

         }

         else if (err_table[i].em_msg_type == INFO) {

            fprintf(list_file,
                    "                 %s\n",
                    err_table[i].em_msg_text);

         }

         i++;

      }

      line_num++;
   }

   fclose(src_file);
   fclose(list_file);

}

/*\
 *  \function{print\_listing}
 *
 *  This function prints a source listing.
\*/

void generate_markup(SETL_SYSTEM_PROTO_VOID)

{
FILE *src_in,*src_out;
char buffer[200];
int line_num;
int i,j,k;                             /* temporary looping variable        */
char temp_name[PATH_LENGTH + 1];

   strcpy(temp_name,C_SOURCE_NAME);
   strcpy(temp_name + strlen(temp_name) - 3,"mrk");

   src_out = fopen(temp_name,"w");
   if (src_out == NULL)
      giveup(SETL_SYSTEM "Unable to open marked source file => %s",temp_name);

   src_in = fopen(C_SOURCE_NAME,"r");
   if (src_in == NULL)
      giveup(SETL_SYSTEM msg_missing_source_file,C_SOURCE_NAME);

   line_num = 1;
   i = 0;
   for (;;) {

      if (fgets(buffer,200,src_in) == NULL)
         break;

      fputs(buffer,src_out);
      if (buffer[strlen(buffer) - 1] != '\n')
         fputs("\n",src_out);

      while (i < err_top && err_table[i].em_file_pos.fp_line <= line_num) {

         fputs("--!",src_out);
         k = 4;

         for (j = i;
              j < err_top && err_table[j].em_file_pos.fp_line <= line_num;
              j++) {

            while (k < err_table[j].em_file_pos.fp_column) {
               fputs(" ",src_out);
               k++;
            }

            fputs("^",src_out);
            k++;

         }

         fputs("\n",src_out);

         for (;
              i < err_top && err_table[i].em_file_pos.fp_line <= line_num;
              i++) {

            if (err_table[i].em_msg_type == WARNING) {

               fprintf(src_out,
                       "--! WARNING => %s\n",
                       err_table[i].em_msg_text);

            }

            else if (err_table[i].em_msg_type == SETL_ERROR) {

               fprintf(src_out,
                       "--! *ERROR* => %s\n",
                       err_table[i].em_msg_text);

            }

            else if (err_table[i].em_msg_type == INFO) {

               fprintf(src_out,
                       "--!             %s\n",
                       err_table[i].em_msg_text);

            }
         }
      }

      line_num++;

   }

   fclose(src_in);
   fclose(src_out);

}

void free_err_table(SETL_SYSTEM_PROTO_VOID)
{
      err_top = err_max = 0;
      if (err_table!=NULL) {
              free (err_table);
              err_table = NULL;
      }
	  /* Is the next call needed??? I don't think so, but under WIN32
		 the IDE doesn't work without it... */

	  expand_err_table(SETL_SYSTEM_VOID);
}

#ifdef PLUGIN

int setl_num_errors()
{

   /* if there were no errors, don't bother */

   if (err_top != 0) {

   /* sort the table */

   qsort((void *)err_table,
         (size_t)err_top,
         sizeof(err_msg),(int (*) (const void *,const void *))err_msg_cmp);
  
   }

   return err_top;

}


char * setl_err_string(int i)
{
char position[15];                     /* error position (display form)     */
static char err_msg_buf[256];          /* error position (display form)     */
char *p;                               /* temporary looping variable        */

   /* now print it */

   if ((i<0)||(i>=err_top)) {
      strcpy(err_msg_buf,"");
      return err_msg_buf;
   } else {

	   sprintf(position,"[%d:%d]",err_table[i].em_file_pos.fp_line,
                                 err_table[i].em_file_pos.fp_column);

      for (p = position+strlen(position); p < position+15; *p++ = ' ');
      position[14] = '\0';
   }

      if (err_table[i].em_msg_type == WARNING) {

         sprintf(err_msg_buf,
                 "%s WARNING => %s\n",
                 position,
                 err_table[i].em_msg_text);

      }

      else if (err_table[i].em_msg_type == SETL_ERROR) {

         sprintf(err_msg_buf,
                 "%s *ERROR* => %s\n",
                 position,
                 err_table[i].em_msg_text);

      }

      else if (err_table[i].em_msg_type == INFO) {

         sprintf(err_msg_buf,
                 "                           %s\n",
                 err_table[i].em_msg_text);

      }

   return err_msg_buf;

}

#endif
