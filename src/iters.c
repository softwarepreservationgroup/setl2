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
 *  \package{Iterators}
 *
 *  This package contains definitions of the structures used to implement
 *  SETL2 iterators, and several low level functions to manipulate those
 *  structures.  We freely confess that we used a very ugly, non-portable
 *  C coding style here, in hopes of getting a fast implementation.  We
 *  have tried to isolate this ugliness to the macros and functions which
 *  allocate and release nodes.  In particular, there are some unsafe
 *  castes, so please make sure this file is compiled with unsafe
 *  optimizations disabled!
 *
 *  \texify{iters.h}
 *
 *  \packagebody{Iterators}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "mcode.h"                     /* method codes                      */
#include "builtins.h"                  /* built-in symbols                  */
#include "unittab.h"                   /* unit table                        */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_strngs.h"                  /* strings                           */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "objects.h"                   /* objects                           */
#include "slots.h"                     /* procedures                        */
#include "iters.h"                     /* iterators                         */


/* performance tuning constants */

#define ITER_BLOCK_SIZE        50      /* iterator block size               */

/*\
 *  \function{alloc\_iterators()}
 *
 *  This function allocates a block of iterators and links them together
 *  into a free list.  Note carefully the castes used here: we caste
 *  iterator items to pointers to iterator items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the iterator
 *  node.  It will work provided a iterator item is larger than a
 *  pointer, which is the case.
\*/

void alloc_iterators(
   SETL_SYSTEM_PROTO_VOID)

{
iter_ptr_type new_block;               /* first iterator in allocated block */

   /* allocate a new block */

   new_block = (iter_ptr_type)malloc((size_t)
         (ITER_BLOCK_SIZE * sizeof(struct iter_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (ITER_NEXT_FREE = new_block;
        ITER_NEXT_FREE < new_block + ITER_BLOCK_SIZE - 1;
        ITER_NEXT_FREE++) {

      *((iter_ptr_type *)ITER_NEXT_FREE) = ITER_NEXT_FREE + 1;

   }

   *((iter_ptr_type *)ITER_NEXT_FREE) = NULL;

   /* set next free node to new block */

   ITER_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{start\_set\_iterator()}
 *
 *  This function initializes an iterator for iteration over a set.  All
 *  we do is initialize an iterator item.
\*/

void start_set_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source set                        */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_set;

   mark_specifier(left);

   iter_ptr->it_itype.it_setiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_setiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_setiter.it_source_work_hdr =
         left->sp_val.sp_set_ptr;
   iter_ptr->it_itype.it_setiter.it_source_cell = NULL;
   iter_ptr->it_itype.it_setiter.it_source_height =
         (left->sp_val.sp_set_ptr)->s_ntype.s_root.s_height;
   iter_ptr->it_itype.it_setiter.it_source_index = 0;

   /* set the target value and return */

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   return;

}

/*\
 *  \function{set\_iterator\_next()}
 *
 *  This function picks out the next item in an iteration over a set.
 *  The algorithm is the same as those in the set package, except that we
 *  use the iterator record to keep track of the position in the set.
\*/

int set_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* next element in set               */
   specifier *left)                    /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   source_root =
         iter_ptr->it_itype.it_setiter.it_spec.sp_val.sp_set_ptr;
   source_work_hdr = iter_ptr->it_itype.it_setiter.it_source_work_hdr;
   source_cell = iter_ptr->it_itype.it_setiter.it_source_cell;
   source_height = iter_ptr->it_itype.it_setiter.it_source_height;
   source_index = iter_ptr->it_itype.it_setiter.it_source_index;

   /* loop until we explicitly return */

   for (;;) {

      /* if we have an element already, return it */

      if (source_cell != NULL) {

         mark_specifier(&(source_cell->s_spec));
         unmark_specifier(target);
         target->sp_form = source_cell->s_spec.sp_form;
         target->sp_val.sp_biggest =
               source_cell->s_spec.sp_val.sp_biggest;

         source_cell = source_cell->s_next;

         iter_ptr->it_itype.it_setiter.it_source_work_hdr =
               source_work_hdr;
         iter_ptr->it_itype.it_setiter.it_source_cell = source_cell;
         iter_ptr->it_itype.it_setiter.it_source_height = source_height;
         iter_ptr->it_itype.it_setiter.it_source_index = source_index;

         return YES;

      }

      /* start on the next clash list, if we're at a leaf */

      if (!source_height && source_index < SET_HASH_SIZE) {

         source_cell = source_work_hdr->s_child[source_index].s_cell;
         source_index++;
         continue;

      }

      /* move up if we're at the end of a node */

      if (source_index >= SET_HASH_SIZE) {

         /* if we return to the root the set is exhausted */

         if (source_work_hdr == source_root) {

            unmark_specifier(target);
            target->sp_form = ft_omega;
            return NO;

         }

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
 *  \function{start\_map\_iterator()}
 *
 *  This function initializes an iterator for iteration over a map.  All
 *  we do is initialize an iterator item.
\*/

void start_map_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source map                        */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_map;

   mark_specifier(left);

   iter_ptr->it_itype.it_mapiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_mapiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_mapiter.it_source_work_hdr =
         left->sp_val.sp_map_ptr;
   iter_ptr->it_itype.it_mapiter.it_source_cell = NULL;
   iter_ptr->it_itype.it_mapiter.it_source_height =
         (left->sp_val.sp_map_ptr)->m_ntype.m_root.m_height;
   iter_ptr->it_itype.it_mapiter.it_source_index = 0;
   iter_ptr->it_itype.it_mapiter.it_valset_root = NULL;

   /* set the target value and return */

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   return;

}

/*\
 *  \function{map\_iterator\_next()}
 *
 *  This function picks out the next item in an iteration over a map.
 *  We produce the next pair, using the same algorithm we use to convert
 *  maps to sets.
\*/

int map_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* next element in set               */
   specifier *source)                  /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
tuple_h_ptr_type tuple_root;           /* tuple header                      */
tuple_c_ptr_type tuple_cell;           /* tuple cell node                   */
set_h_ptr_type valset_root, valset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type valset_cell;            /* current cell pointer              */
int valset_height, valset_index;       /* current height and index          */
int i;                                 /* temporary looping variable        */

   /* reload the iteration variables */

   iter_ptr = source->sp_val.sp_iter_ptr;
   source_root = iter_ptr->it_itype.it_mapiter.it_spec.sp_val.sp_map_ptr;
   source_work_hdr = iter_ptr->it_itype.it_mapiter.it_source_work_hdr;
   source_cell = iter_ptr->it_itype.it_mapiter.it_source_cell;
   source_height = iter_ptr->it_itype.it_mapiter.it_source_height;
   source_index = iter_ptr->it_itype.it_mapiter.it_source_index;
   valset_root = iter_ptr->it_itype.it_mapiter.it_valset_root;
   valset_work_hdr = iter_ptr->it_itype.it_mapiter.it_valset_work_hdr;
   valset_cell = iter_ptr->it_itype.it_mapiter.it_valset_cell;
   valset_height = iter_ptr->it_itype.it_mapiter.it_valset_height;
   valset_index = iter_ptr->it_itype.it_mapiter.it_valset_index;

   /* loop until we explicitly return */

   for (;;) {

      /* find the next element in the map */

      while (source_cell == NULL) {

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < MAP_HASH_SIZE) {

            source_cell = source_work_hdr->m_child[source_index].m_cell;
            source_index++;

            continue;

         }

         /* move up if we're at the end of a node */

         if (source_index >= MAP_HASH_SIZE) {

            /* there are no more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->m_ntype.m_intern.m_child_index + 1;
            source_work_hdr =
               source_work_hdr->m_ntype.m_intern.m_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->m_child[source_index].m_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->m_child[source_index].m_header;
         source_index = 0;
         source_height--;

      }

      /* if the map is empty, return NO */

      if (source_cell == NULL) {

         unmark_specifier(target);
         target->sp_form = ft_omega;

         return NO;

      }

      /* save our location in the map */

      iter_ptr->it_itype.it_mapiter.it_source_work_hdr = source_work_hdr;
      iter_ptr->it_itype.it_mapiter.it_source_cell = source_cell;
      iter_ptr->it_itype.it_mapiter.it_source_height = source_height;
      iter_ptr->it_itype.it_mapiter.it_source_index = source_index;
      iter_ptr->it_itype.it_mapiter.it_valset_root = valset_root;

      /* if we're not at a multi-value cell, return the pair */

      if (!(source_cell->m_is_multi_val)) {

         /*
          *  At this point we have a pair from the map, and the hash code
          *  of the domain element.  We have to form a tuple from this
          *  pair.
          */

         get_tuple_header(tuple_root);
         tuple_root->t_use_count = 1;
         tuple_root->t_hash_code = 0;
         tuple_root->t_ntype.t_root.t_length = 2;
         tuple_root->t_ntype.t_root.t_height = 0;
         for (i = 2;
              i < TUP_HEADER_SIZE;
              tuple_root->t_child[i++].t_cell = NULL);

         /* insert domain element */

         get_tuple_cell(tuple_cell);
         mark_specifier(&(source_cell->m_domain_spec));
         tuple_cell->t_spec.sp_form = source_cell->m_domain_spec.sp_form;
         tuple_cell->t_spec.sp_val.sp_biggest =
               source_cell->m_domain_spec.sp_val.sp_biggest;

         tuple_cell->t_hash_code = source_cell->m_hash_code;
         tuple_root->t_hash_code ^= source_cell->m_hash_code;
         tuple_root->t_child[0].t_cell = tuple_cell;

         /* insert range element */

         get_tuple_cell(tuple_cell);
         mark_specifier(&(source_cell->m_range_spec));
         tuple_cell->t_spec.sp_form = source_cell->m_range_spec.sp_form;
         tuple_cell->t_spec.sp_val.sp_biggest =
               source_cell->m_range_spec.sp_val.sp_biggest;

         spec_hash_code(tuple_cell->t_hash_code,&(source_cell->m_range_spec));
         tuple_root->t_hash_code ^=
               tuple_cell->t_hash_code;
         tuple_root->t_child[1].t_cell = tuple_cell;

         unmark_specifier(target);
         target->sp_form = ft_tuple;
         target->sp_val.sp_tuple_ptr = tuple_root;

         iter_ptr->it_itype.it_mapiter.it_source_cell = source_cell->m_next;

         return YES;

      }

      /* otherwise we find the next item in the multi-value set */

      if (valset_root == NULL) {

         valset_root = source_cell->m_range_spec.sp_val.sp_set_ptr;
         valset_work_hdr = valset_root;
         valset_height = valset_root->s_ntype.s_root.s_height;
         valset_cell = NULL;
         valset_index = 0;

      }

      for (;;) {

         /* if we have an element, return it */

         if (valset_cell != NULL) {

            /*
             *  At this point we have a pair from the map, and the hash
             *  code of the domain element.  We have to form a tuple from
             *  this pair.
             */

            get_tuple_header(tuple_root);
            tuple_root->t_use_count = 1;
            tuple_root->t_hash_code = 0;
            tuple_root->t_ntype.t_root.t_length = 2;
            tuple_root->t_ntype.t_root.t_height = 0;
            for (i = 2;
                 i < TUP_HEADER_SIZE;
                 tuple_root->t_child[i++].t_cell = NULL);

            /* insert domain element */

            get_tuple_cell(tuple_cell);
            mark_specifier(&(source_cell->m_domain_spec));
            tuple_cell->t_spec.sp_form = source_cell->m_domain_spec.sp_form;
            tuple_cell->t_spec.sp_val.sp_biggest =
                  source_cell->m_domain_spec.sp_val.sp_biggest;

            tuple_cell->t_hash_code = source_cell->m_hash_code;
            tuple_root->t_hash_code ^=
                  source_cell->m_hash_code;
            tuple_root->t_child[0].t_cell = tuple_cell;

            /* insert range element */

            get_tuple_cell(tuple_cell);
            mark_specifier(&(valset_cell->s_spec));
            tuple_cell->t_spec.sp_form = valset_cell->s_spec.sp_form;
            tuple_cell->t_spec.sp_val.sp_biggest =
                  valset_cell->s_spec.sp_val.sp_biggest;

            tuple_cell->t_hash_code = valset_cell->s_hash_code;
            tuple_root->t_hash_code ^=
                  tuple_cell->t_hash_code;
            tuple_root->t_child[1].t_cell = tuple_cell;

            unmark_specifier(target);
            target->sp_form = ft_tuple;
            target->sp_val.sp_tuple_ptr = tuple_root;

            iter_ptr->it_itype.it_mapiter.it_valset_root = valset_root;
            iter_ptr->it_itype.it_mapiter.it_valset_work_hdr =
                  valset_work_hdr;
            iter_ptr->it_itype.it_mapiter.it_valset_cell =
                  valset_cell->s_next;
            iter_ptr->it_itype.it_mapiter.it_valset_height = valset_height;
            iter_ptr->it_itype.it_mapiter.it_valset_index = valset_index;

            return YES;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!valset_height && valset_index < SET_HASH_SIZE) {

            valset_cell = valset_work_hdr->s_child[valset_index].s_cell;
            valset_index++;

            continue;

         }

         /* move up if we're at the end of a node */

         if (valset_index >= SET_HASH_SIZE) {

            /* there are no more elements, so break */

            if (valset_work_hdr == valset_root)
               break;

            /* otherwise move up */

            valset_height++;
            valset_index =
               valset_work_hdr->s_ntype.s_intern.s_child_index + 1;
            valset_work_hdr =
               valset_work_hdr->s_ntype.s_intern.s_parent;

            continue;

         }

         /* skip over null nodes */

         if (valset_work_hdr->s_child[valset_index].s_header == NULL) {

            valset_index++;
            continue;

         }

         /* otherwise drop down a level */

         valset_work_hdr =
            valset_work_hdr->s_child[valset_index].s_header;
         valset_index = 0;
         valset_height--;

      }

      source_cell = source_cell->m_next;
      valset_root = NULL;

   }
}

/*\
 *  \function{start\_tuple\_iterator()}
 *
 *  This function initializes an iterator for iteration over a tuple. All
 *  we do is initialize an iterator item.
\*/

void start_tuple_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source tuple                      */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_tuple;

   mark_specifier(left);

   iter_ptr->it_itype.it_tupiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_tupiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_tupiter.it_source_number = 0;

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

}

/*\
 *  \function{tuple\_iterator\_next()}
 *
 *  This function returns the next tuple element in an iteration over a
 *  tuple.
\*/

int tuple_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* next element in tuple             */
   specifier *left)                    /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   source_root = iter_ptr->it_itype.it_tupiter.it_spec.sp_val.sp_tuple_ptr;
   source_number = iter_ptr->it_itype.it_tupiter.it_source_number;

   /* check whether the tuple is finished */

   if (source_number >= source_root->t_ntype.t_root.t_length) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return NO;

   }

   iter_ptr->it_itype.it_tupiter.it_source_number++;

   /* descend the header tree until we get to a leaf */

   source_work_hdr = source_root;
   for (source_height = source_root->t_ntype.t_root.t_height;
        source_height;
        source_height--) {

      /* extract the element's index at this level */

      source_index = (source_number >>
                           (source_height * TUP_SHIFT_DIST)) &
                        TUP_SHIFT_MASK;

      /* if we're missing a header record, the element is om */

      if (source_work_hdr->t_child[source_index].t_header == NULL) {

         unmark_specifier(target);
         target->sp_form = ft_omega;

         return YES;

      }

      /* otherwise drop down a level */

      source_work_hdr = source_work_hdr->t_child[source_index].t_header;

   }

   /*
    *  At this point, source_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   source_index = source_number & TUP_SHIFT_MASK;
   if (source_work_hdr->t_child[source_index].t_cell == NULL) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return YES;

   }

   /* we do have an item to return */

   source_cell = source_work_hdr->t_child[source_index].t_cell;

   mark_specifier(&(source_cell->t_spec));
   unmark_specifier(target);
   target->sp_form = source_cell->t_spec.sp_form;
   target->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;

   return YES;

}

/*\
 *  \function{start\_string\_iterator()}
 *
 *  This function initializes an iterator for iteration over a string. All
 *  we do is initialize an iterator item.
\*/

void start_string_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source string                     */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_string;

   mark_specifier(left);

   iter_ptr->it_itype.it_striter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_striter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_striter.it_string_cell =
         (left->sp_val.sp_string_ptr)->s_head;
   iter_ptr->it_itype.it_striter.it_string_index = 0;
   iter_ptr->it_itype.it_striter.it_char_number = 0;

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

}

/*\
 *  \function{string\_iterator\_next()}
 *
 *  This function returns the next string element in an iteration over a
 *  string.
\*/

int string_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* next element in string            */
   specifier *left)                    /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
string_h_ptr_type string_hdr;          /* string header node                */
string_c_ptr_type string_cell;         /* current cell pointer              */
int string_index;                      /* current index                     */
string_h_ptr_type new_hdr;             /* created string header             */
string_c_ptr_type new_cell;            /* created string cell               */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   string_hdr = iter_ptr->it_itype.it_striter.it_spec.sp_val.sp_string_ptr;

   /* if there are no characters left we're done */

   if (iter_ptr->it_itype.it_striter.it_char_number >=
       string_hdr->s_length) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return NO;

   }

   /* set up for the next character */

   iter_ptr->it_itype.it_striter.it_char_number++;

   string_cell = iter_ptr->it_itype.it_striter.it_string_cell;
   string_index = iter_ptr->it_itype.it_striter.it_string_index;

   if (string_index == STR_CELL_WIDTH) {

      iter_ptr->it_itype.it_striter.it_string_cell =
         string_cell = string_cell->s_next;
      iter_ptr->it_itype.it_striter.it_string_index = string_index = 0;

   }

   iter_ptr->it_itype.it_striter.it_string_index++;

   /* create a one character string */

   get_string_header(new_hdr);
   new_hdr->s_use_count = 1;
   new_hdr->s_hash_code = -1;
   new_hdr->s_length = 1;

   get_string_cell(new_cell);
   new_hdr->s_head = new_hdr->s_tail = new_cell;
   new_cell->s_next = NULL;
   new_cell->s_prev = NULL;
   new_cell->s_cell_value[0] =
      string_cell->s_cell_value[string_index];

   /* assign it to the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = new_hdr;

   return YES;

}

/*\
 *  \function{start\_domain\_iterator()}
 *
 *  This function initializes an iterator for iteration over the domain
 *  of a map.  All we do is initialize an iterator item.
\*/

void start_domain_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source map                        */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_domain;

   mark_specifier(left);

   iter_ptr->it_itype.it_mapiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_mapiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_mapiter.it_source_work_hdr =
         left->sp_val.sp_map_ptr;
   iter_ptr->it_itype.it_mapiter.it_source_cell = NULL;
   iter_ptr->it_itype.it_mapiter.it_source_height =
         (left->sp_val.sp_map_ptr)->m_ntype.m_root.m_height;
   iter_ptr->it_itype.it_mapiter.it_source_index = 0;

   /* set the target value and return */

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   return;

}

/*\
 *  \function{domain\_iterator\_next()}
 *
 *  This function picks out the next item in an iteration over a map
 *  domain.  We produce the next domain element.
\*/

int domain_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* next element in set               */
   specifier *source)                  /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */

   /* reload the iteration variables */

   iter_ptr = source->sp_val.sp_iter_ptr;
   source_root = iter_ptr->it_itype.it_mapiter.it_spec.sp_val.sp_map_ptr;
   source_work_hdr = iter_ptr->it_itype.it_mapiter.it_source_work_hdr;
   source_cell = iter_ptr->it_itype.it_mapiter.it_source_cell;
   source_height = iter_ptr->it_itype.it_mapiter.it_source_height;
   source_index = iter_ptr->it_itype.it_mapiter.it_source_index;

   /* find the next element in the map */

   while (source_cell == NULL) {

      /* start on the next clash list, if we're at a leaf */

      if (!source_height && source_index < MAP_HASH_SIZE) {

         source_cell = source_work_hdr->m_child[source_index].m_cell;
         source_index++;

         continue;

      }

      /* move up if we're at the end of a node */

      if (source_index >= MAP_HASH_SIZE) {

         /* there are no more elements, so break */

         if (source_work_hdr == source_root)
            break;

         /* otherwise move up */

         source_height++;
         source_index =
            source_work_hdr->m_ntype.m_intern.m_child_index + 1;
         source_work_hdr =
            source_work_hdr->m_ntype.m_intern.m_parent;

         continue;

      }

      /* skip over null nodes */

      if (source_work_hdr->m_child[source_index].m_header == NULL) {

         source_index++;
         continue;

      }

      /* otherwise drop down a level */

      source_work_hdr =
         source_work_hdr->m_child[source_index].m_header;
      source_index = 0;
      source_height--;

   }

   /* if the map is empty, return NO */

   if (source_cell == NULL) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return NO;

   }

   /* save our location in the map */

   iter_ptr->it_itype.it_mapiter.it_source_work_hdr = source_work_hdr;
   iter_ptr->it_itype.it_mapiter.it_source_cell = source_cell;
   iter_ptr->it_itype.it_mapiter.it_source_height = source_height;
   iter_ptr->it_itype.it_mapiter.it_source_index = source_index;

   /* we return the domain element */

   mark_specifier(&(source_cell->m_domain_spec));
   unmark_specifier(target);
   target->sp_form = source_cell->m_domain_spec.sp_form;
   target->sp_val.sp_biggest =
         source_cell->m_domain_spec.sp_val.sp_biggest;

   iter_ptr->it_itype.it_mapiter.it_source_cell = source_cell->m_next;

   return YES;

}

/*\
 *  \function{start\_pow\_iterator()}
 *
 *  This function starts iteration over a power set.  We prefer to
 *  iterate over a power set rather than forming it.
\*/

void start_pow_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* power set                         */
   specifier *left)                    /* source set                        */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */
struct source_elem_item *se_array;     /* array of source elements          */
int se_array_length;                   /* length of above array             */
int se_index;                          /* index into above array            */
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_pow;

   mark_specifier(left);

   iter_ptr->it_itype.it_powiter.it_spec.sp_form = ft_set;
   iter_ptr->it_itype.it_powiter.it_spec.sp_val.sp_set_ptr =
         left->sp_val.sp_set_ptr;

   source_root = left->sp_val.sp_set_ptr;

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

   /* we've built up the array, set the iterator data */

   iter_ptr->it_itype.it_powiter.it_se_array = se_array;
   iter_ptr->it_itype.it_powiter.it_se_array_length = se_array_length;
   iter_ptr->it_itype.it_powiter.it_done = NO;

   /* set the target value and return */

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   return;

}

/*\
 *  \function{pow\_iterator\_next()}
 *
 *  This function returns the next subset in a power set iterator.
\*/

int pow_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* power set                         */
   specifier *left)                    /* source set                        */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
struct source_elem_item *se_array;     /* array of source elements          */
int se_array_length;                   /* length of above array             */
int se_index;                          /* index into above array            */
set_h_ptr_type subset_root, subset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type subset_cell;            /* current cell pointer              */
set_c_ptr_type *subset_tail;           /* attach new cells here             */
int subset_height, subset_index;       /* current height and index          */
specifier *subset_element;             /* set element                       */
int32 subset_hash_code;                /* hash code of subset element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int i;                                 /* temporary looping variable        */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   se_array = iter_ptr->it_itype.it_powiter.it_se_array;
   se_array_length = iter_ptr->it_itype.it_powiter.it_se_array_length;

   /* if we've finished return NO */

   if (iter_ptr->it_itype.it_powiter.it_done) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return NO;

   }

   /* create a new set for the subset */

   get_set_header(subset_root);
   subset_root->s_use_count = 1;
   subset_root->s_hash_code = 0;
   subset_root->s_ntype.s_root.s_cardinality = 0;
   subset_root->s_ntype.s_root.s_height = 0;
   for (i = 0;
        i < SET_HASH_SIZE;
        subset_root->s_child[i++].s_cell = NULL);
   expansion_trigger = SET_HASH_SIZE * 2;

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
    *  We treat the field se_in_set as a single bit in a binary number
    *  representing a subset.  We effectively add one to this binary
    *  number.
    */

   for (se_index = 0;
        se_index < se_array_length && se_array[se_index].se_in_set;
        se_array[se_index++].se_in_set = NO);

   if (se_index >= se_array_length) {

      iter_ptr->it_itype.it_powiter.it_done = YES;

   }
   else {

      se_array[se_index].se_in_set = YES;

   }

   /* finally, set the target value */

   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = subset_root;

   return YES;

}

/*\
 *  \function{start\_npow\_iterator()}
 *
 *  This function starts iteration over a power set.  We prefer to
 *  iterate over a power set rather than forming it.
\*/

void start_npow_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* power set                         */
   specifier *left,                    /* source set                        */
   int32 n)                            /* size of each subset               */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */
struct source_elem_item *se_array;     /* array of source elements          */
int se_array_length;                   /* length of above array             */
int se_index;                          /* index into above array            */
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_npow;

   mark_specifier(left);

   iter_ptr->it_itype.it_powiter.it_spec.sp_form = ft_set;
   iter_ptr->it_itype.it_powiter.it_spec.sp_val.sp_set_ptr =
         left->sp_val.sp_set_ptr;

   source_root = left->sp_val.sp_set_ptr;

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

   /* initially the first n elements are in the set */

   for (se_index = 0;
        se_index < se_array_length && se_index < n;
        se_array[se_index++].se_in_set = YES);

   /* we've built up the array, set the iterator data */

   iter_ptr->it_itype.it_powiter.it_se_array = se_array;
   iter_ptr->it_itype.it_powiter.it_se_array_length = se_array_length;
   iter_ptr->it_itype.it_powiter.it_n = (int)n;
   if (n > se_array_length)
      iter_ptr->it_itype.it_powiter.it_done = YES;
   else
      iter_ptr->it_itype.it_powiter.it_done = NO;

   /* set the target value and return */

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   return;

}

/*\
 *  \function{npow\_iterator\_next()}
 *
 *  This function returns the next subset in a power set iterator.
\*/

int npow_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* power set                         */
   specifier *left)                    /* source set                        */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
struct source_elem_item *se_array;     /* array of source elements          */
int se_array_length;                   /* length of above array             */
int32 n;                               /* size of each subset               */
int se_index;                          /* index into above array            */
int se_right_no;                       /* rightmost no                      */
set_h_ptr_type subset_root, subset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type subset_cell;            /* current cell pointer              */
set_c_ptr_type *subset_tail;           /* attach new cells here             */
int subset_height, subset_index;       /* current height and index          */
specifier *subset_element;             /* set element                       */
int32 subset_hash_code;                /* hash code of subset element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int i;                                 /* temporary looping variable        */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   se_array = iter_ptr->it_itype.it_powiter.it_se_array;
   se_array_length = iter_ptr->it_itype.it_powiter.it_se_array_length;
   n = iter_ptr->it_itype.it_powiter.it_n;

   /* if we've finished return NO */

   if (iter_ptr->it_itype.it_powiter.it_done) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return NO;

   }

   /* create a new set for the subset */

   get_set_header(subset_root);
   subset_root->s_use_count = 1;
   subset_root->s_hash_code = 0;
   subset_root->s_ntype.s_root.s_cardinality = 0;
   subset_root->s_ntype.s_root.s_height = 0;
   for (i = 0;
        i < SET_HASH_SIZE;
        subset_root->s_child[i++].s_cell = NULL);
   expansion_trigger = SET_HASH_SIZE * 2;

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

   if (se_index < 0) {

      iter_ptr->it_itype.it_powiter.it_done = YES;

   }
   else {

      se_array[se_index++].se_in_set = NO;
      se_array[se_index++].se_in_set = YES;

      while (se_right_no < se_array_length) {

         se_array[se_right_no++].se_in_set = NO;
         se_array[se_index++].se_in_set = YES;

      }
   }

   /* finally, set the target value */

   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = subset_root;

   return YES;

}

/*\
 *  \function{start\_string\_pair\_iterator()}
 *
 *  This function initializes an iterator for iteration over a string,
 *  where we want pairs of indices and characters.  All we do is
 *  initialize an iterator item.
\*/

void start_string_pair_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source string                     */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_string_pair;

   mark_specifier(left);

   iter_ptr->it_itype.it_striter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_striter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_striter.it_string_cell =
         (left->sp_val.sp_string_ptr)->s_head;
   iter_ptr->it_itype.it_striter.it_string_index = 0;
   iter_ptr->it_itype.it_striter.it_char_number = 0;

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

}

/*\
 *  \function{string\_pair\_iterator\_next()}
 *
 *  This function returns the next string element in an iteration over a
 *  string.
\*/

int string_pair_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *dtarget,                 /* domain target                     */
   specifier *rtarget,                 /* range target                      */
   specifier *left)                    /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
string_h_ptr_type string_hdr;          /* string header node                */
string_c_ptr_type string_cell;         /* current cell pointer              */
int string_index;                      /* current index                     */
string_h_ptr_type new_hdr;             /* created string header             */
string_c_ptr_type new_cell;            /* created string cell               */
int32 short_hi_bits;                   /* high order bits of short          */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   string_hdr = iter_ptr->it_itype.it_striter.it_spec.sp_val.sp_string_ptr;

   /* if there are no characters left we're done */

   if (iter_ptr->it_itype.it_striter.it_char_number >=
       string_hdr->s_length) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      return NO;

   }

   /* set up for the next character */

   iter_ptr->it_itype.it_striter.it_char_number++;

   string_cell = iter_ptr->it_itype.it_striter.it_string_cell;
   string_index = iter_ptr->it_itype.it_striter.it_string_index;

   if (string_index == STR_CELL_WIDTH) {

      iter_ptr->it_itype.it_striter.it_string_cell =
         string_cell = string_cell->s_next;
      iter_ptr->it_itype.it_striter.it_string_index = string_index = 0;

   }

   iter_ptr->it_itype.it_striter.it_string_index++;

   /* create a one character string */

   get_string_header(new_hdr);
   new_hdr->s_use_count = 1;
   new_hdr->s_hash_code = -1;
   new_hdr->s_length = 1;

   get_string_cell(new_cell);
   new_hdr->s_head = new_hdr->s_tail = new_cell;
   new_cell->s_next = NULL;
   new_cell->s_prev = NULL;
   new_cell->s_cell_value[0] =
      string_cell->s_cell_value[string_index];

   /* assign it to the target */

   unmark_specifier(rtarget);
   rtarget->sp_form = ft_string;
   rtarget->sp_val.sp_string_ptr = new_hdr;

   if (!(short_hi_bits =
         iter_ptr->it_itype.it_striter.it_char_number & INT_HIGH_BITS) ||
        short_hi_bits == INT_HIGH_BITS) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_short;
      dtarget->sp_val.sp_short_value =
            iter_ptr->it_itype.it_striter.it_char_number;

   }
   else {

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM dtarget,iter_ptr->it_itype.it_striter.it_char_number);

   }

   return YES;

}

/*\
 *  \function{start\_tuple\_pair\_iterator()}
 *
 *  This function initializes an iterator for iteration over a tuple,
 *  where we want pairs of indices and elements.  All we do is
 *  initialize an iterator item.
\*/

void start_tuple_pair_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source tuple                      */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_tuple_pair;

   mark_specifier(left);

   iter_ptr->it_itype.it_tupiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_tupiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_tupiter.it_source_number = 0;

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

}

/*\
 *  \function{tuple\_pair\_iterator\_next()}
 *
 *  This function returns the next tuple element in an iteration over a
 *  tuple.
\*/

int tuple_pair_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *dtarget,                 /* domain target                     */
   specifier *rtarget,                 /* range target                      */
   specifier *left)                    /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
int32 short_hi_bits;                   /* high order bits of short          */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   source_root = iter_ptr->it_itype.it_tupiter.it_spec.sp_val.sp_tuple_ptr;
   source_number = iter_ptr->it_itype.it_tupiter.it_source_number;

   /* check whether the tuple is finished */

   if (source_number >= source_root->t_ntype.t_root.t_length) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      return NO;

   }

   iter_ptr->it_itype.it_tupiter.it_source_number++;

   /* descend the header tree until we get to a leaf */

   source_work_hdr = source_root;
   for (source_height = source_root->t_ntype.t_root.t_height;
        source_height;
        source_height--) {

      /* extract the element's index at this level */

      source_index = (source_number >>
                           (source_height * TUP_SHIFT_DIST)) &
                        TUP_SHIFT_MASK;

      /* if we're missing a header record, the element is om */

      if (source_work_hdr->t_child[source_index].t_header == NULL) {

         source_number++;
         if (!(short_hi_bits = source_number & INT_HIGH_BITS) ||
              short_hi_bits == INT_HIGH_BITS) {

            unmark_specifier(dtarget);
            dtarget->sp_form = ft_short;
            dtarget->sp_val.sp_short_value = source_number;

         }
         else {

            /* if we exceed the maximum short, convert to long */

            short_to_long(SETL_SYSTEM dtarget,source_number);

         }

         unmark_specifier(rtarget);
         rtarget->sp_form = ft_omega;

         return YES;

      }

      /* otherwise drop down a level */

      source_work_hdr = source_work_hdr->t_child[source_index].t_header;

   }

   /*
    *  At this point, source_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   source_index = source_number & TUP_SHIFT_MASK;
   if (source_work_hdr->t_child[source_index].t_cell == NULL) {

      source_number++;
      if (!(short_hi_bits = source_number & INT_HIGH_BITS) ||
           short_hi_bits == INT_HIGH_BITS) {

         unmark_specifier(dtarget);
         dtarget->sp_form = ft_short;
         dtarget->sp_val.sp_short_value = source_number;

      }
      else {

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM dtarget,source_number);

      }

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      return YES;

   }

   /* we do have an item to return */

   source_cell = source_work_hdr->t_child[source_index].t_cell;

   mark_specifier(&(source_cell->t_spec));
   unmark_specifier(rtarget);
   rtarget->sp_form = source_cell->t_spec.sp_form;
   rtarget->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;

   source_number++;
   if (!(short_hi_bits = source_number & INT_HIGH_BITS) ||
        short_hi_bits == INT_HIGH_BITS) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_short;
      dtarget->sp_val.sp_short_value = source_number;

   }
   else {

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM dtarget,source_number);

   }

   return YES;

}

/*\
 *  \function{start\_alt_tuple\_pair\_iterator()}
 *
 *  This function initializes an iterator for iteration over a tuple,
 *  where we want pairs of indices and elements.  All we do is
 *  initialize an iterator item.
 *
 *  This version is used where the compiler expected a map, but we
 *  actually found a tuple of pairs.
\*/

void start_alt_tuple_pair_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source tuple                      */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_alt_tuple_pair;

   mark_specifier(left);

   iter_ptr->it_itype.it_tupiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_tupiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_tupiter.it_source_number = 0;

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

}

/*\
 *  \function{tuple\_pair\_iterator\_next()}
 *
 *  This function returns the next tuple element in an iteration over a
 *  tuple.
 *
 *  This version is used where the compiler expected a map, but we
 *  actually found a tuple of pairs.
\*/

int alt_tuple_pair_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *dtarget,                 /* domain target                     */
   specifier *rtarget,                 /* range target                      */
   specifier *left)                    /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   source_root = iter_ptr->it_itype.it_tupiter.it_spec.sp_val.sp_tuple_ptr;
   source_number = iter_ptr->it_itype.it_tupiter.it_source_number;

   /* check whether the tuple is finished */

   if (source_number >= source_root->t_ntype.t_root.t_length) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      return NO;

   }

   iter_ptr->it_itype.it_tupiter.it_source_number++;

   /* descend the header tree until we get to a leaf */

   source_work_hdr = source_root;
   for (source_height = source_root->t_ntype.t_root.t_height;
        source_height;
        source_height--) {

      /* extract the element's index at this level */

      source_index = (source_number >>
                           (source_height * TUP_SHIFT_DIST)) &
                        TUP_SHIFT_MASK;

      /* if we're missing a header record, abend */

      if (source_work_hdr->t_child[source_index].t_header == NULL)
         abend(SETL_SYSTEM msg_invalid_tup_assign);

      /* otherwise drop down a level */

      source_work_hdr = source_work_hdr->t_child[source_index].t_header;

   }

   /*
    *  At this point, source_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   source_index = source_number & TUP_SHIFT_MASK;
   if (source_work_hdr->t_child[source_index].t_cell == NULL)
      abend(SETL_SYSTEM msg_invalid_tup_assign);

   /* we do have an item to return */

   source_cell = source_work_hdr->t_child[source_index].t_cell;

   /* we insist that the tuple element be a tuple */

   if (source_cell->t_spec.sp_form != ft_tuple)
      abend(SETL_SYSTEM msg_invalid_tup_assign);

   source_root = source_cell->t_spec.sp_val.sp_tuple_ptr;

   /* we need the left-most child */

   source_height = source_root->t_ntype.t_root.t_height;
   source_work_hdr = source_root;
   while (source_height-- && source_work_hdr != NULL)
      source_work_hdr = source_work_hdr->t_child[0].t_header;

   /* if we couldn't make it to a leaf, both elements are om */

   if (source_work_hdr == NULL) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      return YES;

   }

   /* set the targets */

   if (source_root->t_ntype.t_root.t_length > 0 &&
       (source_cell = source_work_hdr->t_child[0].t_cell) != NULL) {

      mark_specifier(&(source_cell->t_spec));
      unmark_specifier(dtarget);
      dtarget->sp_form = source_cell->t_spec.sp_form;
      dtarget->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;

   }
   else {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

   }

   if (source_root->t_ntype.t_root.t_length > 1 &&
       (source_cell = source_work_hdr->t_child[1].t_cell) != NULL) {

      mark_specifier(&(source_cell->t_spec));
      unmark_specifier(rtarget);
      rtarget->sp_form = source_cell->t_spec.sp_form;
      rtarget->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;

   }
   else {

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

   }

   return YES;

}

/*\
 *  \function{start\_map\_pair\_iterator()}
 *
 *  This function initializes an iterator for iteration over a map.  All
 *  we do is initialize an iterator item.
\*/

void start_map_pair_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source map                        */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_map_pair;

   mark_specifier(left);

   iter_ptr->it_itype.it_mapiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_mapiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_mapiter.it_source_work_hdr =
         left->sp_val.sp_map_ptr;
   iter_ptr->it_itype.it_mapiter.it_source_cell = NULL;
   iter_ptr->it_itype.it_mapiter.it_source_height =
         (left->sp_val.sp_map_ptr)->m_ntype.m_root.m_height;
   iter_ptr->it_itype.it_mapiter.it_source_index = 0;
   iter_ptr->it_itype.it_mapiter.it_valset_root = NULL;

   /* set the target value and return */

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   return;

}

/*\
 *  \function{map\_pair\_iterator\_next()}
 *
 *  This function picks out the next item in an iteration over a map.
 *  We produce the next pair, using the same algorithm we use to convert
 *  maps to sets.
\*/

int map_pair_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *dtarget,                 /* domain target                     */
   specifier *rtarget,                 /* range target                      */
   specifier *source)                  /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type valset_root, valset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type valset_cell;            /* current cell pointer              */
int valset_height, valset_index;       /* current height and index          */

   /* reload the iteration variables */

   iter_ptr = source->sp_val.sp_iter_ptr;
   source_root = iter_ptr->it_itype.it_mapiter.it_spec.sp_val.sp_map_ptr;
   source_work_hdr = iter_ptr->it_itype.it_mapiter.it_source_work_hdr;
   source_cell = iter_ptr->it_itype.it_mapiter.it_source_cell;
   source_height = iter_ptr->it_itype.it_mapiter.it_source_height;
   source_index = iter_ptr->it_itype.it_mapiter.it_source_index;
   valset_root = iter_ptr->it_itype.it_mapiter.it_valset_root;
   valset_work_hdr = iter_ptr->it_itype.it_mapiter.it_valset_work_hdr;
   valset_cell = iter_ptr->it_itype.it_mapiter.it_valset_cell;
   valset_height = iter_ptr->it_itype.it_mapiter.it_valset_height;
   valset_index = iter_ptr->it_itype.it_mapiter.it_valset_index;

   /* loop until we explicitly return */

   for (;;) {

      /* find the next element in the map */

      while (source_cell == NULL) {

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < MAP_HASH_SIZE) {

            source_cell = source_work_hdr->m_child[source_index].m_cell;
            source_index++;

            continue;

         }

         /* move up if we're at the end of a node */

         if (source_index >= MAP_HASH_SIZE) {

            /* there are no more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->m_ntype.m_intern.m_child_index + 1;
            source_work_hdr =
               source_work_hdr->m_ntype.m_intern.m_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->m_child[source_index].m_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->m_child[source_index].m_header;
         source_index = 0;
         source_height--;

      }

      /* if the map is empty, return NO */

      if (source_cell == NULL) {

         unmark_specifier(dtarget);
         dtarget->sp_form = ft_omega;
         unmark_specifier(rtarget);
         rtarget->sp_form = ft_omega;

         return NO;

      }

      /* save our location in the map */

      iter_ptr->it_itype.it_mapiter.it_source_work_hdr = source_work_hdr;
      iter_ptr->it_itype.it_mapiter.it_source_cell = source_cell;
      iter_ptr->it_itype.it_mapiter.it_source_height = source_height;
      iter_ptr->it_itype.it_mapiter.it_source_index = source_index;
      iter_ptr->it_itype.it_mapiter.it_valset_root = valset_root;

      /* if we're not at a multi-value cell, return the pair */

      if (!(source_cell->m_is_multi_val)) {

         mark_specifier(&(source_cell->m_domain_spec));
         unmark_specifier(dtarget);
         dtarget->sp_form = source_cell->m_domain_spec.sp_form;
         dtarget->sp_val.sp_biggest =
               source_cell->m_domain_spec.sp_val.sp_biggest;

         mark_specifier(&(source_cell->m_range_spec));
         unmark_specifier(rtarget);
         rtarget->sp_form = source_cell->m_range_spec.sp_form;
         rtarget->sp_val.sp_biggest =
               source_cell->m_range_spec.sp_val.sp_biggest;

         iter_ptr->it_itype.it_mapiter.it_source_cell = source_cell->m_next;

         return YES;

      }

      /* otherwise we find the next item in the multi-value set */

      if (valset_root == NULL) {

         valset_root = source_cell->m_range_spec.sp_val.sp_set_ptr;
         valset_work_hdr = valset_root;
         valset_height = valset_root->s_ntype.s_root.s_height;
         valset_cell = NULL;
         valset_index = 0;

      }

      for (;;) {

         /* if we have an element, return it */

         if (valset_cell != NULL) {

            mark_specifier(&(source_cell->m_domain_spec));
            unmark_specifier(dtarget);
            dtarget->sp_form = source_cell->m_domain_spec.sp_form;
            dtarget->sp_val.sp_biggest =
                  source_cell->m_domain_spec.sp_val.sp_biggest;

            mark_specifier(&(valset_cell->s_spec));
            unmark_specifier(rtarget);
            rtarget->sp_form = valset_cell->s_spec.sp_form;
            rtarget->sp_val.sp_biggest =
                  valset_cell->s_spec.sp_val.sp_biggest;

            iter_ptr->it_itype.it_mapiter.it_valset_root = valset_root;
            iter_ptr->it_itype.it_mapiter.it_valset_work_hdr =
                  valset_work_hdr;
            iter_ptr->it_itype.it_mapiter.it_valset_cell =
                  valset_cell->s_next;
            iter_ptr->it_itype.it_mapiter.it_valset_height = valset_height;
            iter_ptr->it_itype.it_mapiter.it_valset_index = valset_index;

            return YES;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!valset_height && valset_index < SET_HASH_SIZE) {

            valset_cell = valset_work_hdr->s_child[valset_index].s_cell;
            valset_index++;

            continue;

         }

         /* move up if we're at the end of a node */

         if (valset_index >= SET_HASH_SIZE) {

            /* there are no more elements, so break */

            if (valset_work_hdr == valset_root)
               break;

            /* otherwise move up */

            valset_height++;
            valset_index =
               valset_work_hdr->s_ntype.s_intern.s_child_index + 1;
            valset_work_hdr =
               valset_work_hdr->s_ntype.s_intern.s_parent;

            continue;

         }

         /* skip over null nodes */

         if (valset_work_hdr->s_child[valset_index].s_header == NULL) {

            valset_index++;
            continue;

         }

         /* otherwise drop down a level */

         valset_work_hdr =
            valset_work_hdr->s_child[valset_index].s_header;
         valset_index = 0;
         valset_height--;

      }

      source_cell = source_cell->m_next;
      valset_root = NULL;

   }
}

/*\
 *  \function{start\_map\_multi\_iterator()}
 *
 *  This function initializes an iterator for iteration over a
 *  multi-valued map.  All we do is initialize an iterator item.
\*/

void start_map_multi_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source map                        */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_map_multi;

   mark_specifier(left);

   iter_ptr->it_itype.it_mapiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_mapiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   iter_ptr->it_itype.it_mapiter.it_source_work_hdr =
         left->sp_val.sp_map_ptr;
   iter_ptr->it_itype.it_mapiter.it_source_cell = NULL;
   iter_ptr->it_itype.it_mapiter.it_source_height =
         (left->sp_val.sp_map_ptr)->m_ntype.m_root.m_height;
   iter_ptr->it_itype.it_mapiter.it_source_index = 0;

   /* set the target value and return */

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   return;

}

/*\
 *  \function{map\_multi\_iterator\_next()}
 *
 *  This function picks out the next item in an iteration over a
 *  multi-valued map.  We produce the next pair, using the same algorithm
 *  we use to convert maps to sets.
\*/

int map_multi_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *dtarget,                 /* domain target                     */
   specifier *rtarget,                 /* range target                      */
   specifier *source)                  /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type valset_root;            /* root and internal node pointers   */
set_c_ptr_type valset_cell;            /* current cell pointer              */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int i;                                 /* temporary looping variable        */

   /* reload the iteration variables */

   iter_ptr = source->sp_val.sp_iter_ptr;
   source_root = iter_ptr->it_itype.it_mapiter.it_spec.sp_val.sp_map_ptr;
   source_work_hdr = iter_ptr->it_itype.it_mapiter.it_source_work_hdr;
   source_cell = iter_ptr->it_itype.it_mapiter.it_source_cell;
   source_height = iter_ptr->it_itype.it_mapiter.it_source_height;
   source_index = iter_ptr->it_itype.it_mapiter.it_source_index;

   /* find the next element in the map */

   while (source_cell == NULL) {

      /* start on the next clash list, if we're at a leaf */

      if (!source_height && source_index < MAP_HASH_SIZE) {

         source_cell = source_work_hdr->m_child[source_index].m_cell;
         source_index++;

         continue;

      }

      /* move up if we're at the end of a node */

      if (source_index >= MAP_HASH_SIZE) {

         /* there are no more elements, so break */

         if (source_work_hdr == source_root)
            break;

         /* otherwise move up */

         source_height++;
         source_index =
            source_work_hdr->m_ntype.m_intern.m_child_index + 1;
         source_work_hdr =
            source_work_hdr->m_ntype.m_intern.m_parent;

         continue;

      }

      /* skip over null nodes */

      if (source_work_hdr->m_child[source_index].m_header == NULL) {

         source_index++;
         continue;

      }

      /* otherwise drop down a level */

      source_work_hdr =
         source_work_hdr->m_child[source_index].m_header;
      source_index = 0;
      source_height--;

   }

   /* if the map is empty, return NO */

   if (source_cell == NULL) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;
      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      return NO;

   }

   /* save our location in the map */

   iter_ptr->it_itype.it_mapiter.it_source_work_hdr = source_work_hdr;
   iter_ptr->it_itype.it_mapiter.it_source_cell = source_cell;
   iter_ptr->it_itype.it_mapiter.it_source_height = source_height;
   iter_ptr->it_itype.it_mapiter.it_source_index = source_index;
   iter_ptr->it_itype.it_mapiter.it_source_cell = source_cell->m_next;

   /* if we're at a multi-value cell, return the pair */

   if (source_cell->m_is_multi_val) {

      mark_specifier(&(source_cell->m_domain_spec));
      unmark_specifier(dtarget);
      dtarget->sp_form = source_cell->m_domain_spec.sp_form;
      dtarget->sp_val.sp_biggest =
            source_cell->m_domain_spec.sp_val.sp_biggest;

      mark_specifier(&(source_cell->m_range_spec));
      unmark_specifier(rtarget);
      rtarget->sp_form = source_cell->m_range_spec.sp_form;
      rtarget->sp_val.sp_biggest =
            source_cell->m_range_spec.sp_val.sp_biggest;

      return YES;

   }

   /* otherwise, we must make a singleton set */

   get_set_header(valset_root);
   valset_root->s_use_count = 1;
   valset_root->s_ntype.s_root.s_cardinality = 1;
   valset_root->s_ntype.s_root.s_height = 0;
   for (i = 0;
        i < SET_HASH_SIZE;
        valset_root->s_child[i++].s_cell = NULL);

   spec_hash_code(work_hash_code,&(source_cell->m_range_spec));
   valset_root->s_hash_code = work_hash_code;
   get_set_cell(valset_cell);
   valset_cell->s_spec.sp_form = source_cell->m_range_spec.sp_form;
   valset_cell->s_spec.sp_val.sp_biggest =
         source_cell->m_range_spec.sp_val.sp_biggest;
   mark_specifier(&(valset_cell->s_spec));
   valset_cell->s_hash_code = work_hash_code;
   valset_cell->s_next = NULL;
   valset_root->s_child[work_hash_code & SET_HASH_MASK].s_cell = valset_cell;

   /* set the domain element */

   mark_specifier(&(source_cell->m_domain_spec));
   unmark_specifier(dtarget);
   dtarget->sp_form = source_cell->m_domain_spec.sp_form;
   dtarget->sp_val.sp_biggest =
         source_cell->m_domain_spec.sp_val.sp_biggest;

   /* set the value set */

   unmark_specifier(rtarget);
   rtarget->sp_form = ft_set;
   rtarget->sp_val.sp_set_ptr = valset_root;

   return YES;

}

/*\
 *  \function{start\_object\_iterator()}
 *
 *  This function initializes an iterator for iteration over an object.
 *  We initialize an interator value and call the method for iteration
 *  over an object.
\*/

void start_object_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source object                     */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */
unittab_ptr_type class_ptr;            /* class pointer                     */
object_h_ptr_type object_root;         /* object header pointer             */
struct slot_info_item *slot_info;      /* slot information record           */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_object;

   mark_specifier(left);

   iter_ptr->it_itype.it_objiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_objiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   /* now let the object know we're going to iterate */

   object_root = left->sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + m_iterstart;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM msg_missing_method,"Iterator_Start",
            class_ptr->ut_name);

   /* call the procedure */

   call_procedure(SETL_SYSTEM NULL,
                  slot_info->si_spec,
                  &(iter_ptr->it_itype.it_objiter.it_spec),
                  0L,NO,YES,0);

   return;

}

/*\
 *  \function{object\_iterator\_next()}
 *
 *  This function returns the next object element in an iteration over a
 *  object.
\*/

int object_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* next element in object            */
   specifier *left)                    /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
unittab_ptr_type class_ptr;            /* class pointer                     */
object_h_ptr_type object_root;         /* object header pointer             */
struct slot_info_item *slot_info;      /* slot information record           */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int source_height;                     /* current height and index          */
specifier spare;                       /* spare specifier                   */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   object_root = iter_ptr->it_itype.it_objiter.it_spec.sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + m_iternext;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM msg_missing_method,"Iterator_Next",
            class_ptr->ut_name);

   /* call the procedure */

   spare.sp_form = ft_omega;
   call_procedure(SETL_SYSTEM &spare,
                  slot_info->si_spec,
                  &(iter_ptr->it_itype.it_objiter.it_spec),
                  0L,YES,YES,0);

   /* we must get a tuple or omega */

   if (spare.sp_form != ft_omega &&
       spare.sp_form != ft_tuple)
      abend(SETL_SYSTEM "Return from Iterator_Next must be tuple or omega:\nValue => %s",
            abend_opnd_str(SETL_SYSTEM &spare));

   /* if the method returns omega we quit */

   if (spare.sp_form == ft_omega) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return NO;

   }

   /* if the method returns a tuple we pick out the return value */

   source_root = spare.sp_val.sp_tuple_ptr;

   /* we need the left-most child */

   source_height = source_root->t_ntype.t_root.t_height;
   source_work_hdr = source_root;
   while (source_height-- && source_work_hdr != NULL)
      source_work_hdr = source_work_hdr->t_child[0].t_header;

   /* if we couldn't make it to a leaf, return om */

   if (source_work_hdr == NULL) {

      unmark_specifier(&spare);
      unmark_specifier(target);
      target->sp_form = ft_omega;

      return YES;

   }

   /* set the target */

   if (source_root->t_ntype.t_root.t_length > 0 &&
       (source_cell = source_work_hdr->t_child[0].t_cell) != NULL) {

      mark_specifier(&(source_cell->t_spec));
      unmark_specifier(target);
      target->sp_form = source_cell->t_spec.sp_form;
      target->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;
      unmark_specifier(&spare);

   }
   else {

      unmark_specifier(target);
      target->sp_form = ft_omega;
      unmark_specifier(&spare);

   }

   return YES;

}

/*\
 *  \function{start\_object\_pair\_iterator()}
 *
 *  This function initializes an iterator for iteration over a object,
 *  where we want pairs of elements.  We initialize an iterator item and
 *  call the method to let the object know we're going to iterate.
\*/

void start_object_pair_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source object                     */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */
unittab_ptr_type class_ptr;            /* class pointer                     */
object_h_ptr_type object_root;         /* object header pointer             */
struct slot_info_item *slot_info;      /* slot information record           */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_object_pair;

   mark_specifier(left);

   iter_ptr->it_itype.it_objiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_objiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   /* now let the object know we're going to iterate */

   object_root = left->sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + m_iterstart;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM msg_missing_method,"Iterator_Start",
            class_ptr->ut_name);

   /* call the procedure */

   call_procedure(SETL_SYSTEM NULL,
                  slot_info->si_spec,
                  &(iter_ptr->it_itype.it_objiter.it_spec),
                  0L,NO,YES,0);

   return;

}

/*\
 *  \function{object\_pair\_iterator\_next()}
 *
 *  This function returns the next object element in an iteration over an
 *  object.
\*/

int object_pair_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *dtarget,                 /* domain target                     */
   specifier *rtarget,                 /* range target                      */
   specifier *left)                    /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
unittab_ptr_type class_ptr;            /* class pointer                     */
object_h_ptr_type object_root;         /* object header pointer             */
struct slot_info_item *slot_info;      /* slot information record           */
specifier spare;                       /* spare specifier                   */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int source_height;                     /* current height and index          */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   object_root = iter_ptr->it_itype.it_objiter.it_spec.sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + m_iternext;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM msg_missing_method,"Iterator_Next",
            class_ptr->ut_name);

   /* call the procedure */

   spare.sp_form = ft_omega;
   call_procedure(SETL_SYSTEM &spare,
                  slot_info->si_spec,
                  &(iter_ptr->it_itype.it_objiter.it_spec),
                  0L,YES,YES,0);

   /* we must get a tuple or omega */

   if (spare.sp_form != ft_omega &&
       spare.sp_form != ft_tuple)
      abend(SETL_SYSTEM "Return from Iterator_Next must be tuple or omega:\nValue => %s",
            abend_opnd_str(SETL_SYSTEM &spare));

   /* if the method returns omega we quit */

   if (spare.sp_form == ft_omega) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      return NO;

   }

   /* if the method returns a tuple we pick out the return value */

   source_root = spare.sp_val.sp_tuple_ptr;

   /* we need the left-most child */

   source_height = source_root->t_ntype.t_root.t_height;
   source_work_hdr = source_root;
   while (source_height-- && source_work_hdr != NULL)
      source_work_hdr = source_work_hdr->t_child[0].t_header;

   /* if we couldn't make it to a leaf, abend */

   if (source_work_hdr == NULL)
      abend(SETL_SYSTEM "Return from Iterator_Next must be a nested tuple:\nValue => %s",
            abend_opnd_str(SETL_SYSTEM &spare));

   /* make sure we find something */

   source_cell = source_work_hdr->t_child[0].t_cell;
   if (source_root->t_ntype.t_root.t_length == 0 ||
       source_cell == NULL)
      abend(SETL_SYSTEM "Return from Iterator_Next must be a nested tuple:\nValue => %s",
            abend_opnd_str(SETL_SYSTEM &spare));

   /* it must be a tuple */

   if (source_cell->t_spec.sp_form != ft_tuple)
      abend(SETL_SYSTEM "Return from Iterator_Next must be a nested tuple:\nValue => %s",
            abend_opnd_str(SETL_SYSTEM &spare));

   /* now pick apart the tuple */

   source_root = source_cell->t_spec.sp_val.sp_tuple_ptr;

   /* we need the left-most child */

   source_height = source_root->t_ntype.t_root.t_height;
   source_work_hdr = source_root;
   while (source_height-- && source_work_hdr != NULL)
      source_work_hdr = source_work_hdr->t_child[0].t_header;

   /* if we couldn't make it to a leaf, both elements are om */

   if (source_work_hdr == NULL) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      unmark_specifier(&spare);

      return YES;

   }

   /* set the targets */

   if (source_root->t_ntype.t_root.t_length > 0 &&
       (source_cell = source_work_hdr->t_child[0].t_cell) != NULL) {

      mark_specifier(&(source_cell->t_spec));
      unmark_specifier(dtarget);
      dtarget->sp_form = source_cell->t_spec.sp_form;
      dtarget->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;

   }
   else {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

   }

   if (source_root->t_ntype.t_root.t_length > 1 &&
       (source_cell = source_work_hdr->t_child[1].t_cell) != NULL) {

      mark_specifier(&(source_cell->t_spec));
      unmark_specifier(rtarget);
      rtarget->sp_form = source_cell->t_spec.sp_form;
      rtarget->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;

   }
   else {

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

   }

   unmark_specifier(&spare);

   return YES;

}

/*\
 *  \function{start\_object\_multi\_iterator()}
 *
 *  This function initializes an iterator for iteration over a object,
 *  where we want pairs of elements and we expect the second to be a set.
 *  We initialize an iterator item and call the method to let the object
 *  know we're going to iterate.
\*/

void start_object_multi_iterator(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* iterator variable                 */
   specifier *left)                    /* source object                     */

{
iter_ptr_type iter_ptr;                /* created iterator pointer          */
unittab_ptr_type class_ptr;            /* class pointer                     */
object_h_ptr_type object_root;         /* object header pointer             */
struct slot_info_item *slot_info;      /* slot information record           */

   /* allocate and initialize an iterator */

   get_iterator(iter_ptr);
   iter_ptr->it_use_count = 1;
   iter_ptr->it_type = it_object_multi;

   mark_specifier(left);

   iter_ptr->it_itype.it_objiter.it_spec.sp_form = left->sp_form;
   iter_ptr->it_itype.it_objiter.it_spec.sp_val.sp_biggest =
         left->sp_val.sp_biggest;

   unmark_specifier(target);
   target->sp_form = ft_iter;
   target->sp_val.sp_iter_ptr = iter_ptr;

   /* now let the object know we're going to iterate */

   object_root = left->sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + m_siterstart;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM msg_missing_method,"Set_Iterator_Start",
            class_ptr->ut_name);

   /* call the procedure */

   call_procedure(SETL_SYSTEM NULL,
                  slot_info->si_spec,
                  &(iter_ptr->it_itype.it_objiter.it_spec),
                  0L,NO,YES,0);

   return;

}

/*\
 *  \function{object\_multi\_iterator\_next()}
 *
 *  This function returns the next object element in an iteration over an
 *  object.
\*/

int object_multi_iterator_next(
   SETL_SYSTEM_PROTO
   specifier *dtarget,                 /* domain target                     */
   specifier *rtarget,                 /* range target                      */
   specifier *left)                    /* iterator item                     */

{
iter_ptr_type iter_ptr;                /* iterator pointer                  */
unittab_ptr_type class_ptr;            /* class pointer                     */
object_h_ptr_type object_root;         /* object header pointer             */
struct slot_info_item *slot_info;      /* slot information record           */
specifier spare;                       /* spare specifier                   */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int source_height;                     /* current height and index          */

   /* reload the iteration variables */

   iter_ptr = left->sp_val.sp_iter_ptr;
   object_root = iter_ptr->it_itype.it_objiter.it_spec.sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + m_siternext;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM msg_missing_method,"Set_Iterator_Next",
            class_ptr->ut_name);

   /* call the procedure */

   spare.sp_form = ft_omega;
   call_procedure(SETL_SYSTEM &spare,
                  slot_info->si_spec,
                  &(iter_ptr->it_itype.it_objiter.it_spec),
                  0L,YES,YES,0);

   /* we must get a tuple or omega */

   if (spare.sp_form != ft_omega &&
       spare.sp_form != ft_tuple)
      abend(SETL_SYSTEM 
         "Return from Set_Iterator_Next must be tuple or omega:\nValue => %s",
         abend_opnd_str(SETL_SYSTEM &spare));

   /* if the method returns omega we quit */

   if (spare.sp_form == ft_omega) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      return NO;

   }

   /* if the method returns a tuple we pick out the return value */

   source_root = spare.sp_val.sp_tuple_ptr;

   /* we need the left-most child */

   source_height = source_root->t_ntype.t_root.t_height;
   source_work_hdr = source_root;
   while (source_height-- && source_work_hdr != NULL)
      source_work_hdr = source_work_hdr->t_child[0].t_header;

   /* if we couldn't make it to a leaf, abend */

   if (source_work_hdr == NULL)
      abend(SETL_SYSTEM 
         "Return from Set_Iterator_Next must be a nested tuple:\nValue => %s",
         abend_opnd_str(SETL_SYSTEM &spare));

   /* make sure we find something */

   source_cell = source_work_hdr->t_child[0].t_cell;
   if (source_root->t_ntype.t_root.t_length == 0 ||
       source_cell == NULL)
      abend(SETL_SYSTEM 
         "Return from Set_Iterator_Next must be a nested tuple:\nValue => %s",
         abend_opnd_str(SETL_SYSTEM &spare));

   /* it must be a tuple */

   if (source_cell->t_spec.sp_form != ft_tuple)
      abend(SETL_SYSTEM 
         "Return from Set_Iterator_Next must be a nested tuple:\nValue => %s",
         abend_opnd_str(SETL_SYSTEM &spare));

   /* now pick apart the tuple */

   source_root = source_cell->t_spec.sp_val.sp_tuple_ptr;

   /* we need the left-most child */

   source_height = source_root->t_ntype.t_root.t_height;
   source_work_hdr = source_root;
   while (source_height-- && source_work_hdr != NULL)
      source_work_hdr = source_work_hdr->t_child[0].t_header;

   /* if we couldn't make it to a leaf, both elements are om */

   if (source_work_hdr == NULL) {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

      unmark_specifier(&spare);

      return YES;

   }

   /* set the targets */

   if (source_root->t_ntype.t_root.t_length > 0 &&
       (source_cell = source_work_hdr->t_child[0].t_cell) != NULL) {

      mark_specifier(&(source_cell->t_spec));
      unmark_specifier(dtarget);
      dtarget->sp_form = source_cell->t_spec.sp_form;
      dtarget->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;

   }
   else {

      unmark_specifier(dtarget);
      dtarget->sp_form = ft_omega;

   }

   if (source_root->t_ntype.t_root.t_length > 1 &&
       (source_cell = source_work_hdr->t_child[1].t_cell) != NULL) {

      mark_specifier(&(source_cell->t_spec));
      unmark_specifier(rtarget);
      rtarget->sp_form = source_cell->t_spec.sp_form;
      rtarget->sp_val.sp_biggest = source_cell->t_spec.sp_val.sp_biggest;

   }
   else {

      unmark_specifier(rtarget);
      rtarget->sp_form = ft_omega;

   }

   unmark_specifier(&spare);

   return YES;

}

