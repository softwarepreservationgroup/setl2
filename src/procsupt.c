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
 *  \package{Process-Oriented Built-In Procedures}
 *
 *  In this package we have built-in procedures meant to support
 *  processes.  They control suspension, killing, waiting, and mailboxes.
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "builtins.h"                  /* built-in symbols                  */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "procs.h"                     /* procedures                        */
#include "objects.h"                   /* objects                           */
#include "process.h"                   /* objects                           */
#include "mailbox.h"                   /* objects                           */

/* local procedures */

#ifdef PROCESSES
static void aload(int, specifier *, specifier *);
                                       /* common to wait and check          */
#endif

/*\
 *  \function{setl2\_suspend()}
 *
 *  This function is the \verb"suspend" built-in function.  The argument
 *  must be a process.  We just flag it as suspended.
\*/

void setl2_suspend(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
#ifdef PROCESSES
process_ptr_type process_ptr;          /* process to be suspended           */

   /* make sure we were passed a process */

   if (argv->sp_form != ft_process ||
       argv->sp_val.sp_object_ptr->o_process_ptr == NULL) {

      abend(SETL_SYSTEM "Invalid argument to suspend\nArg => %s",
            abend_opnd_str(SETL_SYSTEM argv));
   
   }

   /* flag it suspended */

   process_ptr = argv->sp_val.sp_object_ptr->o_process_ptr;
   process_ptr->pc_suspended = YES;

   /* force a switch if argument is current process */

   if (process_ptr == process_head) 
      opcodes_until_switch = 0;

   /* always return OM */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;
#endif

}

/*\
 *  \function{setl2\_resume()}
 *
 *  This function is the \verb"resume" built-in function.  The argument
 *  must be a process.  We just clear the suspend flag.
\*/

void setl2_resume(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
#ifdef PROCESSES
process_ptr_type process_ptr;          /* process to be resumed             */

   /* make sure we were passed a process */

   if (argv->sp_form != ft_process ||
       argv->sp_val.sp_object_ptr->o_process_ptr == NULL) {

      abend(SETL_SYSTEM "Invalid argument to resume\nArg => %s",
            abend_opnd_str(SETL_SYSTEM argv));
   
   }

   /* clear the suspend flag */

   process_ptr = argv->sp_val.sp_object_ptr->o_process_ptr;
   process_ptr->pc_suspended = NO;

   /* always return OM */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

#endif
}

/*\
 *  \function{setl2\_kill()}
 *
 *  This function is the \verb"kill" built-in function.  The argument
 *  must be a process.  We just clear whatever it might be doing.
\*/

void setl2_kill(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
#ifdef PROCESSES
process_ptr_type process_ptr;          /* process to be suspended           */
request_ptr_type request_ptr;          /* used to loop over requests.       */
request_ptr_type old_request_ptr;      /* previous request                  */
mailbox_h_ptr_type mailbox_ptr;        /* insert into this mailbox          */
mailbox_c_ptr_type mailbox_cell;       /* mailbox cell pointer              */
struct specifier_item *arg_ptr;        /* work argument pointer             */
proc_ptr_type proc_ptr;                /* procedure pointer                 */
int32 arg_count;                       /* number of procedure arguments     */
int32 i;                               /* temporary looping variable        */

   /* make sure we were passed a process */

   if (argv->sp_form != ft_process ||
       argv->sp_val.sp_object_ptr->o_process_ptr == NULL) {

      abend(SETL_SYSTEM "Invalid argument to kill\nArg => %s",
            abend_opnd_str(SETL_SYSTEM argv));
   
   }

   process_ptr = argv->sp_val.sp_object_ptr->o_process_ptr;

   /* if the process is waiting, clear the wait key */

   if (process_ptr->pc_waiting ||
       process_ptr->pc_checking) {

      unmark_specifier(&(process_ptr->pc_wait_key));
      process_ptr->pc_waiting = NO;
      process_ptr->pc_checking = NO;

   }

   /* clear all pending requests */

   request_ptr = process_ptr->pc_request_head;
   while (request_ptr != NULL) {

      /* insert an OM in the associated mailbox */

      mailbox_ptr = request_ptr->rq_mailbox_ptr;

      get_mailbox_cell(mailbox_cell);
      *(mailbox_ptr->mb_tail) = mailbox_cell;
      mailbox_ptr->mb_tail = &(mailbox_cell->mb_next);
      mailbox_cell->mb_next = NULL;

      mailbox_ptr->mb_cell_count++;
      mailbox_cell->mb_spec.sp_form = ft_omega;
      mailbox_ptr->mb_use_count++;
   
      /* if we haven't started on the request, free the arguments */

      if (process_ptr->pc_idle) {

         proc_ptr = request_ptr->rq_proc;
         arg_count = proc_ptr->p_formal_count;

         for (i = 0; i < arg_count; i++) {
            unmark_specifier(&((request_ptr->rq_args)[i]));
            (request_ptr->rq_args)[i].sp_form = ft_omega;
         }
      }
      else {

         process_ptr->pc_idle = YES;
         proc_ptr = request_ptr->rq_proc;
         for (arg_ptr = proc_ptr->p_spec_ptr;
              arg_ptr < proc_ptr->p_spec_ptr + proc_ptr->p_spec_count;
              arg_ptr++) {

            unmark_specifier(arg_ptr);
            arg_ptr->sp_form = ft_omega;

         }
      }

      /* remove the request record */

      process_ptr->pc_request_head = request_ptr->rq_next;
      if (process_ptr->pc_request_head == NULL)
         process_ptr->pc_request_tail = &(process_ptr->pc_request_head);         
      free((void *)(request_ptr->rq_args));
      free_request(request_ptr);

      request_ptr = process_ptr->pc_request_head;

   }

   /* clear everything from the stacks */

   while (process_ptr->pc_pstack_top > 0) {
      
      unmark_specifier(&(process_ptr->pc_pstack[process_ptr->pc_pstack_top]));
      process_ptr->pc_pstack_top--;

   }
   process_ptr->pc_cstack_top = 0;

   /* the process is now idle */

   process_ptr->pc_idle = YES;
   process_ptr->pc_waiting = NO;
   process_ptr->pc_checking = NO;

   /* force a switch if argument is current process */

   if (process_ptr == process_head) 
      opcodes_until_switch = 0;

   /* always return OM */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

#endif
}

/*\
 *  \function{setl2\_newmbox()}
 *
 *  This function is the \verb"newmbox" built-in function.  It just
 *  creates and returns an empty mailbox.
\*/

void setl2_newmbox(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
#ifdef PROCESSES
mailbox_h_ptr_type mailbox_ptr;        /* returned mailbox header           */

   get_mailbox_header(mailbox_ptr);
   mailbox_ptr->mb_use_count = 1;
   mailbox_ptr->mb_cell_count = 0;
   mailbox_ptr->mb_head = NULL;
   mailbox_ptr->mb_tail = &(mailbox_ptr->mb_head);
   
   unmark_specifier(target);
   target->sp_form = ft_mailbox;
   target->sp_val.sp_mailbox_ptr = mailbox_ptr;

   return;

#endif
}

/*\
 *  \function{setl2\_await}
 *
 *  The \verb"await" procedure waits for a mailbox value or a group of
 *  mailbox values, depending on the type of the argument.  We actually
 *  do minimal work here.  Much of the logic for \verb"await" and
 *  \verb"acheck" is identical, so we do that in the \verb"aload"
 *  function which follows both.
 *
 *  I should say that the `real' work for both of these functions is done
 *  much later, when we try to switch processes.
\*/

void setl2_await(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
#ifdef PROCESSES

   /* check the argument types */

   aload(argc,argv,target);

   /* save the key on the process record */

   mark_specifier(argv);
   process_head->pc_wait_key.sp_form = argv->sp_form;
   process_head->pc_wait_key.sp_val.sp_biggest = argv->sp_val.sp_biggest;

   /* flag the process as waiting and save the real target */

   process_head->pc_waiting = YES;
   process_head->pc_wait_target = ex_wait_target;

   opcodes_until_switch = 0;

   /* this is a dummy return of OM */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

#endif
}

/*\
 *  \function{setl2\_acheck}
 *
 *  The \verb"acheck" procedure waits for a mailbox value or a group of
 *  mailbox values, depending on the type of the argument.  We actually
 *  do minimal work here.  Much of the logic for \verb"acheck" and
 *  \verb"await" is identical, so we do that in the \verb"aload"
 *  function which follows both.
 *
 *  I should say that the `real' work for both of these functions is done
 *  much later, when we try to switch processes.
\*/

void setl2_acheck(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
#ifdef PROCESSES

   /* check the argument types */

   aload(argc,argv,target);

   /* save the key on the process record */

   mark_specifier(argv);
   process_head->pc_wait_key.sp_form = argv->sp_form;
   process_head->pc_wait_key.sp_val.sp_biggest = argv->sp_val.sp_biggest;

   /* flag the process as waiting and save the real target */

   process_head->pc_checking = YES;
   process_head->pc_wait_target = ex_wait_target;

   opcodes_until_switch = 0;

   /* this is a dummy return of OM */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

#endif
}

/*\
 *  \function{aload}
 *
 *  This function checks the argument for both \verb"await" and
 *  \verb"acheck".
\*/
#ifdef PROCESSES

static void aload(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   switch (argv->sp_form) {

/*\
 *  \case{mailboxes}
 *
 *  Hopefully, the most common use of this function will be to wait for a
 *  single mailbox.
\*/

case ft_mailbox :

{

   return;

}

/*\
 *  \case{sets}
 *
 *  Each element of the set must be a mailbox.
\*/

case ft_set :

{
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */

   source_root = argv->sp_val.sp_set_ptr;
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

      if (source_element->sp_form != ft_mailbox) {

         abend(SETL_SYSTEM "Invalid argument to wait\nArg => %s",
               abend_opnd_str(SETL_SYSTEM argv));

      }
   }

   return;

}

/*\
 *  \case{tuples}
 *
 *  We loop over the elements of a tuple printing each. When we encounter
 *  a string we print enclosing quotes.
\*/

case ft_tuple :

{
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */

   source_root = argv->sp_val.sp_tuple_ptr;
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

      if (source_element == NULL)
         break;

      if (source_element->sp_form != ft_mailbox) {

         abend(SETL_SYSTEM "Invalid argument to wait\nArg => %s",
               abend_opnd_str(SETL_SYSTEM argv));

      }
   }

   return;

}

/*\
 *  \case{errors}
 *
 *  Oops. That's all we can wait on.
\*/

default :

{

   abend(SETL_SYSTEM "Invalid argument to wait\nArg => %s",
         abend_opnd_str(SETL_SYSTEM argv));

}

/* back to normal indentation */

   }

   return;

}
#endif

/*\
 *  \function{setl2\_pass()}
 *
 *  This function is the \verb"pass" built-in function.  It just
 *  abandons a time slice.
\*/

void setl2_pass(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{
#ifdef PROCESSES

   opcodes_until_switch = 0;

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;
#endif

}


