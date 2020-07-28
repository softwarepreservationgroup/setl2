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
 *  \package{Miscellaneous Built-In Procedures}
 *
 *  In this package we have built-in procedures which don't fit in to
 *  some other category.
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "builtins.h"                  /* built-in symbols                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_strngs.h"                  /* strings                           */


/*\
 *  \function{setl2\_newat()}
 *
 *  This function is the \verb"newat" built-in function.  We allocate a
 *  new atom.
\*/

void setl2_newat(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
static int32 atom_number = 0;          /* next atom to be allocated         */

   unmark_specifier(target);
   target->sp_form = ft_atom;
   target->sp_val.sp_atom_num = atom_number++;

   return;

}

/*\
 *  \function{setl2\_date()}
 *
 *  This function is the \verb"date" built-in function.  We return the
 *  date as a character string.
\*/

void setl2_date(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector                   */
   specifier *target)                  /* return value                      */

{
time_t bintime;                        /* time in seconds                   */
struct tm *localt;                      /* local time                        */
string_h_ptr_type string_hdr;          /* string header pointer             */
string_c_ptr_type string_cell;         /* string cell pointer               */
char *p;                               /* temporary looping variable        */

   /* get the time from the operating system */

   time(&bintime);

   /* create a character string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   get_string_cell(string_cell);
   string_cell->s_next = string_cell->s_prev = NULL;
   string_hdr->s_head = string_hdr->s_tail = string_cell;
   localt = localtime(&bintime);
   sprintf(string_cell->s_cell_value,
           "%2d/%2d/%2d",
           localt->tm_mon + 1,
           localt->tm_mday,
           localt->tm_year);

   for (p = string_cell->s_cell_value; *p; p++) {
      if (*p == ' ')
         *p = '0';
   }

   string_hdr->s_length = strlen(string_cell->s_cell_value);

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

}

/*\
 *  \function{setl2\_time()}
 *
 *  This function is the \verb"time" built-in function.  We return the
 *  time as a character string.
\*/

void setl2_time(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector                   */
   specifier *target)                  /* return value                      */

{
time_t bintime;                        /* time in seconds                   */
struct tm *localt;                      /* local time                        */
string_h_ptr_type string_hdr;          /* string header pointer             */
string_c_ptr_type string_cell;         /* string cell pointer               */
char *p;                               /* temporary looping variable        */

   /* get the time from the operating system */

   time(&bintime);

   /* create a character string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   get_string_cell(string_cell);
   string_cell->s_next = string_cell->s_prev = NULL;
   string_hdr->s_head = string_hdr->s_tail = string_cell;
   localt = localtime(&bintime);
   sprintf(string_cell->s_cell_value,
           "%2d:%2d:%2d",
           localt->tm_hour,
           localt->tm_min,
           localt->tm_sec);

   for (p = string_cell->s_cell_value; *p; p++) {
      if (*p == ' ')
         *p = '0';
   }

   string_hdr->s_length = strlen(string_cell->s_cell_value);

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

} 
