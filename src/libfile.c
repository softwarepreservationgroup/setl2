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
 *  \package{The Library File Table}
 *
 *  The library manager must keep a table of valid library files
 *  available to it.  During input searches, it will open and search
 *  every file in that table, to find a matching unit.  This package just
 *  provides the primitive functions to allocate and deallocate nodes in
 *  that table.  It should be regarded as internal to the library
 *  manager, since that is the only package which uses it.  It is made
 *  separate from that package simply to allow the names used to
 *  dynamically allocate space to be re-used by other dynamic structures
 *  in the library manager.
 *
 *  \texify{libfile.h}
 *
 *  \packagebody{Library File Table}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "libman.h"                    /* library manager                   */
#include "libfile.h"                   /* library file table                */

/* performance tuning constants */

#define LIBFILE_BLOCK_SIZE    10       /* library file block size           */

/* generic table item structure (library file use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct libfile_item ti_data;     /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[LIBFILE_BLOCK_SIZE];
                                       /* array of table items              */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */

/*\
 *  \function{get\_libfile()}
 *
 *  This procedure allocates a library file table node. It is just like
 *  most of the other dynamic table allocation functions in the
 *  compiler.
\*/

libfile_ptr_type get_libfile(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *old_head;          /* name table block list head        */
libfile_ptr_type return_ptr;           /* return pointer                    */

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
              table_block_head->tb_data + LIBFILE_BLOCK_SIZE - 2;
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

   clear_libfile(return_ptr);

   return return_ptr;

}

/*\
 *  \function{free\_libfile()}
 *
 *  This function is the complement to \verb"get_libfile()". All we do is
 *  push the passed library file pointer on the free list.
\*/

void free_libfile(
   libfile_ptr_type discard)           /* item to be discarded              */

{

   ((struct table_item *)discard)->ti_union.ti_next = table_next_free;
   table_next_free = (struct table_item *)discard;

}

