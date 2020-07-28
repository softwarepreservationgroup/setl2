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
 *  \packagespec{Library Free List Table}
\*/

#ifndef LIBFREE_LOADED

/* free libfree structure */

struct libfree_item {
   int lf_head;                        /* first record in free list         */
   int lf_tail;                        /* last record in free list          */
   struct libfree_item *lf_next;       /* next free list record             */
};

/* clear free list record */

#define clear_libfree(l) { \
   (l)->lf_head = -1;                  (l)->lf_tail = -1; \
   (l)->lf_next = NULL; \
}

/* public function declarations */

#ifdef LIBWRITE

struct libfree_item *get_libfree(SETL_SYSTEM_PROTO_VOID);
                                       /* allocate free list                */
void free_libfree(libfree_ptr_type);   /* return to free pool               */

#endif

#define LIBFREE_LOADED 1
#endif

