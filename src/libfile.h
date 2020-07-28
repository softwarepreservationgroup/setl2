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
 *  \packagespec{Library File Table}
\*/

#ifndef LIBFILE_LOADED

/* library file table item */

struct libfile_item {
   struct libfile_item *lf_next;       /* next file in list                 */
   char lf_fname[PATH_LENGTH + 1];     /* library file name                 */
   int32 lf_header_pos;                /* position of library header        */
   struct lib_header_item *lf_header;  /* library header (allocated with    */
                                       /* malloc() )                        */
   struct libfree_item *lf_libfree_list;
                                       /* list of free chains               */
   int lf_next_free;                   /* next free record in library       */
   struct libunit_item *lf_libunit_list;
                                       /* list of open units in library     */
   FILE *lf_stream;                    /* file stream                       */
   unsigned lf_is_writeable : 1;       /* YES if the file can be written to */
   unsigned lf_is_open : 1;            /* YES if the file is open           */
   unsigned lf_mem_lib : 1;            /* YES if library is in memory       */
};

/* macro to clear one table item */

#define clear_libfile(f) { \
   (f)->lf_next = NULL;                (f)->lf_fname[0] = '\0'; \
   (f)->lf_header_pos = 0;             (f)->lf_header = NULL; \
   (f)->lf_libfree_list = NULL;        (f)->lf_next_free = -1; \
   (f)->lf_libunit_list = NULL;        (f)->lf_stream = NULL; \
   (f)->lf_mem_lib =0; \
   (f)->lf_is_writeable = 0;           (f)->lf_is_open = 0; \
}

/* public function declarations */

struct libfile_item *get_libfile(SETL_SYSTEM_PROTO_VOID);
                                       /* allocate a new table item         */
void free_libfile(struct libfile_item *);
                                       /* deallocate a table item           */

#define LIBFILE_LOADED 1
#endif

