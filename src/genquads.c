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
 *  \package{The Quadruple Generator}
 *
 *  This is the driver package for intermediate code generation.  The
 *  intermediate code generator is actually quite large and complex, but
 *  most of the work is done in other packages which generate code for
 *  statments, expressions, boolean conditions, etc.
 *
 *  Here, we set up for code generation and call the statement generator.
 *  We set up the symbol table (loading any used or inherited units),
 *  transform name table pointers to symbol table pointers (following
 *  correct visibility rules), and generate a quadruple stream.
 *
 *  \texify{genquads.h}
 *
 *  \packagebody{Quadruple Generator}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "chartab.h"                   /* character type table              */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "libman.h"                    /* library manager                   */
#include "builtins.h"                  /* built-in symbols                  */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "ast.h"                       /* abstract syntax tree              */
#include "quads.h"                     /* quadruples                        */
#include "c_integers.h"                  /* integer literals                  */
#include "c_reals.h"                     /* real literals                     */
#include "c_strngs.h"                    /* string literals                   */
#include "import.h"                    /* imported packages and classes     */
#include "lex.h"                       /* lexical analyzer                  */
#include "genquads.h"                  /* quadruple generator               */
#include "semcheck.h"                  /* semantic checks (2nd pass)        */
#include "genstmt.h"                   /* statement code generator          */
#include "listing.h"                   /* source and error listings         */


/* standard C header files */

#include <ctype.h>                     /* character macros                  */
#if UNIX_VARARGS
#include <varargs.h>                   /* variable argument macros          */
#else
#include <stdarg.h>                    /* variable argument macros          */
#endif


#ifdef PLUGIN
#define fprintf plugin_fprintf
#define fputs   plugin_fputs
#endif

/* performance tuning constants */

#define LSTACK_BLOCK_SIZE     20       /* loop stack block size             */

/* forward declarations */

static void setup_symtab(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* load a procedure symbol table     */
static void load_superclass_list(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* load a procedure symbol table     */
static void load_used_units(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* load an imported package          */
static void load_superclasses(SETL_SYSTEM_PROTO proctab_ptr_type,proctab_ptr_type,int);
                                       /* load superclasses                 */
static void load_package_spec(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* load a package specification      */
static void load_class_spec(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* load a class specification        */
static void gen_procedure(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* generate code for a procedure     */
#if UNIX_VARARGS
static int get_yes_no();               /* query user                        */
#else
static int get_yes_no(char *, ...);    /* query user                        */
#endif

/*\
 *  \function{gen\_quads()}
 *
 *  This function generates quadruples from abstract syntax trees.  The
 *  real work is done in \verb"gen_procedure()",  all we do here is make
 *  sure we can update the library.
\*/

void gen_quads(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* procedure to be generated         */

{
libunit_ptr_type libunit_ptr;          /* work library unit pointer         */
libstr_ptr_type libstr_ptr;            /* work library stream pointer       */
unit_control_record unit_control;      /* unit control record               */

#ifdef DEBUG

   if (AST_DEBUG || SYM_DEBUG) {

      fprintf(DEBUG_FILE,"\nINTERMEDIATE CODE GENERATION PHASE\n");
      fprintf(DEBUG_FILE,"==================================\n\n");

   }

#endif

   /*
    *  First we do some error checking.  We check if there is an existing
    *  unit in the library with the same name, and if so we check whether
    *  it can be replaced.
    */

   if ((libunit_ptr = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                   I2_FILE,LIB_READ_UNIT)) != NULL ||
       (libunit_ptr = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                   DEFAULT_LIBFILE,LIB_READ_UNIT)) != NULL) {

      /* read the unit control record */

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
                  sizeof(unit_control_record));
      close_libstr(SETL_SYSTEM libstr_ptr);
      close_libunit(SETL_SYSTEM libunit_ptr);

      /* the checks vary with compilation unit type */

      switch (proctab_ptr->pr_type) {

         case pr_package_spec :
         case pr_native_package :

            if (strcmp(unit_control.uc_spec_source_name,C_SOURCE_NAME) != 0) {

               if (SAFETY_CHECK && !get_yes_no(
                   msg_existing_unit,
                   (proctab_ptr->pr_namtab_ptr)->nt_name,
                   unit_control.uc_spec_source_name)) {

                  error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_unit_not_comp,
                       (proctab_ptr->pr_namtab_ptr)->nt_name);

               }
            }
            else if ((unit_control.uc_type != PACKAGE_UNIT)&&
                     (unit_control.uc_type != NATIVE_UNIT)) {

               if (SAFETY_CHECK && !get_yes_no(
                   msg_expected_pack,
                   (proctab_ptr->pr_namtab_ptr)->nt_name)) {

                  error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_unit_not_comp,
                       (proctab_ptr->pr_namtab_ptr)->nt_name);

               }
            }

            break;

         case pr_package_body :

            /* we check package bodies when we load the specification */

            break;

         case pr_class_spec :

            if (strcmp(unit_control.uc_spec_source_name,C_SOURCE_NAME) != 0) {

               if (SAFETY_CHECK && !get_yes_no(
                   msg_existing_unit,
                   (proctab_ptr->pr_namtab_ptr)->nt_name,
                   unit_control.uc_spec_source_name)) {

                  error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_unit_not_comp,
                       (proctab_ptr->pr_namtab_ptr)->nt_name);

               }
            }
            else if (unit_control.uc_type != CLASS_UNIT) {

               if (SAFETY_CHECK && !get_yes_no(
                   "%s is not a class. Overwrite? ",
                   (proctab_ptr->pr_namtab_ptr)->nt_name)) {

                  error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_unit_not_comp,
                       (proctab_ptr->pr_namtab_ptr)->nt_name);

               }
            }

            break;

         case pr_class_body :

            /* we check class bodies when we load the specification */

            break;

         case pr_process_spec :

            if (strcmp(unit_control.uc_spec_source_name,C_SOURCE_NAME) != 0) {

               if (SAFETY_CHECK && !get_yes_no(
                   msg_existing_unit,
                   (proctab_ptr->pr_namtab_ptr)->nt_name,
                   unit_control.uc_spec_source_name)) {

                  error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_unit_not_comp,
                       (proctab_ptr->pr_namtab_ptr)->nt_name);

               }
            }
            else if (unit_control.uc_type != PROCESS_UNIT) {

               if (SAFETY_CHECK && !get_yes_no(
                   "%s is not a process class. Overwrite? ",
                   (proctab_ptr->pr_namtab_ptr)->nt_name)) {

                  error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_unit_not_comp,
                       (proctab_ptr->pr_namtab_ptr)->nt_name);

               }
            }

            break;

         case pr_process_body :

            /* we check process bodies when we load the specification */

            break;

         case pr_program :

            if (strcmp(unit_control.uc_spec_source_name,C_SOURCE_NAME) != 0) {

               if (SAFETY_CHECK && !get_yes_no(
                   msg_existing_unit,
                   (proctab_ptr->pr_namtab_ptr)->nt_name,
                   unit_control.uc_spec_source_name)) {

                  error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_unit_not_comp,
                       (proctab_ptr->pr_namtab_ptr)->nt_name);

               }
            }
            else if (unit_control.uc_type != PROGRAM_UNIT) {

               if (SAFETY_CHECK && !get_yes_no(
                   msg_expected_prog,
                   (proctab_ptr->pr_namtab_ptr)->nt_name)) {

                  error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_unit_not_comp,
                       (proctab_ptr->pr_namtab_ptr)->nt_name);

               }
            }

            break;

      }
   }

   /* we passed the error check -- process each procedure */

   unit_proctab_ptr = proctab_ptr;
   gen_procedure(SETL_SYSTEM proctab_ptr);

   /*
    *  if we're using intermediate files initialize the ast (to free the
    *  memory)
    */

   if (USE_INTERMEDIATE_FILES)
      init_ast();

}

/*\
 *  \function{gen\_procedure()}
 *
 *  This function generates code for one unit, of any kind.  Most of the
 *  time these will be simple procedures or methods.
\*/

static void gen_procedure(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* procedure to be processed         */

{
ast_ptr_type ast_root;                 /* root of tree being processed      */
symtab_ptr_type save_next_temp;        /* saved temporary list              */
int save_next_label;                   /* saved label                       */
symtab_ptr_type operand[3];            /* array of operands                 */
symtab_ptr_type formal_ptr;            /* formal parameter pointer          */
int formal_num;                        /* formal number                     */
int opnd_num;                          /* temporary looping variable        */
symtab_ptr_type symtab_ptr;            /* temporary looping variable        */

#ifdef DYNAMIC_COMP
global_ptr_type global_ptr;            /* temporary looping variable        */
#endif

symtab_ptr_type symtab_ptr2;           /* temporary looping variable        */

ast_ptr_type var_ptr;                  /* variable subtree                  */
ast_ptr_type sym_ptr;                  /* variable subtree                  */
ast_ptr_type assign_ptr;    
ast_ptr_type enum_tup_ptr;    
ast_ptr_type enum_tup_str_ptr;    
ast_ptr_type enum_tup_proc_ptr;    
ast_ptr_type list_ptr;    
int notnullmap;
namtab_ptr_type namtab_ptr;            /* pointer to installed name         */
char *proc_symbol;
char *tmp;
int nunds;

   /* loop over procedures on this level */

   while (proctab_ptr != NULL) {

      /* load the symbol table for this procedure */

      setup_symtab(SETL_SYSTEM proctab_ptr);

      /* load the initialization ast */

      ast_root = load_ast(SETL_SYSTEM &(proctab_ptr->pr_init_code));
      curr_proctab_ptr = proctab_ptr;

      /* perform semantic checks */

      check_semantics(SETL_SYSTEM ast_root);

#ifdef DYNAMIC_COMP
      if ((COMPILING_EVAL==YES)&&(proctab_ptr->pr_type == pr_program))   {

         /* Check if a toplevel variable has been declared global */

         for (symtab_ptr = proctab_ptr->pr_symtab_head;
            symtab_ptr != NULL; symtab_ptr = symtab_ptr->st_thread) {

            if ((symtab_ptr->st_unit_num < 0) &&
                ((symtab_ptr->st_type == sym_id)  ||
                 (symtab_ptr->st_type == sym_procedure)) ) {

               global_ptr = GLOBAL_HEAD;
	       while (global_ptr != NULL ) {
	          if ((strcmp(symtab_ptr->st_namtab_ptr->nt_name,
			 global_ptr->gl_name)==0)) {
                     global_ptr->gl_present=YES;
                    
                    /* 
                     *
                     * A global variable can't be declared again as a 
                     * global procedure, and vice-versa...
                     *
                     */

		/* SPAM (remove for now...)

                     if (symtab_ptr->st_type!=global_ptr->gl_type) {
                        error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                             "Duplicate variable declaration => %s",
                             (symtab_ptr->st_namtab_ptr)->nt_name);
                     }
		*/
		     if ((global_ptr->gl_global==YES)&&
			 (symtab_ptr->st_type==sym_id)) {

                    /* 
                     * 
                     * The global variable had been declared in a
                     * previous run. Reuse the old variable 
                     * 
                     */
                        symtab_ptr->st_unit_num=2;
	   	        symtab_ptr->st_offset=global_ptr->gl_number;
		     }
                  } 
                  global_ptr=global_ptr->gl_next_ptr; 
               }

            }
         }

         global_ptr = GLOBAL_HEAD;

        /*  
         *
         * Add to the symbol table the variables that have been
         * declared previously, but have not been declared now!
         *
         */

         while ((global_ptr != NULL)&&
              (global_ptr->gl_present==NO) &&
              (global_ptr->gl_global==YES)) {

            symtab_ptr=enter_symbol(SETL_SYSTEM
                                    get_namtab(SETL_SYSTEM global_ptr->gl_name),
                                    proctab_ptr, NULL);	
            symtab_ptr->st_type = sym_id; 
            symtab_ptr->st_has_lvalue = YES;
            symtab_ptr->st_has_rvalue = YES;
            symtab_ptr->st_unit_num=2;
            symtab_ptr->st_offset=global_ptr->gl_number;

            global_ptr=global_ptr->gl_next_ptr; 

         }


      }
#endif

/* 
 *
 *  Syntax extension 
 *
 */

      if ((proctab_ptr->pr_type == pr_program)||  
         (proctab_ptr->pr_type == pr_package_body))   {
         assign_ptr = get_ast(SETL_SYSTEM_VOID);
         assign_ptr->ast_type = ast_call;

         namtab_ptr=get_namtab(SETL_SYSTEM "$PASS_SYMTAB");
         symtab_ptr=namtab_ptr->nt_symtab_ptr;

         var_ptr = get_ast(SETL_SYSTEM_VOID);
         var_ptr->ast_type=ast_symtab;
         var_ptr->ast_child.ast_symtab_ptr=symtab_ptr;

         assign_ptr->ast_child.ast_child_ast = var_ptr;

         list_ptr=get_ast(SETL_SYSTEM_VOID);
         list_ptr->ast_type=ast_list;
         list_ptr->ast_next=NULL;
         var_ptr->ast_next = list_ptr;

         sym_ptr=get_ast(SETL_SYSTEM_VOID);
         list_ptr->ast_child.ast_child_ast=sym_ptr;
         sym_ptr->ast_type=ast_symtab;
         sym_ptr->ast_child.ast_symtab_ptr=sym_nullset;
         sym_ptr->ast_next=NULL;


      notnullmap=NO;
      proc_symbol=NULL;
      for (symtab_ptr = proctab_ptr->pr_symtab_head;
        symtab_ptr != NULL;
        symtab_ptr = symtab_ptr->st_thread) {

           if  ((symtab_ptr->st_type == sym_procedure)&&
                (symtab_ptr->st_namtab_ptr !=NULL)) {
	 tmp=(symtab_ptr->st_namtab_ptr)->nt_name;
         if ((tmp!=NULL) && (symtab_ptr->st_unit_num!=-1) &&
              (strncmp(tmp,"$ERR_EXT",8)==0) ) {

              if (notnullmap==NO) {
                notnullmap=YES;	
                sym_ptr->ast_type=ast_enum_set;
	        sym_ptr->ast_child.ast_symtab_ptr=NULL;
              }
         enum_tup_ptr = get_ast(SETL_SYSTEM_VOID);
         enum_tup_ptr->ast_type=ast_enum_tup;
         enum_tup_ptr->ast_next=sym_ptr->ast_child.ast_child_ast;
         sym_ptr->ast_child.ast_child_ast=enum_tup_ptr;

         enum_tup_str_ptr = get_ast(SETL_SYSTEM_VOID);
         enum_tup_proc_ptr = get_ast(SETL_SYSTEM_VOID);

         enum_tup_ptr->ast_child.ast_child_ast=enum_tup_str_ptr;

         enum_tup_str_ptr->ast_next=enum_tup_proc_ptr;
         enum_tup_proc_ptr->ast_next=NULL;
         
         proc_symbol=malloc(strlen(tmp)+3);
         sprintf(proc_symbol,"\"%s\"",tmp);

         namtab_ptr = get_namtab(SETL_SYSTEM proc_symbol);

   /* if we didn't find it, build a literal item */

   if (namtab_ptr->nt_symtab_ptr == NULL) {

      namtab_ptr->nt_token_class = tok_literal;
      namtab_ptr->nt_token_subclass = tok_string;
      symtab_ptr2 = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                proctab_ptr,
                                0);

      symtab_ptr2->st_type = sym_string;
      symtab_ptr2->st_has_rvalue = YES;
      symtab_ptr2->st_is_initialized = YES;
      symtab_ptr2->st_aux.st_string_ptr = 
             char_to_string(SETL_SYSTEM proc_symbol);
 
      free(proc_symbol);

   } else symtab_ptr2=namtab_ptr->nt_symtab_ptr;




         enum_tup_str_ptr->ast_type=ast_symtab;
         enum_tup_str_ptr->ast_child.ast_symtab_ptr=symtab_ptr2;

         enum_tup_proc_ptr->ast_type=ast_symtab;
         enum_tup_proc_ptr->ast_child.ast_symtab_ptr=symtab_ptr;

	    }

	   }
         }
        

         assign_ptr->ast_next = ast_root->ast_child.ast_child_ast;
         ast_root->ast_child.ast_child_ast=assign_ptr;
         store_ast(SETL_SYSTEM &(proctab_ptr->pr_init_code),ast_root);
      }

#ifdef DEBUG

      /* print the symbol table and initialization AST */

      if (AST_DEBUG || SYM_DEBUG) {

         fprintf(DEBUG_FILE,"\n%s : %s\n",
                 (proctab_ptr->pr_namtab_ptr)->nt_name,
                 proctab_desc[proctab_ptr->pr_type]);

         if (SYM_DEBUG)
            print_symtab(SETL_SYSTEM proctab_ptr);

         if (AST_DEBUG) {

            print_ast(SETL_SYSTEM ast_root,"Initialization Tree");
            fputs("\n",DEBUG_FILE);

         }
      }

#endif

      if (!(FILE_ERROR_COUNT + UNIT_ERROR_COUNT)) {

         /* generate initialization code */

         next_temp = NULL;
         next_label = 0;
         open_emit(SETL_SYSTEM &(proctab_ptr->pr_init_code));
         gen_statement(SETL_SYSTEM ast_root);
         close_emit(SETL_SYSTEM_VOID);
         kill_ast(ast_root);

      }

      /* process slot initialization, if necessary */

      if (proctab_ptr->pr_type == pr_class_spec ||
          proctab_ptr->pr_type == pr_process_spec) {

         /* load the slot initialization ast */

         ast_root = load_ast(SETL_SYSTEM &(proctab_ptr->pr_slot_code));
         curr_proctab_ptr = proctab_ptr;

         /* perform semantic checks */

         check_semantics(SETL_SYSTEM ast_root);

#ifdef DEBUG

         /* print the slot initialization AST */

         if (AST_DEBUG) {

            if (AST_DEBUG) {

               print_ast(SETL_SYSTEM ast_root,"Slot Initialization Tree");
               fputs("\n",DEBUG_FILE);

            }
         }

#endif

         if (!(FILE_ERROR_COUNT + UNIT_ERROR_COUNT)) {

            /* generate initialization code */

            next_temp = NULL;
            next_label = 0;
            open_emit(SETL_SYSTEM &(proctab_ptr->pr_slot_code));
            gen_statement(SETL_SYSTEM ast_root);
            close_emit(SETL_SYSTEM_VOID);
            kill_ast(ast_root);

         }
      }

      /*
       *  we call this procedure recursively for children BEFORE
       *  processing the current procedure, so that implicit variable
       *  declarations are made in all scopes where an undeclared
       *  variable is referenced
       */

      if (proctab_ptr->pr_type != pr_package_spec &&
          proctab_ptr->pr_type != pr_native_package &&
          proctab_ptr->pr_type != pr_class_spec &&
          proctab_ptr->pr_type != pr_process_spec &&
          !(FILE_ERROR_COUNT + UNIT_ERROR_COUNT)) {

         save_next_temp = next_temp;
         save_next_label = next_label;
         gen_procedure(SETL_SYSTEM proctab_ptr->pr_child);
         next_temp = save_next_temp;
         next_label = save_next_label;
         curr_proctab_ptr = proctab_ptr;

      }

      /* generate body code */

      if ((proctab_ptr->pr_type == pr_procedure ||
           proctab_ptr->pr_type == pr_method ||
           proctab_ptr->pr_type == pr_program) &&
          !(FILE_ERROR_COUNT + UNIT_ERROR_COUNT)) {

         ast_root = load_ast(SETL_SYSTEM &(proctab_ptr->pr_body_code));

         /* perform semantic checks */

         check_semantics(SETL_SYSTEM ast_root);

#ifdef DEBUG

         if (AST_DEBUG) {

            print_ast(SETL_SYSTEM ast_root,"Body Tree");
            fputs("\n",DEBUG_FILE);

         }

#endif

         if (!(FILE_ERROR_COUNT + UNIT_ERROR_COUNT)) {

            open_emit(SETL_SYSTEM &(proctab_ptr->pr_body_code));
            gen_statement(SETL_SYSTEM ast_root);

            /* procedures need exit code, programs don't */

            if (proctab_ptr->pr_type == pr_procedure ||
                proctab_ptr->pr_type == pr_method) {

               /* push write parameters */

               opnd_num = 0;
               for (formal_ptr = curr_proctab_ptr->pr_symtab_head,
                       formal_num = 0;
                    formal_num < curr_proctab_ptr->pr_formal_count;
                    formal_ptr = formal_ptr->st_thread, formal_num++) {

                  /* we push at most three arguments per instruction */

                  if (opnd_num == 3) {

                     emit(q_push3,
                          operand[0],
                          operand[1],
                          operand[2],
                          &(ast_root->ast_file_pos));

                     opnd_num = 0;

                  }

                  if (formal_ptr->st_is_wparam) {

                     operand[opnd_num] = formal_ptr;
                     opnd_num++;

                  }
               }

               /* push whatever arguments we've accumulated */

               if (opnd_num == 1) {
                  emit(q_push1,
                       operand[0],
                       NULL,
                       NULL,
                       &(ast_root->ast_file_pos));
               }
               else if (opnd_num == 2) {
                  emit(q_push2,
                       operand[0],
                       operand[1],
                       NULL,
                       &(ast_root->ast_file_pos));
               }
               else if (opnd_num == 3) {
                  emit(q_push3,
                       operand[0],
                       operand[1],
                       operand[2],
                       &(ast_root->ast_file_pos));
               }

               emit(q_return,
                    sym_omega,
                    NULL,NULL,&(ast_root->ast_file_pos));

            }

            emit(q_stop,
                 NULL,NULL,NULL,&(ast_root->ast_file_pos));

            close_emit(SETL_SYSTEM_VOID);
            kill_ast(ast_root);

         }
      }

      proctab_ptr->pr_label_count = next_label;

      detach_symtab(proctab_ptr->pr_symtab_head);

      /* set up for next procedure */

      if (proctab_ptr->pr_type == pr_procedure ||
          proctab_ptr->pr_type == pr_method)
         proctab_ptr = proctab_ptr->pr_next;
      else
         proctab_ptr = NULL;

   }
}

/*\
 *  \function{setup\_symtab()}
 *
 *  This somewhat nasty procedure will set up the name and symbol tables
 *  in preparation for code generation for a unit.  For procedures and
 *  methods this is simple, we just re-attach the symbol table to the
 *  name table.  For compilation units we also have to load imported and
 *  inherited units, and possibly the specification corresponding to the
 *  unit body.
\*/

static void setup_symtab(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* procedure to be loaded            */

{
symtab_ptr_type save_symtab_head;      /* saved symbol table list head      */
symtab_ptr_type *save_symtab_tail;     /* saved symbol table list tail      */
symtab_ptr_type test_sym_ptr;          /* test for hidden names             */
symtab_ptr_type symtab_ptr;            /* used to attach symbols            */
symtab_ptr_type clash_ptr;             /* name table symbol clash ptr       */
symtab_ptr_type active_ptr;            /* visible slot with given name      */
int last_slot_num;                     /* last slot number used             */

   /* save the current symbol table head and tail */

   save_symtab_head = proctab_ptr->pr_symtab_head;
   save_symtab_tail = proctab_ptr->pr_symtab_tail;
   proctab_ptr->pr_symtab_head = NULL;
   proctab_ptr->pr_symtab_tail = &(proctab_ptr->pr_symtab_head);

   /* load the superclasses from the class specification (if body) */

   if (proctab_ptr->pr_type == pr_class_body) {

      load_superclass_list(SETL_SYSTEM proctab_ptr);

   }

   /* get units brought in with an 'inherit' clause */

   load_superclasses(SETL_SYSTEM proctab_ptr,proctab_ptr,YES);

   /* get units brought in with a 'use' clause */

   load_used_units(SETL_SYSTEM proctab_ptr);

   /* reattach locally declared symbols */

   for (symtab_ptr = save_symtab_head;
        symtab_ptr != NULL;
        symtab_ptr = symtab_ptr->st_thread) {

      if (symtab_ptr->st_namtab_ptr == NULL)
         continue;

      /* check for slot name conflicts */

      if (proctab_ptr->pr_type == pr_class_spec ||
          proctab_ptr->pr_type == pr_class_body ||
          proctab_ptr->pr_type == pr_process_spec ||
          proctab_ptr->pr_type == pr_process_body) {

         for (test_sym_ptr = (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr;
              test_sym_ptr != NULL;
              test_sym_ptr = test_sym_ptr->st_name_link) {

            if (test_sym_ptr->st_class != proctab_ptr)
               continue;

            /* check for name conflicts */

            if (symtab_ptr->st_type != test_sym_ptr->st_type) {

               error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                             "Name conflict: %s %s and %s %s",
                             symtab_desc[symtab_ptr->st_type],
                             (symtab_ptr->st_namtab_ptr)->nt_name,
                             symtab_desc[test_sym_ptr->st_type],
                             (test_sym_ptr->st_namtab_ptr)->nt_name);

               continue;

            }

            if (symtab_ptr->st_type == sym_slot ||
                symtab_ptr->st_type == sym_id) {

               error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                             "Duplicate variable declaration => %s",
                             (symtab_ptr->st_namtab_ptr)->nt_name);

               continue;

            }
         }
      }

      /* push the symbol on the appropriate name list */

      symtab_ptr->st_name_link =
           (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr;
      (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr = symtab_ptr;

      symtab_ptr->st_is_name_attached = YES;

   }

   /* get the specifications of unit bodies */

   if (proctab_ptr->pr_type == pr_package_body) {

      load_package_spec(SETL_SYSTEM proctab_ptr);

   }
   else if (proctab_ptr->pr_type == pr_class_body ||
            proctab_ptr->pr_type == pr_process_body) {

      load_class_spec(SETL_SYSTEM proctab_ptr);

   }

   /* reattach locally declared identifiers */

   if (proctab_ptr->pr_symtab_head == NULL) {

      proctab_ptr->pr_symtab_head = save_symtab_head;
      proctab_ptr->pr_symtab_tail = save_symtab_tail;

   }
   else if (save_symtab_head != NULL) {

      *save_symtab_tail = proctab_ptr->pr_symtab_head;
      proctab_ptr->pr_symtab_head = save_symtab_head;

   }

   /*
    *  This next mess has two goals: to give each instance variable and
    *  method a unique identifying number within this compilation unit,
    *  and to flag the most-visible instance variable and method with
    *  each name.  It is somewhat ugly, but remember that name clash
    *  lists will be very short, so a direct, if inelegant solution is
    *  likely to be fastest.
    */

   /* clear all the slots to unnumbered */

   for (symtab_ptr = proctab_ptr->pr_symtab_head;
        symtab_ptr != NULL;
        symtab_ptr = symtab_ptr->st_thread) {

      symtab_ptr->st_is_visible_slot = NO;

      if (symtab_ptr->st_slot_num < m_user)
         continue;

      symtab_ptr->st_slot_num = m_user;

   }

   /* set up to number the slots */

   last_slot_num = m_user+1;
   for (symtab_ptr = proctab_ptr->pr_symtab_head;
        symtab_ptr != NULL;
        symtab_ptr = symtab_ptr->st_thread) {

      /* In case of dynamic compilation, identify the global variables */

#ifdef DYNAMIC_COMP
      if ((COMPILING_EVAL==YES)&&(proctab_ptr->pr_type == pr_program) &&
          (symtab_ptr->st_unit_num < 0) &&
             ((symtab_ptr->st_type == sym_id) ||
              (symtab_ptr->st_type == sym_procedure)) ) {

         symtab_ptr->st_global_var=YES;

      }
#endif

      if (symtab_ptr->st_type != sym_slot &&
          symtab_ptr->st_type != sym_method)
         continue;

      /* number this slot, if it isn't already */

      if (symtab_ptr->st_slot_num == m_user)
         symtab_ptr->st_slot_num = last_slot_num++;

      /* check slots with the same name */

      active_ptr = (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr;
      for (clash_ptr = (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr;
           clash_ptr != NULL;
           clash_ptr = clash_ptr->st_name_link) {

         if (clash_ptr->st_type != sym_slot &&
             clash_ptr->st_type != sym_method)
            continue;

         clash_ptr->st_slot_num = symtab_ptr->st_slot_num;

         /* override anything not a slot or method */

         if (active_ptr->st_type != sym_slot &&
             active_ptr->st_type != sym_method) {

            active_ptr = clash_ptr;
            continue;

         }

         /* override anything not in this class */

         if (active_ptr->st_class != proctab_ptr &&
             clash_ptr->st_class == proctab_ptr) {

            active_ptr = clash_ptr;
            continue;

         }
      }

      active_ptr->st_is_visible_slot = YES;

   }
}

/*\
 *  \function{load\_superclass\_list()}
 *
 *  Inherit clauses are in the class specification, not the body, because
 *  constant initialization can refer to values in other classes.  We can
 *  not have cycles in inheritance graphs.  This function loads the list
 *  of inherited classes from the class specification.
\*/

static void load_superclass_list(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* package to be loaded              */

{
libunit_ptr_type libunit_ptr;          /* library unit                      */
libstr_ptr_type libstr_ptr;            /* symbol table stream               */
unit_control_record unit_control;
                                       /* unit control record               */
import_record import;                  /* imported package record           */
import_ptr_type *import_ptr;           /* used to loop over import list     */
symtab_ptr_type symtab_ptr;            /* inherited class symbol            */
int32 i;                               /* temporary looping variable        */

   /* open the class specification */

   if ((libunit_ptr = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                   I2_FILE,LIB_READ_UNIT)) == NULL &&
       (libunit_ptr = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                   DEFAULT_LIBFILE,LIB_READ_UNIT)) == NULL) {

      error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                    msg_no_spec,
                    (proctab_ptr->pr_namtab_ptr)->nt_name);

      return;

   }

   /* read the unit control record */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
   read_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
                sizeof(unit_control_record));
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* the unit had better be a class */

   if (unit_control.uc_type != CLASS_UNIT) {

      error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                    "Expected %s to be a class specification",
                    (proctab_ptr->pr_namtab_ptr)->nt_name);

      return;

   }

   /* open the symbol table stream */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INHERIT_STREAM);

   /* read through the symbol table stream */

   import_ptr = &(proctab_ptr->pr_inherit_list);
   for (i = 0; i < unit_control.uc_inherit_count; i++) {

      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&import,
                  sizeof(import_record));

      *import_ptr = get_import(SETL_SYSTEM_VOID);
      (*import_ptr)->im_namtab_ptr = get_namtab(SETL_SYSTEM import.ir_name);
      (*import_ptr)->im_inherited = YES;

      /* install the class name */

      symtab_ptr = enter_symbol(SETL_SYSTEM
                                (*import_ptr)->im_namtab_ptr,
                                proctab_ptr,
                                NULL);

      if (symtab_ptr != NULL) {

         symtab_ptr->st_type = sym_inherit;
         symtab_ptr->st_aux.st_import_ptr = *import_ptr;
         (*import_ptr)->im_symtab_ptr = symtab_ptr;

      }

      strcpy((*import_ptr)->im_source_name,import.ir_source_name);
      (*import_ptr)->im_time_stamp = import.ir_time_stamp;
      import_ptr = &((*import_ptr)->im_next);

   }

   close_libstr(SETL_SYSTEM libstr_ptr);
   close_libunit(SETL_SYSTEM libunit_ptr);

}

/*\
 *  \function{load\_used\_units()}
 *
 *  This function loads the specifications of used packages and classes.
 *  There are a few things to notice here.  First, if the imported unit
 *  is a class, we must recursively load any superclasses.  Second, since
 *  we load imported units before locally declared variables, the local
 *  variables hide those in imported units.
\*/

static void load_used_units(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* unit to be loaded                 */

{
import_ptr_type import_ptr;            /* used to loop over units           */
import_ptr_type *import_tail;          /* attach new unit here              */
libunit_ptr_type libunit_ptr;          /* library unit                      */
libstr_ptr_type libstr_ptr;            /* symbol table stream               */
unit_control_record unit_control;      /* unit control record               */
import_record import;                  /* imported package record           */
symtab_record symtab;                  /* symbol table record               */
char name_buffer[MAX_TOK_LEN + 1];     /* identifier name buffer            */
int symbol_count;                      /* used to count the input symbols   */
namtab_ptr_type namtab_ptr;            /* name table pointer for symbol     */
symtab_ptr_type symtab_ptr;            /* entered or existing symbol table  */
                                       /* pointer                           */
int formal_count;                      /* number of formals in a procedure  */
proctab_ptr_type package_proc;         /* package's procedure record        */
proctab_ptr_type curr_proc;            /* current procedure                 */
int selector_length;                   /* length of selector name           */
symtab_ptr_type selector_ptr;          /* pointer to selector key           */
symtab_ptr_type test_sym_ptr;          /* test for hidden names             */
proctab_ptr_type test_proc_ptr;        /* test for hidden names             */
int32 i;                               /* temporary looping variable        */

   /* loop over the package names */

   for (import_ptr = proctab_ptr->pr_import_list;
        import_ptr != NULL;
        import_ptr = import_ptr->im_next) {

      /* create a dummy procedure table item for the unit */

      package_proc = get_proctab(SETL_SYSTEM_VOID);
      package_proc->pr_parent = proctab_ptr;
      package_proc->pr_namtab_ptr =
         (import_ptr->im_symtab_ptr)->st_namtab_ptr;
      import_ptr->im_unit_num = ++(unit_proctab_ptr->pr_unit_count);
      (import_ptr->im_symtab_ptr)->st_unit_num = import_ptr->im_unit_num;
      (import_ptr->im_symtab_ptr)->st_aux.st_proctab_ptr = package_proc;

      /* open the unit */

      if ((libunit_ptr = open_libunit(SETL_SYSTEM (import_ptr->im_namtab_ptr)->nt_name,
                                      NULL,LIB_READ_UNIT)) == NULL) {

         error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_missing_package,
                       (import_ptr->im_namtab_ptr)->nt_name);

         continue;

      }

      /* read the unit control record */

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
                  sizeof(unit_control_record));
      close_libstr(SETL_SYSTEM libstr_ptr);

      /* if we found a class, load superclasses */

      if (unit_control.uc_type == CLASS_UNIT) {

         package_proc->pr_type = pr_class_body;
         (import_ptr->im_symtab_ptr)->st_type = sym_class;

         libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INHERIT_STREAM);

         import_tail = &(package_proc->pr_inherit_list);
         for (i = 0; i < unit_control.uc_inherit_count; i++) {

            read_libstr(SETL_SYSTEM libstr_ptr,(char *)&import,
                        sizeof(import_record));

            *import_tail = get_import(SETL_SYSTEM_VOID);
            (*import_tail)->im_namtab_ptr = get_namtab(SETL_SYSTEM import.ir_name);
            (*import_tail)->im_inherited = YES;

            /* install the class name */

            symtab_ptr = enter_symbol(SETL_SYSTEM
                                      (*import_tail)->im_namtab_ptr,
                                      package_proc,
                                      NULL);

            if (symtab_ptr != NULL) {

               symtab_ptr->st_type = sym_inherit;
               symtab_ptr->st_aux.st_import_ptr = *import_tail;
               (*import_tail)->im_symtab_ptr = symtab_ptr;

            }

            strcpy((*import_tail)->im_source_name,import.ir_source_name);
            (*import_tail)->im_time_stamp = import.ir_time_stamp;
            import_tail = &((*import_tail)->im_next);

         }

         close_libstr(SETL_SYSTEM libstr_ptr);

         /* load symbols from the superclasses */

         load_superclasses(SETL_SYSTEM package_proc,package_proc,NO);

      }

      else if (unit_control.uc_type == PROCESS_UNIT) {

         package_proc->pr_type = pr_process_body;
         (import_ptr->im_symtab_ptr)->st_type = sym_process;

      }

      else if ((unit_control.uc_type == PACKAGE_UNIT)||
               (unit_control.uc_type == NATIVE_UNIT)) {

         package_proc->pr_type = pr_package_body;
         (import_ptr->im_symtab_ptr)->st_type = sym_package;

      }
      else {

         error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       "Can not import program %s, only packages and classes",
                       (import_ptr->im_namtab_ptr)->nt_name);

         continue;

      }

      /* update the imported package table */

      strcpy(import_ptr->im_source_name,unit_control.uc_spec_source_name);
      import_ptr->im_time_stamp = unit_control.uc_time_stamp;

      /* open the symbol table stream */

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_SYMTAB_STREAM);

      /* read through the symbol table stream */

      curr_proc = package_proc;
      formal_count = 0;
      for (symbol_count = 0;
           symbol_count < unit_control.uc_symtab_count;
           symbol_count++) {

         /* read the symbol table record and the name string */

         read_libstr(SETL_SYSTEM libstr_ptr,
                     (char *)&symtab,
                     sizeof(symtab_record));

         read_libstr(SETL_SYSTEM libstr_ptr,
                     name_buffer,
                     symtab.sr_name_length);
         name_buffer[symtab.sr_name_length] = '\0';

         /* only load public symbols */

         if (!symtab.sr_symtab_item.st_is_public && formal_count == 0)
            continue;

         /* find the corresponding name table entry */

         namtab_ptr = get_namtab(SETL_SYSTEM name_buffer);
         symtab_ptr = get_symtab(SETL_SYSTEM_VOID);
         memcpy((void *)symtab_ptr,
                (void *)&symtab.sr_symtab_item,
                sizeof(struct symtab_item));
         symtab_ptr->st_namtab_ptr = namtab_ptr;

         /* insert the symbol in the appropriate procedure */

         symtab_ptr->st_thread = NULL;
         *(curr_proc->pr_symtab_tail) = symtab_ptr;
         curr_proc->pr_symtab_tail = &(symtab_ptr->st_thread);
         symtab_ptr->st_in_spec = NO;
         symtab_ptr->st_owner_proc = curr_proc;
         symtab_ptr->st_unit_num = import_ptr->im_unit_num;
         if (curr_proc->pr_type == pr_class_body ||
             curr_proc->pr_type == pr_process_body)
            symtab_ptr->st_class = curr_proc;

         /* reset procedure pointer, if we finished a formal list */

         if (formal_count > 0) {
            if (!(--formal_count)) {
               curr_proc = package_proc;
            }
            continue;
         }

         /* hide duplicate names */

         for (test_sym_ptr = (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr;
              test_sym_ptr != NULL;
              test_sym_ptr = test_sym_ptr->st_name_link) {

            /* once the symbol in question is hidden, so are successors */

            if (symtab_ptr->st_is_hidden) {

               test_sym_ptr->st_is_hidden = YES;
               symtab_ptr->st_is_hidden = YES;
               continue;

            }

            /* predefined symbols don't hide, but can be hidden */

            if (test_sym_ptr->st_owner_proc == predef_proctab_ptr)
               continue;

            /* slots and methods don't hide each other */

            if ((symtab_ptr->st_type == sym_slot ||
                   symtab_ptr->st_type == sym_method) &&
                (test_sym_ptr->st_type == sym_slot ||
                   test_sym_ptr->st_type == sym_method))
               continue;

            /* see if duplicate names are children */

            for (test_proc_ptr = test_sym_ptr->st_owner_proc;
                 test_proc_ptr != NULL && test_proc_ptr != package_proc;
                 test_proc_ptr = test_proc_ptr->pr_parent);

            if (test_proc_ptr == NULL) {

               test_sym_ptr->st_is_hidden = YES;
               symtab_ptr->st_is_hidden = YES;

            }
         }

         /* push the symbol on the appropriate name list */

         symtab_ptr->st_name_link =
              (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr;
         (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr = symtab_ptr;
         symtab_ptr->st_is_name_attached = YES;

         /* if the symbol in the library is a procedure ... */

         if (symtab.sr_symtab_item.st_type == sym_procedure ||
             symtab.sr_symtab_item.st_type == sym_method) {

            /* create a dummy procedure table item */

            curr_proc = get_proctab(SETL_SYSTEM_VOID);
            symtab_ptr->st_aux.st_proctab_ptr = curr_proc;
            curr_proc->pr_formal_count = symtab.sr_param_count;
            curr_proc->pr_namtab_ptr = namtab_ptr;
            curr_proc->pr_parent = package_proc;

            /* we have to read formal_count symbols */

            formal_count = symtab.sr_param_count;
            if (formal_count == 0)
               curr_proc = package_proc;

            continue;

         }

         /* if the symbol in the library is a selector ... */

         if (symtab.sr_symtab_item.st_type == sym_selector) {

            /* read in the selector key */

            read_libstr(SETL_SYSTEM libstr_ptr,
                        (char *)&selector_length,
                        sizeof(int));

            read_libstr(SETL_SYSTEM libstr_ptr,
                        name_buffer,
                        selector_length);
            name_buffer[selector_length] = '\0';

            /* look up the lexeme in the name table */

            namtab_ptr = get_namtab(SETL_SYSTEM name_buffer);

            /* if we don't find it, make a literal item */

            if (namtab_ptr->nt_symtab_ptr == NULL) {

               namtab_ptr->nt_token_class = tok_literal;
               namtab_ptr->nt_token_subclass = tok_integer;
               selector_ptr = enter_symbol(SETL_SYSTEM
                                           namtab_ptr,
                                           package_proc,
                                           NULL);

               selector_ptr->st_has_rvalue = YES;
               selector_ptr->st_is_initialized = YES;
               selector_ptr->st_type = sym_integer;
               selector_ptr->st_aux.st_integer_ptr =
                  char_to_int(SETL_SYSTEM name_buffer);

            }
            else {

               selector_ptr = namtab_ptr->nt_symtab_ptr;

            }

            symtab_ptr->st_aux.st_selector_ptr = selector_ptr;

         }
      }

      close_libstr(SETL_SYSTEM libstr_ptr);
      close_libunit(SETL_SYSTEM libunit_ptr);

      /* move the symbols to the base procedure */

      *(proctab_ptr->pr_symtab_tail) = package_proc->pr_symtab_head;
      if (package_proc->pr_symtab_head != NULL)
         proctab_ptr->pr_symtab_tail = package_proc->pr_symtab_tail;

      package_proc->pr_symtab_head = NULL;
      package_proc->pr_symtab_tail = &(package_proc->pr_symtab_head);

   }
}

/*\
 *  \function{load\_superclasses()}
 *
 *  This function loads the specifications of inherited packages.
 *  We do not allow name conflicts here.
\*/

static void load_superclasses(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr,       /* unit to be loaded                 */
   proctab_ptr_type class_ptr,         /* class being loaded                */
   int inherit)                        /* YES if class is inherited         */

{
import_ptr_type import_ptr;            /* used to loop over units           */
import_ptr_type *import_tail;          /* attach new unit here              */
libunit_ptr_type libunit_ptr;          /* library unit                      */
libstr_ptr_type libstr_ptr;            /* symbol table stream               */
unit_control_record unit_control;      /* unit control record               */
import_record import;                  /* imported package record           */
symtab_record symtab;                  /* symbol table record               */
char name_buffer[MAX_TOK_LEN + 1];     /* identifier name buffer            */
int symbol_count;                      /* used to count the input symbols   */
namtab_ptr_type namtab_ptr;            /* name table pointer for symbol     */
symtab_ptr_type symtab_ptr;            /* entered or existing symbol table  */
                                       /* pointer                           */
int formal_count;                      /* number of formals in a procedure  */
proctab_ptr_type package_proc;         /* package's procedure record        */
proctab_ptr_type curr_proc;            /* current procedure                 */
int selector_length;                   /* length of selector name           */
symtab_ptr_type selector_ptr;          /* pointer to selector key           */
int32 i;                               /* temporary looping variable        */
symtab_ptr_type test_sym_ptr;          /* test for hidden names             */
proctab_ptr_type test_proc_ptr;        /* test for hidden names             */

   /* loop over the package names */

   for (import_ptr = proctab_ptr->pr_inherit_list;
        import_ptr != NULL;
        import_ptr = import_ptr->im_next) {

      /* create a dummy procedure table item for the unit */

      package_proc = get_proctab(SETL_SYSTEM_VOID);
      package_proc->pr_parent = proctab_ptr;
      package_proc->pr_namtab_ptr =
         (import_ptr->im_symtab_ptr)->st_namtab_ptr;
      import_ptr->im_unit_num = ++(unit_proctab_ptr->pr_unit_count);
      (import_ptr->im_symtab_ptr)->st_unit_num = import_ptr->im_unit_num;
      (import_ptr->im_symtab_ptr)->st_aux.st_proctab_ptr = package_proc;

      /* open the unit */

      if ((libunit_ptr = open_libunit(SETL_SYSTEM (import_ptr->im_namtab_ptr)->nt_name,
                                      NULL,LIB_READ_UNIT)) == NULL) {

         error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       msg_missing_package,
                       (import_ptr->im_namtab_ptr)->nt_name);

         continue;

      }

      /* read the unit control record */

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
      read_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
                  sizeof(unit_control_record));
      close_libstr(SETL_SYSTEM libstr_ptr);

      /* if we found a class, load superclasses */

      if (unit_control.uc_type == CLASS_UNIT) {

         package_proc->pr_type = pr_class_body;

         libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INHERIT_STREAM);

         import_tail = &(package_proc->pr_inherit_list);
         for (i = 0; i < unit_control.uc_inherit_count; i++) {

            read_libstr(SETL_SYSTEM libstr_ptr,(char *)&import,
                        sizeof(import_record));

            *import_tail = get_import(SETL_SYSTEM_VOID);
            (*import_tail)->im_namtab_ptr = get_namtab(SETL_SYSTEM import.ir_name);
            (*import_tail)->im_inherited = YES;

            /* install the class name */

            symtab_ptr = enter_symbol(SETL_SYSTEM
                                      (*import_tail)->im_namtab_ptr,
                                      package_proc,
                                      NULL);

            if (symtab_ptr != NULL) {

               symtab_ptr->st_type = sym_inherit;
               symtab_ptr->st_aux.st_import_ptr = *import_tail;
               (*import_tail)->im_symtab_ptr = symtab_ptr;

            }

            strcpy((*import_tail)->im_source_name,import.ir_source_name);
            (*import_tail)->im_time_stamp = import.ir_time_stamp;
            import_tail = &((*import_tail)->im_next);

         }

         close_libstr(SETL_SYSTEM libstr_ptr);

         /* load symbols from the superclasses */

         load_superclasses(SETL_SYSTEM package_proc,class_ptr,inherit);

      }
      else {

         error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                       "Can not inherit %s, only classes",
                       (import_ptr->im_namtab_ptr)->nt_name);

         continue;

      }

      /* check the compilation date / time */

      if (import_ptr->im_time_stamp == -1) {

         strcpy(import_ptr->im_source_name,unit_control.uc_spec_source_name);
         import_ptr->im_time_stamp = unit_control.uc_time_stamp;

      }
      else {

         if (strcmp(import_ptr->im_source_name,
                    unit_control.uc_spec_source_name) != 0 ||
             import_ptr->im_time_stamp != unit_control.uc_time_stamp) {

            error_message(SETL_SYSTEM NULL,
                          "Class %s needs recompiled",
                          (import_ptr->im_namtab_ptr)->nt_name);

         }
      }

      /* open the symbol table stream */

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_SYMTAB_STREAM);

      /* read through the symbol table stream */

      curr_proc = package_proc;
      formal_count = 0;
      for (symbol_count = 0;
           symbol_count < unit_control.uc_symtab_count;
           symbol_count++) {

         /* read the symbol table record and the name string */

         read_libstr(SETL_SYSTEM libstr_ptr,
                     (char *)&symtab,
                     sizeof(symtab_record));

         read_libstr(SETL_SYSTEM libstr_ptr,
                     name_buffer,
                     symtab.sr_name_length);
         name_buffer[symtab.sr_name_length] = '\0';

         /* only load public symbols if not inheriting */

         if (!inherit &&
             !symtab.sr_symtab_item.st_is_public &&
             formal_count == 0)
               continue;

         if (symtab.sr_symtab_item.st_is_temp)
            continue;

         /* find the corresponding name table entry */

         namtab_ptr = get_namtab(SETL_SYSTEM name_buffer);
         symtab_ptr = get_symtab(SETL_SYSTEM_VOID);
         memcpy((void *)symtab_ptr,
                (void *)&symtab.sr_symtab_item,
                sizeof(struct symtab_item));
         symtab_ptr->st_namtab_ptr = namtab_ptr;

         /* insert the symbol in the appropriate procedure */

         symtab_ptr->st_thread = NULL;
         *(curr_proc->pr_symtab_tail) = symtab_ptr;
         curr_proc->pr_symtab_tail = &(symtab_ptr->st_thread);
         symtab_ptr->st_in_spec = NO;
         symtab_ptr->st_owner_proc = curr_proc;
         symtab_ptr->st_unit_num = import_ptr->im_unit_num;
         symtab_ptr->st_class = class_ptr;

         /* reset procedure pointer, if we finished a formal list */

         if (formal_count > 0) {
            if (!(--formal_count)) {
               curr_proc = package_proc;
            }
            continue;
         }

         /* hide duplicate names */

         for (test_sym_ptr = (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr;
              test_sym_ptr != NULL;
              test_sym_ptr = test_sym_ptr->st_name_link) {

            /* see if duplicate names are children */

            for (test_proc_ptr = test_sym_ptr->st_owner_proc;
                 test_proc_ptr != NULL && test_proc_ptr != package_proc;
                 test_proc_ptr = test_proc_ptr->pr_parent);

            /* don't allow name conflicts in classes */

            if (inherit && test_proc_ptr != NULL) {

               if (symtab_ptr->st_type != test_sym_ptr->st_type) {

                  error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                                "Name conflict: %s %s and %s %s",
                                symtab_desc[symtab_ptr->st_type],
                                (symtab_ptr->st_namtab_ptr)->nt_name,
                                symtab_desc[test_sym_ptr->st_type],
                                (test_sym_ptr->st_namtab_ptr)->nt_name);

                  continue;

               }

               if (symtab_ptr->st_type == sym_slot ||
                   symtab_ptr->st_type == sym_id) {

                  error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                                "Duplicate variable declaration => %s",
                                (symtab_ptr->st_namtab_ptr)->nt_name);

                  continue;

               }
            }

            /* once the symbol in question is hidden, so are successors */

            if (symtab_ptr->st_is_hidden) {

               test_sym_ptr->st_is_hidden = YES;
               symtab_ptr->st_is_hidden = YES;

            }

            /* predefined symbols don't hide, but can be hidden */

            if (test_sym_ptr->st_owner_proc == predef_proctab_ptr)
               continue;

            /* slots and methods don't hide each other */

            if ((symtab_ptr->st_type == sym_slot ||
                   symtab_ptr->st_type == sym_method) &&
                (test_sym_ptr->st_type == sym_slot ||
                   test_sym_ptr->st_type == sym_method))
               continue;

            /* other conflicts cause hidden names */

            if (test_proc_ptr == NULL) {

               test_sym_ptr->st_is_hidden = YES;
               symtab_ptr->st_is_hidden = YES;

            }
         }

         /* push the symbol on the appropriate name list */

         symtab_ptr->st_name_link =
              (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr;
         (symtab_ptr->st_namtab_ptr)->nt_symtab_ptr = symtab_ptr;
         symtab_ptr->st_is_name_attached = YES;

         /* if the symbol in the library is a procedure ... */

         if (symtab.sr_symtab_item.st_type == sym_procedure ||
             symtab.sr_symtab_item.st_type == sym_method) {

            /* create a dummy procedure table item */

            curr_proc = get_proctab(SETL_SYSTEM_VOID);
            symtab_ptr->st_aux.st_proctab_ptr = curr_proc;
            curr_proc->pr_formal_count = symtab.sr_param_count;
            curr_proc->pr_namtab_ptr = namtab_ptr;
            curr_proc->pr_parent = package_proc;

            /* we have to read formal_count symbols */

            formal_count = symtab.sr_param_count;
            if (formal_count == 0)
               curr_proc = package_proc;

            continue;

         }

         /* if the symbol in the library is a selector ... */

         if (symtab.sr_symtab_item.st_type == sym_selector) {

            /* read in the selector key */

            read_libstr(SETL_SYSTEM libstr_ptr,
                        (char *)&selector_length,
                        sizeof(int));

            read_libstr(SETL_SYSTEM libstr_ptr,
                        name_buffer,
                        selector_length);
            name_buffer[selector_length] = '\0';

            /* look up the lexeme in the name table */

            namtab_ptr = get_namtab(SETL_SYSTEM name_buffer);

            /* if we don't find it, make a literal item */

            if (namtab_ptr->nt_symtab_ptr == NULL) {

               namtab_ptr->nt_token_class = tok_literal;
               namtab_ptr->nt_token_subclass = tok_integer;
               selector_ptr = enter_symbol(SETL_SYSTEM
                                           namtab_ptr,
                                           package_proc,
                                           NULL);

               selector_ptr->st_has_rvalue = YES;
               selector_ptr->st_is_initialized = YES;
               selector_ptr->st_type = sym_integer;
               selector_ptr->st_aux.st_integer_ptr =
                  char_to_int(SETL_SYSTEM name_buffer);

            }
            else {

               selector_ptr = namtab_ptr->nt_symtab_ptr;

            }

            symtab_ptr->st_aux.st_selector_ptr = selector_ptr;

         }
      }

      close_libstr(SETL_SYSTEM libstr_ptr);
      close_libunit(SETL_SYSTEM libunit_ptr);

      /* move the symbols to the base procedure */

      *(proctab_ptr->pr_symtab_tail) = package_proc->pr_symtab_head;
      if (package_proc->pr_symtab_head != NULL)
         proctab_ptr->pr_symtab_tail = package_proc->pr_symtab_tail;

      package_proc->pr_symtab_head = NULL;
      package_proc->pr_symtab_tail = &(package_proc->pr_symtab_head);

   }
}

/*\
 *  \function{load\_package\_spec()}
 *
 *  This function loads a package specification, in preparation for
 *  coding the corresponding package body.  We have two distinct types of
 *  symbols at this point -- procedures and others (variables, constants,
 *  \& selectors).  Since we have already loaded the declared variables
 *  in the package body, we should already have symbol table entries for
 *  the procedure names, and all we do here is verify that they match the
 *  specification in the library.  Other symbols are added to the package
 *  scope.
\*/

static void load_package_spec(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* package to be loaded              */

{
libunit_ptr_type libunit_ptr;          /* library unit                      */
libstr_ptr_type libstr_ptr;            /* symbol table stream               */
unit_control_record unit_control;      /* unit control record               */
symtab_record symtab;                  /* symbol table record               */
char name_buffer[MAX_TOK_LEN + 1];     /* identifier name buffer            */
int symbol_count;                      /* used to count the input symbols   */
namtab_ptr_type namtab_ptr;            /* name table pointer for symbol     */
symtab_ptr_type symtab_ptr;            /* entered or existing symbol table  */
                                       /* pointer                           */
symtab_ptr_type procedure_pointer;     /* symbol table item for procedure   */
int procedure_error;                   /* YES if we find an error in a      */
                                       /* procedure item                    */
int formal_count;                      /* number of formals in a procedure  */
int selector_length;                   /* length of selector name           */
symtab_ptr_type selector_ptr;          /* pointer to selector key           */

   /* open the package specification */

   if ((libunit_ptr = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                   I2_FILE,LIB_READ_UNIT)) == NULL &&
       (libunit_ptr = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                   DEFAULT_LIBFILE,LIB_READ_UNIT)) == NULL) {

      error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                    msg_no_spec,
                    (proctab_ptr->pr_namtab_ptr)->nt_name);

      return;

   }

   /* read the unit control record */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
   read_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
                sizeof(unit_control_record));
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* open the symbol table stream */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_SYMTAB_STREAM);

   /* read through the symbol table stream */

   for (symbol_count = 0;
        symbol_count < unit_control.uc_symtab_count;
        symbol_count++) {

      /* read the symbol table record and the name string */

      read_libstr(SETL_SYSTEM libstr_ptr,
                  (char *)&symtab,
                  sizeof(symtab_record));

      read_libstr(SETL_SYSTEM libstr_ptr,
                  name_buffer,
                  symtab.sr_name_length);

      /* find the corresponding name table entry */

      name_buffer[symtab.sr_name_length] = '\0';
      if (symtab.sr_name_length > 0)
         namtab_ptr = get_namtab(SETL_SYSTEM name_buffer);
      else
         namtab_ptr = NULL;

      /* skip anything not in the specification */

      if (!symtab.sr_symtab_item.st_in_spec)
         continue;

      /* if the symbol in the library is a procedure ... */

      if (symtab.sr_symtab_item.st_type == sym_procedure ||
          symtab.sr_symtab_item.st_type == sym_method) {

         procedure_error = NO;
         symtab_ptr = namtab_ptr->nt_symtab_ptr;

         /* the procedure should be in the symbol table */

         if (symtab_ptr == NULL || symtab_ptr->st_unit_num > 1) {

            error_message(SETL_SYSTEM NULL,
                          msg_missing_proc,
                          name_buffer);
            procedure_error = YES;

         }

         if (!procedure_error &&
             symtab_ptr->st_type != sym_procedure &&
             symtab_ptr->st_type != sym_method) {

            error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                          msg_expected_tok_proc,
                          name_buffer);
            procedure_error = YES;

         }

         /* we have to read past formal_count symbols */

         formal_count = symtab.sr_param_count;
         symbol_count += formal_count;

         /* start the list of formal parameters in the symbol table */

         if (!procedure_error) {

            procedure_pointer = symtab_ptr;
            symtab_ptr->st_unit_num = 1;
            symtab_ptr->st_offset = symtab.sr_symtab_item.st_offset;
            symtab_ptr->st_is_alloced = YES;
            symtab_ptr->st_in_spec = YES;
            symtab_ptr->st_is_public = YES;
            symtab_ptr =
               (procedure_pointer->st_aux.st_proctab_ptr)->pr_symtab_head;

         }

         /* the number of formals must match */

         if (!procedure_error &&
             (procedure_pointer->st_aux.st_proctab_ptr)->pr_formal_count !=
              formal_count) {

            error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                          msg_bad_proc_def,
                          name_buffer);
            procedure_error = YES;

         }

         /* we have to check each formal parameter */

         while (formal_count--) {

            /* read the symbol table record and the name string */

            read_libstr(SETL_SYSTEM libstr_ptr,
                        (char *)&symtab,
                        sizeof(symtab_record));

            read_libstr(SETL_SYSTEM libstr_ptr,
                        name_buffer,
                        symtab.sr_name_length);

            name_buffer[symtab.sr_name_length] = '\0';

            /*
             *  the name of the formal in the library should match the
             *  one in the symbol table
             */

            if (!procedure_error &&
                (strcmp((symtab_ptr->st_namtab_ptr)->nt_name,
                       name_buffer) != 0 ||
                 symtab.sr_symtab_item.st_is_rparam !=
                    symtab_ptr->st_is_rparam ||
                 symtab.sr_symtab_item.st_is_wparam !=
                    symtab_ptr->st_is_wparam)) {

               error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                             msg_bad_proc_def,
                             (procedure_pointer->st_namtab_ptr)->nt_name);
               procedure_error = YES;

            }

            /* set up for the next formal */

            if (!procedure_error) {
               symtab_ptr->st_in_spec = YES;
               symtab_ptr = symtab_ptr->st_thread;
            }
         }

         symtab_ptr = procedure_pointer;

      }
      else {

         /*
          *  at this point we have a non-procedure symbol, so we must
          *  enter it in the symbol table
          */

         if (namtab_ptr != NULL) {

            for (symtab_ptr = namtab_ptr->nt_symtab_ptr;
                 symtab_ptr != NULL &&
                    symtab_ptr->st_unit_num > 1;
                 symtab_ptr = symtab_ptr->st_name_link);

            if (symtab_ptr != NULL &&
                symtab_ptr->st_type != sym_integer &&
                symtab_ptr->st_type != sym_real &&
                symtab_ptr->st_type != sym_string) {

               error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                             msg_dup_declaration,
                             name_buffer);

            }
         }

         symtab_ptr = get_symtab(SETL_SYSTEM_VOID);
         memcpy((void *)symtab_ptr,
                (void *)&symtab.sr_symtab_item,
                sizeof(struct symtab_item));

         /* only make public symbols visible */

         if (namtab_ptr != NULL && symtab_ptr->st_is_public) {

            symtab_ptr->st_name_link = namtab_ptr->nt_symtab_ptr;
            namtab_ptr->nt_symtab_ptr = symtab_ptr;
            symtab_ptr->st_is_name_attached = YES;

         }

         /* insert the symbol in the appropriate procedure */

         symtab_ptr->st_thread = NULL;
         *(proctab_ptr->pr_symtab_tail) = symtab_ptr;
         proctab_ptr->pr_symtab_tail = &(symtab_ptr->st_thread);
         symtab_ptr->st_owner_proc = proctab_ptr;

         symtab_ptr->st_namtab_ptr = namtab_ptr;

      }

      /* load selectors and literal values */

      switch (symtab_ptr->st_type) {

         case sym_selector :

            /* read in the selector key */

            read_libstr(SETL_SYSTEM libstr_ptr,
                        (char *)&selector_length,
                        sizeof(int));

            read_libstr(SETL_SYSTEM libstr_ptr,
                        name_buffer,
                        selector_length);
            name_buffer[selector_length] = '\0';

            /* look up the lexeme in the name table */

            namtab_ptr = get_namtab(SETL_SYSTEM name_buffer);

            /* if we don't find it, make a literal item */

            if (namtab_ptr->nt_symtab_ptr == NULL) {

               namtab_ptr->nt_token_class = tok_literal;
               namtab_ptr->nt_token_subclass = tok_integer;
               selector_ptr = enter_symbol(SETL_SYSTEM
                                           namtab_ptr,
                                           proctab_ptr,
                                           NULL);

               selector_ptr->st_has_rvalue = YES;
               selector_ptr->st_is_initialized = YES;
               selector_ptr->st_type = sym_integer;
               selector_ptr->st_aux.st_integer_ptr =
                  char_to_int(SETL_SYSTEM name_buffer);

            }
            else {

               selector_ptr = namtab_ptr->nt_symtab_ptr;

            }

            symtab_ptr->st_aux.st_selector_ptr = selector_ptr;

            break;

         case sym_integer :

            symtab_ptr->st_aux.st_integer_ptr =
                  char_to_int(SETL_SYSTEM name_buffer);

            break;

         case sym_real :

            symtab_ptr->st_aux.st_real_ptr =
                  char_to_real(SETL_SYSTEM name_buffer,NULL);

            break;

         case sym_string :

            symtab_ptr->st_aux.st_string_ptr =
                  char_to_string(SETL_SYSTEM name_buffer);

            break;

      }
   }

   close_libstr(SETL_SYSTEM libstr_ptr);
   close_libunit(SETL_SYSTEM libunit_ptr);

}

/*\
 *  \function{load\_class\_spec()}
 *
 *  This function loads a class specification, in preparation for
 *  coding the corresponding class body.  We have two distinct types of
 *  symbols at this point -- methods and others (variables, constants,
 *  \& selectors).  Since we have already loaded the declared variables
 *  in the package body, we should already have symbol table entries for
 *  the method names, and all we do here is verify that they match the
 *  specification in the library.  Other symbols are added to the package
 *  scope.
\*/

static void load_class_spec(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* package to be loaded              */

{
libunit_ptr_type libunit_ptr;          /* library unit                      */
libstr_ptr_type libstr_ptr;            /* symbol table stream               */
unit_control_record unit_control;      /* unit control record               */
symtab_record symtab;                  /* symbol table record               */
char name_buffer[MAX_TOK_LEN + 1];     /* identifier name buffer            */
int symbol_count;                      /* used to count the input symbols   */
namtab_ptr_type namtab_ptr;            /* name table pointer for symbol     */
symtab_ptr_type symtab_ptr;            /* entered or existing symbol table  */
                                       /* pointer                           */
symtab_ptr_type procedure_pointer;     /* symbol table item for procedure   */
int procedure_error;                   /* YES if we find an error in a      */
                                       /* procedure item                    */
int formal_count;                      /* number of formals in a procedure  */
int selector_length;                   /* length of selector name           */
symtab_ptr_type selector_ptr;          /* pointer to selector key           */

   /* open the class specification */

   if ((libunit_ptr = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                   I2_FILE,LIB_READ_UNIT)) == NULL &&
       (libunit_ptr = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                   DEFAULT_LIBFILE,LIB_READ_UNIT)) == NULL) {

      error_message(SETL_SYSTEM &(proctab_ptr->pr_file_pos),
                    msg_no_spec,
                    (proctab_ptr->pr_namtab_ptr)->nt_name);

      return;

   }

   /* read the unit control record */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
   read_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
                sizeof(unit_control_record));
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* open the symbol table stream */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_SYMTAB_STREAM);

   /* read through the symbol table stream */

   for (symbol_count = 0;
        symbol_count < unit_control.uc_symtab_count;
        symbol_count++) {

      /* read the symbol table record and the name string */

      read_libstr(SETL_SYSTEM libstr_ptr,
                  (char *)&symtab,
                  sizeof(symtab_record));

      read_libstr(SETL_SYSTEM libstr_ptr,
                  name_buffer,
                  symtab.sr_name_length);
      name_buffer[symtab.sr_name_length] = '\0';

      /* skip anything not in the specification */

      if (!symtab.sr_symtab_item.st_in_spec)
         continue;

      /* find the corresponding name table entry */

      if (symtab.sr_name_length > 0)
         namtab_ptr = get_namtab(SETL_SYSTEM name_buffer);
      else
         namtab_ptr = NULL;

      /* if the symbol in the library is a method ... */

      if (symtab.sr_symtab_item.st_type == sym_method) {

         procedure_error = NO;
         symtab_ptr = namtab_ptr->nt_symtab_ptr;

         /* the procedure should be in the symbol table */

         if (symtab_ptr == NULL || symtab_ptr->st_unit_num > 1) {

            error_message(SETL_SYSTEM NULL,
                          msg_missing_proc,
                          name_buffer);
            procedure_error = YES;

         }

         if (!procedure_error && symtab_ptr->st_type != sym_method) {

            error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                          msg_expected_tok_proc,
                          name_buffer);
            procedure_error = YES;

         }

         /* we have to read past formal_count symbols */

         formal_count = symtab.sr_param_count;
         symbol_count += formal_count;

         /* start the list of formal parameters in the symbol table */

         if (!procedure_error) {

            procedure_pointer = symtab_ptr;
            symtab_ptr->st_unit_num = 1;
            symtab_ptr->st_offset = symtab.sr_symtab_item.st_offset;
            symtab_ptr->st_is_alloced = YES;
            symtab_ptr->st_in_spec = YES;
            symtab_ptr->st_is_public = YES;
            symtab_ptr =
               (procedure_pointer->st_aux.st_proctab_ptr)->pr_symtab_head;

         }

         /* the number of formals must match */

         if (!procedure_error &&
             (procedure_pointer->st_aux.st_proctab_ptr)->pr_formal_count !=
              formal_count) {

            error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                          msg_bad_proc_def,
                          name_buffer);
            procedure_error = YES;

         }

         /* we have to check each formal parameter */

         while (formal_count--) {

            /* read the symbol table record and the name string */

            read_libstr(SETL_SYSTEM libstr_ptr,
                        (char *)&symtab,
                        sizeof(symtab_record));

            read_libstr(SETL_SYSTEM libstr_ptr,
                        name_buffer,
                        symtab.sr_name_length);

            name_buffer[symtab.sr_name_length] = '\0';

            /*
             *  the name of the formal in the library should match the
             *  one in the symbol table
             */

            if (!procedure_error &&
                (strcmp((symtab_ptr->st_namtab_ptr)->nt_name,
                       name_buffer) != 0 ||
                 symtab.sr_symtab_item.st_is_rparam !=
                    symtab_ptr->st_is_rparam ||
                 symtab.sr_symtab_item.st_is_wparam !=
                    symtab_ptr->st_is_wparam)) {

               error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                             msg_bad_proc_def,
                             (procedure_pointer->st_namtab_ptr)->nt_name);
               procedure_error = YES;

            }

            /* set up for the next formal */

            if (!procedure_error) {
               symtab_ptr->st_in_spec = YES;
               symtab_ptr = symtab_ptr->st_thread;
            }
         }

         symtab_ptr = procedure_pointer;

      }
      else {

         /*
          *  at this point we have a non-procedure symbol, so we must
          *  enter it in the symbol table
          */

         if (namtab_ptr != NULL) {

            for (symtab_ptr = namtab_ptr->nt_symtab_ptr;
                 symtab_ptr != NULL &&
                    symtab_ptr->st_unit_num > 1;
                 symtab_ptr = symtab_ptr->st_name_link);

            if (symtab_ptr != NULL &&
                symtab_ptr->st_type != sym_integer &&
                symtab_ptr->st_type != sym_real &&
                symtab_ptr->st_type != sym_string) {

               error_message(SETL_SYSTEM &(symtab_ptr->st_file_pos),
                             msg_dup_declaration,
                             name_buffer);

            }
         }

         symtab_ptr = get_symtab(SETL_SYSTEM_VOID);
         memcpy((void *)symtab_ptr,
                (void *)&symtab.sr_symtab_item,
                sizeof(struct symtab_item));

         /* only make public symbols visible */

         if (namtab_ptr != NULL && symtab_ptr->st_is_public) {

            symtab_ptr->st_name_link = namtab_ptr->nt_symtab_ptr;
            namtab_ptr->nt_symtab_ptr = symtab_ptr;
            symtab_ptr->st_is_name_attached = YES;

         }

         /* insert the symbol in the appropriate procedure */

         symtab_ptr->st_thread = NULL;
         *(proctab_ptr->pr_symtab_tail) = symtab_ptr;
         proctab_ptr->pr_symtab_tail = &(symtab_ptr->st_thread);
         symtab_ptr->st_owner_proc = proctab_ptr;
         symtab_ptr->st_class = proctab_ptr;
         symtab_ptr->st_namtab_ptr = namtab_ptr;

      }

      /* load selectors and literal values */

      switch (symtab_ptr->st_type) {

         case sym_selector :

            /* read in the selector key */

            read_libstr(SETL_SYSTEM libstr_ptr,
                        (char *)&selector_length,
                        sizeof(int));

            read_libstr(SETL_SYSTEM libstr_ptr,
                        name_buffer,
                        selector_length);
            name_buffer[selector_length] = '\0';

            /* look up the lexeme in the name table */

            namtab_ptr = get_namtab(SETL_SYSTEM name_buffer);

            /* if we don't find it, make a literal item */

            if (namtab_ptr->nt_symtab_ptr == NULL) {

               namtab_ptr->nt_token_class = tok_literal;
               namtab_ptr->nt_token_subclass = tok_integer;
               selector_ptr = enter_symbol(SETL_SYSTEM
                                           namtab_ptr,
                                           proctab_ptr,
                                           NULL);

               selector_ptr->st_has_rvalue = YES;
               selector_ptr->st_is_initialized = YES;
               selector_ptr->st_type = sym_integer;
               selector_ptr->st_aux.st_integer_ptr =
                  char_to_int(SETL_SYSTEM name_buffer);

            }
            else {

               selector_ptr = namtab_ptr->nt_symtab_ptr;

            }

            symtab_ptr->st_aux.st_selector_ptr = selector_ptr;

            break;

         case sym_integer :

            symtab_ptr->st_aux.st_integer_ptr =
                  char_to_int(SETL_SYSTEM name_buffer);

            break;

         case sym_real :

            symtab_ptr->st_aux.st_real_ptr =
                  char_to_real(SETL_SYSTEM name_buffer,NULL);

            break;

         case sym_string :

            symtab_ptr->st_aux.st_string_ptr =
                  char_to_string(SETL_SYSTEM name_buffer);

            break;

      }
   }

   close_libstr(SETL_SYSTEM libstr_ptr);
   close_libunit(SETL_SYSTEM libunit_ptr);

}

/*\
 *  \function{get\_lstack()}
 *
 *  This function expands the loop stack.
\*/

int get_lstack(SETL_SYSTEM_PROTO_VOID)

{
struct loop_stack_item *old_lstack;    /* temporary program stack           */

   if (++lstack_top < lstack_max)
      return lstack_top;

   /* expand the table */

   old_lstack = lstack;
   lstack = (struct loop_stack_item *)malloc((size_t)(
         (lstack_max + LSTACK_BLOCK_SIZE) * sizeof(struct loop_stack_item)));
   if (lstack == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* copy the old table to the new table, and free the old table */

   if (lstack_max > 0) {

      memcpy((void *)lstack,
             (void *)old_lstack,
             (size_t)(lstack_max * sizeof(struct loop_stack_item)));

      free(old_lstack);

   }

   lstack_max += LSTACK_BLOCK_SIZE;

   return lstack_top;

}

/*\
 *  \function{get\_yes\_no()}
 *
 *  This function displays a message for the operator, and demands a yes
 *  or no response.  It will return 1 or 0 according to the operator's
 *  answer.
\*/

#if UNIX_VARARGS
static int get_yes_no(message,va_alist)
   char *message;                      /* first argument of message         */
   va_dcl                              /* any other arguments               */
#else
static int get_yes_no(
   char *message,                      /* first argument of message         */
   ...)                                /* any other arguments               */
#endif

{
va_list arg_pointer;                   /* argument list pointer             */
char prompt[100];                      /* prompt string                     */
char answer[5];                        /* typed answer from user            */
char *p;                               /* temporary looping variable        */
#if UNIX_VARARGS
char vp_pattern[20];                   /* saved vsprintf string             */
char vp_size;                          /* saved vp_size field               */
char vp_c_arg;                         /* hold character arg                */
char *vp_s_arg;                        /* hold string                       */
int vp_i_arg;                          /* hold integer arg                  */
int *vp_n_arg;                         /* hold integer pointer              */
void *vp_p_arg;                        /* hold void pointer                 */
int32 vp_l_arg;                        /* hold long arg                     */
double vp_f_arg;                       /* hold float arg                    */
char *vp_p, *vp_s, *vp_t;              /* used to traverse buffer           */
#endif

   /*
    *  The following ugliness accommodates MIPS brain damage.  First, the
    *  stdarg.h macros in gcc don't work.  To make matters worse, the
    *  varargs.h macros don't work with the v?printf functions.  So, I
    *  have to do this futzing myself.
    */

#if UNIX_VARARGS

   va_start(arg_pointer);
   vp_s = message;
   vp_t = prompt;
   while (*vp_s) {

      /* find the next format string */

      while (*vp_s && *vp_s != '%')  
         *vp_t++ = *vp_s++;

      if (!*vp_s) {
         *vp_t = '\0';
         break;
      }
      if (*(vp_s+1) == '%') {
         *vp_t++ = '%';
         vp_s += 2;
         continue;
      }
      vp_p = vp_pattern;
      *vp_p++ = *vp_s++;
      
      /* skip flags */

      while (*vp_s == '+' ||
             *vp_s == '-' ||
             *vp_s == '#' ||
             *vp_s == ' ')
         *vp_p++ = *vp_s++;

      /* skip width, precision */

      if (is_digit(*vp_s,10)) {
         while (is_digit(*vp_s,10))
            *vp_p++ = *vp_s++;
         if (*vp_s == '.') {
            *vp_p++ = *vp_s++;
            while (is_digit(*vp_s,10))
               *vp_p++ = *vp_s++;
         }
      }

      /* skip vp_size */

      if (*vp_s == 'h' ||
          *vp_s == 'l' ||
          *vp_s == 'L') {

         vp_size = *vp_s;
         *vp_p++ = *vp_s++;

      }
      else {
       
         vp_size = 0;

      }

      /* finally, we should have the type field */
     
      *vp_p = *vp_s++;
      *(vp_p+1) = '\0';
      switch (*vp_p) {

         case 'c' :

            vp_c_arg = va_arg(arg_pointer,char);
            sprintf(vp_t,vp_pattern,vp_c_arg);
            break;

         case 'd' :
         case 'i' :
         case 'o' :
         case 'u' :
         case 'x' :
         case 'X' :

            if (vp_size == 'l') { 
               vp_l_arg = va_arg(arg_pointer,long);
               sprintf(vp_t,vp_pattern,vp_l_arg);
            }
            else {
               vp_i_arg = va_arg(arg_pointer,int);
               sprintf(vp_t,vp_pattern,vp_i_arg);
            }
            break;

         case 'e' :
         case 'E' :
         case 'f' :
         case 'g' :
         case 'G' :
          
            vp_f_arg = va_arg(arg_pointer,double);
            sprintf(vp_t,vp_pattern,vp_f_arg);
            break;

         case 's' :
          
            vp_s_arg = va_arg(arg_pointer,char *);
            sprintf(vp_t,vp_pattern,vp_s_arg);
            break;

         case 'n' :

            vp_n_arg = va_arg(arg_pointer,int *);
            sprintf(vp_t,vp_pattern,vp_n_arg);
            break;

         case 'p' :

            vp_p_arg = va_arg(arg_pointer,void *);
            sprintf(vp_t,vp_pattern,vp_p_arg);
            break;

         default :
            strcpy(vp_t,vp_pattern);

      }

      /* skip past the appended string */

      while (*vp_t)
         vp_t++;

   }

   va_end(arg_pointer);

#else

   va_start(arg_pointer,message);
   vsprintf(prompt,message,arg_pointer);
   va_end(arg_pointer);

#endif

   /* wait for yes or no */

   for (;;) {

      /* display the prompt, and get a response */

      fflush(stdin);
      fputs(prompt,stderr);
      fflush(stderr);
      fgets(answer,5,stdin);
      fflush(stdin);

      /* strip the carriage return and convert to lower case */

      answer[4] = '\0';
      for (p = answer; *p; p++) {
         if (*p == '\n')
            *p = '\0';
         if (isupper(*p))
            *p = tolower(*p);
      }

      /* return if the answer is yes or no */

      if (strcmp(answer,"y") == 0 || strcmp(answer,"yes") == 0)
         return YES;

      if (strcmp(answer,"n") == 0 || strcmp(answer,"no") == 0)
         return NO;

      /* display an error message and continue */

      fputs(msg_want_yes_no,stderr);

   }
}

