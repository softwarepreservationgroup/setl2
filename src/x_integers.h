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
 *  \packagespec{Integer Table}
\*/

#ifndef INTEGERS_LOADED

#define INT_CELL_WIDTH    (sizeof(int32) * 4 - 1)
                                       /* width of integer cell in bits     */
#define MAX_INT_CELL      ((1L << INT_CELL_WIDTH) - 1)
                                       /* maximum value of a cell           */
#define INT_HIGH_BITS     (((1L << INT_CELL_WIDTH + 2) - 1) << INT_CELL_WIDTH)

/* integer header node structure */

struct integer_h_item {
   int32 i_use_count;                  /* usage count                       */
   int32 i_hash_code;                  /* hash code                         */
   int32 i_cell_count;                 /* number of cells in list           */
   int i_is_negative;                  /* YES if integer is negative        */
   struct integer_c_item *i_head;      /* first cell in list                */
   struct integer_c_item *i_tail;      /* last cell in list                 */
};

typedef struct integer_h_item *integer_h_ptr_type;
                                       /* header node pointer               */

/* integer cell node structure */

struct integer_c_item {
   struct integer_c_item *i_next;      /* next cell in list                 */
   struct integer_c_item *i_prev;      /* previous cell in list             */
   int32 i_cell_value;                 /* cell value                        */
};

typedef struct integer_c_item *integer_c_ptr_type;
                                       /* cell node pointer                 */

/* global data */

#ifdef TSAFE
#define INTEGER_H_NEXT_FREE plugin_instance->integer_h_next_free 
#define INTEGER_C_NEXT_FREE plugin_instance->integer_c_next_free 
#else
#define INTEGER_H_NEXT_FREE integer_h_next_free 
#define INTEGER_C_NEXT_FREE integer_c_next_free 

#ifdef SHARED

integer_h_ptr_type integer_h_next_free = NULL;
                                       /* next free header                  */
integer_c_ptr_type integer_c_next_free = NULL;
                                       /* next free cell                    */

#else

extern integer_h_ptr_type integer_h_next_free;
                                       /* next free header                  */
extern integer_c_ptr_type integer_c_next_free;
                                       /* next free cell                    */

#endif
#endif

/* allocate and free header nodes */

#ifdef HAVE_MPATROL

#define get_integer_header(t) {\
   t = (integer_h_ptr_type)malloc(sizeof(struct integer_h_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_integer_header(s) free(s)

#define get_integer_cell(t) {\
   t = (integer_c_ptr_type)malloc(sizeof(struct integer_c_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_integer_cell(s) free(s)

#else

#define get_integer_header(t) {\
   if (INTEGER_H_NEXT_FREE == NULL) alloc_integer_headers(SETL_SYSTEM_VOID); \
   (t) = INTEGER_H_NEXT_FREE; \
   INTEGER_H_NEXT_FREE = *((integer_h_ptr_type *)(INTEGER_H_NEXT_FREE)); \
}

#define free_integer_header(s) {\
   *((integer_h_ptr_type *)(s)) = INTEGER_H_NEXT_FREE; \
   INTEGER_H_NEXT_FREE = s; \
}

/* allocate and free cell nodes */

#define get_integer_cell(t) {\
   if (INTEGER_C_NEXT_FREE == NULL) alloc_integer_cells(SETL_SYSTEM_VOID); \
   (t) = INTEGER_C_NEXT_FREE; \
   INTEGER_C_NEXT_FREE = *((integer_c_ptr_type *)(INTEGER_C_NEXT_FREE)); \
}

#define free_integer_cell(s) {\
   *((integer_c_ptr_type *)(s)) = INTEGER_C_NEXT_FREE; \
   INTEGER_C_NEXT_FREE = s; \
}

#endif

/* public function declarations */

void alloc_integer_headers(SETL_SYSTEM_PROTO_VOID);      
                                       /* allocate a block of header nodes  */
void alloc_integer_cells(SETL_SYSTEM_PROTO_VOID);        
                                       /* allocate a block of cell nodes    */
void free_interp_integer(SETL_SYSTEM_PROTO integer_h_ptr_type);
                                       /* release memory used by integer    */
integer_h_ptr_type copy_integer(SETL_SYSTEM_PROTO integer_h_ptr_type);
                                       /* copy an integer structure         */
void short_to_long(SETL_SYSTEM_PROTO
                   struct specifier_item *, int32);
                                       /* convert a C long to SETL2 integer */
int32 long_to_short(SETL_SYSTEM_PROTO
                    integer_h_ptr_type);
                                       /* convert a SETL2 integer to C long */
double long_to_double(SETL_SYSTEM_PROTO
                      struct specifier_item *);
                                       /* convert a SETL2 long to double    */
char *integer_string(SETL_SYSTEM_PROTO
                     struct specifier_item *, int);
                                       /* generate character string from    */
                                       /* integer structure                 */
void integer_add(SETL_SYSTEM_PROTO
                 struct specifier_item *, struct specifier_item *,
                 struct specifier_item *);
                                       /* add two integers, of either type  */
void integer_subtract(SETL_SYSTEM_PROTO
                      struct specifier_item *, struct specifier_item *,
                      struct specifier_item *);
                                       /* subtract two integers             */
void integer_multiply(SETL_SYSTEM_PROTO
                      struct specifier_item *, struct specifier_item *,
                      struct specifier_item *);
                                       /* multiply two integers             */
void integer_divide(SETL_SYSTEM_PROTO
                    struct specifier_item *, struct specifier_item *,
                    struct specifier_item *);
                                       /* divide two integers               */
void integer_power(SETL_SYSTEM_PROTO
                   struct specifier_item *, struct specifier_item *,
                   struct specifier_item *);
                                       /* raise an integer to a power       */
void integer_mod(SETL_SYSTEM_PROTO
                 struct specifier_item *, struct specifier_item *,
                 struct specifier_item *);
                                       /* find the left mod right           */
int integer_lt(SETL_SYSTEM_PROTO
               struct specifier_item *, struct specifier_item *);
                                       /* compare two arbitrary integers    */
int integer_le(SETL_SYSTEM_PROTO
               struct specifier_item *, struct specifier_item *);
                                       /* compare two arbitrary integers    */

#define INTEGERS_LOADED 1
#endif


