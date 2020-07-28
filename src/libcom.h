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
 *  \packagespec{Library Manager --- Common}
 *
 *  The specification of the library manager is really split into two
 *  header files.  This header file contains anything used exclusively
 *  within the library manager, but in more than one file of the library
 *  manager, and another file which contains anything used by files
 *  outside the library manager which access it.  It is one of those
 *  unusual situations in which we would like to have the full block
 *  structure of Ada, rather than the two level name space provided by C.
\*/

#ifndef LIBCOM_LOADED

#define LIB_BLOCK_SIZE     512         /* size of library file block        */
#define LIB_HASH_SIZE      ((504 - sizeof(unsigned)) / sizeof(unsigned))
                                       /* header hash table size            */
#define LIB_DATA_SIZE      (LIB_BLOCK_SIZE - sizeof(unsigned))
                                       /* size of logical data record       */
#define LIB_ID             "S2~Lb22"   /* good library identifier           */
#define LIB_STREAM_COUNT   15          /* number of different data types    */
#define LIB_MAX_OPEN       10          /* maximum number of libraries       */
                                       /* allowed to be open                */

/* type definitions */

typedef struct libfree_item *libfree_ptr_type;
                                       /* pointer to libfree record         */

/*
 *  unit header format --- This must be in the common file, since it is
 *  part of a unit table node, and must be used by both the main library
 *  manager file and the unit table file.
 */

typedef struct unit_header_item {
   char uh_name[MAX_UNIT_NAME + 1];    /* unit name                         */
   int32 uh_data_length[LIB_STREAM_COUNT];
                                       /* size of each stream               */
   int uh_data_head[LIB_STREAM_COUNT]; /* data list pointers                */
   int uh_data_tail[LIB_STREAM_COUNT]; /* tail pointers of above            */
} unit_header;

#define LIBCOM_LOADED 1
#endif
