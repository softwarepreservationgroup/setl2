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
 *  \packagespec{Library Stream Table}
\*/

#ifndef LIBSTR_LOADED

/* SETL2 system header files */

#include "libcom.h"                    /* library manager -- common         */

/* library stream table item */

struct libstr_item {
   struct libstr_item *ls_next;        /* next stream in list               */
   struct libunit_item *ls_libunit_ptr;
                                       /* unit containing stream            */
   int ls_index;                       /* stream index                      */
   int ls_record_num;                  /* current record number             */
   char ls_buffer[LIB_DATA_SIZE];      /* input/output buffer               */
   char *ls_buff_cursor;               /* next to be read/written           */
   int32 ls_bytes_left;                /* bytes left in stream              */
};

/* macro to clear one table item */

#define clear_libstr(s) { \
   (s)->ls_next = NULL;                (s)->ls_libunit_ptr = NULL; \
   (s)->ls_index = -1;                 (s)->ls_record_num = -1; \
   (s)->ls_buffer[0] = '\0';           (s)->ls_buff_cursor = NULL; \
   (s)->ls_bytes_left = 0; \
}

/* public function declarations */

struct libstr_item *get_libstr(SETL_SYSTEM_PROTO_VOID);  /* allocate a new table item         */
void free_libstr(struct libstr_item *);
                                       /* deallocate a table item           */

#define LIBSTR_LOADED 1
#endif

