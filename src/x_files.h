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
 *  \packagespec{Files}
\*/

#ifndef FILES_LOADED

/* constants */

#define FILE_BUFF_SIZE  256            /* buffer size for text input files  */
#define MAX_LOOKAHEAD    32            /* lookahead for above               */

/* file node structure */

struct file_item {
   int f_type;                         /* file type for new I/O             */
   int f_flag;                         /* field used in new I/O             */
   int f_mode;                         /* file access mode                  */
   char f_file_name[PATH_LENGTH + 1];  /* file name                         */
   int f_samerun;                      /* YES if binary input file was      */
                                       /* created in this run               */
   int f_file_fd;                      /* fd for unbuffered I/O             */
   FILE *f_file_stream;                /* text output handle                */
   unsigned char *f_file_buffer;       /* character buffer for text input   */
                                       /* files                             */
   unsigned char *f_start;             /* start pointer for buffer          */
   unsigned char *f_endofbuffer;       /* end of buffer pointer             */
   unsigned char *f_eof_ptr;           /* end of file pointer               */
};

typedef struct file_item *file_ptr_type;
                                       /* file node pointer                 */

/* global data */

#ifdef TSAFE
#define FILE_NEXT_FREE plugin_instance->file_next_free
#else
#define FILE_NEXT_FREE file_next_free
#ifdef SHARED

file_ptr_type file_next_free = NULL;   /* next free file                    */

#else

extern file_ptr_type file_next_free;   /* next free file                    */

#endif
#endif

/* allocate and free file nodes */

#ifdef HAVE_MPATROL

#define get_file(t) {\
   t = (file_ptr_type)malloc(sizeof(struct file_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_file(s) free(s)

#else

#define get_file(t) {\
   if (FILE_NEXT_FREE == NULL) alloc_files(SETL_SYSTEM_VOID); \
   t = FILE_NEXT_FREE; \
   FILE_NEXT_FREE = *((file_ptr_type *)(FILE_NEXT_FREE)); \
}

#define free_file(s) {\
   *((file_ptr_type *)(s)) = FILE_NEXT_FREE; \
   FILE_NEXT_FREE = s; \
}

#endif

/* public function declarations */

void alloc_files(SETL_SYSTEM_PROTO_VOID);                
                                       /* allocate a block of file nodes    */

#define FILES_LOADED 1
#endif


