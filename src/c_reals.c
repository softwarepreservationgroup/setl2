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
 *  \package{The Real Literal Table}
 *
 *  This table stores real literals from the time they are scanned or
 *  read from a library until the module is written to a library.  It
 *  contains low-level functions for manipulating these structures and a
 *  function to convert character strings into real structures.
 *
 *  \texify{reals.h}
 *
 *  \packagebody{Real Literal Table}
\*/


#include "system.h"                    /* SETL2 system constants            */

/* standard C header files */

#include <math.h>                      /* real literal table                */
#ifdef HAVE_SIGNAL_H
#include <signal.h>                    /* signal macros                     */
#endif


/* SETL2 system header files */


#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "chartab.h"                   /* character type table              */
#include "c_reals.h"                     /* real literals                     */
#include "listing.h"                   /* source and error listings         */

/* performance tuning constants */

#define REALS_BLOCK_SIZE      50       /* real block size                   */

/* generic table item structure (real literal use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct c_real_item ti_data;        /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[REALS_BLOCK_SIZE];
                                       /* array of table items              */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */
static int bad_float_flag;             /* YES if floating point error       */

/*\
 *  \function{math\_error()}
 *
 *  This function is installed as the floating point error handler.  It
 *  just calls \verb"abend()".
\*/

static void math_error(
   int interrupt_num)

{

   bad_float_flag = YES;

}

#if INFNAN

void c_infnan()

{
   bad_float_flag = YES;
}

#endif

/*\
 *  \function{init\_real()}
 *
 *  This procedure initializes the real number table.  All we do is push
 *  everything onto the free list.
\*/

void init_compiler_reals(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *tb_ptr;            /* work table pointer                */

   /* set floating point error trap */

#ifdef HAVE_SIGNAL_H
   if (signal(SIGFPE,math_error) == SIG_ERR)
      giveup(SETL_SYSTEM msg_trap_float);
#endif

   while (table_block_head != NULL) {
      tb_ptr = table_block_head;
      table_block_head = table_block_head->tb_next;
      free((void *)tb_ptr);
   }
   table_next_free = NULL;

}

/*\
 *  \function{get\_real()}
 *
 *  This procedure allocates a real node. It is just like most of the
 *  other dynamic table allocation functions in the compiler.
\*/

c_real_ptr_type get_real(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *old_head;          /* name table block list head        */
c_real_ptr_type return_ptr;            /* return pointer                    */

   if (table_next_free == NULL) {

      /* allocate a new block */

      old_head = table_block_head;
      table_block_head = (struct table_block *)
                         malloc(sizeof(struct table_block));
      if (table_block_head == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      table_block_head->tb_next = old_head;

      /* link items on the free list */

      for (table_next_free = table_block_head->tb_data;
           table_next_free <=
              table_block_head->tb_data + REALS_BLOCK_SIZE - 2;
           table_next_free++) {
         table_next_free->ti_union.ti_next = table_next_free + 1;
      }

      table_next_free->ti_union.ti_next = NULL;
      table_next_free = table_block_head->tb_data;
   }

   /* at this point, we know the free list is not empty */

   return_ptr = &(table_next_free->ti_union.ti_data);
   table_next_free = table_next_free->ti_union.ti_next;

   /* initialize the new entry */

   clear_real(return_ptr);

   return return_ptr;

}

/*\
 *  \function{free\_real()}
 *
 *  This function is the complement to \verb"get_real()". All we do is
 *  push the passed real table pointer on the free list.
\*/

void free_real(
   c_real_ptr_type discard)            /* item to be discarded              */

{

   ((struct table_item *)discard)->ti_union.ti_next = table_next_free;
   table_next_free = (struct table_item *)discard;

}

/*\
 *  \function{char\_to\_real()}
 *
 *  This function accepts a character string and returns a pointer to a
 *  real.  It assumes that the character string is a valid real literal.
\*/

c_real_ptr_type char_to_real(
   SETL_SYSTEM_PROTO
   char *in_string,                    /* string to be converted            */
   file_pos_type *file_pos)            /* literal's file position           */

{
c_real_ptr_type return_ptr;            /* return value                      */
double whole_part;                     /* whole part of real                */
double decimal_part;                   /* fraction part of real             */
double decimal_divisor;                /* decimal divisor                   */
int exponent_sign;                     /* 1 => positive exponent, -1 o/w    */
int exponent;                          /* exponent value                    */
int base;                              /* integer base                      */
int special_base;                      /* YES if we have a base change      */
char *p;                               /* temporary looping variable        */

   /* first, check for a specified base */

   for (p = in_string; *p && *p != '.' && *p != '#'; p++);

   /* if we found a '#', we have a base change */

   special_base = NO;
   if (*p == '#') {

      base = 0;
      special_base = YES;
      for (p = in_string; *p != '#'; p++)
         base = base * 10 + numeric_val(*p);

      p++;

   }
   else {

      base = 10;
      p = in_string;

   }

   /* pick out the whole part */

   whole_part = 0.0;
   while (*p != '.') {

      if (*p == '_') {
         p++;
         continue;
      }

      whole_part = whole_part * (double)base +
                   (double)(numeric_val(*p));
      p++;

   }

   p++;

   /* pick out the decimal part */

   decimal_part = 0.0;
   decimal_divisor = 1.0;
   while (*p && *p != '#' && *p != 'e' && *p != 'E') {

      if (*p == '_') {
         p++;
         continue;
      }

      decimal_part = decimal_part * (double)base +
                     (double)(numeric_val(*p));
      decimal_divisor *= base;
      p++;

   }

   /* skip the trailing pound sign */

   if (special_base)
      p++;

   /* pick out the exponent */

   exponent = 0;
   exponent_sign = 1;
   if (*p == 'e' || *p == 'E') {

      p++;
      if (*p == '-') {
         exponent_sign = -1;
         p++;
      }
      else if (*p == '+') {
         p++;
      }

      while (*p) {

         exponent = exponent * 10 + numeric_val(*p);
         p++;

      }
   }

   return_ptr = get_real(SETL_SYSTEM_VOID);
   bad_float_flag = NO;
   return_ptr->r_value = (whole_part +
                         (decimal_part / decimal_divisor)) *
                         pow((double)base,
                             (double)(exponent * exponent_sign));

   if (bad_float_flag) {

      error_message(SETL_SYSTEM file_pos,
                    "Floating point literal out of range => %s\n",
                    in_string);

   }

   return return_ptr;

}

