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
 *  \package{Mailboxs}
 *
 *  This package contains definitions of the structures used to implement
 *  infinite length mailboxes, and several low level functions to
 *  manipulate those structures.  We freely confess that we used a very
 *  ugly, non-portable C coding style here, in hopes of getting a fast
 *  implementation.  We have tried to isolate this ugliness to the macros
 *  and functions which allocate and release nodes.  In particular, there
 *  are some unsafe castes, so please make sure this file is compiled
 *  with unsafe optimizations disabled!
 *
 *  \texify{mailbox.h}
 *
 *  \packagebody{Mailboxs}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "mailbox.h"                   /* mailboxes                         */

/* performance tuning constants */

#define MBOX_HEADER_BLOCK_SIZE  50     /* mailbox header block size         */
#define MBOX_CELL_BLOCK_SIZE   200     /* mailbox cell block size           */

/*\
 *  \function{alloc\_mailbox\_headers()}
 *
 *  This function allocates a block of mailbox headers and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste header items to pointers to header items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the header node.
 *  It will work provided a header item is larger than a pointer, which
 *  is the case.
\*/

void alloc_mailbox_headers(
   SETL_SYSTEM_PROTO_VOID)

{      
mailbox_h_ptr_type new_block;          /* first header in allocated block   */

   /* allocate a new block */

   new_block = (mailbox_h_ptr_type)malloc((size_t)
         (MBOX_HEADER_BLOCK_SIZE * sizeof(struct mailbox_h_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (MAILBOX_H_NEXT_FREE = new_block;
        MAILBOX_H_NEXT_FREE < new_block + MBOX_HEADER_BLOCK_SIZE - 1;
        MAILBOX_H_NEXT_FREE++) {

      *((mailbox_h_ptr_type *)MAILBOX_H_NEXT_FREE) = MAILBOX_H_NEXT_FREE + 1;

   }

   *((mailbox_h_ptr_type *)MAILBOX_H_NEXT_FREE) = NULL;

   /* set next free node to new block */

   MAILBOX_H_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{alloc\_mailbox\_cells()}
 *
 *  This function allocates a block of mailbox cells and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste cell items to pointers to cell items in order to form the free
 *  list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently, while
 *  at the same time avoiding an extra pointer on the cell node.  It
 *  will work provided a cell item is larger than a pointer, which is
 *  the case.
\*/

void alloc_mailbox_cells(
   SETL_SYSTEM_PROTO_VOID)

{      
mailbox_c_ptr_type new_block;          /* first cell in allocated block     */

   /* allocate a new block */

   new_block = (mailbox_c_ptr_type)malloc((size_t)
         (MBOX_CELL_BLOCK_SIZE * sizeof(struct mailbox_c_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (MAILBOX_C_NEXT_FREE = new_block;
        MAILBOX_C_NEXT_FREE < new_block + MBOX_CELL_BLOCK_SIZE - 1;
        MAILBOX_C_NEXT_FREE++) {

      *((mailbox_c_ptr_type *)MAILBOX_C_NEXT_FREE) = MAILBOX_C_NEXT_FREE + 1;

   }

   *((mailbox_c_ptr_type *)MAILBOX_C_NEXT_FREE) = NULL;

   /* set next free node to new block */

   MAILBOX_C_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{free\_mailbox()}
 *
 *  This function releases an entire mailbox structure.
\*/

void free_mailbox(
   SETL_SYSTEM_PROTO
   mailbox_h_ptr_type header)          /* mailbox to be freed               */

{      
mailbox_c_ptr_type t1,t2;              /* temporary looping variables       */

   t1 = header->mb_head;

   while (t1 != NULL) {

      t2 = t1;
      t1 = t1->mb_next;
      unmark_specifier(&(t2->mb_spec));
      free_mailbox_cell(t2);

   }

   free_mailbox_header(header);

}

