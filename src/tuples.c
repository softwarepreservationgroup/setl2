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
 *  \package{Tuples}
 *
 *  This package contains definitions of the structures used to implement
 *  SETL2 tuples, and several low level functions to manipulate those
 *  structures.  We freely confess that we used a very ugly, non-portable
 *  C coding style here, in hopes of getting a fast implementation.  We
 *  have tried to isolate this ugliness to the macros and functions which
 *  allocate and release nodes.  In particular, there are some unsafe
 *  castes, so please make sure this file is compiled with unsafe
 *  optimizations disabled!
 *
 *  \texify{tuples.h}
 *
 *  \packagebody{Tuples}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "tuples.h"                    /* tuples                            */
#include "pcode.h"                     /* pseudo code                       */
#include "execute.h"                   /* core interpreter                  */

/* performance tuning constants */

#define TUPLE_HEADER_BLOCK_SIZE 100    /* tuple header block size           */
#define TUPLE_CELL_BLOCK_SIZE   400    /* tuple cell block size             */

/*\
 *  \function{alloc\_tuple\_headers()}
 *
 *  This function allocates a block of tuple headers and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste header items to pointers to header items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the header node.
 *  It will work provided a header item is larger than a pointer, which
 *  is the case.
\*/

void alloc_tuple_headers(
   SETL_SYSTEM_PROTO_VOID)

{
tuple_h_ptr_type new_block;            /* first header in allocated block   */

   /* allocate a new block */

   new_block = (tuple_h_ptr_type)malloc((size_t)
         (TUPLE_HEADER_BLOCK_SIZE * sizeof(struct tuple_h_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (TUPLE_H_NEXT_FREE = new_block;
        TUPLE_H_NEXT_FREE < new_block + TUPLE_HEADER_BLOCK_SIZE - 1;
        TUPLE_H_NEXT_FREE++) {

      *((tuple_h_ptr_type *)TUPLE_H_NEXT_FREE) = TUPLE_H_NEXT_FREE + 1;

   }

   *((tuple_h_ptr_type *)TUPLE_H_NEXT_FREE) = NULL;

   /* set next free node to new block */

   TUPLE_H_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{alloc\_tuple\_cells()}
 *
 *  This function allocates a block of tuple cells and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste cell items to pointers to cell items in order to form the free
 *  list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently, while
 *  at the same time avoiding an extra pointer on the cell node.  It
 *  will work provided a cell item is larger than a pointer, which is
 *  the case.
\*/

void alloc_tuple_cells(
   SETL_SYSTEM_PROTO_VOID)
{
tuple_c_ptr_type new_block;            /* first cell in allocated block     */

   /* allocate a new block */

   new_block = (tuple_c_ptr_type)malloc((size_t)
         (TUPLE_CELL_BLOCK_SIZE * sizeof(struct tuple_c_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (TUPLE_C_NEXT_FREE = new_block;
        TUPLE_C_NEXT_FREE < new_block + TUPLE_CELL_BLOCK_SIZE - 1;
        TUPLE_C_NEXT_FREE++) {

      *((tuple_c_ptr_type *)TUPLE_C_NEXT_FREE) = TUPLE_C_NEXT_FREE + 1;

   }

   *((tuple_c_ptr_type *)TUPLE_C_NEXT_FREE) = NULL;

   /* set next free node to new block */

   TUPLE_C_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{new\_tuple()}
 *
 *  This function returns a new tuple structure.
\*/

tuple_h_ptr_type new_tuple(
   SETL_SYSTEM_PROTO_VOID)
{
tuple_h_ptr_type tuple_root;           /* header pointer                    */
int i;                                 /* temporary looping variable        */

   /* allocate a new root header node */

   get_tuple_header(tuple_root);

   tuple_root->t_use_count = 1;
   tuple_root->t_hash_code = 0;
   tuple_root->t_ntype.t_root.t_length = 0;
   tuple_root->t_ntype.t_root.t_height = 0;

   for (i = 0;
        i < TUP_HEADER_SIZE;
        tuple_root->t_child[i++].t_header = NULL);

   return tuple_root;
}


/*\
 *  \function{copy\_tuple()}
 *
 *  This function copies an entire tuple structure.
\*/

tuple_h_ptr_type copy_tuple(
   SETL_SYSTEM_PROTO
   tuple_h_ptr_type source_root)       /* tuple to be copied                */

{
tuple_h_ptr_type source_work_hdr;      /* used to loop over source internal */
                                       /* nodes                             */
int source_height, source_index;       /* current height and index          */
tuple_c_ptr_type source_cell;          /* cell pointer                      */
tuple_h_ptr_type target_root, target_work_hdr;
                                       /* header pointers                   */
tuple_c_ptr_type target_cell;          /* cell pointer                      */
tuple_h_ptr_type new_hdr;              /* created header node               */
int i;                                 /* temporary looping variable        */
#ifdef DEBUG
#ifdef HAVE_GETRUSAGE
struct timeval start;                   /* structure to fill                 */
struct timeval stop;                    /* structure to fill                 */
struct timezone tz;
#endif

   if (PROF_DEBUG) {
#ifdef HAVE_GETRUSAGE
   gettimeofday(&start,&tz);
#endif
      if (profi!=NULL) profi->copies++;
      copy_operations[OPCODE_EXECUTED]++;
   }
   if ((TRACING_ON)&&(TRACE_COPIES))
      fprintf(DEBUG_FILE,"*COPY_TUPLE*\n");
#endif

   /* allocate a new root header node */

   get_tuple_header(target_root);
   target_root->t_use_count = 1;
   target_root->t_hash_code =
         source_root->t_hash_code;
   target_root->t_ntype.t_root.t_length =
         source_root->t_ntype.t_root.t_length;
   target_root->t_ntype.t_root.t_height =
         source_root->t_ntype.t_root.t_height;

   for (i = 0;
        i < TUP_HEADER_SIZE;
        target_root->t_child[i++].t_header = NULL);

   /* we start iterating from the root, at the left of the header table */

   source_height = source_root->t_ntype.t_root.t_height;
   source_work_hdr = source_root;
   target_work_hdr = target_root;
   source_index = 0;

   /* copy nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, copy all the tuple elements */

      if (!source_height) {

         for (source_index = 0;
              source_index < TUP_HEADER_SIZE;
              source_index++) {

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            if (source_cell != NULL) {

               get_tuple_cell(target_cell);
               target_cell->t_spec.sp_form = source_cell->t_spec.sp_form;
               target_cell->t_spec.sp_val.sp_biggest =
                     source_cell->t_spec.sp_val.sp_biggest;
               target_cell->t_hash_code = source_cell->t_hash_code;
               mark_specifier(&(target_cell->t_spec));
               target_work_hdr->t_child[source_index].t_cell = target_cell;

            }
            else {

               target_work_hdr->t_child[source_index].t_cell = NULL;

            }
         }
      }

      /* if we've finished an internal node, move up */

      if (source_index >= TUP_HEADER_SIZE) {

         /* if we've finished the root, quit */

         if (source_work_hdr == source_root)
            break;

         source_height++;
         source_index = source_work_hdr->t_ntype.t_intern.t_child_index + 1;
         source_work_hdr = source_work_hdr->t_ntype.t_intern.t_parent;
         target_work_hdr = target_work_hdr->t_ntype.t_intern.t_parent;

         continue;

      }

      /* if we can't move down, continue */

      if (source_work_hdr->t_child[source_index].t_header == NULL) {

         target_work_hdr->t_child[source_index].t_header = NULL;
         source_index++;

         continue;

      }

      /* we can move down, so do so */

      source_work_hdr = source_work_hdr->t_child[source_index].t_header;
      get_tuple_header(new_hdr);
      target_work_hdr->t_child[source_index].t_header = new_hdr;
      new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
      new_hdr->t_ntype.t_intern.t_child_index = source_index;

      for (i = 0;
           i < TUP_HEADER_SIZE;
           new_hdr->t_child[i++].t_header = NULL);

      target_work_hdr = new_hdr;

      source_index = 0;
      source_height--;

   }

   /* that's it, return the root */

#ifdef DEBUG
#ifdef HAVE_GETRUSAGE
   if (profi!=NULL) {
      gettimeofday(&stop,&tz);
      profi->timec.tv_sec +=(stop.tv_sec-start.tv_sec);
      profi->timec.tv_usec+=(stop.tv_usec-start.tv_usec);
      while (profi->timec.tv_usec<0) {
         profi->timec.tv_sec--;
         profi->timec.tv_usec+=1000000;
      }
      while (profi->timec.tv_usec>=1000000) {
         profi->timec.tv_sec++;
         profi->timec.tv_usec-=1000000;
      }
   } 
#endif
#endif
   return target_root;

}

/*\
 *  \function{tuple\_concat()}
 *
 *  This function concatenates two tuples.
\*/

void tuple_concat(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */
int32 source_hash_code;                /* hash code of source element       */
int32 base_number;                     /* initial target length             */
tuple_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
int target_height, target_index;       /* current height and index          */
int32 target_number;                   /* target element number             */
tuple_h_ptr_type new_hdr;              /* created header record pointer     */
tuple_c_ptr_type new_cell;             /* created cell record pointer       */
int32 work_number;                     /* working index (destroyed)         */
int32 expansion_trigger;               /* tuple size which triggers header  */
                                       /* expansion                         */
int i;                                 /* temporary looping variable        */

   /*
    *  First we set up source and target tuples.  We would like to use
    *  the left tuple destructively if possible.
    */

   if (target == left && target != right &&
       (target->sp_val.sp_tuple_ptr)->t_use_count == 1) {

      target_root = target->sp_val.sp_tuple_ptr;
      target->sp_form = ft_omega;

   }
   else {

      target_root = copy_tuple(SETL_SYSTEM left->sp_val.sp_tuple_ptr);

   }

   /* set the source root */

   source_root = right->sp_val.sp_tuple_ptr;

   /* set the initial target length */

   base_number = target_root->t_ntype.t_root.t_length;
   target_root->t_ntype.t_root.t_length +=
         source_root->t_ntype.t_root.t_length;

   /* expand the target header if necessary */

   expansion_trigger = 1L << ((target_root->t_ntype.t_root.t_height + 1)
                              * TUP_SHIFT_DIST);

   while (target_root->t_ntype.t_root.t_length >= expansion_trigger) {

      target_work_hdr = target_root;

      get_tuple_header(target_root);

      target_root->t_use_count = 1;
      target_root->t_hash_code =
            target_work_hdr->t_hash_code;
      target_root->t_ntype.t_root.t_length =
            target_work_hdr->t_ntype.t_root.t_length;
      target_root->t_ntype.t_root.t_height =
            target_work_hdr->t_ntype.t_root.t_height + 1;

      for (i = 1;
           i < TUP_HEADER_SIZE;
           target_root->t_child[i++].t_header = NULL);

      target_root->t_child[0].t_header = target_work_hdr;

      target_work_hdr->t_ntype.t_intern.t_parent = target_root;
      target_work_hdr->t_ntype.t_intern.t_child_index = 0;

      expansion_trigger *= TUP_HEADER_SIZE;

   }

   /* set up to iterate over the source */

   source_work_hdr = source_root;
   source_number = -1;
   source_height = source_root->t_ntype.t_root.t_height;
   source_index = 0;

   /* loop over the elements of source */

   while (source_number < source_root->t_ntype.t_root.t_length) {

      /* find the next element in the tuple */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, return it */

         if (!source_height && source_index < TUP_HEADER_SIZE) {

            if (source_work_hdr->t_child[source_index].t_cell == NULL) {

               source_number++;
               source_index++;

               continue;

            }

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            source_element = &(source_cell->t_spec);
            source_hash_code = source_cell->t_hash_code;

            source_number++;
            source_index++;

            break;

         }

         /* the current header node is exhausted -- find the next one */

         /* move up if we're at the end of a node */

         if (source_index >= TUP_HEADER_SIZE) {

            /* break if we've exhausted the source */

            if (source_work_hdr == source_root)
               break;

            source_height++;
            source_index =
               source_work_hdr->t_ntype.t_intern.t_child_index + 1;
            source_work_hdr =
               source_work_hdr->t_ntype.t_intern.t_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->t_child[source_index].t_header == NULL) {

            source_number += 1L << (source_height * TUP_SHIFT_DIST);
            source_index++;

            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element of the source in
       *  source_element.  We insert it into the target.
       */

      target_number = base_number + source_number;

      /* update the target hash code */

      target_root->t_hash_code ^= source_hash_code;

      /* we don't insert omegas */

      if (source_element->sp_form == ft_omega)
         continue;

      /* descend the header tree until we get to a leaf */

      work_number = target_number;
      target_work_hdr = target_root;
      for (target_height = target_root->t_ntype.t_root.t_height;
           target_height;
           target_height--) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (target_work_hdr->t_child[target_index].t_header == NULL) {

            get_tuple_header(new_hdr);
            new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
            new_hdr->t_ntype.t_intern.t_child_index = target_index;

            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_hdr->t_child[i++].t_cell = NULL);

            target_work_hdr->t_child[target_index].t_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->t_child[target_index].t_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  We insert the new element in the appropriate slot.
       */

      mark_specifier(source_element);
      get_tuple_cell(new_cell);
      new_cell->t_spec.sp_form = source_element->sp_form;
      new_cell->t_spec.sp_val.sp_biggest =
         source_element->sp_val.sp_biggest;
      new_cell->t_hash_code = source_hash_code;
      target_index = target_number & TUP_SHIFT_MASK;
      target_work_hdr->t_child[target_index].t_cell = new_cell;

   }

   /* finally, we set the target value */

   unmark_specifier(target);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = target_root;

   return;

}

/*\
 *  \function{tuple\_multiply()}
 *
 *  This function concatenates copies of a tuple.
\*/

void tuple_multiply(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *left,                    /* left operand                      */
   int32 copies)                       /* integer copies                    */

{
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */
int32 source_hash_code;                /* hash code of source element       */
int32 base_number;                     /* initial target length             */
tuple_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
int target_height, target_index;       /* current height and index          */
int32 target_number;                   /* target element number             */
tuple_h_ptr_type new_hdr;              /* created header record pointer     */
tuple_c_ptr_type new_cell;             /* created cell record pointer       */
int32 work_number;                     /* working index (destroyed)         */
int32 expansion_trigger;               /* tuple size which triggers header  */
                                       /* expansion                         */
int i;                                 /* temporary looping variable        */

   /* set the source root */

   source_root = left->sp_val.sp_tuple_ptr;

   /* we start with a null target tuple */

   get_tuple_header(target_root);
   target_root->t_use_count = 1;
   target_root->t_hash_code = 0;
   target_root->t_ntype.t_root.t_length = 0;
   target_root->t_ntype.t_root.t_height = 0;

   for (i = 0;
        i < TUP_HEADER_SIZE;
        target_root->t_child[i++].t_cell = NULL);

   while (copies--) {

      /* set the initial target length */

      base_number = target_root->t_ntype.t_root.t_length;
      target_root->t_ntype.t_root.t_length +=
            source_root->t_ntype.t_root.t_length;

      /* expand the target header if necessary */

      expansion_trigger = 1L << ((target_root->t_ntype.t_root.t_height + 1)
                                 * TUP_SHIFT_DIST);

      while (target_root->t_ntype.t_root.t_length >= expansion_trigger) {

         target_work_hdr = target_root;

         get_tuple_header(target_root);

         target_root->t_use_count = 1;
         target_root->t_hash_code =
               target_work_hdr->t_hash_code;
         target_root->t_ntype.t_root.t_length =
               target_work_hdr->t_ntype.t_root.t_length;
         target_root->t_ntype.t_root.t_height =
               target_work_hdr->t_ntype.t_root.t_height + 1;

         for (i = 1;
              i < TUP_HEADER_SIZE;
              target_root->t_child[i++].t_header = NULL);

         target_root->t_child[0].t_header = target_work_hdr;

         target_work_hdr->t_ntype.t_intern.t_parent = target_root;
         target_work_hdr->t_ntype.t_intern.t_child_index = 0;

         expansion_trigger *= TUP_HEADER_SIZE;

      }

      /* set up to iterate over the source */

      source_work_hdr = source_root;
      source_number = -1;
      source_height = source_root->t_ntype.t_root.t_height;
      source_index = 0;

      /* loop over the elements of source */

      while (source_number < source_root->t_ntype.t_root.t_length) {

         /* find the next element in the tuple */

         source_element = NULL;
         for (;;) {

            /* if we have an element already, return it */

            if (!source_height && source_index < TUP_HEADER_SIZE) {

               if (source_work_hdr->t_child[source_index].t_cell == NULL) {

                  source_number++;
                  source_index++;

                  continue;

               }

               source_cell = source_work_hdr->t_child[source_index].t_cell;
               source_element = &(source_cell->t_spec);
               source_hash_code = source_cell->t_hash_code;

               source_number++;
               source_index++;

               break;

            }

            /* the current header node is exhausted -- find the next one */

            /* move up if we're at the end of a node */

            if (source_index >= TUP_HEADER_SIZE) {

               /* break if we've exhausted the source */

               if (source_work_hdr == source_root)
                  break;

               source_height++;
               source_index =
                  source_work_hdr->t_ntype.t_intern.t_child_index + 1;
               source_work_hdr =
                  source_work_hdr->t_ntype.t_intern.t_parent;

               continue;

            }

            /* skip over null nodes */

            if (source_work_hdr->t_child[source_index].t_header == NULL) {

               source_number += 1L << (source_height * TUP_SHIFT_DIST);
               source_index++;

               continue;

            }

            /* otherwise drop down a level */

            source_work_hdr =
               source_work_hdr->t_child[source_index].t_header;
            source_index = 0;
            source_height--;

         }

         if (source_element == NULL)
            break;

         /*
          *  At this point we have an element of the source in
          *  source_element.  We insert it into the target.
          */

         target_number = base_number + source_number;

         /* update the target hash code */

         target_root->t_hash_code ^= source_hash_code;

         /* we don't insert omegas */

         if (source_element->sp_form == ft_omega)
            continue;

         /* descend the header tree until we get to a leaf */

         work_number = target_number;
         target_work_hdr = target_root;
         for (target_height = target_root->t_ntype.t_root.t_height;
              target_height;
              target_height--) {

            /* extract the element's index at this level */

            target_index = (target_number >>
                                 (target_height * TUP_SHIFT_DIST)) &
                              TUP_SHIFT_MASK;

            /* if we're missing a header record, allocate one */

            if (target_work_hdr->t_child[target_index].t_header == NULL) {

               get_tuple_header(new_hdr);
               new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
               new_hdr->t_ntype.t_intern.t_child_index = target_index;
               for (i = 0;
                    i < TUP_HEADER_SIZE;
                    new_hdr->t_child[i++].t_cell = NULL);
               target_work_hdr->t_child[target_index].t_header = new_hdr;
               target_work_hdr = new_hdr;

            }
            else {

               target_work_hdr =
                  target_work_hdr->t_child[target_index].t_header;

            }
         }

         /*
          *  At this point, target_work_hdr points to the lowest level
          *  header record.  We insert the new element in the appropriate
          *  slot.
          */

         mark_specifier(source_element);
         get_tuple_cell(new_cell);
         new_cell->t_spec.sp_form = source_element->sp_form;
         new_cell->t_spec.sp_val.sp_biggest =
            source_element->sp_val.sp_biggest;
         new_cell->t_hash_code = source_hash_code;
         target_index = target_number & TUP_SHIFT_MASK;
         target_work_hdr->t_child[target_index].t_cell = new_cell;

      }
   }

   /* finally, we set the target value */

   unmark_specifier(target);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = target_root;

   return;

}

/*\
 *  \function{tuple\_slice()}
 *
 *  This function produces a {\em slice} of a tuple.  We start by
 *  creating a new tuple, then append the appropriate elements from
 *  the source tuple.
\*/

void tuple_slice(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *source,                  /* left operand                      */
   int32 start_index,                  /* start index of slice              */
   int32 end_index)                    /* end index of slice                */

{
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */
int32 source_hash_code;                /* hash code of source element       */
int32 base_number;                     /* initial target length             */
tuple_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
int target_height, target_index;       /* current height and index          */
int32 target_number;                   /* target element number             */
tuple_h_ptr_type new_hdr;              /* created header record pointer     */
tuple_c_ptr_type new_cell;             /* created cell record pointer       */
int32 expansion_trigger;               /* tuple size which triggers header  */
                                       /* expansion                         */
int i;                                 /* temporary looping variable        */

   /* our indices are zero-based */

   start_index--;
   end_index--;

   /* pick out the source root */

   source_root = source->sp_val.sp_tuple_ptr;

   /* we start with a null target tuple */

   get_tuple_header(target_root);
   target_root->t_use_count = 1;
   target_root->t_hash_code = 0;
   target_root->t_ntype.t_root.t_length = 0;
   target_root->t_ntype.t_root.t_height = 0;

   for (i = 0;
        i < TUP_HEADER_SIZE;
        target_root->t_child[i++].t_cell = NULL);

   /* set the initial target length */

   base_number = target_root->t_ntype.t_root.t_length;
   target_root->t_ntype.t_root.t_length += (end_index - start_index + 1);

   /* expand the target header if necessary */

   expansion_trigger = 1L << ((target_root->t_ntype.t_root.t_height + 1)
                              * TUP_SHIFT_DIST);

   while (target_root->t_ntype.t_root.t_length > expansion_trigger) {

      target_work_hdr = target_root;

      get_tuple_header(target_root);

      target_root->t_use_count = 1;
      target_root->t_hash_code =
            target_work_hdr->t_hash_code;
      target_root->t_ntype.t_root.t_length =
            target_work_hdr->t_ntype.t_root.t_length;
      target_root->t_ntype.t_root.t_height =
            target_work_hdr->t_ntype.t_root.t_height + 1;

      for (i = 1;
           i < TUP_HEADER_SIZE;
           target_root->t_child[i++].t_cell = NULL);

      target_root->t_child[0].t_header = target_work_hdr;

      target_work_hdr->t_ntype.t_intern.t_parent = target_root;
      target_work_hdr->t_ntype.t_intern.t_child_index = 0;

      expansion_trigger *= TUP_HEADER_SIZE;

   }

   /* try to go down to the appropriate leaf */

   source_number = start_index;
   source_work_hdr = source_root;
   for (source_height = source_root->t_ntype.t_root.t_height;
        source_height;
        source_height--) {

      /* extract the element's index at this level */

      source_index = (source_number >>
                           (source_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

      /* if we're missing a header record, break */

      if (source_work_hdr->t_child[source_index].t_header == NULL)
         break;

      /* otherwise move down */

      source_work_hdr = source_work_hdr->t_child[source_index].t_header;

   }

   /* if we're not at a leaf, fix up the source number */

   if (source_height > 0) {

      source_number = (source_number |
                         ((1L << (source_height * TUP_SHIFT_DIST)) - 1))
                       ^ ((1L << (source_height * TUP_SHIFT_DIST)) - 1);

   }
   else {

       source_index = source_number & TUP_SHIFT_MASK;

   }

   /* loop over the elements of source */

   source_number--;
   while (source_number < end_index) {

      /* find the next element in the tuple */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, return it */

         if (!source_height && source_index < TUP_HEADER_SIZE) {

            if (source_work_hdr->t_child[source_index].t_cell == NULL) {

               source_number++;
               source_index++;

               continue;

            }

            /* we do have an element from source */

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            source_element = &(source_cell->t_spec);
            source_hash_code = source_cell->t_hash_code;

            source_number++;
            source_index++;

            break;

         }

         /* move up if we're at the end of a node */

         if (source_index >= TUP_HEADER_SIZE) {

            /* break if we've exhausted the source */

            if (source_work_hdr == source_root)
               break;

            source_height++;
            source_index =
               source_work_hdr->t_ntype.t_intern.t_child_index + 1;
            source_work_hdr =
               source_work_hdr->t_ntype.t_intern.t_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->t_child[source_index].t_header == NULL) {

            source_number += 1L << (source_height * TUP_SHIFT_DIST);
            source_index++;

            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr = source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      if (source_element == NULL || source_number > end_index)
         break;

      /*
       *  At this point we have an element of the source in
       *  source_element.  We insert it into the target.
       */

      target_number = base_number + source_number - start_index;

      /* we don't insert omegas */

      if (source_element->sp_form == ft_omega)
         continue;

      /* update the target hash code */

      target_root->t_hash_code ^= source_hash_code;

      /* descend the header tree until we get to a leaf */

      target_work_hdr = target_root;
      for (target_height = target_root->t_ntype.t_root.t_height;
           target_height;
           target_height--) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (target_work_hdr->t_child[target_index].t_header == NULL) {

            get_tuple_header(new_hdr);
            new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
            new_hdr->t_ntype.t_intern.t_child_index = target_index;

            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_hdr->t_child[i++].t_cell = NULL);

            target_work_hdr->t_child[target_index].t_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->t_child[target_index].t_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  We insert the new element in the appropriate slot.
       */

      mark_specifier(source_element);
      get_tuple_cell(new_cell);
      new_cell->t_spec.sp_form = source_element->sp_form;
      new_cell->t_spec.sp_val.sp_biggest =
         source_element->sp_val.sp_biggest;
      new_cell->t_hash_code = source_hash_code;
      target_index = target_number & TUP_SHIFT_MASK;
      target_work_hdr->t_child[target_index].t_cell = new_cell;

   }

   /*
    *  Since we've concatenated a slice, not an entire tuple, there is no
    *  guarantee that the rightmost cells are not omega.  We insist that
    *  the rightmost cells of a finished tuple not be omega, so we
    *  strip off right-most omegas.
    */

   /* if the length is zero, don't try this */

   if (target_root->t_ntype.t_root.t_length == 0) {

      unmark_specifier(target);
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = target_root;

      return;

   }

   /* drop to a leaf at the rightmost position */

   target_number = target_root->t_ntype.t_root.t_length - 1;
   target_work_hdr = target_root;
   for (target_height = target_root->t_ntype.t_root.t_height;
        target_height;
        target_height--) {

      /* extract the element's index at this level */

      target_index = (target_number >>
                           (target_height * TUP_SHIFT_DIST)) &
                        TUP_SHIFT_MASK;

      /* if we're missing a header record, allocate one */

      if (target_work_hdr->t_child[target_index].t_header == NULL) {

         get_tuple_header(new_hdr);
         new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
         new_hdr->t_ntype.t_intern.t_child_index = target_index;

         for (i = 0;
              i < TUP_HEADER_SIZE;
              new_hdr->t_child[i++].t_cell = NULL);

         target_work_hdr->t_child[target_index].t_header = new_hdr;
         target_work_hdr = new_hdr;

      }
      else {

         target_work_hdr =
            target_work_hdr->t_child[target_index].t_header;

      }
   }

   /* set the target index to the last element */

   target_index = target_number & TUP_SHIFT_MASK;

   /* keep stripping omegas */

   for (;;) {

      if (!target_height && target_index >= 0) {

         if (target_work_hdr->t_child[target_index].t_cell != NULL)
            break;

         target_root->t_ntype.t_root.t_length--;
         target_index--;

         continue;

      }

      /* move up if we're at the end of a node */

      if (target_index < 0) {

         if (target_work_hdr == target_root)
            break;

         target_height++;
         target_index = target_work_hdr->t_ntype.t_intern.t_child_index;
         target_work_hdr = target_work_hdr->t_ntype.t_intern.t_parent;
         free_tuple_header(target_work_hdr->t_child[target_index].t_header);
         target_work_hdr->t_child[target_index].t_header = NULL;
         target_index--;

         continue;

      }

      /* skip over null nodes */

      if (target_work_hdr->t_child[target_index].t_header == NULL) {

         target_root->t_ntype.t_root.t_length -=
               1L << (target_height * TUP_SHIFT_DIST);
         target_index--;

         continue;

      }

      /* otherwise drop down a level */

      target_work_hdr =
         target_work_hdr->t_child[target_index].t_header;
      target_index = TUP_HEADER_SIZE - 1;

      target_height--;

   }

   /* we've shortened the tuple -- now reduce the height */

   while (target_root->t_ntype.t_root.t_height > 0 &&
          target_root->t_ntype.t_root.t_length <=
             (int32)(1L << (target_root->t_ntype.t_root.t_height *
                           TUP_SHIFT_DIST))) {

      target_work_hdr = target_root->t_child[0].t_header;

      /* it's possible that we deleted internal headers */

      if (target_work_hdr == NULL) {

         target_root->t_ntype.t_root.t_height--;
         continue;

      }

      /* delete the root node */

      target_work_hdr->t_use_count = target_root->t_use_count;
      target_work_hdr->t_hash_code =
            target_root->t_hash_code;
      target_work_hdr->t_ntype.t_root.t_length =
            target_root->t_ntype.t_root.t_length;
      target_work_hdr->t_ntype.t_root.t_height =
            target_root->t_ntype.t_root.t_height - 1;

      free_tuple_header(target_root);
      target_root = target_work_hdr;

   }

   /* finally, we set the target value */

   unmark_specifier(target);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = target_root;

   return;

}

/*\
 *  \function{tuple\_sslice()}
 *
 *  This function assignes a tuple to a slice of a tuple.  We start by
 *  creating a new tuple, then appending slices from the target and
 *  source.
\*/

void tuple_sslice(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *source,                  /* left operand                      */
   int32 start_index,                  /* start index of slice              */
   int32 end_index)                    /* end index of slice                */

{
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */
int32 source_hash_code;                /* hash code of source element       */
int32 base_number;                     /* initial target length             */
tuple_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
int target_height, target_index;       /* current height and index          */
int32 target_number;                   /* target element number             */
tuple_h_ptr_type new_hdr;              /* created header record pointer     */
tuple_c_ptr_type new_cell;             /* created cell record pointer       */
int32 expansion_trigger;               /* tuple size which triggers header  */
                                       /* expansion                         */
int i;                                 /* temporary looping variable        */

   /* our indices are zero-based */

   start_index--;
   end_index--;

   /* we start with a null target tuple */

   get_tuple_header(target_root);
   target_root->t_use_count = 1;
   target_root->t_hash_code = 0;
   target_root->t_ntype.t_root.t_length = 0;
   target_root->t_ntype.t_root.t_height = 0;

   for (i = 0;
        i < TUP_HEADER_SIZE;
        target_root->t_child[i++].t_cell = NULL);

   /*
    *  We first copy the target up to the start of the slice.
    */

   /* pick out the source root */

   source_root = target->sp_val.sp_tuple_ptr;

   /* set the initial target length */

   base_number = target_root->t_ntype.t_root.t_length;
   target_root->t_ntype.t_root.t_length += start_index;

   /* expand the target header if necessary */

   expansion_trigger = 1L << ((target_root->t_ntype.t_root.t_height + 1)
                              * TUP_SHIFT_DIST);

   while (target_root->t_ntype.t_root.t_length > expansion_trigger) {

      target_work_hdr = target_root;

      get_tuple_header(target_root);

      target_root->t_use_count = 1;
      target_root->t_hash_code =
            target_work_hdr->t_hash_code;
      target_root->t_ntype.t_root.t_length =
            target_work_hdr->t_ntype.t_root.t_length;
      target_root->t_ntype.t_root.t_height =
            target_work_hdr->t_ntype.t_root.t_height + 1;

      for (i = 1;
           i < TUP_HEADER_SIZE;
           target_root->t_child[i++].t_cell = NULL);

      target_root->t_child[0].t_header = target_work_hdr;

      target_work_hdr->t_ntype.t_intern.t_parent = target_root;
      target_work_hdr->t_ntype.t_intern.t_child_index = 0;

      expansion_trigger *= TUP_HEADER_SIZE;

   }

   /* loop over the elements of source */

   source_work_hdr = source_root;
   source_number = -1;
   source_height = source_root->t_ntype.t_root.t_height;
   source_index = 0;

   while (source_number < start_index - 1) {

      /* find the next element in the tuple */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, return it */

         if (!source_height && source_index < TUP_HEADER_SIZE) {

            if (source_work_hdr->t_child[source_index].t_cell == NULL) {

               source_number++;
               source_index++;

               continue;

            }

            /* we do have an element from source */

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            source_element = &(source_cell->t_spec);
            source_hash_code = source_cell->t_hash_code;

            source_number++;
            source_index++;

            break;

         }

         /* move up if we're at the end of a node */

         if (source_index >= TUP_HEADER_SIZE) {

            /* break if we've exhausted the source */

            if (source_work_hdr == source_root)
               break;

            source_height++;
            source_index =
               source_work_hdr->t_ntype.t_intern.t_child_index + 1;
            source_work_hdr =
               source_work_hdr->t_ntype.t_intern.t_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->t_child[source_index].t_header == NULL) {

            source_number += 1L << (source_height * TUP_SHIFT_DIST);
            source_index++;

            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr = source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      if (source_element == NULL || source_number >= start_index)
         break;

      /*
       *  At this point we have an element of the source in
       *  source_element.  We insert it into the target.
       */

      target_number = base_number + source_number;

      /* we don't insert omegas */

      if (source_element->sp_form == ft_omega)
         continue;

      /* update the target hash code */

      target_root->t_hash_code ^= source_hash_code;

      /* descend the header tree until we get to a leaf */

      target_work_hdr = target_root;
      for (target_height = target_root->t_ntype.t_root.t_height;
           target_height;
           target_height--) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (target_work_hdr->t_child[target_index].t_header == NULL) {

            get_tuple_header(new_hdr);
            new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
            new_hdr->t_ntype.t_intern.t_child_index = target_index;

            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_hdr->t_child[i++].t_cell = NULL);

            target_work_hdr->t_child[target_index].t_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->t_child[target_index].t_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  We insert the new element in the appropriate slot.
       */

      mark_specifier(source_element);
      get_tuple_cell(new_cell);
      new_cell->t_spec.sp_form = source_element->sp_form;
      new_cell->t_spec.sp_val.sp_biggest =
         source_element->sp_val.sp_biggest;
      new_cell->t_hash_code = source_hash_code;
      target_index = target_number & TUP_SHIFT_MASK;
      target_work_hdr->t_child[target_index].t_cell = new_cell;

   }

   /*
    *  Now we copy the entire source.
    */

   /* pick out the source root */

   source_root = source->sp_val.sp_tuple_ptr;

   /* set the initial target length */

   base_number = target_root->t_ntype.t_root.t_length;
   target_root->t_ntype.t_root.t_length +=
         source_root->t_ntype.t_root.t_length;

   /* expand the target header if necessary */

   expansion_trigger = 1L << ((target_root->t_ntype.t_root.t_height + 1)
                              * TUP_SHIFT_DIST);

   while (target_root->t_ntype.t_root.t_length > expansion_trigger) {

      target_work_hdr = target_root;

      get_tuple_header(target_root);

      target_root->t_use_count = 1;
      target_root->t_hash_code =
            target_work_hdr->t_hash_code;
      target_root->t_ntype.t_root.t_length =
            target_work_hdr->t_ntype.t_root.t_length;
      target_root->t_ntype.t_root.t_height =
            target_work_hdr->t_ntype.t_root.t_height + 1;

      for (i = 1;
           i < TUP_HEADER_SIZE;
           target_root->t_child[i++].t_cell = NULL);

      target_root->t_child[0].t_header = target_work_hdr;

      target_work_hdr->t_ntype.t_intern.t_parent = target_root;
      target_work_hdr->t_ntype.t_intern.t_child_index = 0;

      expansion_trigger *= TUP_HEADER_SIZE;

   }

   /* loop over the elements of source */

   source_work_hdr = source_root;
   source_number = -1;
   source_height = source_root->t_ntype.t_root.t_height;
   source_index = 0;

   for (;;) {

      /* find the next element in the tuple */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, return it */

         if (!source_height && source_index < TUP_HEADER_SIZE) {

            if (source_work_hdr->t_child[source_index].t_cell == NULL) {

               source_number++;
               source_index++;

               continue;

            }

            /* we do have an element from source */

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            source_element = &(source_cell->t_spec);
            source_hash_code = source_cell->t_hash_code;

            source_number++;
            source_index++;

            break;

         }

         /* move up if we're at the end of a node */

         if (source_index >= TUP_HEADER_SIZE) {

            /* break if we've exhausted the source */

            if (source_work_hdr == source_root)
               break;

            source_height++;
            source_index =
               source_work_hdr->t_ntype.t_intern.t_child_index + 1;
            source_work_hdr =
               source_work_hdr->t_ntype.t_intern.t_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->t_child[source_index].t_header == NULL) {

            source_number += 1L << (source_height * TUP_SHIFT_DIST);
            source_index++;

            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr = source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element of the source in
       *  source_element.  We insert it into the target.
       */

      target_number = base_number + source_number;

      /* we don't insert omegas */

      if (source_element->sp_form == ft_omega)
         continue;

      /* update the target hash code */

      target_root->t_hash_code ^= source_hash_code;

      /* descend the header tree until we get to a leaf */

      target_work_hdr = target_root;
      for (target_height = target_root->t_ntype.t_root.t_height;
           target_height;
           target_height--) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (target_work_hdr->t_child[target_index].t_header == NULL) {

            get_tuple_header(new_hdr);
            new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
            new_hdr->t_ntype.t_intern.t_child_index = target_index;

            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_hdr->t_child[i++].t_cell = NULL);

            target_work_hdr->t_child[target_index].t_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->t_child[target_index].t_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  We insert the new element in the appropriate slot.
       */

      mark_specifier(source_element);
      get_tuple_cell(new_cell);
      new_cell->t_spec.sp_form = source_element->sp_form;
      new_cell->t_spec.sp_val.sp_biggest =
         source_element->sp_val.sp_biggest;
      new_cell->t_hash_code = source_hash_code;
      target_index = target_number & TUP_SHIFT_MASK;
      target_work_hdr->t_child[target_index].t_cell = new_cell;

   }

   /*
    *  Finally, we copy the tail from the target.
    */

   /* pick out the source root */

   source_root = target->sp_val.sp_tuple_ptr;

   /* set the initial target length */

   base_number = target_root->t_ntype.t_root.t_length;
   target_root->t_ntype.t_root.t_length +=
         (source_root->t_ntype.t_root.t_length - end_index - 1);

   /* expand the target header if necessary */

   expansion_trigger = 1L << ((target_root->t_ntype.t_root.t_height + 1)
                              * TUP_SHIFT_DIST);

   while (target_root->t_ntype.t_root.t_length > expansion_trigger) {

      target_work_hdr = target_root;

      get_tuple_header(target_root);

      target_root->t_use_count = 1;
      target_root->t_hash_code =
            target_work_hdr->t_hash_code;
      target_root->t_ntype.t_root.t_length =
            target_work_hdr->t_ntype.t_root.t_length;
      target_root->t_ntype.t_root.t_height =
            target_work_hdr->t_ntype.t_root.t_height + 1;

      for (i = 1;
           i < TUP_HEADER_SIZE;
           target_root->t_child[i++].t_cell = NULL);

      target_root->t_child[0].t_header = target_work_hdr;

      target_work_hdr->t_ntype.t_intern.t_parent = target_root;
      target_work_hdr->t_ntype.t_intern.t_child_index = 0;

      expansion_trigger *= TUP_HEADER_SIZE;

   }

   /* try to go down to the appropriate leaf */

   source_number = end_index + 1;
   source_work_hdr = source_root;
   for (source_height = source_root->t_ntype.t_root.t_height;
        source_height;
        source_height--) {

      /* extract the element's index at this level */

      source_index = (source_number >>
                           (source_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

      /* if we're missing a header record, break */

      if (source_work_hdr->t_child[source_index].t_header == NULL)
         break;

      /* otherwise move down */

      source_work_hdr = source_work_hdr->t_child[source_index].t_header;

   }

   /* if we're not at a leaf, fix up the source number */

   if (source_height > 0) {

      source_number = (source_number |
                         ((1L << (source_height * TUP_SHIFT_DIST)) - 1))
                       ^ ((1L << (source_height * TUP_SHIFT_DIST)) - 1);

   }
   else {

       source_index = source_number & TUP_SHIFT_MASK;

   }

   /* loop over the elements of source */

   source_number--;
   for (;;) {

      /* find the next element in the tuple */

      source_element = NULL;
      for (; source_number < source_root->t_ntype.t_root.t_length - 1;) {

         /* if we have an element already, return it */

         if (!source_height && source_index < TUP_HEADER_SIZE) {

            if (source_work_hdr->t_child[source_index].t_cell == NULL) {

               source_number++;
               source_index++;

               continue;

            }

            /* we do have an element from source */

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            source_element = &(source_cell->t_spec);
            source_hash_code = source_cell->t_hash_code;

            source_number++;
            source_index++;

            break;

         }

         /* move up if we're at the end of a node */

         if (source_index >= TUP_HEADER_SIZE) {

            /* break if we've exhausted the source */

            if (source_work_hdr == source_root)
               break;

            source_height++;
            source_index =
               source_work_hdr->t_ntype.t_intern.t_child_index + 1;
            source_work_hdr =
               source_work_hdr->t_ntype.t_intern.t_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->t_child[source_index].t_header == NULL) {

            source_number += 1L << (source_height * TUP_SHIFT_DIST);
            source_index++;

            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr = source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element of the source in
       *  source_element.  We insert it into the target.
       */

      target_number = base_number + source_number - end_index - 1;

      /* we don't insert omegas */

      if (source_element->sp_form == ft_omega)
         continue;

      /* update the target hash code */

      target_root->t_hash_code ^= source_hash_code;

      /* descend the header tree until we get to a leaf */

      target_work_hdr = target_root;
      for (target_height = target_root->t_ntype.t_root.t_height;
           target_height;
           target_height--) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (target_work_hdr->t_child[target_index].t_header == NULL) {

            get_tuple_header(new_hdr);
            new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
            new_hdr->t_ntype.t_intern.t_child_index = target_index;

            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_hdr->t_child[i++].t_cell = NULL);

            target_work_hdr->t_child[target_index].t_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->t_child[target_index].t_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  We insert the new element in the appropriate slot.
       */

      mark_specifier(source_element);
      get_tuple_cell(new_cell);
      new_cell->t_spec.sp_form = source_element->sp_form;
      new_cell->t_spec.sp_val.sp_biggest =
         source_element->sp_val.sp_biggest;
      new_cell->t_hash_code = source_hash_code;
      target_index = target_number & TUP_SHIFT_MASK;
      target_work_hdr->t_child[target_index].t_cell = new_cell;

   }

   /*
    *  Since we've concatenated slices, not an entire tuple, there is no
    *  guarantee that the rightmost cells are not omega.  We insist that
    *  the rightmost cells of a finished tuple not be omega, so we
    *  strip off right-most omegas.
    */

   /* if the length is zero, don't try this */

   if (target_root->t_ntype.t_root.t_length == 0) {

      unmark_specifier(target);
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = target_root;

      return;

   }

   /* drop to a leaf at the rightmost position */

   target_number = target_root->t_ntype.t_root.t_length - 1;
   target_work_hdr = target_root;
   for (target_height = target_root->t_ntype.t_root.t_height;
        target_height;
        target_height--) {

      /* extract the element's index at this level */

      target_index = (target_number >>
                           (target_height * TUP_SHIFT_DIST)) &
                        TUP_SHIFT_MASK;

      /* if we're missing a header record, allocate one */

      if (target_work_hdr->t_child[target_index].t_header == NULL) {

         get_tuple_header(new_hdr);
         new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
         new_hdr->t_ntype.t_intern.t_child_index = target_index;

         for (i = 0;
              i < TUP_HEADER_SIZE;
              new_hdr->t_child[i++].t_cell = NULL);

         target_work_hdr->t_child[target_index].t_header = new_hdr;
         target_work_hdr = new_hdr;

      }
      else {

         target_work_hdr =
            target_work_hdr->t_child[target_index].t_header;

      }
   }

   /* set the target index to the last element */

   target_index = target_number & TUP_SHIFT_MASK;

   /* keep stripping omegas */

   for (;;) {

      if (!target_height && target_index >= 0) {

         if (target_work_hdr->t_child[target_index].t_cell != NULL)
            break;

         target_root->t_ntype.t_root.t_length--;
         target_index--;

         continue;

      }

      /* move up if we're at the end of a node */

      if (target_index < 0) {

         if (target_work_hdr == target_root)
            break;

         target_height++;
         target_index = target_work_hdr->t_ntype.t_intern.t_child_index;
         target_work_hdr = target_work_hdr->t_ntype.t_intern.t_parent;
         free_tuple_header(target_work_hdr->t_child[target_index].t_header);
         target_work_hdr->t_child[target_index].t_header = NULL;
         target_index--;

         continue;

      }

      /* skip over null nodes */

      if (target_work_hdr->t_child[target_index].t_header == NULL) {

         target_root->t_ntype.t_root.t_length -=
               1L << (target_height * TUP_SHIFT_DIST);
         target_index--;

         continue;

      }

      /* otherwise drop down a level */

      target_work_hdr =
         target_work_hdr->t_child[target_index].t_header;
      target_index = TUP_HEADER_SIZE - 1;

      target_height--;

   }

   /* we've shortened the tuple -- now reduce the height */

   while (target_root->t_ntype.t_root.t_height > 0 &&
          target_root->t_ntype.t_root.t_length <=
             (int32)(1L << (target_root->t_ntype.t_root.t_height *
                           TUP_SHIFT_DIST))) {

      target_work_hdr = target_root->t_child[0].t_header;

      /* it's possible that we deleted internal headers */

      if (target_work_hdr == NULL) {

         target_root->t_ntype.t_root.t_height--;
         continue;

      }

      /* delete the root node */

      target_work_hdr->t_use_count = target_root->t_use_count;
      target_work_hdr->t_hash_code =
            target_root->t_hash_code;
      target_work_hdr->t_ntype.t_root.t_length =
            target_root->t_ntype.t_root.t_length;
      target_work_hdr->t_ntype.t_root.t_height =
            target_root->t_ntype.t_root.t_height - 1;

      free_tuple_header(target_root);
      target_root = target_work_hdr;

   }

   /* finally, we set the target value */

   unmark_specifier(target);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = target_root;

   return;

}

/*\
 *  \function{tuple\_fromb()}
 *
 *  This functions implements the SETL2 \verb"FROMB" operation.  Notice
 *  that all three operands may be modified.
\*/

void tuple_fromb(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */
int32 source_hash_code;                /* hash code of source element       */
tuple_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
int target_height, target_index;       /* current height and index          */
int32 work_length;                     /* working copy of length            */
                                       /* (destroyed finding height)        */
int32 target_number;                   /* target element number             */
tuple_h_ptr_type new_hdr;              /* created header record pointer     */
tuple_c_ptr_type new_cell;             /* created cell record pointer       */
int i;                                 /* temporary looping variable        */

   /* if the source set is empty, return omega */

   source_root = right->sp_val.sp_tuple_ptr;
   if (source_root->t_ntype.t_root.t_length == 0) {

      unmark_specifier(left);
      left->sp_form = ft_omega;

      if (target != NULL) {

         unmark_specifier(target);
         target->sp_form = ft_omega;

      }

      return;

   }

   /* we start with a null target tuple */

   get_tuple_header(target_root);
   target_root->t_use_count = 1;
   target_root->t_hash_code =
         source_root->t_hash_code;
   target_root->t_ntype.t_root.t_length =
         source_root->t_ntype.t_root.t_length - 1;

   /* calculate the height of the new header tree */

   target_height = 0;
   work_length = target_root->t_ntype.t_root.t_length;
   while (work_length = work_length >> TUP_SHIFT_DIST)
      target_height++;

   target_root->t_ntype.t_root.t_height = target_height;

   for (i = 0;
        i < TUP_HEADER_SIZE;
        target_root->t_child[i++].t_cell = NULL);

   /* try to go down to the appropriate leaf */

   source_work_hdr = source_root;
   for (source_height = source_root->t_ntype.t_root.t_height;
        source_height;
        source_height--) {

      /* if we're missing a header record, break */

      if (source_work_hdr->t_child[0].t_header == NULL)
         break;

      /* otherwise move down */

      source_work_hdr = source_work_hdr->t_child[0].t_header;

   }

   /* set the extracted element */

   if (!source_height) {
      source_cell = source_work_hdr->t_child[0].t_cell;
      source_number = 0;
      source_index = 1;
   }
   else {
      source_cell = NULL;
      source_number = -1;
      source_index = 0;
   }

   if (source_cell != NULL) {

      mark_specifier(&(source_cell->t_spec));
      unmark_specifier(left);
      left->sp_form = source_cell->t_spec.sp_form;
      left->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;
      target_root->t_hash_code ^= source_cell->t_hash_code;

   }
   else {

      unmark_specifier(left);
      left->sp_form = ft_omega;

   }

   /* loop over the elements of source */

   for (;;) {

      /* find the next element in the tuple */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, return it */

         if (!source_height && source_index < TUP_HEADER_SIZE) {

            if (source_work_hdr->t_child[source_index].t_cell == NULL) {

               source_number++;
               source_index++;

               continue;

            }

            /* we do have an element from source */

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            source_element = &(source_cell->t_spec);
            source_hash_code = source_cell->t_hash_code;

            source_number++;
            source_index++;

            break;

         }

         /* move up if we're at the end of a node */

         if (source_index >= TUP_HEADER_SIZE) {

            /* break if we've exhausted the source */

            if (source_work_hdr == source_root)
               break;

            source_height++;
            source_index =
               source_work_hdr->t_ntype.t_intern.t_child_index + 1;
            source_work_hdr =
               source_work_hdr->t_ntype.t_intern.t_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->t_child[source_index].t_header == NULL) {

            source_number += 1L << (source_height * TUP_SHIFT_DIST);
            source_index++;

            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr = source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element of the source in
       *  source_element.  We insert it into the target.
       */

      target_number = source_number - 1;

      /* we don't insert omegas */

      if (source_element->sp_form == ft_omega)
         continue;

      /* descend the header tree until we get to a leaf */

      target_work_hdr = target_root;
      for (target_height = target_root->t_ntype.t_root.t_height;
           target_height;
           target_height--) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (target_work_hdr->t_child[target_index].t_header == NULL) {

            get_tuple_header(new_hdr);
            new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
            new_hdr->t_ntype.t_intern.t_child_index = target_index;

            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_hdr->t_child[i++].t_cell = NULL);

            target_work_hdr->t_child[target_index].t_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->t_child[target_index].t_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  We insert the new element in the appropriate slot.
       */

      mark_specifier(source_element);
      get_tuple_cell(new_cell);
      new_cell->t_spec.sp_form = source_element->sp_form;
      new_cell->t_spec.sp_val.sp_biggest =
         source_element->sp_val.sp_biggest;
      new_cell->t_hash_code = source_hash_code;
      target_index = target_number & TUP_SHIFT_MASK;
      target_work_hdr->t_child[target_index].t_cell = new_cell;

   }

   /* set the other target values */

   unmark_specifier(right);
   right->sp_form = ft_tuple;
   right->sp_val.sp_tuple_ptr = target_root;

   if (target != NULL) {

      mark_specifier(left);
      unmark_specifier(target);
      target->sp_form = left->sp_form;
      target->sp_val.sp_biggest = left->sp_val.sp_biggest;

   }

   return;

}

/*\
 *  \function{tuple\_frome()}
 *
 *  This functions implements the SETL2 \verb"FROME" operation.  Notice
 *  that all three operands may be modified.
\*/

void tuple_frome(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
tuple_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type target_cell;          /* current cell pointer              */
int target_height, target_index;       /* current height and index          */
int32 target_number;                   /* target element number             */
tuple_h_ptr_type new_hdr;              /* created header record pointer     */
int i;                                 /* temporary looping variable        */

   /* if the source set is empty, return omega */

   target_root = right->sp_val.sp_tuple_ptr;
   if (target_root->t_ntype.t_root.t_length == 0) {

      unmark_specifier(left);
      left->sp_form = ft_omega;

      if (target != NULL) {

         unmark_specifier(target);
         target->sp_form = ft_omega;

      }

      return;

   }

   /*
    *  We would like to use the right operand destructively if possible.
    */

   if (right == target || right == left ||
       target_root->t_use_count != 1) {

      target_root = copy_tuple(SETL_SYSTEM right->sp_val.sp_tuple_ptr);

   }
   else {

      right->sp_form = ft_omega;

   }

   /* try to go down to the appropriate leaf */

   target_number = target_root->t_ntype.t_root.t_length - 1;
   target_work_hdr = target_root;
   for (target_height = target_root->t_ntype.t_root.t_height;
        target_height;
        target_height--) {

      /* extract the element's index at this level */

      target_index = (target_number >>
                           (target_height * TUP_SHIFT_DIST)) &
                        TUP_SHIFT_MASK;

#ifdef TRAPS

      /* we shouldn't be missing any header records */

      if (target_work_hdr->t_child[target_index].t_header == NULL)
         trap(__FILE__,__LINE__,msg_missing_tup_header);

#endif

      /* move down */

      target_work_hdr = target_work_hdr->t_child[target_index].t_header;

   }

   /* set the left target, and remove the last cell */

   target_index = target_number & TUP_SHIFT_MASK;
   target_cell = target_work_hdr->t_child[target_index].t_cell;
   target_work_hdr->t_child[target_index].t_cell = NULL;
   target_root->t_hash_code ^= target_cell->t_hash_code;
   target_root->t_ntype.t_root.t_length--;

   unmark_specifier(left);
   left->sp_form = target_cell->t_spec.sp_form;
   left->sp_val.sp_biggest = target_cell->t_spec.sp_val.sp_biggest;
   free_tuple_cell(target_cell);

   /*
    *  We now need to strip any trailing omegas from the tuple.
    */

   /* if the length is zero, don't try this */

   if (target_root->t_ntype.t_root.t_length != 0) {

      /* drop to a leaf at the rightmost position */

      target_number = target_root->t_ntype.t_root.t_length - 1;
      target_work_hdr = target_root;
      for (target_height = target_root->t_ntype.t_root.t_height;
           target_height;
           target_height--) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (target_work_hdr->t_child[target_index].t_header == NULL) {

            get_tuple_header(new_hdr);
            new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
            new_hdr->t_ntype.t_intern.t_child_index = target_index;

            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_hdr->t_child[i++].t_cell = NULL);

            target_work_hdr->t_child[target_index].t_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->t_child[target_index].t_header;

         }
      }

      /* set the target index to the last element */

      target_index = target_number & TUP_SHIFT_MASK;

      /* keep stripping omegas */

      for (;;) {

         if (!target_height && target_index >= 0) {

            if (target_work_hdr->t_child[target_index].t_cell != NULL)
               break;

            target_root->t_ntype.t_root.t_length--;
            target_index--;

            continue;

         }

         /* move up if we're at the end of a node */

         if (target_index < 0) {

            if (target_work_hdr == target_root)
               break;

            target_height++;
            target_index = target_work_hdr->t_ntype.t_intern.t_child_index;
            target_work_hdr = target_work_hdr->t_ntype.t_intern.t_parent;
            free_tuple_header(
                  target_work_hdr->t_child[target_index].t_header);
            target_work_hdr->t_child[target_index].t_header = NULL;
            target_index--;

            continue;

         }

         /* skip over null nodes */

         if (target_work_hdr->t_child[target_index].t_header == NULL) {

            target_root->t_ntype.t_root.t_length -=
                  1L << (target_height * TUP_SHIFT_DIST);
            target_index--;

            continue;

         }

         /* otherwise drop down a level */

         target_work_hdr =
            target_work_hdr->t_child[target_index].t_header;
         target_index = TUP_HEADER_SIZE - 1;

         target_height--;

      }

      /* we've shortened the tuple -- now reduce the height */

      while (target_root->t_ntype.t_root.t_height > 0 &&
             target_root->t_ntype.t_root.t_length <=
                (int32)(1L << (target_root->t_ntype.t_root.t_height *
                              TUP_SHIFT_DIST))) {

         target_work_hdr = target_root->t_child[0].t_header;

         /* it's possible that we deleted internal headers */

         if (target_work_hdr == NULL) {

            target_root->t_ntype.t_root.t_height--;
            continue;

         }

         /* delete the root node */

         target_work_hdr->t_use_count = target_root->t_use_count;
         target_work_hdr->t_hash_code =
               target_root->t_hash_code;
         target_work_hdr->t_ntype.t_root.t_length =
               target_root->t_ntype.t_root.t_length;
         target_work_hdr->t_ntype.t_root.t_height =
               target_root->t_ntype.t_root.t_height - 1;

         free_tuple_header(target_root);
         target_root = target_work_hdr;

      }
   }

   /* set the other target values */

   unmark_specifier(right);
   right->sp_form = ft_tuple;
   right->sp_val.sp_tuple_ptr = target_root;

   if (target != NULL) {

      mark_specifier(left);
      unmark_specifier(target);
      target->sp_form = left->sp_form;
      target->sp_val.sp_biggest = left->sp_val.sp_biggest;

   }

   return;

}

/*\
 *  \function{tuple\_arb()}
 *
 *  This functions implements the SETL2 \verb"ARB" operation.
\*/

void tuple_arb(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *source)                  /* source tuple                      */

{
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
int32 source_number;                   /* source element number             */

   /* if the source set is empty, return omega */

   source_root = source->sp_val.sp_tuple_ptr;
   if (source_root->t_ntype.t_root.t_length == 0) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return;

   }

   /* go down to the appropriate leaf */

   source_number = source_root->t_ntype.t_root.t_length - 1;
   source_work_hdr = source_root;
   for (source_height = source_root->t_ntype.t_root.t_height;
        source_height;
        source_height--) {

      /* extract the element's index at this level */

      source_index = (source_number >>
                           (source_height * TUP_SHIFT_DIST)) &
                        TUP_SHIFT_MASK;

#ifdef TRAPS

      /* we shouldn't be missing any header records */

      if (source_work_hdr->t_child[source_index].t_header == NULL)
         trap(__FILE__,__LINE__,msg_missing_tup_header);

#endif

      /* move down */

      source_work_hdr = source_work_hdr->t_child[source_index].t_header;

   }

   /* set the target and return */

   source_index = source_number & TUP_SHIFT_MASK;
   source_cell = source_work_hdr->t_child[source_index].t_cell;

   mark_specifier(&(source_cell->t_spec));
   unmark_specifier(target);
   target->sp_form = source_cell->t_spec.sp_form;
   target->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;

   return;

}
