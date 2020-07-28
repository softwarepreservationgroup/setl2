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
 *  \package{The Procedure Table}
 *
 *  This package contains definitions of the structures used to implement
 *  SETL2 procedures, and several low level functions to manipulate those
 *  structures.  We freely confess that we used a very ugly, non-portable
 *  C coding style here, in hopes of getting a fast implementation.  We
 *  have tried to isolate this ugliness to the macros and functions which
 *  allocate and release nodes.  In particular, there are some unsafe
 *  castes, so please make sure this file is compiled with unsafe
 *  optimizations disabled!
 *
 *  \texify{procs.h}
 *
 *  \packagebody{Procedure Table}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "specs.h"                     /* specifiers                        */
#include "procs.h"                     /* procedures                        */
#include "objects.h"                   /* objects                           */

/* performance tuning constants */

#define PROC_BLOCK_SIZE        100     /* procedure block size              */

/*\
 *  \function{alloc\_procs()}
 *
 *  This function allocates a block of procedures and links them together
 *  into a free list.  Note carefully the castes used here: we caste
 *  procedure items to pointers to procedure items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the procedure
 *  node.  It will work provided a procedure item is larger than a
 *  pointer, which is the case.
\*/

void alloc_procs(SETL_SYSTEM_PROTO_VOID)

{
proc_ptr_type new_block;               /* first procedure in allocated      */
                                       /* block                             */

   /* allocate a new block */

   new_block = (proc_ptr_type)malloc((size_t)
         (PROC_BLOCK_SIZE * sizeof(struct proc_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (PROC_NEXT_FREE = new_block;
        PROC_NEXT_FREE < new_block + PROC_BLOCK_SIZE - 1;
        PROC_NEXT_FREE++) {

      *((proc_ptr_type *)PROC_NEXT_FREE) = PROC_NEXT_FREE + 1;

   }
   *((proc_ptr_type *)PROC_NEXT_FREE) = NULL;

   /* set next free node to new block */

   PROC_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{free\_procedure()}
 *
 *  This function frees a procedure, decrementing its parent's use count.
 *  If the parent's use count goes to zero, it then frees the parent.
\*/

void free_procedure(
   SETL_SYSTEM_PROTO
   proc_ptr_type proc_ptr)             /* procedure to be deleted           */

{
specifier *spec_ptr;                   /* pointer to specifier              */

   /* free the variable swap area */

   if (proc_ptr->p_save_specs != NULL) {

      for (spec_ptr = proc_ptr->p_save_specs;
           spec_ptr < proc_ptr->p_save_specs + proc_ptr->p_spec_count;
           spec_ptr++) {

         unmark_specifier(spec_ptr);

      }

      free((void *)(proc_ptr->p_save_specs));
      proc_ptr->p_save_specs = NULL;

   }

   /* decrement parent's use count */

   if (proc_ptr->p_parent != NULL &&
         !(--(proc_ptr->p_parent)->p_use_count)) {
      free_procedure(SETL_SYSTEM proc_ptr->p_parent);
   }

   /* decrement self's use count */

   if (proc_ptr->p_self_ptr != NULL &&
         !(--(proc_ptr->p_self_ptr)->o_use_count)) {
      free_object(SETL_SYSTEM proc_ptr->p_self_ptr);
      proc_ptr->p_self_ptr = NULL;
   }

   /* free node */

   if (!proc_ptr->p_is_const)
      free_proc(proc_ptr);

}
