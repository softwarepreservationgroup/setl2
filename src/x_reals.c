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
 *  \package{Real Numbers}
 *
 *  This package contains definitions of the structures used to implement
 *  SETL2 reals, and several low level functions to manipulate those
 *  structures.  We freely confess that we used a very ugly, non-portable
 *  C coding style here, in hopes of getting a fast implementation.  We
 *  have tried to isolate this ugliness to the macros and functions which
 *  allocate and release nodes.  In particular, there are some unsafe
 *  castes, so please make sure this file is compiled with unsafe
 *  optimizations disabled!
 *
 *  \texify{reals.h}
 *
 *  \packagebody{Real Numbers}
\*/

/* standard C header files */
#include <stdio.h>
#ifndef HAVE_SIGNAL_H
#include <signal.h>                    /* signal macros                     */
#endif

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_reals.h"                   /* real numbers                      */

/* performance tuning constants */

#define REAL_BLOCK_SIZE        100     /* real block size                   */

/*\
 *  \function{math\_error()}
 *
 *  This function is installed as the floating point error handler.  It
 *  just calls \verb"abend(SETL_SYSTEM )".
\*/

static void math_error(
   int interrupt_num)

{
/* What a mess again!!! */

#ifdef TSAFE
   abend(NULL,msg_float_error);
#else
   abend(msg_float_error);
#endif
}

#if INFNAN

void x_infnan()

{

/* What a mess again!!! */

#ifdef TSAFE
   abend(NULL,msg_float_error);
#else
   abend(msg_float_error);
#endif

}

#endif

/*\
 *  \function{init\_interp\_reals()}
 *
 *  This function initializes the real numbers.  All we do is install a
 *  procedure as error handler, for floating point errors.
\*/

void init_interp_reals(SETL_SYSTEM_PROTO_VOID)

{

   /* set ^C trap */
#ifdef HAVE_SIGNAL_H
   if (signal(SIGFPE,math_error) == SIG_ERR)
      giveup(SETL_SYSTEM msg_trap_float);
#endif

}

/*\
 *  \function{alloc\_reals()}
 *
 *  This function allocates a block of real numbers and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste real items to pointers to real items in order to form the free
 *  list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the real node.
 *  It will work provided a real item is larger than a pointer, which is
 *  the case.
\*/

void i_alloc_reals(SETL_SYSTEM_PROTO_VOID)

{
i_real_ptr_type new_block;             /* first real in allocated block     */

   /* allocate a new block */

   new_block = (i_real_ptr_type)malloc((size_t)
         (REAL_BLOCK_SIZE * sizeof(struct i_real_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (REAL_NEXT_FREE = new_block;
        REAL_NEXT_FREE < new_block + REAL_BLOCK_SIZE - 1;
        REAL_NEXT_FREE++) {

      *((i_real_ptr_type *)REAL_NEXT_FREE) = REAL_NEXT_FREE + 1;

   }
   *((i_real_ptr_type *)REAL_NEXT_FREE) = NULL;

   /* set next free node to new block */

   REAL_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{copy\_real()}
 *
 *  This function copies a real item, usually in preparation for
 *  destructive use.
\*/

i_real_ptr_type copy_real(
   SETL_SYSTEM_PROTO
   i_real_ptr_type source)             /* real to be copied                 */

{
i_real_ptr_type target;                /* returned real item                */

   i_get_real(target);
   target->r_value = source->r_value;
   target->r_use_count = 1;

   return target;

}

/*\
 *  \function{real\_value()}
 *
 *  This function just returns the value of a real.  It would be easy for
 *  the caller to access this field directly, but we would like to hide
 *  the real structures from built-in functions.
\*/

double i_real_value(
   specifier *spec)                    /* specifier                         */

{

   return (spec->sp_val.sp_real_ptr)->r_value;

}

