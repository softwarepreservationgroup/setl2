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
 *  \package{Sets}
 *
 *  This package contains definitions of the structures used to implement
 *  SETL2 sets, and several low level functions to manipulate those
 *  structures.  We freely confess that we used a very ugly, non-portable
 *  C coding style here, in hopes of getting a fast implementation.  We
 *  have tried to isolate this ugliness to the macros and functions which
 *  allocate and release nodes.  In particular, there are some unsafe
 *  castes, so please make sure this file is compiled with unsafe
 *  optimizations disabled!
 *
 *  Most of these functions directly implement SETL2 set operators.
 *
 *  \texify{sets.h}
 *
 *  \packagebody{Sets}
\*/

/* standard C header files */

#include <math.h>                      /* C math functions                  */

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "specs.h"                     /* specifiers                        */
#include "sets.h"                      /* sets                              */
#include "iters.h"                     /* iterators                         */
#include "pcode.h"                     /* pseudo code                       */
#include "execute.h"                   /* core interpreter                  */

/* performance tuning constants */

#define SET_HEADER_BLOCK_SIZE  100     /* set header block size             */
#define SET_CELL_BLOCK_SIZE    400     /* set cell block size               */

/*\
 *  \function{alloc\_set\_headers()}
 *
 *  This function allocates a block of set headers and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste header items to pointers to header items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the header node.
 *  It will work provided a header item is larger than a pointer, which
 *  is the case.
\*/

void alloc_set_headers(
   SETL_SYSTEM_PROTO_VOID)

{
set_h_ptr_type new_block;              /* first header in allocated block   */

   /* allocate a new block */

   new_block = (set_h_ptr_type)malloc((size_t)
         (SET_HEADER_BLOCK_SIZE * sizeof(struct set_h_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (SET_H_NEXT_FREE = new_block;
        SET_H_NEXT_FREE < new_block + SET_HEADER_BLOCK_SIZE - 1;
        SET_H_NEXT_FREE++) {

      *((set_h_ptr_type *)SET_H_NEXT_FREE) = SET_H_NEXT_FREE + 1;

   }

   *((set_h_ptr_type *)SET_H_NEXT_FREE) = NULL;

   /* set next free node to new block */

   SET_H_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{alloc\_set\_cells()}
 *
 *  This function allocates a block of set cells and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste cell items to pointers to cell items in order to form the free
 *  list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently, while
 *  at the same time avoiding an extra pointer on the cell node.  It
 *  will work provided a cell item is larger than a pointer, which is
 *  the case.
\*/

void alloc_set_cells(
   SETL_SYSTEM_PROTO_VOID)

{
set_c_ptr_type new_block;              /* first cell in allocated block     */

   /* allocate a new block */

   new_block = (set_c_ptr_type)malloc((size_t)
         (SET_CELL_BLOCK_SIZE * sizeof(struct set_c_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (SET_C_NEXT_FREE = new_block;
        SET_C_NEXT_FREE < new_block + SET_CELL_BLOCK_SIZE - 1;
        SET_C_NEXT_FREE++) {

      *((set_c_ptr_type *)SET_C_NEXT_FREE) = SET_C_NEXT_FREE + 1;

   }

   *((set_c_ptr_type *)SET_C_NEXT_FREE) = NULL;

   /* set next free node to new block */

   SET_C_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{free\_set()}
 *
 *  This function frees the memory used by an entire set structure.
\*/

void free_set(
   SETL_SYSTEM_PROTO
   set_h_ptr_type root)                /* set to be deleted                 */

{
set_h_ptr_type work_hdr;               /* used to traverse header tree      */
set_h_ptr_type old_hdr;                /* deleted header record pointer     */
set_c_ptr_type t1,t2;                  /* used to loop over cell list       */
                                       /* deleting cells                    */
int height;                            /* working height of header tree     */
int index;                             /* index within header hash table    */

   /* we start iterating from the root, at the left of the hash table */

   height = root->s_ntype.s_root.s_height;
   work_hdr = root;
   index = 0;

   /* delete nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, delete all the set elements */

      if (!height) {

         for (index = 0; index < SET_HASH_SIZE; index++) {

            t1 = work_hdr->s_child[index].s_cell;
            while (t1 != NULL) {

               t2 = t1;
               t1 = t1->s_next;
               unmark_specifier(&(t2->s_spec));
               free_set_cell(t2);

            }
         }
      }

      /* if we've finished a header node, move up */

      if (index >= SET_HASH_SIZE) {

         /* when we return to the root we're done */

         if (work_hdr == root)
            break;

         height++;
         index = work_hdr->s_ntype.s_intern.s_child_index + 1;
         old_hdr = work_hdr;
         work_hdr = work_hdr->s_ntype.s_intern.s_parent;
         free_set_header(old_hdr);
         continue;

      }

      /* if we can't move down, continue */

      if (work_hdr->s_child[index].s_header == NULL) {

         index++;
         continue;

      }

      /* we can move down, so do so */

      work_hdr = work_hdr->s_child[index].s_header;
      index = 0;
      height--;

   }

   free_set_header(root);

}

set_h_ptr_type null_set(
   SETL_SYSTEM_PROTO_VOID)

{
set_h_ptr_type target_root;           /* root and internal node pointers   */
int i;

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_hash_code = 0 ;
   target_root->s_ntype.s_root.s_height = 0;
   target_root->s_ntype.s_root.s_cardinality = 0;
   for (i = 0;
        i< SET_HASH_SIZE;
        target_root->s_child[i++].s_cell=NULL);

   return target_root;
}

/*\
 *  \function{copy\_set()}
 *
 *  This function copies an entire set structure.
\*/

set_h_ptr_type copy_set(
   SETL_SYSTEM_PROTO
   set_h_ptr_type source_root)         /* set to be copied                  */

{
set_h_ptr_type source_work_hdr;        /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type *target_tail;           /* attach new cells here             */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */

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
      fprintf(DEBUG_FILE,"*COPY_SET*\n");
#endif

   /* allocate a new root header node */

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_ntype.s_root.s_height =
         source_root->s_ntype.s_root.s_height;
   target_root->s_hash_code =
         source_root->s_hash_code;
   target_root->s_ntype.s_root.s_cardinality =
         source_root->s_ntype.s_root.s_cardinality;

   /* we start iterating from the root, at the left of the hash table */

   source_height = source_root->s_ntype.s_root.s_height;
   source_work_hdr = source_root;
   target_work_hdr = target_root;
   source_index = 0;

   /* copy nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, copy all the set elements */

      if (!source_height) {

         for (source_index = 0;
              source_index < SET_HASH_SIZE;
              source_index++) {

            /* loop over the clash list */

            target_tail = &(target_work_hdr->s_child[source_index].s_cell);

            for (source_cell = source_work_hdr->s_child[source_index].s_cell;
                 source_cell != NULL;
                 source_cell = source_cell->s_next) {

               get_set_cell(new_cell);
               memcpy((void *)new_cell,
                      (void *)source_cell,
                      sizeof(struct set_c_item));
               *target_tail = new_cell;
               target_tail = &(new_cell->s_next);
               mark_specifier(&(new_cell->s_spec));

            }

            *target_tail = NULL;

         }
      }

      /* if we've finished an internal node, move up */

      if (source_index >= SET_HASH_SIZE) {

         if (source_work_hdr == source_root)
            break;

         source_height++;
         source_index = source_work_hdr->s_ntype.s_intern.s_child_index + 1;
         source_work_hdr = source_work_hdr->s_ntype.s_intern.s_parent;
         target_work_hdr = target_work_hdr->s_ntype.s_intern.s_parent;

         continue;

      }

      /* if we can't move down, continue */

      if (source_work_hdr->s_child[source_index].s_header == NULL) {

         target_work_hdr->s_child[source_index].s_header = NULL;
         source_index++;
         continue;

      }

      /* we can move down, so do so */

      source_work_hdr = source_work_hdr->s_child[source_index].s_header;
      get_set_header(new_hdr);
      target_work_hdr->s_child[source_index].s_header = new_hdr;
      new_hdr->s_ntype.s_intern.s_parent = target_work_hdr;
      new_hdr->s_ntype.s_intern.s_child_index = source_index;
      target_work_hdr = new_hdr;

      source_index = 0;
      source_height--;

   }

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
 *  \function{expand\_set\_header()}
 *
 *  This function adds one level to the height of a set header tree.
 *  It should be called when the average length of the clash lists is at
 *  least two.  We loop over the leaves of the header tree, splitting
 *  each one into a tree.
\*/

set_h_ptr_type set_expand_header(
   SETL_SYSTEM_PROTO
   set_h_ptr_type source_root)         /* set to be enlarged                */

{
set_h_ptr_type source_leaf;            /* internal node pointer             */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type target_subtree, target_work_hdr;
                                       /* root and internal node pointers   */
int target_index;                      /* current index                     */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int i;                                 /* temporary looping variable        */
int shift_distance;                    /* distance leaf hash codes must     */
                                       /* be shifted, a function of depth   */

   /*
    *  Set up to loop over the source set, producing one leaf node at a
    *  time.
    */

   source_leaf = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_root->s_ntype.s_root.s_height++;
   source_index = 0;
   shift_distance = source_height * SET_SHIFT_DIST;

   /* loop over the nodes of source */

   for (;;) {

      /* descend to a leaf */

      while (source_height) {

         /* move down if possible */

         if (source_index < SET_HASH_SIZE) {

            /* skip over null nodes */

            if (source_leaf->s_child[source_index].s_header == NULL) {

               source_index++;
               continue;

            }

            /* we can move down, so do so */

            source_leaf =
               source_leaf->s_child[source_index].s_header;
            source_index = 0;
            source_height--;

            continue;

         }

         /* if there are no more elements, break */

         if (source_leaf == source_root) {

            source_leaf = NULL;
            break;

         }

         /* move up if we're at the end of a node */

         source_height++;
         source_index =
               source_leaf->s_ntype.s_intern.s_child_index + 1;
         source_leaf =
            source_leaf->s_ntype.s_intern.s_parent;

         continue;

      }

      /* break if we can't find a leaf */

      if (source_leaf == NULL)
         break;

      /*
       *  At this point we have a leaf which must be split.  We create a
       *  new header node, then loop over the source copying the clash
       *  lists.
       */

      get_set_header(target_subtree);
      memcpy((void *)target_subtree,
             (void *)source_leaf,
             sizeof(struct set_h_item));

      for (i = 0;
           i < SET_HASH_SIZE;
           target_subtree->s_child[i++].s_header = NULL);

      for (source_index = 0; source_index < SET_HASH_SIZE; source_index++) {

         source_cell = source_leaf->s_child[source_index].s_cell;
         while (source_cell != NULL) {

            work_hash_code = (source_cell->s_hash_code >> shift_distance);

            target_index = work_hash_code & SET_HASH_MASK;
            work_hash_code = work_hash_code >> SET_SHIFT_DIST;

            /* if we're missing a header record, insert it */

            if (target_subtree->s_child[target_index].s_header == NULL) {

               get_set_header(target_work_hdr);
               target_work_hdr->s_ntype.s_intern.s_parent =
                     target_subtree;
               target_work_hdr->s_ntype.s_intern.s_child_index =
                     target_index;
               for (i = 0;
                    i < SET_HASH_SIZE;
                    target_work_hdr->s_child[i++].s_cell = NULL);
               target_subtree->s_child[target_index].s_header =
                     target_work_hdr;

            }
            else {

               target_work_hdr =
                  target_subtree->s_child[target_index].s_header;

            }

            /* search the clash list for the correct position */

            target_index = work_hash_code & SET_HASH_MASK;
            target_tail = &(target_work_hdr->s_child[target_index].s_cell);
            for (target_cell = *target_tail;
                 target_cell != NULL &&
                    target_cell->s_hash_code < source_cell->s_hash_code;
                 target_cell = target_cell->s_next) {

               target_tail = &(target_cell->s_next);

            }

            /* shift the source cell to the new subtree */

            target_cell = source_cell;
            source_cell = source_cell->s_next;
            target_cell->s_next = *target_tail;
            *target_tail = target_cell;

         }
      }

      /* if the leaf is the root, we're done */

      if (source_leaf == source_root) {

         free_set_header(source_root);
         return target_subtree;

      }

      /* set up to find the next leaf, by moving to the parent */

      source_height++;
      source_index =
            source_leaf->s_ntype.s_intern.s_child_index;
      source_leaf =
         source_leaf->s_ntype.s_intern.s_parent;
      free_set_header(source_leaf->s_child[source_index].s_header);
      source_leaf->s_child[source_index].s_header = target_subtree;
      source_index++;

   }

   /* if we break without returning, return the original root */

   return source_root;

}

/*\
 *  \function{contract\_set\_header()}
 *
 *  This function subtracts one level from the height of a set header
 *  tree.  It should be called when the average length of the clash
 *  lists for a tree with smaller height is no more than one.  We loop
 *  over the lowest level internal nodes of the source tree, collapsing
 *  each such node.
\*/

set_h_ptr_type set_contract_header(
   SETL_SYSTEM_PROTO
   set_h_ptr_type source_root)         /* set to be contracted              */

{
set_h_ptr_type source_subtree;         /* internal node pointer             */
set_h_ptr_type source_leaf;            /* leaf node                         */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type target_leaf;            /* target leaf node                  */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int i;                                 /* temporary looping variable        */

   /*
    *  Set up to loop over the source set, producing one bottom level node
    *  at a time.
    */

   /* TOTO */
   
   
   if (source_root->s_ntype.s_root.s_height==0) 
   		return source_root;

   source_subtree = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_root->s_ntype.s_root.s_height--;
   
   source_index = 0;

   /* loop over the nodes of source */

   for (;;) {

      /* descend to a leaf */

      while (source_height > 1) {

         /* move down if possible */

         if (source_index < SET_HASH_SIZE) {

            /* skip over null nodes */

            if (source_subtree->s_child[source_index].s_header == NULL) {

               source_index++;
               continue;

            }

            /* we can move down, so do so */

            source_subtree =
               source_subtree->s_child[source_index].s_header;
            source_index = 0;
            source_height--;

            continue;

         }

         /* if there are no more elements, break */

         if (source_subtree == source_root) {

            source_subtree = NULL;
            break;

         }

         /* move up if we're at the end of a node */

         source_height++;
         source_index =
               source_subtree->s_ntype.s_intern.s_child_index + 1;
         source_subtree =
            source_subtree->s_ntype.s_intern.s_parent;

         continue;

      }

      /* break if we can't find a leaf */

      if (source_subtree == NULL)
         break;

      /*
       *  At this point we have a leaf which must be collapsed.  We
       *  create a new header node, then merge clash lists from the
       *  source node into the target node.
       */

      get_set_header(target_leaf);
      memcpy((void *)target_leaf,
             (void *)source_subtree,
             sizeof(struct set_h_item));

      for (i = 0;
           i < SET_HASH_SIZE;
           target_leaf->s_child[i++].s_header = NULL);

      /* merge the clash lists of each leaf node */

      for (source_index = 0; source_index < SET_HASH_SIZE; source_index++) {

         source_leaf = source_subtree->s_child[source_index].s_header;
         if (source_leaf == NULL)
            continue;

         /* merge the clash lists of the current leaf into one target list */

         for (i = 0; i < SET_HASH_SIZE; i++) {

            target_tail = &(target_leaf->s_child[source_index].s_cell);
            target_cell = *target_tail;

            source_cell = source_leaf->s_child[i].s_cell;
            while (source_cell != NULL) {

               /* search the clash list for the correct position */

               while (target_cell != NULL &&
                      target_cell->s_hash_code < source_cell->s_hash_code) {

                  target_tail = &(target_cell->s_next);
                  target_cell = target_cell->s_next;

               }

               /* shift the source cell to the new subtree */

               target_cell = source_cell;
               source_cell = source_cell->s_next;
               target_cell->s_next = *target_tail;
               *target_tail = target_cell;
               target_tail = &(target_cell->s_next);

            }
         }

         free_set_header(source_leaf);

      }

      /* if the subtree is the root, we're done */

      if (source_subtree == source_root) {

         free_set_header(source_root);
         return target_leaf;

      }

      /* set up to find the next leaf, by moving to the parent */

      source_height++;
      source_index =
            source_subtree->s_ntype.s_intern.s_child_index;
      source_subtree =
         source_subtree->s_ntype.s_intern.s_parent;
      free_set_header(source_subtree->s_child[source_index].s_header);
      source_subtree->s_child[source_index].s_header = target_leaf;
      source_index++;

   }

   /* if we break without returning, return the original root */

   return source_root;

}

/*\
 *  \function{set\_union()}
 *
 *  This function forms the union of two sets.
\*/

void set_union(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target of union operation         */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
specifier *target_element;             /* set element                       */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int is_equal;                          /* YES if two specifiers are equal   */
int i;                                 /* temporary looping variable        */

   /*
    *  First we set up source and target sets.  We would like to
    *  destructively use one of the sets if possible.
    */


   if (target == left && target != right &&
       ((target->sp_val.sp_set_ptr)->s_use_count == 1)) {
         
      target_root = target->sp_val.sp_set_ptr;
      target->sp_form = ft_omega;
      source_root = right->sp_val.sp_set_ptr;

   }
   else if (target == right && target != left &&
            ((target->sp_val.sp_set_ptr)->s_use_count == 1)) {

      target_root = target->sp_val.sp_set_ptr;
      target->sp_form = ft_omega;
      source_root = left->sp_val.sp_set_ptr;

   }
   else {

      /* we can not use a set destructively, so pick the biggest */

      if ((right->sp_val.sp_set_ptr)->s_ntype.s_root.s_cardinality >
          (left->sp_val.sp_set_ptr)->s_ntype.s_root.s_cardinality) {

         target_root = copy_set(SETL_SYSTEM right->sp_val.sp_set_ptr);
         source_root = left->sp_val.sp_set_ptr;

      }
      else {

         target_root = copy_set(SETL_SYSTEM left->sp_val.sp_set_ptr);
         source_root = right->sp_val.sp_set_ptr;

      }
   }

   /* we may have to expand the size of the header, so find the trigger */

   expansion_trigger = (1 << ((target_root->s_ntype.s_root.s_height + 1)
                              * SET_SHIFT_DIST)) * SET_CLASH_SIZE;

   /*
    *  We loop over the source set producing one element at a time.
    */

   source_work_hdr = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_cell = NULL;
   source_index = 0;

   /* loop over the elements of source */

   for (;;) {

      /* find the next element in the set */

      target_element = NULL;
      for (;;) {

         /* if we have an element already, break */

         if (source_cell != NULL) {

            target_element = &(source_cell->s_spec);
            target_hash_code = source_cell->s_hash_code;
            source_cell = source_cell->s_next;
            break;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < SET_HASH_SIZE) {

            source_cell = source_work_hdr->s_child[source_index].s_cell;
            source_index++;
            continue;

         }

         /* the current header node is exhausted -- find the next one */

         /* move up if we're at the end of a node */

         if (source_index >= SET_HASH_SIZE) {

            /* there are not more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->s_ntype.s_intern.s_child_index + 1;
            source_work_hdr =
               source_work_hdr->s_ntype.s_intern.s_parent;
            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->s_child[source_index].s_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->s_child[source_index].s_header;
         source_index = 0;
         source_height--;

      }

      /* if we've exhausted a set break again */

      if (target_element == NULL)
         break;

      /*
       *  At this point we have an element we would like to insert into
       *  the target.
       */

      target_work_hdr = target_root;

      /* get the element's hash code */

      work_hash_code = target_hash_code;

      /* descend the header tree until we get to a leaf */

      target_height = target_root->s_ntype.s_root.s_height;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & SET_HASH_MASK;
         work_hash_code = work_hash_code >> SET_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (target_work_hdr->s_child[target_index].s_header == NULL) {

            get_set_header(new_hdr);
            new_hdr->s_ntype.s_intern.s_parent = target_work_hdr;
            new_hdr->s_ntype.s_intern.s_child_index = target_index;
            for (i = 0;
                 i < SET_HASH_SIZE;
                 new_hdr->s_child[i++].s_cell = NULL);
            target_work_hdr->s_child[target_index].s_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->s_child[target_index].s_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  The next problem is to determine if the element is
       *  already in the set.  We compare the element with the clash
       *  list.
       */

      target_index = work_hash_code & SET_HASH_MASK;
      target_tail = &(target_work_hdr->s_child[target_index].s_cell);
      for (target_cell = *target_tail;
           target_cell != NULL &&
              target_cell->s_hash_code < target_hash_code;
           target_cell = target_cell->s_next) {

         target_tail = &(target_cell->s_next);

      }

      /* check for a duplicate element */

      is_equal = NO;
      while (target_cell != NULL &&
             target_cell->s_hash_code == target_hash_code) {

         spec_equal(is_equal,&(target_cell->s_spec),target_element);

         if (is_equal)
            break;

         target_tail = &(target_cell->s_next);
         target_cell = target_cell->s_next;

      }

      /* if we found the element, continue */

      if (is_equal)
         continue;

      /* if we reach this point we didn't find the element, so we insert it */

      get_set_cell(new_cell);
      mark_specifier(target_element);
      new_cell->s_spec.sp_form = target_element->sp_form;
      new_cell->s_spec.sp_val.sp_biggest =
         target_element->sp_val.sp_biggest;
      new_cell->s_hash_code = target_hash_code;
      new_cell->s_next = *target_tail;
      *target_tail = new_cell;
      target_root->s_ntype.s_root.s_cardinality++;
      target_root->s_hash_code ^= target_hash_code;

      /* expand the set header if necessary */

      if (target_root->s_ntype.s_root.s_cardinality > expansion_trigger) {

         target_root = set_expand_header(SETL_SYSTEM target_root);
         expansion_trigger *= SET_HASH_SIZE;

      }
   }

   /* finally, set the target value */

   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = target_root;

   return;

}

/*\
 *  \function{set\_difference()}
 *
 *  This function forms the set difference for two sets.
\*/

void set_difference(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target of union operation         */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
specifier *target_element;             /* set element                       */
int32 target_hash_code;                /* hash code of target element       */
int32 contraction_trigger;             /* cardinality which triggers        */
                                       /* header contraction                */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int is_equal;                          /* YES if two specifiers are equal   */

   /*
    *  First we set up a target set.  We would like to use the left
    *  operand destructively if possible.
    */

   if (target == left && target != right &&
       (target->sp_val.sp_set_ptr)->s_use_count == 1) {

      target_root = target->sp_val.sp_set_ptr;
      target->sp_form = ft_omega;

   }
   else {

      target_root = copy_set(SETL_SYSTEM left->sp_val.sp_set_ptr);

   }

   source_root = right->sp_val.sp_set_ptr;

   /*
    *  We loop over the source set producing one element at a time.
    */

   source_work_hdr = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_cell = NULL;
   source_index = 0;

   /* we may have to compress the header, so find the trigger */

   contraction_trigger = (1 << ((target_root->s_ntype.s_root.s_height)
                                 * SET_SHIFT_DIST));
   if (contraction_trigger == 1)
      contraction_trigger = 0;

   /* loop over the elements of source */

   for (;;) {

      /* find the next element in the set */

      target_element = NULL;
      for (;;) {

         /* if we have an element already, break */

         if (source_cell != NULL) {

            target_element = &(source_cell->s_spec);
            target_hash_code = source_cell->s_hash_code;
            source_cell = source_cell->s_next;
            break;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < SET_HASH_SIZE) {

            source_cell = source_work_hdr->s_child[source_index].s_cell;
            source_index++;
            continue;

         }

         /* the current header node is exhausted -- find the next one */

         /* move up if we're at the end of a node */

         if (source_index >= SET_HASH_SIZE) {

            /* there are not more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->s_ntype.s_intern.s_child_index + 1;
            source_work_hdr =
               source_work_hdr->s_ntype.s_intern.s_parent;
            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->s_child[source_index].s_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->s_child[source_index].s_header;
         source_index = 0;
         source_height--;

      }

      /* if we've exhausted a set break again */

      if (target_element == NULL)
         break;

      /*
       *  At this point we have an element we would like to remove from
       *  the target.
       */

      target_work_hdr = target_root;

      /* get the element's hash code */

      work_hash_code = target_hash_code;

      /* descend the header tree until we get to a leaf */

      target_height = target_root->s_ntype.s_root.s_height;
      while (target_work_hdr != NULL && target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & SET_HASH_MASK;
         work_hash_code = work_hash_code >> SET_SHIFT_DIST;

         target_work_hdr =
            target_work_hdr->s_child[target_index].s_header;

      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  The next problem is to determine if the element is in the
       *  set.  We compare the element with the clash list.
       */

      if (target_work_hdr != NULL) {

         target_index = work_hash_code & SET_HASH_MASK;
         target_tail = &(target_work_hdr->s_child[target_index].s_cell);
         for (target_cell = *target_tail;
              target_cell != NULL &&
                 target_cell->s_hash_code < target_hash_code;
              target_cell = target_cell->s_next) {

            target_tail = &(target_cell->s_next);

         }

         /* search the list of elements with the same hash code */

         is_equal = NO;
         while (target_cell != NULL &&
                target_cell->s_hash_code == target_hash_code) {

            spec_equal(is_equal,&(target_cell->s_spec),target_element);

            if (is_equal)
               break;

            target_tail = &(target_cell->s_next);
            target_cell = target_cell->s_next;

         }

         /* if we found the element, delete it */

         if (is_equal) {

            unmark_specifier(&(target_cell->s_spec));
            *target_tail = target_cell->s_next;
            target_root->s_ntype.s_root.s_cardinality--;
            target_root->s_hash_code ^= target_hash_code;
            free_set_cell(target_cell);

            /* we may have to reduce the height of the set */

            if (target_root->s_ntype.s_root.s_cardinality <
                contraction_trigger) {

               target_root = set_contract_header(SETL_SYSTEM target_root);
               contraction_trigger /= SET_HASH_SIZE;

            }
         }
      }
   }

   /* finally, set the target value */

   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = target_root;

   return;

}

/*\
 *  \function{set\_intersection()}
 *
 *  This function forms the intersection of two sets.
\*/

void set_intersection(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
set_h_ptr_type lsource_root, lsource_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type lsource_list;           /* clash list                        */
set_c_ptr_type lsource_cell;           /* cell pointer                      */
int lsource_height, lsource_index;     /* current height and index          */
set_h_ptr_type rsource_root, rsource_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type rsource_list;           /* clash list                        */
set_c_ptr_type rsource_cell, rsource_work_cell;
                                       /* cell pointers                     */
int rsource_height;                    /* current height                    */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
specifier *target_element;             /* set element                       */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 contraction_trigger;             /* cardinality which triggers        */
                                       /* header contraction                */
int is_equal;                          /* YES if two specifiers are equal   */
int i;                                 /* temporary looping variable        */

   /* we want the set with the greater height on the left */

   if ((left->sp_val.sp_set_ptr)->s_ntype.s_root.s_height >=
       (right->sp_val.sp_set_ptr)->s_ntype.s_root.s_height) {

      lsource_root = left->sp_val.sp_set_ptr;
      rsource_root = right->sp_val.sp_set_ptr;

   }
   else {

      lsource_root = right->sp_val.sp_set_ptr;
      rsource_root = left->sp_val.sp_set_ptr;

   }

   /* create a new set for the target */

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_hash_code = 0;
   target_root->s_ntype.s_root.s_cardinality = 0;
   target_root->s_ntype.s_root.s_height =
      rsource_root->s_ntype.s_root.s_height;
   for (i = 0;
        i < SET_HASH_SIZE;
        target_root->s_child[i++].s_cell = NULL);

   /* set up to loop over the left and right sets in parallel */

   lsource_work_hdr = lsource_root;
   lsource_height = lsource_root->s_ntype.s_root.s_height;
   lsource_index = 0;

   rsource_work_hdr = rsource_root;
   rsource_height = rsource_root->s_ntype.s_root.s_height;
   rsource_list = NULL;

   /* find successive clash lists, where the right should contain the left */

   for (;;) {

      /* find the next clash list */

      lsource_list = NULL;
      while (lsource_list == NULL) {

         /* return the clash list if we're at a leaf */

         if (!lsource_height && lsource_index < SET_HASH_SIZE) {

            lsource_list =
               lsource_work_hdr->s_child[lsource_index].s_cell;

            /* if the right is also at a leaf, we set the right list */

            if (!rsource_height) {

               rsource_list =
                  rsource_work_hdr->s_child[lsource_index].s_cell;

            }

            lsource_index++;

            continue;

         }

         /* move up if we're at the end of a node */

         if (lsource_index >= SET_HASH_SIZE) {

            if (lsource_work_hdr == lsource_root)
               break;

            lsource_height++;
            lsource_index =
               lsource_work_hdr->s_ntype.s_intern.s_child_index + 1;
            lsource_work_hdr =
               lsource_work_hdr->s_ntype.s_intern.s_parent;

            /* move up on the right also */

            if (rsource_height >= 0) {

               rsource_work_hdr =
                  rsource_work_hdr->s_ntype.s_intern.s_parent;
               rsource_list = NULL;

            }

            rsource_height++;

            continue;

         }

         /* skip over null nodes */

         if (lsource_work_hdr->s_child[lsource_index].s_header == NULL) {

            lsource_index++;
            continue;

         }

         /* otherwise drop down a level */

         lsource_work_hdr =
            lsource_work_hdr->s_child[lsource_index].s_header;
         lsource_height--;

         /* drop down on the right, or return a right list */

         if (rsource_height > 0) {

            if (rsource_work_hdr->s_child[lsource_index].s_header == NULL) {

               get_set_header(new_hdr);
               new_hdr->s_ntype.s_intern.s_parent = rsource_work_hdr;
               new_hdr->s_ntype.s_intern.s_child_index = lsource_index;
               for (i = 0;
                    i < SET_HASH_SIZE;
                    new_hdr->s_child[i++].s_cell = NULL);
               rsource_work_hdr->s_child[lsource_index].s_header = new_hdr;
               rsource_work_hdr = new_hdr;

            }
            else {

               rsource_work_hdr =
                  rsource_work_hdr->s_child[lsource_index].s_header;

            }
         }
         else if (!rsource_height) {

            rsource_list =
               rsource_work_hdr->s_child[lsource_index].s_cell;

         }

         rsource_height--;
         lsource_index = 0;

      }

      /* break if we didn't find a list */

      if (lsource_list == NULL)
         break;

      /*
       *  At this point we have a clash list from each set.  The left
       *  lists will be unique for each iteration, but the right lists
       *  may repeat.  The right list will always contain those elements
       *  on the left which are in the right set, and perhaps some
       *  others.
       */

      /* loop over the left list */

      rsource_cell = rsource_list;
      for (lsource_cell = lsource_list;
           lsource_cell != NULL;
           lsource_cell = lsource_cell->s_next) {

         /* search for the element in the right list */

         while (rsource_cell != NULL &&
                rsource_cell->s_hash_code < lsource_cell->s_hash_code)
            rsource_cell = rsource_cell->s_next;

         /* search through elements with identical hash codes */

         is_equal = NO;
         for (rsource_work_cell = rsource_cell;
              rsource_work_cell != NULL &&
                 rsource_work_cell->s_hash_code ==
                    lsource_cell->s_hash_code &&
                 !is_equal;
              rsource_work_cell = rsource_work_cell->s_next) {

            spec_equal(is_equal,
                       &(lsource_cell->s_spec),
                       &(rsource_work_cell->s_spec));

         }

         /* if we didn't find the element in both lists, continue */

         if (!is_equal)
            continue;

         target_element = &(lsource_cell->s_spec);
         target_hash_code = lsource_cell->s_hash_code;

         /*
          *  At this point we have an element we would like to insert into
          *  the target.
          */

         target_work_hdr = target_root;
         work_hash_code = target_hash_code;

         /* descend the header tree until we get to a leaf */

         target_height = target_root->s_ntype.s_root.s_height;
         while (target_height--) {

            /* extract the element's index at this level */

            target_index = work_hash_code & SET_HASH_MASK;
            work_hash_code = work_hash_code >> SET_SHIFT_DIST;

            /* if we're missing a header record, insert it */

            if (target_work_hdr->s_child[target_index].s_header == NULL) {

               get_set_header(new_hdr);
               new_hdr->s_ntype.s_intern.s_parent = target_work_hdr;
               new_hdr->s_ntype.s_intern.s_child_index = target_index;
               for (i = 0;
                    i < SET_HASH_SIZE;
                    new_hdr->s_child[i++].s_cell = NULL);
               target_work_hdr->s_child[target_index].s_header = new_hdr;
               target_work_hdr = new_hdr;

            }
            else {

               target_work_hdr =
                  target_work_hdr->s_child[target_index].s_header;

            }
         }

         /*
          *  At this point, target_work_hdr points to the lowest level header
          *  record.  Now we must find the position within the clash list
          *  where we would like to insert the element.  Remember that we
          *  keep the clash lists sorted by hash code.
          */

         target_index = work_hash_code & SET_HASH_MASK;
         target_tail = &(target_work_hdr->s_child[target_index].s_cell);
         for (target_cell = *target_tail;
              target_cell != NULL &&
                 target_cell->s_hash_code < target_hash_code;
              target_cell = target_cell->s_next) {

            target_tail = &(target_cell->s_next);

         }

         /* we insert the element */

         get_set_cell(new_cell);
         mark_specifier(target_element);
         new_cell->s_spec.sp_form = target_element->sp_form;
         new_cell->s_spec.sp_val.sp_biggest =
            target_element->sp_val.sp_biggest;
         new_cell->s_hash_code = target_hash_code;
         new_cell->s_next = *target_tail;
         *target_tail = new_cell;
         target_root->s_ntype.s_root.s_cardinality++;
         target_root->s_hash_code ^= target_hash_code;

      }
   }

   /* if our estimate of the header height was too large, compress it */

   contraction_trigger = (1 << ((target_root->s_ntype.s_root.s_height)
                              * SET_SHIFT_DIST));
   if (contraction_trigger == 1)
      contraction_trigger = 0;

   while (target_root->s_ntype.s_root.s_cardinality < contraction_trigger) {

      target_root = set_contract_header(SETL_SYSTEM target_root);
      contraction_trigger /= SET_HASH_SIZE;

   }

   /* finally, set the target value */

   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = target_root;

}

/*\
 *  \function{set\_symdiff()}
 *
 *  This function forms the symmetric difference of two sets.
\*/

void set_symdiff(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target of union operation         */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
specifier *target_element;             /* set element                       */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 contraction_trigger;             /* cardinality which triggers        */
                                       /* header contraction                */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int is_equal;                          /* YES if two specifiers are equal   */
int i;                                 /* temporary looping variable        */

   /*
    *  First we set up source and target sets.  We would like to
    *  destructively use one of the sets if possible.
    */

   if (target == left && target != right &&
       (target->sp_val.sp_set_ptr)->s_use_count == 1) {

      target_root = target->sp_val.sp_set_ptr;
      target->sp_form = ft_omega;
      source_root = right->sp_val.sp_set_ptr;

   }
   else if (target == right && target != left &&
            (target->sp_val.sp_set_ptr)->s_use_count == 1) {

      target_root = target->sp_val.sp_set_ptr;
      target->sp_form = ft_omega;
      source_root = left->sp_val.sp_set_ptr;

   }
   else {

      /* we can not use a set destructively, so pick the biggest */

      if ((right->sp_val.sp_set_ptr)->s_ntype.s_root.s_cardinality >
          (left->sp_val.sp_set_ptr)->s_ntype.s_root.s_cardinality) {

         target_root = copy_set(SETL_SYSTEM right->sp_val.sp_set_ptr);
         source_root = left->sp_val.sp_set_ptr;

      }
      else {

         target_root = copy_set(SETL_SYSTEM left->sp_val.sp_set_ptr);
         source_root = right->sp_val.sp_set_ptr;

      }
   }

   /* we may have to expand the size of the header, so find the trigger */

   expansion_trigger = (1 << ((target_root->s_ntype.s_root.s_height + 1)
                              * SET_SHIFT_DIST)) * SET_CLASH_SIZE;

   /* we may have to compress the header, so find the trigger */

   contraction_trigger = (1 << ((target_root->s_ntype.s_root.s_height)
                                 * SET_SHIFT_DIST));
   if (contraction_trigger == 1)
      contraction_trigger = 0;

   /*
    *  We loop over the source set producing one element at a time.
    */

   source_work_hdr = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_cell = NULL;
   source_index = 0;

   /* loop over the elements of source */

   for (;;) {

      /* find the next element in the set */

      target_element = NULL;
      for (;;) {

         /* if we have an element already, break */

         if (source_cell != NULL) {

            target_element = &(source_cell->s_spec);
            target_hash_code = source_cell->s_hash_code;
            source_cell = source_cell->s_next;
            break;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < SET_HASH_SIZE) {

            source_cell = source_work_hdr->s_child[source_index].s_cell;
            source_index++;
            continue;

         }

         /* the current header node is exhausted -- find the next one */

         /* move up if we're at the end of a node */

         if (source_index >= SET_HASH_SIZE) {

            /* there are not more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->s_ntype.s_intern.s_child_index + 1;
            source_work_hdr =
               source_work_hdr->s_ntype.s_intern.s_parent;
            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->s_child[source_index].s_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->s_child[source_index].s_header;
         source_index = 0;
         source_height--;

      }

      /* if we've exhausted a set break again */

      if (target_element == NULL)
         break;

      /*
       *  At this point we have an element from the source set.
       */

      target_work_hdr = target_root;

      /* get the element's hash code */

      work_hash_code = target_hash_code;

      /* descend the header tree until we get to a leaf */

      target_height = target_root->s_ntype.s_root.s_height;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & SET_HASH_MASK;
         work_hash_code = work_hash_code >> SET_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (target_work_hdr->s_child[target_index].s_header == NULL) {

            get_set_header(new_hdr);
            new_hdr->s_ntype.s_intern.s_parent = target_work_hdr;
            new_hdr->s_ntype.s_intern.s_child_index = target_index;
            for (i = 0;
                 i < SET_HASH_SIZE;
                 new_hdr->s_child[i++].s_cell = NULL);
            target_work_hdr->s_child[target_index].s_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->s_child[target_index].s_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  The next problem is to determine if the element is
       *  already in the set.  We compare the element with the clash
       *  list.
       */

      target_index = work_hash_code & SET_HASH_MASK;
      target_tail = &(target_work_hdr->s_child[target_index].s_cell);
      for (target_cell = *target_tail;
           target_cell != NULL &&
              target_cell->s_hash_code < target_hash_code;
           target_cell = target_cell->s_next) {

         target_tail = &(target_cell->s_next);

      }

      /* check for a duplicate element */

      is_equal = NO;
      while (target_cell != NULL &&
             target_cell->s_hash_code == target_hash_code) {

         spec_equal(is_equal,&(target_cell->s_spec),target_element);

         if (is_equal)
            break;

         target_tail = &(target_cell->s_next);
         target_cell = target_cell->s_next;

      }

      /* if we found the element, delete it */

      if (is_equal) {

         unmark_specifier(&(target_cell->s_spec));
         *target_tail = target_cell->s_next;
         target_root->s_ntype.s_root.s_cardinality--;
         target_root->s_hash_code ^= target_hash_code;
         free_set_cell(target_cell);

         /* we may have to reduce the height of the set */

         if (target_root->s_ntype.s_root.s_cardinality <
             contraction_trigger) {

            target_root = set_contract_header(SETL_SYSTEM target_root);
            contraction_trigger /= SET_HASH_SIZE;
            expansion_trigger /= SET_HASH_SIZE;

         }
      }

      /* if we didn't find it, insert it */

      else {

         get_set_cell(new_cell);
         mark_specifier(target_element);
         new_cell->s_spec.sp_form = target_element->sp_form;
         new_cell->s_spec.sp_val.sp_biggest =
            target_element->sp_val.sp_biggest;
         new_cell->s_hash_code = target_hash_code;
         new_cell->s_next = *target_tail;
         *target_tail = new_cell;
         target_root->s_ntype.s_root.s_cardinality++;
         target_root->s_hash_code ^= target_hash_code;

         /* expand the set header if necessary */

         if (target_root->s_ntype.s_root.s_cardinality > expansion_trigger) {

            target_root = set_expand_header(SETL_SYSTEM target_root);
            expansion_trigger *= SET_HASH_SIZE;
            contraction_trigger *= SET_HASH_SIZE;

         }
      }
   }

   /* finally, set the target value */

   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = target_root;

   return;

}

/*\
 *  \function{set\_subset()}
 *
 *  This is a simple and stupid subset test.  We generate elements from
 *  the left and check whether each is in the right.
\*/

int set_subset(
   SETL_SYSTEM_PROTO
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
set_h_ptr_type left_root, left_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type left_cell;              /* current cell pointer              */
int left_height, left_index;           /* current height and index          */
set_h_ptr_type right_root, right_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type right_cell;             /* current cell pointer              */
int right_height, right_index;         /* current height and index          */
specifier *right_element;              /* set element                       */
int32 right_hash_code;                 /* hash code of target element       */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int is_equal;                          /* YES if two specifiers are equal   */

   left_root = left->sp_val.sp_set_ptr;
   right_root = right->sp_val.sp_set_ptr;

   /*
    *  We loop over the source set producing one element at a time.
    */

   left_work_hdr = left_root;
   left_height = left_root->s_ntype.s_root.s_height;
   left_cell = NULL;
   left_index = 0;

   /* loop over the elements of source */

   for (;;) {

      /* find the next element in the set */

      right_element = NULL;
      for (;;) {

         /* if we have an element already, break */

         if (left_cell != NULL) {

            right_element = &(left_cell->s_spec);
            right_hash_code = left_cell->s_hash_code;
            left_cell = left_cell->s_next;
            break;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!left_height && left_index < SET_HASH_SIZE) {

            left_cell = left_work_hdr->s_child[left_index].s_cell;
            left_index++;
            continue;

         }

         /* the current header node is exhausted -- find the next one */

         /* move up if we're at the end of a node */

         if (left_index >= SET_HASH_SIZE) {

            /* there are not more elements, so break */

            if (left_work_hdr == left_root)
               break;

            /* otherwise move up */

            left_height++;
            left_index =
               left_work_hdr->s_ntype.s_intern.s_child_index + 1;
            left_work_hdr =
               left_work_hdr->s_ntype.s_intern.s_parent;
            continue;

         }

         /* skip over null nodes */

         if (left_work_hdr->s_child[left_index].s_header == NULL) {

            left_index++;
            continue;

         }

         /* otherwise drop down a level */

         left_work_hdr =
            left_work_hdr->s_child[left_index].s_header;
         left_index = 0;
         left_height--;

      }

      /* if we've exhausted a set break again */

      if (right_element == NULL)
         break;

      /*
       *  At this point we have an element we would like to find in
       *  the target.
       */

      right_work_hdr = right_root;

      /* get the element's hash code */

      work_hash_code = right_hash_code;

      /* descend the header tree until we get to a leaf */

      for (right_height = right_root->s_ntype.s_root.s_height;
           right_height;
           right_height--) {

         /* extract the element's index at this level */

         right_index = work_hash_code & SET_HASH_MASK;
         work_hash_code = work_hash_code >> SET_SHIFT_DIST;

         /* if we're missing a header record, we're done */

         if (right_work_hdr->s_child[right_index].s_header == NULL) {

            return NO;

         }
         else {

            right_work_hdr =
               right_work_hdr->s_child[right_index].s_header;

         }
      }

      /*
       *  At this point, right_work_hdr points to the lowest level
       *  header record.  The next problem is to determine if the element
       *  is in the set.  We compare the element with the clash list.
       */

      right_index = work_hash_code & SET_HASH_MASK;
      for (right_cell = right_work_hdr->s_child[right_index].s_cell;
           right_cell != NULL &&
              right_cell->s_hash_code < right_hash_code;
           right_cell = right_cell->s_next);

      /* check for a target element */

      is_equal = NO;
      for (;
           right_cell != NULL &&
              right_cell->s_hash_code == right_hash_code &&
              !is_equal;
           right_cell = right_cell->s_next) {

         spec_equal(is_equal,&(right_cell->s_spec),right_element);

      }

      /* if we found the element, continue */

      if (!is_equal)
         return NO;

   }

   return YES;

}

/*\
 *  \function{set\_pow()}
 *
 *  This function finds the power set of a source set.  It used as a last
 *  resort -- we much prefer to iterate over a power set, rather than
 *  forming the power set.  When we are forced to form a power set, the
 *  source set had better be very small, or we will run out of memory
 *  forming the power set.
\*/

void set_pow(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* power set                         */
   specifier *source)                  /* source set                        */

{
struct source_elem_item *se_array;     /* array of source elements          */
int se_array_length;                   /* length of above array             */
int se_index;                          /* index into above array            */
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type subset_root, subset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type subset_cell;            /* current cell pointer              */
set_c_ptr_type *subset_tail;           /* attach new cells here             */
int subset_height, subset_index;       /* current height and index          */
specifier *subset_element;             /* set element                       */
int32 subset_hash_code;                /* hash code of subset element       */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int i;                                 /* temporary looping variable        */

   source_root = source->sp_val.sp_set_ptr;

   /* create an array of set elements in the source set */

   se_array_length = (int)(source_root->s_ntype.s_root.s_cardinality);

   se_array = (struct source_elem_item *)malloc((size_t)
                    (se_array_length * sizeof(struct source_elem_item)));
   if (se_array == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   se_index = 0;

   /*
    *  Set up to loop over the source set, producing one leaf node at a
    *  time.
    */

   source_work_hdr = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_index = 0;

   /* loop over the nodes of source */

   for (;;) {

      /* descend to a leaf */

      while (source_height) {

         /* move down if possible */

         if (source_index < SET_HASH_SIZE) {

            /* skip over null nodes */

            if (source_work_hdr->s_child[source_index].s_header == NULL) {

               source_index++;
               continue;

            }

            /* we can move down, so do so */

            source_work_hdr =
               source_work_hdr->s_child[source_index].s_header;
            source_index = 0;
            source_height--;

            continue;

         }

         /* if there are no more elements, break */

         if (source_work_hdr == source_root) {

            source_work_hdr = NULL;
            break;

         }

         /* move up if we're at the end of a node */

         source_height++;
         source_index =
               source_work_hdr->s_ntype.s_intern.s_child_index + 1;
         source_work_hdr =
            source_work_hdr->s_ntype.s_intern.s_parent;

         continue;

      }

      /* break if we can't find a leaf */

      if (source_work_hdr == NULL)
         break;

      /*
       *  At this point we have a leaf in the header tree.  We loop over
       *  the elements in this leaf inserting each in the specifier
       *  array.
       */

      for (source_index = 0; source_index < SET_HASH_SIZE; source_index++) {

         for (source_cell = source_work_hdr->s_child[source_index].s_cell;
              source_cell != NULL;
              source_cell = source_cell->s_next) {

            se_array[se_index].se_element = source_cell;
            se_array[se_index].se_in_set = NO;
            se_index++;

         }
      }

      /* if the leaf is the root, we're done */

      if (source_work_hdr == source_root)
         break;

      /* set up to find the next leaf, by moving to the parent */

      source_height++;
      source_index =
            source_work_hdr->s_ntype.s_intern.s_child_index + 1;
      source_work_hdr =
         source_work_hdr->s_ntype.s_intern.s_parent;

   }

   /*
    *  Now we're done with the source set.  We have all its elements in
    *  an array, so it's easy to loop over this array forming subsets.
    */

   /* create a new set for the target */

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_hash_code = 0;
   target_root->s_ntype.s_root.s_cardinality = 0;
   target_root->s_ntype.s_root.s_height =
         max(0,se_array_length - SET_HASH_SIZE);
   for (i = 0;
        i < SET_HASH_SIZE;
        target_root->s_child[i++].s_cell = NULL);

   /* keep looping until we've produced all subsets */

   for (;;) {

      /* create a new set for the subset */

      get_set_header(subset_root);
      subset_root->s_use_count = 1;
      subset_root->s_hash_code = 0;
      subset_root->s_ntype.s_root.s_cardinality = 0;
      subset_root->s_ntype.s_root.s_height = 0;
      for (i = 0;
           i < SET_HASH_SIZE;
           subset_root->s_child[i++].s_cell = NULL);
      expansion_trigger = SET_HASH_SIZE * SET_CLASH_SIZE;

      /* loop over the array of cells, building up a subset */

      for (se_index = 0; se_index < se_array_length; se_index++) {

         if (!se_array[se_index].se_in_set)
            continue;

         subset_element = &(se_array[se_index].se_element->s_spec);
         subset_hash_code = se_array[se_index].se_element->s_hash_code;

         /*
          *  At this point we have an element we would like to insert into
          *  the subset.
          */

         subset_work_hdr = subset_root;

         /* get the element's hash code */

         work_hash_code = subset_hash_code;

         /* descend the header tree until we get to a leaf */

         subset_height = subset_root->s_ntype.s_root.s_height;
         while (subset_height--) {

            /* extract the element's index at this level */

            subset_index = work_hash_code & SET_HASH_MASK;
            work_hash_code = work_hash_code >> SET_SHIFT_DIST;

            /* if we're missing a header record, insert it */

            if (subset_work_hdr->s_child[subset_index].s_header == NULL) {

               get_set_header(new_hdr);
               new_hdr->s_ntype.s_intern.s_parent = subset_work_hdr;
               new_hdr->s_ntype.s_intern.s_child_index = subset_index;
               for (i = 0;
                    i < SET_HASH_SIZE;
                    new_hdr->s_child[i++].s_cell = NULL);
               subset_work_hdr->s_child[subset_index].s_header = new_hdr;
               subset_work_hdr = new_hdr;

            }
            else {

               subset_work_hdr =
                  subset_work_hdr->s_child[subset_index].s_header;

            }
         }

         /*
          *  Now subset_work_hdr points to the lowest level header node
          *  in the set.  We don't have to worry about duplicates, but we
          *  must find the position in the clash list where we would like
          *  to insert this element.
          */

         subset_index = work_hash_code & SET_HASH_MASK;
         subset_tail = &(subset_work_hdr->s_child[subset_index].s_cell);
         for (subset_cell = *subset_tail;
              subset_cell != NULL &&
                 subset_cell->s_hash_code < subset_hash_code;
              subset_cell = subset_cell->s_next) {

            subset_tail = &(subset_cell->s_next);

         }

         /* we insert it */

         get_set_cell(new_cell);
         mark_specifier(subset_element);
         new_cell->s_spec.sp_form = subset_element->sp_form;
         new_cell->s_spec.sp_val.sp_biggest =
            subset_element->sp_val.sp_biggest;
         new_cell->s_hash_code = subset_hash_code;
         new_cell->s_next = *subset_tail;
         *subset_tail = new_cell;
         subset_root->s_ntype.s_root.s_cardinality++;
         subset_root->s_hash_code ^= subset_hash_code;

         /* expand the set header if necessary */

         if (subset_root->s_ntype.s_root.s_cardinality > expansion_trigger) {

            subset_root = set_expand_header(SETL_SYSTEM subset_root);
            expansion_trigger *= SET_HASH_SIZE;

         }
      }

      /*
       *  At this point we've finished building a subset, and must insert
       *  it into the target set.
       */

      target_hash_code = subset_root->s_hash_code;
      target_work_hdr = target_root;

      /* get the element's hash code */

      work_hash_code = target_hash_code;

      /* descend the header tree until we get to a leaf */

      target_height = target_root->s_ntype.s_root.s_height;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & SET_HASH_MASK;
         work_hash_code = work_hash_code >> SET_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (target_work_hdr->s_child[target_index].s_header == NULL) {

            get_set_header(new_hdr);
            new_hdr->s_ntype.s_intern.s_parent = target_work_hdr;
            new_hdr->s_ntype.s_intern.s_child_index = target_index;
            for (i = 0;
                 i < SET_HASH_SIZE;
                 new_hdr->s_child[i++].s_cell = NULL);
            target_work_hdr->s_child[target_index].s_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->s_child[target_index].s_header;

         }
      }

     /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  We find the position in the clash list in which we
       *  would like to insert the subset.
       */

      target_index = work_hash_code & SET_HASH_MASK;
      target_tail = &(target_work_hdr->s_child[target_index].s_cell);
      for (target_cell = *target_tail;
           target_cell != NULL &&
              target_cell->s_hash_code < target_hash_code;
           target_cell = target_cell->s_next) {

         target_tail = &(target_cell->s_next);

      }

      /* we insert it */

      get_set_cell(new_cell);
      new_cell->s_spec.sp_form = ft_set;
      new_cell->s_spec.sp_val.sp_set_ptr = subset_root;
      new_cell->s_hash_code = target_hash_code;
      new_cell->s_next = *target_tail;
      *target_tail = new_cell;
      target_root->s_ntype.s_root.s_cardinality++;
      target_root->s_hash_code ^= target_hash_code;

      /*
       *  We treat the field se_in_set as a single bit in a binary number
       *  representing a subset.  We effectively add one to this binary
       *  number.
       */

      for (se_index = 0;
           se_index < se_array_length && se_array[se_index].se_in_set;
           se_array[se_index++].se_in_set = NO);

      if (se_index >= se_array_length)
         break;

      se_array[se_index].se_in_set = YES;

   }

   /* finally, set the target value */

   free(se_array);
   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = target_root;

   return;

}

/*\
 *  \function{set\_npow()}
 *
 *  This function finds the power set of a source set.  It used as a last
 *  resort -- we much prefer to iterate over a power set, rather than
 *  forming the power set.  When we are forced to form a power set, the
 *  source set had better be very small, or we will run out of memory
 *  forming the power set.
\*/

void set_npow(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* power set                         */
   specifier *source,                  /* source set                        */
   int32 n)                            /* size of each subset               */

{
struct source_elem_item *se_array;     /* array of source elements          */
int32 se_array_length;                 /* length of above array             */
int32 se_index;                        /* index into above array            */
int32 se_right_no;                     /* position of rightmost zero        */
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type subset_root, subset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type subset_cell;            /* current cell pointer              */
set_c_ptr_type *subset_tail;           /* attach new cells here             */
int subset_height, subset_index;       /* current height and index          */
specifier *subset_element;             /* set element                       */
int32 subset_hash_code;                /* hash code of subset element       */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int i;                                 /* temporary looping variable        */

   source_root = source->sp_val.sp_set_ptr;

   /* create an array of set elements in the source set */

   se_array_length = source_root->s_ntype.s_root.s_cardinality;
   se_array = (struct source_elem_item *)malloc((size_t)
                    (se_array_length * sizeof(struct source_elem_item)));
   if (se_array == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   se_index = 0;

   /*
    *  Set up to loop over the source set, producing one leaf node at a
    *  time.
    */

   source_work_hdr = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_index = 0;

   /* loop over the nodes of source */

   for (;;) {

      /* descend to a leaf */

      while (source_height) {

         /* move down if possible */

         if (source_index < SET_HASH_SIZE) {

            /* skip over null nodes */

            if (source_work_hdr->s_child[source_index].s_header == NULL) {

               source_index++;
               continue;

            }

            /* we can move down, so do so */

            source_work_hdr =
               source_work_hdr->s_child[source_index].s_header;
            source_index = 0;
            source_height--;

            continue;

         }

         /* if there are no more elements, break */

         if (source_work_hdr == source_root) {

            source_work_hdr = NULL;
            break;

         }

         /* move up if we're at the end of a node */

         source_height++;
         source_index =
               source_work_hdr->s_ntype.s_intern.s_child_index + 1;
         source_work_hdr =
            source_work_hdr->s_ntype.s_intern.s_parent;

         continue;

      }

      /* break if we can't find a leaf */

      if (source_work_hdr == NULL)
         break;

      /*
       *  At this point we have a leaf in the header tree.  We loop over
       *  the elements in this leaf inserting each in the specifier
       *  array.
       */

      for (source_index = 0; source_index < SET_HASH_SIZE; source_index++) {

         for (source_cell = source_work_hdr->s_child[source_index].s_cell;
              source_cell != NULL;
              source_cell = source_cell->s_next) {

            se_array[se_index].se_element = source_cell;
            se_array[se_index].se_in_set = NO;
            se_index++;

         }
      }

      /* if the leaf is the root, we're done */

      if (source_work_hdr == source_root)
         break;

      /* set up to find the next leaf, by moving to the parent */

      source_height++;
      source_index =
            source_work_hdr->s_ntype.s_intern.s_child_index + 1;
      source_work_hdr =
         source_work_hdr->s_ntype.s_intern.s_parent;

   }

   /* initially the first n elements are in the set */

   for (se_index = 0;
        se_index < se_array_length && se_index < n;
        se_array[se_index++].se_in_set = YES);

   /*
    *  Now we're done with the source set.  We have all its elements in
    *  an array, so it's easy to loop over this array forming subsets.
    */

   /* create a new set for the target */

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_hash_code = 0;
   target_root->s_ntype.s_root.s_cardinality = 0;
   target_root->s_ntype.s_root.s_height =
         max(0,se_array_length - SET_HASH_SIZE);
   for (i = 0;
        i < SET_HASH_SIZE;
        target_root->s_child[i++].s_cell = NULL);

   /* keep looping until we've produced all subsets */

   for (;;) {

      /*
       *  if we are asked to produce sets larger than the base, return an
       *  empty set
       */

      if (n > se_array_length)
         break;

      /* create a new set for the subset */

      get_set_header(subset_root);
      subset_root->s_use_count = 1;
      subset_root->s_hash_code = 0;
      subset_root->s_ntype.s_root.s_cardinality = 0;
      subset_root->s_ntype.s_root.s_height = 0;
      for (i = 0;
           i < SET_HASH_SIZE;
           subset_root->s_child[i++].s_cell = NULL);
      expansion_trigger = SET_HASH_SIZE * SET_CLASH_SIZE;

      /* loop over the array of cells, building up a subset */

      for (se_index = 0; se_index < se_array_length; se_index++) {

         if (!se_array[se_index].se_in_set)
            continue;

         subset_element = &(se_array[se_index].se_element->s_spec);
         subset_hash_code = se_array[se_index].se_element->s_hash_code;

         /*
          *  At this point we have an element we would like to insert into
          *  the subset.
          */

         subset_work_hdr = subset_root;

         /* get the element's hash code */

         work_hash_code = subset_hash_code;

         /* descend the header tree until we get to a leaf */

         subset_height = subset_root->s_ntype.s_root.s_height;
         while (subset_height--) {

            /* extract the element's index at this level */

            subset_index = work_hash_code & SET_HASH_MASK;
            work_hash_code = work_hash_code >> SET_SHIFT_DIST;

            /* if we're missing a header record, insert it */

            if (subset_work_hdr->s_child[subset_index].s_header == NULL) {

               get_set_header(new_hdr);
               new_hdr->s_ntype.s_intern.s_parent = subset_work_hdr;
               new_hdr->s_ntype.s_intern.s_child_index = subset_index;
               for (i = 0;
                    i < SET_HASH_SIZE;
                    new_hdr->s_child[i++].s_cell = NULL);
               subset_work_hdr->s_child[subset_index].s_header = new_hdr;
               subset_work_hdr = new_hdr;

            }
            else {

               subset_work_hdr =
                  subset_work_hdr->s_child[subset_index].s_header;

            }
         }

         /*
          *  Now subset_work_hdr points to the lowest level header node
          *  in the set.  We don't have to worry about duplicates, but we
          *  must find the position in the clash list where we would like
          *  to insert this element.
          */

         subset_index = work_hash_code & SET_HASH_MASK;
         subset_tail = &(subset_work_hdr->s_child[subset_index].s_cell);
         for (subset_cell = *subset_tail;
              subset_cell != NULL &&
                 subset_cell->s_hash_code < subset_hash_code;
              subset_cell = subset_cell->s_next) {

            subset_tail = &(subset_cell->s_next);

         }

         /* we insert it */

         get_set_cell(new_cell);
         mark_specifier(subset_element);
         new_cell->s_spec.sp_form = subset_element->sp_form;
         new_cell->s_spec.sp_val.sp_biggest =
            subset_element->sp_val.sp_biggest;
         new_cell->s_hash_code = subset_hash_code;
         new_cell->s_next = *subset_tail;
         *subset_tail = new_cell;
         subset_root->s_ntype.s_root.s_cardinality++;
         subset_root->s_hash_code ^= subset_hash_code;

         /* expand the set header if necessary */

         if (subset_root->s_ntype.s_root.s_cardinality > expansion_trigger) {

            subset_root = set_expand_header(SETL_SYSTEM subset_root);
            expansion_trigger *= SET_HASH_SIZE;

         }
      }

      /*
       *  At this point we've finished building a subset, and must insert
       *  it into the target set.
       */

      target_hash_code = subset_root->s_hash_code;
      target_work_hdr = target_root;

      /* get the element's hash code */

      work_hash_code = target_hash_code;

      /* descend the header tree until we get to a leaf */

      target_height = target_root->s_ntype.s_root.s_height;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & SET_HASH_MASK;
         work_hash_code = work_hash_code >> SET_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (target_work_hdr->s_child[target_index].s_header == NULL) {

            get_set_header(new_hdr);
            new_hdr->s_ntype.s_intern.s_parent = target_work_hdr;
            new_hdr->s_ntype.s_intern.s_child_index = target_index;
            for (i = 0;
                 i < SET_HASH_SIZE;
                 new_hdr->s_child[i++].s_cell = NULL);
            target_work_hdr->s_child[target_index].s_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->s_child[target_index].s_header;

         }
      }

     /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  We find the position in the clash list in which we
       *  would like to insert the subset.
       */

      target_index = work_hash_code & SET_HASH_MASK;
      target_tail = &(target_work_hdr->s_child[target_index].s_cell);
      for (target_cell = *target_tail;
           target_cell != NULL &&
              target_cell->s_hash_code < target_hash_code;
           target_cell = target_cell->s_next) {

         target_tail = &(target_cell->s_next);

      }

      /* we insert it */

      get_set_cell(new_cell);
      new_cell->s_spec.sp_form = ft_set;
      new_cell->s_spec.sp_val.sp_set_ptr = subset_root;
      new_cell->s_hash_code = target_hash_code;
      new_cell->s_next = *target_tail;
      *target_tail = new_cell;
      target_root->s_ntype.s_root.s_cardinality++;
      target_root->s_hash_code ^= target_hash_code;

      /*
       *  We treat the field se_in_set as a single bit in a binary number
       *  representing a subset.  We find the next binary number with n
       *  bits.
       */

      /* find the right-most NO */

      for (se_index = se_array_length - 1;
           se_index >= 0 && se_array[se_index].se_in_set;
           se_index--);

      se_right_no = se_index + 1;

      /* find the next YES */

      for (;
           se_index >= 0 && !se_array[se_index].se_in_set;
           se_index--);

      if (se_index < 0)
         break;

      se_array[se_index++].se_in_set = NO;
      se_array[se_index++].se_in_set = YES;

      while (se_right_no < se_array_length) {

         se_array[se_right_no++].se_in_set = NO;
         se_array[se_index++].se_in_set = YES;

      }
   }

   /* finally, set the target value */

   free(se_array);
   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = target_root;

   return;

}

/*\
 *  \function{set\_arb()}
 *
 *  This function finds the power set of a source set.  It used as a last
 *  resort -- we much prefer to iterate over a power set, rather than
 *  forming the power set.  When we are forced to form a power set, the
 *  source set had better be very small, or we will run out of memory
 *  forming the power set.
\*/

void set_arb(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* power set                         */
   specifier *source)                  /* source set                        */

{
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */

   source_root = source->sp_val.sp_set_ptr;

   /*
    *  We loop over the source set looking for an element.
    */

   source_work_hdr = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_cell = NULL;
   source_index = 0;

   /* find the next element in the set */

   for (;;) {

      /* if we have an element already, break */

      if (source_cell != NULL) {

         mark_specifier(&(source_cell->s_spec));
         unmark_specifier(target);
         target->sp_form = source_cell->s_spec.sp_form;
         target->sp_val.sp_biggest =
            source_cell->s_spec.sp_val.sp_biggest;

         return;

      }

      /* start on the next clash list, if we're at a leaf */

      if (!source_height && source_index < SET_HASH_SIZE) {

         source_cell = source_work_hdr->s_child[source_index].s_cell;
         source_index++;
         continue;

      }

      /* the current header node is exhausted -- find the next one */

      /* move up if we're at the end of a node */

      if (source_index >= SET_HASH_SIZE) {

         /* there are no more elements, so return omega */

         if (source_work_hdr == source_root) {

            unmark_specifier(target);
            target->sp_form = ft_omega;

            return;

         }

         /* otherwise move up */

         source_height++;
         source_index =
            source_work_hdr->s_ntype.s_intern.s_child_index + 1;
         source_work_hdr =
            source_work_hdr->s_ntype.s_intern.s_parent;
         continue;

      }

      /* skip over null nodes */

      if (source_work_hdr->s_child[source_index].s_header == NULL) {

         source_index++;
         continue;

      }

      /* otherwise drop down a level */

      source_work_hdr =
         source_work_hdr->s_child[source_index].s_header;
      source_index = 0;
      source_height--;

   }
}

/*\
 *  \function{set\_from()}
 *
 *  This functions implements the SETL2 \verb"FROM" operation.  Notice
 *  that all three operands may be modified.
\*/

void set_from(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
int target_height, target_index;       /* current height and index          */
int32 contraction_trigger;             /* cardinality which triggers        */
                                       /* header contraction                */
int i;                                 /* temporary looping variable        */

   /* if the source set is empty, return omega */

   target_root = right->sp_val.sp_set_ptr;
   if (target_root->s_ntype.s_root.s_cardinality == 0) {

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
       (right->sp_val.sp_set_ptr)->s_use_count != 1) {

      target_root = copy_set(SETL_SYSTEM right->sp_val.sp_set_ptr);

   }
   else {

      right->sp_form = ft_omega;

   }

   /*
    *  We loop over the target set looking for an element.
    */

   target_work_hdr = target_root;
   target_height = target_root->s_ntype.s_root.s_height;
   target_cell = NULL;
   target_index = 0;

   /* find the next element in the set */

   while (target_cell == NULL) {

      /* start on the next clash list, if we're at a leaf */

      if (!target_height && target_index < SET_HASH_SIZE) {

         target_cell = target_work_hdr->s_child[target_index].s_cell;
         target_index++;

         continue;

      }

      /* move up if we're at the end of a node */

      if (target_index >= SET_HASH_SIZE) {

#ifdef TRAPS

         /* there are must be elements in the set */

         if (target_work_hdr == target_root)
            trap(__FILE__,__LINE__,msg_corrupted_set);

#endif

         /* otherwise move up */

         target_height++;
         target_index =
            target_work_hdr->s_ntype.s_intern.s_child_index + 1;
         target_work_hdr =
            target_work_hdr->s_ntype.s_intern.s_parent;

         continue;

      }

      /* skip over null nodes */

      if (target_work_hdr->s_child[target_index].s_header == NULL) {

         target_index++;
         continue;

      }

      /* otherwise drop down a level */

      target_work_hdr =
         target_work_hdr->s_child[target_index].s_header;
      target_index = 0;
      target_height--;

   }

   /*
    *  At this point we have an element we must delete from the source.
    *  Note two things: we've already handled the case in which the
    *  source is empty, and since we remove an element from the set and
    *  set a target to that element, we can skip the unmarking and
    *  remarking of that element.
    */

   unmark_specifier(left);
   left->sp_form = target_cell->s_spec.sp_form;
   left->sp_val.sp_biggest = target_cell->s_spec.sp_val.sp_biggest;

   target_work_hdr->s_child[target_index - 1].s_cell = target_cell->s_next;
   target_root->s_ntype.s_root.s_cardinality--;
   target_root->s_hash_code ^= target_cell->s_hash_code;
   free_set_cell(target_cell);

   /* delete any null headers on this subtree */

   for (;;) {

      if (target_work_hdr == target_root)
         break;

      for (i = 0;
           i < SET_HASH_SIZE &&
              target_work_hdr->s_child[i].s_header == NULL;
           i++);

      if (i < SET_HASH_SIZE)
         break;

      target_index =
         target_work_hdr->s_ntype.s_intern.s_child_index + 1;
      target_work_hdr =
         target_work_hdr->s_ntype.s_intern.s_parent;

   }

   /* we may have to reduce the height of the set */

   contraction_trigger = (1 << ((target_root->s_ntype.s_root.s_height)
                                 * SET_SHIFT_DIST));
   if (contraction_trigger == 1)
      contraction_trigger = 0;

   if (target_root->s_ntype.s_root.s_cardinality <
       contraction_trigger) {

      target_root = set_contract_header(SETL_SYSTEM target_root);

   }

   /* set the other target values */

   unmark_specifier(right);
   right->sp_form = ft_set;
   right->sp_val.sp_set_ptr = target_root;

   if (target != NULL) {

      mark_specifier(left);
      unmark_specifier(target);
      target->sp_form = left->sp_form;
      target->sp_val.sp_biggest = left->sp_val.sp_biggest;

   }

   return;

}

