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
 *  \package{The Integer Literal Table}
 *
 *  This table stores integer literals from the time they are scanned or
 *  read from a library until the module is written to a library.  It
 *  contains low-level functions for manipulating these structures and a
 *  function to convert character strings into integer structures.
 *
 *  \texify{integers.h}
 *
 *  \packagebody{Integer Literal Table}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "chartab.h"                   /* character type table              */
#include "c_integers.h"                  /* integer literals                  */


/* performance tuning constants */

#define INTEGERS_BLOCK_SIZE   50       /* integer block size                */

/* generic table item structure (integer literal use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct integer_item ti_data;     /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[INTEGERS_BLOCK_SIZE];
                                       /* array of table items              */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */

/*\
 *  \function{init\_integers()}
 *
 *  This procedure initializes the integer literal table.  All we do is
 *  free all the allocated blocks.
\*/

void init_integers()

{
struct table_block *tb_ptr;            /* work table pointer                */

   while (table_block_head != NULL) {
      tb_ptr = table_block_head;
      table_block_head = table_block_head->tb_next;
      free((void *)tb_ptr);
   }
   table_next_free = NULL;

}

/*\
 *  \function{get\_integer()}
 *
 *  This procedure allocates a integer node. It is just like most of the
 *  other dynamic table allocation functions in the compiler.
\*/

integer_ptr_type get_integer(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *old_head;          /* name table block list head        */
integer_ptr_type return_ptr;           /* return pointer                    */

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
              table_block_head->tb_data + INTEGERS_BLOCK_SIZE - 2;
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

   clear_integer(return_ptr);

   return return_ptr;

}

/*\
 *  \function{free\_integer()}
 *
 *  This function is the complement to \verb"get_integer()". All we do is
 *  push the passed integer table pointer on the free list.
\*/

void free_compiler_integer(
   integer_ptr_type discard)           /* item to be discarded              */

{

   ((struct table_item *)discard)->ti_union.ti_next = table_next_free;
   table_next_free = (struct table_item *)discard;

}

/*\
 *  \function{char\_to\_int()}
 *
 *  This function accepts a character string and returns a pointer to an
 *  integer list.  It assumes that the character string is a valid
 *  integer.
\*/

integer_ptr_type char_to_int(
   SETL_SYSTEM_PROTO
   char *in_string)                    /* string to be converted            */

{
integer_ptr_type return_ptr;           /* return value                      */
int base;                              /* integer base                      */
int32 max_multiplier;                  /* maximum value we can multiply     */
                                       /* by cell                           */
int32 multiplier;                      /* actual value we multiply by cell  */
int32 addend;                          /* amount to add to next cell        */
integer_ptr_type last_cell;            /* last cell added to list           */
integer_ptr_type cell_ptr;             /* work cell pointer                 */
char *p;                               /* temporary looping variable        */

   /* first, check for a specified base */

   for (p = in_string; *p && *p != '#'; p++);

   /* if we found a '#', we have a base change */

   if (*p) {

      base = 0;
      for (p = in_string; *p != '#'; p++)
         base = base * 10 + numeric_val(*p);

      p++;

   }
   else {

      base = 10;
      p = in_string;

   }

   /* start out with an empty list */

   return_ptr = get_integer(SETL_SYSTEM_VOID);
   last_cell = return_ptr;

   /* find the maximum cell multiplier */

   max_multiplier = MAX_INT_CELL / base;

   /* loop over the entire string */

   for (;;) {

      /* pick out as many digits as we can handle */

      multiplier = 1;
      addend = 0;
      while (*p && *p != '#' && multiplier < max_multiplier) {

         if (*p == '_') {
            p++;
            continue;
         }

         addend = addend * base + numeric_val(*p);
         multiplier = multiplier * base;
         p++;

      }

      /* traverse the list, updating each cell */

      for (cell_ptr = return_ptr;
           cell_ptr != NULL || addend != 0;
           cell_ptr = cell_ptr->i_next) {

         /* if the next pointer is null, extend the list */

         if (cell_ptr == NULL) {
            cell_ptr = get_integer(SETL_SYSTEM_VOID);
            if (last_cell != NULL)
               last_cell->i_next = cell_ptr;
            cell_ptr->i_prev = last_cell;
            last_cell = cell_ptr;
         }

         /* update the cell */

         addend = cell_ptr->i_value * multiplier + addend;
         cell_ptr->i_value = addend & MAX_INT_CELL;
         addend >>= INT_CELL_WIDTH;

      }

      /* break if we're at the end of the string */

      if (!*p || *p == '#')
         break;

   }

   /* close the pointer cycle */

   return_ptr->i_prev = last_cell;
   last_cell->i_next = return_ptr;

   return return_ptr;

}

