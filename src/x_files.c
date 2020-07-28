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
 *  \package{Files}
 *
 *  This package contains definitions of the structures used to implement
 *  SETL2 files, and several low level functions to manipulate those
 *  structures.  We freely confess that we used a very ugly, non-portable
 *  C coding style here, in hopes of getting a fast implementation.  We
 *  have tried to isolate this ugliness to the macros and functions which
 *  allocate and release nodes.  In particular, there are some unsafe
 *  castes, so please make sure this file is compiled with unsafe
 *  optimizations disabled!
 *
 *  \texify{files.h}
 *
 *  \packagebody{Files}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "x_files.h"                     /* files                             */

/* performance tuning constants */

#define FILE_BLOCK_SIZE        10      /* file block size                   */

/*\
 *  \function{alloc\_files()}
 *
 *  This function allocates a block of files and links them together
 *  into a free list.  Note carefully the castes used here: we caste
 *  file items to pointers to file items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the file
 *  node.  It will work provided a file item is larger than a
 *  pointer, which is the case.
\*/

void alloc_files(
   SETL_SYSTEM_PROTO_VOID)

{
file_ptr_type new_block;               /* first file in allocated           */
                                       /* block                             */

   /* allocate a new block */

   new_block = (file_ptr_type)malloc((size_t)
         (FILE_BLOCK_SIZE * sizeof(struct file_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (FILE_NEXT_FREE = new_block;
        FILE_NEXT_FREE < new_block + FILE_BLOCK_SIZE - 1;
        FILE_NEXT_FREE++) {

      *((file_ptr_type *)FILE_NEXT_FREE) = FILE_NEXT_FREE + 1;

   }

   *((file_ptr_type *)FILE_NEXT_FREE) = NULL;

   /* set next free node to new block */

   FILE_NEXT_FREE = new_block;

   return;

}

