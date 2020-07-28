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
 *  \packagespec{File Names}
\*/

#ifndef FILENAME_LOADED

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */

/* file node in file list */

struct filelist_item {
   char fl_name[PATH_LENGTH + 1];      /* expanded file name                */
   struct filelist_item *fl_next;      /* next name in list                 */
};

typedef struct filelist_item *filelist_ptr_type;
                                       /* file list pointer typedef         */

/* public function declarations */

void expand_filename(SETL_SYSTEM_PROTO char *);          
				       /* prepend drive / path to name      */
filelist_ptr_type setl_get_filelist(SETL_SYSTEM_PROTO char *);
                                       /* get a list of files matching a    */
                                       /* list of specifications            */
void setl_free_filelist(filelist_ptr_type); /* free a file list                  */
void get_tempname(SETL_SYSTEM_PROTO char *, char*);      
                                       /* find a temporary file name        */

#define FILENAME_LOADED 1

#endif

