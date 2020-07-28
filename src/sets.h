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
 *  \packagespec{Sets}
\*/

#ifndef SETS_LOADED

/* constants */

#define SET_HASH_SIZE    4             /* set hash table size (each header) */
#define SET_SHIFT_DIST   2             /* log (base 2) of SET_HASH_SIZE     */
#define SET_CLASH_SIZE   3             /* average clash length which        */
                                       /* triggers header expansion         */
#define SET_HASH_MASK 0x03             /* pick out one level of header tree */

/* set header node structure */

struct set_h_item {
   int32 s_use_count;                  /* usage count                       */
   int32 s_hash_code;                  /* hash code                         */
   union {
      struct {                         /* root header info                  */
         int32 s_cardinality;          /* number of elements in set         */
         int s_height;                 /* height of header tree             */
      } s_root;
      struct {                         /* internal node info                */
         struct set_h_item *s_parent;  /* parent in header tree             */
         int s_child_index;            /* index in parent's hash table      */
      } s_intern;
   } s_ntype;
   union {
      struct set_c_item *s_cell;       /* cell child pointer                */
      struct set_h_item *s_header;     /* internal header node pointer      */
   } s_child[SET_HASH_SIZE];
};

typedef struct set_h_item *set_h_ptr_type;
                                       /* header node pointer               */

/* set cell node structure */

struct set_c_item {
   struct set_c_item *s_next;          /* next cell on clash list           */
   int32 s_hash_code;                  /* element's full hash code          */
   struct specifier_item s_spec;       /* element specifier                 */
};

typedef struct set_c_item *set_c_ptr_type;
                                       /* cell node pointer                 */

/* global data */

#ifdef TSAFE
#define SET_H_NEXT_FREE plugin_instance->set_h_next_free 
#define SET_C_NEXT_FREE plugin_instance->set_c_next_free 
#else
#define SET_H_NEXT_FREE set_h_next_free 
#define SET_C_NEXT_FREE set_c_next_free 

#ifdef SHARED

set_h_ptr_type set_h_next_free = NULL; /* next free header                  */
set_c_ptr_type set_c_next_free = NULL; /* next free cell                    */

#else

extern set_h_ptr_type set_h_next_free; /* next free header                  */
extern set_c_ptr_type set_c_next_free; /* next free cell                    */

#endif
#endif

/* allocate and free header nodes */

#ifdef HAVE_MPATROL

#define get_set_header(t) {\
   t = (set_h_ptr_type)malloc(sizeof(struct set_h_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_set_header(s) free(s)

#define get_set_cell(t) {\
   t = (set_c_ptr_type)malloc(sizeof(struct set_c_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_set_cell(s) free(s)

#else

#define get_set_header(t) {\
   if (SET_H_NEXT_FREE == NULL) alloc_set_headers(SETL_SYSTEM_VOID); \
   t = SET_H_NEXT_FREE; \
   SET_H_NEXT_FREE = *((set_h_ptr_type *)(SET_H_NEXT_FREE)); \
}

#define free_set_header(s) {\
   *((set_h_ptr_type *)(s)) = SET_H_NEXT_FREE; \
   SET_H_NEXT_FREE = s; \
}

/* allocate and free cell nodes */

#define get_set_cell(t) {\
   if (SET_C_NEXT_FREE == NULL) alloc_set_cells(SETL_SYSTEM_VOID); \
   t = SET_C_NEXT_FREE; \
   SET_C_NEXT_FREE = *((set_c_ptr_type *)(SET_C_NEXT_FREE)); \
}

#define free_set_cell(s) {\
   *((set_c_ptr_type *)(s)) = SET_C_NEXT_FREE; \
   SET_C_NEXT_FREE = s; \
}
#endif

/* public function declarations */

void alloc_set_headers(SETL_SYSTEM_PROTO_VOID);         
                                       /* allocate a block of header nodes  */
void alloc_set_cells(SETL_SYSTEM_PROTO_VOID); 
                                       /* allocate a block of cell nodes    */
void free_set(SETL_SYSTEM_PROTO set_h_ptr_type);   
                                       /* free memory used by set           */
set_h_ptr_type null_set(SETL_SYSTEM_PROTO_VOID);
                                       /* return an empty set               */
set_h_ptr_type copy_set(SETL_SYSTEM_PROTO set_h_ptr_type);
                                       /* copy a set structure              */
set_h_ptr_type set_expand_header(SETL_SYSTEM_PROTO set_h_ptr_type);
                                       /* expand the header size            */
set_h_ptr_type set_contract_header(SETL_SYSTEM_PROTO set_h_ptr_type);
                                       /* reduce the header size            */
void set_union(SETL_SYSTEM_PROTO 
               struct specifier_item *, struct specifier_item *,
               struct specifier_item *);
                                       /* union two sets                    */
void set_difference(SETL_SYSTEM_PROTO
                    struct specifier_item *, struct specifier_item *,
                    struct specifier_item *);
                                       /* find the difference in two sets   */
void set_intersection(SETL_SYSTEM_PROTO
                      struct specifier_item *, struct specifier_item *,
                      struct specifier_item *);
                                       /* form intersection                 */
void set_symdiff(SETL_SYSTEM_PROTO
                 struct specifier_item *, struct specifier_item *,
                 struct specifier_item *);
                                       /* find the symmetric difference     */
void set_npow(SETL_SYSTEM_PROTO
              struct specifier_item *, struct specifier_item *, int32);
                                       /* find the power set of a set       */
void set_pow(SETL_SYSTEM_PROTO
             struct specifier_item *, struct specifier_item *);
                                       /* find the power set of a set       */
void set_arb(SETL_SYSTEM_PROTO
             struct specifier_item *, struct specifier_item *);
                                       /* find the power set of a set       */
int set_subset(SETL_SYSTEM_PROTO
               struct specifier_item *, struct specifier_item *);
                                       /* test whether a set is a subset of */
                                       /* another set                       */
void set_from(SETL_SYSTEM_PROTO
              struct specifier_item *, struct specifier_item *,
              struct specifier_item *);
                                       /* find the symmetric difference     */

#define SETS_LOADED 1
#endif


