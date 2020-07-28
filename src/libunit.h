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
 *  \packagespec{Library Unit Table}
\*/

#ifndef LIBUNIT_LOADED

/* SETL2 system header files */

#include "libcom.h"                    /* library manager -- common         */

/* library unit table item */

struct libunit_item {
   struct libunit_item *lu_next;       /* next unit in list                 */
   struct unit_header_item lu_header;  /* unit header structure             */
   struct libfile_item *lu_libfile_ptr;
                                       /* file containing unit              */
   struct libstr_item *lu_libstr_list; /* list of open streams in unit      */
   unsigned lu_is_output : 1;          /* YES if unit opened output         */
};

/* macro to clear one table item */

#define clear_unit(u) { \
   (u)->lu_next = NULL;                (u)->lu_libfile_ptr = NULL; \
   (u)->lu_libstr_list = NULL;         (u)->lu_is_output = 0; \
}

/* public function declarations */

struct libunit_item *get_libunit(SETL_SYSTEM_PROTO_VOID);
                                       /* allocate a new table item         */
void free_libunit(struct libunit_item *);
                                       /* deallocate a table item           */

#define LIBUNIT_LOADED 1
#endif

