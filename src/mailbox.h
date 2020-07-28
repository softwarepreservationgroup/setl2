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
 *  \packagespec{Mailbox Table}
\*/

#ifndef MAILBOX_LOADED

/* mailbox header node structure */

struct mailbox_h_item {
   int32 mb_use_count;                 /* usage count                       */
   int32 mb_cell_count;                /* number of cells                   */
   struct mailbox_c_item *mb_head;     /* first cell in list                */
   struct mailbox_c_item **mb_tail;    /* last cell in list                 */
};

typedef struct mailbox_h_item *mailbox_h_ptr_type;
                                       /* header node pointer               */

/* mailbox cell node structure */

struct mailbox_c_item {
   struct mailbox_c_item *mb_next;     /* next cell in list                 */
   struct specifier_item mb_spec;      /* cell value                        */
};

typedef struct mailbox_c_item *mailbox_c_ptr_type;
                                       /* cell node pointer                 */

/* global data */

#ifdef TSAFE
#define MAILBOX_H_NEXT_FREE plugin_instance->mailbox_h_next_free 
#define MAILBOX_C_NEXT_FREE plugin_instance->mailbox_c_next_free 
#else
#define MAILBOX_H_NEXT_FREE mailbox_h_next_free 
#define MAILBOX_C_NEXT_FREE mailbox_c_next_free 

#ifdef SHARED
       
mailbox_h_ptr_type mailbox_h_next_free = NULL;
                                       /* next free header                  */
mailbox_c_ptr_type mailbox_c_next_free = NULL;
                                       /* next free cell                    */

#else

extern mailbox_h_ptr_type mailbox_h_next_free;
                                       /* next free header                  */
extern mailbox_c_ptr_type mailbox_c_next_free;
                                       /* next free cell                    */

#endif
#endif

/* allocate and free header nodes */

#define get_mailbox_header(t) {\
   if (MAILBOX_H_NEXT_FREE == NULL) alloc_mailbox_headers(SETL_SYSTEM_VOID); \
   (t) = MAILBOX_H_NEXT_FREE; \
   MAILBOX_H_NEXT_FREE = *((mailbox_h_ptr_type *)(MAILBOX_H_NEXT_FREE)); \
}

#define free_mailbox_header(s) {\
   *((mailbox_h_ptr_type *)(s)) = MAILBOX_H_NEXT_FREE; \
   MAILBOX_H_NEXT_FREE = s; \
}

/* allocate and free cell nodes */

#define get_mailbox_cell(t) {\
   if (MAILBOX_C_NEXT_FREE == NULL) alloc_mailbox_cells(SETL_SYSTEM_VOID); \
   (t) = MAILBOX_C_NEXT_FREE; \
   MAILBOX_C_NEXT_FREE = *((mailbox_c_ptr_type *)(MAILBOX_C_NEXT_FREE)); \
}

#define free_mailbox_cell(s) {\
   *((mailbox_c_ptr_type *)(s)) = MAILBOX_C_NEXT_FREE; \
   MAILBOX_C_NEXT_FREE = s; \
}

/* public function declarations */

void alloc_mailbox_headers(SETL_SYSTEM_PROTO_VOID);      
                                       /* allocate a block of header nodes  */
void alloc_mailbox_cells(SETL_SYSTEM_PROTO_VOID);        
                                       /* allocate a block of cell nodes    */
void free_mailbox(SETL_SYSTEM_PROTO mailbox_h_ptr_type); 
                                       /* release memory used by mailbox    */

#define MAILBOX_LOADED 1
#endif


