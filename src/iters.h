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
 *  \packagespec{Iterators}
\*/

#ifndef ITERS_LOADED

/* iterator table node structure */

struct iter_item {
   int32 it_use_count;                 /* usage count                       */
   int it_type;                        /* iteration type                    */
   union {
      struct {                         /* set iteration information         */
         struct specifier_item it_spec;
                                       /* set specifier                     */
         struct set_h_item *it_source_work_hdr;
                                       /* current internal node             */
         struct set_c_item *it_source_cell;
                                       /* clash list pointer                */
         int it_source_height;         /* current source height             */
         int it_source_index;          /* current source index              */
      } it_setiter;
      struct {                         /* map iteration information         */
         struct specifier_item it_spec;
                                       /* map specifier                     */
         struct map_h_item *it_source_work_hdr;
                                       /* current internal node             */
         struct map_c_item *it_source_cell;
                                       /* clash list pointer                */
         int it_source_height;         /* current source height             */
         int it_source_index;          /* current source index              */
         struct set_h_item *it_valset_root;
                                       /* current root node                 */
         struct set_h_item *it_valset_work_hdr;
                                       /* current internal node             */
         struct set_c_item *it_valset_cell;
                                       /* clash list pointer                */
         int it_valset_height;         /* current source height             */
         int it_valset_index;          /* current source index              */
      } it_mapiter;
      struct {                         /* tuple iteration information       */
         struct specifier_item it_spec;
                                       /* tuple specifier                   */
         int32 it_source_number;       /* next element number to return     */
      } it_tupiter;
      struct {                         /* string iteration information      */
         struct specifier_item it_spec;
                                       /* string specifier                  */
         struct string_c_item *it_string_cell;
                                       /* string cell pointer               */
         int it_string_index;          /* current character in cell         */
         int32 it_char_number;         /* next character number             */
      } it_striter;
      struct {                         /* power set iterator information    */
         struct specifier_item it_spec;
                                       /* set specifier                     */
         struct source_elem_item *it_se_array;
                                       /* array of source elements          */
         int it_se_array_length;       /* length of above array             */
         int it_n;                     /* size of each subset               */
         int it_done;                  /* YES when finished                 */
      } it_powiter;
      struct {                         /* object iteration information      */
         struct specifier_item it_spec;
                                       /* object specifier                  */
      } it_objiter;
   } it_itype;
};

typedef struct iter_item *iter_ptr_type;
                                       /* node node pointer                 */

/* iteration types */

/* ## begin iter_types */
#define it_set                 0       /* set iterator                      */
#define it_map                 1       /* map iterator                      */
#define it_tuple               2       /* tuple iterator                    */
#define it_string              3       /* string iterator                   */
#define it_object              4       /* object iterator                   */
#define it_domain              5       /* domain set iterator               */
#define it_pow                 6       /* power set iterator                */
#define it_npow                7       /* npow set iterator                 */
#define it_map_pair            8       /* map pair iterator                 */
#define it_tuple_pair          9       /* tuple pair iterator               */
#define it_alt_tuple_pair     10       /* alt tuple pair iterator           */
#define it_string_pair        11       /* string pair iterator              */
#define it_object_pair        12       /* object pair iterator              */
#define it_map_multi          13       /* map multi iterator                */
#define it_object_multi       14       /* object multi iterator             */
#define it_single             15       /* single value iterator             */
#define it_pair               16       /* pair value iterator               */
#define it_multi              17       /* multi value iterator              */
#define it_arith              18       /* arithmetic iterator               */
/* ## end iter_types */

/* power set source item */

struct source_elem_item {              /* source set element structure      */
   struct set_c_item *se_element;      /* source element pointer            */
   short se_in_set;                    /* YES if element is in current      */
                                       /* subset                            */
};

/* global data */

#ifdef TSAFE
#define ITER_NEXT_FREE plugin_instance->iter_next_free 
#else
#define ITER_NEXT_FREE iter_next_free 

#ifdef SHARED

iter_ptr_type iter_next_free = NULL;   /* next free node                    */

#else

extern iter_ptr_type iter_next_free;   /* next free node                    */

#endif
#endif

/* allocate and free iterator nodes */

#ifdef HAVE_MPATROL

#define get_iterator(t) {\
   t = (iter_ptr_type)malloc(sizeof(struct iter_item));\
   if (t == NULL)\
      giveup(SETL_SYSTEM msg_malloc_error);\
}

#define free_iterator(s) free(s)

#else

#define get_iterator(i) {\
   if (ITER_NEXT_FREE == NULL) alloc_iterators(SETL_SYSTEM_VOID); \
   i = ITER_NEXT_FREE; \
   ITER_NEXT_FREE = *((iter_ptr_type *)(ITER_NEXT_FREE)); \
}

#define free_iterator(i) {\
   *((iter_ptr_type *)(i)) = ITER_NEXT_FREE; \
   ITER_NEXT_FREE = i; \
}
#endif

/* public function declarations */

void alloc_iterators(SETL_SYSTEM_PROTO_VOID);            
                                       /* allocate a block of node nodes    */

void start_set_iterator(SETL_SYSTEM_PROTO
                        struct specifier_item *, struct specifier_item *);
                                       /* start iteration over set          */
int set_iterator_next(SETL_SYSTEM_PROTO
                      struct specifier_item *, struct specifier_item *);
                                       /* next item in set iteration        */

void start_map_iterator(SETL_SYSTEM_PROTO
                        struct specifier_item *, struct specifier_item *);
                                       /* start iteration over map          */
int map_iterator_next(SETL_SYSTEM_PROTO
                      struct specifier_item *, struct specifier_item *);
                                       /* next item in map iteration        */

void start_tuple_iterator(SETL_SYSTEM_PROTO
                          struct specifier_item *, struct specifier_item *);
                                       /* start iteration over tuple        */
int tuple_iterator_next(SETL_SYSTEM_PROTO
                        struct specifier_item *, struct specifier_item *);
                                       /* next item in tuple iteration      */

void start_string_iterator(SETL_SYSTEM_PROTO
                           struct specifier_item *, struct specifier_item *);
                                       /* start iteration over string       */
int string_iterator_next(SETL_SYSTEM_PROTO
                         struct specifier_item *, struct specifier_item *);
                                       /* next item in string iteration     */

void start_domain_iterator(SETL_SYSTEM_PROTO
                           struct specifier_item *, struct specifier_item *);
                                       /* start iteration over domain set   */
int domain_iterator_next(SETL_SYSTEM_PROTO
                         struct specifier_item *, struct specifier_item *);
                                       /* next item in domain iteration     */

void start_pow_iterator(SETL_SYSTEM_PROTO
                        struct specifier_item *, struct specifier_item *);
                                       /* start iteration over power set    */
int pow_iterator_next(SETL_SYSTEM_PROTO
                      struct specifier_item *, struct specifier_item *);
                                       /* next item in power set            */

void start_npow_iterator(SETL_SYSTEM_PROTO
                         struct specifier_item *, struct specifier_item *,
                         int32);
                                       /* start iteration over power set    */
int npow_iterator_next(SETL_SYSTEM_PROTO
                       struct specifier_item *, struct specifier_item *);
                                       /* next item in power set            */

void start_map_pair_iterator(SETL_SYSTEM_PROTO
                             struct specifier_item *,
                             struct specifier_item *);
                                       /* start iteration over map          */
int map_pair_iterator_next(SETL_SYSTEM_PROTO
                           struct specifier_item *, struct specifier_item *,
                           struct specifier_item *);
                                       /* next pair in map                  */

void start_string_pair_iterator(SETL_SYSTEM_PROTO
                                struct specifier_item *,
                                struct specifier_item *);
                                       /* start iteration over string       */
int string_pair_iterator_next(SETL_SYSTEM_PROTO
                              struct specifier_item *,
                              struct specifier_item *,
                              struct specifier_item *);
                                       /* next pair in string               */

void start_tuple_pair_iterator(SETL_SYSTEM_PROTO
                               struct specifier_item *,
                               struct specifier_item *);
                                       /* start iteration over tuple        */
int tuple_pair_iterator_next(SETL_SYSTEM_PROTO
                             struct specifier_item *,
                             struct specifier_item *,
                             struct specifier_item *);
                                       /* next pair in tuple                */

void start_alt_tuple_pair_iterator(SETL_SYSTEM_PROTO
                                   struct specifier_item *,
                                   struct specifier_item *);
                                       /* start iteration over tuple        */
int alt_tuple_pair_iterator_next(SETL_SYSTEM_PROTO
                                 struct specifier_item *,
                                 struct specifier_item *,
                                 struct specifier_item *);
                                       /* next pair in tuple                */

void start_map_multi_iterator(SETL_SYSTEM_PROTO
                              struct specifier_item *,
                              struct specifier_item *);
                                       /* start iteration over multi-valued */
                                       /* map                               */
int map_multi_iterator_next(SETL_SYSTEM_PROTO
                            struct specifier_item *, struct specifier_item *,
                            struct specifier_item *);
                                       /* next pair in multi-valued map     */

void start_object_iterator(SETL_SYSTEM_PROTO
                           struct specifier_item *, struct specifier_item *);
                                       /* start iteration over object       */
int object_iterator_next(SETL_SYSTEM_PROTO
                         struct specifier_item *, struct specifier_item *);
                                       /* next item in object iteration     */

void start_object_pair_iterator(SETL_SYSTEM_PROTO
                                struct specifier_item *,
                                struct specifier_item *);
                                       /* start iteration over object       */
int object_pair_iterator_next(SETL_SYSTEM_PROTO
                              struct specifier_item *,
                              struct specifier_item *,
                              struct specifier_item *);
                                       /* next pair in object               */

void start_object_multi_iterator(SETL_SYSTEM_PROTO
                                 struct specifier_item *,
                                 struct specifier_item *);
                                       /* start iteration over object       */
int object_multi_iterator_next(SETL_SYSTEM_PROTO
                               struct specifier_item *,
                               struct specifier_item *,
                               struct specifier_item *);
                                       /* next pair in object               */

#define ITERS_LOADED 1
#endif

