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
 *  \package{Objects}
 *
 *  This package contains definitions of the structures used to implement
 *  SETL2 objects, and several low level functions to manipulate those
 *  structures.  We freely confess that we used a very ugly, non-portable
 *  C coding style here, in hopes of getting a fast implementation.  We
 *  have tried to isolate this ugliness to the macros and functions which
 *  allocate and release nodes.  In particular, there are some unsafe
 *  castes, so please make sure this file is compiled with unsafe
 *  optimizations disabled!
 *
 *  \texify{objects.h}
 *
 *  \packagebody{Objects}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "unittab.h"                   /* unit table                        */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "objects.h"                   /* objects                           */
#include "process.h"                   /* objects                           */
#include "mailbox.h"                   /* objects                           */

/* performance tuning constants */

#define OBJECT_HEADER_BLOCK_SIZE 100   /* object header block size          */
#define OBJECT_CELL_BLOCK_SIZE  400    /* object cell block size            */
#define SELF_STACK_BLOCK_SIZE   50     /* self block size                   */

/*\
 *  \function{alloc\_object\_headers()}
 *
 *  This function allocates a block of object headers and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste header items to pointers to header items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the header node.
 *  It will work provided a header item is larger than a pointer, which
 *  is the case.
\*/

void alloc_object_headers(
SETL_SYSTEM_PROTO_VOID)

{
object_h_ptr_type new_block;           /* first header in allocated block   */

   /* allocate a new block */

   new_block = (object_h_ptr_type)malloc((size_t)
         (OBJECT_HEADER_BLOCK_SIZE * sizeof(struct object_h_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (OBJECT_H_NEXT_FREE = new_block;
        OBJECT_H_NEXT_FREE < new_block + OBJECT_HEADER_BLOCK_SIZE - 1;
        OBJECT_H_NEXT_FREE++) {

      *((object_h_ptr_type *)OBJECT_H_NEXT_FREE) = OBJECT_H_NEXT_FREE + 1;

   }

   *((object_h_ptr_type *)OBJECT_H_NEXT_FREE) = NULL;

   /* set next free node to new block */

   OBJECT_H_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{alloc\_object\_cells()}
 *
 *  This function allocates a block of object cells and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste cell items to pointers to cell items in order to form the free
 *  list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently, while
 *  at the same time avoiding an extra pointer on the cell node.  It
 *  will work provided a cell item is larger than a pointer, which is
 *  the case.
\*/

void alloc_object_cells(
   SETL_SYSTEM_PROTO_VOID)

{
object_c_ptr_type new_block;           /* first cell in allocated block     */

   /* allocate a new block */

   new_block = (object_c_ptr_type)malloc((size_t)
         (OBJECT_CELL_BLOCK_SIZE * sizeof(struct object_c_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (OBJECT_C_NEXT_FREE = new_block;
        OBJECT_C_NEXT_FREE < new_block + OBJECT_CELL_BLOCK_SIZE - 1;
        OBJECT_C_NEXT_FREE++) {

      *((object_c_ptr_type *)OBJECT_C_NEXT_FREE) = OBJECT_C_NEXT_FREE + 1;

   }

   *((object_c_ptr_type *)OBJECT_C_NEXT_FREE) = NULL;

   /* set next free node to new block */

   OBJECT_C_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{free\_object()}
 *
 *  This function copies an entire object structure.
\*/

void free_object(
   SETL_SYSTEM_PROTO
   object_h_ptr_type object_root)      /* root of structure                 */

{
object_h_ptr_type object_work_hdr, object_save_hdr;
                                       /* object work headers               */
unittab_ptr_type class_ptr;            /* object class                      */
int height;                            /* working height of header tree     */
int index;                             /* index within header hash table    */
process_ptr_type process_ptr;

   class_ptr = object_root->o_ntype.o_root.o_class;

   process_ptr = object_root->o_process_ptr;
   if (process_ptr != NULL) {

      process_ptr->pc_prev->pc_next = process_ptr->pc_next;
      process_ptr->pc_next->pc_prev = process_ptr->pc_prev;
      free(process_ptr->pc_pstack);
      free(process_ptr->pc_cstack);

   }

   /* we start iterating from the root, at the left of the header table */

   height = class_ptr->ut_obj_height;
   object_work_hdr = object_root;
   index = 0;

   /* delete nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, delete all the object elements */

      if (!height) {

         for (index = 0; index < OBJ_HEADER_SIZE; index++) {

            if (object_work_hdr->o_child[index].o_cell != NULL) {

               unmark_specifier(
                  &((object_work_hdr->o_child[index].o_cell)->o_spec));
               free_object_cell(object_work_hdr->o_child[index].o_cell);

            }
         }
      }

      /* if we've finished an internal node, move up */

      if (index >= OBJ_HEADER_SIZE) {

         /* when we get back to the root we're finished */

         if (object_work_hdr == object_root)
            break;

         height++;
         index = object_work_hdr->o_ntype.o_intern.o_child_index + 1;
         object_save_hdr = object_work_hdr;
         object_work_hdr = object_work_hdr->o_ntype.o_intern.o_parent;
         free_object_header(object_save_hdr);

         continue;

      }

      /* if we can't move down, continue */

      if (object_work_hdr->o_child[index].o_header == NULL) {

         index++;
         continue;

      }

      /* we can move down, so do so */

      object_work_hdr = object_work_hdr->o_child[index].o_header;
      index = 0;
      height--;

   }

   free_object_header(object_root);

   return;

}

/*\
 *  \function{copy\_object()}
 *
 *  This function copies an entire object structure.
\*/

object_h_ptr_type copy_object(
   SETL_SYSTEM_PROTO
   object_h_ptr_type source_root)      /* object to be copied               */

{
object_h_ptr_type source_work_hdr;     /* used to loop over source internal */
                                       /* nodes                             */
int source_height, source_index;       /* current height and index          */
object_c_ptr_type source_cell;         /* cell pointer                      */
object_h_ptr_type target_root, target_work_hdr;
                                       /* header pointers                   */
object_c_ptr_type target_cell;         /* cell pointer                      */
object_h_ptr_type new_hdr;             /* created header node               */
int i;                                 /* temporary looping variable        */

   /* allocate a new root header node */

   get_object_header(target_root);
   target_root->o_use_count = 1;
   target_root->o_hash_code =
         source_root->o_hash_code;
   target_root->o_ntype.o_root.o_class =
         source_root->o_ntype.o_root.o_class;
   target_root->o_process_ptr =
         source_root->o_process_ptr;

   for (i = 0;
        i < OBJ_HEADER_SIZE;
        target_root->o_child[i++].o_header = NULL);

   /* we start iterating from the root, at the left of the header table */

   source_height = source_root->o_ntype.o_root.o_class->ut_obj_height;
   source_work_hdr = source_root;
   target_work_hdr = target_root;
   source_index = 0;

   /* copy nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, copy all the object elements */

      if (!source_height) {

         for (source_index = 0;
              source_index < OBJ_HEADER_SIZE;
              source_index++) {

            source_cell = source_work_hdr->o_child[source_index].o_cell;
            if (source_cell != NULL) {

               get_object_cell(target_cell);
               target_cell->o_spec.sp_form = source_cell->o_spec.sp_form;
               target_cell->o_spec.sp_val.sp_biggest =
                     source_cell->o_spec.sp_val.sp_biggest;
               target_cell->o_hash_code = source_cell->o_hash_code;
               mark_specifier(&(target_cell->o_spec));
               target_work_hdr->o_child[source_index].o_cell = target_cell;

            }
            else {

               target_work_hdr->o_child[source_index].o_cell = NULL;

            }
         }
      }

      /* if we've finished an internal node, move up */

      if (source_index >= OBJ_HEADER_SIZE) {

         /* if we've finished the root, quit */

         if (source_work_hdr == source_root)
            break;

         source_height++;
         source_index = source_work_hdr->o_ntype.o_intern.o_child_index + 1;
         source_work_hdr = source_work_hdr->o_ntype.o_intern.o_parent;
         target_work_hdr = target_work_hdr->o_ntype.o_intern.o_parent;

         continue;

      }

      /* if we can't move down, continue */

      if (source_work_hdr->o_child[source_index].o_header == NULL) {

         target_work_hdr->o_child[source_index].o_header = NULL;
         source_index++;

         continue;

      }

      /* we can move down, so do so */

      source_work_hdr = source_work_hdr->o_child[source_index].o_header;
      get_object_header(new_hdr);
      target_work_hdr->o_child[source_index].o_header = new_hdr;
      new_hdr->o_ntype.o_intern.o_parent = target_work_hdr;
      new_hdr->o_ntype.o_intern.o_child_index = source_index;

      for (i = 0;
           i < OBJ_HEADER_SIZE;
           new_hdr->o_child[i++].o_header = NULL);

      target_work_hdr = new_hdr;

      source_index = 0;
      source_height--;

   }

   /* that's it, return the root */

   return target_root;

}

/*\
 *  \function{alloc\_self\_stack()}
 *
 *  This function allocates a block of self records and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste header items to pointers to records in order to form the free
 *  list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the header node.
 *  It will work provided a header item is larger than a pointer, which
 *  is the case.
\*/

void alloc_self_stack(
   SETL_SYSTEM_PROTO_VOID)

{
self_stack_ptr_type new_block;         /* first header in allocated block   */

   /* allocate a new block */

   new_block = (self_stack_ptr_type)malloc((size_t)
         (SELF_STACK_BLOCK_SIZE * sizeof(struct self_stack_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (SELF_STACK_NEXT_FREE = new_block;
        SELF_STACK_NEXT_FREE < new_block + SELF_STACK_BLOCK_SIZE - 1;
        SELF_STACK_NEXT_FREE++) {

      *((self_stack_ptr_type *)SELF_STACK_NEXT_FREE) =
         SELF_STACK_NEXT_FREE + 1;

   }

   *((self_stack_ptr_type *)SELF_STACK_NEXT_FREE) = NULL;

   /* set next free node to new block */

   SELF_STACK_NEXT_FREE = new_block;

   return;

}

