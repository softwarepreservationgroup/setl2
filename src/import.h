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
 *  \packagespec{Imported Package Table}
\*/

#ifndef IMPORT_LOADED

/* import table node structure */

struct import_item {
   struct import_item *im_next;        /* next item in list                 */
   struct namtab_item *im_namtab_ptr;  /* package name                      */
   int im_unit_num;                    /* unit number of imported unit      */
   char im_source_name[PATH_LENGTH + 1];
                                       /* package source file name          */
   time_t im_time_stamp;               /* compilation time                  */
   int im_inherited;                   /* YES if unit is inherited          */
   struct symtab_item *im_symtab_ptr;  /* symbol for unit                   */
};

typedef struct import_item *import_ptr_type;
                                       /* node pointer                      */

/* clear an import table item */

#define clear_import(i) { \
   (i)->im_next = NULL;                (i)->im_namtab_ptr = NULL; \
   (i)->im_unit_num = -1;              (i)->im_source_name[0] = '\0'; \
   (i)->im_time_stamp = -1;            (i)->im_inherited = 0; \
}

/* public function declarations */

void init_import(void);                /* initialize the imported package   */
                                       /* table                             */
import_ptr_type get_import(SETL_SYSTEM_PROTO_VOID);
                                       /* allocate a new item               */
void free_import(import_ptr_type);     /* return an item to the free pool   */

#define IMPORT_LOADED 1
#endif
