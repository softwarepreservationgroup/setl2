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
 *  \packagespec{Expression Code Generator}
\*/

#ifndef GENEXPR_LOADED

/* SETL2 system header files */

#include "symtab.h"                    /* symbol table                      */
#include "ast.h"                       /* abstract syntax tree              */

/*\
 *  \table{statement generator functions}
\*/

/* expression generator function definition */

typedef symtab_ptr_type((*gen_expr_func)(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type));

#ifdef SHARED

symtab_ptr_type gen_expr_list(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* expression lists                  */
symtab_ptr_type gen_expr_symtab(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* symbol table pointer              */
symtab_ptr_type gen_expr_binop(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* binary operators                  */
symtab_ptr_type gen_expr_andor(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* and & or operators                */
symtab_ptr_type gen_expr_question(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* question mark operator            */
symtab_ptr_type gen_expr_unop(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* binary operators                  */
symtab_ptr_type gen_expr_of(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* string, map, or tuple component   */
symtab_ptr_type gen_expr_ofa(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* multi-valued map reference        */
symtab_ptr_type gen_expr_slice(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* string or tuple slice             */
symtab_ptr_type gen_expr_end(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* string or tuple tail              */
symtab_ptr_type gen_expr_assign(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* assignment expressions            */
symtab_ptr_type gen_expr_assignop(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* assignment operators              */
symtab_ptr_type gen_expr_enum(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* enumerated sets and tuples        */
symtab_ptr_type gen_expr_settup(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* general sets and tuples           */
symtab_ptr_type gen_expr_exists(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* exists expression                 */
symtab_ptr_type gen_expr_forall(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* forall expression                 */
symtab_ptr_type gen_expr_apply(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* application expression            */
symtab_ptr_type gen_expr_binapply(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* binary application expression     */
symtab_ptr_type gen_expr_if(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* if expression                     */
symtab_ptr_type gen_expr_while(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* while expression                  */
symtab_ptr_type gen_expr_until(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* until expression                  */
symtab_ptr_type gen_expr_loop(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* loop expression                   */
symtab_ptr_type gen_expr_for(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* for expression                    */
symtab_ptr_type gen_expr_case(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* case expression                   */
symtab_ptr_type gen_expr_guard(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* guard expression                  */
symtab_ptr_type gen_expr_call(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* procedure calls                   */
symtab_ptr_type gen_expr_error(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* error node                        */
symtab_ptr_type gen_expr_from(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* error node                        */
symtab_ptr_type gen_expr_menviron(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* error node                        */
symtab_ptr_type gen_expr_slot(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* error node                        */
symtab_ptr_type gen_expr_initobj(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* error node                        */
symtab_ptr_type gen_expr_self(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* error node                        */
symtab_ptr_type gen_expr_slotof(SETL_SYSTEM_PROTO ast_ptr_type, symtab_ptr_type);
                                       /* error node                        */

/*\
 *  \table{expression generator functions}
\*/

gen_expr_func gen_expr_table[] = {     /* expression generator functions    */
/* ## begin gen_expr_table */
   gen_expr_error,                     /* null tree                         */
   gen_expr_list,                      /* statement or expression list      */
   gen_expr_error,                     /* name table pointer                */
   gen_expr_symtab,                    /* symbol table pointer              */
   gen_expr_error,                     /* name qualifier                    */
   gen_expr_binop,                     /* +                                 */
   gen_expr_binop,                     /* -                                 */
   gen_expr_binop,                     /* *                                 */
   gen_expr_binop,                     /* /                                 */
   gen_expr_binop,                     /* **                                */
   gen_expr_binop,                     /* MOD                               */
   gen_expr_binop,                     /* MIN                               */
   gen_expr_binop,                     /* MAX                               */
   gen_expr_question,                  /* ?                                 */
   gen_expr_binop,                     /* with operator                     */
   gen_expr_binop,                     /* less operator                     */
   gen_expr_binop,                     /* lessf operator                    */
   gen_expr_binop,                     /* npow operator                     */
   gen_expr_unop,                      /* unary minus                       */
   gen_expr_unop,                      /* unary from                        */
   gen_expr_unop,                      /* map domain                        */
   gen_expr_unop,                      /* map range                         */
   gen_expr_unop,                      /* not                               */
   gen_expr_unop,                      /* arb                               */
   gen_expr_unop,                      /* pow                               */
   gen_expr_unop,                      /* #                                 */
   gen_expr_of,                        /* string, map, or tuple component   */
   gen_expr_ofa,                       /* multi-valued map `of'             */
   gen_expr_of,                        /* string, map, or tuple component   */
                                       /* kill temp after assignment        */
   gen_expr_ofa,                       /* multi-valued map `of'             */
                                       /* kill temp after assignment        */
   gen_expr_slice,                     /* string or tuple slice             */
   gen_expr_end,                       /* string or tuple tail              */
   gen_expr_assign,                    /* general assignment                */
   gen_expr_assignop,                  /* assignment operators              */
   gen_expr_unop,                      /* procedure with environment        */
   gen_expr_error,                     /* constant initialization           */
   gen_expr_error,                     /* place holder in tuple lhs         */
   gen_expr_from,                      /* from operator                     */
   gen_expr_from,                      /* fromb operator                    */
   gen_expr_from,                      /* frome operator                    */
   gen_expr_binop,                     /* =                                 */
   gen_expr_binop,                     /* /=                                */
   gen_expr_binop,                     /* <                                 */
   gen_expr_binop,                     /* <=                                */
   gen_expr_binop,                     /* >                                 */
   gen_expr_binop,                     /* >=                                */
   gen_expr_binop,                     /* in                                */
   gen_expr_binop,                     /* notin                             */
   gen_expr_binop,                     /* in                                */
   gen_expr_binop,                     /* subset                            */
   gen_expr_andor,                     /* or operator                       */
   gen_expr_andor,                     /* and operator                      */
   gen_expr_enum,                      /* enumerated set                    */
   gen_expr_enum,                      /* enumerated tuple                  */
   gen_expr_settup,                    /* general set former                */
   gen_expr_settup,                    /* general tuple former              */
   gen_expr_settup,                    /* general set former without        */
                                       /* expression                        */
   gen_expr_settup,                    /* general tuple former without      */
                                       /* expression                        */
   gen_expr_settup,                    /* arithmetic set former             */
   gen_expr_settup,                    /* arithmetic tuple former           */
   gen_expr_exists,                    /* exists expression                 */
   gen_expr_forall,                    /* forall expression                 */
   gen_expr_apply,                     /* application over set              */
   gen_expr_binapply,                  /* binary application over set       */
   gen_expr_error,                     /* iterator list                     */
   gen_expr_error,                     /* exists iterator list              */
   gen_expr_error,                     /* if statement                      */
   gen_expr_if,                        /* if expression                     */
   gen_expr_loop,                      /* loop statement                    */
   gen_expr_while,                     /* while statement                   */
   gen_expr_until,                     /* until statement                   */
   gen_expr_for,                       /* for statement                     */
   gen_expr_error,                     /* case statement                    */
   gen_expr_case,                      /* case expression                   */
   gen_expr_error,                     /* guard statement                   */
   gen_expr_guard,                     /* guard expression                  */
   gen_expr_error,                     /* when clause                       */
   gen_expr_call,                      /* procedure call                    */
   gen_expr_error,                     /* return statement                  */
   gen_expr_error,                     /* stop statement                    */
   gen_expr_error,                     /* break out of loop                 */
   gen_expr_error,                     /* continue loop                     */
   gen_expr_error,                     /* assert expressions                */
   gen_expr_initobj,                   /* initialize object                 */
   gen_expr_slot,                      /* slot reference                    */
   gen_expr_slotof,                    /* call slot reference               */
   gen_expr_slotof,                    /* call slot reference               */
   gen_expr_menviron,                  /* method with environment           */
                                       /* or instance                       */
   gen_expr_self,                      /* self reference                    */
/* ## end gen_expr_table */
   NULL};

#else

extern gen_expr_func gen_expr_table[]; /* expression generator functions    */

#endif

/* gen_expression macro */

#define gen_expression(r,s) (*(gen_expr_table[(r)->ast_type]))(r,s)

/* public function declarations */

#define GENEXPR_LOADED 1
#endif
