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
 *  \packagespec{Tuples}
\*/

#ifndef TUPLES_LOADED

/* SETL2 system header files */

#include "specs.h"                     /* specifiers                        */

/* performance tuning constants */

#define TUP_HEADER_SIZE    4           /* tuple header size (table)         */
#define TUP_SHIFT_DIST     2           /* log (base 2) of TUP_HEADER_SIZE   */
#define TUP_SHIFT_MASK  0x03           /* pick out one level of header tree */

/* tuple header node structure */

struct tuple_h_item {
   int32 t_use_count;                  /* usage count                       */
   int32 t_hash_code;                  /* hash code                         */
   union {
      struct {                         /* root header info                  */
         int32 t_length;               /* number of elements in tuple       */
         int t_height;                 /* height of header tree             */
      } t_root;
      struct {                         /* internal node info                */
         struct tuple_h_item *t_parent;
                                       /* parent in header tree             */
         int t_child_index;            /* index in parent's table           */
      } t_intern;
   } t_ntype;
   union {
      struct tuple_c_item *t_cell;     /* cell child pointer                */
      struct tuple_h_item *t_header;   /* internal header node pointer      */
   } t_child[TUP_HEADER_SIZE];
};

typedef struct tuple_h_item *tuple_h_ptr_type;
                                       /* header node pointer               */

/* tuple cell node structure */

struct tuple_c_item {
   int32 t_hash_code;                  /* full hash code of element         */
   struct specifier_item t_spec;       /* tuple element specifier           */
};

typedef struct tuple_c_item *tuple_c_ptr_type;
                                       /* cell node pointer                 */

/* global data */

#ifdef TSAFE
#define TUPLE_H_NEXT_FREE plugin_instance->tuple_h_next_free 
#define TUPLE_C_NEXT_FREE plugin_instance->tuple_c_next_free 
#else
#define TUPLE_H_NEXT_FREE tuple_h_next_free 
#define TUPLE_C_NEXT_FREE tuple_c_next_free 

#ifdef SHARED

tuple_h_ptr_type tuple_h_next_free = NULL;
                                       /* next free header                  */
tuple_c_ptr_type tuple_c_next_free = NULL;
                                       /* next free cell                    */

#else

extern tuple_h_ptr_type tuple_h_next_free;
                                       /* next free header                  */
extern tuple_c_ptr_type tuple_c_next_free;
                                       /* next free cell                    */

#endif
#endif

/* allocate and free header nodes */

#ifdef HAVE_MPATROL

#define get_tuple_header(t) {\
   t = (tuple_h_ptr_type)malloc(sizeof(struct tuple_h_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_tuple_header(s) free(s)

#define get_tuple_cell(t) {\
   t = (tuple_c_ptr_type)malloc(sizeof(struct tuple_c_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_tuple_cell(s) free(s)

#else

#define get_tuple_header(t) {\
   if (TUPLE_H_NEXT_FREE == NULL) alloc_tuple_headers(SETL_SYSTEM_VOID); \
   t = TUPLE_H_NEXT_FREE; \
   TUPLE_H_NEXT_FREE = *((tuple_h_ptr_type *)(TUPLE_H_NEXT_FREE)); \
}

#define free_tuple_header(s) {\
   *((tuple_h_ptr_type *)(s)) = TUPLE_H_NEXT_FREE; \
   TUPLE_H_NEXT_FREE = s; \
}

/* allocate and free cell nodes */

#define get_tuple_cell(t) {\
   if (TUPLE_C_NEXT_FREE == NULL) alloc_tuple_cells(SETL_SYSTEM_VOID); \
   t = TUPLE_C_NEXT_FREE; \
   TUPLE_C_NEXT_FREE = *((tuple_c_ptr_type *)(TUPLE_C_NEXT_FREE)); \
}

#define free_tuple_cell(s) {\
   *((tuple_c_ptr_type *)(s)) = TUPLE_C_NEXT_FREE; \
   TUPLE_C_NEXT_FREE = s; \
}

#endif

/* public function declarations */

void alloc_tuple_headers(SETL_SYSTEM_PROTO_VOID);        
                                       /* allocate a block of header nodes  */
void alloc_tuple_cells(SETL_SYSTEM_PROTO_VOID);          
                                       /* allocate a block of cell nodes    */
tuple_h_ptr_type copy_tuple(SETL_SYSTEM_PROTO tuple_h_ptr_type);
                                       /* copy a tuple structure            */
tuple_h_ptr_type new_tuple(SETL_SYSTEM_PROTO_VOID);
                                       /* return a new tuple structure      */
void tuple_concat(SETL_SYSTEM_PROTO struct specifier_item *, 
                  struct specifier_item *,
                  struct specifier_item *);
                                       /* concatenate two tuples            */
void tuple_multiply(SETL_SYSTEM_PROTO
                    struct specifier_item *, struct specifier_item *, int32);
                                       /* multiple concatenation            */
void tuple_slice(SETL_SYSTEM_PROTO
                 struct specifier_item *, struct specifier_item *,
                 int32, int32);
                                       /* generate a slice of a tuple       */
void tuple_sslice(SETL_SYSTEM_PROTO
                  struct specifier_item *, struct specifier_item *,
                  int32, int32);
                                       /* slice assignment                  */
void tuple_fromb(SETL_SYSTEM_PROTO
                 struct specifier_item *, struct specifier_item *,
                 struct specifier_item *);
                                       /* concatenate two tuples            */
void tuple_frome(SETL_SYSTEM_PROTO
                 struct specifier_item *, struct specifier_item *,
                 struct specifier_item *);
                                       /* concatenate two tuples            */
void tuple_arb(SETL_SYSTEM_PROTO
               struct specifier_item *, struct specifier_item *);
                                       /* get arbitrary element             */

#define TUPLES_LOADED 1
#endif


