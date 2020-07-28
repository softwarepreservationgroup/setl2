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
 *  \packagespec{Iterator Code Generator}
\*/

#ifndef GENITER_LOADED

/* iterator node structure */

struct compiler_iter_item {

   unsigned it_type : 5;               /* iterator type                     */
   unsigned it_bvar_count : 2;         /* number of bound variables         */
   unsigned it_source_type : 2;        /* set or child                      */
   unsigned it_using_bvar : 1;         /* YES if we're using the bound      */
                                       /* variable as next_integer          */

   struct compiler_iter_item *it_next; /* next in iterator list             */
   struct symtab_item *it_iter_var;    /* iterator variable (temporary)     */

   /* labels */

   int it_top_label;                   /* top of this iterator              */
   int it_loop_label;                  /* iterator loop label               */
   int it_fail_label;                  /* branch if can not find next       */

   /* bound variable fields */

   struct {
      struct symtab_item *it_symtab_ptr;
                                       /* symbol of bound variable          */
      struct ast_item *it_target_ptr;  /* ast of bound variable             */
      struct ast_item *it_source_ptr;  /* bound variable as a function of   */
                                       /* source                            */
   } it_bvar[2];

   /* source descriptor fields */

   struct compiler_iter_item *it_source_child;
                                       /* child iterator list               */
   struct ast_item *it_source_cond;    /* inclusion condition               */

   /* arithmetic iterator information */

   struct symtab_item *it_next_integer;
                                       /* next generated value              */
   struct symtab_item *it_increment;   /* increment                         */
   struct symtab_item *it_last_integer;
                                       /* last acceptable value             */

};

typedef struct compiler_iter_item *c_iter_ptr_type;
                                       /* node pointer                      */

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

/* source types */

#define it_source_set     1            /* the source is a set, tuple or     */
                                       /* string                            */
#define it_source_iter    2            /* the source is another iterator    */

/* public function declarations */

struct compiler_iter_item *get_iter(SETL_SYSTEM_PROTO_VOID); 
                                       /* get an interator item             */
void init_iter(void);                  /* initialize iterators              */
void clear_iter(c_iter_ptr_type);      /* clear an iterator table item      */
symtab_ptr_type gen_iter_values(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type,
                                c_iter_ptr_type *,int);
                                       /* top of value stream               */
c_iter_ptr_type gen_iter_varvals(SETL_SYSTEM_PROTO ast_ptr_type, ast_ptr_type);
                                       /* top of variable-value stream      */
void gen_iter_bottom(SETL_SYSTEM_PROTO c_iter_ptr_type); 
                                       /* bottom of iterator loop           */

#define GENITER_LOADED 1
#endif
