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
 *  \packagespec{Maps Table}
\*/

#ifndef MAPS_LOADED

/* SETL2 system header files */

#include "specs.h"                     /* specifiers                        */

/* constants */

#define MAP_HASH_SIZE      4           /* map hash table size (each header) */
#define MAP_SHIFT_DIST     2           /* log (base 2) of MAP_HASH_SIZE     */
#define MAP_CLASH_SIZE     3           /* average clash length which        */
                                       /* triggers header expansion         */
#define MAP_HASH_MASK   0x03           /* pick out one level of header tree */

/* map header node structure */

struct map_h_item {
   int32 m_use_count;                  /* usage count                       */
   int32 m_hash_code;                  /* hash code                         */
   union {
      struct {                         /* root header info                  */
         int32 m_cardinality;          /* number of elements in map         */
         int32 m_cell_count;           /* number of cells in map            */
         int m_height;                 /* height of header tree             */
      } m_root;
      struct {                         /* internal node info                */
         struct map_h_item *m_parent;  /* parent in header tree             */
         int m_child_index;            /* index in parent's hash table      */
      } m_intern;
   } m_ntype;
   union {
      struct map_c_item *m_cell;       /* cell child pointer                */
      struct map_h_item *m_header;     /* internal header node pointer      */
   } m_child[MAP_HASH_SIZE];
};

typedef struct map_h_item *map_h_ptr_type;
                                       /* header node pointer               */

/* map cell node structure */

struct map_c_item {
   struct map_c_item *m_next;          /* next cell on clash list           */
   int32 m_hash_code;                  /* element's full hash code          */
   struct specifier_item m_domain_spec; /* element specifier                */
   struct specifier_item m_range_spec; /* element specifier                 */
   short m_is_multi_val;               /* YES if cell is a multi-value      */
};

typedef struct map_c_item *map_c_ptr_type;
                                       /* cell node pointer                 */

/* global data */

#ifdef TSAFE
#define MAP_H_NEXT_FREE plugin_instance->map_h_next_free 
#define MAP_C_NEXT_FREE plugin_instance->map_c_next_free 
#else
#define MAP_H_NEXT_FREE map_h_next_free 
#define MAP_C_NEXT_FREE map_c_next_free 

#ifdef SHARED

map_h_ptr_type map_h_next_free = NULL; /* next free header                  */
map_c_ptr_type map_c_next_free = NULL; /* next free cell                    */

#else

extern map_h_ptr_type map_h_next_free; /* next free header                  */
extern map_c_ptr_type map_c_next_free; /* next free cell                    */

#endif
#endif

/* allocate and free header nodes */

#ifdef HAVE_MPATROL

#define get_map_header(t) {\
   t = (map_h_ptr_type)malloc(sizeof(struct map_h_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_map_header(s) free(s)

#define get_map_cell(t) {\
   t = (map_c_ptr_type)malloc(sizeof(struct map_c_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_map_cell(s) free(s)

#else

#define get_map_header(t) {\
   if (MAP_H_NEXT_FREE == NULL) alloc_map_headers(SETL_SYSTEM_VOID); \
   t = MAP_H_NEXT_FREE; \
   MAP_H_NEXT_FREE = *((map_h_ptr_type *)(MAP_H_NEXT_FREE)); \
}

#define free_map_header(s) {\
   *((map_h_ptr_type *)(s)) = MAP_H_NEXT_FREE; \
   MAP_H_NEXT_FREE = s; \
}

/* allocate and free cell nodes */

#define get_map_cell(t) {\
   if (MAP_C_NEXT_FREE == NULL) alloc_map_cells(SETL_SYSTEM_VOID); \
   t = MAP_C_NEXT_FREE; \
   MAP_C_NEXT_FREE = *((map_c_ptr_type *)(MAP_C_NEXT_FREE)); \
}

#define free_map_cell(s) {\
   *((map_c_ptr_type *)(s)) = MAP_C_NEXT_FREE; \
   MAP_C_NEXT_FREE = s; \
}
#endif

/* public function declarations */

void alloc_map_headers(SETL_SYSTEM_PROTO_VOID);          
                                       /* allocate a block of header nodes  */
void alloc_map_cells(SETL_SYSTEM_PROTO_VOID);            
                                       /* allocate a block of cell nodes    */
map_h_ptr_type copy_map(SETL_SYSTEM_PROTO
                        map_h_ptr_type);
                                       /* copy a map structure              */
map_h_ptr_type map_expand_header(SETL_SYSTEM_PROTO
                                 map_h_ptr_type);
                                       /* enlarge a map header tree         */
map_h_ptr_type map_contract_header(SETL_SYSTEM_PROTO
                                   map_h_ptr_type);
                                       /* shrink a map header tree          */
int set_to_map(SETL_SYSTEM_PROTO
               struct specifier_item *, struct specifier_item *,
               int domain_omega_allowed);
                                       /* convert a set to a map            */
void set_to_smap(SETL_SYSTEM_PROTO
                 struct specifier_item *, struct specifier_item *);
                                       /* convert a set to a smap           */
void map_to_set(SETL_SYSTEM_PROTO
                struct specifier_item *, struct specifier_item *);
                                       /* convert a set to a map            */
void map_lessf(SETL_SYSTEM_PROTO
               struct specifier_item *, struct specifier_item *,
               struct specifier_item *);
                                       /* remove an element from the domain */
                                       /* of a map                          */
void map_domain(SETL_SYSTEM_PROTO
                struct specifier_item *, struct specifier_item *);
                                       /* produce the domain of a map       */
void map_range(SETL_SYSTEM_PROTO
               struct specifier_item *, struct specifier_item *);
                                       /* produce the range of a map        */

#define MAPS_LOADED 1
#endif


