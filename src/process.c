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
 *  \package{Processs}
 *
 *  This package contains definitions of the structures used to implement
 *  SETL2 processes, and several low level functions to manipulate those
 *  structures.  We freely confess that we used a very ugly, non-portable
 *  C coding style here, in hopes of getting a fast implementation.  We
 *  have tried to isolate this ugliness to the macros and functions which
 *  allocate and release nodes.  In particular, there are some unsafe
 *  castes, so please make sure this file is compiled with unsafe
 *  optimizations disabled!
 *
 *  \texify{process.h}
 *
 *  \packagebody{Processes}
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
#include "x_integers.h"                /* integers                          */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "objects.h"                   /* objects                           */
#include "process.h"                   /* processes                         */
#include "mailbox.h"                   /* mailboxes                         */

/* performance tuning constants */

#define PROCESS_BLOCK_SIZE 100         /* process block size                */
#define REQUEST_BLOCK_SIZE 100         /* request block size                */

/*\
 *  \function{alloc\_processes()}
 *
 *  This function allocates a block of processes and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste process items to pointers to process items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the process node.
 *  It will work provided a process item is larger than a pointer, which
 *  is the case.
\*/

void alloc_processes(
   SETL_SYSTEM_PROTO_VOID)

{       
process_ptr_type new_block;            /* first process in allocated block  */

   /* allocate a new block */

   new_block = (process_ptr_type)malloc((size_t)
         (PROCESS_BLOCK_SIZE * sizeof(struct process_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (PROCESS_NEXT_FREE = new_block;
        PROCESS_NEXT_FREE < new_block + PROCESS_BLOCK_SIZE - 1;
        PROCESS_NEXT_FREE++) {

      *((process_ptr_type *)PROCESS_NEXT_FREE) = PROCESS_NEXT_FREE + 1;

   }

   *((process_ptr_type *)PROCESS_NEXT_FREE) = NULL;

   /* set next free node to new block */

   PROCESS_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{alloc\_requests()}
 *
 *  This function allocates a block of requests and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste request items to pointers to request items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the request node.
 *  It will work provided a request item is larger than a pointer, which
 *  is the case.
\*/

void alloc_requests(
   SETL_SYSTEM_PROTO_VOID)

{      
request_ptr_type new_block;            /* first request in allocated block  */

   /* allocate a new block */

   new_block = (request_ptr_type)malloc((size_t)
         (REQUEST_BLOCK_SIZE * sizeof(struct request_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (REQUEST_NEXT_FREE = new_block;
        REQUEST_NEXT_FREE < new_block + REQUEST_BLOCK_SIZE - 1;
        REQUEST_NEXT_FREE++) {

      *((request_ptr_type *)REQUEST_NEXT_FREE) = REQUEST_NEXT_FREE + 1;

   }

   *((request_ptr_type *)REQUEST_NEXT_FREE) = NULL;

   /* set next free node to new block */

   REQUEST_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{unblock\_process}
 *
 *  This function checks whether a waiting process can be unblocked.
 *  It's called by the \verb"switch_process" procedure, and works in
 *  tandem with the \verb"await" and \verb"acheck" procedures.  When
 *  called, the process is waiting for a mailbox, a set of mailboxes, or
 *  a tuple of mailboxes.
\*/

int process_unblock(
   SETL_SYSTEM_PROTO
   struct process_item *process_ptr)   /* process to be checked             */

{

   switch (process_ptr->pc_wait_key.sp_form) {

/*\
 *  \case{mailboxes}
 *
 *  Hopefully, the most common use of this function will be to wait for a
 *  single mailbox.
\*/

case ft_mailbox :

{
mailbox_h_ptr_type mailbox_ptr;        /* mailbox pointer                   */
mailbox_c_ptr_type mailbox_cell;       /* a mailbox value                   */
struct specifier_item *target;         /* return target                     */

   mailbox_ptr = process_ptr->pc_wait_key.sp_val.sp_mailbox_ptr;
   
   /* if the mailbox is empty we can't unblock */

   if (mailbox_ptr->mb_cell_count == 0)
      return NO;

   /* we'll pop the first value */

   mailbox_cell = mailbox_ptr->mb_head;
   
   /* if the target isn't null, set a return value */

   if (process_ptr->pc_wait_target != NULL) {
      target = &(process_ptr->pc_wait_return);
      target->sp_form = mailbox_cell->mb_spec.sp_form;     
      target->sp_val.sp_biggest = mailbox_cell->mb_spec.sp_val.sp_biggest;     
      mark_specifier(target);
   }

   /* pop the value from the mailbox */

   if (process_ptr->pc_waiting) {
      unmark_specifier(&(mailbox_cell->mb_spec));
      mailbox_ptr->mb_head = mailbox_cell->mb_next;
      if (mailbox_ptr->mb_head == NULL)
         mailbox_ptr->mb_tail = &(mailbox_ptr->mb_head);
      mailbox_ptr->mb_cell_count--;
      free_mailbox_cell(mailbox_cell);
   }

   unmark_specifier(&(process_ptr->pc_wait_key));
   return YES;

}

/*\
 *  \case{sets}
 *
 *  Each element of the set must be a mailbox.  We search for one which
 *  has something in it.
\*/

case ft_set :

{
mailbox_h_ptr_type mailbox_ptr;        /* mailbox pointer                   */
mailbox_c_ptr_type mailbox_cell;       /* a mailbox value                   */
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */
tuple_h_ptr_type tuple_root;           /* root of returned tuple            */
tuple_c_ptr_type tuple_cell;           /* created cell node                 */
int32 i;                               /* temporary looping variable        */

   /* set up to loop over the set */

   source_root = process_ptr->pc_wait_key.sp_val.sp_set_ptr;
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

      /* otherwise check the mailbox */

      mailbox_ptr = source_element->sp_val.sp_mailbox_ptr;
   
      /* if the mailbox is empty we can't unblock */

      if (mailbox_ptr->mb_cell_count == 0)
         continue;

      /* we'll pop the first value */

      mailbox_cell = mailbox_ptr->mb_head;
   
      /* if the target isn't null, set a return value */

      if (process_ptr->pc_wait_target != NULL) {

         /* create a new tuple for the target */

         get_tuple_header(tuple_root);
         tuple_root->t_use_count = 1;
         tuple_root->t_hash_code = 0;
         tuple_root->t_ntype.t_root.t_length = 2;
         tuple_root->t_ntype.t_root.t_height = 0;
         for (i = 2;
              i < TUP_HEADER_SIZE;
              tuple_root->t_child[i++].t_cell = NULL);

         /* the first element is the mailbox key */

         get_tuple_cell(tuple_cell);
         tuple_cell->t_spec.sp_form = ft_mailbox;
         tuple_cell->t_spec.sp_val.sp_mailbox_ptr = mailbox_ptr;
         mailbox_ptr->mb_use_count++;
         spec_hash_code(tuple_cell->t_hash_code,
                        &(tuple_cell->t_spec));
         tuple_root->t_child[0].t_cell = tuple_cell;
         tuple_root->t_hash_code ^= tuple_cell->t_hash_code;

         /* the second element is the value of the mailbox */

         get_tuple_cell(tuple_cell);
         tuple_cell->t_spec.sp_form = 
            mailbox_cell->mb_spec.sp_form;     
         tuple_cell->t_spec.sp_val.sp_biggest =
            mailbox_cell->mb_spec.sp_val.sp_biggest;     
         mark_specifier(&(tuple_cell->t_spec));
         spec_hash_code(tuple_cell->t_hash_code,
                        &(tuple_cell->t_spec));
         tuple_root->t_child[1].t_cell = tuple_cell;
         tuple_root->t_hash_code ^= tuple_cell->t_hash_code;

         /* stick the result on the process record */

         process_ptr->pc_wait_return.sp_form = ft_tuple;
         process_ptr->pc_wait_return.sp_val.sp_tuple_ptr = tuple_root;

      }

      /* pop the value from the mailbox */

      if (process_ptr->pc_waiting) {
         unmark_specifier(&(mailbox_cell->mb_spec));
         mailbox_ptr->mb_head = mailbox_cell->mb_next;
         if (mailbox_ptr->mb_head == NULL)
            mailbox_ptr->mb_tail = &(mailbox_ptr->mb_head);
         mailbox_ptr->mb_cell_count--;
         free_mailbox_cell(mailbox_cell);
      }

      unmark_specifier(&(process_ptr->pc_wait_key));
      return YES;

   }

   return NO;

}

/*\
 *  \case{tuples}
 *
 *  When we find a tuple of mailboxes we insist that each have a value.
\*/

case ft_tuple :

{
mailbox_h_ptr_type mailbox_ptr;        /* mailbox pointer                   */
mailbox_c_ptr_type mailbox_cell;       /* a mailbox value                   */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */
tuple_h_ptr_type tuple_root, tuple_work_hdr, new_tuple_hdr;
                                       /* tuple header pointers             */
int tuple_index, tuple_height;         /* used to descend header trees      */
int32 tuple_length;                    /* current tuple length              */
int32 expansion_trigger;               /* size which triggers header        */
                                       /* expansion                         */
tuple_c_ptr_type tuple_cell;           /* created cell node                 */
int32 i;                               /* temporary looping variable        */

   /*
    *  We'll have to loop over the tuple twice, the first time just
    *  checking each mailbox.
    */

   /* set up to loop over the tuple */

   source_root = process_ptr->pc_wait_key.sp_val.sp_tuple_ptr;
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

         source_work_hdr =
            source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      /* if we've exhausted the tuple, success */

      if (source_element == NULL)
         break;

      /* otherwise check the mailbox */

      mailbox_ptr = source_element->sp_val.sp_mailbox_ptr;
   
      /* if the mailbox is empty we can't unblock */

      if (mailbox_ptr->mb_cell_count == 0)
         return NO;

   }

   /*
    *  Time for the second pass.  Every mailbox does have at least one
    *  value.
    */

   /* initialize a tuple to be returned */

   get_tuple_header(tuple_root);
   tuple_root->t_use_count = 1;
   tuple_root->t_hash_code = 0;
   tuple_root->t_ntype.t_root.t_length = 0;
   tuple_root->t_ntype.t_root.t_height = 0;
   for (i = 0;
        i < TUP_HEADER_SIZE;
        tuple_root->t_child[i++].t_cell = NULL);
   tuple_length = 0;
   expansion_trigger = TUP_HEADER_SIZE;

   /* set up to loop over the tuple */

   source_root = process_ptr->pc_wait_key.sp_val.sp_tuple_ptr;
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

         source_work_hdr =
            source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      /* if we've exhausted the tuple, we're done */

      if (source_element == NULL)
         break;

      /* get the mailbox value */

      mailbox_ptr = source_element->sp_val.sp_mailbox_ptr;
      mailbox_cell = mailbox_ptr->mb_head;
   
      /*
       *  Now we have to insert the value into the return tuple.
       */

      /* expand the tuple tree if necessary */

      if (tuple_length >= expansion_trigger) {

         tuple_work_hdr = tuple_root;

         get_tuple_header(tuple_root);

         tuple_root->t_use_count = 1;
         tuple_root->t_hash_code =
            tuple_work_hdr->t_hash_code;
         tuple_root->t_ntype.t_root.t_length =
            tuple_work_hdr->t_ntype.t_root.t_length;
         tuple_root->t_ntype.t_root.t_height =
            tuple_work_hdr->t_ntype.t_root.t_height + 1;

         for (i = 1;
              i < TUP_HEADER_SIZE;
              tuple_root->t_child[i++].t_header = NULL);

         tuple_root->t_child[0].t_header = tuple_work_hdr;

         tuple_work_hdr->t_ntype.t_intern.t_parent = tuple_root;
         tuple_work_hdr->t_ntype.t_intern.t_child_index = 0;

         expansion_trigger *= TUP_HEADER_SIZE;

      }

      tuple_root->t_ntype.t_root.t_length++;

      /* descend the tree to a leaf */

      tuple_work_hdr = tuple_root;
      for (tuple_height = tuple_work_hdr->t_ntype.t_root.t_height;
           tuple_height;
           tuple_height--) {

         /* extract the element's index at this level */

         tuple_index = (tuple_length >>
                              (tuple_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (tuple_work_hdr->t_child[tuple_index].t_header == NULL) {

            get_tuple_header(new_tuple_hdr);
            new_tuple_hdr->t_ntype.t_intern.t_parent = tuple_work_hdr;
            new_tuple_hdr->t_ntype.t_intern.t_child_index = tuple_index;
            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_tuple_hdr->t_child[i++].t_cell = NULL);
            tuple_work_hdr->t_child[tuple_index].t_header =
                  new_tuple_hdr;
            tuple_work_hdr = new_tuple_hdr;

         }
         else {

            tuple_work_hdr =
               tuple_work_hdr->t_child[tuple_index].t_header;

         }
      }

      /*
       *  At this point, tuple_work_hdr points to the lowest level header
       *  record.  We insert the new element.
       */

      tuple_index = tuple_length & TUP_SHIFT_MASK;
      get_tuple_cell(tuple_cell);
      tuple_cell->t_spec.sp_form = 
         mailbox_cell->mb_spec.sp_form;     
      tuple_cell->t_spec.sp_val.sp_biggest =
         mailbox_cell->mb_spec.sp_val.sp_biggest;     
      mark_specifier(&(tuple_cell->t_spec));
      spec_hash_code(tuple_cell->t_hash_code,
                     &(tuple_cell->t_spec));
      tuple_root->t_hash_code ^= tuple_cell->t_hash_code;
      tuple_work_hdr->t_child[tuple_index].t_cell = tuple_cell;

      /* increment the tuple size */

      tuple_length++;

      /* pop the value from the mailbox */

      if (process_ptr->pc_waiting) {
         unmark_specifier(&(mailbox_cell->mb_spec));
         mailbox_ptr->mb_head = mailbox_cell->mb_next;
         if (mailbox_ptr->mb_head == NULL)
            mailbox_ptr->mb_tail = &(mailbox_ptr->mb_head);
         mailbox_ptr->mb_cell_count--;
         free_mailbox_cell(mailbox_cell);
      }
   }

   /* stick the result on the process record */

   tuple_root->t_ntype.t_root.t_length = tuple_length;
   process_ptr->pc_wait_return.sp_form = ft_tuple;
   process_ptr->pc_wait_return.sp_val.sp_tuple_ptr = tuple_root;

   unmark_specifier(&(process_ptr->pc_wait_key));
   return YES;

}

/* back to normal indentation */

   }

   return NO;

}
