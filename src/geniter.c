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
 *  \package{The Iterator Code Generator}
 *
 *  This package generates code for iterators.  We are trying to look
 *  ahead, and structure these functions to facilitate optimization
 *  later.  In particular, we would like to be able to avoid forming sets
 *  and tuples as much as possible.  There are two impediments to this
 *  goal.  First, the inclusion conditions for set and tuple formers may
 *  have side effects, or the body of the iteration may affect the
 *  condition.  Second, we do not allow sets to have duplicate elements,
 *  so expressions in set formers must be monotonic.
 *
 *  \texify{geniter.h}
 *
 *  \packagebody{Iterator Code Generator}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "builtins.h"                  /* built-in symbols                  */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "ast.h"                       /* abstract syntax tree              */
#include "genquads.h"                  /* quadruple generator               */
#include "genexpr.h"                   /* expression code generator         */
#include "genbool.h"                   /* boolean expression code generator */
#include "genlhs.h"                    /* left hand side code generator     */
#include "geniter.h"                   /* iterator code generator           */


#ifdef PLUGIN
#define fprintf plugin_fprintf
#endif


/* performance tuning constants */

#define ITER_BLOCK_SIZE       20       /* iterator block size               */

/* generic table item structure (iterator use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct compiler_iter_item ti_data;
                                       /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[ITER_BLOCK_SIZE];
                                       /* array of table items              */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */
static file_pos_type null_file_pos = {-1,-1};
                                       /* null file position                */

/* forward declarations */

static c_iter_ptr_type gen_iter_list(SETL_SYSTEM_PROTO ast_ptr_type, int, int);
                                       /* generate top of iterator list     */
static void gen_iter_source(SETL_SYSTEM_PROTO c_iter_ptr_type, ast_ptr_type, int);
                                       /* initialize iterator source        */
static void gen_iter_looptop(SETL_SYSTEM_PROTO c_iter_ptr_type);
                                       /* generate code to find next item   */
static void gen_iter_clear(SETL_SYSTEM_PROTO c_iter_ptr_type);
                                       /* free memory used by iterator      */

/*\
 *  \function{init\_iter()}
 *
 *  This procedure initializes the iterator table.  All we do is free all
 *  the allocated blocks.
\*/

void init_iter()

{
struct table_block *tb_ptr;            /* work table pointer                */

   while (table_block_head != NULL) {
      tb_ptr = table_block_head;
      table_block_head = table_block_head->tb_next;
      free((void *)tb_ptr);
   }
   table_next_free = NULL;

}

/*\
 *  \function{get\_iter()}
 *
 *  This procedure allocates an iterator node. It is just like most of
 *  the other dynamic table allocation functions in the compiler.
\*/

c_iter_ptr_type get_iter(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *old_head;          /* name table block list head        */
c_iter_ptr_type return_ptr;            /* return pointer                    */

   if (table_next_free == NULL) {

      /* allocate a new block */

      old_head = table_block_head;
      table_block_head = (struct table_block *)
                         malloc(sizeof(struct table_block));
      if (table_block_head == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      table_block_head->tb_next = old_head;

      /* link items on the free list */

      for (table_next_free = table_block_head->tb_data;
           table_next_free <=
              table_block_head->tb_data + ITER_BLOCK_SIZE - 2;
           table_next_free++) {
         table_next_free->ti_union.ti_next = table_next_free + 1;
      }

      table_next_free->ti_union.ti_next = NULL;
      table_next_free = table_block_head->tb_data;

   }

   /* at this point, we know the free list is not empty */

   return_ptr = &(table_next_free->ti_union.ti_data);
   table_next_free = table_next_free->ti_union.ti_next;

   /* initialize the new entry */

   clear_iter(return_ptr);

   return return_ptr;

}

/*\
 *  \function{clear\_iter()}
 *
 *  This procedure clears a single iterator item.  It's convenient to do
 *  that in a separate procedure, since then I probably won't forget to
 *  clear some field I add later.
\*/

void clear_iter(
   c_iter_ptr_type i)                  /* item to be cleared                */

{

   (i)->it_type = 0;
   (i)->it_next = NULL; 
   (i)->it_iter_var = NULL;
   (i)->it_bvar_count = 0; 
   (i)->it_top_label = -1;
   (i)->it_loop_label = -1; 
   (i)->it_bvar[0].it_symtab_ptr = NULL; 
   (i)->it_bvar[1].it_symtab_ptr = NULL; 
   (i)->it_bvar[0].it_target_ptr = NULL; 
   (i)->it_bvar[1].it_target_ptr = NULL; 
   (i)->it_bvar[0].it_source_ptr = NULL; 
   (i)->it_bvar[1].it_source_ptr = NULL; 
   (i)->it_source_type = 0;
   (i)->it_source_child = NULL; 
   (i)->it_source_cond = NULL;
   (i)->it_using_bvar = 0; 
   (i)->it_next_integer = NULL;
   (i)->it_increment = NULL; 
   (i)->it_last_integer = NULL; 

}

/*\
 *  \function{free\_iter()}
 *
 *  This function is the complement to \verb"get_iter()". All we do is
 *  push the passed iterator pointer on the free list.
\*/

void free_iter(
   c_iter_ptr_type discard)            /* item to be discarded              */

{

   ((struct table_item *)discard)->ti_union.ti_next = table_next_free;
   table_next_free = (struct table_item *)discard;

}

/*\
 *  \function{gen\_iter\_values()}
 *
 *  This function generates the code at the top of an iteration, where we
 *  need a stream of values.  It is really just an interface, so that the
 *  caller does not have to worry about the recursion in this package.
\*/

symtab_ptr_type gen_iter_values(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* root of iterator list             */
   symtab_ptr_type bound_var,          /* bound variable                    */
   c_iter_ptr_type *iter_root,         /* created iterator                  */
   int side_effect_possible)           /* YES if the caller can affect      */
                                       /* expressions in the iterator       */

{
c_iter_ptr_type iter_ptr;              /* created iterator                  */

   /* allocate an iterator node */

   iter_ptr = get_iter(SETL_SYSTEM_VOID);

   /* set labels to initial values */

   iter_ptr->it_fail_label = next_label++;
   iter_ptr->it_loop_label = iter_ptr->it_top_label = next_label++;

   /* set up bound variable information */

   if (bound_var == NULL) {
      get_temp(bound_var);
   }

   iter_ptr->it_bvar_count = 1;
   iter_ptr->it_bvar[0].it_symtab_ptr = bound_var;
   iter_ptr->it_bvar[0].it_target_ptr = NULL;
   iter_ptr->it_bvar[0].it_source_ptr = NULL;
   iter_ptr->it_type = it_single;

   /* generate the initialization stuff */

   gen_iter_source(SETL_SYSTEM iter_ptr,root,side_effect_possible);

   /* return the iterator and bound variable */

   *iter_root = iter_ptr;

   return bound_var;

}

/*\
 *  \function{gen\_iter\_varvals()}
 *
 *  This function generates the code at the top of an iteration, where we
 *  want to assign values to a group of variables.  It is really just an
 *  interface, so that the caller does not have to worry about the
 *  recursion in this package.
\*/

c_iter_ptr_type gen_iter_varvals(
   SETL_SYSTEM_PROTO
   ast_ptr_type iter_list_ptr,         /* list of iterators                 */
   ast_ptr_type cond_ptr)              /* set inclusion condition           */

{
c_iter_ptr_type iter_ptr;              /* created iterator                  */

   /* allocate an iterator node */

   iter_ptr = get_iter(SETL_SYSTEM_VOID);

   /* set labels to initial values */

   iter_ptr->it_fail_label = next_label++;
   iter_ptr->it_loop_label = iter_ptr->it_top_label = next_label++;

   /* set up bound variable information */

   iter_ptr->it_bvar_count = 0;

   /* set inclusion condition and expression */

   iter_ptr->it_source_cond = cond_ptr;
   iter_ptr->it_source_type = it_source_iter;

   /* generate source stream */

   iter_ptr->it_source_child = gen_iter_list(SETL_SYSTEM iter_list_ptr,
                                             iter_ptr->it_fail_label,
                                             YES);
   iter_ptr->it_loop_label = (iter_ptr->it_source_child)->it_loop_label;

   gen_iter_looptop(SETL_SYSTEM iter_ptr);

   /* return the iterator */

   return iter_ptr;

}

/*\
 *  \function{gen\_iter\_list()}
 *
 *  This function generates the initialization code for an iterator list.
 *  It loops over the iterators setting the bound variable information,
 *  and calls \verb"gen_iter_source" to initialize the source iteration.
\*/

static c_iter_ptr_type gen_iter_list(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* root of iterator list             */
   int fail_label,                     /* parent's fail label               */
   int side_effect_possible)           /* YES if the caller can affect      */
                                       /* expressions in the iterator       */

{
ast_ptr_type ast_ptr;                  /* used to loop over iterators       */
c_iter_ptr_type iter_head;             /* head of generated iterator items  */
c_iter_ptr_type *iter_tail;            /* tail of generated iterator items  */
c_iter_ptr_type iter_ptr;              /* current iterator item             */

   /* we start with a null iterator list */

   iter_head = NULL;
   iter_tail = &iter_head;

   /* loop over the iterator nodes */

   for (ast_ptr = root->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

#ifdef DEBUG

      if (CODE_DEBUG) {

         fprintf(DEBUG_FILE,"ITER : %s\n",ast_desc[ast_ptr->ast_type]);

      }

#endif

      /* generate an iterator and append it to the list */

      iter_ptr = get_iter(SETL_SYSTEM_VOID);
      *iter_tail = iter_ptr;
      iter_tail = &(iter_ptr->it_next);

      /* set labels to initial values */

      iter_ptr->it_fail_label = fail_label;
      fail_label = next_label++;
      iter_ptr->it_top_label = fail_label;
      iter_ptr->it_loop_label = fail_label;

      switch (ast_ptr->ast_type) {

/*\
 *  \ast{ast\_in}{``x in S'' iterator}{
 *     \astnode{ast\_in}
 *        {\astnode{{\em bound var}}
 *           {\etcast}
 *           {\astnode{{\em source}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This case handles iterators of the form \verb"x in S", where \verb"x"
 *  can be any bound variable form, and \verb"S" can be any expression.
\*/

case ast_in :

{
ast_ptr_type left_ptr;                 /* bound variable subtree            */
ast_ptr_type source_ptr, domain_ptr, range_ptr;
                                       /* source, domain element, and range */
                                       /* element pointers                  */

   /* pick out child pointers, to shorten expressions */

   left_ptr = ast_ptr->ast_child.ast_child_ast;

   /*
    *  Look for iterators of the form "[lhs1,lhs2] in f", and process
    *  those as map interators (sometimes improves code, rarely degrades
    *  it).
    */

   if (left_ptr->ast_type == ast_enum_tup) {

      domain_ptr = left_ptr->ast_child.ast_child_ast;
      if (domain_ptr != NULL)
         range_ptr = domain_ptr->ast_next;
      if (domain_ptr != NULL && range_ptr != NULL &&
                                range_ptr->ast_next == NULL) {

         source_ptr = left_ptr->ast_next;

         /* set up bound variable information */

         iter_ptr->it_bvar_count = 2;

         if (range_ptr->ast_type == ast_symtab) {

            iter_ptr->it_bvar[1].it_symtab_ptr =
                  range_ptr->ast_child.ast_symtab_ptr;
            iter_ptr->it_bvar[1].it_target_ptr = NULL;

         }
         else {

            get_temp(iter_ptr->it_bvar[1].it_symtab_ptr);
            iter_ptr->it_bvar[1].it_target_ptr = range_ptr;

         }

         iter_ptr->it_bvar[1].it_source_ptr = NULL;

         if (domain_ptr->ast_type == ast_symtab) {

            iter_ptr->it_bvar[0].it_symtab_ptr =
                  domain_ptr->ast_child.ast_symtab_ptr;
            iter_ptr->it_bvar[0].it_target_ptr = NULL;

         }
         else {

            get_temp(iter_ptr->it_bvar[0].it_symtab_ptr);
            iter_ptr->it_bvar[0].it_target_ptr = domain_ptr;

         }

         iter_ptr->it_bvar[0].it_source_ptr = NULL;

         /* we've finished the bound variable, start on the source */

         iter_ptr->it_type = it_pair;

         gen_iter_source(SETL_SYSTEM iter_ptr,
                         source_ptr,
                         side_effect_possible);

         break;

      }
   }

   /* set up bound variable information */

   iter_ptr->it_bvar_count = 1;

   if (left_ptr->ast_type == ast_symtab) {

      iter_ptr->it_bvar[0].it_symtab_ptr =
            left_ptr->ast_child.ast_symtab_ptr;
      iter_ptr->it_bvar[0].it_target_ptr = NULL;

   }
   else {

      get_temp(iter_ptr->it_bvar[0].it_symtab_ptr);
      iter_ptr->it_bvar[0].it_target_ptr = left_ptr;

   }

   iter_ptr->it_bvar[0].it_source_ptr = NULL;

   /* we've finished the bound variable, start on the source */

   iter_ptr->it_type = it_single;
   gen_iter_source(SETL_SYSTEM iter_ptr,
                   left_ptr->ast_next,
                   side_effect_possible);

   break;

}

/*\
 *  \ast{ast\_eq}{``y = f(x)'' iterator}{
 *     \astnode{ast\_eq}
 *        {\astnode{{\em bound var 1}}
 *           {\etcast}
 *           {\astnode{ast\_of}
 *              {\astnode{{\em source}}
 *                 {\etcast}
 *                 {\astnode{{\em bound var 2}}
 *                    {\etcast}
 *                    {\nullast}
 *                 }
 *              }
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This case handles iterators of the form \verb"y = f(x)", and
 *  \verb"y = f{x}" where \verb"x" and \verb"y" can be any bound
 *  variable form, and \verb"f" can be any expression.  We iterate over a
 *  map or multi-valued map.
\*/

case ast_eq :

{
ast_ptr_type source_ptr, domain_ptr, range_ptr;
                                       /* source, domain element, and range */
                                       /* element pointers                  */

   /* pick out child pointers, to shorten expressions */

   range_ptr = ast_ptr->ast_child.ast_child_ast;
   source_ptr = (range_ptr->ast_next)->ast_child.ast_child_ast;
   domain_ptr = (source_ptr->ast_next)->ast_child.ast_child_ast;

   /* set up bound variable information */

   iter_ptr->it_bvar_count = 2;

   if (range_ptr->ast_type == ast_symtab) {

      iter_ptr->it_bvar[1].it_symtab_ptr =
            range_ptr->ast_child.ast_symtab_ptr;
      iter_ptr->it_bvar[1].it_target_ptr = NULL;

   }
   else {

      get_temp(iter_ptr->it_bvar[1].it_symtab_ptr);
      iter_ptr->it_bvar[1].it_target_ptr = range_ptr;

   }

   iter_ptr->it_bvar[1].it_source_ptr = NULL;

   if (domain_ptr->ast_type == ast_symtab) {

      iter_ptr->it_bvar[0].it_symtab_ptr =
            domain_ptr->ast_child.ast_symtab_ptr;
      iter_ptr->it_bvar[0].it_target_ptr = NULL;

   }
   else {

      get_temp(iter_ptr->it_bvar[0].it_symtab_ptr);
      iter_ptr->it_bvar[0].it_target_ptr = domain_ptr;

   }

   iter_ptr->it_bvar[0].it_source_ptr = NULL;

   /* we've finished the bound variable, start on the source */

   if ((range_ptr->ast_next)->ast_type == ast_of)
      iter_ptr->it_type = it_map_pair;
   else
      iter_ptr->it_type = it_multi;

   gen_iter_source(SETL_SYSTEM iter_ptr,
                   source_ptr,
                   side_effect_possible);

   break;

}

/*\
 *  \case{default}
 *
 *  We reach the default case when we find an abstract syntax tree node
 *  with a type we don't know how to handle.  This should NOT happen.
\*/

default :

{

#ifdef TRAPS

   trap(__FILE__,__LINE__,msg_bad_ast_node,ast_ptr->ast_type);

#endif

}

   /* back to normal indentation */

      }

      /* propagate the top of loop label upward */

      iter_head->it_loop_label = fail_label = iter_ptr->it_loop_label;

   }

   return iter_head;

}

/*\
 *  \function{gen\_iter\_source()}
 *
 *  This function generates initialization for the source of an
 *  iterator.
\*/

static void gen_iter_source(
   SETL_SYSTEM_PROTO
   c_iter_ptr_type iter_ptr,           /* iterator record to be updated     */
   ast_ptr_type root,                  /* source subtree                    */
   int side_effect_possible)           /* YES if the caller can affect      */
                                       /* expressions in the iterator       */

{
symtab_ptr_type left_symtab_ptr;       /* evaluated left operand            */
symtab_ptr_type right_symtab_ptr;      /* evaluated right operand           */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"ITER : %s\n",ast_desc[root->ast_type]);

   }

#endif

   switch (root->ast_type) {

/*\
 *  \ast{ast\_genset}{``\{e : x in S | C\}'' set and tuple formers}
 *     {\astnode{ast\_genset}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{{\em iterator list}}
 *              {\etcast}
 *              {\astnode{{\em condition}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This case handles iteration over a general set or tuple former.  In
 *  most cases we simply form the set or tuple and iterate over the
 *  result.  We do generate code to iterate directly over the former in
 *  the rare event that is safe.
\*/

case ast_genset :       case ast_gentup :

{
ast_ptr_type expr_ptr;                 /* expression defining set members   */
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type cond_ptr;                 /* set inclusion condition           */

   if (side_effect_possible)
      break;
   if (iter_ptr->it_type != it_single && iter_ptr->it_type != it_map_pair)
      break;

   /* pick out child pointers, to shorten expressions */

   expr_ptr = root->ast_child.ast_child_ast;
   iter_list_ptr = expr_ptr->ast_next;
   cond_ptr = iter_list_ptr->ast_next;

   /* generate source stream */

   iter_ptr->it_source_child = gen_iter_list(SETL_SYSTEM iter_list_ptr,
                                             iter_ptr->it_fail_label,
                                             YES);
   iter_ptr->it_loop_label = (iter_ptr->it_source_child)->it_loop_label;

   /* set inclusion condition and expression */

   iter_ptr->it_bvar[0].it_source_ptr = expr_ptr;
   iter_ptr->it_source_cond = cond_ptr;

   iter_ptr->it_source_type = it_source_iter;

   gen_iter_looptop(SETL_SYSTEM iter_ptr);

   return;

}

/*\
 *  \ast{ast\_genset\_noexp}{``\{x in S | C\}'' set and tuple formers}
 *     {\astnode{genset\_noexp}
 *        {\astnode{{\em iterator list}}
 *           {\etcast}
 *           {\astnode{{\em condition}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This case handles set and tuple formers in which we do not have an
 *  expression for the elements --- the bound variables themselves are
 *  inserted into the set.
\*/

case ast_genset_noexp : case ast_gentup_noexp :

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type cond_ptr;                 /* set inclusion condition           */

   if (side_effect_possible)
      break;

   /* pick out child pointers, to shorten expressions */

   iter_list_ptr = root->ast_child.ast_child_ast;
   cond_ptr = iter_list_ptr->ast_next;

   if ((iter_list_ptr->ast_child.ast_child_ast)->ast_type != ast_in)
      break;
   if (iter_ptr->it_bvar_count != 1)
      break;

   /* generate source stream */

   iter_ptr->it_source_child = gen_iter_list(SETL_SYSTEM iter_list_ptr,
                                             iter_ptr->it_fail_label,
                                             YES);
   iter_ptr->it_loop_label = (iter_ptr->it_source_child)->it_loop_label;

   /* set inclusion condition and expression */

   iter_ptr->it_bvar[0].it_source_ptr = NULL;
   iter_ptr->it_bvar[1].it_source_ptr = NULL;

   iter_ptr->it_source_cond = cond_ptr;

   iter_ptr->it_source_type = it_source_iter;

   gen_iter_looptop(SETL_SYSTEM iter_ptr);

   return;

}

/*\
 *  \ast{ast\_arith}{arithmetic set and tuple iterators}
 *     {\astnode{ast\_arith}
 *        {\astnode{{\em list}}
 *           {\astnode{{\em first element}}
 *              {\etcast}
 *              {\astnode{{\em second element}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *           {\astnode{{\em last element}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This case handles arithmetic set and tuple formers.  Here we directly
 *  generate code to manipulate integers, rather than using the
 *  interpreter's iterator structure as we would for other types of
 *  iterators.
\*/

case ast_arith_set :    case ast_arith_tup :

{
ast_ptr_type first, second, last;      /* iteration limits                  */
symtab_ptr_type operand[3];            /* array of operands                 */
int operand_num;                       /* operand number for integer check  */

   if (iter_ptr->it_bvar_count != 1)
      break;

   /* pick out child pointers, to shorten expressions */

   first = (root->ast_child.ast_child_ast)->ast_child.ast_child_ast;
   second = first->ast_next;
   last = (root->ast_child.ast_child_ast)->ast_next;
   operand_num = 0;

   /* try to use the bound variable, without a shadow */

   if (!side_effect_possible &&
       iter_ptr->it_bvar[0].it_symtab_ptr != NULL) {

      iter_ptr->it_next_integer = iter_ptr->it_bvar[0].it_symtab_ptr;
      iter_ptr->it_using_bvar = YES;

   }
   else {

      get_temp(iter_ptr->it_next_integer);
      iter_ptr->it_using_bvar = NO;

   }

   /* evaluate the first integer */

/*   iter_ptr->it_next_integer = /* TOTO */
      gen_expression(SETL_SYSTEM first,iter_ptr->it_next_integer);

   /* make sure it is an integer */

   if (first->ast_type != ast_symtab ||
       (first->ast_child.ast_symtab_ptr)->st_type != sym_integer)
      operand[operand_num++] = iter_ptr->it_next_integer;

   /* if we have the three-argument form ... */

   if (second != NULL) {

      /* load the second value for the iterator */

      get_temp(iter_ptr->it_increment);
      iter_ptr->it_increment =
         gen_expression(SETL_SYSTEM second,iter_ptr->it_increment);

      /* make sure it is an integer */

      if (second->ast_type != ast_symtab ||
          (second->ast_child.ast_symtab_ptr)->st_type != sym_integer)
         operand[operand_num++] = iter_ptr->it_increment;

      /* find the increment */

      emit(q_sub,
           iter_ptr->it_increment,
           iter_ptr->it_increment,
           iter_ptr->it_next_integer,
           &(second->ast_file_pos));

   }
   else {

      iter_ptr->it_increment = sym_one;

   }

   /* decrement the initial value */

   emit(q_sub,
        iter_ptr->it_next_integer,
        iter_ptr->it_next_integer,
        iter_ptr->it_increment,
        &(first->ast_file_pos));

   /* find the final value */

   if (last->ast_type != ast_symtab ||
       (last->ast_child.ast_symtab_ptr)->st_type != sym_integer) {

      get_temp(iter_ptr->it_last_integer);
      iter_ptr->it_last_integer =
         gen_expression(SETL_SYSTEM last,iter_ptr->it_last_integer);

      /* make sure it is an integer */

      operand[operand_num++] = iter_ptr->it_last_integer;

   }
   else {

      iter_ptr->it_last_integer = last->ast_child.ast_symtab_ptr;

   }

   /* emit the instruction to make sure our arguments are integers */

   if (operand_num > 0) {

      while (operand_num < 3)
         operand[operand_num++] = NULL;

      emit(q_intcheck,
           operand[0],
           operand[1],
           operand[2],
           &(root->ast_file_pos));

   }

   /* finish up the iterator record */

   iter_ptr->it_type = it_arith;
   iter_ptr->it_source_type = it_source_set;
   iter_ptr->it_source_cond = NULL;

   gen_iter_looptop(SETL_SYSTEM iter_ptr);

   return;

}

/*\
 *  \ast{ast\_pow}{power set expressions}
 *     {\astnode{ast\_pow}
 *        {\astnode{{\em set expression}}
 *           {\etcast}
 *           {\nullast}
 *        }
 *        {\etcast}
 *     }
 *
 *  This case handles power set operations.  We prefer not to evaluate
 *  the power set, but rather to iterate directly over its elements.
\*/

case ast_pow :

{

   if (iter_ptr->it_type != it_single)
      break;

   /* allocate a temporary for the iterator */

   get_temp(iter_ptr->it_iter_var);

   /* create the set */

   left_symtab_ptr = gen_expression(SETL_SYSTEM root->ast_child.ast_child_ast,NULL);

   /* start the iterator */

   iter_ptr->it_type = it_pow;

   emitssi(q_iter,
           iter_ptr->it_iter_var,
           left_symtab_ptr,
           it_pow,
           &(root->ast_file_pos));

   /* if the source was an expression, free the temporary */

   if (left_symtab_ptr->st_is_temp) {
      free_temp(left_symtab_ptr);
   }

   /* set the source information in the iterator */

   iter_ptr->it_source_type = it_source_set;
   iter_ptr->it_source_cond = NULL;

   gen_iter_looptop(SETL_SYSTEM iter_ptr);

   return;

}

/*\
 *  \ast{ast\_npow}{npow set expressions}
 *     {\astnode{ast\_npow}
 *        {\astnode{{\em left operand}}
 *           {\etcast}
 *           {\astnode{{\em right operand}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This case handles npow set operations.  We prefer not to evaluate
 *  the power set, but rather to iterate directly over its elements.
\*/

case ast_npow :

{

   if (iter_ptr->it_type != it_single)
      break;

   /* allocate a temporary for the iterator */

   get_temp(iter_ptr->it_iter_var);

   /* create the set */

   left_symtab_ptr = gen_expression(SETL_SYSTEM root->ast_child.ast_child_ast,NULL);
   right_symtab_ptr =
         gen_expression(SETL_SYSTEM (root->ast_child.ast_child_ast)->ast_next,NULL);

   /* start the iterator */

   iter_ptr->it_type = it_pow;

   emitssi(q_iter,
           iter_ptr->it_iter_var,
           left_symtab_ptr,
           it_npow,
           &(root->ast_file_pos));

   emit(q_noop,
        right_symtab_ptr,
        NULL,
        NULL,
        &(root->ast_file_pos));

   /* if the source was an expression, free the temporary */

   if (left_symtab_ptr->st_is_temp) {
      free_temp(left_symtab_ptr);
   }
   if (right_symtab_ptr->st_is_temp) {
      free_temp(right_symtab_ptr);
   }

   /* set the source information in the iterator */

   iter_ptr->it_source_type = it_source_set;
   iter_ptr->it_source_cond = NULL;

   gen_iter_looptop(SETL_SYSTEM iter_ptr);

   return;

}

/*\
 *  \ast{ast\_domain}{domain set expressions}
 *     {\astnode{ast\_domain}
 *        {\astnode{{\em set expression}}
 *           {\etcast}
 *           {\nullast}
 *        }
 *        {\etcast}
 *     }
 *
 *  This case handles domain set operations.  We prefer not to evaluate
 *  the domain set, but rather to iterate directly over its elements.
\*/

case ast_domain :

{

   if (iter_ptr->it_type != it_single)
      break;

   /* allocate a temporary for the iterator */

   get_temp(iter_ptr->it_iter_var);

   /* create the set */

   left_symtab_ptr = gen_expression(SETL_SYSTEM root->ast_child.ast_child_ast,NULL);

   /* start the iterator */

   iter_ptr->it_type = it_domain;

   emitssi(q_iter,
           iter_ptr->it_iter_var,
           left_symtab_ptr,
           it_domain,
           &(root->ast_file_pos));

   /* if the source was an expression, free the temporary */

   if (left_symtab_ptr->st_is_temp) {
      free_temp(left_symtab_ptr);
   }

   /* set the source information in the iterator */

   iter_ptr->it_source_type = it_source_set;
   iter_ptr->it_source_cond = NULL;

   gen_iter_looptop(SETL_SYSTEM iter_ptr);

   return;

}

/*\
 *  \case{default}
 *
 *  We reach the default case when we find a set, tuple, or string
 *  yielding expression which we can not directly iterate over.  In this
 *  case we drop out and form the set.
\*/

default :

{

   break;

}}

/*\
 *  \newpage
 *
 *  If we reach this point, we are unable to use lazy evaluation on the
 *  source set and are forced to create it.  We generate code to make the
 *  set and set up source information accordingly.
\*/

   /* allocate a temporary for the iterator */

   get_temp(iter_ptr->it_iter_var);

   /* create the set */

   left_symtab_ptr = gen_expression(SETL_SYSTEM root,NULL);

   /* start the iterator */

   emitssi(q_iter,
           iter_ptr->it_iter_var,
           left_symtab_ptr,
           iter_ptr->it_type,
           &(root->ast_file_pos));

   /* if the source was an expression, free the temporary */

   if (left_symtab_ptr->st_is_temp) {
      free_temp(left_symtab_ptr);
   }

   /* set the source information in the iterator */

   iter_ptr->it_source_type = it_source_set;
   iter_ptr->it_source_cond = NULL;

   gen_iter_looptop(SETL_SYSTEM iter_ptr);

   return;

}

/*\
 *  \function{gen\_iter\_looptop()}
 *
 *  This function generates the stuff at the top of an iteration loop.
 *  Generally, it produces one value of the iteration variables.
\*/

static void gen_iter_looptop(
   SETL_SYSTEM_PROTO
   c_iter_ptr_type iter_ptr)           /* root of iterator tree             */

{
int found_label;                       /* branch location if we find an     */
                                       /* item                              */
int arith_flip_label;                  /* flip operands (negative           */
                                       /* increment)                        */
int arith_found_label;                 /* found next value for arithmetic   */
                                       /* iterator                          */
symtab_ptr_type operand[3];            /* operand array                     */
int i;                                 /* temporary looping variable        */

   /* if the source is a set ... */

   if (iter_ptr->it_source_type == it_source_set) {

      /* set our loop label */

      emitiss(q_label,
              iter_ptr->it_top_label,
              NULL,
              NULL,
              &null_file_pos);

      /* increment arithmetic iterator */

      if (iter_ptr->it_type == it_arith) {

         emit(q_add,
              iter_ptr->it_next_integer,
              iter_ptr->it_next_integer,
              iter_ptr->it_increment,
              &null_file_pos);

         if (iter_ptr->it_increment == sym_one) {

            emitiss(q_golt,
                    iter_ptr->it_fail_label,
                    iter_ptr->it_last_integer,
                    iter_ptr->it_next_integer,
                    &null_file_pos);

         }
         else {

            arith_flip_label = next_label++;
            arith_found_label = next_label++;

            emitiss(q_gole,
                    arith_flip_label,
                    iter_ptr->it_increment,
                    sym_zero,
                    &null_file_pos);

            emitiss(q_golt,
                    iter_ptr->it_fail_label,
                    iter_ptr->it_last_integer,
                    iter_ptr->it_next_integer,
                    &null_file_pos);

            emitiss(q_go,
                    arith_found_label,
                    NULL,
                    NULL,
                    &null_file_pos);

            emitiss(q_label,
                    arith_flip_label,
                    NULL,
                    NULL,
                    &null_file_pos);

            emitiss(q_goeq,
                    iter_ptr->it_fail_label,
                    iter_ptr->it_increment,
                    sym_zero,
                    &null_file_pos);

            emitiss(q_golt,
                    iter_ptr->it_fail_label,
                    iter_ptr->it_next_integer,
                    iter_ptr->it_last_integer,
                    &null_file_pos);

            emitiss(q_label,
                    arith_found_label,
                    NULL,
                    NULL,
                    &null_file_pos);

         }

         if (!iter_ptr->it_using_bvar) {

            emit(q_assign,
                 iter_ptr->it_bvar[0].it_symtab_ptr,
                 iter_ptr->it_next_integer,
                 NULL,
                 &null_file_pos);

         }
      }

      /* one-bound variable case */

      else if (iter_ptr->it_bvar_count == 1) {

         emitssi(q_inext,
                 iter_ptr->it_bvar[0].it_symtab_ptr,
                 iter_ptr->it_iter_var,
                 iter_ptr->it_fail_label,
                 &null_file_pos);

      }

      /* two bound variable case */

      else {

         emitssi(q_inext,
                 iter_ptr->it_bvar[0].it_symtab_ptr,
                 iter_ptr->it_iter_var,
                 iter_ptr->it_fail_label,
                 &null_file_pos);

         emit(q_noop,
              iter_ptr->it_bvar[1].it_symtab_ptr,
              NULL,
              NULL,
              &null_file_pos);

      }

      /* copy each temporary to the corresponding target */

      for (i = 0; i < iter_ptr->it_bvar_count; i++) {

         if (iter_ptr->it_bvar[i].it_target_ptr != NULL) {
            gen_lhs(SETL_SYSTEM iter_ptr->it_bvar[i].it_target_ptr,
                    iter_ptr->it_bvar[i].it_symtab_ptr);
         }
      }
   }

   /* if the source is another iterator ... */

   else if (iter_ptr->it_source_type == it_source_iter) {

      /* if we have an inclusion condition, check it */

      if (iter_ptr->it_source_cond != NULL) {

         found_label = next_label++;
         gen_boolean(SETL_SYSTEM iter_ptr->it_source_cond,
                     found_label,
                     (iter_ptr->it_source_child)->it_loop_label,
                     found_label);

         emitiss(q_label,
                 found_label,
                 NULL,
                 NULL,
                 &null_file_pos);

      }

      /* copy the generated values into the target */

      if (iter_ptr->it_bvar_count == 1 &&
          (iter_ptr->it_source_child)->it_type == it_pair &&
          iter_ptr->it_bvar[0].it_source_ptr == NULL) {

         emit(q_push2,
              (iter_ptr->it_source_child)->it_bvar[0].it_symtab_ptr,
              (iter_ptr->it_source_child)->it_bvar[1].it_symtab_ptr,
              NULL,
              &null_file_pos);

         emit(q_tuple,
              iter_ptr->it_bvar[0].it_symtab_ptr,
              sym_two,
              NULL,
              &null_file_pos);

      }
      else {

         for (i = 0; i < iter_ptr->it_bvar_count; i++) {

            if (iter_ptr->it_bvar[i].it_source_ptr == NULL) {

               emit(q_assign,
                    iter_ptr->it_bvar[i].it_symtab_ptr,
                    (iter_ptr->it_source_child)->it_bvar[i].it_symtab_ptr,
                    NULL,
                    &null_file_pos);

            }
            else {
               operand[0] =
                  gen_expression(SETL_SYSTEM iter_ptr->it_bvar[i].it_source_ptr,
                                 iter_ptr->it_bvar[i].it_symtab_ptr);

            }

            if (iter_ptr->it_bvar[i].it_target_ptr != NULL) {
               gen_lhs(SETL_SYSTEM iter_ptr->it_bvar[i].it_target_ptr,
                       iter_ptr->it_bvar[i].it_symtab_ptr);
            }
         }
      }
   }

   return;

}

/*\
 *  \function{gen\_iter\_bottom()}
 *
 *  This function generates the code at the bottom of an iterator loop.
 *  We just generate a branch to the top and emit the ending label.
\*/

void gen_iter_bottom(
   SETL_SYSTEM_PROTO
   c_iter_ptr_type iter_ptr)           /* iterator to be finished           */

{

   /* brach back to the top of the loop */

   emitiss(q_go,
           iter_ptr->it_loop_label,
           NULL,
           NULL,
           &null_file_pos);

   /* emit the loop exit label */

   emitiss(q_label,
           iter_ptr->it_fail_label,
           NULL,
           NULL,
           &null_file_pos);

   /* release the iterator */

   gen_iter_clear(SETL_SYSTEM iter_ptr);

}

/*\
 *  \function{gen\_iter\_clear ()}
 *
 *  This function releases the memory used by an iterator, and all
 *  temporaries locked by it.
\*/

static void gen_iter_clear(
   SETL_SYSTEM_PROTO
   c_iter_ptr_type iter_ptr)           /* iterator to be cleared            */

{
c_iter_ptr_type delete_ptr;            /* item to be deleted                */
int i;                                 /* temporary looping variable        */

   /* traverse the iterator list */

   while (iter_ptr != NULL) {

      /* free the temporary variable / child list */

      if (iter_ptr->it_source_type == it_source_set) {

         /* arithmetic iterators */

         if (iter_ptr->it_type == it_arith) {

            if (!(iter_ptr->it_using_bvar)) {
               free_temp(iter_ptr->it_next_integer);
            }

            if (iter_ptr->it_increment != sym_one) {
               free_temp(iter_ptr->it_increment);
            }

            if ((iter_ptr->it_last_integer)->st_is_temp) {
               free_temp(iter_ptr->it_last_integer);
            }

         }

         /* free the iterator variable, for non-arithmetic iterators */

         else {

            free_temp(iter_ptr->it_iter_var);
            for (i = 0; i < iter_ptr->it_bvar_count; i++) {

               if (iter_ptr->it_bvar[i].it_target_ptr != NULL) {
                  free_temp(iter_ptr->it_bvar[i].it_symtab_ptr);
               }

            }
         }
      }

      /* clear children, if any */

      else {
         gen_iter_clear(SETL_SYSTEM iter_ptr->it_source_child);
      }

      /* delete the iterator item */

      delete_ptr = iter_ptr;
      iter_ptr = iter_ptr->it_next;
      free_iter(delete_ptr);

   }
}

