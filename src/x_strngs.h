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
 *  \packagespec{Strings}
\*/

#ifndef STRINGS_LOADED

#define STR_CELL_WIDTH    32           /* characters in string cell         */

/* string header node structure */

struct string_h_item {
   int32 s_use_count;                  /* usage count                       */
   int32 s_hash_code;                  /* hash code                         */
   int32 s_length;                     /* length of string in bytes         */
   struct string_c_item *s_head;       /* first cell in list                */
   struct string_c_item *s_tail;       /* last cell in list                 */
};

typedef struct string_h_item *string_h_ptr_type;
                                       /* header node pointer               */

/* string cell node structure */

struct string_c_item {
   struct string_c_item *s_next;       /* next cell in list                 */
   struct string_c_item *s_prev;       /* previous cell in list             */
   char s_cell_value[STR_CELL_WIDTH];  /* cell value                        */
};

typedef struct string_c_item *string_c_ptr_type;
                                       /* cell node pointer                 */

/* global data */

#ifdef TSAFE
#define STRING_H_NEXT_FREE plugin_instance->string_h_next_free 
#define STRING_C_NEXT_FREE plugin_instance->string_c_next_free 
#else
#define STRING_H_NEXT_FREE string_h_next_free 
#define STRING_C_NEXT_FREE string_c_next_free 
#ifdef SHARED

string_h_ptr_type string_h_next_free = NULL;
                                       /* next free header                  */
string_c_ptr_type string_c_next_free = NULL;
                                       /* next free cell                    */

#else

extern string_h_ptr_type string_h_next_free;
                                       /* next free header                  */
extern string_c_ptr_type string_c_next_free;
                                       /* next free cell                    */

#endif
#endif

/* allocate and free header nodes */

#ifdef HAVE_MPATROL

#define get_string_header(t) {\
   t = (string_h_ptr_type)malloc(sizeof(struct string_h_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_string_header(s) free(s)

#define get_string_cell(t) {\
   t = (string_c_ptr_type)malloc(sizeof(struct string_c_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_string_cell(s) free(s)

#else

#define get_string_header(t) {\
   if (STRING_H_NEXT_FREE == NULL) alloc_string_headers(SETL_SYSTEM_VOID); \
   t = STRING_H_NEXT_FREE; \
   STRING_H_NEXT_FREE = *((string_h_ptr_type *)(STRING_H_NEXT_FREE)); \
}

#define free_string_header(s) {\
   *((string_h_ptr_type *)(s)) = STRING_H_NEXT_FREE; \
   STRING_H_NEXT_FREE = s; \
}

/* allocate and free cell nodes */

#define get_string_cell(t) {\
   if (STRING_C_NEXT_FREE == NULL) alloc_string_cells(SETL_SYSTEM_VOID); \
   t = STRING_C_NEXT_FREE; \
   STRING_C_NEXT_FREE = *((string_c_ptr_type *)(STRING_C_NEXT_FREE)); \
}

#define free_string_cell(s) {\
   *((string_c_ptr_type *)(s)) = STRING_C_NEXT_FREE; \
   STRING_C_NEXT_FREE = s; \
}
#endif

/* public function declarations */

void alloc_string_headers(SETL_SYSTEM_PROTO_VOID);       
                                       /* allocate a block of header nodes  */
void alloc_string_cells(SETL_SYSTEM_PROTO_VOID);         
                                       /* allocate a block of cell nodes    */
void free_string(SETL_SYSTEM_PROTO string_h_ptr_type);   
                                        /* release string                    */
string_h_ptr_type copy_string(SETL_SYSTEM_PROTO string_h_ptr_type);
                                       /* copy a string structure           */
void string_multiply(SETL_SYSTEM_PROTO 
                     struct specifier_item *, struct specifier_item *, int32);
                                       /* replicate string                  */
void string_fromb(SETL_SYSTEM_PROTO 
                  struct specifier_item *, struct specifier_item *,
                  struct specifier_item *);
                                       /* remove character from front       */
void string_frome(SETL_SYSTEM_PROTO 
                  struct specifier_item *, struct specifier_item *,
                  struct specifier_item *);
                                       /* remove character from end         */
string_h_ptr_type new_string(SETL_SYSTEM_PROTO 
                             char *s); /* convert string from C to SETL     */

#define STRINGS_LOADED 1
#endif


