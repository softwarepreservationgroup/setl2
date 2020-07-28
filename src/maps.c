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
 *  \package{Maps}
 *
 *  This package contains definitions of the structures used to implement
 *  SETL2 maps, and several low level functions to manipulate those
 *  structures.  We freely confess that we used a very ugly, non-portable
 *  C coding style here, in hopes of getting a fast implementation.  We
 *  have tried to isolate this ugliness to the macros and functions which
 *  allocate and release nodes.  In particular, there are some unsafe
 *  castes, so please make sure this file is compiled with unsafe
 *  optimizations disabled!
 *
 *  \texify{maps.h}
 *
 *  \packagebody{Maps}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "pcode.h"                     /* pseudo code                       */
#include "execute.h"                   /* core interpreter                  */

/* performance tuning constants */

#define MAP_HEADER_BLOCK_SIZE  100     /* map header block size             */
#define MAP_CELL_BLOCK_SIZE    400     /* map cell block size               */

/*\
 *  \function{alloc\_map\_headers()}
 *
 *  This function allocates a block of map headers and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste header items to pointers to header items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the header node.
 *  It will work provided a header item is larger than a pointer, which
 *  is the case.
\*/

void alloc_map_headers(
   SETL_SYSTEM_PROTO_VOID)

{
map_h_ptr_type new_block;              /* first header in allocated block   */

   /* allocate a new block */

   new_block = (map_h_ptr_type)malloc((size_t)
         (MAP_HEADER_BLOCK_SIZE * sizeof(struct map_h_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (MAP_H_NEXT_FREE = new_block;
        MAP_H_NEXT_FREE < new_block + MAP_HEADER_BLOCK_SIZE - 1;
        MAP_H_NEXT_FREE++) {

      *((map_h_ptr_type *)MAP_H_NEXT_FREE) = MAP_H_NEXT_FREE + 1;

   }

   *((map_h_ptr_type *)MAP_H_NEXT_FREE) = NULL;

   /* map next free node to new block */

   MAP_H_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{alloc\_map\_cells()}
 *
 *  This function allocates a block of map cells and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste cell items to pointers to cell items in order to form the free
 *  list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently, while
 *  at the same time avoiding an extra pointer on the cell node.  It
 *  will work provided a cell item is larger than a pointer, which is
 *  the case.
\*/

void alloc_map_cells(
   SETL_SYSTEM_PROTO_VOID)

{
map_c_ptr_type new_block;              /* first cell in allocated block     */

   /* allocate a new block */

   new_block = (map_c_ptr_type)malloc((size_t)
         (MAP_CELL_BLOCK_SIZE * sizeof(struct map_c_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (MAP_C_NEXT_FREE = new_block;
        MAP_C_NEXT_FREE < new_block + MAP_CELL_BLOCK_SIZE - 1;
        MAP_C_NEXT_FREE++) {

      *((map_c_ptr_type *)MAP_C_NEXT_FREE) = MAP_C_NEXT_FREE + 1;

   }

   *((map_c_ptr_type *)MAP_C_NEXT_FREE) = NULL;

   /* map next free node to new block */

   MAP_C_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{copy\_map()}
 *
 *  This function copies an entire map structure.
\*/

map_h_ptr_type copy_map(
   SETL_SYSTEM_PROTO
   map_h_ptr_type source_root)         /* map to be copied                  */

{
map_h_ptr_type source_work_hdr;        /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
map_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type *target_tail;           /* attach new cells here             */
map_h_ptr_type new_hdr;                /* created header node               */
map_c_ptr_type new_cell;               /* created cell node                 */

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
      fprintf(DEBUG_FILE,"*COPY_MAP*\n");
#endif

   /* allocate a new root header node */

   get_map_header(target_root);
   memcpy((void *)target_root,
          (void *)source_root,
          sizeof(struct map_h_item));
   target_root->m_use_count = 1;

   /* we start iterating from the root, at the left of the hash table */

   source_height = source_root->m_ntype.m_root.m_height;
   source_work_hdr = source_root;
   target_work_hdr = target_root;
   source_index = 0;

   /* copy nodes until we finish the tree */

   for (;;) {

      /* if we're at a leaf, copy all the map elements */

      if (!source_height) {

         for (source_index = 0;
              source_index < MAP_HASH_SIZE;
              source_index++) {

            /* loop over the clash list */

            target_tail = &(target_work_hdr->m_child[source_index].m_cell);

            for (source_cell = source_work_hdr->m_child[source_index].m_cell;
                 source_cell != NULL;
                 source_cell = source_cell->m_next) {

               get_map_cell(new_cell);
               memcpy((void *)new_cell,
                      (void *)source_cell,
                      sizeof(struct map_c_item));
               *target_tail = new_cell;
               target_tail = &(new_cell->m_next);
               mark_specifier(&(new_cell->m_domain_spec));
               mark_specifier(&(new_cell->m_range_spec));

            }

            *target_tail = NULL;

         }
      }

      /* if we've finished an internal node, move up */

      if (source_index >= MAP_HASH_SIZE) {

         if (source_work_hdr == source_root)
            break;

         source_height++;
         source_index = source_work_hdr->m_ntype.m_intern.m_child_index + 1;
         source_work_hdr = source_work_hdr->m_ntype.m_intern.m_parent;
         target_work_hdr = target_work_hdr->m_ntype.m_intern.m_parent;

         continue;

      }

      /* if we can't move down, continue */

      if (source_work_hdr->m_child[source_index].m_header == NULL) {

         target_work_hdr->m_child[source_index].m_header = NULL;
         source_index++;
         continue;

      }

      /* we can move down, so do so */

      source_work_hdr = source_work_hdr->m_child[source_index].m_header;
      get_map_header(new_hdr);
      target_work_hdr->m_child[source_index].m_header = new_hdr;
      new_hdr->m_ntype.m_intern.m_parent = target_work_hdr;
      new_hdr->m_ntype.m_intern.m_child_index = source_index;
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
 *  \function{map\_expand\_header()}
 *
 *  This function adds one level to the height of a map header tree.
 *  It should be called when the average length of the clash lists is at
 *  least two.  We loop over the leaves of the header tree, splitting
 *  each one into a tree.
\*/

map_h_ptr_type map_expand_header(
   SETL_SYSTEM_PROTO
   map_h_ptr_type source_root)         /* map to be enlarged                */

{
map_h_ptr_type source_leaf;            /* internal node pointer             */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
map_h_ptr_type target_subtree, target_work_hdr;
                                       /* root and internal node pointers   */
int target_index;                      /* current index                     */
map_c_ptr_type target_cell;            /* current cell pointer              */
map_c_ptr_type *target_tail;           /* attach new cells here             */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int i;                                 /* temporary looping variable        */
int shift_distance;                    /* distance leaf hash codes must     */
                                       /* be shifted, a function of depth   */

   /*
    *  Set up to loop over the source map, producing one leaf node at a
    *  time.
    */

   source_leaf = source_root;
   source_height = source_root->m_ntype.m_root.m_height;
   source_root->m_ntype.m_root.m_height++;
   source_index = 0;
   shift_distance = source_height * MAP_SHIFT_DIST;

   /* loop over the nodes of source */

   for (;;) {

      /* descend to a leaf */

      while (source_height) {

         /* move down if possible */

         if (source_index < MAP_HASH_SIZE) {

            /* skip over null nodes */

            if (source_leaf->m_child[source_index].m_header == NULL) {

               source_index++;
               continue;

            }

            /* we can move down, so do so */

            source_leaf =
               source_leaf->m_child[source_index].m_header;
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
               source_leaf->m_ntype.m_intern.m_child_index + 1;
         source_leaf =
            source_leaf->m_ntype.m_intern.m_parent;

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

      get_map_header(target_subtree);
      memcpy((void *)target_subtree,
             (void *)source_leaf,
             sizeof(struct map_h_item));

      for (i = 0;
           i < MAP_HASH_SIZE;
           target_subtree->m_child[i++].m_header = NULL);

      for (source_index = 0; source_index < MAP_HASH_SIZE; source_index++) {

         source_cell = source_leaf->m_child[source_index].m_cell;
         while (source_cell != NULL) {

            work_hash_code = (source_cell->m_hash_code >> shift_distance);

            target_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

            /* if we're missing a header record, insert it */

            if (target_subtree->m_child[target_index].m_header == NULL) {

               get_map_header(target_work_hdr);
               target_work_hdr->m_ntype.m_intern.m_parent =
                     target_subtree;
               target_work_hdr->m_ntype.m_intern.m_child_index =
                     target_index;
               for (i = 0;
                    i < MAP_HASH_SIZE;
                    target_work_hdr->m_child[i++].m_cell = NULL);
               target_subtree->m_child[target_index].m_header =
                     target_work_hdr;

            }
            else {

               target_work_hdr =
                  target_subtree->m_child[target_index].m_header;

            }

            /* search the clash list for the correct position */

            target_index = work_hash_code & MAP_HASH_MASK;
            target_tail = &(target_work_hdr->m_child[target_index].m_cell);
            for (target_cell = *target_tail;
                 target_cell != NULL &&
                    target_cell->m_hash_code < source_cell->m_hash_code;
                 target_cell = target_cell->m_next) {

               target_tail = &(target_cell->m_next);

            }

            /* shift the source cell to the new subtree */

            target_cell = source_cell;
            source_cell = source_cell->m_next;
            target_cell->m_next = *target_tail;
            *target_tail = target_cell;

         }
      }

      /* if the leaf is the root, we're done */

      if (source_leaf == source_root) {

         free_map_header(source_root);
         return target_subtree;

      }

      /* set up to find the next leaf, by moving to the parent */

      source_height++;
      source_index =  source_leaf->m_ntype.m_intern.m_child_index;
      source_leaf = source_leaf->m_ntype.m_intern.m_parent;
      free_map_header(source_leaf->m_child[source_index].m_header);
      source_leaf->m_child[source_index].m_header = target_subtree;
      source_index++;

   }

   /* if we break without returning, return the original root */

   return source_root;

}

/*\
 *  \function{map\_contract\_header()}
 *
 *  This function subtracts one level from the height of a map header
 *  tree.  It should be called when the average length of the clash
 *  lists for a tree with smaller height is no more than one.  We loop
 *  over the lowest level internal nodes of the source tree, collapsing
 *  each such node.
\*/

map_h_ptr_type map_contract_header(
   SETL_SYSTEM_PROTO
   map_h_ptr_type source_root)         /* map to be contracted              */

{
map_h_ptr_type source_subtree;         /* internal node pointer             */
map_h_ptr_type source_leaf;            /* leaf node                         */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
map_h_ptr_type target_leaf;            /* target leaf node                  */
map_c_ptr_type target_cell;            /* current cell pointer              */
map_c_ptr_type *target_tail;           /* attach new cells here             */
int i;                                 /* temporary looping variable        */

   /*
    *  Set up to loop over the source map, producing one bottom level node
    *  at a time.
    */

   source_subtree = source_root;
   source_height = source_root->m_ntype.m_root.m_height;
   source_root->m_ntype.m_root.m_height--;
   source_index = 0;

   /* loop over the nodes of source */

   for (;;) {

      /* descend to a leaf */

      while (source_height > 1) {

         /* move down if possible */

         if (source_index < MAP_HASH_SIZE) {

            /* skip over null nodes */

            if (source_subtree->m_child[source_index].m_header == NULL) {

               source_index++;
               continue;

            }

            /* we can move down, so do so */

            source_subtree =
               source_subtree->m_child[source_index].m_header;
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
               source_subtree->m_ntype.m_intern.m_child_index + 1;
         source_subtree =
            source_subtree->m_ntype.m_intern.m_parent;

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

      get_map_header(target_leaf);
      memcpy((void *)target_leaf,
             (void *)source_subtree,
             sizeof(struct map_h_item));

      for (i = 0;
           i < MAP_HASH_SIZE;
           target_leaf->m_child[i++].m_header = NULL);

      /* merge the clash lists of each leaf node */

      for (source_index = 0; source_index < MAP_HASH_SIZE; source_index++) {

         source_leaf = source_subtree->m_child[source_index].m_header;
         if (source_leaf == NULL)
            continue;

         /* merge the clash lists of the current leaf into one target list */

         for (i = 0; i < MAP_HASH_SIZE; i++) {

            target_tail = &(target_leaf->m_child[source_index].m_cell);
            target_cell = *target_tail;

            source_cell = source_leaf->m_child[i].m_cell;
            while (source_cell != NULL) {

               /* search the clash list for the correct position */

               while (target_cell != NULL &&
                      target_cell->m_hash_code < source_cell->m_hash_code) {

                  target_tail = &(target_cell->m_next);
                  target_cell = target_cell->m_next;

               }

               /* shift the source cell to the new subtree */

               target_cell = source_cell;
               source_cell = source_cell->m_next;
               target_cell->m_next = *target_tail;
               *target_tail = target_cell;
               target_tail = &(target_cell->m_next);

            }
         }

         free_map_header(source_leaf);

      }

      /* if the subtree is the root, we're done */

      if (source_subtree == source_root) {

         free_map_header(source_root);
         return target_leaf;

      }

      /* map up to find the next leaf, by moving to the parent */

      source_height++;
      source_index = source_subtree->m_ntype.m_intern.m_child_index;
      source_subtree = source_subtree->m_ntype.m_intern.m_parent;
      free_map_header(source_subtree->m_child[source_index].m_header);
      source_subtree->m_child[source_index].m_header = target_leaf;
      source_index++;

   }

   /* if we break without returning, return the original root */

   return source_root;

}

/*\
 *  \function{set\_to\_map()}
 *
 *  This function converts a set structure into a map.  We use different
 *  structures for these two types of objects even though they are the
 *  same from the programmer's point of view.  We do this primarily for
 *  efficiency.  The conversions from set to map and back are done when
 *  the structure is used as a set or map.
 *
 *  This function expects every element of the set to be a tuple of
 *  length two.  If that is not the case it will return \verb"NO",
 *  indicating failure.  If that is the case it will copy the source to
 *  the target, changing it from a set to a map, and return \verb"YES".
\*/

int set_to_map(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *source,                  /* source operand                    */
   int domain_omega_allowed)
{
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* source element                    */
tuple_h_ptr_type tuple_root;           /* tuple header                      */
int tuple_height;                      /* height of tuple (ususally 0)      */
map_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type target_cell;            /* current cell pointer              */
map_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
specifier *domain_element, *range_element;
                                       /* domain and range elements         */
int32 domain_hash_code, range_hash_code;
                                       /* domain and range element hash     */
map_h_ptr_type new_hdr;                /* created header node               */
map_c_ptr_type new_cell;               /* created cell node                 */
set_h_ptr_type valset_root, valset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type valset_cell;            /* current cell pointer              */
set_c_ptr_type *valset_tail;           /* attach new cells here             */
int valset_height, valset_index;       /* current height and index          */
int32 valset_hash_code;                /* valset element hash code          */
set_h_ptr_type new_set_hdr;            /* created header node               */
set_c_ptr_type new_set_cell;           /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 contraction_trigger;             /* cardinality which triggers        */
                                       /* header contraction                */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int is_equal;                          /* YES if two specifiers are equal   */
int i;                                 /* temporary looping variable        */
int len;
specifier spare1,spare2;

   source_root = source->sp_val.sp_set_ptr;

   /* create a new map for the target */

   get_map_header(target_root);
   target_root->m_use_count = 1;
   target_root->m_hash_code = 0;
   target_root->m_ntype.m_root.m_cardinality = 0;
   target_root->m_ntype.m_root.m_cell_count = 0;
   target_root->m_ntype.m_root.m_height =
         source_root->s_ntype.s_root.s_height;
   for (i = 0;
        i < MAP_HASH_SIZE;
        target_root->m_child[i++].m_cell = NULL);

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

      source_element = NULL;
      for (;;) {

         /* if we have an element already, break */

         if (source_cell != NULL) {

            source_element = &(source_cell->s_spec);
            source_cell = source_cell->s_next;

            break;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < SET_HASH_SIZE) {

            source_cell = source_work_hdr->s_child[source_index].s_cell;
            source_index++;

            continue;

         }

         /* move up if we're at the end of a node */

         if (source_index >= SET_HASH_SIZE) {

            /* there are no more elements, so break */

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

      /* if we've exhausted the set break again */

      if (source_element == NULL)
         break;

      /* we insist that the set element be a tuple of length 2 */

      if (source_element->sp_form != ft_tuple)
         return NO;

      tuple_root = source_element->sp_val.sp_tuple_ptr;

	  /*
      if (tuple_root->t_ntype.t_root.t_length == 1)
         continue;
		*/
		
      len=tuple_root->t_ntype.t_root.t_length;
      
      if ((len==0)||(len>2)) 
         return NO;

      /* we need the left-most child */

      tuple_height = tuple_root->t_ntype.t_root.t_height;
      while (tuple_height--) {

         tuple_root = tuple_root->t_child[0].t_header;

#ifdef TRAPS

         if (tuple_root == NULL)
            giveup(SETL_SYSTEM msg_corrupted_tuple);

#endif

      }

      /* if either the domain or range elements are omega, skip them */

      if (tuple_root->t_child[0].t_cell == NULL) {
      	 if (domain_omega_allowed==NO)
      	 	return NO;
      	 domain_element=&spare1;
      	 domain_element->sp_form=ft_omega;
      } else domain_element = &((tuple_root->t_child[0].t_cell)->t_spec);
      
      if (len>1) range_element = &((tuple_root->t_child[1].t_cell)->t_spec);
      else {
      	range_element=&spare2;
      	range_element->sp_form=ft_omega;
      }
      
    

      if (domain_element->sp_form == ft_omega) {
      	 if (domain_omega_allowed==NO)
      	 	return NO;
      	 domain_hash_code=0;
      } else  domain_hash_code = (tuple_root->t_child[0].t_cell)->t_hash_code;
      
      if ((len==1)||(range_element->sp_form==ft_omega))
      	 range_hash_code=0;
      else  range_hash_code = (tuple_root->t_child[1].t_cell)->t_hash_code;

      /*
       *  We now have a valid pair, so we search for it in the map.
       */

      target_work_hdr = target_root;
      work_hash_code = domain_hash_code;
      target_height = target_root->m_ntype.m_root.m_height;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & MAP_HASH_MASK;
         work_hash_code >>= MAP_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (target_work_hdr->m_child[target_index].m_header == NULL) {

            get_map_header(new_hdr);
            new_hdr->m_ntype.m_intern.m_parent = target_work_hdr;
            new_hdr->m_ntype.m_intern.m_child_index = target_index;
            for (i = 0;
                 i < MAP_HASH_SIZE;
                 new_hdr->m_child[i++].m_cell = NULL);
            target_work_hdr->m_child[target_index].m_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->m_child[target_index].m_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level
       *  header record.  The next problem is to determine if the domain
       *  element is already in the map.  We compare the element with the
       *  clash list.
       */

      target_index = work_hash_code & MAP_HASH_MASK;
      target_tail = &(target_work_hdr->m_child[target_index].m_cell);
      for (target_cell = *target_tail;
           target_cell != NULL &&
              target_cell->m_hash_code < domain_hash_code;
           target_cell = target_cell->m_next) {

         target_tail = &(target_cell->m_next);

      }

      /* try to find the domain element */

      is_equal = NO;
      while (target_cell != NULL &&
             target_cell->m_hash_code == domain_hash_code) {

         spec_equal(is_equal,&(target_cell->m_domain_spec),domain_element);

         if (is_equal)
            break;

         target_tail = &(target_cell->m_next);
         target_cell = target_cell->m_next;

      }

      /* The hash code for a map is now identical to the one for sets */

      target_root->m_hash_code ^= domain_hash_code;
      target_root->m_hash_code ^= range_hash_code;

      /* if we don't find the domain element, add a cell */

      if (!is_equal) {

         get_map_cell(new_cell);
         mark_specifier(domain_element);
         mark_specifier(range_element);
         new_cell->m_domain_spec.sp_form = domain_element->sp_form;
         new_cell->m_domain_spec.sp_val.sp_biggest =
            domain_element->sp_val.sp_biggest;
         new_cell->m_range_spec.sp_form = range_element->sp_form;
         new_cell->m_range_spec.sp_val.sp_biggest =
            range_element->sp_val.sp_biggest;
         new_cell->m_is_multi_val = NO;
         new_cell->m_hash_code = domain_hash_code;
         new_cell->m_next = *target_tail;
         *target_tail = new_cell;
         target_root->m_ntype.m_root.m_cardinality++;
         target_root->m_ntype.m_root.m_cell_count++;

         continue;

      }

      /*
       *  Here we know that there is already a domain element for this
       *  pair.  We either have to convert the range element to a set, or
       *  insert our new range element into a set.
       */

      if (!(target_cell->m_is_multi_val)) {

         /* if the range element is already in the range, continue */

         spec_equal(is_equal,&(target_cell->m_range_spec),range_element);

         if (is_equal)
            continue;

         /* create a new singleton set for values */

         get_set_header(valset_root);
         valset_root->s_use_count = 1;
         valset_root->s_ntype.s_root.s_cardinality = 1;
         valset_root->s_ntype.s_root.s_height = 0;
         for (i = 0;
              i < SET_HASH_SIZE;
              valset_root->s_child[i++].s_cell = NULL);

         spec_hash_code(valset_hash_code,&target_cell->m_range_spec);
         valset_root->s_hash_code = valset_hash_code;
         get_set_cell(new_set_cell);
         new_set_cell->s_spec.sp_form = target_cell->m_range_spec.sp_form;
         new_set_cell->s_spec.sp_val.sp_biggest =
               target_cell->m_range_spec.sp_val.sp_biggest;
         new_set_cell->s_hash_code = valset_hash_code;
         new_set_cell->s_next = NULL;
         valset_root->s_child[valset_hash_code & SET_HASH_MASK].s_cell =
               new_set_cell;

         target_cell->m_is_multi_val = YES;
         target_cell->m_range_spec.sp_form = ft_set;
         target_cell->m_range_spec.sp_val.sp_set_ptr = valset_root;

      }
      else {

         valset_root = target_cell->m_range_spec.sp_val.sp_set_ptr;

      }

      /*
       *  Now we have a value set, and we must insert our range element
       *  into that set.
       */

      valset_work_hdr = valset_root;

      /* get the element's hash code */

      work_hash_code = valset_hash_code = range_hash_code;

      /* descend the header tree until we get to a leaf */

      valset_height = valset_root->s_ntype.s_root.s_height;
      while (valset_height--) {

         /* extract the element's index at this level */

         valset_index = work_hash_code & SET_HASH_MASK;
         work_hash_code >>= SET_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (valset_work_hdr->s_child[valset_index].s_header == NULL) {

            get_set_header(new_set_hdr);
            new_set_hdr->s_ntype.s_intern.s_parent = valset_work_hdr;
            new_set_hdr->s_ntype.s_intern.s_child_index = valset_index;
            for (i = 0;
                 i < SET_HASH_SIZE;
                 new_set_hdr->s_child[i++].s_cell = NULL);
            valset_work_hdr->s_child[valset_index].s_header = new_set_hdr;
            valset_work_hdr = new_set_hdr;

         }
         else {

            valset_work_hdr =
               valset_work_hdr->s_child[valset_index].s_header;

         }
      }

      /*
       *  At this point, valset_work_hdr points to the lowest level header
       *  record.  The next problem is to determine if the element is
       *  already in the set.  We compare the element with the clash
       *  list.
       */

      valset_index = work_hash_code & SET_HASH_MASK;
      valset_tail = &(valset_work_hdr->s_child[valset_index].s_cell);
      for (valset_cell = *valset_tail;
           valset_cell != NULL &&
              valset_cell->s_hash_code < valset_hash_code;
           valset_cell = valset_cell->s_next) {

         valset_tail = &(valset_cell->s_next);

      }

      /* check for a duplicate element */

      is_equal = NO;
      while (valset_cell != NULL &&
             valset_cell->s_hash_code == valset_hash_code) {

         spec_equal(is_equal,&(valset_cell->s_spec),range_element);

         if (is_equal)
            break;

         valset_tail = &(valset_cell->s_next);
         valset_cell = valset_cell->s_next;

      }

      /* if we reach this point we didn't find the element, so we insert it */

      if (is_equal)
         continue;

      get_set_cell(new_set_cell);
      mark_specifier(range_element);
      new_set_cell->s_spec.sp_form = range_element->sp_form;
      new_set_cell->s_spec.sp_val.sp_biggest =
         range_element->sp_val.sp_biggest;
      new_set_cell->s_hash_code = valset_hash_code;
      new_set_cell->s_next = *valset_tail;
      *valset_tail = new_set_cell;
      valset_root->s_ntype.s_root.s_cardinality++;
      target_root->m_ntype.m_root.m_cardinality++;
      valset_root->s_hash_code ^= valset_hash_code;

      /* we may have to expand the header, so find the trigger */

      expansion_trigger =
            (1 << ((valset_root->s_ntype.s_root.s_height + 1)
                    * SET_SHIFT_DIST)) * SET_CLASH_SIZE;

      /* expand the set header if necessary */

      if (valset_root->s_ntype.s_root.s_cardinality > expansion_trigger) {

         valset_root = set_expand_header(SETL_SYSTEM valset_root);
         target_cell->m_range_spec.sp_val.sp_set_ptr = valset_root;

      }
   }

   /*
    *  We initially used the height of the set as an estimate for the
    *  height of the map.  That will be a good estimate, if the map is
    *  not multi-valued.  Otherwise we will have to shrink the header
    *  tree.
    */

   contraction_trigger = (1 << ((target_root->m_ntype.m_root.m_height)
                                 * MAP_SHIFT_DIST));
   if (contraction_trigger == 1)
      contraction_trigger = 0;

   while (target_root->m_ntype.m_root.m_cell_count < contraction_trigger) {

      target_root = map_contract_header(SETL_SYSTEM target_root);
      contraction_trigger /= MAP_HASH_SIZE;

   }

   /* finally, set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_map;
   target->sp_val.sp_map_ptr = target_root;
   return YES;

}

/*\
 *  \function{set\_to\_smap()}
 *
 *  This function converts a set structure into a single-valued map.  It
 *  is only used for case's, to prevent duplicate labels.
\*/

void set_to_smap(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *source)                  /* source operand                    */

{
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* source element                    */
tuple_h_ptr_type tuple_root;           /* tuple header                      */
int tuple_height;                      /* height of tuple (ususally 0)      */
map_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type target_cell;            /* current cell pointer              */
map_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
specifier *domain_element, *range_element;
                                       /* domain and range elements         */
int32 domain_hash_code, range_hash_code;
                                       /* domain and range element hash     */
map_h_ptr_type new_hdr;                /* created header node               */
map_c_ptr_type new_cell;               /* created cell node                 */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int is_equal;                          /* YES if two specifiers are equal   */
int i;                                 /* temporary looping variable        */
specifier spare1;    

   source_root = source->sp_val.sp_set_ptr;

   /* create a new map for the target */

   get_map_header(target_root);
   target_root->m_use_count = 1;
   target_root->m_hash_code = 0;
   target_root->m_ntype.m_root.m_cardinality = 0;
   target_root->m_ntype.m_root.m_cell_count = 0;
   target_root->m_ntype.m_root.m_height =
         source_root->s_ntype.s_root.s_height;
   for (i = 0;
        i < MAP_HASH_SIZE;
        target_root->m_child[i++].m_cell = NULL);

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

      source_element = NULL;
      for (;;) {

         /* if we have an element already, break */

         if (source_cell != NULL) {

            source_element = &(source_cell->s_spec);
            source_cell = source_cell->s_next;

            break;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < SET_HASH_SIZE) {

            source_cell = source_work_hdr->s_child[source_index].s_cell;
            source_index++;

            continue;

         }

         /* move up if we're at the end of a node */

         if (source_index >= SET_HASH_SIZE) {

            /* there are no more elements, so break */

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

      /* if we've exhausted the set break again */

      if (source_element == NULL)
         break;

      /* the element will always be a tuple of length 2 */

      tuple_root = source_element->sp_val.sp_tuple_ptr;

      /* we need the left-most child */

      tuple_height = tuple_root->t_ntype.t_root.t_height;
      while (tuple_height--) {

         tuple_root = tuple_root->t_child[0].t_header;

#ifdef TRAPS

         if (tuple_root == NULL)
            giveup(SETL_SYSTEM msg_corrupted_tuple);

#endif

      }

      /* if either the domain or range elements are omega, skip them */

      domain_hash_code = 0;
      domain_element = &((tuple_root->t_child[0].t_cell)->t_spec);

      if ((tuple_root->t_child[0].t_cell)==NULL) {
         domain_element=&spare1;
         domain_element->sp_form=ft_omega;
      } else { 
         if (domain_element->sp_form != ft_omega)
            domain_hash_code = (tuple_root->t_child[0].t_cell)->t_hash_code;
      }

      range_element = &((tuple_root->t_child[1].t_cell)->t_spec);
      range_hash_code = (tuple_root->t_child[1].t_cell)->t_hash_code;

      /*
       *  We now have a valid pair, so we search for it in the map.
       */

      target_work_hdr = target_root;
      work_hash_code = domain_hash_code;
      target_height = target_root->m_ntype.m_root.m_height;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & MAP_HASH_MASK;
         work_hash_code >>= MAP_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (target_work_hdr->m_child[target_index].m_header == NULL) {

            get_map_header(new_hdr);
            new_hdr->m_ntype.m_intern.m_parent = target_work_hdr;
            new_hdr->m_ntype.m_intern.m_child_index = target_index;
            for (i = 0;
                 i < MAP_HASH_SIZE;
                 new_hdr->m_child[i++].m_cell = NULL);
            target_work_hdr->m_child[target_index].m_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->m_child[target_index].m_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level
       *  header record.  The next problem is to determine if the domain
       *  element is already in the map.  We compare the element with the
       *  clash list.
       */

      target_index = work_hash_code & MAP_HASH_MASK;
      target_tail = &(target_work_hdr->m_child[target_index].m_cell);
      for (target_cell = *target_tail;
           target_cell != NULL &&
              target_cell->m_hash_code < domain_hash_code;
           target_cell = target_cell->m_next) {

         target_tail = &(target_cell->m_next);

      }

      /* try to find the domain element */

      is_equal = NO;
      while (target_cell != NULL &&
             target_cell->m_hash_code == domain_hash_code) {

         spec_equal(is_equal,&(target_cell->m_domain_spec),domain_element);

         if (is_equal)
            break;

         target_tail = &(target_cell->m_next);
         target_cell = target_cell->m_next;

      }

      /* if we don't find the domain element, add a cell */

      if (!is_equal) {

         get_map_cell(new_cell);
         mark_specifier(domain_element);
         mark_specifier(range_element);
         new_cell->m_domain_spec.sp_form = domain_element->sp_form;
         new_cell->m_domain_spec.sp_val.sp_biggest =
            domain_element->sp_val.sp_biggest;
         new_cell->m_range_spec.sp_form = range_element->sp_form;
         new_cell->m_range_spec.sp_val.sp_biggest =
            range_element->sp_val.sp_biggest;
         new_cell->m_is_multi_val = NO;
         new_cell->m_hash_code = domain_hash_code;
         new_cell->m_next = *target_tail;
         *target_tail = new_cell;
         target_root->m_ntype.m_root.m_cardinality++;
         target_root->m_ntype.m_root.m_cell_count++;
         target_root->m_hash_code ^= domain_hash_code;

         continue;

      }

      /*
       *  Here we know that there is already a domain element for this
       *  pair.  This is the error we were searching for.  We abort,
       *  displaying an error message.
       */

      abend(SETL_SYSTEM "Duplicate case label\nLabel => %s",
            abend_opnd_str(SETL_SYSTEM domain_element));

   }

   /* finally, set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_map;
   target->sp_val.sp_map_ptr = target_root;

   return;

}

/*\
 *  \function{map\_to\_set()}
 *
 *  This function converts a map structure into a set.  We use different
 *  structures for these two types of objects even though they are the
 *  same from the programmer's point of view.  We do this primarily for
 *  efficiency.  The conversions from set to map and back are done when
 *  the structure is used as a set or map.
 *
 *  We produce pairs of elements, make them into tuples, and insert them
 *  into a set.
\*/

void map_to_set(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *source)                  /* source operand                    */

{
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type valset_root, valset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type valset_cell;            /* current cell pointer              */
int valset_height, valset_index;       /* current height and index          */
specifier *domain_element, *range_element;
                                       /* pair from map                     */
int32 domain_hash_code;                /* hash code of domain element       */
tuple_h_ptr_type tuple_root;           /* tuple header                      */
tuple_c_ptr_type tuple_cell;           /* tuple cell node                   */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int i;                                 /* temporary looping variable        */

   source_root = source->sp_val.sp_map_ptr;

   /* create a new set for the target */

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_hash_code = 0;
   target_root->s_ntype.s_root.s_cardinality = 0;
   target_root->s_ntype.s_root.s_height = 0;
   for (i = 0;
        i < SET_HASH_SIZE;
        target_root->s_child[i++].s_cell = NULL);
   expansion_trigger = SET_HASH_SIZE * SET_CLASH_SIZE;

   /*
    *  We loop over the source map producing one pair at a time.
    */

   source_work_hdr = source_root;
   source_height = source_root->m_ntype.m_root.m_height;
   source_cell = NULL;
   source_index = 0;
   valset_root = NULL;

   /* loop over the elements of source */

   for (;;) {

      /* find the next cell in the map */

      while (source_cell == NULL) {

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < MAP_HASH_SIZE) {

            source_cell = source_work_hdr->m_child[source_index].m_cell;
            source_index++;
            continue;

         }

         /* the current header node is exhausted -- find the next one */

         /* move up if we're at the end of a node */

         if (source_index >= MAP_HASH_SIZE) {

            /* there are not more elements, so break */

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

      /* if there are no more cells, break */

      if (source_cell == NULL)
         break;

      /* if the cell is not multi-value, use the pair directly */

      if (!(source_cell->m_is_multi_val)) {

         domain_element = &(source_cell->m_domain_spec);
         domain_hash_code = source_cell->m_hash_code;
         range_element = &(source_cell->m_range_spec);
         source_cell = source_cell->m_next;

      }

      /* otherwise find the next element in the value set */

      else {

         domain_element = &(source_cell->m_domain_spec);
         domain_hash_code = source_cell->m_hash_code;
         if (valset_root == NULL) {

            valset_root = source_cell->m_range_spec.sp_val.sp_set_ptr;
            valset_work_hdr = valset_root;
            valset_height = valset_root->s_ntype.s_root.s_height;
            valset_cell = NULL;
            valset_index = 0;

         }

         range_element = NULL;
         for (;;) {

            /* if we have an element already, break */

            if (valset_cell != NULL) {

               range_element = &(valset_cell->s_spec);
               valset_cell = valset_cell->s_next;
               break;

            }

            /* start on the next clash list, if we're at a leaf */

            if (!valset_height && valset_index < SET_HASH_SIZE) {

               valset_cell = valset_work_hdr->s_child[valset_index].s_cell;
               valset_index++;
               continue;

            }

            /* the current header node is exhausted -- find the next one */

            /* move up if we're at the end of a node */

            if (valset_index >= SET_HASH_SIZE) {

               /* there are not more elements, so break */

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

         if (range_element == NULL) {

            source_cell = source_cell->m_next;
            valset_root = NULL;
            continue;

         }
      }

      /*
       *  At this point we have a pair from the map, and the hash code of
       *  the domain element.  We have to form a tuple from this pair.
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
      tuple_cell->t_spec.sp_form = domain_element->sp_form;
      tuple_cell->t_spec.sp_val.sp_biggest =
            domain_element->sp_val.sp_biggest;
      mark_specifier(domain_element);
      tuple_cell->t_hash_code = domain_hash_code;
      tuple_root->t_hash_code ^= domain_hash_code;
      tuple_root->t_child[0].t_cell = tuple_cell;

      /* insert range element */

      get_tuple_cell(tuple_cell);
      tuple_cell->t_spec.sp_form = range_element->sp_form;
      tuple_cell->t_spec.sp_val.sp_biggest =
            range_element->sp_val.sp_biggest;
      mark_specifier(range_element);
      spec_hash_code(tuple_cell->t_hash_code,range_element);
      tuple_root->t_hash_code ^=
            tuple_cell->t_hash_code;
      tuple_root->t_child[1].t_cell = tuple_cell;

      /*
       *  At this point we have an element we would like to insert into
       *  the target.
       */

      target_work_hdr = target_root;

      /* get the element's hash code */

      work_hash_code = target_hash_code =
         tuple_root->t_hash_code;

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

      /* note that duplicates are impossible here, so we insert the tuple */

      get_set_cell(new_cell);
      new_cell->s_spec.sp_form = ft_tuple;
      new_cell->s_spec.sp_val.sp_tuple_ptr = tuple_root;
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
 *  \function{map\_domain()}
 *
 *  This function finds the domain of a map.  It loops over the map
 *  producing one domain element at a time and inserting each into a
 *  target set.
\*/

void map_domain(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *source)                  /* source operand                    */

{
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
specifier *target_element;             /* domain element from map           */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int i;                                 /* temporary looping variable        */

   source_root = source->sp_val.sp_map_ptr;

   /* create a new set for the target */

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_hash_code = 0;
   target_root->s_ntype.s_root.s_cardinality = 0;
   target_root->s_ntype.s_root.s_height =
      source_root->m_ntype.m_root.m_height;
   for (i = 0;
        i < SET_HASH_SIZE;
        target_root->s_child[i++].s_cell = NULL);
   expansion_trigger = SET_HASH_SIZE * SET_CLASH_SIZE;

   /*
    *  We loop over the source map producing one domain element at a time.
    */

   source_work_hdr = source_root;
   source_height = source_root->m_ntype.m_root.m_height;
   source_cell = NULL;
   source_index = 0;

   /* loop over the elements of source */

   for (;;) {

      /* find the next cell in the map */

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

      /* if there are no more cells, break */

      if (source_cell == NULL)
         break;

      /*
       *  At this point we have an element we would like to insert into
       *  the target.
       */

      target_element = &(source_cell->m_domain_spec);
      target_hash_code = work_hash_code = source_cell->m_hash_code;
      source_cell = source_cell->m_next;

      /* descend the header tree until we get to a leaf */

      target_height = target_root->s_ntype.s_root.s_height;
      target_work_hdr = target_root;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & SET_HASH_MASK;
         work_hash_code >>= SET_SHIFT_DIST;

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

      /* note that duplicates are impossible here, so we insert the element */

      mark_specifier(target_element);
      get_set_cell(new_cell);
      new_cell->s_spec.sp_form = target_element->sp_form;
      new_cell->s_spec.sp_val.sp_biggest = target_element->sp_val.sp_biggest;
      new_cell->s_hash_code = target_hash_code;
      new_cell->s_next = *target_tail;
      *target_tail = new_cell;
      target_root->s_ntype.s_root.s_cardinality++;
      target_root->s_hash_code ^= target_hash_code;

   }

   /* finally, set the target value */

   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = target_root;

   return;

}

/*\
 *  \function{map\_range()}
 *
 *  This function finds the range of a map.  It loops over the map
 *  producing one range element at a time and inserting each into a
 *  target set.
\*/

void map_range(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *source)                  /* source operand                    */

{
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type valset_root, valset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type valset_cell;            /* current cell pointer              */
int valset_height, valset_index;       /* current height and index          */
specifier *target_element;             /* pair from map                     */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int is_equal;                          /* YES if two specifiers are equal   */
int i;                                 /* temporary looping variable        */

   source_root = source->sp_val.sp_map_ptr;

   /* create a new set for the target */

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_hash_code = 0;
   target_root->s_ntype.s_root.s_cardinality = 0;
   target_root->s_ntype.s_root.s_height = 0;
   for (i = 0;
        i < SET_HASH_SIZE;
        target_root->s_child[i++].s_cell = NULL);
   expansion_trigger = SET_HASH_SIZE * SET_CLASH_SIZE;

   /*
    *  We loop over the source map producing one range element at a time.
    */

   source_work_hdr = source_root;
   source_height = source_root->m_ntype.m_root.m_height;
   source_cell = NULL;
   source_index = 0;
   valset_root = NULL;
   valset_cell = NULL;

   /* loop over the elements of source */

   for (;;) {

      /* find the next cell in the map */

      while (source_cell == NULL) {

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < MAP_HASH_SIZE) {

            source_cell = source_work_hdr->m_child[source_index].m_cell;
            source_index++;
            continue;

         }

         /* move up if we're at the end of a node */

         if (source_index >= MAP_HASH_SIZE) {

            /* there are not more elements, so break */

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

      /* if there are no more cells, break */

      if (source_cell == NULL)
         break;

      /* if the cell is not multi-value, use the pair directly */

      if (!(source_cell->m_is_multi_val)) {

         target_element = &(source_cell->m_range_spec);
         source_cell = source_cell->m_next;

      }

      /* otherwise find the next element in the value set */

      else {

         if (valset_root == NULL) {

            valset_root = source_cell->m_range_spec.sp_val.sp_set_ptr;
            valset_work_hdr = valset_root;
            valset_height = valset_root->s_ntype.s_root.s_height;
            valset_cell = NULL;
            valset_index = 0;

         }

         target_element = NULL;
         for (;;) {

            /* if we have an element already, break */

            if (valset_cell != NULL) {

               target_element = &(valset_cell->s_spec);
               valset_cell = valset_cell->s_next;
               break;

            }

            /* start on the next clash list, if we're at a leaf */

            if (!valset_height && valset_index < SET_HASH_SIZE) {

               valset_cell = valset_work_hdr->s_child[valset_index].s_cell;
               valset_index++;
               continue;

            }

            /* move up if we're at the end of a node */

            if (valset_index >= SET_HASH_SIZE) {

               /* there are not more elements, so break */

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

         if (target_element == NULL) {

            source_cell = source_cell->m_next;
            valset_root = NULL;
            continue;

         }
      }

      /*
       *  At this point we have an element we would like to insert into
       *  the target.
       */

      target_work_hdr = target_root;

      /* get the element's hash code */

      spec_hash_code(work_hash_code,target_element);
      target_hash_code = work_hash_code;

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

      mark_specifier(target_element);
      get_set_cell(new_cell);
      new_cell->s_spec.sp_form = target_element->sp_form;
      new_cell->s_spec.sp_val.sp_biggest = target_element->sp_val.sp_biggest;
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
 *  \function{map\_lessf()}
 *
 *  This function deletes an element from a map, as in a \verb"LESSF".
\*/

void map_lessf(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
map_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type target_cell;            /* current cell pointer              */
map_c_ptr_type *target_cell_tail;      /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
int32 target_hash_code;                /* hash code of domain element       */
int32 contraction_trigger;             /* cardinality which triggers        */
                                       /* header contraction                */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int is_equal;                          /* YES if two specifiers are equal   */
int32 work_cardinality;                /* working cardinality               */

   /*
    *  First we set up a target set.  We would like to use the left
    *  operand destructively if possible.
    */

   if (target == left && target != right &&
       (target->sp_val.sp_map_ptr)->m_use_count == 1) {

      target_root = target->sp_val.sp_map_ptr;
      target->sp_form = ft_omega;

   }
   else {

      target_root = copy_map(SETL_SYSTEM left->sp_val.sp_map_ptr);

   }

   /* look up the domain element in the map */

   target_work_hdr = target_root;
   spec_hash_code(work_hash_code,right);
   target_hash_code = work_hash_code;

   for (target_height = target_work_hdr->m_ntype.m_root.m_height;
        target_height;
        target_height--) {

      /* extract the element's index at this level */

      target_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code >>= MAP_SHIFT_DIST;

      /* if we're missing a header record, we can return */

      if (target_work_hdr->m_child[target_index].m_header == NULL) {

         unmark_specifier(target);
         target->sp_form = ft_map;
         target->sp_val.sp_map_ptr = target_root;

         return;

      }
      else {

         target_work_hdr = target_work_hdr->m_child[target_index].m_header;

      }
   }

   /*
    *  At this point, target_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   target_index = work_hash_code & MAP_HASH_MASK;
   target_cell_tail = &(target_work_hdr->m_child[target_index].m_cell);
   for (target_cell = *target_cell_tail;
        target_cell != NULL &&
           target_cell->m_hash_code < target_hash_code;
        target_cell = target_cell->m_next) {

      target_cell_tail = &(target_cell->m_next);

   }

   /* try to find the domain element */

   is_equal = NO;
   while (target_cell != NULL &&
          target_cell->m_hash_code == target_hash_code) {

      spec_equal(is_equal,&(target_cell->m_domain_spec),right);

      if (is_equal)
         break;

      target_cell_tail = &(target_cell->m_next);
      target_cell = target_cell->m_next;

   }

   /* if we didn't find the domain element we can return */

   if (!is_equal) {

      unmark_specifier(target);
      target->sp_form = ft_map;
      target->sp_val.sp_map_ptr = target_root;

      return;

   }

   /* otherwise we delete the image set */

   spec_hash_code(work_hash_code,&(target_cell->m_range_spec));
   target_root->m_hash_code ^= work_hash_code;

   if (target_cell->m_is_multi_val) {

      work_cardinality = (target_cell->m_range_spec.sp_val.sp_set_ptr)->
              s_ntype.s_root.s_cardinality;

      target_root->m_ntype.m_root.m_cardinality -= work_cardinality;

      if ((work_cardinality%2)==0) 
         target_root->m_hash_code ^= target_cell->m_hash_code;

   }
   else {

      target_root->m_ntype.m_root.m_cardinality--;

   }

   target_root->m_ntype.m_root.m_cell_count--;
   target_root->m_hash_code ^= target_cell->m_hash_code;
   *target_cell_tail = target_cell->m_next;
   unmark_specifier(&(target_cell->m_domain_spec));
   unmark_specifier(&(target_cell->m_range_spec));
   free_map_cell(target_cell);

   /*
    *  We've reduced the number of cells, so we may have to
    *  contract the map header.
    */

   contraction_trigger = (1 << ((target_root->m_ntype.m_root.m_height)
                                * MAP_SHIFT_DIST));
   if (contraction_trigger == 1)
      contraction_trigger = 0;

   if (target_root->m_ntype.m_root.m_cell_count < contraction_trigger) {

      target_root = map_contract_header(SETL_SYSTEM target_root);

   }

   /* set the target and return */

   unmark_specifier(target);
   target->sp_form = ft_map;
   target->sp_val.sp_map_ptr = target_root;

   return;

}
