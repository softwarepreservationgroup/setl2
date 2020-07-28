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
 *  \package{Semantic Checks}
 *
 *  This package has two functions:  it resolves name table pointers into
 *  symbol table pointers, and it detects errors which are difficult or
 *  impossible to detect in the parse phase.
 *
 *  During parsing we install symbols in the symbol table as they are
 *  declared but make no attempt to match references to a name with
 *  corresponding symbol table entries.  The reason for this is that we
 *  can not determine if a reference \verb"f" in \verb"f(x)" is to a map
 *  \verb"f" we are to implicitly declare, or to a procedure declared
 *  later in the text until we have seen the entire program.
 *
 *  When this package is invoked, we have parsed the entire file and all
 *  declarations have been processed.  Here we make any implicit
 *  declarations and resolve names into symbols.  At that point we can
 *  also do a lot of semantic checking which is not possible during
 *  parsing.
 *
 *  \texify{semcheck.h}
 *
 *  \packagebody{Semantic Checks}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "ast.h"                       /* abstract syntax tree              */
#include "listing.h"                   /* source and error listings         */

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define printf plugin_printf
#endif

/* constants */


#define STMT         1                 /* subtree should be statement       */

#define LHS_GEN      2                 /* subtree should be unrestricted    */
                                       /* left hand side                    */
#define LHS_BV       4                 /* subtree should be iterator bound  */
                                       /* variable                          */
#define LHS_MAP      8                 /* subtree should be map, tuple, or  */
                                       /* string left hand side             */
#define LHS         14                 /* subtree should be left hand side  */

#define RHS_VAL     16                 /* subtree should yield value        */
#define RHS_COND    32                 /* subtree should be a condition     */
#define RHS_CALL    64                 /* subtree should be call or map     */
#define RHS        112                 /* subtree should be right hand side */

#define CONST      128                 /* subtree should be constant        */
                                       /* expression                        */

/*\
 *  \table{semantic check functions}
\*/

typedef void ((*sem_check_func)(SETL_SYSTEM_PROTO ast_ptr_type, int));
                                       /* semantic check function typedef   */

static void check_sem_null(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* null tree                         */
static void check_sem_list(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* statement or expression list      */
static void check_sem_namtab(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* name table pointer                */
static void check_sem_symtab(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* symbol table pointer              */
static void check_sem_dot(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* name qualifier                    */
static void check_sem_binop(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* binary operators                  */
static void check_sem_unop(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* unary operators                   */
static void check_sem_of(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* string, map, or tuple component   */
static void check_sem_ofa(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* multi-valued map component        */
static void check_sem_slice(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* string or tuple slice             */
static void check_sem_end(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* string or tuple tail              */
static void check_sem_assign(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* assignment expression             */
static void check_sem_assignop(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* assignment operator               */
static void check_sem_cassign(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* constant initialization           */
static void check_sem_place(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* tuple placeholder                 */
static void check_sem_from(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* from expression                   */
static void check_sem_enum_set(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* enumerated set                    */
static void check_sem_enum_tup(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* enumerated tuple                  */
static void check_sem_genset(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* general set former                */
static void check_sem_genset_noexp(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* set former without expression     */
static void check_sem_arith(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* arithmetic set former             */
static void check_sem_exists(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* exists expression                 */
static void check_sem_forall(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* forall expression                 */
static void check_sem_apply(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* unary apply operation             */
static void check_sem_binapply(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* binary apply operation            */
static void check_sem_iter_list(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* iterator list                     */
static void check_sem_ex_iter(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* exists iterator list              */
static void check_sem_if_stmt(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* if statement                      */
static void check_sem_if_expr(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* if expression                     */
static void check_sem_while(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* while expression or statement     */
static void check_sem_loop(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* loop expression or statement      */
static void check_sem_for(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* for expression or statement       */
static void check_sem_case_stmt(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* case statement                    */
static void check_sem_case_expr(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* case expression                   */
static void check_sem_guard_stmt(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* guard statement                   */
static void check_sem_guard_expr(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* guard expression                  */
static void check_sem_when(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* when clause                       */
static void check_sem_return(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* return statment                   */
static void check_sem_stop(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* stop statement                    */
static void check_sem_exit(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* quit statement                    */
static void check_sem_continue(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* continue statement                */
static void check_sem_assert(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* assert statement                  */
static void check_sem_error(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* error condition                   */
static void check_sem_slot(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* instance variable or method       */
static void check_sem_method(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* instance variable or method       */
static void check_sem_self(SETL_SYSTEM_PROTO ast_ptr_type, int);
                                       /* instance variable or method       */

/*\
 *  \table{semantic check function table}
\*/

static sem_check_func sem_check_table[] = {
                                       /* semantic check functions          */
/* ## begin sem_check_table */
   check_sem_null,                     /* null tree                         */
   check_sem_list,                     /* statement or expression list      */
   check_sem_namtab,                   /* name table pointer                */
   check_sem_symtab,                   /* symbol table pointer              */
   check_sem_dot,                      /* name qualifier                    */
   check_sem_binop,                    /* +                                 */
   check_sem_binop,                    /* -                                 */
   check_sem_binop,                    /* *                                 */
   check_sem_binop,                    /* /                                 */
   check_sem_binop,                    /* **                                */
   check_sem_binop,                    /* MOD                               */
   check_sem_binop,                    /* MIN                               */
   check_sem_binop,                    /* MAX                               */
   check_sem_binop,                    /* ?                                 */
   check_sem_binop,                    /* with operator                     */
   check_sem_binop,                    /* less operator                     */
   check_sem_binop,                    /* lessf operator                    */
   check_sem_binop,                    /* npow operator                     */
   check_sem_unop,                     /* unary minus                       */
   check_sem_unop,                     /* unary from                        */
   check_sem_unop,                     /* map domain                        */
   check_sem_unop,                     /* map range                         */
   check_sem_unop,                     /* not                               */
   check_sem_unop,                     /* arb                               */
   check_sem_unop,                     /* pow                               */
   check_sem_unop,                     /* #                                 */
   check_sem_of,                       /* string, map, or tuple component   */
   check_sem_ofa,                      /* multi-valued map `of'             */
   check_sem_of,                       /* string, map, or tuple component   */
                                       /* kill temp after assignment        */
   check_sem_ofa,                      /* multi-valued map `of'             */
                                       /* kill temp after assignment        */
   check_sem_slice,                    /* string or tuple slice             */
   check_sem_end,                      /* string or tuple tail              */
   check_sem_assign,                   /* general assignment                */
   check_sem_assignop,                 /* assignment operators              */
   check_sem_error,                    /* procedure with environment        */
   check_sem_cassign,                  /* constant initialization           */
   check_sem_place,                    /* place holder in tuple lhs         */
   check_sem_from,                     /* from operator                     */
   check_sem_from,                     /* fromb operator                    */
   check_sem_from,                     /* frome operator                    */
   check_sem_binop,                    /* =                                 */
   check_sem_binop,                    /* /=                                */
   check_sem_binop,                    /* <                                 */
   check_sem_binop,                    /* <=                                */
   check_sem_binop,                    /* >                                 */
   check_sem_binop,                    /* >=                                */
   check_sem_binop,                    /* in                                */
   check_sem_binop,                    /* notin                             */
   check_sem_binop,                    /* in                                */
   check_sem_binop,                    /* subset                            */
   check_sem_binop,                    /* or operator                       */
   check_sem_binop,                    /* and operator                      */
   check_sem_enum_set,                 /* enumerated set                    */
   check_sem_enum_tup,                 /* enumerated tuple                  */
   check_sem_genset,                   /* general set former                */
   check_sem_genset,                   /* general tuple former              */
   check_sem_genset_noexp,             /* general set former without        */
                                       /* expression                        */
   check_sem_genset_noexp,             /* general tuple former without      */
                                       /* expression                        */
   check_sem_arith,                    /* arithmetic set former             */
   check_sem_arith,                    /* arithmetic tuple former           */
   check_sem_exists,                   /* exists expression                 */
   check_sem_forall,                   /* forall expression                 */
   check_sem_apply,                    /* application over set              */
   check_sem_binapply,                 /* binary application over set       */
   check_sem_iter_list,                /* iterator list                     */
   check_sem_ex_iter,                  /* exists iterator list              */
   check_sem_if_stmt,                  /* if statement                      */
   check_sem_if_expr,                  /* if expression                     */
   check_sem_loop,                     /* loop statement                    */
   check_sem_while,                    /* while statement                   */
   check_sem_while,                    /* until statement                   */
   check_sem_for,                      /* for statement                     */
   check_sem_case_stmt,                /* case statement                    */
   check_sem_case_expr,                /* case expression                   */
   check_sem_guard_stmt,               /* guard statement                   */
   check_sem_guard_expr,               /* guard expression                  */
   check_sem_when,                     /* when clause                       */
   check_sem_of,                       /* procedure call                    */
   check_sem_return,                   /* return statement                  */
   check_sem_stop,                     /* stop statement                    */
   check_sem_exit,                     /* break out of loop                 */
   check_sem_continue,                 /* continue loop                     */
   check_sem_assert,                   /* assert expressions                */
   check_sem_error,                    /* initialize object                 */
   check_sem_slot,                     /* slot reference                    */
   check_sem_error,                    /* call slot reference               */
   check_sem_error,                    /* call slot reference               */
   check_sem_error,                    /* method with environment           */
                                       /* or instance                       */
   check_sem_self,                     /* self reference                    */
/* ## end sem_check_table */
   NULL};

/* semantic check macro */

#define check_sem(r,i) (*(sem_check_table[(r)->ast_type]))(SETL_SYSTEM r,i)

/* package-global data */

static proctab_ptr_type iter_proctab_ptr;
                                       /* iterator scope                    */
static int loop_level;                 /* loop nesting level                */

/*\
 *  \function{check\_semantics()}
 *
 *  This is the entry function for the semantic check package.
 *  Basically, it just calls the recursive function \verb"check_sem()" to
 *  do the actual checking.
\*/

void check_semantics(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* tree to be processed              */

{

   iter_proctab_ptr = NULL;
   loop_level = 0;
   check_sem(root,STMT);

}

/*\
 *  \astnotree{ast\_null}{null subtree}
 *
 *  We should only find null trees in statements and conditions, not in
 *  left hand side contexts or constants.
\*/

static void check_sem_null(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

#ifdef TRAPS

   if (check_type & LHS)
      trap(__FILE__,__LINE__,msg_null_as_lhs);

   if (check_type & (RHS_VAL | RHS_CALL))
      trap(__FILE__,__LINE__,
           msg_null_as_rhs);

#endif

   return;

}

/*\
 *  \ast{ast\_list}{statement and expression lists}
 *     {\astnode{ast\_list}
 *        {\astnode{{\em first child}}
 *           {\etcast}
 *           {\astnode{{\em second child}}
 *              {\etcast}
 *              {\etcast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles statement and expression lists.  They can
 *  appear in any context (imagine places where tuples are permissible),
 *  so all we do here is loop over the children checking each for
 *  the same condition.
\*/

static void check_sem_list(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type ast_ptr;                  /* used to loop over list            */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* loop over list, checking each subtree */

   for (ast_ptr = root->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

      check_sem(ast_ptr,check_type);

   }

   return;

}

/*\
 *  \astnotree{ast\_name}{name table pointer}
 *
 *  At this point, we match the abstract syntax tree with the symbol
 *  table.  We look up the name in the symbol table first.  If implicit
 *  declarations are enabled, we declare the variable.  We also check if
 *  the symbol is a valid left hand side or constant, if desired.
\*/

static void check_sem_namtab(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
namtab_ptr_type namtab_ptr;            /* name table pointer from AST       */
symtab_ptr_type symtab_ptr;            /* installed symbol table pointer    */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out the name table pointer */

   namtab_ptr = root->ast_child.ast_namtab_ptr;

   /* if we're processing a bound variable list, declare the identifier */

   if (check_type & LHS_BV) {

      symtab_ptr = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                iter_proctab_ptr,
                                &(root->ast_file_pos));
      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;

   }

   /* look up the name in the symbol table */

   for (symtab_ptr = namtab_ptr->nt_symtab_ptr;
        symtab_ptr != NULL && symtab_ptr->st_is_hidden;
        symtab_ptr = symtab_ptr->st_name_link);

   /* if the name isn't in the symbol table, list the error and install it */

   if (symtab_ptr == NULL) {

      if (!IMPLICIT_DECLS) {

         error_message(SETL_SYSTEM
                       &(root->ast_file_pos),
                       msg_missing_id,
                       namtab_ptr->nt_name);

      }

      symtab_ptr = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                curr_proctab_ptr,
                                &(root->ast_file_pos));

      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;

   }

   /* build a symbol table node */

   root->ast_type = ast_symtab;
   root->ast_child.ast_symtab_ptr = symtab_ptr;

   /* call again, passing to symtab check */

   check_sem(root,check_type);

   return;

}

/*\
 *  \astnotree{ast\_symtab}{symbol table pointer}
 *
 *  When we find a symbol table already in the symbol table, we just
 *  check whether it is a valid left or right hand side, if desired.
\*/

static void check_sem_symtab(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
symtab_ptr_type symtab_ptr;            /* installed symbol table pointer    */
proctab_ptr_type proctab_ptr;          /* procedure record for symbol       */
symtab_ptr_type s1;                    /* temporary symtab pointer          */
ast_ptr_type t1,t2;                    /* temporary ast pointers            */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick up the symbol table pointer from the AST */

   symtab_ptr = root->ast_child.ast_symtab_ptr;

   /* constants must be initialized */

   if (check_type & CONST) {

      if (symtab_ptr->st_has_lvalue) {

         error_message(SETL_SYSTEM
                       &(root->ast_file_pos),
                       msg_expected_const);

      }
      else if (!(symtab_ptr->st_is_initialized)) {

         error_message(SETL_SYSTEM
                       &(root->ast_file_pos),
                       msg_uninit_const);

      }

      return;

   }

   /* left hand sides may not be constants */

   else if (check_type & LHS) {

      if (!(symtab_ptr->st_has_lvalue)) {

         error_message(SETL_SYSTEM
                       &(root->ast_file_pos),
                       msg_expected_lhs);

      }

      return;

   }

   /* references to methods and slots are only valid within their class */

   if ((symtab_ptr->st_type == sym_slot ||
          symtab_ptr->st_type == sym_method) &&
       symtab_ptr->st_class != unit_proctab_ptr) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    "%s is an instance variable, but not in this class",
                    (symtab_ptr->st_namtab_ptr)->nt_name);

      return;

   }

   /* right hand sides must have a value (except calls!) */

   if (check_type & (RHS ^ RHS_CALL)) {

      if (!(symtab_ptr->st_has_rvalue)) {

         error_message(SETL_SYSTEM
                       &(root->ast_file_pos),
                       msg_expected_rhs);

         return;

      }

      /* if we find a procedure as a right hand side, copy the environment */

      if (symtab_ptr->st_type == sym_procedure) {

         proctab_ptr = symtab_ptr->st_aux.st_proctab_ptr;
         if (proctab_ptr->pr_parent != NULL &&
             (proctab_ptr->pr_parent)->pr_type == pr_procedure) {

            t1 = get_ast(SETL_SYSTEM_VOID);
            memcpy((void *)t1,
                   (void *)root,
                   sizeof(struct ast_item));
            t1->ast_next = NULL;
            root->ast_type = ast_penviron;
            root->ast_child.ast_child_ast = t1;

         }
      }

      /* if we find a method as a right hand side, copy the environment */

      if (symtab_ptr->st_type == sym_method) {

         proctab_ptr = symtab_ptr->st_aux.st_proctab_ptr;

         t1 = get_ast(SETL_SYSTEM_VOID);
         memcpy((void *)t1,
                (void *)root,
                sizeof(struct ast_item));
         t1->ast_next = NULL;
         root->ast_type = ast_menviron;
         root->ast_child.ast_child_ast = t1;

      }

      return;

   }

   /*
    *  An object creation may be just a reference to the class name, but
    *  the parser can not determine that.  Here we have to fix that with
    *  a little tree surgery.
    */

   if (check_type & STMT && (symtab_ptr->st_type == sym_class ||
                             symtab_ptr->st_type == sym_process)) {

      /* install an empty argument list */

      t2 = get_ast(SETL_SYSTEM_VOID);
      t2->ast_type = ast_list;
      copy_file_pos(&(t2->ast_file_pos),
                    &(root->ast_file_pos));

      /* find the 'create' procedure */

      proctab_ptr = symtab_ptr->st_aux.st_proctab_ptr;
      for (s1 = method_name[m_create]->nt_symtab_ptr;
           s1 != NULL && s1->st_class != proctab_ptr;
           s1 = s1->st_name_link);

      /* make sure we have the right number of arguments */

      if (s1 != NULL) {
         if ((s1->st_aux.st_proctab_ptr)->pr_formal_count != 0) {

            error_message(SETL_SYSTEM
                          &(root->ast_file_pos),
                          "Wrong number of parameters for create");
            return;

         }
      }

      /* splice it into the tree */

      t1 = get_ast(SETL_SYSTEM_VOID);
      if (s1 != NULL && s1->st_type == sym_method) {
         t1->ast_type = ast_symtab;
         t1->ast_child.ast_symtab_ptr = symtab_ptr;
      }
      else {
         t1->ast_type = ast_null;
      }
      t1->ast_next = t2;
      copy_file_pos(&(t1->ast_file_pos),
                    &(root->ast_file_pos));

      /* find the instance initialization procedure */

      for (s1 = method_name[m_initobj]->nt_symtab_ptr;
           s1 != NULL && s1->st_class != proctab_ptr;
           s1 = s1->st_name_link);

#ifdef TRAPS

      if (s1 == NULL) {
         trap(__FILE__,__LINE__,"Class without initialization function",
                                (proctab_ptr->pr_namtab_ptr)->nt_name);
      }

#endif

      /* splice it into the tree */

      t2 = get_ast(SETL_SYSTEM_VOID);
      t2->ast_type = ast_symtab;
      t2->ast_child.ast_symtab_ptr = s1;
      t2->ast_next = t1;
      copy_file_pos(&(t2->ast_file_pos),
                    &(root->ast_file_pos));

      /* copy the root */

      t1 = get_ast(SETL_SYSTEM_VOID);
      memcpy((void *)t1,
             (void *)root,
             sizeof(struct ast_item));
      t1->ast_next = t2;

      root->ast_type = ast_initobj;
      root->ast_child.ast_child_ast = t1;

   }

   return;

}

/*\
 *  \ast{ast\_dot}{period name qualifiers and selectors}
 *     {\astnode{ast\_dot}
 *        {\astnode{{\em name}}
 *           {\etcast}
 *           {\astnode{{\em name}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  The period operator is heavily overloaded: if the left operand is a
 *  procedure, program, class, or package, the right operand must be an
 *  identifier in the respective procedure, program, class, or package.
 *  Otherwise, the left operand is some general expression and the right
 *  operand is a declared selector or a slot.
 *
 *  We transform period operators here, early in the translation process
 *  as if they were macros.  In fact, they are intended to replace macros
 *  in SETL.
 *
 *  A high level description of the algorithm is as follows:  we traverse
 *  the list of nodes from left to right.  At each node, we pick up
 *  either some description of the scope which must contain the following
 *  name, a selector, or an expression to be selected.  As we see names
 *  we try to use the description of an outer scope found previously.
\*/

static void check_sem_dot(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
namtab_ptr_type namtab_ptr;            /* name table pointer from AST       */
symtab_ptr_type symtab_ptr;            /* symbol table search pointer       */
ast_ptr_type node_ptr;                 /* used to loop over node list       */
ast_ptr_type expression_ptr;           /* expression being selected from    */
                                       /* or qualified name                 */
proctab_ptr_type owner_proctab_ptr;    /* procedure to be searched for      */
                                       /* symbol                            */
int owner_unit_num;                    /* unit to be searched for symbol    */
ast_ptr_type t1,t2;                    /* work AST pointers                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* by default we search all visible names */

   owner_proctab_ptr = NULL;
   owner_unit_num = -1;
   expression_ptr = NULL;

   /* traverse the list of AST nodes */

   node_ptr = root->ast_child.ast_child_ast;
   while (node_ptr != NULL) {

      /*
       *  Most of the work here is in resolving names.  First, we look up
       *  the name following visibility rules in effect now.
       */

      if (node_ptr->ast_type == ast_namtab) {

         /* look up the name in the symbol table */

         namtab_ptr = node_ptr->ast_child.ast_namtab_ptr;
         for (symtab_ptr = namtab_ptr->nt_symtab_ptr;
              symtab_ptr != NULL;
              symtab_ptr = symtab_ptr->st_name_link) {

            /* if owner_unit_num > 1, we've specified an imported package */

            if (owner_unit_num > 1) {

               if (symtab_ptr->st_unit_num == owner_unit_num) {

                  /* slots may never be qualified */

                  if (symtab_ptr->st_type == sym_slot) {

                     error_message(SETL_SYSTEM
                                   &(node_ptr->ast_file_pos),
                                   "Can not qualify instance variable => %s",
                                   namtab_ptr->nt_name);

                     root->ast_child.ast_child_ast = NULL;
                     return;

                  }
                  else if (symtab_ptr->st_type == sym_method &&
                           symtab_ptr->st_class != unit_proctab_ptr) {

                     /* methods may be qualified only within a class */

                     error_message(SETL_SYSTEM
                                   &(node_ptr->ast_file_pos),
                                  "Can not qualify method => %s",
                                   namtab_ptr->nt_name);

                     root->ast_child.ast_child_ast = NULL;
                     return;

                  }

                  break;

               }
               else
                  continue;

            }

            /* if owner_proctab_ptr != NULL, we've specified a procedure */

            if (owner_unit_num <= 1 && owner_proctab_ptr != NULL) {

               if (symtab_ptr->st_owner_proc == owner_proctab_ptr)
                  break;
               else
                  continue;

            }

            /* otherwise we have no restrictions yet */

            if (!(symtab_ptr->st_is_hidden))
               break;

         }

         /*
          *  If we didn't find the name, we might have to declare an
          *  identifier, depending on compiler option.
          */

         if (symtab_ptr == NULL) {

            if (owner_proctab_ptr != NULL) {

               error_message(SETL_SYSTEM
                             &(node_ptr->ast_file_pos),
                             "Identifier %s is not in %s",
                             namtab_ptr->nt_name,
                             (owner_proctab_ptr->pr_namtab_ptr)->nt_name);

               root->ast_child.ast_child_ast = NULL;
               return;

            }

            if (!IMPLICIT_DECLS) {

               error_message(SETL_SYSTEM
                             &(node_ptr->ast_file_pos),
                             msg_missing_id,
                             namtab_ptr->nt_name);

               root->ast_child.ast_child_ast = NULL;
               return;

            }

            /* we declare the symbol as a normal identifier */

            symtab_ptr = enter_symbol(SETL_SYSTEM
                                      namtab_ptr,
                                      curr_proctab_ptr,
                                      &(root->ast_file_pos));

            symtab_ptr->st_type = sym_id;
            symtab_ptr->st_has_lvalue = YES;
            symtab_ptr->st_has_rvalue = YES;

         }

         /*
          *  At this point, we've found the symbol corresponding to the
          *  given name.
          */

         /* set the procedure pointer if we found an enclosing unit */

         if (symtab_ptr->st_type == sym_package ||
             symtab_ptr->st_type == sym_class ||
             symtab_ptr->st_type == sym_process ||
             symtab_ptr->st_type == sym_program) {

            owner_proctab_ptr = symtab_ptr->st_aux.st_proctab_ptr;
            owner_unit_num = -1;
            t1 = node_ptr;
            node_ptr = node_ptr->ast_next;
            free_ast(t1);

            continue;

         }

         /* check for enclosing procedures */

         if ((symtab_ptr->st_type == sym_procedure ||
               symtab_ptr->st_type == sym_method) &&
             node_ptr->ast_next != NULL) {

            owner_proctab_ptr = symtab_ptr->st_aux.st_proctab_ptr;
            owner_unit_num = -1;
            t1 = node_ptr;
            node_ptr = node_ptr->ast_next;
            free_ast(t1);

            continue;

         }

         /* a procedure without a following name is an expression itself */

         if (symtab_ptr->st_type == sym_procedure) {

            if (expression_ptr != NULL) {

               error_message(SETL_SYSTEM
                             &(node_ptr->ast_file_pos),
                             msg_expected_selector,
                             namtab_ptr->nt_name);

               root->ast_child.ast_child_ast = NULL;
               return;

            }

            owner_proctab_ptr = NULL;
            owner_unit_num = -1;
            expression_ptr = node_ptr;
            expression_ptr->ast_type = ast_symtab;
            expression_ptr->ast_child.ast_symtab_ptr = symtab_ptr;
            node_ptr = node_ptr->ast_next;

            continue;

         }

         /* set the unit number if we find an imported package */

         if (symtab_ptr->st_type == sym_use ||
             symtab_ptr->st_type == sym_inherit) {

            owner_proctab_ptr = symtab_ptr->st_aux.st_proctab_ptr;
            owner_unit_num = symtab_ptr->st_unit_num;
            t1 = node_ptr;
            node_ptr = node_ptr->ast_next;
            free_ast(t1);

            continue;

         }

         /* change the ast to correct selectors */

         if (symtab_ptr->st_type == sym_selector) {

            /* we must have an expression to select from */

            if (expression_ptr == NULL) {

               error_message(SETL_SYSTEM
                             &(node_ptr->ast_file_pos),
                             msg_missing_sel_exp,
                             namtab_ptr->nt_name);

               root->ast_child.ast_child_ast = NULL;
               return;

            }

            owner_proctab_ptr = NULL;
            owner_unit_num = -1;

            /* build a new 'of' subtree */

            t1 = get_ast(SETL_SYSTEM_VOID);
            t1->ast_type = ast_symtab;
            t1->ast_child.ast_symtab_ptr =
               symtab_ptr->st_aux.st_selector_ptr;
            copy_file_pos(&(t1->ast_file_pos),
                          &(node_ptr->ast_file_pos));
            t2 = t1;

            t1 = get_ast(SETL_SYSTEM_VOID);
            t1->ast_type = ast_list;
            t1->ast_child.ast_child_ast = t2;
            copy_file_pos(&(t1->ast_file_pos),
                          &(node_ptr->ast_file_pos));
            t2 = t1;

            t1 = get_ast(SETL_SYSTEM_VOID);
            t1->ast_type = ast_of;
            t1->ast_child.ast_child_ast = expression_ptr;
            expression_ptr->ast_next = t2;
            copy_file_pos(&(t1->ast_file_pos),
                          &(node_ptr->ast_file_pos));
            expression_ptr = t1;

            t1 = node_ptr;
            node_ptr = node_ptr->ast_next;
            free_ast(t1);

            continue;

         }

         /* change the ast to correct slots */

         if (symtab_ptr->st_type == sym_method ||
             symtab_ptr->st_type == sym_slot) {

            /* slots without expressions must be implicitly self */

            if (expression_ptr == NULL) {

               if (symtab_ptr->st_class != unit_proctab_ptr) {

                  error_message(SETL_SYSTEM
                                &(node_ptr->ast_file_pos),
                                "Missing object containing %s",
                                namtab_ptr->nt_name);

                  root->ast_child.ast_child_ast = NULL;
                  return;

               }
            }

            /* if we have an expression, change to slot */

            else {

               owner_proctab_ptr = NULL;
               owner_unit_num = -1;

               /* build a new 'slot' subtree */

               node_ptr->ast_type = ast_symtab;
               node_ptr->ast_child.ast_symtab_ptr = symtab_ptr;

               t1 = get_ast(SETL_SYSTEM_VOID);
               t1->ast_type = ast_slot;
               t1->ast_child.ast_child_ast = expression_ptr;
               expression_ptr->ast_next = node_ptr;
               copy_file_pos(&(t1->ast_file_pos),
                             &(node_ptr->ast_file_pos));
               expression_ptr = t1;

               t1 = node_ptr;
               node_ptr = node_ptr->ast_next;
               t1->ast_next = NULL;

               continue;

            }
         }

         /* anything else must be an expression to be selected from */

         if (expression_ptr != NULL) {

            error_message(SETL_SYSTEM
                          &(node_ptr->ast_file_pos),
                          msg_expected_selector,
                          namtab_ptr->nt_name);

            root->ast_child.ast_child_ast = NULL;
            return;

         }

         owner_proctab_ptr = NULL;
         owner_unit_num = -1;
         expression_ptr = node_ptr;
         expression_ptr->ast_type = ast_symtab;
         expression_ptr->ast_child.ast_symtab_ptr = symtab_ptr;
         node_ptr = node_ptr->ast_next;

         continue;

      }

      /*
       *  At this point we know we have some expression.  We make sure we
       *  don't have an extra expression, and install it.
       */

       else {

         if (expression_ptr != NULL) {

            error_message(SETL_SYSTEM
                          &(node_ptr->ast_file_pos),
                          msg_expected_selector,
                          namtab_ptr->nt_name);

            root->ast_child.ast_child_ast = NULL;
            return;

         }

         owner_proctab_ptr = NULL;
         owner_unit_num = -1;
         expression_ptr = node_ptr;
         node_ptr = node_ptr->ast_next;

         continue;

      }
   }

   /*
    *  Now we've reached the end of the chain of names.  We should have
    *  found an expression.
    */

   if (expression_ptr == NULL) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_missing_exp);

      root->ast_child.ast_child_ast = NULL;
      return;

   }

   /* copy the expression to the root & free the expression pointer */

   expression_ptr->ast_next = root->ast_next;
   if (expression_ptr != root) {

      memcpy((void *)root,
             (void *)expression_ptr,
             sizeof(struct ast_item));
      free_ast(expression_ptr);

   }

   /* finally, we can actually perform the semantic checks */

   check_sem(root,check_type);

   return;

}

/*\
 *  \ast{ast\_add}{binary operators}
 *     {\astnode{{\em operator}}
 *        {\astnode{{\em left hand side}}
 *           {\etcast}
 *           {\astnode{{\em right hand side}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  Binary operators are valid in right hand side contexts (constant or
 *  not), but not in any left hand sides or statements.
\*/

static void check_sem_binop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type t1,t2;
#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* check the children */

   check_sem(root->ast_child.ast_child_ast,
             (check_type | (LHS | STMT)) ^ (LHS | STMT));
   check_sem((root->ast_child.ast_child_ast)->ast_next,
             (check_type | (LHS | STMT)) ^ (LHS | STMT));

   if ((root->ast_type==ast_sub)&&(root->ast_extension!=NULL)) {
     
     
		t1 = get_ast(SETL_SYSTEM_VOID);
		memcpy((void *)t1,
		   (void *)root,
		   sizeof(struct ast_item));
		t1->ast_type = ast_list;
		t1->ast_next = NULL;

		t2 = get_ast(SETL_SYSTEM_VOID);
		t2->ast_type = ast_namtab;
		t2->ast_child.ast_namtab_ptr = root->ast_extension;
		t2->ast_next = t1;
		copy_file_pos(&(t2->ast_file_pos),
		             &(root->ast_file_pos));

		root->ast_type = ast_of;
		root->ast_child.ast_child_ast = t2;

		check_sem(root,STMT);
   }
   return;

}

/*\
 *  \ast{ast\_uminus}{unary operators}
 *     {\astnode{{\em operator}}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\nullast}
 *        }
 *        {\etcast}
 *     }
 *
 *  A unary operator is not allowed on the left or in statements, but is
 *  fine everywhere else.
\*/

static void check_sem_unop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type t1,t2;

   /* unary operators can not be on the left */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }
 

   check_sem(root->ast_child.ast_child_ast,
             (check_type | (LHS | STMT)) ^ (LHS | STMT));
             
          
   if ((root->ast_type==ast_uminus)&&(root->ast_extension!=NULL)) {
  		
            t1 = get_ast(SETL_SYSTEM_VOID);
            memcpy((void *)t1,
                   (void *)root,
                   sizeof(struct ast_item));
            t1->ast_type = ast_list;
            t1->ast_next = NULL;
            
            t2 = get_ast(SETL_SYSTEM_VOID);
            t2->ast_type = ast_namtab;
            t2->ast_child.ast_namtab_ptr = root->ast_extension;
            t2->ast_next = t1;
            copy_file_pos(&(t2->ast_file_pos),
                             &(root->ast_file_pos));
         
            root->ast_type = ast_of;
            root->ast_child.ast_child_ast = t2;

 			check_sem(root,STMT);
   
   }

   return;

}

/*\
 *  \ast{ast\_of}{procedure calls, map and tuple references}
 *     {\astnode{ast\_of}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{ast\_list}
 *              {\astnode{{\em argument 1}}
 *                 {\etcast}
 *                 {\astnode{{\em argument 2}}
 *                    {\etcast}
 *                    {\etcast}
 *                 }
 *              }
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This node can be a bunch of different things, so we have a lot of
 *  work to do.  If the left hand side is a literal procedure, we check
 *  the number of arguments.  If a class, we transform it into an
 *  \verb"ast_initobj" node.  If a slot, we transform it into a
 *  \verb"ast_slotcall" node.
\*/

static void check_sem_of(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
proctab_ptr_type proctab_ptr;          /* procedure table item              */
symtab_ptr_type symtab_ptr;            /* parameter pointer                 */
ast_ptr_type arg_ptr;                  /* pointer to argument               */
ast_ptr_type ast_ptr;                  /* work pointer for tree surgery     */
int arg_count;                         /* number of arguments               */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);
      fprintf(DEBUG_FILE,"CHK : type = %d \n",check_type);

   }

#endif

   /* we don't allow sinister assignments with bound variables */

   if (check_type & LHS_BV) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_bad_iter_lhs);

   }

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* if used on the left, the target must be a variable */

   if (check_type & LHS) {

      check_sem(left_ptr,LHS_MAP);
      check_sem(right_ptr,RHS_VAL);

      return;

   }

   /*
    *  At this point, we know we have a right hand side, a statement, or
    *  a condition.  We do not know whether we have a procedure call, a
    *  map, a tuple or a string reference, an object initialization or a
    *  method call.
    */

   /* count the actual parameters */

   arg_count = 0;
   for (arg_ptr = right_ptr->ast_child.ast_child_ast;
        arg_ptr != NULL;
        arg_ptr = arg_ptr->ast_next, arg_count++);

   /* check the left hand side */

   if (check_type & RHS)
      check_sem(left_ptr,((check_type | RHS) ^ RHS) | RHS_CALL);
   else
      check_sem(left_ptr,((check_type | STMT) ^ STMT) | RHS_CALL);

   /*
    *  If we find a literal procedure we can check the arguments and
    *  convert it to a literal call, which is cheaper than a general
    *  call.
    */

   if (left_ptr->ast_type == ast_symtab &&
       ((left_ptr->ast_child.ast_symtab_ptr)->st_type == sym_procedure ||
        (left_ptr->ast_child.ast_symtab_ptr)->st_type == sym_method)) {

      /* don't allow calls in constant initialization */

      if (check_type & CONST) {

         error_message(SETL_SYSTEM
                       &(root->ast_file_pos),
                       msg_expected_const);

      }

      /* pick out the procedure record */

      proctab_ptr =
         (left_ptr->ast_child.ast_symtab_ptr)->st_aux.st_proctab_ptr;

      /* make sure the actual parameters are compatible with the formal */

      if (proctab_ptr->pr_var_args) {

         if (arg_count < proctab_ptr->pr_formal_count) {

            error_message(SETL_SYSTEM
                          &(right_ptr->ast_file_pos),
                          msg_bad_proc_call);
            return;

         }
      }
      else {

         if (arg_count != proctab_ptr->pr_formal_count) {

            error_message(SETL_SYSTEM
                          &(right_ptr->ast_file_pos),
                          msg_too_few_parms);
            return;

         }
      }

      /* code directly as a procedure call */

      root->ast_type = ast_call;

      /* check children (dependent on parameter mode) */

      symtab_ptr = proctab_ptr->pr_symtab_head;
      arg_count = 0;
      for (arg_ptr = right_ptr->ast_child.ast_child_ast;
           arg_ptr != NULL;
           arg_ptr = arg_ptr->ast_next, arg_count++) {

         if (symtab_ptr->st_is_rparam)
            check_sem(arg_ptr,(check_type & CONST) | RHS_VAL);

         if (symtab_ptr->st_is_wparam)
            check_sem(arg_ptr,(check_type & CONST) | LHS_GEN);

         if (arg_count < proctab_ptr->pr_formal_count)
            symtab_ptr = symtab_ptr->st_thread;

      }

      return;

   }

   /*
    *  An object creation is just like a procedure call or a map
    *  reference, as far as the parser is concerned.  Here we have to fix
    *  that with a little tree surgery, however.
    */

   if (left_ptr->ast_type == ast_symtab &&
       ((left_ptr->ast_child.ast_symtab_ptr)->st_type == sym_class ||
        (left_ptr->ast_child.ast_symtab_ptr)->st_type == sym_process)) {

      /* we don't allow this in statements */

      if (check_type & STMT) {

         error_message(SETL_SYSTEM
                       &(root->ast_file_pos),
                       msg_rhs_as_statement);

      }

      /* find the 'create' procedure */

      proctab_ptr =
            (left_ptr->ast_child.ast_symtab_ptr)->st_aux.st_proctab_ptr;
      for (symtab_ptr = method_name[m_create]->nt_symtab_ptr;
           symtab_ptr != NULL &&
              (symtab_ptr->st_class != proctab_ptr ||
               symtab_ptr->st_type != sym_method);
           symtab_ptr = symtab_ptr->st_name_link);

      /* make sure we have the right number of arguments */

      if (symtab_ptr != NULL) {
         if (arg_count !=
                (symtab_ptr->st_aux.st_proctab_ptr)->pr_formal_count) {

            error_message(SETL_SYSTEM
                          &(right_ptr->ast_file_pos),
                          "Wrong number of parameters for create");
            return;

         }
      }
      else {
         if (arg_count != 0) {

            error_message(SETL_SYSTEM
                          &(right_ptr->ast_file_pos),
                          "There is no create procedure for %s",
                          (proctab_ptr->pr_namtab_ptr)->nt_name);
            return;

         }
      }

      /* splice it into the tree */

      ast_ptr = get_ast(SETL_SYSTEM_VOID);
      if (symtab_ptr != NULL && symtab_ptr->st_type == sym_method) {
         ast_ptr->ast_type = ast_symtab;
         ast_ptr->ast_child.ast_symtab_ptr = symtab_ptr;
      }
      else {
         ast_ptr->ast_type = ast_null;
      }
      ast_ptr->ast_next = left_ptr->ast_next;
      left_ptr->ast_next = ast_ptr;
      copy_file_pos(&(ast_ptr->ast_file_pos),
                    &(root->ast_file_pos));

      /* find the instance initialization procedure */

      for (symtab_ptr = method_name[m_initobj]->nt_symtab_ptr;
           symtab_ptr != NULL && symtab_ptr->st_class != proctab_ptr;
           symtab_ptr = symtab_ptr->st_name_link);

#ifdef TRAPS

      if (symtab_ptr == NULL) {
         trap(__FILE__,__LINE__,"Class without initialization function",
                                (proctab_ptr->pr_namtab_ptr)->nt_name);
      }

#endif

      /* splice it into the tree */

      ast_ptr = get_ast(SETL_SYSTEM_VOID);
      ast_ptr->ast_type = ast_symtab;
      ast_ptr->ast_child.ast_symtab_ptr = symtab_ptr;
      ast_ptr->ast_next = left_ptr->ast_next;
      left_ptr->ast_next = ast_ptr;
      copy_file_pos(&(ast_ptr->ast_file_pos),
                    &(root->ast_file_pos));

      /* check the arguments */

      check_sem(right_ptr,(check_type & CONST) | RHS_VAL);

      root->ast_type = ast_initobj;
      return;

   }

   /*
    *  If the left operand is a slot, we transform the node into an
    *  ast_slotcall.
    */

   if (left_ptr->ast_type == ast_slot) {

      if (arg_count == 0 || (check_type & STMT))
         root->ast_type = ast_slotcall;
      else
         root->ast_type = ast_slotof;

      /* check the arguments */

      check_sem(right_ptr,(check_type & CONST) | RHS_VAL);

      return;

   }

   /* check the children */

   check_sem(right_ptr,(check_type & CONST) | RHS_VAL);

   /* the zero-argument form must be a procedure call */

   if (arg_count == 0)
      root->ast_type = ast_call;

   /* if used as a statement, this must be a procedure call */

   if (check_type & STMT)
      root->ast_type = ast_call;

   return;

}

/*\
 *  \ast{ast\_ofa}{multi-valued map references or assignments}
 *     {\astnode{ast\_ofa}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{ast\_list}
 *              {\astnode{{\em argument 1}}
 *                 {\etcast}
 *                 {\astnode{{\em argument 2}}
 *                    {\etcast}
 *                    {\etcast}
 *                 }
 *              }
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles multi-valued map references or assignments.
 *  When used on the left, the left child must be something which can
 *  accept an assignment (eventually a simple variable, as we descend
 *  left children).  When used on the right both children must yield
 *  acceptable right hand side values.
\*/

static void check_sem_ofa(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* we don't allow sinister assignments with bound variables */

   if (check_type & LHS_BV) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_bad_iter_lhs);

   }

   /* value expressions can not be used as statements */

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* we must have at least one argument */

   if (right_ptr->ast_child.ast_child_ast == NULL) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_missing_map_arg);

   }

   /* if used on the left, the target must be a variable */

   if (check_type & LHS) {

      check_sem(left_ptr,LHS_MAP);
      check_sem(right_ptr,RHS_VAL);

      return;

   }

   /* if used on the right, the map and arguments must be right hand side */

   check_sem(left_ptr,(check_type & CONST) | RHS_VAL);
   check_sem(right_ptr,(check_type & CONST) | RHS_VAL);

   return;

}

/*\
 *  \ast{ast\_slice}{string or tuple slice}
 *     {\astnode{ast\_slice}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{{\em begin}}
 *              {\etcast}
 *              {\astnode{{\em end}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles string and tuple slice references or
 *  assignments.  When used on the left, the left child must be something
 *  which can accept an assignment (eventually a simple variable, as we
 *  descend left children).  When used on the right all children must
 *  yield acceptable right hand side values.
\*/

static void check_sem_slice(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type end_ptr;                  /* end of slice node                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* we don't allow sinister assignments with bound variables */

   if (check_type & LHS_BV) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_bad_iter_lhs);

   }

   /* value expressions can not be used as statements */

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;
   end_ptr = right_ptr->ast_next;

   /* if used on the left, the target must be a variable */

   if (check_type & LHS) {

      check_sem(left_ptr,LHS_MAP);
      check_sem(right_ptr,RHS_VAL);
      check_sem(end_ptr,RHS_VAL);

      return;

   }

   /* the string and arguments must be right hand side */

   check_sem(left_ptr,(check_type & CONST) | RHS_VAL);
   check_sem(right_ptr,(check_type & CONST) | RHS_VAL);
   check_sem(end_ptr,(check_type & CONST) | RHS_VAL);

   return;

}

/*\
 *  \ast{ast\_end}{string or tuple tail}
 *     {\astnode{ast\_end}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{{\em begin}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles string and tuple tail references or
 *  assignments.  When used on the left, the left child must be something
 *  which can accept an assignment (eventually a simple variable, as we
 *  descend left children).  When used on the right all children must
 *  yield acceptable right hand side values.
\*/

static void check_sem_end(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* we don't allow sinister assignments with bound variables */

   if (check_type & LHS_BV) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_bad_iter_lhs);

   }

   /* value expressions can not be used as statements */

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* if used on the left, the target must be a variable */

   if (check_type & LHS) {

      check_sem(left_ptr,LHS_MAP);
      check_sem(right_ptr,RHS_VAL);

      return;

   }

   /* the string and arguments must be right hand side */

   check_sem(left_ptr,(check_type & CONST) | RHS_VAL);
   check_sem(right_ptr,(check_type & CONST) | RHS_VAL);

   return;

}

/*\
 *  \ast{ast\_assign}{assignment expressions}
 *     {\astnode{ast\_assign}
 *        {\astnode{{\em left hand side}}
 *           {\etcast}
 *           {\astnode{{\em right hand side}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  The grammar does not prevent assignment expressions from appearing in
 *  constant initialization expressions, so we must check that here.
\*/

static void check_sem_assign(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* check the children */

   check_sem(root->ast_child.ast_child_ast,LHS_GEN);
   check_sem((root->ast_child.ast_child_ast)->ast_next,RHS_VAL);

   return;

}

/*\
 *  \ast{ast\_cassign}{constant assignment expressions}{
 *     \astnode{ast\_cassign}
 *        {\astnode{{\em left hand side}}
 *           {\etcast}
 *           {\astnode{{\em right hand side}}
 *              {\etcast}
 *              {\etcast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This node type corresponds to constant assignments.  We just check
 *  that the right hand side is a constant, and change the node type to
 *  an ordinary assignment.
\*/

static void check_sem_cassign(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
symtab_ptr_type symtab_ptr;            /* constant symbol                   */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* check the children */

   check_sem(root->ast_child.ast_child_ast,0);
   check_sem((root->ast_child.ast_child_ast)->ast_next,CONST);

   /* the left must be a symbol, which is now initialized */

   symtab_ptr = (root->ast_child.ast_child_ast)->ast_child.ast_symtab_ptr;
   symtab_ptr->st_is_initialized = YES;

   root->ast_type = ast_assign;

   return;

}

/*\
 *  \ast{ast\_assignop}{assignment operator expressions}
 *     {\astnode{ast\_binop}
 *        {\astnode{ast\_assignop}
 *           {\astnode{{\em left hand side}}
 *              {\etcast}
 *              {\astnode{{\em right hand side}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *           {\etcast}
 *        }
 *        {\etcast}
 *     }
 *
 *  Assignment operators are a little tricky, since we do not want to
 *  evaluate indices to sinister assignments twice.
\*/

static void check_sem_assignop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type index_ptr;                /* index node                        */

   /* assignment operators may not appear on left or in constants */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* pick out child pointers, for readability */

   left_ptr = (root->ast_child.ast_child_ast)->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* the left child must be valid on the left or right */

   check_sem(left_ptr,LHS_GEN | RHS_VAL);
   check_sem(right_ptr,RHS_VAL);

   /*
    *  NOTE: small, ugly surgery here.  If the left operand is a map
    *  reference, and it has more than one index, we have to change the
    *  list node to an enumerated tuple!
    */

   if (left_ptr->ast_type == ast_of || left_ptr->ast_type == ast_ofa) {

      right_ptr = (left_ptr->ast_child.ast_child_ast)->ast_next;
      if ((right_ptr->ast_child.ast_child_ast)->ast_next != NULL) {

         index_ptr = get_ast(SETL_SYSTEM_VOID);
         copy_file_pos(&(index_ptr->ast_file_pos),
                       &(right_ptr->ast_file_pos));
         index_ptr->ast_type = ast_enum_tup;
         index_ptr->ast_child.ast_child_ast =
            right_ptr->ast_child.ast_child_ast;
         right_ptr->ast_child.ast_child_ast = index_ptr;

      }
   }

   return;

}

/*\
 *  \astnotree{ast\_placeholder}{tuple placeholders}
 *
 *  A tuple placeholder is a dash appearing as a tuple element, in tuples
 *  used on the left.  The grammar accepts them anywhere, but they are
 *  only valid on the left.
\*/

static void check_sem_place(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

   /* placeholders must be on the left */

   if (!(check_type & LHS) ||
       check_type & (RHS | CONST | STMT)) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_bad_dash);

   }

   return;

}

/*\
 *  \ast{ast\_from}{from operators}
 *     {\astnode{{\em operator}}
 *        {\astnode{{\em left hand side}}
 *           {\etcast}
 *           {\astnode{{\em right hand side}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  From operators are valid in statements or on the right.  They are not
 *  valid in constants or left hand sides.
\*/

static void check_sem_from(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* check the children */

   check_sem(root->ast_child.ast_child_ast,LHS_GEN);
   check_sem((root->ast_child.ast_child_ast)->ast_next,LHS_MAP);

   return;

}

/*\
 *  \ast{ast\_enum\_set}{enumerated set formers}
 *     {\astnode{ast\_enum\_set}
 *        {\astnode{{\em element 1}}
 *           {\etcast}
 *           {\astnode{{\em element 2}}
 *              {\etcast}
 *              {\etcast}
 *           }
 *        }
 *        {\nullast}
 *     }
 *
 *  Enumerated set formers may not appear on the left hand side.  They may
 *  appear in constants, provided all children are constants.
\*/

static void check_sem_enum_set(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type elem_ptr;                 /* pointer to set element            */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* set formers can not appear on the left hand side */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* check the children */

   for (elem_ptr = root->ast_child.ast_child_ast;
        elem_ptr != NULL;
        elem_ptr = elem_ptr->ast_next) {

      check_sem(elem_ptr,(check_type | LHS | STMT) ^ (LHS | STMT));

   }
   return;

}

/*\
 *  \ast{ast\_enum\_tup}{enumerated tuple formers}
 *     {\astnode{ast\_enum\_tup}
 *        {\astnode{{\em element 1}}
 *           {\etcast}
 *           {\astnode{{\em element 2}}
 *              {\etcast}
 *              {\etcast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  Enumerate tuple formers may appear in general left hand sides or right
 *  hand sides, but not in map left hand sides.
\*/

static void check_sem_enum_tup(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type elem_ptr;                 /* pointer to tuple element          */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   if (check_type & LHS_MAP) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   /* check each child */

   for (elem_ptr = root->ast_child.ast_child_ast;
        elem_ptr != NULL;
        elem_ptr = elem_ptr->ast_next) {

      check_sem(elem_ptr,(check_type | LHS_MAP | STMT) ^ (LHS_MAP | STMT));

   }

   return;

}

/*\
 *  \ast{ast\_genset}{set and tuple formers}
 *     {\astnode{ast\_genset}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{{\em iter list}}
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
 *  This function handles the most general form of a set or tuple former.  We
 *  open a new scope for the iteration, and check the expression and
 *  condition.
\*/

static void check_sem_genset(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type condition_ptr;            /* inclusion condition               */
ast_ptr_type expression_ptr;           /* loop body list                    */
ast_ptr_type ast_ptr;                  /* iterator pointer                  */
proctab_ptr_type old_proctab_ptr;      /* proctab item to be deleted        */
symtab_ptr_type symtab_ptr;            /* used to loop over bound variables */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* set and tuple formers are invalid in left hand sides */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* pick out child pointers, for readability */

   expression_ptr = root->ast_child.ast_child_ast;
   iter_list_ptr = expression_ptr->ast_next;
   condition_ptr = iter_list_ptr->ast_next;

   /* check the iterator list, opening iterator scopes */

   check_sem(iter_list_ptr,check_type & CONST);

   /* check the other children */

   check_sem(condition_ptr,RHS_COND);
   check_sem(expression_ptr,RHS_VAL);

   /* close the iterator scopes */

   for (ast_ptr = iter_list_ptr->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

      /* move bound variables to the current scope, and detach them */

      detach_symtab(iter_proctab_ptr->pr_symtab_head);

      for (symtab_ptr = iter_proctab_ptr->pr_symtab_head;
           symtab_ptr != NULL;
           symtab_ptr = symtab_ptr->st_thread) {

         symtab_ptr->st_owner_proc = curr_proctab_ptr;

      }

      if (iter_proctab_ptr->pr_symtab_head != NULL) {

         *(curr_proctab_ptr->pr_symtab_tail) =
            iter_proctab_ptr->pr_symtab_head;
         curr_proctab_ptr->pr_symtab_tail =
            iter_proctab_ptr->pr_symtab_tail;

      }

      old_proctab_ptr = iter_proctab_ptr;
      iter_proctab_ptr = iter_proctab_ptr->pr_parent;
      free_proctab(old_proctab_ptr);

   }

   return;

}

/*\
 *  \ast{ast\_genset\_noexp}{set formers without expression}
 *     {\astnode{ast\_genset}
 *        {\astnode{{\em iter list}}
 *           {\etcast}
 *           {\astnode{{\em condition}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles set and tuple formers in which we are not passed
 *  an expression.  We separate this case from the general one in hopes
 *  of someday optimizing iterations over this case.  We will probably
 *  always have to iterate over the general form, unless we can show that
 *  the expression is monotonic and the inclusion condition has no side
 *  effects.  Here we only need worry about the inclusion condition.
 *  For the moment we don't try to look for side effects.
\*/

static void check_sem_genset_noexp(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type condition_ptr;            /* inclusion condition               */
ast_ptr_type ast_ptr;                  /* iterator pointer                  */
proctab_ptr_type old_proctab_ptr;      /* proctab item to be deleted        */
symtab_ptr_type symtab_ptr;            /* used to loop over bound variables */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* set and tuple formers are invalid in left hand sides */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* pick out child pointers, for readability */

   iter_list_ptr = root->ast_child.ast_child_ast;
   condition_ptr = iter_list_ptr->ast_next;

   /* check the iterator list, opening an iterator scope */

   check_sem(iter_list_ptr,check_type & CONST);
   if ((iter_list_ptr->ast_child.ast_child_ast)->ast_type != ast_in) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_bad_map_iter);

   }

   /* check the condition */

   check_sem(condition_ptr,RHS_COND);

   /* close the iterator scopes */

   for (ast_ptr = iter_list_ptr->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

      /* move bound variables to the current scope, and detach them */

      detach_symtab(iter_proctab_ptr->pr_symtab_head);

      for (symtab_ptr = iter_proctab_ptr->pr_symtab_head;
           symtab_ptr != NULL;
           symtab_ptr = symtab_ptr->st_thread) {

         symtab_ptr->st_owner_proc = curr_proctab_ptr;

      }

      if (iter_proctab_ptr->pr_symtab_head != NULL) {

         *(curr_proctab_ptr->pr_symtab_tail) =
            iter_proctab_ptr->pr_symtab_head;
         curr_proctab_ptr->pr_symtab_tail =
            iter_proctab_ptr->pr_symtab_tail;

      }

      old_proctab_ptr = iter_proctab_ptr;
      iter_proctab_ptr = iter_proctab_ptr->pr_parent;
      free_proctab(old_proctab_ptr);

   }

   return;

}


/*\
 *  \ast{ast\_arith}{arithmetic set formers}
 *     {\astnode{ast\_arith\_set}
 *        {\astnode{ast\_list}
 *           {\astnode{{\em element 1}}
 *              {\etcast}
 *              {\astnode{{\em element 2}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *           {\astnode{{\em last element}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\nullast}
 *     }
 *
 *  Arithmetic set formers may not appear on the left hand side.  They
 *  may appear on the right, provided all children are constants.
\*/

static void check_sem_arith(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type elem_ptr;                 /* pointer to set element            */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* set formers can not appear on the left hand side */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* check each child */

   for (elem_ptr = (root->ast_child.ast_child_ast)->ast_child.ast_child_ast;
        elem_ptr != NULL;
        elem_ptr = elem_ptr->ast_next) {

      check_sem(elem_ptr,(check_type | LHS | STMT) ^ (LHS | STMT));

   }
   check_sem((root->ast_child.ast_child_ast)->ast_next,
             (check_type | LHS | STMT) ^ (LHS | STMT));

   return;

}

/*\
 *  \ast{ast\_exists}{exists quantifier expressions}
 *     {\astnode{ast\_exists}
 *        {\astnode{{\em iter list}}
 *           {\etcast}
 *           {\astnode{{\em condition}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles exists expressions.  They may appear in
 *  conditions or in right hand sides only.
\*/

static void check_sem_exists(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type condition_ptr;            /* inclusion condition               */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* quantifier expressions are invalid in left hand sides */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* pick out child pointers, for readability */

   iter_list_ptr = root->ast_child.ast_child_ast;
   condition_ptr = iter_list_ptr->ast_next;

   /* check the iterator list */

   check_sem(iter_list_ptr,check_type & CONST);

   /* check the condition */

   check_sem(condition_ptr,RHS_COND);

   return;

}

/*\
 *  \ast{ast\_forall}{forall quantifier expressions}
 *     {\astnode{ast\_forall}
 *        {\astnode{{\em iter list}}
 *           {\etcast}
 *           {\astnode{{\em condition}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles forall expressions.  They may appear in
 *  conditions or in right hand sides only.
\*/

static void check_sem_forall(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type condition_ptr;            /* inclusion condition               */
ast_ptr_type ast_ptr;                  /* iterator pointer                  */
proctab_ptr_type old_proctab_ptr;      /* proctab item to be deleted        */
symtab_ptr_type symtab_ptr;            /* used to loop over bound variables */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* quantifier expressions are invalid in left hand sides */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* pick out child pointers, for readability */

   iter_list_ptr = root->ast_child.ast_child_ast;
   condition_ptr = iter_list_ptr->ast_next;

   /* check the iterator list, opening an iterator scope */

   check_sem(iter_list_ptr,check_type & CONST);

   /* check the condition */

   check_sem(condition_ptr,RHS_COND);

   /* close the iterator scopes */

   for (ast_ptr = iter_list_ptr->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

      /* move bound variables to the current scope, and detach them */

      detach_symtab(iter_proctab_ptr->pr_symtab_head);

      for (symtab_ptr = iter_proctab_ptr->pr_symtab_head;
           symtab_ptr != NULL;
           symtab_ptr = symtab_ptr->st_thread) {

         symtab_ptr->st_owner_proc = curr_proctab_ptr;

      }

      if (iter_proctab_ptr->pr_symtab_head != NULL) {

         *(curr_proctab_ptr->pr_symtab_tail) =
            iter_proctab_ptr->pr_symtab_head;
         curr_proctab_ptr->pr_symtab_tail =
            iter_proctab_ptr->pr_symtab_tail;

      }

      old_proctab_ptr = iter_proctab_ptr;
      iter_proctab_ptr = iter_proctab_ptr->pr_parent;
      free_proctab(old_proctab_ptr);

   }

   return;

}

/*\
 *  \ast{ast\_apply}{unary application operator}
 *     {\astnode{ast\_apply}
 *        {\astnode{{\em operator}}
 *           {\astnode{{\em source set}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *           {\nullast}
 *        }
 *        {\etcast}
 *     }
 *
 *  An application expression is allowed only in right hand side
 *  expressions.
\*/

static void check_sem_apply(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* applications can not be on the left */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* check the source set */

   check_sem((root->ast_child.ast_child_ast)->ast_child.ast_child_ast,
             (check_type | LHS | STMT) ^ (LHS | STMT));

   return;

}

/*\
 *  \ast{ast\_binapply}{binary application operator}
 *     {\astnode{ast\_binapply}
 *        {\astnode{{\em operator}}
 *           {\astnode{{\em first item}}
 *              {\etcast}
 *              {\astnode{{\em source set}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *           {\nullast}
 *        }
 *        {\etcast}
 *     }
 *
 *  An application expression is allowed only in right hand side
 *  expressions.
\*/

static void check_sem_binapply(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* applications can not be on the left */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   /* check the source items */

   check_sem((root->ast_child.ast_child_ast)->ast_child.ast_child_ast,
             (check_type | LHS | STMT) ^ (LHS | STMT));
   check_sem(
      (root->ast_child.ast_child_ast)->ast_child.ast_child_ast->ast_next,
      (check_type | LHS | STMT) ^ (LHS | STMT));

   return;

}

/*\
 *  \ast{ast\_iter\_list}{iterator lists}
 *     {\astnode{ast\_list}
 *        {\astnode{{\em first child}}
 *           {\etcast}
 *           {\astnode{{\em second child}}
 *              {\etcast}
 *              {\etcast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles lists of iterators.  First we traverse the list
 *  checking any right hand side contexts.  Then we open a scope for the
 *  bound variables, and traverse the list again declaring those bound
 *  variables.  Notice that we are unbalanced here --- we open a scope
 *  for bound variables but do not close it.  It is the responsibility of
 *  the caller to close that scope.
\*/

static void check_sem_iter_list(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type ast_ptr;                  /* used to loop over iterators       */
ast_ptr_type left,right;               /* left and right children of        */
                                       /* iterator                          */
proctab_ptr_type new_proc;             /* allocated proctab item            */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* traverse the list of iterators */

   for (ast_ptr = root->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

      /* first check the RHS */

      switch (ast_ptr->ast_type) {

         case ast_in :

            right = (ast_ptr->ast_child.ast_child_ast)->ast_next;
            check_sem(right,RHS_VAL | (check_type & CONST));

            break;

         case ast_eq :

            right = (ast_ptr->ast_child.ast_child_ast)->ast_next;

            if (right->ast_type != ast_of &&
                right->ast_type != ast_ofa) {

               error_message(SETL_SYSTEM
                             &(ast_ptr->ast_file_pos),
                             msg_bad_iterator);

            }
            else {

               check_sem(right->ast_child.ast_child_ast,
                         RHS_VAL | (check_type & CONST));

            }

            break;

         default :

            error_message(SETL_SYSTEM
                          &(ast_ptr->ast_file_pos),
                          msg_bad_iterator);

      }

      /* open up a new scope, by pushing a procedure */

      new_proc = get_proctab(SETL_SYSTEM_VOID);
      new_proc->pr_parent = iter_proctab_ptr;
      iter_proctab_ptr = new_proc;

      /* now handle the bound variables */

      switch (ast_ptr->ast_type) {

         case ast_in :

            left = ast_ptr->ast_child.ast_child_ast;
            check_sem(left,LHS_BV);

            break;

         case ast_eq :

            left = ast_ptr->ast_child.ast_child_ast;
            right = left->ast_next;

            if (right->ast_type == ast_of ||
                right->ast_type == ast_ofa) {

               check_sem(left,LHS_BV);
               right = (right->ast_child.ast_child_ast)->ast_next;
               right = right->ast_child.ast_child_ast;
               check_sem(right,LHS_BV);

            }

            break;

      }
   }

   return;

}

/*\
 *  \ast{ast\_ex\_iter}{exists iterator lists}
 *     {\astnode{ast\_list}
 *        {\astnode{{\em first child}}
 *           {\etcast}
 *           {\astnode{{\em second child}}
 *              {\etcast}
 *              {\etcast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles lists of iterators in an exists expression.  We
 *  separate it from other iterators since we do not make bound variables
 *  local to iterators in exists.
\*/

static void check_sem_ex_iter(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type ast_ptr;                  /* used to loop over iterators       */
ast_ptr_type left,right;               /* left and right children of        */
                                       /* iterator                          */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* traverse the list of iterators, checking each */

   for (ast_ptr = root->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

      /* an iterator expression must be 'x in S' or x = f(x) */

      switch (ast_ptr->ast_type) {

         case ast_in :

            left = ast_ptr->ast_child.ast_child_ast;
            check_sem(left,LHS_GEN);
            right = (ast_ptr->ast_child.ast_child_ast)->ast_next;
            check_sem(right,RHS_VAL | (check_type & CONST));

            break;

         case ast_eq :

            left = ast_ptr->ast_child.ast_child_ast;
            right = left->ast_next;

            if (right->ast_type != ast_of &&
                right->ast_type != ast_ofa) {

               error_message(SETL_SYSTEM
                             &(ast_ptr->ast_file_pos),
                             msg_bad_iterator);

            }
            else {
         
               check_sem(left,LHS_GEN);
               check_sem(right,LHS_GEN);
               check_sem(right->ast_child.ast_child_ast,
                         RHS_VAL | (check_type & CONST));

            }

            break;

         default :

            error_message(SETL_SYSTEM
                          &(ast_ptr->ast_file_pos),
                          msg_bad_iterator);

      }
   }

   return;

}

/*\
 *  \ast{ast\_if\_stmt}{if statements}
 *     {\astnode{ast\_if\_stmt}
 *        {\astnode{{\em condition}}
 *           {\etcast}
 *           {\astnode{{\em if clause}}
 *              {\etcast}
 *              {\astnode{{\em else clause}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  If statements are only valid in statement contexts.
\*/

static void check_sem_if_stmt(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* if's are invalid in constants or left hand sides */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & RHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_rhs);

   }

   /* check the children */

   check_sem(root->ast_child.ast_child_ast,RHS_COND | (check_type & CONST));
   check_sem((root->ast_child.ast_child_ast)->ast_next,STMT);
   check_sem(((root->ast_child.ast_child_ast)->ast_next)->ast_next,STMT);

   return;

}

/*\
 *  \ast{ast\_if\_expr}{if expressions}
 *     {\astnode{ast\_if\_expr}
 *        {\astnode{{\em condition}}
 *           {\etcast}
 *           {\astnode{{\em if clause}}
 *              {\etcast}
 *              {\astnode{{\em else clause}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  if expressions are pretty straighforward.  They are not valid in
 *  left hand sides or constant expressions, but right hand sides and
 *  statements are OK.
\*/

static void check_sem_if_expr(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* if's are invalid in constants or left hand sides */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   /* check the children */

   check_sem(root->ast_child.ast_child_ast,RHS_COND | (check_type & CONST));
   check_sem((root->ast_child.ast_child_ast)->ast_next,RHS);
   check_sem(((root->ast_child.ast_child_ast)->ast_next)->ast_next,RHS);

   return;

}

/*\
 *  \ast{ast\_while}{while expressions}
 *     {\astnode{ast\_while}
 *        {\astnode{{\em condition}}
 *           {\etcast}
 *           {\astnode{ast\_list}
 *              {\astnode{{\em first child}}
 *                 {\etcast}
 *                 {\astnode{{\em second child}}
 *                    {\etcast}
 *                    {\etcast}
 *                 }
 *              }
 *              {\etcast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  While expressions are pretty straighforward.  They are not valid in
 *  left hand sides or constant expressions, but right hand sides and
 *  statements are OK.
\*/

static void check_sem_while(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* while's are invalid in constants or left hand sides */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   /* check the children */

   loop_level++;
   check_sem(root->ast_child.ast_child_ast,RHS_COND | (check_type & CONST));
   check_sem((root->ast_child.ast_child_ast)->ast_next,STMT);
   loop_level--;

   return;

}

/*\
 *  \ast{ast\_loop}{loop expressions}
 *     {\astnode{ast\_while}
 *        {\astnode{ast\_list}
 *           {\astnode{{\em first child}}
 *              {\etcast}
 *              {\astnode{{\em second child}}
 *                 {\etcast}
 *                 {\etcast}
 *              }
 *           }
 *           {\etcast}
 *        }
 *        {\etcast}
 *     }
 *
 *  A loop expression is like {\tt while true loop}.
\*/

static void check_sem_loop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* loop's are invalid in constants or left hand sides */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   /* check the children */

   loop_level++;
   check_sem(root->ast_child.ast_child_ast,STMT);
   loop_level--;

   return;

}

/*\
 *  \ast{ast\_for}{for expressions}
 *     {\astnode{ast\_for}
 *        {\astnode{{\em iterator list}}
 *           {\etcast}
 *           {\astnode{ast\_list}
 *              {\astnode{{\em first child}}
 *                 {\etcast}
 *                 {\astnode{{\em second child}}
 *                    {\etcast}
 *                    {\etcast}
 *                 }
 *              }
 *              {\etcast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  For expressions require all the normal iterator work in addition to
 *  error checking.  They are not valid in left hand sides or constant
 *  expressions, but right hand sides and statements are OK.
\*/

static void check_sem_for(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type condition_ptr;            /* inclusion condition               */
ast_ptr_type stmt_list_ptr;            /* loop body list                    */
ast_ptr_type ast_ptr;                  /* iterator pointer                  */
proctab_ptr_type old_proctab_ptr;      /* proctab item to be deleted        */
symtab_ptr_type symtab_ptr;            /* used to loop over bound variables */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* for's are invalid in constants or left hand sides */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   /* pick out child pointers, for readability */

   iter_list_ptr = root->ast_child.ast_child_ast;
   condition_ptr = iter_list_ptr->ast_next;
   stmt_list_ptr = condition_ptr->ast_next;

   /* check the iterator list, opening iterator scopes */

   check_sem(iter_list_ptr,0);

   /* check the other children */

   check_sem(condition_ptr,RHS_COND);
   loop_level++;
   check_sem(stmt_list_ptr, STMT);
   loop_level--;

   /* close the iterator scopes */

   for (ast_ptr = iter_list_ptr->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

      /* move bound variables to the current scope, and detach them */

      detach_symtab(iter_proctab_ptr->pr_symtab_head);

      for (symtab_ptr = iter_proctab_ptr->pr_symtab_head;
           symtab_ptr != NULL;
           symtab_ptr = symtab_ptr->st_thread) {

         symtab_ptr->st_owner_proc = curr_proctab_ptr;

      }

      if (iter_proctab_ptr->pr_symtab_head != NULL) {

         *(curr_proctab_ptr->pr_symtab_tail) =
            iter_proctab_ptr->pr_symtab_head;
         curr_proctab_ptr->pr_symtab_tail =
            iter_proctab_ptr->pr_symtab_tail;

      }

      old_proctab_ptr = iter_proctab_ptr;
      iter_proctab_ptr = iter_proctab_ptr->pr_parent;
      free_proctab(old_proctab_ptr);

   }

   return;

}


/*\
 *  \ast{ast\_case}{case expressions}
 *     {\astnode{ast\_case}
 *        {\astnode{{\em discriminant}}
 *           {\etcast}
 *           {\astnode{{\em when list}}
 *              {\astnode{{\em when}}
 *                 {\astnode{{\em value list}}
 *                    {\astnode{{\em value}}
 *                       {\etcast}
 *                       {\astnode{{\em value}}
 *                          {\etcast}
 *                          {\etcast}
 *                       }
 *                    }
 *                    {\astnode{{\em statement list}}
 *                       {\etcast}
 *                       {\nullast}
 *                    }
 *                 }
 *                 {\astnode{{\em when}}
 *                    {\etcast}
 *                    {\etcast}
 *                 }
 *              }
 *              {\astnode{{\em statement list}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  Case expressions are permitted on the right hand side and in
 *  statements.
\*/

static void check_sem_case_stmt(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* case expressions can not be on the left */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & RHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_rhs);

   }

   check_sem(root->ast_child.ast_child_ast,RHS_VAL);
   check_sem((root->ast_child.ast_child_ast)->ast_next,STMT);
   check_sem(((root->ast_child.ast_child_ast)->ast_next)->ast_next,STMT);

   return;

}

/*\
 *  \ast{ast\_case}{case expressions}
 *     {\astnode{ast\_case}
 *        {\astnode{{\em discriminant}}
 *           {\etcast}
 *           {\astnode{{\em when list}}
 *              {\astnode{{\em when}}
 *                 {\astnode{{\em value list}}
 *                    {\astnode{{\em value}}
 *                       {\etcast}
 *                       {\astnode{{\em value}}
 *                          {\etcast}
 *                          {\etcast}
 *                       }
 *                    }
 *                    {\astnode{{\em statement list}}
 *                       {\etcast}
 *                       {\nullast}
 *                    }
 *                 }
 *                 {\astnode{{\em when}}
 *                    {\etcast}
 *                    {\etcast}
 *                 }
 *              }
 *              {\astnode{{\em statement list}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  Case expressions are permitted on the right hand side and in
 *  statements.
\*/

static void check_sem_case_expr(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* case expressions can not be on the left */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   check_sem(root->ast_child.ast_child_ast,RHS_VAL);
   check_sem((root->ast_child.ast_child_ast)->ast_next,RHS);
   check_sem(((root->ast_child.ast_child_ast)->ast_next)->ast_next,RHS);

   return;

}

/*\
 *  \ast{ast\_guard}{guard expressions}
 *     {\astnode{ast\_guard}
 *        {\astnode{{\em when list}}
 *           {\astnode{{\em when}}
 *              {\astnode{{\em value list}}
 *                 {\astnode{{\em value}}
 *                    {\etcast}
 *                    {\astnode{{\em value}}
 *                       {\etcast}
 *                       {\etcast}
 *                    }
 *                 }
 *                 {\astnode{{\em statement list}}
 *                    {\etcast}
 *                    {\nullast}
 *                 }
 *              }
 *              {\astnode{{\em when}}
 *                 {\etcast}
 *                 {\etcast}
 *              }
 *           }
 *           {\astnode{{\em statement list}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  Guard expressions are permitted on the right hand side and in
 *  statements.
\*/

static void check_sem_guard_stmt(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & RHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_rhs);

   }

   check_sem(root->ast_child.ast_child_ast,STMT);
   check_sem((root->ast_child.ast_child_ast)->ast_next,STMT);

   return;

}

/*\
 *  \ast{ast\_guard}{guard expressions}
 *     {\astnode{ast\_guard}
 *        {\astnode{{\em when list}}
 *           {\astnode{{\em when}}
 *              {\astnode{{\em value list}}
 *                 {\astnode{{\em value}}
 *                    {\etcast}
 *                    {\astnode{{\em value}}
 *                       {\etcast}
 *                       {\etcast}
 *                    }
 *                 }
 *                 {\astnode{{\em statement list}}
 *                    {\etcast}
 *                    {\nullast}
 *                 }
 *              }
 *              {\astnode{{\em when}}
 *                 {\etcast}
 *                 {\etcast}
 *              }
 *           }
 *           {\astnode{{\em statement list}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  Guard expressions are permitted on the right hand side and in
 *  statements.
\*/

static void check_sem_guard_expr(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_rhs_as_statement);

   }

   check_sem(root->ast_child.ast_child_ast,RHS);
   check_sem((root->ast_child.ast_child_ast)->ast_next,RHS);

   return;

}

/*\
 *  \ast{ast\_when}{when clause of case or guard}
 *     {\astnode{ast\_when}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{ast\_list}
 *              {\astnode{{\em argument 1}}
 *                 {\etcast}
 *                 {\astnode{{\em argument 2}}
 *                    {\etcast}
 *                    {\etcast}
 *                 }
 *              }
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  When clauses are guarded by their corresponding case nodes, so we
 *  don't need to do much checking here.
\*/

static void check_sem_when(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   check_sem(root->ast_child.ast_child_ast,RHS_VAL);
   check_sem((root->ast_child.ast_child_ast)->ast_next,check_type);

   return;

}

/*\
 *  \ast{ast\_return}{return expressions}
 *     {\astnode{ast\_return}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\etcast}
 *        }
 *        {\etcast}
 *     }
 *
 *  Our grammar allows return statements to appear in right hand side
 *  expressions, and even in constants, so we must check for those things
 *  here.
\*/

static void check_sem_return(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* return can not appear in constant expressions */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* return doesn't produce a value */

   if (check_type & RHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_return_as_rhs);

   }

   /* we can only return from procedures */

   if (curr_proctab_ptr->pr_type != pr_procedure &&
       curr_proctab_ptr->pr_type != pr_method) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_return_from_prog);

   }

   /* we must return a valid right hand side */

   if (root->ast_child.ast_child_ast != NULL)
      check_sem(root->ast_child.ast_child_ast,RHS_VAL);

   return;

}

/*\
 *  \astnotree{ast\_stop}{stop expressions}
 *
 *  Stops are only valid as statements.
\*/

static void check_sem_stop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (!(check_type & STMT)) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_stop_as_rhs);

   }

   return;

}

/*\
 *  \ast{ast\_exit}{exit expressions}
 *     {\astnode{ast\_exit}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\etcast}
 *        }
 *        {\etcast}
 *     }
 *
 *  Our grammar allows exit statements to appear in right hand side
 *  expressions, and even in constants, so we must check for those things
 *  here.
\*/

static void check_sem_exit(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* exit can not appear in constant expressions */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* exit doesn't produce a value */

   if (check_type & RHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_return_as_rhs);

   }

   /* we can only exit from loops */

   if (!loop_level) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_bad_exit);

   }

   /* we must return a valid right hand side */

   if (root->ast_child.ast_child_ast != NULL)
      check_sem(root->ast_child.ast_child_ast,RHS_VAL);

   return;

}

/*\
 *  \astnotree{ast\_continue}{return expressions}
 *
 *  Our grammar allows continue statements to appear in right hand side
 *  expressions, and even in constants, so we must check for those things
 *  here.
\*/

static void check_sem_continue(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* continue can not appear in constant expressions */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* continue doesn't produce a value */

   if (check_type & RHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_return_as_rhs);

   }

   /* we can only continue from loops */

   if (!loop_level) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_bad_continue);

   }

   return;

}

/*\
 *  \ast{ast\_assert}{assert statements}
 *     {\astnode{ast\_assert}
 *        {\astnode{{\em condition}}
 *           {\etcast}
 *           {\etcast}
 *        }
 *        {\etcast}
 *     }
 *
 *  Assert statements can only be statements.
\*/

static void check_sem_assert(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* if's are invalid in constants or left hand sides */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   if (check_type & RHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_rhs);

   }

   /* check the children */

   check_sem(root->ast_child.ast_child_ast,RHS_COND | (check_type & CONST));

   return;

}

/*\
 *  \ast{ast\_of}{procedure calls, map and tuple references}
 *     {\astnode{ast\_of}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{ast\_list}
 *              {\astnode{{\em argument 1}}
 *                 {\etcast}
 *                 {\astnode{{\em argument 2}}
 *                    {\etcast}
 *                    {\etcast}
 *                 }
 *              }
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles procedure calls, map and tuple references.  If
 *  used on the left, the item being referenced must be an identifier and
 *  may not be constant.
 *
 *  If we find that the left hand is a procedure constant, we change the
 *  node type to call and check that the number of passed arguments is
 *  acceptable.
\*/

static void check_sem_method(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   /* we can't reference methods in constants */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* if we find a method as a right hand side, copy the environment */

   if (check_type & (RHS ^ RHS_CALL) &&
       (right_ptr->ast_child.ast_symtab_ptr)->st_type == sym_method) {

      check_sem(left_ptr,RHS_VAL);
      root->ast_type = ast_menviron;

      return;

   }

   /* check the left hand side */

   check_sem(left_ptr,(check_type | CONST) ^ CONST);

   return;

}

/*\
 *  \astnotree{{\em error}}{error node}
 *
 *  This function is invoked when we find an ast type which should not
 *  occur.
\*/

static void check_sem_error(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type discard_root;             /* eliminate compiler warning        */
int discard_check_type;                /* eliminate compiler warning        */

   discard_root = root;
   discard_check_type = check_type;

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

#ifdef TRAPS

   trap(__FILE__,__LINE__,msg_bad_ast_node,root->ast_type);

#endif

   return;

}

/*\
 *  \ast{ast\_slot}{slot or method value}
 *     {\astnode{ast\_slot}
 *        {\astnode{{\em object}}
 *           {\etcast}
 *           {\astnode{{\em slot name}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  Slot are valid anywhere maps are valid.
\*/

static void check_sem_slot(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* we can't reference slots in constants */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* we don't allow sinister assignments with bound variables */

   if (check_type & LHS_BV) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_bad_iter_lhs);

   }

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* if used on the left, the target must be a variable */

   if (check_type & LHS) {

      check_sem(left_ptr,LHS_MAP);

      return;

   }

   /* check the left hand side */

   check_sem(left_ptr,(check_type | CONST) ^ CONST);

   return;

}

/*\
 *  \astnotree{ast\_self}{self copy}
 *
 *  Self is a nullary operator, which just returns the current self.  We
 *  maintain value semantics here, so self is not a pointer, but the
 *  current value (a copy is always made).
\*/

static void check_sem_self(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* subtree to be checked             */
   int check_type)                     /* condition to be checked           */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"CHK : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* self is only valid in class bodies */

   if (unit_proctab_ptr->pr_type != pr_class_body &&
       unit_proctab_ptr->pr_type != pr_process_body) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    "Self is only allowed in class bodies");

   }

   /* self isn't a variable */

   if (check_type & LHS) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_lhs);

   }

   /* self isn't constant */

   if (check_type & CONST) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    msg_expected_const);

   }

   /* self isn't a statement */

   if (check_type & STMT) {

      error_message(SETL_SYSTEM
                    &(root->ast_file_pos),
                    "Self is not a statement");

   }

   return;

}

