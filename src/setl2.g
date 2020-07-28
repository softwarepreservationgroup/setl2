/*\
 *  %
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
 *  \package{\setl\ Grammar}
 *
 *  This file contains a grammar for \setl. Please note --- the purpose of
 *  this grammar is to generate LALR(1) parse tables automatically.  It
 *  is not intended as a description of \setl. Although it is possible to
 *  give an LALR(1) grammar for \setl, it is much simpler to give a grammar
 *  which accepts a superset of \setl, and use checks in the semantic
 *  actions to enforce a few syntax rules.  Major areas in which we accept
 *  a superset of \setl\ are:
 *
 *  \begin{enumerate}
 *  \item
 *  {\bf Enumerated and arithmetic tuples.}
 *  We are allowed to use enumerated tuples in left hand side contexts,
 *  and may have place-holders on the left.  Although we could enforce
 *  these rules in the grammar, it would require many rules and have
 *  implications in seemingly unrelated parts of the grammar.  Instead,
 *  we accept placeholders and general expressions on both the left and
 *  right hand sides, and expect the semantic actions to detect errors.
 *  Unfortunately, this also affects arithmetic tuple formers, in that
 *  they must also accept placeholders.
 *
 *  \item
 *  {\bf Iterators.}
 *  Unfortunately, iterators (i.e. \verb"x IN S", \verb"x = f(x)", and
 *  \verb"x = f{x}") are also valid expression terms.  This creates a
 *  conflict between enumerated set and tuple formers and general set and
 *  tuple formers.  To solve this problem, we accept general expressions
 *  as iterators, and expect semantic actions to detect errors.
 *  \end{enumerate}
 *
 *  There are several other situations in which we accept a superset of
 *  the language, but the superset is less drastic.  We have noted those
 *  areas in comments with the rules.
 *
 *  Another `trick' we use to simplify the grammar is to require the
 *  lexical analyzer to merge some operators for us.  \setl\ allows
 *  assignment operators, like C, and also has application operators.
 *  Assignment operators are binary operators followed by \verb":=" and
 *  application operators are binary operators followed by \verb"/".  We
 *  expect the lexical analyzer to look for these sequences and merge
 *  them into a single token.
\*/

/* output files */

%nonterm_desc_file = "ntdesc.h"
%rule_desc_file    = "ruledesc.h"
%action_file       = "semact.c"
%parse_table_file  = "parsetab.c"

%terminals

/* ## begin terminals */
tok_id             -> "identifier"     /* identifier                        */
tok_literal        -> "literal"        /* literal                           */
tok_and            -> "AND"            /* keyword => AND                    */
tok_assert         -> "ASSERT"         /* keyword => ASSERT                 */
tok_body           -> "BODY"           /* keyword => BODY                   */
tok_case           -> "CASE"           /* keyword => CASE                   */
tok_class          -> "CLASS"          /* keyword => CLASS                  */
tok_const          -> "CONST"          /* keyword => CONST                  */
tok_continue       -> "CONTINUE"       /* keyword => CONTINUE               */
tok_else           -> "ELSE"           /* keyword => ELSE                   */
tok_elseif         -> "ELSEIF"         /* keyword => ELSEIF                 */
tok_end            -> "END"            /* keyword => END                    */
tok_exit           -> "EXIT"           /* keyword => EXIT                   */
tok_for            -> "FOR"            /* keyword => FOR                    */
tok_if             -> "IF"             /* keyword => IF                     */
tok_inherit        -> "INHERIT"        /* keyword => INHERIT                */
tok_lambda         -> "LAMBDA"         /* keyword => LAMBDA                 */
tok_loop           -> "LOOP"           /* keyword => LOOP                   */
tok_not            -> "NOT"            /* keyword => NOT                    */
tok_null           -> "NULL"           /* keyword => NULL                   */
tok_or             -> "OR"             /* keyword => OR                     */
tok_otherwise      -> "OTHERWISE"      /* keyword => OTHERWISE              */
tok_package        -> "PACKAGE"        /* keyword => PACKAGE                */
tok_procedure      -> "PROCEDURE"      /* keyword => PROCEDURE              */
tok_process        -> "PROCESS"        /* keyword => PROCESS                */
tok_program        -> "PROGRAM"        /* keyword => PROGRAM                */
tok_rd             -> "RD"             /* keyword => RD                     */
tok_return         -> "RETURN"         /* keyword => RETURN                 */
tok_rw             -> "RW"             /* keyword => RW                     */
tok_sel            -> "SEL"            /* keyword => SEL                    */
tok_self           -> "SELF"           /* keyword => SELF                   */
tok_stop           -> "STOP"           /* keyword => STOP                   */
tok_then           -> "THEN"           /* keyword => THEN                   */
tok_until          -> "UNTIL"          /* keyword => UNTIL                  */
tok_use            -> "USE"            /* keyword => USE                    */
tok_var            -> "VAR"            /* keyword => VAR                    */
tok_when           -> "WHEN"           /* keyword => WHEN                   */
tok_while          -> "WHILE"          /* keyword => WHILE                  */
tok_wr             -> "WR"             /* keyword => WR                     */
tok_semi           -> ";"              /* ;                                 */
tok_comma          -> ","              /* ,                                 */
tok_colon          -> ":"              /* :                                 */
tok_lparen         -> "("              /* (                                 */
tok_rparen         -> ")"              /* )                                 */
tok_lbracket       -> "["              /* [                                 */
tok_rbracket       -> "]"              /* ]                                 */
tok_lbrace         -> "{"              /* {                                 */
tok_rbrace         -> "}"              /* }                                 */
tok_dot            -> "."              /* .                                 */
tok_dotdot         -> ".."             /* ..                                */
tok_assign         -> ":="             /* :=                                */
tok_suchthat       -> "suchthat"       /* |                                 */
tok_rarrow         -> "=>"             /* =>                                */
tok_assignop       -> "assignop"       /* assignment operator               */
tok_applyop        -> "applyop"        /* assignment operator               */
tok_unop           -> "unop"           /* unary operator                    */
tok_caret          -> "^"              /* pointer reference                 */
tok_addop          -> "addop"          /* addop                             */
tok_dash           -> "-"              /* -                                 */
tok_mulop          -> "mulop"          /* mulop                             */
tok_expon          -> "**"             /* **                                */
tok_relop          -> "relop"          /* relop                             */
tok_fromop         -> "fromop"         /* fromop                            */
tok_quantifier     -> "quantifier"     /* quantifier                        */
tok_native         -> "NATIVE"         /* keyword => NATIVE                 */
/* ## end terminals */

%rules

/.
/*\
 *  %
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
 *  \package{Semantic Actions}
 *
 *  %
 *  % If this message is at the top of the file, the file you are looking
 *  % at was generated by LALR from a grammar file. DO NOT CHANGE THIS
 *  % FILE!!  If you wish to change a semantic action, change the grammar
 *  % file and re-run LALR.  View this file as machine-generated.
 *  %
 *
 *  We are somewhat less ambitious in the semantic actions of this
 *  compiler than we would be in a similar package of another compiler.
 *  Because \setl\ allows forward references, it is extremely difficult to
 *  match the symbol table with the abstract syntax tree on the first
 *  pass.  We would have to do a lot of backpatching, after all
 *  declarations had been processed.  We suspect the backpatching would
 *  be less efficient, and would certainly be more complex to program,
 *  than simply adding another pass to match the symbol table and
 *  abstract syntax tree.
 *
 *  We have chosen the simpler alternative, and defer matching the symbol
 *  table and abstract syntax tree to another pass.  The semantic actions
 *  therefore have two distinct functions:
 *
 *  \begin{itemize}
 *  \item
 *  {\bf Declararion processing.}
 *  We process declarations and make entries in the symbol table, even
 *  though we do not refer to those declarations yet.
 *  \item
 *  {\bf Construction of an abstract syntax tree.}
 *  We build an abstract syntax tree, but every time we come across an
 *  identifier we build a name table node, {\em not} a symbol table
 *  node.  We make no attempt to determine exactly to which symbol a name
 *  refers.
 *  \end{itemize}
 *
 *  The semantic actions were completely hand coded in an ad hoc manner.
 *  To aid the reader in following the construction of an abstract syntax
 *  tree, we give a diagram of the semantic stack with every action which
 *  refers to items on the stack.
\*/

#if MPWC
#pragma segment semact
#endif

#ifdef PLUGIN
#define fprintf plugin_fprintf
#endif

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "builtins.h"                  /* built-in symbols                  */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "ast.h"                       /* abstract syntax tree              */
#include "import.h"                    /* imported packages and classes     */
#include "lex.h"                       /* lexical analyzer                  */
#include "semact.h"                    /* semantic actions                  */
#include "listing.h"                   /* source and error listings         */
#include "chartab.h"                   /* character type table              */

/* constants */

#define YES 1                          /* true constant                     */
#define NO 0                           /* false constant                    */

/* performance tuning constants */

#define SEM_BLOCK_SIZE      50         /* semantic stack block size         */

/* semantic stack */

#define SEM_AST      0                 /* stack item is ast                 */
#define SEM_NAMTAB   1                 /* stack item is name                */

struct sem_stack_item {
   int sem_type;                       /* type of stack entry               */
   file_pos_type sem_file_pos;         /* file position of name             */
   namtab_ptr_type sem_namtab_ptr;     /* name table pointer                */
   ast_ptr_type sem_ast_ptr;           /* ast pointer                       */
   ast_ptr_type *sem_ast_tail;         /* tail of ast list                  */
   int sem_token_subclass;             /* subclass of pushed token          */
};

/* macro to allocate a new semantic stack item */

#ifdef TSAFE
#define get_sem ((sem_top < sem_max - 1) ? ++sem_top : alloc_sem(plugin_instance))
#else
#define get_sem ((sem_top < sem_max - 1) ? ++sem_top : alloc_sem())
#endif

/* global data */

static struct sem_stack_item *sem_stack = NULL;
                                       /* semantic stack                    */
static int sem_top = -1;               /*    stack top                      */
static int sem_max = 0;                /*    size of stack                  */
static ast_ptr_type ast_init_tree;     /* constant initialization tree      */
static ast_ptr_type *ast_init_tail;    /* next constant initialization node */
static ast_ptr_type var_init_tree;     /* variable initialization tree      */
static ast_ptr_type *var_init_tail;    /* next initialization node          */
static ast_ptr_type slot_init_tree;    /* slot initialization tree          */
static ast_ptr_type *slot_init_tail;   /* next initialization node          */

/* forward declarations */

static void build_ast(SETL_SYSTEM_PROTO int, char *, struct file_pos_item *);
                                       /* general AST builder               */
static void build_method(SETL_SYSTEM_PROTO int, char *);
                                       /* general method builder            */
static int alloc_sem(SETL_SYSTEM_PROTO_VOID);            
                                       /* allocate a semantic stack block   */
#ifdef TRAPS
static void verify_sem(int, int);      /* verify the type of a stack item   */
#endif
#ifdef DEBUG
static void print_sem(SETL_SYSTEM_PROTO_VOID); 
                                       /* print the semantic stack          */
#endif
#ifdef SHORT_FUNCS
static void semantic_action2(SETL_SYSTEM_PROTO int);   
                                       /* break up semantic actions         */
static void semantic_action3(SETL_SYSTEM_PROTO int);  
                                       /* break up semantic actions         */
#endif

/*\
 *  \function{semantic\_action()}
 *
 *  This function performs a single semantic action and returns.  It is
 *  organized as one big case statement, but the indentation is
 *  unconventional.  We think of this as a set of functions, each one
 *  dealing with a single production.  We indent it that way, rather than
 *  as cases.  Notice that we must be careful to include an explicit
 *  return statement in every case.
\*/

void semantic_action(
   SETL_SYSTEM_PROTO
   int rule)                           /* rule number                       */

{

   switch (rule) {

./

/*\
 *  \function{compilation units}
 *
 *  A source file consists of one or more compilation units.  We keep
 *  many tables for an entire source file in memory (although we keep
 *  intermediate files for the largest), so it would be nice if the user
 *  did not allow source files to get larger than necessary.  It is
 *  possible to have many compilation units in a file however, it just
 *  means that there is a higher risk of \verb"malloc()" failing.
\*/

   <compilation_unit_list>    ::= <compilation_unit_list> <compilation_unit>
                              |   <compilation_unit>

   <compilation_unit>         ::= <native_package_spec_unit>
                              |   <package_spec_unit>
                              |   <package_body_unit>
                              |   <class_spec_unit>
                              |   <class_body_unit>
                              |   <process_spec_unit>
                              |   <process_body_unit>
                              |   <program_unit>
                              |   %error

/*\
 *  \function{native package specifications}
 *
 *  A native package specification has no associated code, 
 *  only declarations.
\*/

   <native_package_spec_unit> ::= <native_package_spec_header>
                                  <no_body>
                                  <proc_declaration_part>
                                  <unit_tail>
/.
/*\
 *  \semact{native package specification}
 *
 *  When this rule is invoked, we've seen an entire package specification.
 *  There isn't much we have to do here, since we parse all compilation
 *  units before we generate code for any of them.  Here is what we have
 *  to do:
 *
 *  \begin{enumerate}
 *  \item
 *  If desired, we print the symbol table.  By the time we reach this
 *  rule, we should have already printed and purged the abstract syntax
 *  tree.
 *  \item
 *  We pop the procedure table.  At that point, we will be at the root of
 *  the procedure table, since we are at the compilation unit level.
 *  \item
 *  Since package specifications are compilation units, we update
 *  compilation unit statistics.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the package specification, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* update statistics, and set up for a new compilation unit */

   FILE_ERROR_COUNT += UNIT_ERROR_COUNT;
   FILE_WARNING_COUNT += UNIT_WARNING_COUNT;

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <native_package_spec_header>      ::= NATIVE PACKAGE identifier ;
/.
/*\
 *  \semact{native package specification header}
 *  \semcell{package name}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning a package
 *  specification header.  We use this as a trigger, to do a variety of
 *  things necessary to open a compilation unit.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the package name in the symbol table, as part of the new
 *  scope since we want it to be purged outside of the package.
 *  \item
 *  We set up an empty list for the initialization code tree and the code
 *  body tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* initialize the error counts */

   UNIT_ERROR_COUNT = 0;
   UNIT_WARNING_COUNT = 0;

   /* make sure package name is short enough */

   if (strlen(sem_stack[sem_top].sem_namtab_ptr->nt_name) > MAX_UNIT_NAME) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_unit_too_long,
                    sem_stack[sem_top].sem_namtab_ptr->nt_name,
                    MAX_UNIT_NAME);

      return;

   }

   /* open up a new procedure (in this case, a package) */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   new_proc->pr_type = pr_native_package;
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top].sem_file_pos));

   /* install the package specification name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      sem_stack[sem_top].sem_namtab_ptr,
                      new_proc,
                      &(sem_stack[sem_top].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      new_proc->pr_symtab_ptr->st_type = sym_package;
      new_proc->pr_symtab_ptr->st_aux.st_proctab_ptr = new_proc;

   }

   /* install an empty list as initialization code */

   build_ast(SETL_SYSTEM ast_list,"",NULL);
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   build_ast(SETL_SYSTEM ast_list,"",NULL);
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   var_init_tree = NULL;
   var_init_tail = &var_init_tree;

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;
   unit_proctab_ptr = new_proc;

   sem_top--;

   return;

}
./

   <native_package_spec_header>      ::= NATIVE PACKAGE %error ;


/*\
 *  \function{package specifications}
 *
 *  We make package specifications separate compilation units from their
 *  associated bodies, to allow cycles in the package dependency graph.
 *  A package specification has no associated code, only declarations.
\*/


   <package_spec_unit>        ::= <package_spec_header>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_declaration_part>
                                  <unit_tail>
/.
/*\
 *  \semact{package specification}
 *
 *  When this rule is invoked, we've seen an entire package specification.
 *  There isn't much we have to do here, since we parse all compilation
 *  units before we generate code for any of them.  Here is what we have
 *  to do:
 *
 *  \begin{enumerate}
 *  \item
 *  If desired, we print the symbol table.  By the time we reach this
 *  rule, we should have already printed and purged the abstract syntax
 *  tree.
 *  \item
 *  We pop the procedure table.  At that point, we will be at the root of
 *  the procedure table, since we are at the compilation unit level.
 *  \item
 *  Since package specifications are compilation units, we update
 *  compilation unit statistics.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the package specification, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* update statistics, and set up for a new compilation unit */

   FILE_ERROR_COUNT += UNIT_ERROR_COUNT;
   FILE_WARNING_COUNT += UNIT_WARNING_COUNT;

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <package_spec_header>      ::= PACKAGE identifier ;
/.
/*\
 *  \semact{package specification header}
 *  \semcell{package name}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning a package
 *  specification header.  We use this as a trigger, to do a variety of
 *  things necessary to open a compilation unit.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the package name in the symbol table, as part of the new
 *  scope since we want it to be purged outside of the package.
 *  \item
 *  We set up an empty list for the initialization code tree and the code
 *  body tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* initialize the error counts */

   UNIT_ERROR_COUNT = 0;
   UNIT_WARNING_COUNT = 0;

   /* make sure package name is short enough */

   if (strlen(sem_stack[sem_top].sem_namtab_ptr->nt_name) > MAX_UNIT_NAME) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_unit_too_long,
                    sem_stack[sem_top].sem_namtab_ptr->nt_name,
                    MAX_UNIT_NAME);

      return;

   }

   /* open up a new procedure (in this case, a package) */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   new_proc->pr_type = pr_package_spec;
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top].sem_file_pos));

   /* install the package specification name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      sem_stack[sem_top].sem_namtab_ptr,
                      new_proc,
                      &(sem_stack[sem_top].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      new_proc->pr_symtab_ptr->st_type = sym_package;
      new_proc->pr_symtab_ptr->st_aux.st_proctab_ptr = new_proc;

   }

   /* install an empty list as initialization code */

   build_ast(SETL_SYSTEM ast_list,"",NULL);
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   build_ast(SETL_SYSTEM ast_list,"",NULL);
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   var_init_tree = NULL;
   var_init_tail = &var_init_tree;

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;
   unit_proctab_ptr = new_proc;

   sem_top--;

   return;

}
./

   <package_spec_header>      ::= PACKAGE %error ;

/*\
 *  \function{package bodies}
 *
 *  A package body may contain local data, and must contain definitions
 *  of any procedures declared in the associated specification.
\*/

   <package_body_unit>        ::= <package_body_header>
                                  <use_part>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_definition_part>
                                  <unit_tail>
/.
/*\
 *  \semact{package body}
 *
 *  When this rule is invoked, we've seen an entire package body.  There
 *  isn't much we have to do here, since we parse all compilation units
 *  before we generate code for any of them.  Here is what we have to do:
 *
 *  \begin{enumerate}
 *  \item
 *  If desired, we print the symbol table.  By the time we reach this
 *  rule, we should have already printed and purged the abstract syntax
 *  tree.
 *  \item
 *  We pop the procedure table.  At that point, we will be at the root of
 *  the procedure table, since we are at the compilation unit level.
 *  \item
 *  Since package bodies are compilation units, we update compilation
 *  unit statistics.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the package body, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* update statistics, and set up for a new compilation unit */

   FILE_ERROR_COUNT += UNIT_ERROR_COUNT;
   FILE_WARNING_COUNT += UNIT_WARNING_COUNT;

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <package_body_header>      ::= PACKAGE BODY identifier ;
/.
/*\
 *  \semact{package body header}
 *  \semcell{package name}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning a package
 *  body header.  We use this as a trigger, to do a variety of things
 *  necessary to open a compilation unit.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the package name in the symbol table, as part of the new
 *  scope since we want it to be purged outside of the package.
 *  \item
 *  We set up an empty list for the initialization code tree and the code
 *  body tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* initialize the error counts */

   UNIT_ERROR_COUNT = 0;
   UNIT_WARNING_COUNT = 0;

   /* make sure package name is short enough */

   if (strlen(sem_stack[sem_top].sem_namtab_ptr->nt_name) > MAX_UNIT_NAME) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_unit_too_long,
                    sem_stack[sem_top].sem_namtab_ptr->nt_name,
                    MAX_UNIT_NAME);

      return;

   }

   /* open up a new procedure (in this case, a package) */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   new_proc->pr_type = pr_package_body;
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top].sem_file_pos));

   /* install the package specification name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      sem_stack[sem_top].sem_namtab_ptr,
                      new_proc,
                      &(sem_stack[sem_top].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      new_proc->pr_symtab_ptr->st_type = sym_package;
      new_proc->pr_symtab_ptr->st_aux.st_proctab_ptr = new_proc;

   }

   /* install an empty list as initialization code */

   build_ast(SETL_SYSTEM ast_list,"",NULL);
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   build_ast(SETL_SYSTEM ast_list,"",NULL);
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   var_init_tree = NULL;
   var_init_tail = &var_init_tree;

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;
   unit_proctab_ptr = new_proc;

   sem_top--;

   return;

}
./

   <package_body_header>      ::= PACKAGE BODY %error ;

/*\
 *  \function{class specifications}
 *
 *  We use the same syntax for class specifications and class bodies that
 *  we use for package specifications and package bodies, primarily for
 *  consistency.
\*/

   <class_spec_unit>          ::= <class_spec_header>
                                  <inherit_part>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_declaration_part>
                                  <unit_tail>
/.
/*\
 *  \semact{class specification}
 *
 *  When this rule is invoked, we've seen an entire class specification.
 *  There isn't much we have to do here, since we parse all compilation
 *  units before we generate code for any of them.  Here is what we have
 *  to do:
 *
 *  \begin{enumerate}
 *  \item
 *  If desired, we print the symbol table.  By the time we reach this
 *  rule, we should have already printed and purged the abstract syntax
 *  tree.
 *  \item
 *  We pop the procedure table.  At that point, we will be at the root of
 *  the procedure table, since we are at the compilation unit level.
 *  \item
 *  Since class specifications are compilation units, we update
 *  compilation unit statistics.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the class specification, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* update statistics, and set up for a new compilation unit */

   FILE_ERROR_COUNT += UNIT_ERROR_COUNT;
   FILE_WARNING_COUNT += UNIT_WARNING_COUNT;

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <class_spec_header>        ::= CLASS identifier ;
/.
/*\
 *  \semact{class specification header}
 *  \semcell{class name}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning a class
 *  specification header.  We use this as a trigger, to do a variety of
 *  things necessary to open a compilation unit.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the class name in the symbol table, as part of the new
 *  scope since we want it to be purged outside of the class.
 *  \item
 *  We set up an empty list for the initialization code tree and the code
 *  body tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* initialize the error counts */

   UNIT_ERROR_COUNT = 0;
   UNIT_WARNING_COUNT = 0;

   /* make sure class name is short enough */

   if (strlen(sem_stack[sem_top].sem_namtab_ptr->nt_name) > MAX_UNIT_NAME) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_unit_too_long,
                    sem_stack[sem_top].sem_namtab_ptr->nt_name,
                    MAX_UNIT_NAME);

      return;

   }

   /* open up a new procedure (in this case, a class) */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   new_proc->pr_type = pr_class_spec;
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top].sem_file_pos));

   /* install the class specification name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      sem_stack[sem_top].sem_namtab_ptr,
                      new_proc,
                      &(sem_stack[sem_top].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      new_proc->pr_symtab_ptr->st_type = sym_class;
      new_proc->pr_symtab_ptr->st_unit_num = 1;
      new_proc->pr_symtab_ptr->st_aux.st_proctab_ptr = new_proc;

   }

   /* install an empty list as initialization code */

   build_ast(SETL_SYSTEM ast_list,"",NULL);
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   build_ast(SETL_SYSTEM ast_list,"",NULL);
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   var_init_tree = NULL;
   var_init_tail = &var_init_tree;

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;
   unit_proctab_ptr = new_proc;

   sem_top--;

   return;

}
./

   <class_spec_header>        ::= CLASS %error ;

/*\
 *  \function{class bodies}
 *
 *  A class body may contain local data, and must contain definitions
 *  of any procedures declared in the associated specification.
\*/

   <class_body_unit>          ::= <class_body_header>
                                  <use_part>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_definition_part>
                                  <unit_tail>
/.
/*\
 *  \semact{class body}
 *
 *  When this rule is invoked, we've seen an entire class body.    There
 *  isn't much we have to do here, since we parse all compilation units
 *  before we generate code for any of them.  Here is what we have to do:
 *
 *  \begin{enumerate}
 *  \item
 *  If desired, we print the symbol table.  By the time we reach this
 *  rule, we should have already printed and purged the abstract syntax
 *  tree.
 *  \item
 *  We pop the procedure table.  At that point, we will be at the root of
 *  the procedure table, since we are at the compilation unit level.
 *  \item
 *  Since class bodies are compilation units, we update compilation
 *  unit statistics.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the class body, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* update statistics, and set up for a new compilation unit */

   FILE_ERROR_COUNT += UNIT_ERROR_COUNT;
   FILE_WARNING_COUNT += UNIT_WARNING_COUNT;

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <class_body_header>        ::= CLASS BODY identifier ;
/.
/*\
 *  \semact{class body header}
 *  \semcell{class name}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning a class
 *  body header.  We use this as a trigger, to do a variety of things
 *  necessary to open a compilation unit.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the class name in the symbol table, as part of the new
 *  scope since we want it to be purged outside of the class.
 *  \item
 *  We set up an empty list for the initialization code tree and the code
 *  body tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* initialize the error counts */

   UNIT_ERROR_COUNT = 0;
   UNIT_WARNING_COUNT = 0;

   /* make sure class name is short enough */

   if (strlen(sem_stack[sem_top].sem_namtab_ptr->nt_name) > MAX_UNIT_NAME) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_unit_too_long,
                    sem_stack[sem_top].sem_namtab_ptr->nt_name,
                    MAX_UNIT_NAME);

      return;

   }

   /* open up a new procedure (in this case, a class) */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   new_proc->pr_type = pr_class_body;
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top].sem_file_pos));

   /* install the class specification name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      sem_stack[sem_top].sem_namtab_ptr,
                      new_proc,
                      &(sem_stack[sem_top].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      new_proc->pr_symtab_ptr->st_type = sym_class;
      new_proc->pr_symtab_ptr->st_unit_num = 1;
      new_proc->pr_symtab_ptr->st_aux.st_proctab_ptr = new_proc;

   }

   /* install an empty list as initialization code */

   build_ast(SETL_SYSTEM ast_list,"",NULL);
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   build_ast(SETL_SYSTEM ast_list,"",NULL);
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   var_init_tree = NULL;
   var_init_tail = &var_init_tree;

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;
   unit_proctab_ptr = new_proc;

   sem_top--;

   return;

}
./

   <class_body_header>        ::= CLASS BODY %error ;

/*\
 *  \function{process specifications}
 *
 *  We use the same syntax for process specifications and process bodies that
 *  we use for package specifications and package bodies, primarily for
 *  consistency.
\*/

   <process_spec_unit>        ::= <process_spec_header>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_declaration_part>
                                  <unit_tail>
/.
/*\
 *  \semact{process specification}
 *
 *  When this rule is invoked, we've seen an entire process specification.
 *  There isn't much we have to do here, since we parse all compilation
 *  units before we generate code for any of them.  Here is what we have
 *  to do:
 *
 *  \begin{enumerate}
 *  \item
 *  If desired, we print the symbol table.  By the time we reach this
 *  rule, we should have already printed and purged the abstract syntax
 *  tree.
 *  \item
 *  We pop the procedure table.  At that point, we will be at the root of
 *  the procedure table, since we are at the compilation unit level.
 *  \item
 *  Since process specifications are compilation units, we update
 *  compilation unit statistics.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the process specification, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* update statistics, and set up for a new compilation unit */

   FILE_ERROR_COUNT += UNIT_ERROR_COUNT;
   FILE_WARNING_COUNT += UNIT_WARNING_COUNT;

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <process_spec_header>      ::= PROCESS CLASS identifier ;
/.
/*\
 *  \semact{process specification header}
 *  \semcell{process name}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning a process
 *  specification header.  We use this as a trigger, to do a variety of
 *  things necessary to open a compilation unit.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the process name in the symbol table, as part of the new
 *  scope since we want it to be purged outside of the process.
 *  \item
 *  We set up an empty list for the initialization code tree and the code
 *  body tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* initialize the error counts */

   UNIT_ERROR_COUNT = 0;
   UNIT_WARNING_COUNT = 0;

   /* make sure process name is short enough */

   if (strlen(sem_stack[sem_top].sem_namtab_ptr->nt_name) > MAX_UNIT_NAME) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_unit_too_long,
                    sem_stack[sem_top].sem_namtab_ptr->nt_name,
                    MAX_UNIT_NAME);

      return;

   }

   /* open up a new procedure (in this case, a process) */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   new_proc->pr_type = pr_process_spec;
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top].sem_file_pos));

   /* install the process specification name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      sem_stack[sem_top].sem_namtab_ptr,
                      new_proc,
                      &(sem_stack[sem_top].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      new_proc->pr_symtab_ptr->st_type = sym_process;
      new_proc->pr_symtab_ptr->st_unit_num = 1;
      new_proc->pr_symtab_ptr->st_aux.st_proctab_ptr = new_proc;

   }

   /* install an empty list as initialization code */

   build_ast(SETL_SYSTEM ast_list,"",NULL);
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   build_ast(SETL_SYSTEM ast_list,"",NULL);
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   var_init_tree = NULL;
   var_init_tail = &var_init_tree;

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;
   unit_proctab_ptr = new_proc;

   sem_top--;

   return;

}
./

   <process_spec_header>      ::= PROCESS CLASS %error ;

/*\
 *  \function{process bodies}
 *
 *  A process body may contain local data, and must contain definitions
 *  of any procedures declared in the associated specification.
\*/

   <process_body_unit>        ::= <process_body_header>
                                  <use_part>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_definition_part>
                                  <unit_tail>
/.
/*\
 *  \semact{process body}
 *
 *  When this rule is invoked, we've seen an entire process body.  There
 *  isn't much we have to do here, since we parse all compilation units
 *  before we generate code for any of them.  Here is what we have to do:
 *
 *  \begin{enumerate}
 *  \item
 *  If desired, we print the symbol table.  By the time we reach this
 *  rule, we should have already printed and purged the abstract syntax
 *  tree.
 *  \item
 *  We pop the procedure table.  At that point, we will be at the root of
 *  the procedure table, since we are at the compilation unit level.
 *  \item
 *  Since process bodies are compilation units, we update compilation
 *  unit statistics.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the process body, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* update statistics, and set up for a new compilation unit */

   FILE_ERROR_COUNT += UNIT_ERROR_COUNT;
   FILE_WARNING_COUNT += UNIT_WARNING_COUNT;

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <process_body_header>      ::= PROCESS CLASS BODY identifier ;
/.
/*\
 *  \semact{process body header}
 *  \semcell{process name}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning a process
 *  body header.  We use this as a trigger, to do a variety of things
 *  necessary to open a compilation unit.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the process name in the symbol table, as part of the new
 *  scope since we want it to be purged outside of the process.
 *  \item
 *  We set up an empty list for the initialization code tree and the code
 *  body tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* initialize the error counts */

   UNIT_ERROR_COUNT = 0;
   UNIT_WARNING_COUNT = 0;

   /* make sure process name is short enough */

   if (strlen(sem_stack[sem_top].sem_namtab_ptr->nt_name) > MAX_UNIT_NAME) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_unit_too_long,
                    sem_stack[sem_top].sem_namtab_ptr->nt_name,
                    MAX_UNIT_NAME);

      return;

   }

   /* open up a new procedure (in this case, a process) */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   new_proc->pr_type = pr_process_body;
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top].sem_file_pos));

   /* install the process specification name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      sem_stack[sem_top].sem_namtab_ptr,
                      new_proc,
                      &(sem_stack[sem_top].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      new_proc->pr_symtab_ptr->st_type = sym_process;
      new_proc->pr_symtab_ptr->st_unit_num = 1;
      new_proc->pr_symtab_ptr->st_aux.st_proctab_ptr = new_proc;

   }

   /* install an empty list as initialization code */

   build_ast(SETL_SYSTEM ast_list,"",NULL);
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   build_ast(SETL_SYSTEM ast_list,"",NULL);
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   var_init_tree = NULL;
   var_init_tail = &var_init_tree;

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;
   unit_proctab_ptr = new_proc;

   sem_top--;

   return;

}
./

   <process_body_header>      ::= PROCESS CLASS BODY %error ;

/*\
 *  \function{programs}
 *
 *  We expect most compilation units to be programs.  These are the
 *  only units which can be directly executed by the interpreter.
\*/

   <program_unit>             ::= <program_header>
                                  <use_part>
                                  <data_declaration_part>
                                  <body>
                                  <proc_definition_part>
                                  <unit_tail>
/.
/*\
 *  \semact{program}
 *
 *  When this rule is invoked, we've seen an entire program.  There isn't
 *  much we have to do here, since we parse all compilation units before
 *  we generate code for any of them.  Here is what we have to do:
 *
 *  \begin{enumerate}
 *  \item
 *  If desired, we print the symbol table.  By the time we reach this
 *  rule, we should have already printed and purged the abstract syntax
 *  tree.
 *  \item
 *  We pop the procedure table.  At that point, we will be at the root of
 *  the procedure table, since we are at the compilation unit level.
 *  \item
 *  Since programs are compilation units, we update compilation
 *  unit statistics.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the program, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* update statistics, and set up for a new compilation unit */

   FILE_ERROR_COUNT += UNIT_ERROR_COUNT;
   FILE_WARNING_COUNT += UNIT_WARNING_COUNT;

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <program_header>           ::= PROGRAM identifier ;
/.
/*\
 *  \semact{program header}
 *  \semcell{program name}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning a program
 *  header.  We use this as a trigger, to do a variety of things
 *  necessary to open a compilation unit.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the program name in the symbol table, as part of the new
 *  scope since we want it to be purged outside of the class.
 *  \item
 *  We set up an empty list for the initialization code tree and the code
 *  body tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* initialize the error counts */

   UNIT_ERROR_COUNT = 0;
   UNIT_WARNING_COUNT = 0;

   /* make sure program name is short enough */

   if (strlen(sem_stack[sem_top].sem_namtab_ptr->nt_name) > MAX_UNIT_NAME) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_unit_too_long,
                    sem_stack[sem_top].sem_namtab_ptr->nt_name,
                    MAX_UNIT_NAME);

      return;

   }

   /* open up a new procedure (in this case, a program) */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   new_proc->pr_type = pr_program;
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top].sem_file_pos));

   /* install the program name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      sem_stack[sem_top].sem_namtab_ptr,
                      new_proc,
                      &(sem_stack[sem_top].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      new_proc->pr_symtab_ptr->st_type = sym_program;
      new_proc->pr_symtab_ptr->st_aux.st_proctab_ptr = new_proc;

   }

   /* install an empty list as initialization code */

   build_ast(SETL_SYSTEM ast_list,"",NULL);
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   build_ast(SETL_SYSTEM ast_list,"",NULL);
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   var_init_tree = NULL;
   var_init_tail = &var_init_tree;

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;
   unit_proctab_ptr = new_proc;

   sem_top--;

   return;

}
./

   <program_header>           ::= PROGRAM %error ;

/*\
 *  \function{imported package list}
 *
 *  We use a \verb"USE" clause to import other packages.   We don't need
 *  an analog of Ada's \verb"WITH" clause, since we don't have generics.
\*/

   <use_part>                 ::= <use_clause> <use_part>
                              |   %empty

   <use_clause>               ::= USE <use_list> ;

   <use_list>                 ::= <use_list> , <use_item>
                              |   <use_item>

   <use_item>                 ::= identifier
/.
/*\
 *  \semact{imported packages}
 *  \semcell{package name}
 *  \sembottom
 *
 *  This rule adds a package name to the list of imported packages.  We
 *  don't actually look up the packages in the library yet -- there is no
 *  need.  Since we are not trying to match the symbol table with the
 *  abstract syntax tree yet we can defer loading imported packages until
 *  we do so.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
import_ptr_type *import_ptr;           /* used to loop over import list     */
symtab_ptr_type symtab_ptr;            /* imported package symbol           */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* search for the package in the current imported item list */

   for (import_ptr = &(curr_proctab_ptr->pr_import_list);
        *import_ptr != NULL &&
           (*import_ptr)->im_namtab_ptr != sem_stack[sem_top].sem_namtab_ptr;
        import_ptr = &((*import_ptr)->im_next));

   /* if we found it, error */

   if (*import_ptr != NULL) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_dup_use,
                    sem_stack[sem_top].sem_namtab_ptr->nt_name);

      return;

   }
   else {

      /* otherwise add it to the list */

      *import_ptr = get_import(SETL_SYSTEM_VOID);
      (*import_ptr)->im_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;

      /* install the package name */

      symtab_ptr = enter_symbol(SETL_SYSTEM
                                sem_stack[sem_top].sem_namtab_ptr,
                                curr_proctab_ptr,
                                &(sem_stack[sem_top].sem_file_pos));

      if (symtab_ptr != NULL) {

         symtab_ptr->st_type = sym_use;
         symtab_ptr->st_aux.st_import_ptr = *import_ptr;
         (*import_ptr)->im_symtab_ptr = symtab_ptr;

      }
   }

   /* pop the name off of the semantic stack */

   sem_top--;

   return;

}
./

   <use_item>                 ::= %error

/*\
 *  \function{inherited class list}
 *
 *  We use an \verb"INHERIT" clause as in Eiffel, to inherit other
 *  classes.
\*/

   <inherit_part>             ::= <inherit_clause> <inherit_part>
                              |   %empty

   <inherit_clause>           ::= INHERIT <inherit_list> ;

   <inherit_list>             ::= <inherit_list> , <inherit_item>
                              |   <inherit_item>

   <inherit_item>             ::= identifier
/.
/*\
 *  \semact{inherited classes}
 *  \semcell{superclass name}
 *  \sembottom
 *
 *  This rule adds a superclass name to the list of inherited classes.  We
 *  don't actually look up the classes in the library yet -- there is no
 *  need.  Since we are not trying to match the symbol table with the
 *  abstract syntax tree yet we can defer loading inherited classes until
 *  we do so.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
import_ptr_type *import_ptr;           /* used to loop over import list     */
symtab_ptr_type symtab_ptr;            /* inherited class symbol            */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* search for the class in the current imported item list */

   for (import_ptr = &(curr_proctab_ptr->pr_inherit_list);
        *import_ptr != NULL &&
           (*import_ptr)->im_namtab_ptr != sem_stack[sem_top].sem_namtab_ptr;
        import_ptr = &((*import_ptr)->im_next));

   /* if we found it, error */

   if (*import_ptr != NULL) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    "Class %s is already inherited",
                    sem_stack[sem_top].sem_namtab_ptr->nt_name);

      return;

   }
   else {

      /* otherwise add it to the list */

      *import_ptr = get_import(SETL_SYSTEM_VOID);
      (*import_ptr)->im_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
      (*import_ptr)->im_inherited = YES;

      /* install the class name */

      symtab_ptr = enter_symbol(SETL_SYSTEM
                                sem_stack[sem_top].sem_namtab_ptr,
                                curr_proctab_ptr,
                                &(sem_stack[sem_top].sem_file_pos));

      if (symtab_ptr != NULL) {

         symtab_ptr->st_type = sym_inherit;
         symtab_ptr->st_aux.st_import_ptr = *import_ptr;
         (*import_ptr)->im_symtab_ptr = symtab_ptr;

      }

      /* append a call to the class initialization function */

      build_ast(SETL_SYSTEM ast_namtab,"",&(sem_stack[sem_top].sem_file_pos));
      sem_stack[sem_top].sem_ast_ptr->ast_child.ast_namtab_ptr =
         sem_stack[sem_top - 1].sem_namtab_ptr;
      build_ast(SETL_SYSTEM ast_namtab,"",&(sem_stack[sem_top].sem_file_pos));
      sem_stack[sem_top].sem_ast_ptr->ast_child.ast_namtab_ptr =
         method_name[m_initobj];
      build_ast(SETL_SYSTEM ast_dot,"11",&(sem_stack[sem_top].sem_file_pos));
      build_ast(SETL_SYSTEM ast_list,"",&(sem_stack[sem_top].sem_file_pos));
      build_ast(SETL_SYSTEM ast_of,"11",&(sem_stack[sem_top].sem_file_pos));
      *slot_init_tail = sem_stack[sem_top].sem_ast_ptr;
      slot_init_tail = &((*slot_init_tail)->ast_next);

   }

   /* pop the name off of the semantic stack */

   sem_top--;

   return;

}
./

   <inherit_item>             ::= %error

/*\
 *  \function{procedures}
 *
 *  Procedure syntax is a different from packages, since we use them in
 *  two contexts.  There are procedure headers in package and class
 *  specifications as well as preceeding the actual procedure
 *  definitions.  Within specifications we have no declarations or
 *  bodies.
\*/

   <proc_declaration_part>    ::= <proc_declaration_list>
                              |   %empty

   <proc_declaration_list>    ::= <proc_declaration_list> <procedure_header> ;
                              |   <procedure_header> ;

   <proc_definition_part>     ::= <proc_definition_list>
                              |   %empty

   <proc_definition_list>     ::= <proc_definition_list> <procedure_unit>
                              |   <procedure_unit>

   <procedure_unit>           ::= <procedure_header> ;
                                  <data_declaration_part>
                                  <body>
                                  <proc_definition_part>
                                  <unit_tail>
/.
/*\
 *  \semact{procedure}
 *
 *  When this rule is invoked, we've seen an entire procedure.  All we
 *  have to do is print the symbol table and pop the procedure table.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the procedure, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <procedure_header>         ::= <procedure_name> <parameter_part>
/.
/*\
 *  \semact{procedure header}
 *
 *  Procedure headers are quite different from compilation unit headers.
 *  For one thing, they might have lists of formal parameters.  The more
 *  significant difference is that when used in a package specification,
 *  there will be no following procedure code.  For that reason,
 *  procedure headers are split up into several parts, so that we can
 *  perform actions at various points in header.  At this point, we've
 *  seen an entire procedure header.  If we are processing a package
 *  specification, this is also the end of the procedure.  We perform the
 *  following {\em iff} the enclosing unit is a package specification.
 *
 *  \begin{enumerate}
 *  \item
 *  If desired, we print the symbol table.  By the time we reach this
 *  rule, we should have already printed and purged the abstract syntax
 *  tree.
 *  \item
 *  We pop the procedure table.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /*
    *  if we are processing a package specification, we've finished a
    *  procedure
    */

   if (unit_proctab_ptr->pr_type != pr_package_spec &&
       unit_proctab_ptr->pr_type != pr_class_spec &&
       unit_proctab_ptr->pr_type != pr_native_package &&
       unit_proctab_ptr->pr_type != pr_process_spec)
      return;

#ifdef DEBUG

   /* print the symbols in the procedure, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <procedure_name>           ::= PROCEDURE identifier
/.
/*\
 *  \semact{procedure name}
 *  \semcell{procedure name}
 *  \semcell{procedure keyword}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning the name
 *  portion of a procedure header.  At this point, we want to open up a
 *  new scope, since formal parameter names are internal to a procedure.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the procedure name in the symbol table, as part of the
 *  enclosing procedure.
 *  \item
 *  We set up an empty list for the initialization code tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* open up a new procedure */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;

   if (unit_proctab_ptr->pr_type == pr_class_spec ||
       unit_proctab_ptr->pr_type == pr_class_body ||
       unit_proctab_ptr->pr_type == pr_process_spec ||
       unit_proctab_ptr->pr_type == pr_process_body) {

      new_proc->pr_type = pr_method;
      new_proc->pr_method_code =
         sem_stack[sem_top].sem_namtab_ptr->nt_method_code;

   }
   else {
      new_proc->pr_type = pr_procedure;
   }
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top - 1].sem_file_pos));

   /* install the procedure name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      sem_stack[sem_top].sem_namtab_ptr,
                      curr_proctab_ptr,
                      &(sem_stack[sem_top - 1].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      if (new_proc->pr_type == pr_method) {
         (new_proc->pr_symtab_ptr)->st_type = sym_method;
         (new_proc->pr_symtab_ptr)->st_slot_num =
            new_proc->pr_method_code;
         (new_proc->pr_symtab_ptr)->st_class = unit_proctab_ptr;
      }
      else {
         (new_proc->pr_symtab_ptr)->st_type = sym_procedure;
         (new_proc->pr_symtab_ptr)->st_slot_num = -1;
      }
      (new_proc->pr_symtab_ptr)->st_aux.st_proctab_ptr = new_proc;
      (new_proc->pr_symtab_ptr)->st_has_rvalue = YES;
      (new_proc->pr_symtab_ptr)->st_is_initialized = YES;
      (new_proc->pr_symtab_ptr)->st_needs_stored = YES;
      (new_proc->pr_symtab_ptr)->st_is_declared = YES;
      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_native_package || 
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_process_spec)
         (new_proc->pr_symtab_ptr)->st_is_public = YES;

   }

   /* install an empty list as initialization code */

   if (unit_proctab_ptr->pr_type != pr_package_spec &&
       unit_proctab_ptr->pr_type != pr_native_package &&
       unit_proctab_ptr->pr_type != pr_class_spec &&
       unit_proctab_ptr->pr_type != pr_process_spec) {

      build_ast(SETL_SYSTEM ast_list,"",NULL);
      ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
      sem_top--;
      ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
      build_ast(SETL_SYSTEM ast_list,"",NULL);
      slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
      sem_top--;
      slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
      var_init_tree = NULL;
      var_init_tail = &var_init_tree;

   }

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;
   sem_top -= 2;

   return;

}
./

   <procedure_name>           ::= PROCEDURE %error

   <parameter_part>           ::= ( <parameter_list> )
                              |   ( )
                              |   %empty
                              |   %error

   <parameter_list>           ::= <parameter_list> , <parameter_spec>
                              |   <parameter_spec>

   <parameter_spec>           ::= identifier
/.
/*\
 *  \semact{default formal parameter declaration}
 *  \semcell{formal name}
 *  \sembottom
 *
 *  A declaration of a formal parameter is almost identical to a variable
 *  declaration.  The formal paramters of a procedure are just the first
 *  {\em n} symbols in the procedure's local symbol table, where {\em n}
 *  is the number of formals.  All we have to do here is declare a
 *  variable, and bump the count of formal parameters for a procedure.
 *
 *  By default, a formal parameter is read only.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* formal parameter pointer          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top].sem_file_pos));

   if (symtab_ptr != NULL) {

      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;
      symtab_ptr->st_is_rparam = YES;
      symtab_ptr->st_needs_stored = YES;
      symtab_ptr->st_is_declared = YES;

   }

   curr_proctab_ptr->pr_formal_count++;
   sem_top--;

   return;

}
./

   <parameter_spec>           ::= RD identifier
/.
/*\
 *  \semact{read-only formal parameter declaration}
 *  \semcell{formal name}
 *  \sembottom
 *
 *  A declaration of a formal parameter is almost identical to a variable
 *  declaration.  The formal paramters of a procedure are just the first
 *  {\em n} symbols in the procedure's local symbol table, where {\em n}
 *  is the number of formals.  All we have to do here is declare a
 *  variable, and bump the count of formal parameters for a procedure.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* formal parameter pointer          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top].sem_file_pos));

   if (symtab_ptr != NULL) {

      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;
      symtab_ptr->st_is_rparam = YES;
      symtab_ptr->st_needs_stored = YES;
      symtab_ptr->st_is_declared = YES;

   }

   curr_proctab_ptr->pr_formal_count++;
   sem_top--;

   return;

}
./

   <parameter_spec>           ::= WR identifier
/.
/*\
 *  \semact{write-only formal parameter declaration}
 *  \semcell{formal name}
 *  \sembottom
 *
 *  A declaration of a formal parameter is almost identical to a variable
 *  declaration.  The formal paramters of a procedure are just the first
 *  {\em n} symbols in the procedure's local symbol table, where {\em n}
 *  is the number of formals.  All we have to do here is declare a
 *  variable, and bump the count of formal parameters for a procedure.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* formal parameter pointer          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* we don't allow write parameters on methods */

   if (unit_proctab_ptr->pr_type == pr_class_spec ||
       unit_proctab_ptr->pr_type == pr_class_body ||
       unit_proctab_ptr->pr_type == pr_process_spec ||
       unit_proctab_ptr->pr_type == pr_process_body) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    "Write parameters are not allowed in methods => %s",
                    sem_stack[sem_top].sem_namtab_ptr->nt_name);

      return;

   }

   /* enter and flag the symbol */

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top].sem_file_pos));

   if (symtab_ptr != NULL) {

      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;
      symtab_ptr->st_is_wparam = YES;
      symtab_ptr->st_needs_stored = YES;
      symtab_ptr->st_is_declared = YES;

   }

   curr_proctab_ptr->pr_formal_count++;
   curr_proctab_ptr->pr_symtab_ptr->st_has_rvalue = NO;
   sem_top--;

   return;

}
./

   <parameter_spec>           ::= RW identifier
/.
/*\
 *  \semact{read-write formal parameter declaration}
 *  \semcell{formal name}
 *  \sembottom
 *
 *  A declaration of a formal parameter is almost identical to a variable
 *  declaration.  The formal paramters of a procedure are just the first
 *  {\em n} symbols in the procedure's local symbol table, where {\em n}
 *  is the number of formals.  All we have to do here is declare a
 *  variable, and bump the count of formal parameters for a procedure.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* formal parameter pointer          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* we don't allow write parameters on methods */

   if (unit_proctab_ptr->pr_type == pr_class_spec ||
       unit_proctab_ptr->pr_type == pr_class_body ||
       unit_proctab_ptr->pr_type == pr_process_spec ||
       unit_proctab_ptr->pr_type == pr_process_body) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    "Write parameters are not allowed in methods => %s",
                    sem_stack[sem_top].sem_namtab_ptr->nt_name);

      return;

   }

   /* enter and flag the symbol */

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top].sem_file_pos));

   if (symtab_ptr != NULL) {

      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;
      symtab_ptr->st_is_rparam = YES;
      symtab_ptr->st_is_wparam = YES;
      symtab_ptr->st_needs_stored = YES;
      symtab_ptr->st_is_declared = YES;

   }

   curr_proctab_ptr->pr_formal_count++;
   curr_proctab_ptr->pr_symtab_ptr->st_has_rvalue = NO;
   sem_top--;

   return;

}
./

   <parameter_spec>           ::= %error

/*\
 *  \function{methods}
 *
 *  Method headers are a bit strange.  Essentially, these methods are
 *  hooks into the \setl\ interpreter, corresponding to various syntactic
 *  constructs.  We use the syntactic construct itself as the method
 *  name, with identifiers in the operand positions as formal parameters.
\*/

   <procedure_header>         ::= <method_header>
/.
/*\
 *  \semact{method headers}
 *
 *  This rule shields the following ones, making sure that method headers
 *  are only used within a class body.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */


   if (((curr_proctab_ptr->pr_parent)->pr_type != pr_class_body)&&
       (((curr_proctab_ptr->pr_parent)->pr_type!= pr_package_body)||
	((strncmp(((curr_proctab_ptr->pr_parent)->pr_namtab_ptr)->nt_name,"ERROR_EXTENSION",15)!=0)))) {

      error_message(SETL_SYSTEM NULL,"Operator methods are only valid in class bodies");

   }

   return;

}
./


   <method_header>            ::= PROCEDURE SELF addop identifier
/.
/*\
 *  \semact{method headers}
 *  \semcell{{\em varies}}
 *  \sembottom
 *
 *  \setl\ allows methods to override many types of its syntax.  The
 *  following rules allow the declaration of methods corresponding to
 *  those syntactic structures.  Rather than duplicate similar code a
 *  variety of places, we have a general function to handle all of these
 *  cases.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {

      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass],"0S01");

   return;

}
./

   <method_header>            ::= PROCEDURE identifier addop SELF
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {

      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass]+1,
                "010S");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF - identifier
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* build the method */

   build_method(SETL_SYSTEM
                m_sub,"0S01");

   return;

}
./

   <method_header>            ::= PROCEDURE identifier - SELF
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* build the method */

   build_method(SETL_SYSTEM
                m_sub_r,"010S");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF mulop identifier
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {

      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass],"0S01");

   return;

}
./

   <method_header>            ::= PROCEDURE identifier mulop SELF
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {

      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass]+1,
                "010S");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF ** identifier
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {

      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass],"0S01");

   return;

}
./

   <method_header>            ::= PROCEDURE identifier ** SELF
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {

      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass]+1,
                "010S");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF relop identifier
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {

      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass],"0S01");

   return;

}
./

   <method_header>            ::= PROCEDURE identifier relop SELF
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {


      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass]+1,
                "010S");

   return;

}
./

   <method_header>            ::= PROCEDURE fromop SELF
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {

      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass],"00S");

   return;

}
./

   <method_header>            ::= PROCEDURE unop SELF
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* make sure this operator has a method */

   if (tok_mcode[sem_stack[sem_top - 1].sem_token_subclass] < 0) {

      error_message(SETL_SYSTEM &sem_stack[sem_top - 1].sem_file_pos,
                    "No method for %s operator",
                    sem_stack[sem_top - 1].sem_namtab_ptr->nt_name);

      return;

   }

   /* build the method */

   build_method(SETL_SYSTEM
                tok_mcode[sem_stack[sem_top - 1].sem_token_subclass],"00S");

   return;

}
./

   <method_header>            ::= PROCEDURE - SELF
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* build the method */

   build_method(SETL_SYSTEM
                m_uminus,"00S");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF ( identifier )
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_method(SETL_SYSTEM
                m_of,"0S1");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF { identifier }
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_method(SETL_SYSTEM
                m_ofa,"0S1");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF ( identifier .. identifier )
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_method(SETL_SYSTEM
                m_slice,"0S11");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF ( identifier .. )
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_method(SETL_SYSTEM
                m_end,"0S1");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF ( identifier ) := identifier
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_method(SETL_SYSTEM
                m_sof,"0S101");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF { identifier } := identifier
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_method(SETL_SYSTEM
                m_sofa,"0S101");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF ( identifier .. identifier )
                                     := identifier
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_method(SETL_SYSTEM
                m_sslice,"0S1101");

   return;

}
./

   <method_header>            ::= PROCEDURE SELF ( identifier .. )
                                     := identifier
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_method(SETL_SYSTEM
                m_send,"0S101");

   return;

}
./

/*\
 *  \function{tails}
 *
 *  Unit tails may optionally include the unit name.
\*/

   <unit_tail>                ::= END <optional_unit_name> ;

   <optional_unit_name>       ::= identifier
/.
/*\
 *  \semact{procedure / program / package tail identifier}
 *  \semcell{unit name}
 *  \sembottom
 *
 *  When this action is invoked, we found the tail identifier.  We need
 *  to make sure the name matches the name given in the header.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   if (sem_stack[sem_top].sem_namtab_ptr->nt_name !=
       (curr_proctab_ptr->pr_namtab_ptr)->nt_name) {

      warning_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
          msg_bad_tail,
          sem_stack[sem_top].sem_namtab_ptr->nt_name,
          (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

   }

   sem_top--;

   return;

}
./

   <optional_unit_name>       ::= %error

   <optional_unit_name>       ::= %empty

/*\
 *  \function{data declarations}
 *
 *  We allow four forms of data declarations: variables, class variables,
 *  constants, and selectors.
\*/

   <data_declaration_part>    ::= <data_declaration_list>
                              |   %empty

   <data_declaration_list>    ::= <data_declaration_list> <data_declaration> ;
                              |   <data_declaration> ;

   <data_declaration>         ::= <const_clause>
                              |   <sel_clause>
                              |   <var_clause>
                              |   <class_var_clause>

/*\
 *  \function{constant declarations}
 *
 *  Constants are a little misleading.  They are constant in the sense
 *  that the programmer may not change their values in the code body, but
 *  are unusual in that constant values are formed at run time, during
 *  program load rather than by the compiler.  This is because constants
 *  may assume set or tuple values, and these objects are formed by the
 *  interpreter in dynamic memory, so it's much easier to defer
 *  evaluating constants until run time.
\*/

   <const_clause>             ::= CONST <const_declaration_list>

   <const_declaration_list>   ::= <const_declaration_list> ,
                                  <const_declaration>
                              |   <const_declaration>

   <const_declaration>        ::= identifier := <expression>
/.
/*\
 *  \semact{constant declaration}
 *  \semcell{expression}
 *  \semcell{assignment symbol}
 *  \semcell{identifier}
 *  \sembottom
 *
 *  This rule handles a constant declaration.  We enter the symbol into
 *  the symbol table, and append an assignment to the initialization
 *  tree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* constant's symbol table entry     */
ast_ptr_type const_ptr;                /* constant subtree                  */
ast_ptr_type assign_ptr;               /* constant initialization           */

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_NAMTAB);
   verify_sem(sem_top - 2,SEM_NAMTAB);

#endif

   /* first, we declare the symbol as a constant */

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top - 2].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top - 2].sem_file_pos));

   /* if we didn't have a duplicate declaration ... */

   if (symtab_ptr != NULL) {

      /* set up the constant's symbol table entry */

      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_rvalue = YES;

      /* we always store names in package specifications */

      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_native_package ||
          curr_proctab_ptr->pr_type == pr_class_body ||
          curr_proctab_ptr->pr_type == pr_process_body)
         symtab_ptr->st_needs_stored = YES;

      symtab_ptr->st_is_declared = YES;
      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_native_package ||
          curr_proctab_ptr->pr_type == pr_process_spec)
         symtab_ptr->st_is_public = YES;

      /* construct a symbol tree */

      const_ptr = get_ast(SETL_SYSTEM_VOID);
      const_ptr->ast_type = ast_symtab;
      const_ptr->ast_child.ast_symtab_ptr = symtab_ptr;
      const_ptr->ast_next = sem_stack[sem_top].sem_ast_ptr;
      copy_file_pos(&(const_ptr->ast_file_pos),
                    &(sem_stack[sem_top - 2].sem_file_pos));

      /* construct a constant assignment tree */

      assign_ptr = get_ast(SETL_SYSTEM_VOID);
      assign_ptr->ast_type = ast_cassign;
      assign_ptr->ast_child.ast_child_ast = const_ptr;
      copy_file_pos(&(assign_ptr->ast_file_pos),
                    &(sem_stack[sem_top - 2].sem_file_pos));

      /* append the constant assignment to the initialization code */

      *ast_init_tail = assign_ptr;
      ast_init_tail = &(assign_ptr->ast_next);

   }

   /* pop the name and expression off the stack */

   sem_top -= 3;

   return;

}
./

   <const_declaration>        ::= %error

/*\
 *  \function{selector declarations}
 *
 *  Selectors are unusual, and completely absent in SETL.  SETL provides
 *  macros, which are used to name components of tuples, among other
 *  things.  We do not provide macros in \setl, in the belief that they
 *  degrade readability rather than enhance it if used without
 *  restriction.  However, it is still nice to be able to name the
 *  components of a tuple.  Selectors are the mechanism we use to
 *  facilitate that.
\*/

   <sel_clause>               ::= SEL <sel_declaration_list>

   <sel_declaration_list>     ::= <sel_declaration_list> , <sel_declaration>
                              |   <sel_declaration>

   <sel_declaration>          ::= identifier ( literal )
/.
/*\
 *  \semact{selector declaration}
 *  \semcell{literal}
 *  \semcell{identifier}
 *  \sembottom
 *
 *  This rule handles a selector declaration.  We have merged all
 *  literals into a single token class, but we only accept integers as
 *  selectors, so we make sure the literal is an integer here.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type sel_ptr;               /* selector symbol table entry       */
symtab_ptr_type index_ptr;             /* index symbol table entry          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);
   verify_sem(sem_top - 1,SEM_NAMTAB);

#endif

   /* first we verify that the index is an integer */

   index_ptr = sem_stack[sem_top].sem_namtab_ptr->nt_symtab_ptr;
   if (index_ptr->st_type != sym_integer) {

      error_message(SETL_SYSTEM &sem_stack[sem_top].sem_file_pos,
                    msg_bad_selector);

      return;

   }

   /* we declare the symbol as a selector */

   sel_ptr = enter_symbol(SETL_SYSTEM
                          sem_stack[sem_top - 1].sem_namtab_ptr,
                          curr_proctab_ptr,
                          &(sem_stack[sem_top - 1].sem_file_pos));

   /* if we didn't have a duplicate declaration ... */

   if (sel_ptr != NULL) {

      /* set up the selector's symbol table entry */

      sel_ptr->st_type = sym_selector;
      sel_ptr->st_aux.st_selector_ptr = index_ptr;

      /* we always store names in package specifications */

      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_native_package ||
          curr_proctab_ptr->pr_type == pr_class_body ||
          curr_proctab_ptr->pr_type == pr_process_body)
         sel_ptr->st_needs_stored = YES;

      sel_ptr->st_is_declared = YES;
      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_native_package ||
          curr_proctab_ptr->pr_type == pr_process_spec)
         sel_ptr->st_is_public = YES;

   }

   /* pop the name and literal off the stack */

   sem_top -= 2;

   return;

}
./
/.
#ifdef SHORT_FUNCS

default :

   semantic_action2(SETL_SYSTEM rule);
   return;

}}

#if MPWC
#pragma segment semact1
#endif

static void semantic_action2(
   SETL_SYSTEM_PROTO
   int rule)                           /* rule number                       */

{

   switch (rule) {

#endif
./

   <sel_declaration>          ::= identifier ( %error )

   <sel_declaration>          ::= %error

/*\
 *  \function{variable declarations}
 *
 *  Variable declarations are identical to those in SETL, except that we
 *  allow initialization expressions, but not \verb"INIT" clauses.  We do
 *  not have type declarations, but the user can (and possibly must,
 *  depending on compiler option) declare variables.
\*/

   <var_clause>               ::= VAR <var_declaration_list>

   <var_declaration_list>     ::= <var_declaration_list> , <var_declaration>
                              |   <var_declaration>

   <var_declaration>          ::= identifier
/.
/*\
 *  \semact{variable declaration}
 *  \semcell{identifier}
 *  \sembottom
 *
 *  This rule handles a variable declaration.  We just enter the symbol
 *  into the symbol table.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* variable's symbol table entry     */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* first, we declare the symbol as a variable */

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top].sem_file_pos));

   /* if we didn't have a duplicate declaration ... */

   if (symtab_ptr != NULL) {

      /* set up the variable's symbol table entry */

      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;

      if (curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_class_body ||
          curr_proctab_ptr->pr_type == pr_process_spec ||
          curr_proctab_ptr->pr_type == pr_process_body) {

         symtab_ptr->st_type = sym_slot;
         symtab_ptr->st_slot_num = m_user;
         symtab_ptr->st_class = unit_proctab_ptr;

      }
      else {

         symtab_ptr->st_type = sym_id;

      }

      /* we always store names in package specifications */

      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_native_package ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_class_body ||
          curr_proctab_ptr->pr_type == pr_process_spec ||
          curr_proctab_ptr->pr_type == pr_process_body)
         symtab_ptr->st_needs_stored = YES;

      symtab_ptr->st_is_declared = YES;
      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_native_package ||
          curr_proctab_ptr->pr_type == pr_class_spec || 
          curr_proctab_ptr->pr_type == pr_process_spec)
         symtab_ptr->st_is_public = YES;

   }

   /* pop the name off the stack */

   sem_top--;

   return;

}
./

   <var_declaration>          ::= identifier := <expression>
/.
/*\
 *  \semact{variable declaration}
 *  \semcell{expression}
 *  \semcell{assignment symbol}
 *  \semcell{identifier}
 *  \sembottom
 *
 *  This rule handles a variable declaration.  We enter the symbol into
 *  the symbol table, and append an assignment to the code initialization
 *  tree.  Note that we are somewhat restrictive in the expressions we
 *  allow for initialization.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* variable's symbol table entry     */
ast_ptr_type var_ptr;                  /* variable subtree                  */
ast_ptr_type assign_ptr;               /* variable initialization           */

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_NAMTAB);
   verify_sem(sem_top - 2,SEM_NAMTAB);

#endif

   /* first, we declare the symbol as a variable */

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top - 2].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top - 2].sem_file_pos));

   /* if we didn't have a duplicate declaration ... */

   if (symtab_ptr != NULL) {

      /* set up the variable's symbol table entry */

      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;

      /* we always store names in package specifications */

      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_native_package ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_class_body ||
          curr_proctab_ptr->pr_type == pr_process_spec ||
          curr_proctab_ptr->pr_type == pr_process_body)
         symtab_ptr->st_needs_stored = YES;

      symtab_ptr->st_is_declared = YES;
      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_native_package ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_process_spec)
         symtab_ptr->st_is_public = YES;

      /* construct a symbol tree */

      var_ptr = get_ast(SETL_SYSTEM_VOID);
      var_ptr->ast_type = ast_symtab;
      var_ptr->ast_child.ast_symtab_ptr = symtab_ptr;
      var_ptr->ast_next = sem_stack[sem_top].sem_ast_ptr;
      copy_file_pos(&(var_ptr->ast_file_pos),
                    &(sem_stack[sem_top - 2].sem_file_pos));

      /* construct a variable assignment tree */

      assign_ptr = get_ast(SETL_SYSTEM_VOID);
      assign_ptr->ast_type = ast_assign;
      assign_ptr->ast_child.ast_child_ast = var_ptr;
      copy_file_pos(&(assign_ptr->ast_file_pos),
                    &(sem_stack[sem_top - 2].sem_file_pos));

      /* append the variable assignment to the initialization code */

      if (curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_class_body ||
          curr_proctab_ptr->pr_type == pr_process_spec ||
          curr_proctab_ptr->pr_type == pr_process_body) {

         symtab_ptr->st_type = sym_slot;
         symtab_ptr->st_slot_num = m_user;
         symtab_ptr->st_class = unit_proctab_ptr;
         *slot_init_tail = assign_ptr;
         slot_init_tail = &(assign_ptr->ast_next);

      }
      else {

         symtab_ptr->st_type = sym_id;
         *var_init_tail = assign_ptr;
         var_init_tail = &(assign_ptr->ast_next);

      }
   }

   /* pop the name and expression off the stack */

   sem_top -= 3;

   return;

}
./

   <var_declaration>          ::= %error

/*\
 *  \function{class variable declarations}
 *
 *  Class variables are similar to variables, but within classes they are
 *  common to all instances of a class.
\*/

   <class_var_clause>         ::= CLASS VAR <class_var_decl_list>

   <class_var_decl_list>      ::= <class_var_decl_list> , <class_var_decl>
                              |   <class_var_decl>

   <class_var_decl>           ::= identifier
/.
/*\
 *  \semact{variable declaration}
 *  \semcell{identifier}
 *  \sembottom
 *
 *  This rule handles a variable declaration.  We just enter the symbol
 *  into the symbol table.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* variable's symbol table entry     */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   /* first, we declare the symbol as a variable */

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top].sem_file_pos));

   /* if we didn't have a duplicate declaration ... */

   if (symtab_ptr != NULL) {

      /* set up the variable's symbol table entry */

      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;

      /* we always store names in package specifications */

      if (curr_proctab_ptr->pr_type == pr_package_spec || 
          curr_proctab_ptr->pr_type == pr_native_package ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_class_body ||
          curr_proctab_ptr->pr_type == pr_process_spec ||
          curr_proctab_ptr->pr_type == pr_process_body)
         symtab_ptr->st_needs_stored = YES;

      symtab_ptr->st_is_declared = YES;
      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_process_spec)
         symtab_ptr->st_is_public = YES;

   }

   /* pop the name off the stack */

   sem_top--;

   return;

}
./

   <class_var_decl>           ::= identifier := <expression>
/.
/*\
 *  \semact{variable declaration}
 *  \semcell{expression}
 *  \semcell{assignment symbol}
 *  \semcell{identifier}
 *  \sembottom
 *
 *  This rule handles a variable declaration.  We enter the symbol into
 *  the symbol table, and append an assignment to the code initialization
 *  tree.  Note that we are somewhat restrictive in the expressions we
 *  allow for initialization.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* variable's symbol table entry     */
ast_ptr_type var_ptr;                  /* variable subtree                  */
ast_ptr_type assign_ptr;               /* variable initialization           */

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_NAMTAB);
   verify_sem(sem_top - 2,SEM_NAMTAB);

#endif

   /* first, we declare the symbol as a variable */

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top - 2].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top - 2].sem_file_pos));

   /* if we didn't have a duplicate declaration ... */

   if (symtab_ptr != NULL) {

      /* set up the variable's symbol table entry */

      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_has_rvalue = YES;

      /* we always store names in package specifications */

      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_class_body ||
          curr_proctab_ptr->pr_type == pr_process_spec ||
          curr_proctab_ptr->pr_type == pr_process_body)
         symtab_ptr->st_needs_stored = YES;

      symtab_ptr->st_is_declared = YES;
      if (curr_proctab_ptr->pr_type == pr_package_spec ||
          curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_process_spec)
         symtab_ptr->st_is_public = YES;

      /* construct a symbol tree */

      var_ptr = get_ast(SETL_SYSTEM_VOID);
      var_ptr->ast_type = ast_symtab;
      var_ptr->ast_child.ast_symtab_ptr = symtab_ptr;
      var_ptr->ast_next = sem_stack[sem_top].sem_ast_ptr;
      copy_file_pos(&(var_ptr->ast_file_pos),
                    &(sem_stack[sem_top - 2].sem_file_pos));

      /* construct a variable assignment tree */

      assign_ptr = get_ast(SETL_SYSTEM_VOID);
      assign_ptr->ast_type = ast_assign;
      assign_ptr->ast_child.ast_child_ast = var_ptr;
      copy_file_pos(&(assign_ptr->ast_file_pos),
                    &(sem_stack[sem_top - 2].sem_file_pos));

      /* append the variable assignment to the initialization code */

      *var_init_tail = assign_ptr;
      var_init_tail = &(assign_ptr->ast_next);

   }

   /* pop the name and expression off the stack */

   sem_top -= 3;

   return;

}
./

   <class_var_decl>           ::= %error

/*\
 *  \function{statement and expression lists}
 *
 *  At various places in the grammar we collect lists of statements or
 *  expressions.  These rules just provide a mechanism for doing that.
\*/

   <no_body>                  ::= %empty
/.
/*\
 *  \semact{procedure / program / package bodies}
 *
 *  The following two statements correspond to missing bodies.  We create
 *  a null statement list, and drop through to the more general body
 *  case.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

./

   <body>                     ::= %empty
/.
/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* construct a dummy body */

   build_ast(SETL_SYSTEM ast_list,"",NULL);

}
./

   <body>                     ::= <statement_list>
/.
/*\
 *  \semact{procedure / program / package bodies}
 *  \semcell{statement list}
 *  \sembottom
 *
 *  When this action is invoked, we have gathered up the statements in a
 *  procedure body into a list.  All we have to do is print the list, if
 *  desired, and attach it to the procedure table node.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* initobj procedure                 */

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);

#endif

   /*
    *  If we are processing a procedure, splice in the variable
    *  initialization at the beginning of the body.  Otherwise, tack it
    *  on to the end of the constant initialization code.
    */

   if (curr_proctab_ptr->pr_type == pr_procedure ||
       curr_proctab_ptr->pr_type == pr_method) {

      if (var_init_tree != NULL) {

         *var_init_tail =
            (sem_stack[sem_top].sem_ast_ptr)->ast_child.ast_child_ast;
         (sem_stack[sem_top].sem_ast_ptr)->ast_child.ast_child_ast =
            var_init_tree;

      }
   }
   else {

      if (var_init_tree != NULL) {

         *ast_init_tail = var_init_tree;
         ast_init_tail = var_init_tail;

      }
   }

#ifdef DEBUG

   if (AST_DEBUG) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_ast(SETL_SYSTEM ast_init_tree,"Initialization Tree");
      fputs("\n",DEBUG_FILE);

      if (curr_proctab_ptr->pr_type == pr_class_spec ||
          curr_proctab_ptr->pr_type == pr_class_body ||
          curr_proctab_ptr->pr_type == pr_process_body ||
          curr_proctab_ptr->pr_type == pr_process_body) {

         print_ast(SETL_SYSTEM slot_init_tree,"Slot initialization Tree");
         fputs("\n",DEBUG_FILE);

      }

      print_ast(SETL_SYSTEM sem_stack[sem_top].sem_ast_ptr,"Body Tree");
      fputs("\n",DEBUG_FILE);

   }

#endif

   /*
    *  if we're processing a class body, we create an object
    *  initialization procedure
    */

   if (curr_proctab_ptr->pr_type == pr_class_body ||
       curr_proctab_ptr->pr_type == pr_process_body) {

      /* open up a new procedure */

      new_proc = get_proctab(SETL_SYSTEM_VOID);
      new_proc->pr_parent = curr_proctab_ptr;
      *(curr_proctab_ptr->pr_tail) = new_proc;
      curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
      new_proc->pr_namtab_ptr = method_name[m_initobj];
      new_proc->pr_type = pr_method;
      new_proc->pr_method_code = m_initobj;
      copy_file_pos(&(new_proc->pr_file_pos),
                    &(sem_stack[sem_top].sem_file_pos));

      /* install the procedure name */

      new_proc->pr_symtab_ptr = enter_symbol(SETL_SYSTEM
                                             method_name[m_initobj],
                                             curr_proctab_ptr,
                                             NULL);

      (new_proc->pr_symtab_ptr)->st_type = sym_method;
      (new_proc->pr_symtab_ptr)->st_slot_num = m_initobj;
      (new_proc->pr_symtab_ptr)->st_class = unit_proctab_ptr;
      (new_proc->pr_symtab_ptr)->st_is_public = YES;
      (new_proc->pr_symtab_ptr)->st_aux.st_proctab_ptr = new_proc;
      (new_proc->pr_symtab_ptr)->st_has_rvalue = YES;
      (new_proc->pr_symtab_ptr)->st_needs_stored = YES;
      (new_proc->pr_symtab_ptr)->st_is_declared = YES;

      build_ast(SETL_SYSTEM ast_list,"",NULL);
      store_ast(SETL_SYSTEM &(new_proc->pr_init_code),
                sem_stack[sem_top].sem_ast_ptr);
      sem_top--;

      build_ast(SETL_SYSTEM ast_list,"",NULL);
      store_ast(SETL_SYSTEM &(new_proc->pr_slot_code),
                sem_stack[sem_top].sem_ast_ptr);
      sem_top--;

      store_ast(SETL_SYSTEM &(new_proc->pr_body_code),
                slot_init_tree);
      build_ast(SETL_SYSTEM ast_list,"",NULL);
      slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
      sem_top--;
      slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);

   }

   /* save the AST's in the intermediate file */

   store_ast(SETL_SYSTEM &(curr_proctab_ptr->pr_init_code),
             ast_init_tree);
   store_ast(SETL_SYSTEM &(curr_proctab_ptr->pr_slot_code),
             slot_init_tree);
   store_ast(SETL_SYSTEM &(curr_proctab_ptr->pr_body_code),
             sem_stack[sem_top].sem_ast_ptr);

   sem_top--;

   return;

}
./

   <statement_list>           ::= <statement_list> <stmt_or_expression> ;
/.
/*\
 *  \semact{statement lists}
 *  \semcell{new statement}
 *  \semcell{statement list}
 *  \sembottom
 *
 *  When this action is invoked, we've just scanned a statement which
 *  must be appended to the current statement list. We use the tail
 *  pointer to update the current tail, then reset the tail pointer.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_AST);

#endif

   *(sem_stack[sem_top - 1].sem_ast_tail) =
      sem_stack[sem_top].sem_ast_ptr;
   sem_stack[sem_top - 1].sem_ast_tail =
      &(sem_stack[sem_top].sem_ast_ptr->ast_next);
   sem_top--;

   return;

}
./

   <statement_list>           ::= <stmt_or_expression> ;
/.
/*\
 *  \semact{first statement in list}
 *  \semcell{statement}
 *  \sembottom
 *
 *  When this action is invoked, we've just scanned the first statement
 *  in a statement list. We have to make it a child of a list node.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);

#endif

   build_ast(SETL_SYSTEM ast_list,"1",NULL);
   sem_stack[sem_top].sem_ast_tail =
      &((sem_stack[sem_top].sem_ast_ptr->ast_child.ast_child_ast)->ast_next);

   return;

}
./

   <stmt_or_expression>       ::= <statement>
                              |   <expression>

   <expression_list>          ::= <expression_list> , <expression>
/.
/*\
 *  \semact{expression lists}
 *  \semcell{new expression}
 *  \semcell{expression list}
 *  \sembottom
 *
 *  When this action is invoked, we've just scanned an expression which
 *  must be appended to the current expression list. We use the tail
 *  pointer to update the current tail, then reset the tail pointer.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_AST);

#endif

   *(sem_stack[sem_top - 1].sem_ast_tail) =
      sem_stack[sem_top].sem_ast_ptr;
   sem_stack[sem_top - 1].sem_ast_tail =
      &(sem_stack[sem_top].sem_ast_ptr->ast_next);
   sem_top--;

   return;

}
./

   <expression_list>          ::= <expression>
/.
/*\
 *  \semact{first expression in list}
 *  \semcell{expression}
 *  \sembottom
 *
 *  When this action is invoked, we've just scanned the first expression
 *  in an expression list. We have to make it a child of a list node.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);

#endif

   build_ast(SETL_SYSTEM ast_list,"1",NULL);
   sem_stack[sem_top].sem_ast_tail =
      &((sem_stack[sem_top].sem_ast_ptr->ast_child.ast_child_ast)->ast_next);

   return;

}
./

/*\
 *  \function{assignment expressions}
 *
 *  \setl\ is a little more of an expression language than SETL. We treat
 *  statements as expressions, and allow them to be used interchangeably.
 *  This is the first of several expressions which would more commonly be
 *  used as statements.
\*/

   <expression>               ::= <left_term> := <expression>
/.
/*\
 *  \semact{assignment expression}
 *  \semcell{right expression}
 *  \semcell{assignment operator}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  Generally, we would like to do some error checking here to insure
 *  that the left hand expression is indeed a valid left hand side.  We
 *  can not do that in \setl, however, without type declarations. All we do
 *  here is form an AST, and defer error checking to the next phase.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_assign,"101",&(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <expression>               ::= <left_term> assignop <expression>
/.
/*\
 *  \semact{assignment operators}
 *  \semcell{right expression}
 *  \semcell{assignment operator}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  To simplify the grammar, we expect the lexical analyzer to merge
 *  binary operators followed by an assignment symbol into a single
 *  token, which we classify as assignment operators.  We merge all of
 *  these into the token class \verb"tok_assignop", and distinguish them
 *  via the token subclass.
 *
 *  The action here is a little complicated.  First we use the assignment
 *  operator to look up the operation type.  Then we copy the left hand
 *  side into that slot, so the stack contains the right hand side
 *  followed by the left hand side twice.  We then assemble a tree for
 *  the right hand side, followed by the assignment.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "101",
             &(sem_stack[sem_top - 1].sem_file_pos));
   build_ast(SETL_SYSTEM ast_assignop,"1",NULL);

   return;

}
./

/*\
 *  \function{loop expressions}
 *
 *  We have abandoned SETL syntax for loops, since we find it somewhat
 *  archaic.  Loop syntax in \setl\ combines the iterators of SETL with more
 *  modern control structures, such as those in Ada.
\*/

   <primary>                  ::= FOR <for_iterator> LOOP
                                     <statement_list>
                                  END LOOP
/.
/*\
 *  \semact{for loop}
 *  \semcell{LOOP}
 *  \semcell{statement list}
 *  \semcell{LOOP}
 *  \semcell{condition}
 *  \semcell{iterator list}
 *  \semcell{FOR}
 *  \sembottom
 *
 *  For statements iterate over a list of sets, tuples, or strings
 *  executing a statement list for each set of values matching a
 *  condition.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   sem_stack[sem_top - 4].sem_ast_ptr->ast_type = ast_iter_list;
   build_ast(SETL_SYSTEM ast_for,"011010",&(sem_stack[sem_top - 5].sem_file_pos));

   return;

}
./

   <for_iterator>             ::= <expression_list>
/.
/*\
 *  \semact{for iterator}
 *  \semcell{iterator}
 *  \sembottom
 *
 *  If a for iterator does not have a selection condition we push a null
 *  on the stack.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_null,"",&(sem_stack[sem_top].sem_file_pos));

   return;

}
./

   <for_iterator>             ::= <expression_list> suchthat <expression>

   <primary>                  ::= WHILE <expression> LOOP
                                     <statement_list>
                                  END LOOP
/.
/*\
 *  \semact{while expression}
 *  \semcell{LOOP}
 *  \semcell{statement list}
 *  \semcell{LOOP}
 *  \semcell{condition}
 *  \semcell{WHILE}
 *  \sembottom
 *
 *  All we do with a while expression is form the AST subtree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_while,"01010",&(sem_stack[sem_top - 4].sem_file_pos));

   return;

}
./

   <primary>                  ::= UNTIL <expression> LOOP
                                     <statement_list>
                                  END LOOP
/.
/*\
 *  \semact{until expression}
 *  \semcell{LOOP}
 *  \semcell{statement list}
 *  \semcell{LOOP}
 *  \semcell{condition}
 *  \semcell{UNTIL}
 *  \sembottom
 *
 *  All we do with an until expression is form the AST subtree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_until,"01010",&(sem_stack[sem_top - 4].sem_file_pos));

   return;

}
./

   <primary>                  ::= LOOP
                                     <statement_list>
                                  END LOOP
/.
/*\
 *  \semact{loop expression}
 *  \semcell{LOOP}
 *  \semcell{statement list}
 *  \semcell{LOOP}
 *  \sembottom
 *
 *  All we do with a loop expression is form the AST subtree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_loop,"010",&(sem_stack[sem_top - 2].sem_file_pos));

   return;

}
./

/*\
 *  \function{if expressions}
 *
 *  Since most statements are also valid expressions, and vice versa, we
 *  have only one set of rules for if expressions.  They are fairly
 *  conventional.
\*/

   <primary>                  ::= IF <expression> THEN
                                     <statement_list>
                                     <opt_else_stmt_clause>
                                  END IF
/.
/*\
 *  \semact{if expressions}
 *  \semcell{else clause}
 *  \semcell{statement list}
 *  \semcell{condition}
 *  \semcell{IF}
 *  \sembottom
 *
 *  We have incorporated any \verb"ELSEIF" clauses into the \verb"ELSE"
 *  clause already, so all we do here is build the abstract syntax tree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_if_stmt,"01110",&(sem_stack[sem_top - 4].sem_file_pos));

   return;

}
./

   <opt_else_stmt_clause>     ::= ELSEIF <expression> THEN
                                     <statement_list>
                                     <opt_else_stmt_clause>
/.
/*\
 *  \semact{elseif clauses}
 *  \semcell{else clause}
 *  \semcell{statement list}
 *  \semcell{condition}
 *  \semcell{ELSEIF}
 *  \sembottom
 *
 *  We incorporate any \verb"ELSEIF" clauses into the \verb"ELSE" clause
 *  already, to remove \verb"ELSEIF" from the abstract syntax tree
 *  altogether.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_if_stmt,"0111",NULL);

   return;

}
./

   <opt_else_stmt_clause>     ::= ELSE <statement_list>

   <opt_else_stmt_clause>     ::= %empty
/.
/*\
 *  \semact{missing else clauses}
 *
 *  The higher-level rules count on an \verb"ELSE" clause being present,
 *  so we push a null on the stack if there is no \verb"ELSE" clause.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_null,"",NULL);

   return;

}
./

   <primary>                  ::= IF <expression> THEN
                                     <expression>
                                     <opt_else_expr_clause>
                                  END IF
/.
/*\
 *  \semact{if expressions}
 *  \semcell{else clause}
 *  \semcell{statement list}
 *  \semcell{condition}
 *  \semcell{IF}
 *  \sembottom
 *
 *  We have incorporated any \verb"ELSEIF" clauses into the \verb"ELSE"
 *  clause already, so all we do here is build the abstract syntax tree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_if_expr,"01110",&(sem_stack[sem_top - 4].sem_file_pos));

   return;

}
./

   <opt_else_expr_clause>     ::= ELSEIF <expression> THEN
                                     <expression>
                                     <opt_else_expr_clause>
/.
/*\
 *  \semact{elseif clauses}
 *  \semcell{else clause}
 *  \semcell{statement list}
 *  \semcell{condition}
 *  \semcell{ELSEIF}
 *  \sembottom
 *
 *  We incorporate any \verb"ELSEIF" clauses into the \verb"ELSE" clause
 *  already, to remove \verb"ELSEIF" from the abstract syntax tree
 *  altogether.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_if_expr,"0111",NULL);

   return;

}
./

   <opt_else_expr_clause>     ::= ELSE <expression>

   <opt_else_expr_clause>     ::= %empty
/.
/*\
 *  \semact{missing else clauses}
 *
 *  The higher-level rules count on an \verb"ELSE" clause being present,
 *  so we push a null on the stack if there is no \verb"ELSE" clause.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_null,"",NULL);

   return;

}
./

/*\
 *  \function{case and guarded expressions}
 *
 *  \setl\ has two forms of multi-way branches. The case expression is a
 *  standard case statement, with one exception: we do not insist that
 *  the expressions associated with each clause be constant.  The
 *  expressions are evaluated each time the case is executed.
 *
 *  The guarded statement chooses non-deterministically among the clauses
 *  until it finds one which is true.  Then it executes the associated
 *  body and exits.
\*/

   <primary>                  ::= CASE <expression>
                                     <case_when_stmt_list>
                                     <guard_case_stmt_default>
                                  END CASE
/.
/*\
 *  \semact{case expression}
 *  \semcell{CASE}
 *  \semcell{otherwise clause}
 *  \semcell{statement list}
 *  \semcell{choice value}
 *  \semcell{CASE}
 *  \sembottom
 *
 *  Case expressions are difficult for the code generator, but all we do
 *  here is build an abstract syntax tree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_case_stmt,"01110",NULL);

   return;

}
./

   <case_when_stmt_list>      ::= <case_when_stmt_list>
                                  <case_when_stmt_item>
/.
/*\
 *  \semact{case when clause list}
 *  \semcell{when clause}
 *  \semcell{when clause list}
 *  \sembottom
 *
 *  This action is identical to many other list-building actions in the
 *  semantic actions.  When this action is invoked, we've just scanned a
 *  clause which must be appended to the current clause list. We use the
 *  tail pointer to update the current tail, then reset the tail
 *  pointer.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_AST);

#endif

   *(sem_stack[sem_top - 1].sem_ast_tail) =
      sem_stack[sem_top].sem_ast_ptr;
   sem_stack[sem_top - 1].sem_ast_tail =
      &(sem_stack[sem_top].sem_ast_ptr->ast_next);
   sem_top--;

   return;

}
./

   <case_when_stmt_list>      ::= <case_when_stmt_item>
/.
/*\
 *  \semact{case when clause list}
 *  \semcell{when clause}
 *  \sembottom
 *
 *  At this point we've scanned the first clause in a when clause list.
 *  We begin a list, and set the tail pointer.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_list,"1",NULL);
   sem_stack[sem_top].sem_ast_tail =
      &((sem_stack[sem_top].sem_ast_ptr->ast_child.ast_child_ast)->ast_next);

   return;

}
./

   <case_when_stmt_item>      ::= WHEN <expression_list> =>
                                     <statement_list>
/.
/*\
 *  \semact{when clause}
 *  \semcell{statement list}
 *  \semcell{expression}
 *  \semcell{WHEN}
 *  \sembottom
 *
 *  We have to accumulate a list of \verb"WHEN" clauses in the process of
 *  building a \verb"case" or \verb"SELECT" subtree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_when,"011",NULL);

   return;

}
./

   <primary>                  ::= CASE
                                     <guard_when_stmt_list>
                                     <guard_case_stmt_default>
                                  END CASE
/.
/*\
 *  \semact{guard expression}
 *  \semcell{CASE}
 *  \semcell{otherwise clause}
 *  \semcell{statement list}
 *  \semcell{CASE}
 *  \sembottom
 *
 *  Select expressions are nearly identical to case expressions for the
 *  grammar and semantic actions.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_guard_stmt,"0110",NULL);

   return;

}
./

   <guard_when_stmt_list>     ::= <guard_when_stmt_list>
                                  <guard_when_stmt_item>
/.
/*\
 *  \semact{guard when lists}
 *  \semcell{when clause}
 *  \semcell{when clause list}
 *  \sembottom
 *
 *  This is yet another list-building action.  We need to separate it
 *  from case when lists to allow lists of expressions in case when
 *  clauses.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_AST);

#endif

   *(sem_stack[sem_top - 1].sem_ast_tail) =
      sem_stack[sem_top].sem_ast_ptr;
   sem_stack[sem_top - 1].sem_ast_tail =
      &(sem_stack[sem_top].sem_ast_ptr->ast_next);
   sem_top--;

   return;

}
./

   <guard_when_stmt_list>     ::= <guard_when_stmt_item>
/.
/*\
 *  \semact{first when clause in list}
 *  \semcell{clause}
 *  \sembottom
 *
 *  When this action is invoked, we've just scanned the first clause
 *  in a when clause list. We have to make it a child of a list node.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_list,"1",NULL);
   sem_stack[sem_top].sem_ast_tail =
      &((sem_stack[sem_top].sem_ast_ptr->ast_child.ast_child_ast)->ast_next);

   return;

}
./

   <guard_when_stmt_item>     ::= WHEN <expression> =>
                                     <statement_list>
/.
/*\
 *  \semact{guard when clause}
 *  \semcell{statement list}
 *  \semcell{condition}
 *  \semcell{WHEN}
 *  \sembottom
 *
 *  This rule is nearly identical to a case when clause.  The difference
 *  is that we accept only a single expression as a guard.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_when,"011",NULL);

   return;

}
./

   <guard_case_stmt_default>  ::= OTHERWISE =>
                                     <statement_list>

   <guard_case_stmt_default>  ::= %empty
/.
/*\
 *  \semact{missing otherwise clauses}
 *
 *  The higher-level rules count on an \verb"OTHERWISE" clause being
 *  present, so we push a null on the stack if there is no
 *  \verb"OTHERWISE" clause.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_null,"",NULL);

   return;

}
./

   <primary>                  ::= CASE <expression>
                                     <case_when_expr_list>
                                     <guard_case_expr_default>
                                  END CASE
/.
/*\
 *  \semact{case expression}
 *  \semcell{CASE}
 *  \semcell{otherwise clause}
 *  \semcell{statement list}
 *  \semcell{choice value}
 *  \semcell{CASE}
 *  \sembottom
 *
 *  Case expressions are difficult for the code generator, but all we do
 *  here is build an abstract syntax tree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_case_expr,"01110",NULL);

   return;

}
./

   <case_when_expr_list>      ::= <case_when_expr_list>
                                  <case_when_expr_item>
/.
/*\
 *  \semact{case when clause list}
 *  \semcell{when clause}
 *  \semcell{when clause list}
 *  \sembottom
 *
 *  This action is identical to many other list-building actions in the
 *  semantic actions.  When this action is invoked, we've just scanned a
 *  clause which must be appended to the current clause list. We use the
 *  tail pointer to update the current tail, then reset the tail
 *  pointer.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_AST);

#endif

   *(sem_stack[sem_top - 1].sem_ast_tail) =
      sem_stack[sem_top].sem_ast_ptr;
   sem_stack[sem_top - 1].sem_ast_tail =
      &(sem_stack[sem_top].sem_ast_ptr->ast_next);
   sem_top--;

   return;

}
./

   <case_when_expr_list>      ::= <case_when_expr_item>
/.
/*\
 *  \semact{case when clause list}
 *  \semcell{when clause}
 *  \sembottom
 *
 *  At this point we've scanned the first clause in a when clause list.
 *  We begin a list, and set the tail pointer.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_list,"1",NULL);
   sem_stack[sem_top].sem_ast_tail =
      &((sem_stack[sem_top].sem_ast_ptr->ast_child.ast_child_ast)->ast_next);

   return;

}
./

   <case_when_expr_item>      ::= WHEN <expression_list> =>
                                     <expression>
/.
/*\
 *  \semact{when clause}
 *  \semcell{statement list}
 *  \semcell{expression}
 *  \semcell{WHEN}
 *  \sembottom
 *
 *  We have to accumulate a list of \verb"WHEN" clauses in the process of
 *  building a \verb"case" or \verb"SELECT" subtree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_when,"011",NULL);

   return;

}
./

   <primary>                  ::= CASE
                                     <guard_when_expr_list>
                                     <guard_case_expr_default>
                                  END CASE
/.
/*\
 *  \semact{guard expression}
 *  \semcell{CASE}
 *  \semcell{otherwise clause}
 *  \semcell{statement list}
 *  \semcell{CASE}
 *  \sembottom
 *
 *  Select expressions are nearly identical to case expressions for the
 *  grammar and semantic actions.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_guard_expr,"0110",NULL);

   return;

}
./

   <guard_when_expr_list>     ::= <guard_when_expr_list>
                                  <guard_when_expr_item>
/.
/*\
 *  \semact{guard when lists}
 *  \semcell{when clause}
 *  \semcell{when clause list}
 *  \sembottom
 *
 *  This is yet another list-building action.  We need to separate it
 *  from case when lists to allow lists of expressions in case when
 *  clauses.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_AST);

#endif

   *(sem_stack[sem_top - 1].sem_ast_tail) =
      sem_stack[sem_top].sem_ast_ptr;
   sem_stack[sem_top - 1].sem_ast_tail =
      &(sem_stack[sem_top].sem_ast_ptr->ast_next);
   sem_top--;

   return;

}
./

   <guard_when_expr_list>     ::= <guard_when_expr_item>
/.
/*\
 *  \semact{first when clause in list}
 *  \semcell{clause}
 *  \sembottom
 *
 *  When this action is invoked, we've just scanned the first clause
 *  in a when clause list. We have to make it a child of a list node.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_list,"1",NULL);
   sem_stack[sem_top].sem_ast_tail =
      &((sem_stack[sem_top].sem_ast_ptr->ast_child.ast_child_ast)->ast_next);

   return;

}
./

   <guard_when_expr_item>     ::= WHEN <expression> =>
                                     <expression>
/.
/*\
 *  \semact{guard when clause}
 *  \semcell{statement list}
 *  \semcell{condition}
 *  \semcell{WHEN}
 *  \sembottom
 *
 *  This rule is nearly identical to a case when clause.  The difference
 *  is that we accept only a single expression as a guard.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_when,"011",NULL);

   return;

}
./

   <guard_case_expr_default>  ::= OTHERWISE =>
                                     <expression>

   <guard_case_expr_default>  ::= %empty
/.
/*\
 *  \semact{missing otherwise clauses}
 *
 *  The higher-level rules count on an \verb"OTHERWISE" clause being
 *  present.  In an expression, a missing otherwise returns OM.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
ast_ptr_type ast_ptr;                  /* created ast node                  */

   ast_ptr = get_ast(SETL_SYSTEM_VOID);
   ast_ptr->ast_type = ast_symtab;
   ast_ptr->ast_child.ast_symtab_ptr = sym_omega;
   get_sem;
   sem_stack[sem_top].sem_type = SEM_AST;
   copy_file_pos(&(ast_ptr->ast_file_pos),&(sem_stack[sem_top].sem_file_pos));
   sem_stack[sem_top].sem_ast_ptr = ast_ptr;

   return;

}
./
/.
#ifdef SHORT_FUNCS

default :

   semantic_action3(SETL_SYSTEM rule);
   return;

}}

#if MPWC
#pragma segment semact2
#endif

static void semantic_action3(
   SETL_SYSTEM_PROTO
   int rule)                           /* rule number                       */

{

   switch (rule) {

#endif
./

/*\
 *  \function{miscellaneous statement-like expressions}
 *
 *  We have various expressions which are generally used as statements
 *  rather than as expressions.  The semantic actions associated with
 *  these rules are trivial.
\*/

   <statement>                ::= STOP
/.
/*\
 *  \semact{stop expression}
 *  \semcell{STOP}
 *  \sembottom
 *
 *  A \verb"stop" will stop the program.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_stop,"0",&(sem_stack[sem_top].sem_file_pos));

   return;

}
./

   <statement>                ::= RETURN
/.
/*\
 *  \semact{return expression}
 *  \semcell{RETURN}
 *  \sembottom
 *
 *  A \verb"return" without a corresponding return value will return OM.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_return,"0",&(sem_stack[sem_top].sem_file_pos));

   return;

}
./

   <statement>                ::= RETURN <expression>
/.
/*\
 *  \semact{return statement}
 *  \semcell{return value}
 *  \semcell{RETURN}
 *  \sembottom
 *
 *  In \setl, there is no difference between functions and procedures.
 *  Essentially, there is an implicit return of OM at the end of every
 *  procedure, and return statements without a return value return OM.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_return,"01",&(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <statement>                ::= EXIT
/.
/*\
 *  \semact{exit expression}
 *  \semcell{EXIT}
 *  \sembottom
 *
 *  A \verb"EXIT" is used to break out of a \verb"FOR" or \verb"WHILE"
 *  loop.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_exit,"0",&(sem_stack[sem_top].sem_file_pos));

   return;

}
./

   <statement>                ::= EXIT <expression>
/.
/*\
 *  \semact{exit statement}
 *  \semcell{exit value}
 *  \semcell{EXIT}
 *  \sembottom
 *
 *  A exit with a value is only meaningful if a loop expression is used
 *  in a right hand side context.  In that case, the value of the loop is
 *  the value returned.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_exit,"01",&(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <statement>                ::= EXIT WHEN <expression>
/.
/*\
 *  \semact{exit statement}
 *  \semcell{exit value}
 *  \semcell{EXIT}
 *  \sembottom
 *
 *  A exit with a value is only meaningful if a loop expression is used
 *  in a right hand side context.  In that case, the value of the loop is
 *  the value returned.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_exit,"",&(sem_stack[sem_top - 2].sem_file_pos));
   build_ast(SETL_SYSTEM ast_null,"",NULL);
   build_ast(SETL_SYSTEM ast_if_stmt,"00111",&(sem_stack[sem_top - 3].sem_file_pos));

   return;

}
./

   <statement>                ::= EXIT <expression> WHEN <expression>
/.
/*\
 *  \semact{exit statement}
 *  \semcell{exit value}
 *  \semcell{EXIT}
 *  \sembottom
 *
 *  A exit with a value is only meaningful if a loop expression is used
 *  in a right hand side context.  In that case, the value of the loop is
 *  the value returned.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   memcpy(&sem_stack[sem_top - 1],
          &sem_stack[sem_top - 2],
          sizeof(struct sem_stack_item));
   memcpy(&sem_stack[sem_top - 2],
          &sem_stack[sem_top],
          sizeof(struct sem_stack_item));
   sem_top--;
   build_ast(SETL_SYSTEM ast_exit,"1",&(sem_stack[sem_top - 2].sem_file_pos));
   build_ast(SETL_SYSTEM ast_null,"",NULL);
   build_ast(SETL_SYSTEM ast_if_stmt,"0111",&(sem_stack[sem_top - 3].sem_file_pos));

   return;

}
./

   <statement>                ::= CONTINUE
/.
/*\
 *  \semact{continue expression}
 *  \semcell{CONTINUE}
 *  \sembottom
 *
 *  A \verb"CONTINUE" is used to cycle in a \verb"FOR" or \verb"WHILE"
 *  loop.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_continue,"0",&(sem_stack[sem_top].sem_file_pos));

   return;

}
./

   <statement>                ::= CONTINUE WHEN <expression>
/.
/*\
 *  \semact{continue statement}
 *  \semcell{CONTINUE}
 *  \sembottom
 *
 *  A continue with a value is only meaningful if a loop expression is used
 *  in a right hand side context.  In that case, the value of the loop is
 *  the value returned.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_continue,"",&(sem_stack[sem_top - 2].sem_file_pos));
   build_ast(SETL_SYSTEM ast_null,"",NULL);
   build_ast(SETL_SYSTEM ast_if_stmt,"00111",&(sem_stack[sem_top - 3].sem_file_pos));

   return;

}
./

   <statement>                ::= ASSERT <expression>
/.
/*\
 *  \semact{assert statement}
 *  \semcell{assert condition}
 *  \semcell{ASSERT}
 *  \sembottom
 *
 *  An assert is the way to code a program trap in \setl. If the condition
 *  fails, the program is aborted.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_assert,"01",&(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <statement>                ::= NULL
/.
/*\
 *  \semact{null statement}
 *  \semcell{NULL}
 *  \sembottom
 *
 *  A \verb"NULL" is used for empty statement lists.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_null,"0",&(sem_stack[sem_top].sem_file_pos));

   return;

}
./

/*\
 *  \function{lambda expressions}
 *
 *  $\lambda$-expressions are constant expressions which yield a
 *  procedure value.  They look a lot like procedure definitions.
\*/

   <primary>                  ::= <lambda_header> ;
                                  <data_declaration_part>
                                  <body>
                                  <proc_definition_part>
                                  END LAMBDA
/.
/*\
 *  \semact{lambda procedure}
 *
 *  When this rule is invoked, we've seen an entire procedure.  All we
 *  have to do is print the symbol table and pop the procedure table.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef DEBUG

   /* print the symbols in the procedure, if desired */

   if (SYM_DEBUG ) {

      fprintf(DEBUG_FILE,"\n%s => %s\n",
                         proctab_desc[curr_proctab_ptr->pr_type],
                         (curr_proctab_ptr->pr_namtab_ptr)->nt_name);

      print_symtab(SETL_SYSTEM curr_proctab_ptr);
      fputs("\n",DEBUG_FILE);

   }

#endif

   /* restore the initialization trees */

   sem_top--;
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   while (*ast_init_tail != NULL)
      ast_init_tail = &((*ast_init_tail)->ast_next);
   sem_top--;
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   while (*slot_init_tail != NULL)
      slot_init_tail = &((*slot_init_tail)->ast_next);
   sem_top--;
   var_init_tree = sem_stack[sem_top].sem_ast_ptr;
   var_init_tail = &var_init_tree;
   while (*var_init_tail != NULL)
      var_init_tail = &((*var_init_tail)->ast_next);
   sem_top--;

   /* push the created symbol */

   build_ast(SETL_SYSTEM ast_symtab,"",&(curr_proctab_ptr->pr_file_pos));
   (sem_stack[sem_top].sem_ast_ptr)->ast_child.ast_symtab_ptr =
         curr_proctab_ptr->pr_symtab_ptr;

   /* pop the current procedure */

   detach_symtab(curr_proctab_ptr->pr_symtab_head);
   curr_proctab_ptr = curr_proctab_ptr->pr_parent;

   return;

}
./

   <lambda_header>            ::= <lambda_key> <lambda_parameters>

   <lambda_key>               ::= LAMBDA
/.
/*\
 *  \semact{lambda key}
 *  \semcell{LAMBDA}
 *  \sembottom
 *
 *  When this action is invoked, we've just finished scanning the name
 *  portion of a procedure header.  At this point, we want to open up a
 *  new scope, since formal parameter names are internal to a procedure.
 *
 *  \begin{enumerate}
 *  \item
 *  We open a new procedure table item, inserting it as a child of the
 *  current procedure, and making it the new current procedure.
 *  \item
 *  We install the procedure name in the symbol table, as part of the
 *  enclosing procedure.
 *  \item
 *  We set up an empty list for the initialization code tree.
 *  \end{enumerate}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
proctab_ptr_type new_proc;             /* pointer to new procedure          */

   /* save the initialization trees (note: replace LAMBDA) */

   sem_stack[sem_top].sem_type = SEM_AST;
   sem_stack[sem_top].sem_ast_ptr = var_init_tree;
   get_sem;
   sem_stack[sem_top].sem_type = SEM_AST;
   sem_stack[sem_top].sem_ast_ptr = slot_init_tree;
   get_sem;
   sem_stack[sem_top].sem_type = SEM_AST;
   sem_stack[sem_top].sem_ast_ptr = ast_init_tree;

   /* open up a new procedure */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   new_proc->pr_namtab_ptr = nam_lambda;

   if (unit_proctab_ptr->pr_type == pr_class_spec ||
       unit_proctab_ptr->pr_type == pr_class_body ||
       unit_proctab_ptr->pr_type == pr_process_body ||
       unit_proctab_ptr->pr_type == pr_process_body) {
      new_proc->pr_type = pr_method;
   }
   else {
      new_proc->pr_type = pr_procedure;
   }
   copy_file_pos(&(new_proc->pr_file_pos),
                 &(sem_stack[sem_top - 2].sem_file_pos));

   /* install the procedure name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      NULL,
                      curr_proctab_ptr,
                      &(sem_stack[sem_top - 2].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

      (new_proc->pr_symtab_ptr)->st_type = sym_procedure;
      (new_proc->pr_symtab_ptr)->st_slot_num = -1;
      (new_proc->pr_symtab_ptr)->st_aux.st_proctab_ptr = new_proc;
      (new_proc->pr_symtab_ptr)->st_has_rvalue = YES;
      (new_proc->pr_symtab_ptr)->st_needs_stored = YES;
      (new_proc->pr_symtab_ptr)->st_is_declared = YES;

   }

   /* install an empty list as initialization code */

   if (unit_proctab_ptr->pr_type != pr_package_spec &&
       unit_proctab_ptr->pr_type != pr_class_spec ||
       unit_proctab_ptr->pr_type != pr_process_spec) {

      build_ast(SETL_SYSTEM ast_list,"",NULL);
      ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
      sem_top--;
      ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
      build_ast(SETL_SYSTEM ast_list,"",NULL);
      slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
      sem_top--;
      slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
      var_init_tree = NULL;
      var_init_tail = &var_init_tree;

   }

   /* make the new procedure current and pop the name from the stack */

   curr_proctab_ptr = new_proc;

   return;

}
./

   <lambda_parameters>        ::= ( <lambda_param_list> )
                              |   ( )
                              |   %empty
                              |   %error

   <lambda_param_list>        ::= <lambda_param_list> , <lambda_param_spec>
                              |   <lambda_param_spec>

   <lambda_param_spec>        ::= identifier
/.
/*\
 *  \semact{default formal parameter declaration}
 *  \semcell{formal name}
 *  \sembottom
 *
 *  A declaration of a formal parameter is almost identical to a variable
 *  declaration.  The formal paramters of a procedure are just the first
 *  {\em n} symbols in the procedure's local symbol table, where {\em n}
 *  is the number of formals.  All we have to do here is declare a
 *  variable, and bump the count of formal parameters for a procedure.
 *
 *  By default, a formal parameter is read only.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
symtab_ptr_type symtab_ptr;            /* formal parameter pointer          */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   symtab_ptr = enter_symbol(SETL_SYSTEM
                             sem_stack[sem_top].sem_namtab_ptr,
                             curr_proctab_ptr,
                             &(sem_stack[sem_top].sem_file_pos));

   if (symtab_ptr != NULL) {

      symtab_ptr->st_type = sym_id;
      symtab_ptr->st_has_rvalue = YES;
      symtab_ptr->st_has_lvalue = YES;
      symtab_ptr->st_is_rparam = YES;
      symtab_ptr->st_needs_stored = YES;

   }

   curr_proctab_ptr->pr_formal_count++;
   sem_top--;

   return;

}
./

   <lambda_param_spec>        ::= %error

/*\
 *  \function{operators}
 *
 *  The following list of rules enforce normal operator precedence.
\*/

   <expression>               ::= quantifier <expression_list>
                                     suchthat <expression>
/.
/*\
 *  \semact{quantifier expressions}
 *  \semcell{condition}
 *  \semcell{iterator list}
 *  \semcell{quantifier}
 *  \sembottom
 *
 *  Quantifiers in \setl\ are quite different from those in SETL in that they
 *  they are only used as predicates.  The values of the iteration
 *  variables are not set if the predicate is true.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   if (sem_stack[sem_top - 2].sem_token_subclass == tok_exists)
      sem_stack[sem_top - 1].sem_ast_ptr->ast_type = ast_ex_iter;
   else
      sem_stack[sem_top - 1].sem_ast_ptr->ast_type = ast_iter_list;

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 2].sem_token_subclass],
             "011",
             &(sem_stack[sem_top - 2].sem_file_pos));

   return;

}
./

   <expression>               ::= <left_term> fromop <left_term>
/.
/*\
 *  \semact{from expression}
 *  \semcell{right expression}
 *  \semcell{from operator}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  This is one of several places in which we have a set of binary
 *  operators with the same precedence.  We use a table by token subclass
 *  to pick the AST node type corresponding to the actual operator, and
 *  form the AST.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "101",
             &(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <expression>               ::= <and_expression>
                              |   <or_expression>
                              |   <not_term>

   <and_expression>           ::= <and_term> AND <not_term>
/.
/*\
 *  \semact{and expression}
 *  \semcell{right expression}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  \verb"AND" and \verb"OR" operators are unusual in that we don't allow
 *  them to be combined without parentheses.  We control this in the
 *  grammar, but it requires two rules with the same semantic action.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_and,"101",&(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <and_term>                 ::= <and_term> AND <not_term>
/.
/*\
 *  \semact{and expression}
 *  \semcell{right expression}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  \verb"AND" and \verb"OR" operators are unusual in that we don't allow
 *  them to be combined without parentheses.  We control this in the
 *  grammar, but it requires two rules with the same semantic action.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
   build_ast(SETL_SYSTEM ast_and,"101",&(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <and_term>                 ::= <not_term>

   <or_expression>            ::= <or_term> OR <not_term>
/.
/*\
 *  \semact{or expression}
 *  \semcell{right expression}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  \verb"AND" and \verb"OR" operators are unusual in that we don't allow
 *  them to be combined without parentheses.  We control this in the
 *  grammar, but it requires two rules with the same semantic action.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_or,"101",&(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <or_term>                  ::= <or_term> OR <not_term>
/.
/*\
 *  \semact{or expression}
 *  \semcell{right expression}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  \verb"AND" and \verb"OR" operators are unusual in that we don't allow
 *  them to be combined without parentheses.  We control this in the
 *  grammar, but it requires two rules with the same semantic action.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_or,"101",&(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <or_term>                  ::= <not_term>

   <not_term>                 ::= NOT <relop_term>
/.
/*\
 *  \semact{not expression}
 *  \semcell{expression}
 *  \sembottom
 *
 *  The NOT operator has a precedence all to itself.  Therefore, we don't
 *  push it on the semantic stack.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_not,"01",&(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <not_term>                 ::= <relop_term>

   <relop_term>               ::= <relop_term> relop <addop_term>
/.
/*\
 *  \semact{relational operator expression}
 *  \semcell{right expression}
 *  \semcell{relational operator}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  This is one of several places in which we have a set of binary
 *  operators with the same precedence.  We use a table by token subclass
 *  to pick the AST node type corresponding to the actual operator, and
 *  form the AST.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "101",
             &(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <relop_term>               ::= <addop_term>

   <addop_term>               ::= <addop_term> addop <mulop_term>
/.
/*\
 *  \semact{addition operator}
 *  \semcell{right expression}
 *  \semcell{binary operator}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  This is one of several places in which we have a set of binary
 *  operators with the same precedence.  We use a table by token subclass
 *  to pick the AST node type corresponding to the actual operator, and
 *  form the AST.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "101",
             &(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <addop_term>               ::= <addop_term> - <mulop_term>
/.
/*\
 *  \semact{subtraction expression}
 *  \semcell{right expression}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  Although binary subtraction has the same precedence as addition, we
 *  separate it from other addition operators because we also need it as
 *  a placeholder in parallel assignment expressions.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "101",
             &(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <addop_term>               ::= <mulop_term>

   <mulop_term>               ::= <mulop_term> mulop <expon_term>
/.
/*\
 *  \semact{multiplication operator}
 *  \semcell{right expression}
 *  \semcell{relational operator}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  This is one of several places in which we have a set of binary
 *  operators with the same precedence.  We use a table by token subclass
 *  to pick the AST node type corresponding to the actual operator, and
 *  form the AST.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "101",
             &(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <mulop_term>               ::= <mulop_term> applyop <expon_term>
/.
/*\
 *  \semact{binary application operator}
 *  \semcell{right expression}
 *  \semcell{operator}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  This is one of several places in which we have a set of binary
 *  operators with the same precedence.  We use a table by token subclass
 *  to pick the AST node type corresponding to the actual operator, and
 *  form the AST.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "101",
             &(sem_stack[sem_top - 1].sem_file_pos));

   build_ast(SETL_SYSTEM ast_binapply,"1",NULL);

   return;

}
./

   <mulop_term>               ::= <expon_term>

   <expon_term>               ::= <left_term> ** <expon_term>
/.
/*\
 *  \semact{exponentiation expression}
 *  \semcell{right expression}
 *  \semcell{left expression}
 *  \sembottom
 *
 *  Exponentiation is a strange expression.  First, it is not a class of
 *  operators, but a single operator with the highest precedence of all
 *  binary operators.  Secondly, it associates to the right, not the
 *  left.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "101",
             &(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <expon_term>               ::= <left_term>

   <left_term>                ::= unop <left_term>
/.
/*\
 *  \semact{unary operation}
 *  \semcell{expression}
 *  \sembottom
 *
 *  Like the binary operators, there is a group of unary operators which
 *  share the same precedence.  We use a table by token subclass to pick
 *  the AST node type corresponding to the actual operator, and form the
 *  AST.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "01",
             &(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <left_term>                ::= - <left_term>
/.
/*\
 *  \semact{unary minus operation}
 *  \semcell{expression}
 *  \sembottom
 *
 *  Although unary minus has the same precedence as other unary
 *  operators, we separate it from those since it can also be used as a
 *  placeholder in parallel assignments and as a binary operator.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
   build_ast(SETL_SYSTEM ast_uminus,
             "01",
             &(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./

   <left_term>                ::= fromop <left_term>

/.
/*\
 *  \semact{unary from operation}
 *  \semcell{expression}
 *  \sembottom
 *
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
   build_ast(SETL_SYSTEM ast_ufrom,
             "01",
             &(sem_stack[sem_top - 1].sem_file_pos));

   return;

}
./
   <left_term>                ::= ^ <left_term>
/.
/*\
 *  \semact{pointer operation}
 *  \semcell{expression}
 *  \semcell{*}
 *  \sembottom
 *
 *  This is a bit of syntactic sugar.  We map expressions of the form
 *  \verb"^x" into \verb"_memory(x)", where \verb"_memory" is a global
 *  map inaccessible by other means.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
ast_ptr_type ast_ptr;                  /* temporary ast pointer             */

   /* replace the ^ by _memory */

   ast_ptr = get_ast(SETL_SYSTEM_VOID);
   ast_ptr->ast_type = ast_symtab;
   ast_ptr->ast_child.ast_symtab_ptr = sym_memory;
   copy_file_pos(&(ast_ptr->ast_file_pos),
                 &(sem_stack[sem_top - 1].sem_file_pos));
   sem_stack[sem_top - 1].sem_type = SEM_AST;
   sem_stack[sem_top - 1].sem_ast_ptr = ast_ptr;

   /* build the argument list, with just one item */

   build_ast(SETL_SYSTEM ast_list,"1",&(sem_stack[sem_top].sem_file_pos));
   build_ast(SETL_SYSTEM ast_of,"11",&(sem_stack[sem_top].sem_file_pos));

   return;

}
./

   <left_term>                ::= applyop <left_term>
/.
/*\
 *  \semact{application operation}
 *  \semcell{expression}
 *  \sembottom
 *
 *  Application operators can be both unary and binary.  If unary, we
 *  have to add a null subtree as the left operand.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
   build_ast(SETL_SYSTEM tok_ast_type[sem_stack[sem_top - 1].sem_token_subclass],
             "01",
             &(sem_stack[sem_top - 1].sem_file_pos));

   build_ast(SETL_SYSTEM ast_apply,"1",NULL);

   return;

}
./

   <left_term>                ::= <primary>

/*\
 *  \function{primaries}
 *
 *  We think of a {\em primary} as some primitive operand.  It can
 *  include map, tuple, and string references, identifiers, literals, and
 *  anything else that any operator can operate upon.
\*/

   <primary>                  ::= literal
/.
/*\
 *  \semact{literals}
 *  \semcell{literal}
 *  \sembottom
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
ast_ptr_type ast_ptr;

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   ast_ptr = get_ast(SETL_SYSTEM_VOID);
   ast_ptr->ast_type = ast_symtab;
   ast_ptr->ast_child.ast_symtab_ptr =
         (sem_stack[sem_top].sem_namtab_ptr)->nt_symtab_ptr;
   copy_file_pos(&(ast_ptr->ast_file_pos),&(sem_stack[sem_top].sem_file_pos));
   sem_stack[sem_top].sem_type = SEM_AST;
   sem_stack[sem_top].sem_ast_ptr = ast_ptr;

   return;

}
./

   <primary>                  ::= { <expression_list> }
/.
/*\
 *  \semact{enumerated set former}
 *  \semcell{expression list}
 *  \sembottom
 *
 *  This action builds an enumerated set.  All we do is create an
 *  enumerated set node as parent.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);

#endif

   sem_stack[sem_top].sem_ast_ptr->ast_type = ast_enum_set;

   return;

}
./

   <primary>                  ::= { <expression_list> .. <expression> }
/.
/*\
 *  \semact{arithmetic iterator sets}
 *  \semcell{initial list}
 *  \semcell{final value}
 *  \sembottom
 *
 *  This rule handles set arithmetic iterators.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_arith_set,"11",NULL);

   return;

}
./

   <primary>                  ::= { <iterator_expression> suchthat
                                    <expression> }
/.
/*\
 *  \semact{general set (without expression)}
 *  \semcell{inclusion condition}
 *  \semcell{iterator}
 *  \sembottom
 *
 *  This action builds an enumerated set.  All we do is create an
 *  enumerated set node as parent.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_genset_noexp,"11",NULL);

   return;

}
./

   <primary>                  ::= { <expression> :
                                    <expression_list> suchthat
                                    <expression> }
/.
/*\
 *  \semact{general set}
 *  \semcell{inclusion condition}
 *  \semcell{iterator list}
 *  \semcell{expression}
 *  \sembottom
 *
 *  This action builds an enumerated set.  All we do is create an
 *  enumerated set node as parent.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   sem_stack[sem_top - 1].sem_ast_ptr->ast_type = ast_iter_list;
   build_ast(SETL_SYSTEM ast_genset,"111",NULL);

   return;

}
./

   <primary>                  ::= { <expression> : <expression_list> }
/.
/*\
 *  \semact{general set}
 *  \semcell{inclusion condition}
 *  \semcell{iterator list}
 *  \semcell{expression}
 *  \sembottom
 *
 *  This action builds an enumerated set.  All we do is create an
 *  enumerated set node as parent.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_AST);

#endif

   sem_stack[sem_top].sem_ast_ptr->ast_type = ast_iter_list;
   build_ast(SETL_SYSTEM ast_null,"",&(sem_stack[sem_top].sem_file_pos));
   build_ast(SETL_SYSTEM ast_genset,"111",NULL);

   return;

}
./

   <primary>                  ::= { }
/.
/*\
 *  \semact{empty set expression}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
ast_ptr_type ast_ptr;                  /* created ast node                  */

   ast_ptr = get_ast(SETL_SYSTEM_VOID);
   ast_ptr->ast_type = ast_symtab;
   ast_ptr->ast_child.ast_symtab_ptr = sym_nullset;
   get_sem;
   sem_stack[sem_top].sem_type = SEM_AST;
   copy_file_pos(&(ast_ptr->ast_file_pos),&(sem_stack[sem_top].sem_file_pos));
   sem_stack[sem_top].sem_ast_ptr = ast_ptr;

   return;

}
./

   <primary>                  ::= [ <tuple_list> .. <expression> ]
/.
/*\
 *  \semact{arithmetic tuple formers}
 *  \semcell{initial list}
 *  \semcell{final value}
 *  \sembottom
 *
 *  This rule handles arithmetic tuple formers.  All we do here is build
 *  an AST node.  We will have to check that the tuple does not contain
 *  placeholders in the semantic check phase.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_arith_tup,"11",NULL);

   return;

}
./

   <primary>                  ::= [ <iterator_expression> suchthat
                                    <expression> ]
/.
/*\
 *  \semact{general tuple (without expression)}
 *  \semcell{inclusion condition}
 *  \semcell{iterator}
 *  \sembottom
 *
 *  This action builds an enumerated tuple.  All we do is create an
 *  enumerated tuple node as parent.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_gentup_noexp,"11",NULL);

   return;

}
./

   <primary>                  ::= [ <expression> :
                                    <expression_list> suchthat
                                    <expression> ]
/.
/*\
 *  \semact{general tuple}
 *  \semcell{inclusion condition}
 *  \semcell{iterator list}
 *  \semcell{expression}
 *  \sembottom
 *
 *  This action builds an enumerated tuple.  All we do is create an
 *  enumerated tuple node as parent.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   sem_stack[sem_top - 1].sem_ast_ptr->ast_type = ast_iter_list;
   build_ast(SETL_SYSTEM ast_gentup,"111",NULL);

   return;

}
./

   <primary>                  ::= [ <expression> : <expression_list> ]
/.
/*\
 *  \semact{general tuple}
 *  \semcell{inclusion condition}
 *  \semcell{iterator list}
 *  \semcell{expression}
 *  \sembottom
 *
 *  This action builds an enumerated tuple.  All we do is create an
 *  enumerated tuple node as parent.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   sem_stack[sem_top].sem_ast_ptr->ast_type = ast_iter_list;
   build_ast(SETL_SYSTEM ast_null,"",&(sem_stack[sem_top].sem_file_pos));
   build_ast(SETL_SYSTEM ast_gentup,"111",NULL);

   return;

}
./

   <primary>                  ::= [ ]
/.
/*\
 *  \semact{empty tuple expression}
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
ast_ptr_type ast_ptr;                  /* created ast node                  */

   ast_ptr = get_ast(SETL_SYSTEM_VOID);
   ast_ptr->ast_type = ast_symtab;
   ast_ptr->ast_child.ast_symtab_ptr = sym_nulltup;
   get_sem;
   sem_stack[sem_top].sem_type = SEM_AST;
   copy_file_pos(&(ast_ptr->ast_file_pos),&(sem_stack[sem_top].sem_file_pos));
   sem_stack[sem_top].sem_ast_ptr = ast_ptr;

   return;

}
./

   <iterator_expression>      ::= <expression>
/.
/*\
 *  \semact{iterator expression}
 *  \semcell{expression}
 *  \sembottom
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_iter_list,"1",NULL);

   return;

}
./

   <primary>                  ::= ( <expression> )

/*\
 *  \function{left hand sides}
 *
 *  This is an attempt to isolate as many legitimate left hand sides as
 *  possible.  We do not try to completely control left hand sides in
 *  the grammar, but rely on semantic checks as well.
\*/

   <primary>                  ::= identifier
/.
/*\
 *  \semact{identifiers}
 *  \semcell{identifier}
 *  \sembottom
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
ast_ptr_type ast_ptr;

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);

#endif

   ast_ptr = get_ast(SETL_SYSTEM_VOID);
   ast_ptr->ast_type = ast_namtab;
   ast_ptr->ast_child.ast_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   copy_file_pos(&(ast_ptr->ast_file_pos),&(sem_stack[sem_top].sem_file_pos));
   sem_stack[sem_top].sem_type = SEM_AST;
   sem_stack[sem_top].sem_ast_ptr = ast_ptr;

   return;

}
./

   <primary>                  ::= SELF
/.
/*\
 *  \semact{self keyword}
 *  \semcell{SELF}
 *  \sembottom
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
ast_ptr_type ast_ptr;

   if((curr_proctab_ptr->pr_symtab_ptr->st_type==sym_procedure) &&
      (curr_proctab_ptr->pr_symtab_ptr->st_slot_num==0)) {

     ast_ptr = get_ast(SETL_SYSTEM_VOID);
     ast_ptr->ast_type = ast_namtab;
     ast_ptr->ast_child.ast_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
     copy_file_pos(&(ast_ptr->ast_file_pos),&(sem_stack[sem_top].sem_file_pos));
     sem_stack[sem_top].sem_type = SEM_AST;
     sem_stack[sem_top].sem_ast_ptr = ast_ptr;

   } else build_ast(SETL_SYSTEM ast_self,"0",NULL); 

   return;

}
./

   <primary>                  ::= <primary> ( <expression_list> )
/.
/*\
 *  \semact{selector expression}
 *  \semcell{map or procedure}
 *  \semcell{argument}
 *  \sembottom
 *
 *  This rule handles selector expressions.  We don't know what we are
 *  selecting from at this point, it could be a map, tuple, or a
 *  procedure call.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_of,"11",NULL);

   return;

}
./

   <primary>                  ::= <primary> ( <expression> .. <expression> )
/.
/*\
 *  \semact{slice expression}
 *  \semcell{last position}
 *  \semcell{first position}
 *  \semcell{expression}
 *  \sembottom
 *
 *  This rule handles slice expression.  It refers to a slice of a string
 *  or tuple.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_slice,"111",NULL);

   return;

}
./

   <primary>                  ::= <primary> ( <expression> .. )
/.
/*\
 *  \semact{slice expression}
 *  \semcell{first position}
 *  \semcell{expression}
 *  \sembottom
 *
 *  This rule handles an alternate form of slice expression, where the
 *  last element of the slice is the end of the string or tuple.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_end,"11",NULL);

   return;

}
./

   <primary>                  ::= <primary> ( )
/.
/*\
 *  \semact{selector expression}
 *  \semcell{procedure}
 *  \sembottom
 *
 *  This rule handles selector expressions.  In this case we have no
 *  arguments, so it better turn out to be a procedure call.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_list,"",&(sem_stack[sem_top].sem_file_pos));
   build_ast(SETL_SYSTEM ast_of,"11",NULL);

   return;

}
./

   <primary>                  ::= <primary> { <expression_list> }
/.
/*\
 *  \semact{milti-valued map selectors}
 *  \semcell{argument}
 *  \semcell{expression}
 *  \sembottom
 *
 *  This rule handles expressions returning a set for a multi-valued map.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   build_ast(SETL_SYSTEM ast_ofa,"11",NULL);

   return;

}
./

   <primary>                  ::= <primary> . identifier
/.
/*\
 *  \semact{qualifier expressions}
 *  \semcell{right hand side}
 *  \semcell{left hand side}
 *  \sembottom
 *
 *  This rule actually handles two situations, depending on the oprands
 *  given.  If the left hand operand is the name of an enclosing
 *  procedure or an imported package, the right hand side must be an
 *  identifier hidden by a local declaration.  If the right hand side is
 *  a selector, we can replace the expression by the corresponding
 *  selector expression in the next phase.
 *
 *  The semantic processing of this rule is a little tricky.  We use left
 *  recursion in the grammar, but we want to keep a list of operands all
 *  on one level in the tree.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{
ast_ptr_type ast_ptr;                  /* created AST node                  */

#ifdef TRAPS

   verify_sem(sem_top,SEM_NAMTAB);
   verify_sem(sem_top - 1,SEM_AST);

#endif

   /* first, we make the name table item on the top of stack into an AST */

   ast_ptr = get_ast(SETL_SYSTEM_VOID);
   ast_ptr->ast_type = ast_namtab;
   ast_ptr->ast_child.ast_namtab_ptr = sem_stack[sem_top].sem_namtab_ptr;
   copy_file_pos(&(ast_ptr->ast_file_pos),
                 &(sem_stack[sem_top].sem_file_pos));
   sem_stack[sem_top].sem_type = SEM_AST;
   sem_stack[sem_top].sem_ast_ptr = ast_ptr;

   /* if the left child is a dot, we just append to the list of operands */

   if (sem_stack[sem_top - 1].sem_ast_ptr->ast_type == ast_dot) {

      *(sem_stack[sem_top - 1].sem_ast_tail) =
         sem_stack[sem_top].sem_ast_ptr;
      sem_stack[sem_top - 1].sem_ast_tail =
         &(sem_stack[sem_top].sem_ast_ptr->ast_next);
      sem_top--;

   }

   /* otherwise we start a new list */

   else {

      build_ast(SETL_SYSTEM ast_dot,"11",NULL);
      sem_stack[sem_top].sem_ast_tail =
         &(((sem_stack[sem_top].sem_ast_ptr->
         ast_child.ast_child_ast)->ast_next)->ast_next);

   }

   return;

}
./

   <primary>                  ::= [ <tuple_list> ]
/.
/*\
 *  \semact{enumerated tuple former}
 *  \semcell{expression list}
 *  \sembottom
 *
 *  This action builds an enumerated tuple.  All we do now is create an
 *  enumerated tuple node as parent.  In the next phase, we have to look
 *  for placeholders in the tuple, depending on whether it's used on the
 *  left or the right.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);

#endif

   sem_stack[sem_top].sem_ast_ptr->ast_type = ast_enum_tup;

   return;

}
./

   <primary>                  ::= %error

   <tuple_list>               ::= <tuple_list> , <tuple_element>
/.
/*\
 *  \semact{tuple expression lists}
 *  \semcell{new expression}
 *  \semcell{expression list}
 *  \sembottom
 *
 *  When this action is invoked, we've just scanned an expression which
 *  must be appended to the current expression list. We use the tail
 *  pointer to update the current tail, then reset the tail pointer.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);
   verify_sem(sem_top - 1,SEM_AST);

#endif

   *(sem_stack[sem_top - 1].sem_ast_tail) =
      sem_stack[sem_top].sem_ast_ptr;
   sem_stack[sem_top - 1].sem_ast_tail =
      &(sem_stack[sem_top].sem_ast_ptr->ast_next);
   sem_top--;

   return;

}
./

   <tuple_list>               ::= <tuple_element>
/.
/*\
 *  \semact{first expression in tuple list}
 *  \semcell{expression}
 *  \sembottom
 *
 *  When this action is invoked, we've just scanned the first expression
 *  in a tuple expression list. We have to make it a child of a list node.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

#ifdef TRAPS

   verify_sem(sem_top,SEM_AST);

#endif

   build_ast(SETL_SYSTEM ast_list,"1",NULL);
   sem_stack[sem_top].sem_ast_tail =
      &((sem_stack[sem_top].sem_ast_ptr->ast_child.ast_child_ast)->ast_next);

   return;

}
./

   <tuple_element>            ::= <expression>

   <tuple_element>            ::= -
/.
/*\
 *  \semact{placeholders}
 *
 *  \setl\ allows enumerated tuples to appear on the left hand side of
 *  assignment expressions, indicating a tuple element to be discarded.
 *  All we do here is insert a placeholder node in the AST.
\*/

/*
 *  Rule: %rule_text
 */

case %rule_num :

{

   /* remove dash from top of stack */

   sem_top--;

   /* build placeholder */

   build_ast(SETL_SYSTEM ast_placeholder,"",&(sem_stack[sem_top].sem_file_pos));

   return;

}
./
/.

default :

   return;

}}

/*\
 *  \function{init\_semacts()}
 *
 *  This function initializes the semantic actions.  All we do is set the
 *  current procedure to the root.
\*/

void init_semacts()

{

   sem_top = -1;
   unit_proctab_ptr = curr_proctab_ptr = predef_proctab_ptr;
   return;

}

/*\
 *  \function{close\_semacts()}
 *
 *  This function is called after we accept the input file.  If we are
 *  debugging, and a listing of the procedure tree is requested, we print
 *  it.
\*/

void close_semacts(SETL_SYSTEM_PROTO_VOID)

{

#ifdef DEBUG

   if (PROCTAB_DEBUG) {
      print_proctab(SETL_SYSTEM_VOID);
      fputs("\n",DEBUG_FILE);
   }

#endif

}

/*\
 *  \function{push\_token()}
 *
 *  This function pushes a token on the semantic stack.  It is called by
 *  the parser for all `important' tokens.
\*/

void push_token(
   SETL_SYSTEM_PROTO
   token_type *token)                  /* token passed from parser          */

{

   get_sem;
   sem_stack[sem_top].sem_type = SEM_NAMTAB;
   sem_stack[sem_top].sem_namtab_ptr = token->tk_namtab_ptr;
   sem_stack[sem_top].sem_token_subclass = token->tk_token_subclass;
   copy_file_pos(&sem_stack[sem_top].sem_file_pos,&(token->tk_file_pos));

}

/*\
 *  \function{build\_ast()}
 *
 *  This function creates an ast node, with some number of children.  The
 *  top {\em n} items on the semantic stack should all be abstract syntax
 *  trees, and are linked as children of the created node.
\*/

static void build_ast(
   SETL_SYSTEM_PROTO
   int type,                           /* type of node built                */
   char *template,                     /* template of semantic stack        */
   struct file_pos_item *file_pos)     /* root file position                */

{
ast_ptr_type ast_ptr;                  /* pointer to created item           */
ast_ptr_type first_child,*next_child;  /* first child node                  */
int num_children;                      /* number of children                */
char *p;                               /* temporary looping variable        */
int i;                                 /* temporary looping variable        */

   /* save the number of children */

   num_children = strlen(template);

   /* link together the children */

   first_child = NULL;
   next_child = &first_child;
   for (i = sem_top - num_children + 1, p = template; *p; i++, p++) {

      if (*p == '1') {

#ifdef TRAPS

         verify_sem(i,SEM_AST);

#endif
         *next_child = sem_stack[i].sem_ast_ptr;
         next_child = &((*next_child)->ast_next);

      }

#ifdef TRAPS

      else {
         verify_sem(i,SEM_NAMTAB);
      }

#endif

   }

   /* create the new node */

   ast_ptr = get_ast(SETL_SYSTEM_VOID);
   ast_ptr->ast_type = type;

   /* attach children */

   ast_ptr->ast_child.ast_child_ast = first_child;
   ast_ptr->ast_next = NULL;

   /* set the file position */

   if (file_pos != NULL) {

      copy_file_pos(&(ast_ptr->ast_file_pos),file_pos);

   }
   else if (num_children > 0) {

      copy_file_pos(&(ast_ptr->ast_file_pos),
                 &(sem_stack[sem_top - num_children + 1].sem_file_pos));

   }
   else {

      ast_ptr->ast_file_pos.fp_line = 0;
      ast_ptr->ast_file_pos.fp_column = 0;

   }

   /* replace the children by the new node, on the semantic stack */

   sem_top -= num_children;
   get_sem;
   sem_stack[sem_top].sem_type = SEM_AST;
   sem_stack[sem_top].sem_ast_ptr = ast_ptr;
   copy_file_pos(&sem_stack[sem_top].sem_file_pos,&(ast_ptr->ast_file_pos));

}

/*\
 *  \function{build\_method()}
 *
 *  This function builds a method from a template.  There are a number of
 *  these with somewhat different syntax, so we use this general function
 *  rather than creating many nearly identical semantic actions.
\*/

static void build_method(
   SETL_SYSTEM_PROTO
   int method_code,                    /* index of built-in method          */
   char *template)                     /* template of semantic stack        */

{
namtab_ptr_type namtab_ptr;            /* internal name of method           */
int sem_length;                        /* length of semantic stack entries  */
proctab_ptr_type new_proc;             /* pointer to new procedure          */
symtab_ptr_type symtab_ptr;            /* formal parameter pointer          */
char *p;                               /* temporary looping variable        */
int i;                                 /* temporary looping variable        */
int ext;                               /* Defining inside error_extension.. */
char extprocname[32];

  
   ext = NO;
   if (strncmp((unit_proctab_ptr->pr_namtab_ptr)->nt_name,
                "ERROR_EXTENSION",15)==0) ext = YES;

   /* save number of relevant entries */

   sem_length = strlen(template);
   if (ext==NO) 
     namtab_ptr = method_name[method_code];
   else {
     sprintf(extprocname,"$ERR_EXT%d",method_code);
     namtab_ptr = get_namtab(SETL_SYSTEM extprocname);
     


   }

   /* if we are not processing a class body, we've found an error */

   if ((unit_proctab_ptr->pr_type != pr_class_body)&&
       ((unit_proctab_ptr->pr_type!= pr_package_body)||(ext!=YES))) {
      

      error_message(SETL_SYSTEM &sem_stack[sem_top - sem_length + 1].sem_file_pos,
                    "Built-in methods are only valid in class bodies");

      return;

   }

   /* open up a new procedure */

   new_proc = get_proctab(SETL_SYSTEM_VOID);
   new_proc->pr_parent = curr_proctab_ptr;
   *(curr_proctab_ptr->pr_tail) = new_proc;
   curr_proctab_ptr->pr_tail = &(new_proc->pr_next);
   if (ext==NO) {
      new_proc->pr_namtab_ptr = namtab_ptr;
      new_proc->pr_type = pr_method;
      new_proc->pr_file_pos.fp_line = 0;
      new_proc->pr_file_pos.fp_column = 0;
   } else {
      new_proc->pr_namtab_ptr = namtab_ptr;
      new_proc->pr_type = pr_procedure;
      copy_file_pos(&(new_proc->pr_file_pos),
                    &(sem_stack[sem_top - sem_length + 1].sem_file_pos));
   }

   /* install the method name */

   new_proc->pr_symtab_ptr =
         enter_symbol(SETL_SYSTEM
                      namtab_ptr,
                      curr_proctab_ptr,
                      &(sem_stack[sem_top - sem_length + 1].sem_file_pos));

   if (new_proc->pr_symtab_ptr != NULL) {

    if (ext==NO) {
      (new_proc->pr_symtab_ptr)->st_type = sym_method;
      (new_proc->pr_symtab_ptr)->st_slot_num = method_code;
      (new_proc->pr_symtab_ptr)->st_class = unit_proctab_ptr;
    } else {
      (new_proc->pr_symtab_ptr)->st_type = sym_procedure;
      (new_proc->pr_symtab_ptr)->st_slot_num = 0;
      (new_proc->pr_symtab_ptr)->st_is_initialized = YES;
    }
    (new_proc->pr_symtab_ptr)->st_aux.st_proctab_ptr = new_proc;
    (new_proc->pr_symtab_ptr)->st_has_rvalue = YES;
    (new_proc->pr_symtab_ptr)->st_needs_stored = YES;
    (new_proc->pr_symtab_ptr)->st_is_declared = YES;
    (new_proc->pr_symtab_ptr)->st_is_public = YES;
   }

   /* install an empty list as initialization code */

   build_ast(SETL_SYSTEM ast_list,"",NULL);
   ast_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   ast_init_tail = &(ast_init_tree->ast_child.ast_child_ast);
   build_ast(SETL_SYSTEM ast_list,"",NULL);
   slot_init_tree = sem_stack[sem_top].sem_ast_ptr;
   sem_top--;
   slot_init_tail = &(slot_init_tree->ast_child.ast_child_ast);
   var_init_tree = NULL;
   var_init_tail = &var_init_tree;

   /* make the new procedure current */

   curr_proctab_ptr = new_proc;

   /* install the formal parameters */

   for (i = sem_top - sem_length + 1, p = template; *p; i++, p++) {

      if ((*p == '1')||((*p == 'S')&&(ext == YES))) {

#ifdef TRAPS

         verify_sem(i,SEM_NAMTAB);

#endif

         symtab_ptr = enter_symbol(SETL_SYSTEM
                                   sem_stack[i].sem_namtab_ptr,
                                   curr_proctab_ptr,
                                   &(sem_stack[i].sem_file_pos));

         if (symtab_ptr != NULL) {

            symtab_ptr->st_type = sym_id;
            symtab_ptr->st_has_lvalue = YES;
            symtab_ptr->st_has_rvalue = YES;
            symtab_ptr->st_is_rparam = YES;
            symtab_ptr->st_needs_stored = YES;

         }

         curr_proctab_ptr->pr_formal_count++;

      }

#ifdef TRAPS

      else {

         verify_sem(i,SEM_NAMTAB);

      }

#endif

   }

   /* pop the semantic stack */

   sem_top -= sem_length;

   return;

}

/*\
 *  \function{alloc\_sem()}
 *
 *  This function allocates a block in the semantic stack.  This table is
 *  organized as an `expanding array'.  That is, we allocate an array of a
 *  given size, then when that is exceeded, we allocate a larger array and
 *  copy the smaller to the larger.  This makes allocations slower than
 *  for the other tables, but makes it easy to reach down the stack, which
 *  we need to do in this table.  We don't expect the speed penalty to be
 *  a problem, since the stack should never get very big.
 *
 *  Notice: this function is only called indirectly, through the macro
 *  \verb"get_sem()".  Most of the time, all we need to do to allocate a
 *  new item is to increment the stack top.  We therefore defined a
 *  macro which did that, and called this function on a stack overflow.
\*/

static int alloc_sem(SETL_SYSTEM_PROTO_VOID)

{
struct sem_stack_item *temp_sem_stack; /* temporary semantic stack          */

   /* expand the table */

   temp_sem_stack = sem_stack;
   sem_stack = (struct sem_stack_item *)malloc((size_t)(
               (sem_max + SEM_BLOCK_SIZE) * sizeof(struct sem_stack_item)));
   if (sem_stack == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* copy the old table to the new table, and free the old table */

   if (sem_max > 0) {

      memcpy((void *)sem_stack,
             (void *)temp_sem_stack,
             (size_t)(sem_max * sizeof(struct sem_stack_item)));

      free(temp_sem_stack);

   }

   sem_max += SEM_BLOCK_SIZE;

   return ++sem_top;

}
/*\
 *  \function{verify\_sem()}
 *
 *  This function is used many places, simply to verify that the
 *  semantic stack is reasonable.  We can't actually verify that it is
 *  correct, but we can check whether an expected item is present, and
 *  that it is of the correct type.
\*/

#ifdef TRAPS

static void verify_sem(
   int stack_ptr,                      /* item to be checked                */
   int item_type)                      /* expected item type                */

{

#ifdef DEBUG

   if ((stack_ptr) < 0 || sem_stack[(stack_ptr)].sem_type != (item_type)) {
      print_sem(SETL_SYSTEM_VOID);
      trap(__FILE__,__LINE__,msg_bad_sem_stack);
   }

#else

   if ((stack_ptr) < 0 || sem_stack[(stack_ptr)].sem_type != (item_type))
      trap(__FILE__,__LINE__,msg_bad_sem_stack);

#endif

}

#endif

/*\
 *  \function{print\_sem()}
 *
 *  During debugging, it is frequently useful to print the semantic
 *  stack.  One of the more difficult tasks in building a compiler is
 *  writing the semantic actions.  When the stack isn't as expected, it
 *  helps to be able to easily print it.
\*/

#ifdef DEBUG

static void print_sem(SETL_SYSTEM_PROTO_VOID)

{
int i;                                 /* temporary looping variable        */

   fputs("\nSemantic Stack\n--------------\n\n",DEBUG_FILE);

   for (i = sem_top; i >= 0; i--) {

      if (sem_stack[i].sem_type == SEM_AST) {
         fprintf(DEBUG_FILE,"item %2d: AST =>\n",i);
         print_ast(SETL_SYSTEM sem_stack[i].sem_ast_ptr,NULL);
         fputs("\n",DEBUG_FILE);
      }
      else {
         fprintf(DEBUG_FILE,"item %2d: name => %s\n",
                 i,sem_stack[i].sem_namtab_ptr->nt_name);
      }
   }
}

#endif

./

%end

