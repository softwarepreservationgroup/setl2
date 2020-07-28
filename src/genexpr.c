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
 *  \package{The Expression Code Generator}
 *
 *  The functions in this file handle code generation for expressions.
 *  We have separate functions for classes of AST types, rather than a
 *  single function containing a large switch statement.  Basically, we
 *  are trying to dodge a compiler restriction here.  If we used a single
 *  large switch, we would exceed the maximum function size some
 *  compilers accept.  We use a macro to hide this mechanism from the
 *  calling functions.
 *
 *  The code generator is intended to do a fair job without an
 *  optimizer, and will have to be changed when an optimizer is written.
 *  In particular, we do not generate temporaries with the usual abandon.
 *  We go to some effort to minimize the number generated.
 *
 *  >texify genexpr.h
 *
 *  \packagebody{Expression Code Generator}
\*/


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
#include "quads.h"                     /* quadruples                        */
#include "c_integers.h"                  /* integer literals                  */
#include "lex.h"                       /* lexical analyzer                  */
#include "genquads.h"                  /* quadruple generator               */
#include "genstmt.h"                   /* statement code generator          */
#include "genexpr.h"                   /* expression code generator         */
#include "genbool.h"                   /* boolean expression code generator */
#include "genlhs.h"                    /* left hand side code generator     */
#include "geniter.h"                   /* iterator code generator           */
#include "const.h"                     /* constant expression test          */
#include "listing.h"                   /* source and error listings         */

#ifdef PLUGIN
#define fprintf plugin_fprintf
#endif



/*\
 *  \ast{ast\_list}{expression lists}{
 *     \astnode{ast\_list}
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
 *  This function handles lists, which in this context can only be
 *  expression lists.  We would like to return the result of the last
 *  expression evaluated.  First we allocate a return operand.  Then we
 *  loop over the expressions in the list, using our return operand as
 *  the target.
\*/

symtab_ptr_type gen_expr_list(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type ast_ptr;                  /* used to loop over list            */
symtab_ptr_type return_ptr;            /* return operand                    */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* if we are not passed a target, allocate a temporary */

   if (target == NULL) {
      get_temp(return_ptr);
   }
   else {
      return_ptr = target;
   }

   /* loop over expression list, generating code for each expression */

   for (ast_ptr = root->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

      gen_expression(SETL_SYSTEM ast_ptr,return_ptr);

   }

   return return_ptr;

}

/*\
 *  \astnotree{ast\_symtab}{symbol table pointer}
 *
 *  If we are passed a target operand, the caller expects the value to
 *  end up in that operand, so we must emit an assignment.  Otherwise, we
 *  just return the symbol from the AST.
\*/

symtab_ptr_type gen_expr_symtab(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* if we didn't get a target, just return the symbol table pointer */

   if (target == NULL) {

      return root->ast_child.ast_symtab_ptr;

   }

   /* otherwise emit an assignment */

   else {

      emit(q_assign,
           target,
           root->ast_child.ast_symtab_ptr,
           NULL,
           &(root->ast_file_pos));

      return target;

   }
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
 *  For binary operators we simply evaluate each operand and emit an
 *  instruction to calculate the result.
\*/

symtab_ptr_type gen_expr_binop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* some operands have inverted opcodes */

   if (ast_flip_operands[root->ast_type]) {

      operand[2] = gen_expression(SETL_SYSTEM left_ptr,NULL);
      operand[1] = gen_expression(SETL_SYSTEM right_ptr,NULL);

   }
   else {

      operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);
      operand[2] = gen_expression(SETL_SYSTEM right_ptr,NULL);

   }

   /* emit the instruction */

   emit(ast_default_opcode[root->ast_type],
        operand[0],
        operand[1],
        operand[2],
        &(root->ast_file_pos));

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }
   if (operand[2]->st_is_temp) {
      free_temp(operand[2]);
   }

   return operand[0];

}

/*\
 *  \ast{ast\_and}{and/or operators}
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
 *  For binary operators we simply evaluate each operand and emit an
 *  instruction to calculate the result.
\*/

symtab_ptr_type gen_expr_andor(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
int true_label;                        /* result should be true             */
int false_label;                       /* result should be false            */
int done_label;                        /* finished with assignment          */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* we need three labels associated with the expression */

   true_label = next_label++;
   false_label = next_label++;
   done_label = next_label++;

   /* evaluate the condition */

   gen_boolean(SETL_SYSTEM root,true_label,false_label,true_label);

   /* generate the true code */

   emitiss(q_label,true_label,NULL,NULL,&(root->ast_file_pos));
   emit(q_assign,operand[0],sym_true,NULL,&(root->ast_file_pos));
   emitiss(q_go,done_label,NULL,NULL,&(root->ast_file_pos));

   /* generate the false code */

   emitiss(q_label,false_label,NULL,NULL,&(root->ast_file_pos));
   emit(q_assign,operand[0],sym_false,NULL,&(root->ast_file_pos));
   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   return operand[0];

}

/*\
 *  \ast{ast\_question}{question mark operator}
 *     {\astnode{ast\_question}
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
 *  The question mark operator returns the left hand side unless it is
 *  omega, in which case it returns the right.
\*/

symtab_ptr_type gen_expr_question(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
int om_label;                          /* branch around right assignment    */
int done_label;                        /* branch around right assignment    */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* evaluate the left */

   operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   /* if the left is omega, branch to right assignment */

   om_label = next_label++;
   done_label = next_label++;

   if (operand[0]!=operand[1]) {
      emitiss(q_goeq,
           om_label,
           operand[1],
           sym_omega,
           &(root->ast_file_pos));


      emit(q_assign,
           operand[0],
           operand[1],
           NULL,
           &(root->ast_file_pos));
   } else {
      emitiss(q_gone,
           done_label,
           operand[1],
           sym_omega,
           &(root->ast_file_pos));

   }

      if (operand[1]->st_is_temp) {
         free_temp(operand[1]);
      }
      

   if (operand[0]!=operand[1]) {
     emitiss(q_go,
             done_label,
             NULL,
             NULL,
             &(root->ast_file_pos));
   }

   emitiss(q_label,
           om_label,
           NULL,
           NULL,
           &(root->ast_file_pos));

   /* otherwise evaluate the right also */

   gen_expression(SETL_SYSTEM right_ptr,operand[0]);

   emitiss(q_label,
           done_label,
           NULL,
           NULL,
           &(root->ast_file_pos));

   return operand[0];

}

/*\
 *  \ast{ast\_uminus}{unary operators}
 *     {\astnode{operator}
 *        {\astnode{{\em operand}}
 *           {\etcast}
 *           {\nullast}
 *        }
 *        {\etcast}
 *     }
 *
 *  For unary operators we simply evaluate the operand and emit an
 *  instruction to calculate the result.
\*/

symtab_ptr_type gen_expr_unop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type child_ptr;                /* child node                        */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointer, for readability */

   child_ptr = root->ast_child.ast_child_ast;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* evaluate the operand */

   operand[1] = gen_expression(SETL_SYSTEM child_ptr,NULL);

   /* emit the instruction */
   if ((root->ast_type==ast_uminus)&&(root->ast_extension!=NULL)&&(1==2)) {
   /*
   	   emit(q_push1,
	        operand[1],
	        NULL,
	        NULL,
	        &(root->ast_file_pos));
	    */    
	   emit(q_of1,
	        operand[0],
	        root->ast_extension->nt_symtab_ptr,
	        operand[1],
	        &(root->ast_file_pos));

       	 
   } else {

	   emit(ast_default_opcode[root->ast_type],
	        operand[0],
	        operand[1],
	        NULL,
	        &(root->ast_file_pos));
   }
   
   /* free temporary */

   if (operand[1]->st_is_temp)
      free_temp(operand[1]);

   return operand[0];

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
 *  This function generates code for a procedure call, a map or a tuple
 *  reference.  We depend on the semantic check module to do any error
 *  checking.
\*/

symtab_ptr_type gen_expr_of(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type arg_ptr;                  /* pointer to argument               */
int arg_count;                         /* number of arguments               */
symtab_ptr_type operand[3];            /* array of operands                 */
int opnd_num;                          /* temporary looping variable        */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* count the arguments */

   arg_count = 0;
   for (arg_ptr = right_ptr->ast_child.ast_child_ast;
        arg_ptr != NULL;
        arg_ptr = arg_ptr->ast_next) {

      arg_count++;

   }

   /* if we have only one argument, generate a one-argument opcode */

   if (arg_count == 1) {

      /* allocate a temporary if we did't get a target */

      if (target == NULL) {
         get_temp(operand[0]);
      }
      else {
         operand[0] = target;
      }

      /* operand 1 is the procedure to call */

      operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);
      operand[2] = gen_expression(SETL_SYSTEM right_ptr->ast_child.ast_child_ast,NULL);

      emit(q_of1,
           operand[0],
           operand[1],
           operand[2],
           &(root->ast_file_pos));

      /* free temporaries */

      if (operand[1]->st_is_temp) {
         free_temp(operand[1]);
      }
      if (operand[2]->st_is_temp) {
         free_temp(operand[2]);
      }

      return operand[0];

   }

   /*
    *  We have more than one argument.  We push all the arguments on the
    *  stack and generate a more general opcode.
    */

   opnd_num = 0;
   for (arg_ptr = right_ptr->ast_child.ast_child_ast;
        arg_ptr != NULL;
        arg_ptr = arg_ptr->ast_next) {

      /* we push at most three arguments per instruction */

      if (opnd_num == 3) {

         emit(q_push3,
              operand[0],
              operand[1],
              operand[2],
              &(root->ast_file_pos));

         /* free any temporaries allocated for arguments */

         for (opnd_num = 0; opnd_num < 3; opnd_num++) {

            if (operand[opnd_num]->st_is_temp)
               free_temp(operand[opnd_num]);

         }

         opnd_num = 0;

      }

      operand[opnd_num] = gen_expression(SETL_SYSTEM arg_ptr,NULL);
      opnd_num++;

   }

   /* push whatever arguments we've accumulated */

   if (opnd_num == 1) {
      emit(q_push1,operand[0],NULL,NULL,&(root->ast_file_pos));
   }
   else if (opnd_num == 2) {
      emit(q_push2,operand[0],operand[1],NULL,&(root->ast_file_pos));
   }
   else if (opnd_num == 3) {
      emit(q_push3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
   }

   /* free any temporaries allocated for arguments */

   while (opnd_num--) {

      if (operand[opnd_num]->st_is_temp)
         free_temp(operand[opnd_num]);

   }

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* operand 1 is the procedure to call, or the map */

   operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   emitssi(ast_default_opcode[root->ast_type],
           operand[0],
           operand[1],
           arg_count,
           &(root->ast_file_pos));

   /* free temporaries */

   if (operand[1]->st_is_temp)
      free_temp(operand[1]);

   return operand[0];

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
 *  This function handles multi-valued map references.  We count the
 *  number of arguments.  If there is more than one, we form a tuple of
 *  the arguments.  Then we emit a map reference.
\*/

symtab_ptr_type gen_expr_ofa(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type arg_ptr;                  /* pointer to argument               */
int32 arg_count;                        /* number of arguments               */
char arg_count_string[12];             /* argument count literal            */
namtab_ptr_type namtab_ptr;            /* name pointer to map size          */
symtab_ptr_type operand[3];            /* array of operands                 */
int opnd_num;                          /* temporary looping variable        */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* count the arguments */

   arg_count = 0;
   for (arg_ptr = right_ptr->ast_child.ast_child_ast;
        arg_ptr != NULL;
        arg_ptr = arg_ptr->ast_next) {

      arg_count++;

   }

   /* if we have more than one argument, form a tuple */

   if (arg_count > 1) {

      opnd_num = 0;
      for (arg_ptr = right_ptr->ast_child.ast_child_ast;
           arg_ptr != NULL;
           arg_ptr = arg_ptr->ast_next) {

         /* we push at most three arguments per instruction */

         if (opnd_num == 3) {

            emit(q_push3,
                 operand[0],
                 operand[1],
                 operand[2],
                 &(root->ast_file_pos));

            /* free any temporaries allocated for arguments */

            for (opnd_num = 0; opnd_num < 3; opnd_num++) {

               if (operand[opnd_num]->st_is_temp)
                  free_temp(operand[opnd_num]);

            }

            opnd_num = 0;

         }

         operand[opnd_num] = gen_expression(SETL_SYSTEM arg_ptr,NULL);
         opnd_num++;

      }

      /* push whatever arguments we've accumulated */

      if (opnd_num == 1) {
         emit(q_push1,operand[0],NULL,NULL,&(root->ast_file_pos));
      }
      else if (opnd_num == 2) {
         emit(q_push2,operand[0],operand[1],NULL,&(root->ast_file_pos));
      }
      else if (opnd_num == 3) {
         emit(q_push3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
      }

      /* free any temporaries allocated for arguments */

      while (opnd_num--) {

         if (operand[opnd_num]->st_is_temp)
            free_temp(operand[opnd_num]);

      }

      /* allocate a temporary for the tuple */

      get_temp(operand[2]);

      /* we need to have a symbol table pointer as second argument */

      sprintf(arg_count_string,"%ld",arg_count);
      namtab_ptr = get_namtab(SETL_SYSTEM arg_count_string);

      /* if we don't find it, make a literal item */

      if (namtab_ptr->nt_symtab_ptr == NULL) {

         namtab_ptr->nt_token_class = tok_literal;
         namtab_ptr->nt_token_subclass = tok_integer;
         operand[1] = enter_symbol(SETL_SYSTEM
                                   namtab_ptr,
                                   unit_proctab_ptr,
                                   &(root->ast_file_pos));

         operand[1]->st_has_rvalue = YES;
         operand[1]->st_is_initialized = YES;
         operand[1]->st_type = sym_integer;
         operand[1]->st_aux.st_integer_ptr = char_to_int(SETL_SYSTEM arg_count_string);

      }
      else {

         operand[1] = namtab_ptr->nt_symtab_ptr;

      }

      /* emit the tuple instruction */

      emit(q_tuple,
           operand[2],
           operand[1],
           NULL,
           &(root->ast_file_pos));

   }
   else {

      operand[2] = gen_expression(SETL_SYSTEM right_ptr->ast_child.ast_child_ast,NULL);

   }

   /* operand 1 is the map */

   operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* emit the map reference */

   emit(q_ofa,
        operand[0],
        operand[1],
        operand[2],
        &(root->ast_file_pos));

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }
   if (operand[2]->st_is_temp) {
      free_temp(operand[2]);
   }

   return operand[0];

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
 *  assignments.
\*/

symtab_ptr_type gen_expr_slice(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type end_ptr;                  /* end of slice node                 */
symtab_ptr_type operand[4];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;
   end_ptr = right_ptr->ast_next;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* find the operands */

   operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);
   operand[2] = gen_expression(SETL_SYSTEM right_ptr,NULL);
   operand[3] = gen_expression(SETL_SYSTEM end_ptr,NULL);

   /* emit the instruction */

   emit(q_slice,
        operand[0],
        operand[1],
        operand[2],
        &(root->ast_file_pos));

   /* emit a noop, to hold the last operand */

   emit(q_noop,
        operand[3],
        NULL,
        NULL,
        &(root->ast_file_pos));

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }
   if (operand[2]->st_is_temp) {
      free_temp(operand[2]);
   }
   if (operand[3]->st_is_temp) {
      free_temp(operand[3]);
   }

   return operand[0];

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
 *  assignments.
\*/

symtab_ptr_type gen_expr_end(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* find the operands */

   operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);
   operand[2] = gen_expression(SETL_SYSTEM right_ptr,NULL);

   /* emit the instruction */

   emit(q_end,
        operand[0],
        operand[1],
        operand[2],
        &(root->ast_file_pos));

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }
   if (operand[2]->st_is_temp) {
      free_temp(operand[2]);
   }

   return operand[0];

}

/*\
 *  \ast{ast\_assign}{assignment expressions}{
 *     \astnode{ast\_assign}
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
 *  This is similar to assignment statements.  We would like to point out
 *  that assignment expressions return their right hand side, not the
 *  left.  This is to accommodate place holders on the left.
\*/

symtab_ptr_type gen_expr_assign(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* if the left hand side is an identifier, use it as the target */

   if (left_ptr->ast_type == ast_symtab) {

      operand[0] = gen_expression(SETL_SYSTEM right_ptr,
                                  left_ptr->ast_child.ast_symtab_ptr);

   }

   /* if the LHS is not a simple variable ... */

   else {

      operand[0] = gen_expression(SETL_SYSTEM right_ptr,NULL);

      gen_lhs(SETL_SYSTEM left_ptr,operand[0]);

   }

   /* if we got a target, we generate an assignment */

   if (target != NULL) {

      emit(q_assign,
           target,
           operand[0],
           NULL,
           &(root->ast_file_pos));

   }

   return operand[0];

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
 *  The big problem with assignment operators is that if the target is
 *  indexed, we don't want to evaluate the indices twice.  The case
 *  statements here insure that.
\*/

symtab_ptr_type gen_expr_assignop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left operand                      */
ast_ptr_type right_ptr;                /* right operand                     */
ast_ptr_type index_ptr;                /* assignment index                  */
ast_ptr_type *index_place;             /* index position                    */
symtab_ptr_type operand[3];            /* array of operands                 */
symtab_ptr_type temp_list, new_temp;   /* list of generated temporaries     */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   right_ptr = root->ast_child.ast_child_ast;

   /* go down the left branch of the tree, replacing indices by temps */

   temp_list = NULL;
   for (left_ptr = right_ptr->ast_child.ast_child_ast;
        left_ptr != NULL &&
           (left_ptr->ast_type == ast_of ||
            left_ptr->ast_type == ast_ofa ||
            left_ptr->ast_type == ast_end ||
            left_ptr->ast_type == ast_slice);
        left_ptr = left_ptr->ast_child.ast_child_ast) {

      /* we have different tree shapes to handle */

      switch (left_ptr->ast_type) {

         case ast_of :        case ast_ofa :

            index_place = &(((left_ptr->ast_child.ast_child_ast)->ast_next)->
                             ast_child.ast_child_ast);
            index_ptr = *index_place;
            new_temp = gen_expression(SETL_SYSTEM index_ptr,NULL);

            /* if the index is a temporary, surgically change ast */

            if (new_temp->st_is_temp) {

               new_temp->st_is_temp = NO;
               new_temp->st_name_link = temp_list;
               temp_list = new_temp;
               index_ptr = get_ast(SETL_SYSTEM_VOID);
               index_ptr->ast_type = ast_symtab;
               index_ptr->ast_child.ast_symtab_ptr = new_temp;
               index_ptr->ast_next = (*index_place)->ast_next;
               copy_file_pos(&(index_ptr->ast_file_pos),
                             &((*index_place)->ast_file_pos));
               (*index_place)->ast_next = NULL;
               kill_ast(*index_place);
               *index_place = index_ptr;

            }

            break;

         case ast_end :

            index_place = &((left_ptr->ast_child.ast_child_ast)->ast_next);
            index_ptr = *index_place;
            new_temp = gen_expression(SETL_SYSTEM index_ptr,NULL);

            /* if the index is a temporary, surgically change ast */

            if (new_temp->st_is_temp) {

               new_temp->st_is_temp = NO;
               new_temp->st_name_link = temp_list;
               temp_list = new_temp;
               index_ptr = get_ast(SETL_SYSTEM_VOID);
               index_ptr->ast_type = ast_symtab;
               index_ptr->ast_next = (*index_place)->ast_next;
               index_ptr->ast_child.ast_symtab_ptr = new_temp;
               copy_file_pos(&(index_ptr->ast_file_pos),
                             &((*index_place)->ast_file_pos));
               (*index_place)->ast_next = NULL;
               kill_ast(*index_place);
               *index_place = index_ptr;

            }

            break;

         case ast_slice :

            index_place = &((left_ptr->ast_child.ast_child_ast)->ast_next);
            index_ptr = *index_place;
            new_temp = gen_expression(SETL_SYSTEM index_ptr,NULL);

            /* if the index is a temporary, surgically change ast */

            if (new_temp->st_is_temp) {

               new_temp->st_is_temp = NO;
               new_temp->st_name_link = temp_list;
               temp_list = new_temp;
               index_ptr = get_ast(SETL_SYSTEM_VOID);
               index_ptr->ast_type = ast_symtab;
               index_ptr->ast_next = (*index_place)->ast_next;
               index_ptr->ast_child.ast_symtab_ptr = new_temp;
               copy_file_pos(&(index_ptr->ast_file_pos),
                             &((*index_place)->ast_file_pos));
               (*index_place)->ast_next = NULL;
               kill_ast(*index_place);
               *index_place = index_ptr;

            }

            /* start again with the second index */

            index_place =
                  &(((left_ptr->ast_child.ast_child_ast)->ast_next)->ast_next);
            index_ptr = *index_place;
            new_temp = gen_expression(SETL_SYSTEM index_ptr,NULL);

            /* if the index is a temporary, surgically change ast */

            if (new_temp->st_is_temp) {

               new_temp->st_is_temp = NO;
               new_temp->st_name_link = temp_list;
               temp_list = new_temp;
               index_ptr = get_ast(SETL_SYSTEM_VOID);
               index_ptr->ast_type = ast_symtab;
               index_ptr->ast_next = (*index_place)->ast_next;
               index_ptr->ast_child.ast_symtab_ptr = new_temp;
               copy_file_pos(&(index_ptr->ast_file_pos),
                             &((*index_place)->ast_file_pos));
               (*index_place)->ast_next = NULL;
               kill_ast(*index_place);
               *index_place = index_ptr;

            }

            break;

      }
   }

   /* we mucked up the left pointer, restore it */

   left_ptr = right_ptr->ast_child.ast_child_ast;

   /* if the left hand side is an identifier, use it as the target */

   if (left_ptr->ast_type == ast_symtab) {

      operand[0] = gen_expression(SETL_SYSTEM right_ptr,
                                  left_ptr->ast_child.ast_symtab_ptr);

   }

   /* if the LHS is not a simple variable ... */

   else {

      operand[0] = gen_expression(SETL_SYSTEM right_ptr,NULL);

      gen_lhs(SETL_SYSTEM left_ptr,operand[0]);

   }

   /* if we created any temporaries, free them */

   while (temp_list != NULL) {

      new_temp = temp_list;
      temp_list = new_temp->st_name_link;
      new_temp->st_is_temp = YES;
      free_temp(new_temp);

   }

   /* if we got a target, we generate an assignment */

   if (target != NULL) {

      emit(q_assign,
           target,
           operand[0],
           NULL,
           &(root->ast_file_pos));

   }

   return operand[0];

}

/*\
 *  \ast{ast\_enum\_set}{enumerated set and tuple formers}
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
 *  This function handles enumerated sets and enumerated tuples which appear
 *  in right hand side contexts.  All we do is generate code to push all
 *  the arguments on the stack and emit an instruction to build the set
 *  or tuple.
 *
 *  Note that the second argument of a set-former instruction is a SETL2
 *  integer, which we must create, rather than an integer operand.  We
 *  must allow this argument to be calculated at run time in set formers,
 *  so we use SETL2 integers.
\*/

symtab_ptr_type gen_expr_enum(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type elem_ptr;                 /* pointer to element                */
int32 elem_count;                       /* number of elements                */
char elem_count_string[20];            /* character version for symtab      */
namtab_ptr_type namtab_ptr;            /* name pointer to element count     */
symtab_ptr_type operand[3];            /* array of operands                 */
int opnd_num;                          /* temporary looping variable        */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* push all the elements */

   elem_count = 0;
   opnd_num = 0;
   for (elem_ptr = root->ast_child.ast_child_ast;
        elem_ptr != NULL;
        elem_ptr = elem_ptr->ast_next) {

      /* we push at most three arguments per instruction */

      if (opnd_num == 3) {

         emit(q_push3,
              operand[0],
              operand[1],
              operand[2],
              &(root->ast_file_pos));

         /* free any temporaries allocated for arguments */

         for (opnd_num = 0; opnd_num < 3; opnd_num++) {

            if (operand[opnd_num]->st_is_temp)
               free_temp(operand[opnd_num]);

         }

         opnd_num = 0;

      }

      operand[opnd_num] = gen_expression(SETL_SYSTEM elem_ptr,NULL);
      opnd_num++;
      elem_count++;

   }

   /* push whatever arguments we've accumulated */

   if (opnd_num == 1) {
      emit(q_push1,operand[0],NULL,NULL,&(root->ast_file_pos));
   }
   else if (opnd_num == 2) {
      emit(q_push2,operand[0],operand[1],NULL,&(root->ast_file_pos));
   }
   else if (opnd_num == 3) {
      emit(q_push3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
   }

   /* free any temporaries allocated for elements */

   while (opnd_num--) {

      if (operand[opnd_num]->st_is_temp)
         free_temp(operand[opnd_num]);

   }

   /* the target is a temporary, if we weren't passed a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* we need to have a symbol table pointer as second argument */

   sprintf(elem_count_string,"%ld",elem_count);
   namtab_ptr = get_namtab(SETL_SYSTEM elem_count_string);

   /* if we don't find it, make a literal item */

   if (namtab_ptr->nt_symtab_ptr == NULL) {

      namtab_ptr->nt_token_class = tok_literal;
      namtab_ptr->nt_token_subclass = tok_integer;
      operand[1] = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                unit_proctab_ptr,
                                &(root->ast_file_pos));

      operand[1]->st_has_rvalue = YES;
      operand[1]->st_is_initialized = YES;
      operand[1]->st_type = sym_integer;
      operand[1]->st_aux.st_integer_ptr = char_to_int(SETL_SYSTEM elem_count_string);

   }
   else {

      operand[1] = namtab_ptr->nt_symtab_ptr;

   }

   /* emit the set or tuple instruction */

   emit(ast_default_opcode[root->ast_type],
        operand[0],
        operand[1],
        NULL,
        &(root->ast_file_pos));

   return operand[0];

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
 *  This function handles all forms of set and tuple formers.  We depend upon
 *  the iteration package to do most of the work here.
\*/

symtab_ptr_type gen_expr_settup(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
symtab_ptr_type operand[3];            /* array of operands                 */
c_iter_ptr_type iter_ptr;              /* iterator control record           */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* allocate a temporary for the cardinality */

   get_temp(operand[1]);

   /* start the loop, with a cardinality of zero */

   emit(q_assign,operand[1],sym_zero,NULL,&(root->ast_file_pos));

   /* start iteration over source */

   operand[2] = gen_iter_values(SETL_SYSTEM root,NULL,&iter_ptr,NO);

   /* push the element we find */

   emit(q_push1,operand[2],NULL,NULL,&(root->ast_file_pos));
   emit(q_add,operand[1],operand[1],sym_one,&(root->ast_file_pos));

   /* finish up the loop */

   gen_iter_bottom(SETL_SYSTEM iter_ptr);

   /* the target is a temporary, if we weren't passed a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* emit the set or tuple former instruction */

   emit(ast_default_opcode[root->ast_type],
        operand[0],
        operand[1],
        NULL,
        &(root->ast_file_pos));

   /* free temporaries and return */

   free_temp(operand[1]);

   return operand[0];

}

/*\
 *  \ast{ast\_exists}{exists expression}
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
 *  This function handles \verb"EXIST" expressions.  Our exists expressions
 *  are less powerful than those of SETL, since we do not allow iterators
 *  to have side effects.  This exists expression only yields a true or
 *  false value.
\*/

symtab_ptr_type gen_expr_exists(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type cond_ptr;                 /* set inclusion condition           */
symtab_ptr_type operand[3];            /* array of operands                 */
c_iter_ptr_type iter_ptr;              /* iterator control record           */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, to shorten expressions */

   iter_list_ptr = root->ast_child.ast_child_ast;
   cond_ptr = iter_list_ptr->ast_next;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* by default, the value is false */

   emit(q_assign,
        operand[0],
        sym_false,
        NULL,
        &(root->ast_file_pos));

   /* start the search loop */

   iter_ptr = gen_iter_varvals(SETL_SYSTEM iter_list_ptr,cond_ptr);

   /* if we find one the value is true */

   emit(q_assign,
        operand[0],
        sym_true,
        NULL,
        &(root->ast_file_pos));

   /* break the exists loop */

   emitiss(q_go,
           iter_ptr->it_fail_label,
           NULL,
           NULL,
           &(root->ast_file_pos));

   /* finish the loop */

   gen_iter_bottom(SETL_SYSTEM iter_ptr);

   return operand[0];

}

/*\
 *  \ast{ast\_forall}{forall expression}
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
 *  This function handles \verb"FORALL" expressions.  We are unable to
 *  use the iterator condition test here, since we check for failure, not
 *  success.
\*/

symtab_ptr_type gen_expr_forall(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type cond_ptr;                 /* set inclusion condition           */
symtab_ptr_type operand[3];            /* array of operands                 */
c_iter_ptr_type iter_ptr;              /* iterator control record           */
int break_label;                       /* break out of loop with failure    */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, to shorten expressions */

   iter_list_ptr = root->ast_child.ast_child_ast;
   cond_ptr = iter_list_ptr->ast_next;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   break_label = next_label++;

   /* by default, the value is false */

   emit(q_assign,
        operand[0],
        sym_false,
        NULL,
        &(root->ast_file_pos));

   /* start the search loop */

   iter_ptr = gen_iter_varvals(SETL_SYSTEM iter_list_ptr,NULL);

   gen_boolean(SETL_SYSTEM cond_ptr,
               iter_ptr->it_loop_label,
               break_label,
               -1);

   /* finish the loop */

   gen_iter_bottom(SETL_SYSTEM iter_ptr);

   /* if we exit normally, forall is true */

   emit(q_assign,
        operand[0],
        sym_true,
        NULL,
        &(root->ast_file_pos));

   emitiss(q_label,break_label,NULL,NULL,&(root->ast_file_pos));

   return operand[0];

}

/*\
 *  \ast{ast\_apply}{application operator}
 *     {\astnode{ast\_apply}
 *        {\astnode{{\em binary op}}
 *           {\astnode{{\em source}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *           {\etcast}
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles the application operator, which allows us to
 *  apply binary operators to the elements of a set, tuple, string, etc.
\*/

symtab_ptr_type gen_expr_apply(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type binop_ptr;                /* binary operator                   */
ast_ptr_type source_ptr;               /* iteration source                  */
symtab_ptr_type operand[3];            /* array of operands                 */
c_iter_ptr_type iter_ptr;              /* iterator control record           */
symtab_ptr_type flag;                  /* flag indicating we found first    */
int first_label;                       /* code to handle first item         */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, to shorten expressions */

   binop_ptr = root->ast_child.ast_child_ast;
   source_ptr = binop_ptr->ast_child.ast_child_ast;

   /* always allocate a temporary for the target */

   get_temp(operand[0]);

   /* we need a flag, which starts out as true */

   get_temp(flag);

   emit(q_assign,
        flag,
        sym_true,
        NULL,
        &(root->ast_file_pos));

   /* the default value is omega */

   emit(q_assign,
        operand[0],
        sym_omega,
        NULL,
        &(root->ast_file_pos));

   /* generate the iteration top */

   operand[1] = gen_iter_values(SETL_SYSTEM source_ptr,NULL,&iter_ptr,YES);

   first_label = next_label++;

   /* the first time, skip the binary operator */

   emitiss(q_gotrue,
           first_label,
           flag,
           NULL,
           &(root->ast_file_pos));

   /* some operands have inverted opcodes */

   if (ast_flip_operands[binop_ptr->ast_type]) {

      emit(ast_default_opcode[binop_ptr->ast_type],
           operand[0],
           operand[1],
           operand[0],
           &(root->ast_file_pos));

   }
   else {

      emit(ast_default_opcode[binop_ptr->ast_type],
           operand[0],
           operand[0],
           operand[1],
           &(root->ast_file_pos));

   }

   emitiss(q_go,
           iter_ptr->it_loop_label,
           NULL,
           NULL,
           &(root->ast_file_pos));

   /* the first time in the loop, we return the first value */

   emitiss(q_label,
           first_label,
           NULL,
           NULL,
           &(root->ast_file_pos));

   emit(q_assign,
        flag,
        sym_false,
        NULL,
        &(root->ast_file_pos));

   emit(q_assign,
        operand[0],
        operand[1],
        NULL,
        &(root->ast_file_pos));

   /* finish up the loop */

   gen_iter_bottom(SETL_SYSTEM iter_ptr);

   free_temp(flag);

   /* make sure the result ends up in the target */

   if (target != NULL) {

      emit(q_assign,
           target,
           operand[0],
           NULL,
           &(root->ast_file_pos));
      free_temp(operand[0]);

      return target;

   }
   else {

      return operand[0];

   }
}

/*\
 *  \ast{ast\_binapply}{application operator}
 *     {\astnode{ast\_binapply}
 *        {\astnode{{\em binary op}}
 *           {\astnode{{\em source}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *           {\etcast}
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles the application operator, which allows us to
 *  apply binary operators to the elements of a set, tuple, string, etc.
\*/

symtab_ptr_type gen_expr_binapply(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type binop_ptr;                /* binary operator                   */
ast_ptr_type source_ptr;               /* iteration source                  */
ast_ptr_type first_ptr;                /* first value                       */
symtab_ptr_type operand[3];            /* array of operands                 */
c_iter_ptr_type iter_ptr;              /* iterator control record           */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, to shorten expressions */

   binop_ptr = root->ast_child.ast_child_ast;
   first_ptr = binop_ptr->ast_child.ast_child_ast;
   source_ptr = first_ptr->ast_next;

   /* always allocate a temporary for the target */

   get_temp(operand[0]);

   /* the left operand is the default */

   gen_expression(SETL_SYSTEM first_ptr,operand[0]);

   /* generate the iteration top */

   operand[1] = gen_iter_values(SETL_SYSTEM source_ptr,NULL,&iter_ptr,YES);

   /* some operands have inverted opcodes */

   if (ast_flip_operands[binop_ptr->ast_type]) {

      emit(ast_default_opcode[binop_ptr->ast_type],
           operand[0],
           operand[1],
           operand[0],
           &(root->ast_file_pos));

   }
   else {

      emit(ast_default_opcode[binop_ptr->ast_type],
           operand[0],
           operand[0],
           operand[1],
           &(root->ast_file_pos));

   }

   /* finish up the loop */

   gen_iter_bottom(SETL_SYSTEM iter_ptr);

   /* make sure the result ends up in the target */

   if (target != NULL) {

      emit(q_assign,
           target,
           operand[0],
           NULL,
           &(root->ast_file_pos));
      free_temp(operand[0]);

      return target;

   }
   else {

      return operand[0];

   }
}


/*\
 *  \ast{ast\_if}{if expression}
 *     {\astnode{ast\_if}
 *        {\astnode{{\em true code}}
 *           {\etcast}
 *           {\astnode{{\em false code}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles if expressions.  Since SETL2 is an expression
 *  language, we are permitted to use if in a right hand side context.
\*/

symtab_ptr_type gen_expr_if(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type true_ptr, false_ptr;      /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */
int true_label;                        /* start next iteration              */
int false_label;                       /* follows condition check           */
int done_label;                        /* break out of loop                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   true_ptr = left_ptr->ast_next;
   false_ptr = true_ptr->ast_next;

   /* we need three labels associated with the expression */

   true_label = next_label++;
   false_label = next_label++;
   done_label = next_label++;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* evaluate the condition */

   gen_boolean(SETL_SYSTEM left_ptr,true_label,false_label,true_label);

   /* generate the true code */

   emitiss(q_label,true_label,NULL,NULL,&(root->ast_file_pos));
   gen_expression(SETL_SYSTEM true_ptr,operand[0]);
   emitiss(q_go,done_label,NULL,NULL,&(root->ast_file_pos));

   /* generate the false code */

   emitiss(q_label,false_label,NULL,NULL,&(root->ast_file_pos));
   gen_expression(SETL_SYSTEM false_ptr,operand[0]);
   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   return operand[0];

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
 *  This function generates code for a while expression.  We return omega
 *  if there is no quit statement, or the value associated with the quit
 *  statement if there is.
\*/

symtab_ptr_type gen_expr_while(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */
int loop_label;                        /* start next iteration              */
int start_label;                       /* follows condition check           */
int quit_label;                        /* break out of loop                 */
int done_label;                        /* normal loop termination label     */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* we need four labels associated with the loop */

   loop_label = next_label++;
   start_label = next_label++;
   done_label = next_label++;
   quit_label = next_label++;

   /* set the top of the loop */

   emitiss(q_label,loop_label,NULL,NULL,&(root->ast_file_pos));

   /* evaluate the condition */

   gen_boolean(SETL_SYSTEM left_ptr,start_label,done_label,start_label);

   emitiss(q_label,start_label,NULL,NULL,&(root->ast_file_pos));

   /* push the break and cycle label on the loop stack */

   get_lstack(SETL_SYSTEM_VOID);
   lstack[lstack_top].ls_exit_label = quit_label;
   lstack[lstack_top].ls_continue_label = loop_label;
   if (target == NULL) {
      get_temp(lstack[lstack_top].ls_return);
   }
   else {
      lstack[lstack_top].ls_return = target;
   }

   /* generate code for the body */

   gen_statement(SETL_SYSTEM right_ptr);

   /* branch to top of loop */

   emitiss(q_go,loop_label,NULL,NULL,&(root->ast_file_pos));

   /* pop the loop stack */

   operand[0] = lstack[lstack_top].ls_return;
   lstack_top--;

   /* if the loop terminates normally, we return omega */

   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   emit(q_assign,
        operand[0],
        sym_omega,
        NULL,
        &(root->ast_file_pos));

   /* skip the previous assignment, if we break out of the loop */

   emitiss(q_label,quit_label,NULL,NULL,&(root->ast_file_pos));

   return operand[0];

}

/*\
 *  \ast{ast\_until}{until expressions}
 *     {\astnode{ast\_until}
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
 *  This function generates code for a until expression.  We return omega
 *  if there is no quit statement, or the value associated with the quit
 *  statement if there is.
\*/

symtab_ptr_type gen_expr_until(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */
int loop_label;                        /* start next iteration              */
int quit_label;                        /* break out of loop                 */
int done_label;                        /* normal loop termination label     */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* we need three labels associated with the loop */

   loop_label = next_label++;
   done_label = next_label++;
   quit_label = next_label++;

   /* set the top of the loop */

   emitiss(q_label,loop_label,NULL,NULL,&(root->ast_file_pos));

   /* push the break and cycle label on the loop stack */

   get_lstack(SETL_SYSTEM_VOID);
   lstack[lstack_top].ls_exit_label = quit_label;
   lstack[lstack_top].ls_continue_label = loop_label;
   if (target == NULL) {
      get_temp(lstack[lstack_top].ls_return);
   }
   else {
      lstack[lstack_top].ls_return = target;
   }

   /* generate code for the body */

   gen_statement(SETL_SYSTEM right_ptr);

   /* evaluate the condition */

   gen_boolean(SETL_SYSTEM left_ptr,done_label,loop_label,done_label);

   /* pop the loop stack */

   operand[0] = lstack[lstack_top].ls_return;
   lstack_top--;

   /* if the loop terminates normally, we return omega */

   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   emit(q_assign,
        operand[0],
        sym_omega,
        NULL,
        &(root->ast_file_pos));

   /* skip the previous assignment, if we break out of the loop */

   emitiss(q_label,quit_label,NULL,NULL,&(root->ast_file_pos));

   return operand[0];

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

symtab_ptr_type gen_expr_loop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
int loop_label;                        /* start next iteration              */
int quit_label;                        /* break out of loop                 */
int done_label;                        /* normal loop termination label     */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* we need three labels associated with the loop */

   loop_label = next_label++;
   done_label = next_label++;
   quit_label = next_label++;

   /* set the top of the loop */

   emitiss(q_label,loop_label,NULL,NULL,&(root->ast_file_pos));

   /* push the break and cycle label on the loop stack */

   get_lstack(SETL_SYSTEM_VOID);
   lstack[lstack_top].ls_exit_label = quit_label;
   lstack[lstack_top].ls_continue_label = loop_label;
   if (target == NULL) {
      get_temp(lstack[lstack_top].ls_return);
   }
   else {
      lstack[lstack_top].ls_return = target;
   }

   /* generate code for the body */

   gen_statement(SETL_SYSTEM root->ast_child.ast_child_ast);

   /* branch to top of loop */

   emitiss(q_go,loop_label,NULL,NULL,&(root->ast_file_pos));

   /* pop the loop stack */

   operand[0] = lstack[lstack_top].ls_return;
   lstack_top--;

   /* if the loop terminates normally, we return omega */

   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   emit(q_assign,
        operand[0],
        sym_omega,
        NULL,
        &(root->ast_file_pos));

   /* skip the previous assignment, if we break out of the loop */

   emitiss(q_label,quit_label,NULL,NULL,&(root->ast_file_pos));

   return operand[0];

}

/*\
 *  \ast{ast\_for}{for loops}
 *     {\astnode{ast\_for}
 *        {\astnode{{\em iterator}}
 *           {\etcast}
 *           {\astnode{{\em condition}}
 *              {\etcast}
 *              {\astnode{{\em loop body}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles for loops.  Most of the real work here is done
 *  by other functions which handle iterators, boolean conditions, and
 *  expressions.  This is just a template for a for loop.
\*/

symtab_ptr_type gen_expr_for(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type cond_ptr;                 /* inclusion condition               */
ast_ptr_type stmt_list_ptr;            /* loop body list                    */
symtab_ptr_type operand[3];            /* array of operands                 */
c_iter_ptr_type iter_ptr;              /* iterator control record           */
int quit_label;                        /* label for quit statements         */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   iter_list_ptr = root->ast_child.ast_child_ast;
   cond_ptr = iter_list_ptr->ast_next;
   stmt_list_ptr = cond_ptr->ast_next;

   /* we need a break label */

   quit_label = next_label++;

   /* generate loop initialization */

   iter_ptr = gen_iter_varvals(SETL_SYSTEM iter_list_ptr,cond_ptr);
   get_lstack(SETL_SYSTEM_VOID);
   lstack[lstack_top].ls_exit_label = quit_label;
   lstack[lstack_top].ls_continue_label = iter_ptr->it_loop_label;
   if (target == NULL) {
      get_temp(lstack[lstack_top].ls_return);
   }
   else {
      lstack[lstack_top].ls_return = target;
   }

   /* generate code for the body */

   gen_statement(SETL_SYSTEM stmt_list_ptr);

   /* set the bottom of the loop */

   gen_iter_bottom(SETL_SYSTEM iter_ptr);

   /* pop the loop stack */

   operand[0] = lstack[lstack_top].ls_return;
   lstack_top--;

   /* if the loop terminates normally, we return omega */

   emit(q_assign,
        operand[0],
        sym_omega,
        NULL,
        &(root->ast_file_pos));

   /* skip the previous assignment, if we break out of the loop */

   emitiss(q_label,quit_label,NULL,NULL,&(root->ast_file_pos));

   return operand[0];

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
 *  Case and select expressions are probably the most complex statements
 *  to encode.  Generally, we would like to build a map with value /
 *  label pairs.  Then we evaluate the discriminant, and use a map
 *  reference to branch to its corresponding label.  Since we allow
 *  non-constant expressions in cases, we must form the map on the fly.
 *  If we happen to have constant expressions, we only form the map once.
\*/

symtab_ptr_type gen_expr_case(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type when_ptr;                 /* when clause pointer               */
ast_ptr_type case_ptr;                 /* case expression pointer           */
symtab_ptr_type pair_operand[2];       /* value / label pairs               */
symtab_ptr_type operand[3];            /* array of operands                 */
int operand_num;                       /* current operand number            */
int bypass_label;                      /* bypass map building               */
int first_label;                       /* first when clause label           */
int when_label;                        /* when clause label                 */
int default_label;                     /* otherwise label                   */
int done_label;                        /* end of case label                 */
int32 map_card;                         /* cardinality of map                */
char map_card_string[20];              /* string representation of above    */
namtab_ptr_type namtab_ptr;            /* name pointer to map size          */
int can_bypass;                        /* YES if can bypass map building    */
symtab_ptr_type map;                   /* case map                          */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /*
    *  First, we examine each case value.  We allocate labels for each
    *  case body, and determine if each case value is constant.  If so,
    *  we only have to build a map of values and branch locations on the
    *  first pass.
    */

   can_bypass = YES;
   first_label = next_label;
   for (when_ptr = ((root->ast_child.ast_child_ast)->ast_next)->
                                             ast_child.ast_child_ast;
        when_ptr != NULL;
        when_ptr = when_ptr->ast_next) {

      /* check each case value */

      for (case_ptr = (when_ptr->ast_child.ast_child_ast)->
                                      ast_child.ast_child_ast;
           case_ptr != NULL && can_bypass;
           case_ptr = case_ptr->ast_next)
         can_bypass = is_constant(case_ptr);

      next_label++;

   }

   /* create a map specifier */

   map = enter_symbol(SETL_SYSTEM
                      NULL,
                      unit_proctab_ptr,
                      &(root->ast_file_pos));

   map->st_type = sym_id;
   map->st_has_lvalue = YES;
   map->st_has_rvalue = YES;

   /* if each case value is constant, generate a conditional branch */

   if (can_bypass) {

      bypass_label = next_label++;
      emitiss(q_gone,
              bypass_label,
              map,
              sym_omega,
              &(root->ast_file_pos));

   }

   /* now we make a map with value / label pairs */

   when_label = first_label;
   map_card = 0;
   operand_num = 0;
   for (when_ptr = ((root->ast_child.ast_child_ast)->ast_next)->
                                        ast_child.ast_child_ast;
        when_ptr != NULL;
        when_ptr = when_ptr->ast_next) {

      /* make a symbol table entry for the label */

      pair_operand[1] = enter_symbol(SETL_SYSTEM
                                     NULL,
                                     curr_proctab_ptr,
                                     &(root->ast_file_pos));

      pair_operand[1]->st_has_lvalue = YES;
      pair_operand[1]->st_has_rvalue = YES;
      pair_operand[1]->st_is_initialized = YES;
      pair_operand[1]->st_type = sym_label;
      pair_operand[1]->st_aux.st_label_num = when_label++;

      /* loop over the values for this when clause */

      for (case_ptr = (when_ptr->ast_child.ast_child_ast)->
                                               ast_child.ast_child_ast;
           case_ptr != NULL;
           case_ptr = case_ptr->ast_next) {

         /* we push at most three arguments per instruction */

         if (operand_num == 3) {

            emit(q_push3,
                 operand[0],
                 operand[1],
                 operand[2],
                 &(root->ast_file_pos));

            /* free any temporaries allocated for arguments */

            for (operand_num = 0; operand_num < 3; operand_num++) {

               if (operand[operand_num]->st_is_temp)
                  free_temp(operand[operand_num]);

            }

            operand_num = 0;

         }

         /* evaluate the case value */

         pair_operand[0] = gen_expression(SETL_SYSTEM case_ptr,NULL);

         emit(q_push2,
              pair_operand[0],
              pair_operand[1],
              NULL,
              &(root->ast_file_pos));

         if (pair_operand[0]->st_is_temp)
            free_temp(pair_operand[0]);

         get_temp(pair_operand[0]);
         emit(q_tuple,
              pair_operand[0],
              sym_two,
              NULL,
              &(root->ast_file_pos));

         operand[operand_num] = pair_operand[0];
         operand_num++;
         map_card++;

      }
   }

   /* push whatever pairs we've accumulated */

   if (operand_num == 1) {
      emit(q_push1,operand[0],NULL,NULL,&(root->ast_file_pos));
   }
   else if (operand_num == 2) {
      emit(q_push2,operand[0],operand[1],NULL,&(root->ast_file_pos));
   }
   else if (operand_num == 3) {
      emit(q_push3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
   }

   /* free any temporaries allocated for pairs */

   while (operand_num--) {

      if (operand[operand_num]->st_is_temp)
         free_temp(operand[operand_num]);

   }

   /* we need to have a symbol table pointer as second argument */

   sprintf(map_card_string,"%ld",map_card);
   namtab_ptr = get_namtab(SETL_SYSTEM map_card_string);

   /* if we don't find it, make a literal item */

   if (namtab_ptr->nt_symtab_ptr == NULL) {

      namtab_ptr->nt_token_class = tok_literal;
      namtab_ptr->nt_token_subclass = tok_integer;
      operand[1] = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                unit_proctab_ptr,
                                &(root->ast_file_pos));

      operand[1]->st_has_rvalue = YES;
      operand[1]->st_is_initialized = YES;
      operand[1]->st_type = sym_integer;
      operand[1]->st_aux.st_integer_ptr = char_to_int(SETL_SYSTEM map_card_string);

   }
   else {

      operand[1] = namtab_ptr->nt_symtab_ptr;

   }

   /* emit the set instruction to make the map */

   emit(q_set,
        map,
        operand[1],
        NULL,
        &(root->ast_file_pos));

   emit(q_smap,
        map,
        NULL,
        NULL,
        &(root->ast_file_pos));

   /* set the location of the case branch */

   if (can_bypass) {

      emitiss(q_label,bypass_label,NULL,NULL,&(root->ast_file_pos));

   }

   /* now generate a case branch */

   operand[1] = gen_expression(SETL_SYSTEM root->ast_child.ast_child_ast,NULL);
   default_label = next_label++;

   get_temp(operand[2]);

   emit(q_of1,
        operand[2],
        map,
        operand[1],
        &(root->ast_file_pos));

   emitiss(q_goeq,
           default_label,
           operand[2],
           sym_omega,
           &(root->ast_file_pos));

   emit(q_goind,
        operand[2],
        NULL,
        NULL,
        &(root->ast_file_pos));

   /* free any temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }
   if (operand[2]->st_is_temp) {
      free_temp(operand[2]);
   }

   /*
    *  At this point we've finished the case header.  We have to generate
    *  code for each when clause.
    */

   when_label = first_label;
   done_label = next_label++;

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* loop over the when clauses */

   for (when_ptr = ((root->ast_child.ast_child_ast)->ast_next)->
                                                ast_child.ast_child_ast;
        when_ptr != NULL;
        when_ptr = when_ptr->ast_next) {

      /* set the clause's label */

      emitiss(q_label,when_label,NULL,NULL,&(root->ast_file_pos));
      when_label++;

      /* generate the list of expressions */

      operand[0] =
         gen_expression(SETL_SYSTEM (when_ptr->ast_child.ast_child_ast)->ast_next,
                        operand[0]);

      /* by default, go past the end of the case */

      emitiss(q_go,done_label,NULL,NULL,&(root->ast_file_pos));

   }

   /* generate code for the default clause */

   emitiss(q_label,default_label,NULL,NULL,&(root->ast_file_pos));

   operand[0] =
      gen_expression(SETL_SYSTEM root->ast_child.ast_child_ast->ast_next->ast_next,
                     operand[0]);

   /* finally, set the end of case label */

   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   return operand[0];

}

/*\
 *  \ast{ast\_guard}{guard expressions}
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
 *  Case and select expressions are probably the most complex statements
 *  to encode.  Generally, we would like to build a set with value /
 *  label pairs.  Then we evaluate the discriminant, and use a set
 *  reference to branch to its corresponding label.  Since we allow
 *  non-constant expressions in cases, we must form the set on the fly.
 *  If we happen to have constant expressions, we only form the set once.
\*/

symtab_ptr_type gen_expr_guard(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type when_ptr;                 /* when clause pointer               */
symtab_ptr_type operand[3];            /* array of operands                 */
int operand_num;                       /* current operand number            */
int bypass_label;                      /* bypass set building               */
int first_label;                       /* first when clause label           */
int when_label;                        /* when clause label                 */
int loop_label;                        /* when clause label                 */
int true_label;                        /* when clause label                 */
int default_label;                     /* otherwise label                   */
int done_label;                        /* end of case label                 */
int32 set_card;                         /* cardinality of set                */
char set_card_string[20];              /* string representation of above    */
namtab_ptr_type namtab_ptr;            /* name pointer to set size          */
symtab_ptr_type set;                   /* case set                          */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /*
    *  First, we examine each case value, allocating labels for each when
    *  clause.
    */

   first_label = next_label;
   for (when_ptr = (root->ast_child.ast_child_ast)->ast_child.ast_child_ast;
        when_ptr != NULL;
        when_ptr = when_ptr->ast_next) {

      next_label++;

   }

   /* create a set specifier */

   set = enter_symbol(SETL_SYSTEM
                      NULL,
                      unit_proctab_ptr,
                      &(root->ast_file_pos));

   set->st_type = sym_id;
   set->st_has_lvalue = YES;
   set->st_has_rvalue = YES;

   /* only create the set once */

   bypass_label = next_label++;
   emitiss(q_gone,
           bypass_label,
           set,
           sym_omega,
           &(root->ast_file_pos));

   /* now we make a set of labels */

   when_label = first_label;
   set_card = 0;
   operand_num = 0;
   for (when_ptr = (root->ast_child.ast_child_ast)->ast_child.ast_child_ast;
        when_ptr != NULL;
        when_ptr = when_ptr->ast_next) {

      /* make a symbol table entry for the label */

      operand[operand_num] = enter_symbol(SETL_SYSTEM
                                          NULL,
                                          curr_proctab_ptr,
                                          &(root->ast_file_pos));

      operand[operand_num]->st_has_lvalue = YES;
      operand[operand_num]->st_has_rvalue = YES;
      operand[operand_num]->st_is_initialized = YES;
      operand[operand_num]->st_type = sym_label;
      operand[operand_num]->st_aux.st_label_num = when_label++;

      /* we push at most three arguments per instruction */

      if (operand_num == 2) {

         emit(q_push3,
              operand[0],
              operand[1],
              operand[2],
              &(root->ast_file_pos));

         /* free any temporaries allocated for arguments */

         for (operand_num = 0; operand_num < 3; operand_num++) {

            if (operand[operand_num]->st_is_temp)
               free_temp(operand[operand_num]);

         }

         operand_num = -1;

      }

      operand_num++;
      set_card++;

   }

   /* push whatever labels we've accumulated */

   if (operand_num == 1) {
      emit(q_push1,operand[0],NULL,NULL,&(root->ast_file_pos));
   }
   else if (operand_num == 2) {
      emit(q_push2,operand[0],operand[1],NULL,&(root->ast_file_pos));
   }
   else if (operand_num == 3) {
      emit(q_push3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
   }

   /* free any temporaries allocated for pairs */

   while (operand_num--) {

      if (operand[operand_num]->st_is_temp)
         free_temp(operand[operand_num]);

   }

   /* we need to have a symbol table pointer as second argument */

   sprintf(set_card_string,"%ld",set_card);
   namtab_ptr = get_namtab(SETL_SYSTEM set_card_string);

   /* if we don't find it, make a literal item */

   if (namtab_ptr->nt_symtab_ptr == NULL) {

      namtab_ptr->nt_token_class = tok_literal;
      namtab_ptr->nt_token_subclass = tok_integer;
      operand[1] = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                unit_proctab_ptr,
                                &(root->ast_file_pos));

      operand[1]->st_has_rvalue = YES;
      operand[1]->st_is_initialized = YES;
      operand[1]->st_type = sym_integer;
      operand[1]->st_aux.st_integer_ptr = char_to_int(SETL_SYSTEM set_card_string);

   }
   else {

      operand[1] = namtab_ptr->nt_symtab_ptr;

   }

   /* emit the set instruction to make the set */

   emit(q_set,
        set,
        operand[1],
        NULL,
        &(root->ast_file_pos));

   /* set the location of the case branch */

   emitiss(q_label,bypass_label,NULL,NULL,&(root->ast_file_pos));

   /* generate the top of the loop */

   loop_label = next_label++;
   default_label = next_label++;
   done_label = next_label++;

   get_temp(operand[1]);
   get_temp(operand[2]);
   emitssi(q_iter,
           operand[2],
           set,
           it_single,
           &(root->ast_file_pos));

   emitiss(q_label,
           loop_label,
           NULL,
           NULL,
           &(root->ast_file_pos));

   emitssi(q_inext,
           operand[1],
           operand[2],
           default_label,
           &(root->ast_file_pos));

   emit(q_goind,
        operand[1],
        NULL,
        NULL,
        &(root->ast_file_pos));

   /*
    *  At this point we've finished the case header.  We have to generate
    *  code for each when clause.
    */

   when_label = first_label;

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* loop over the when clauses */

   for (when_ptr = (root->ast_child.ast_child_ast)->ast_child.ast_child_ast;
        when_ptr != NULL;
        when_ptr = when_ptr->ast_next) {

      /* set the clause's label */

      emitiss(q_label,when_label,NULL,NULL,&(root->ast_file_pos));
      when_label++;
      true_label = next_label++;

      /* generate the list of expressions */

      gen_boolean(SETL_SYSTEM when_ptr->ast_child.ast_child_ast,
                  true_label,
                  loop_label,
                  true_label);

      emitiss(q_label,
              true_label,
              NULL,
              NULL,
              &(root->ast_file_pos));

      operand[0] =
         gen_expression(SETL_SYSTEM (when_ptr->ast_child.ast_child_ast)->ast_next,
                        operand[0]);

      /* by default, go past the end of the case */

      emitiss(q_go,done_label,NULL,NULL,&(root->ast_file_pos));

   }

   /* generate code for the default clause */

   emitiss(q_label,default_label,NULL,NULL,&(root->ast_file_pos));

   operand[0] =
      gen_expression(SETL_SYSTEM root->ast_child.ast_child_ast->ast_next,operand[0]);

   /* finally, set the end of case label */

   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   /* free any temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }
   if (operand[2]->st_is_temp) {
      free_temp(operand[2]);
   }

   return operand[0];

}


/*\
 *  \ast{ast\_call}{procedure calls}
 *     {\astnode{ast\_call}
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
 *  This function generates code for a procedure call.  We depend on the
 *  semantic check module to do any error checking.
\*/

symtab_ptr_type gen_expr_call(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type arg_ptr;                  /* pointer to argument               */
int arg_count;                         /* number of arguments               */
proctab_ptr_type proctab_ptr;          /* procedure being called            */
symtab_ptr_type formal_ptr;            /* formal parameter pointer          */
int formal_num;                        /* formal number                     */
symtab_ptr_type operand[3];            /* array of operands                 */
ast_ptr_type optree[3];                /* operand trees                     */
symtab_ptr_type return_ptr;            /* returned value                    */
static ast_ptr_type *arg_stack = NULL; /* argument stack                    */
ast_ptr_type *old_arg_stack;           /* previous argument stack           */
static int arg_stack_size = 0;         /* size of argument stack            */
static int arg_stack_top = -1;         /* top of argument stack             */
int arg_stack_base = arg_stack_top;    /* base for this nesting level       */
int opnd_num;                          /* temporary looping variable        */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* count the arguments */

   arg_count = 0;
   for (arg_ptr = right_ptr->ast_child.ast_child_ast;
        arg_ptr != NULL;
        arg_ptr = arg_ptr->ast_next) {

      arg_count++;

   }

   /*
    *  We actually have two distinct procedures here.  The first handles
    *  calls to literal procedures.  The difference is that calls to
    *  literal procedures must be able to handle write parameters.
    */

   if (left_ptr->ast_type == ast_symtab &&
       ((left_ptr->ast_child.ast_symtab_ptr)->st_type == sym_procedure ||
        (left_ptr->ast_child.ast_symtab_ptr)->st_type == sym_method)) {

      /* pick out the procedure record */

      proctab_ptr =
         (left_ptr->ast_child.ast_symtab_ptr)->st_aux.st_proctab_ptr;

      /* push all arguments on the stack */

      formal_ptr = proctab_ptr->pr_symtab_head;
      formal_num = 1;
      opnd_num = 0;
      for (arg_ptr = right_ptr->ast_child.ast_child_ast;
           arg_ptr != NULL;
           arg_ptr = arg_ptr->ast_next, formal_num++) {

         /* we push at most three arguments per instruction */

         if (opnd_num == 3) {

            emit(q_push3,
                 operand[0],
                 operand[1],
                 operand[2],
                 &(root->ast_file_pos));

            /* free any temporaries allocated for arguments */

            for (opnd_num = 0; opnd_num < 3; opnd_num++) {

               if (operand[opnd_num]->st_is_temp)
                  free_temp(operand[opnd_num]);

            }

            opnd_num = 0;

         }

         /* if the parameter is readable, push it */

         if (formal_ptr->st_is_rparam) {
            operand[opnd_num] = gen_expression(SETL_SYSTEM arg_ptr,NULL);
         }
         else {
            operand[opnd_num] = sym_omega;
         }

         opnd_num++;

         /* if the parameter is write ... */

         if (formal_ptr->st_is_wparam) {

            /* push on the argument stack */

            arg_stack_top++;
            if (arg_stack_top == arg_stack_size) {

               /* expand the table */

               old_arg_stack = arg_stack;
               arg_stack = (ast_ptr_type *)malloc((size_t)(
                           (arg_stack_size + 20) * sizeof(ast_ptr_type)));
               if (arg_stack == NULL)
                  giveup(SETL_SYSTEM msg_malloc_error);

               /* copy the old table to the new table */

               if (arg_stack_size > 0) {

                  memcpy((void *)arg_stack,
                         (void *)old_arg_stack,
                         (size_t)(arg_stack_size * sizeof(ast_ptr_type)));

                  free(old_arg_stack);

               }

               arg_stack_size += 20;

            }

            arg_stack[arg_stack_top] = arg_ptr;

         }

         /* set up for the next parameter */

         if (formal_num < proctab_ptr->pr_formal_count)
            formal_ptr = formal_ptr->st_thread;

      }

      /* push whatever arguments we've accumulated */

      if (opnd_num == 1) {
         emit(q_push1,operand[0],NULL,NULL,&(root->ast_file_pos));
      }
      else if (opnd_num == 2) {
         emit(q_push2,operand[0],operand[1],NULL,&(root->ast_file_pos));
      }
      else if (opnd_num == 3) {
         emit(q_push3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
      }

      /* free any temporaries allocated for arguments */

      while (opnd_num--) {

         if (operand[opnd_num]->st_is_temp)
            free_temp(operand[opnd_num]);

      }

      /*
       *  allocate a temporary if we did't get a target, or there are
       *  write parameters
       */

      if (arg_stack_top > arg_stack_base || target == NULL) {
         get_temp(return_ptr);
      }
      else {
         return_ptr = target;
      }

      /* operand 1 is the procedure to call */

      operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);

      emitssi(q_lcall,
              return_ptr,
              operand[1],
              arg_count,
              &(root->ast_file_pos));

      /* free temporaries */

      if (operand[1]->st_is_temp) {
         free_temp(operand[1]);
      }

      /* pop any write parameters */

      opnd_num = 0;
      for (; arg_stack_top > arg_stack_base; arg_stack_top--) {

         /* we pop at most three arguments per instruction */

         if (opnd_num == 3) {

            emit(q_pop3,
                 operand[0],
                 operand[1],
                 operand[2],
                 &(root->ast_file_pos));

            /* generate lhs assingments */

            for (opnd_num = 0; opnd_num < 3; opnd_num++) {

               if (optree[opnd_num] != NULL) {
                  gen_lhs(SETL_SYSTEM optree[opnd_num],operand[opnd_num]);
                  free_temp(operand[opnd_num]);
               }
            }

            opnd_num = 0;

         }

         /* pile up operands to be popped */

         if (arg_stack[arg_stack_top]->ast_type == ast_symtab) {
            operand[opnd_num] =
               arg_stack[arg_stack_top]->ast_child.ast_symtab_ptr;
            optree[opnd_num] = NULL;
         }
         else {
            get_temp(operand[opnd_num]);
            optree[opnd_num] = arg_stack[arg_stack_top];
         }

         opnd_num++;

      }

      /* pop whatever arguments we've accumulated */

      if (opnd_num == 1) {
         emit(q_pop1,operand[0],NULL,NULL,&(root->ast_file_pos));
      }
      else if (opnd_num == 2) {
         emit(q_pop2,operand[0],operand[1],NULL,&(root->ast_file_pos));
      }
      else if (opnd_num == 3) {
         emit(q_pop3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
      }

      while(opnd_num--) {

         if (optree[opnd_num] != NULL) {
            gen_lhs(SETL_SYSTEM optree[opnd_num],operand[opnd_num]);
            free_temp(operand[opnd_num]);
         }
      }

      /* if we have a return pointer, but didn't set it, do so now */

      if (target != NULL && return_ptr != target) {

         emit(q_assign,
              target,
              return_ptr,
              NULL,
              &(root->ast_file_pos));
         free_temp(return_ptr);

         return_ptr = target;

      }

      return return_ptr;

   }

   /*
    *  Now we handle the simpler case, where the procedure is not a
    *  literal.  We don't worry about parameter modes -- all are
    *  read-only.
    */

   /* push all arguments on the stack */

   opnd_num = 0;
   for (arg_ptr = right_ptr->ast_child.ast_child_ast;
        arg_ptr != NULL;
        arg_ptr = arg_ptr->ast_next) {

      /* we push at most three arguments per instruction */

      if (opnd_num == 3) {

         emit(q_push3,
              operand[0],
              operand[1],
              operand[2],
              &(root->ast_file_pos));

         /* free any temporaries allocated for arguments */

         for (opnd_num = 0; opnd_num < 3; opnd_num++) {

            if (operand[opnd_num]->st_is_temp)
               free_temp(operand[opnd_num]);

         }

         opnd_num = 0;

      }

      operand[opnd_num] = gen_expression(SETL_SYSTEM arg_ptr,NULL);
      opnd_num++;

   }

   /* push whatever arguments we've accumulated */

   if (opnd_num == 1) {
      emit(q_push1,operand[0],NULL,NULL,&(root->ast_file_pos));
   }
   else if (opnd_num == 2) {
      emit(q_push2,operand[0],operand[1],NULL,&(root->ast_file_pos));
   }
   else if (opnd_num == 3) {
      emit(q_push3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
   }

   /* free any temporaries allocated for arguments */

   while (opnd_num--) {

      if (operand[opnd_num]->st_is_temp)
         free_temp(operand[opnd_num]);

   }

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(return_ptr);
   }
   else {
      return_ptr = target;
   }

   /* generate procedure call */

   operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   emitssi(q_call,
           return_ptr,
           operand[1],
           arg_count,
           &(root->ast_file_pos));

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }

   return return_ptr;

}

/*\
 *  \astnotree{{\em error}}{error node}
 *
 *  This function is invoked when we find an ast type which should not
 *  occur.
\*/

symtab_ptr_type gen_expr_error(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type discard_root;             /* eliminate compiler warning        */
symtab_ptr_type discard_target;        /* eliminate compiler warning        */

   discard_root = root;
   discard_target = target;

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

#ifdef TRAPS

   trap(__FILE__,__LINE__,msg_bad_ast_node,root->ast_type);

#endif

   return NULL;

}

/*\
 *  \astnotree{{\em error}}{error node}
 *
 *  This function is invoked when we find an ast type which should not
 *  occur.
\*/

symtab_ptr_type gen_expr_from(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* evaluate the operands */

   if (left_ptr->ast_type != ast_symtab) {
      get_temp(operand[1]);
   }
   else {
      operand[1] = left_ptr->ast_child.ast_symtab_ptr;
   }
   operand[2] = gen_expression(SETL_SYSTEM right_ptr,NULL);

   /* emit the instruction */

   emit(ast_default_opcode[root->ast_type],
        operand[0],
        operand[1],
        operand[2],
        &(root->ast_file_pos));

   /* if the operands are not simple variables, generate assignments */

   if (right_ptr->ast_type != ast_symtab) {
      gen_lhs(SETL_SYSTEM right_ptr,operand[2]);
   }

   if (left_ptr->ast_type != ast_symtab) {
      gen_lhs(SETL_SYSTEM left_ptr,operand[1]);
   }

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }
   if (operand[2]->st_is_temp) {
      free_temp(operand[2]);
   }

   return operand[0];

}

/*\
 *  \ast{ast\_initobj}{object initialization}
 *     {\astnode{ast\_initobj}
 *        {\astnode{{\em class}}
 *           {\etcast}
 *           {\astnode{{\em InitObj}}
 *              {\etcast}
 *              {\astnode{{\em Create}}
 *                 {\etcast}
 *                 {\labelast{{\em arguments}}}
 *              }
 *           }
 *        }
 *        {\etcast}
 *     }
 *
 *  This node corresponds to an object creation, which is syntactically
 *  like a procedure call except that the procedure is actually a class
 *  name.  The goal here is to generate stack pushes for all the
 *  arguments followed by a three or four instruction sequence, which
 *  looks like this:
 *
 *  \begin{verbatim}
 *     q_initobj      class       NULL          NULL
 *     q_lcall        NULL        InitObj       0
 *     q_lcall        NULL        Create        n
 *     q_initend      target      class         NULL
 *  \end{verbatim}
 *
 *  \noindent
 *  where n is the number of arguments.  The number of arguments must be
 *  the number expected by the {\em Create} procedure.
 *
 *  Notice that we can avoid a few calls to \verb"gen_expression" here,
 *  since we know in advance that the class and initialization procedures
 *  are symbol table pointers.
\*/

symtab_ptr_type gen_expr_initobj(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type class_ptr;                /* class of object                   */
ast_ptr_type init_ptr;                 /* initialization method             */
ast_ptr_type create_ptr;               /* creation method                   */
ast_ptr_type arg_list_ptr;             /* argument list                     */
ast_ptr_type arg_ptr;                  /* pointer to argument               */
int arg_count;                         /* number of arguments               */
symtab_ptr_type operand[3];            /* array of operands                 */
int opnd_num;                          /* temporary looping variable        */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   class_ptr = root->ast_child.ast_child_ast;
   init_ptr = class_ptr->ast_next;
   create_ptr = init_ptr->ast_next;
   arg_list_ptr = create_ptr->ast_next;

   /* push all arguments on the stack */

   arg_count = 0;
   opnd_num = 0;
   for (arg_ptr = arg_list_ptr->ast_child.ast_child_ast;
        arg_ptr != NULL;
        arg_ptr = arg_ptr->ast_next) {

      /* we push at most three arguments per instruction */

      if (opnd_num == 3) {

         emit(q_push3,
              operand[0],
              operand[1],
              operand[2],
              &(root->ast_file_pos));

         /* free any temporaries allocated for arguments */

         for (opnd_num = 0; opnd_num < 3; opnd_num++) {

            if (operand[opnd_num]->st_is_temp)
               free_temp(operand[opnd_num]);

         }

         opnd_num = 0;

      }

      operand[opnd_num] = gen_expression(SETL_SYSTEM arg_ptr,NULL);
      opnd_num++;
      arg_count++;

   }

   /* push whatever arguments we've accumulated */

   if (opnd_num == 1) {
      emit(q_push1,operand[0],NULL,NULL,&(root->ast_file_pos));
   }
   else if (opnd_num == 2) {
      emit(q_push2,operand[0],operand[1],NULL,&(root->ast_file_pos));
   }
   else if (opnd_num == 3) {
      emit(q_push3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
   }

   /* free any temporaries allocated for arguments */

   while (opnd_num--) {

      if (operand[opnd_num]->st_is_temp)
         free_temp(operand[opnd_num]);

   }

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* generate call sequence */

   emit(q_initobj,
        class_ptr->ast_child.ast_symtab_ptr,
        NULL,
        NULL,
        &(root->ast_file_pos));

   emitssi(q_lcall,
           NULL,
           init_ptr->ast_child.ast_symtab_ptr,
           0,
           &(root->ast_file_pos));

   if (create_ptr->ast_type != ast_null) {

      emitssi(q_lcall,
              NULL,
              create_ptr->ast_child.ast_symtab_ptr,
              arg_count,
              &(root->ast_file_pos));

   }

   emit(q_initend,
        operand[0],
        class_ptr->ast_child.ast_symtab_ptr,
        NULL,
        &(root->ast_file_pos));

   return operand[0];

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
 *  This node corresponds to a reference to a slot or method value.  We
 *  can not determine at compile time whether it is a slot or method,
 *  since it can depend on the class of object.  We depend on the
 *  interpreter to do that for us.  The code generation is pretty
 *  straightforward, we just generate a slot reference instruction.
\*/

symtab_ptr_type gen_expr_slot(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* operand 1 is the object */

   operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   emit(q_slot,
        operand[0],
        operand[1],
        right_ptr->ast_child.ast_symtab_ptr,
        &(root->ast_file_pos));

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }

   return operand[0];

}

/*\
 *  \ast{ast\_slotof}{slot or method call}
 *     {\astnode{ast\_slotof}
 *        {\astnode{ast\_slot}
 *           {\astnode{{\em object}}
 *              {\etcast}
 *              {\astnode{{\em slot name}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *           {\etcast}
 *        }
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
 *     }
 *
 *  This node corresponds to a syntactic structure like this:
 *
 *  \begin{verbatim}
 *     object.slot(...)
 *  \end{verbatim}
 *
 *  It is probably a call to a method, but it might be a reference to a
 *  map-valued instance variable, or even a call to a procedure-valued
 *  instance variable.  We will generate the following strange sequence
 *  of instructions:
 *
 *  \begin{verbatim}
 *     q_slotof     target     slotname     n
 *     q_noop       object     temp         firstarg
 *     q_of1        target     temp         firstarg
 *     q_of         target     temp         n
 *  \end{verbatim}
 *
 *  \noindent
 *  Only one of the last two instructions will be generated.  If we have
 *  one argument, we will generate a \verb"q_of1", and otherwise a
 *  \verb"q_of1".  If we use a \verb"q_of" we will push all the arguments
 *  on the stack first.
 *
 *  At run time, the interpreter will check whether slotname is an
 *  instance variable or method.  If a method, it will call the method
 *  and skip the following instruction.  If an instance variable, it will
 *  copy the value into temp and execute the following instruction.
\*/

symtab_ptr_type gen_expr_slotof(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type arg_ptr;                  /* pointer to argument               */
int arg_count;                         /* number of arguments               */
symtab_ptr_type operand[3];            /* array of operands                 */
symtab_ptr_type first_arg;             /* first argument                    */
symtab_ptr_type object;                /* object symbol                     */
int opnd_num;                          /* temporary looping variable        */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* count the arguments */

   arg_count = 0;
   for (arg_ptr = right_ptr->ast_child.ast_child_ast;
        arg_ptr != NULL;
        arg_ptr = arg_ptr->ast_next) {

      arg_count++;

   }

   /*
    *  If we have other than one argument, we push them all on the stack.
    */

   if (arg_count != 1) {

      first_arg = NULL;
      opnd_num = 0;
      for (arg_ptr = right_ptr->ast_child.ast_child_ast;
           arg_ptr != NULL;
           arg_ptr = arg_ptr->ast_next) {

         /* we push at most three arguments per instruction */

         if (opnd_num == 3) {

            emit(q_push3,
                 operand[0],
                 operand[1],
                 operand[2],
                 &(root->ast_file_pos));

            /* free any temporaries allocated for arguments */

            for (opnd_num = 0; opnd_num < 3; opnd_num++) {

               if (operand[opnd_num]->st_is_temp)
                  free_temp(operand[opnd_num]);

            }

            opnd_num = 0;

         }

         operand[opnd_num] = gen_expression(SETL_SYSTEM arg_ptr,NULL);
         opnd_num++;

      }

      /* push whatever arguments we've accumulated */

      if (opnd_num == 1) {
         emit(q_push1,operand[0],NULL,NULL,&(root->ast_file_pos));
      }
      else if (opnd_num == 2) {
         emit(q_push2,operand[0],operand[1],NULL,&(root->ast_file_pos));
      }
      else if (opnd_num == 3) {
         emit(q_push3,operand[0],operand[1],operand[2],&(root->ast_file_pos));
      }

      /* free any temporaries allocated for arguments */

      while (opnd_num--) {

         if (operand[opnd_num]->st_is_temp)
            free_temp(operand[opnd_num]);

      }
   }

   /* if we have exactly one argument, evaluate it */

   else {

      first_arg = gen_expression(SETL_SYSTEM right_ptr->ast_child.ast_child_ast,NULL);

   }

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /*
    *  First we generate a 'slotof' operation, which looks like this:
    *
    *     q_slotof     target     slot     #args
    *     q_noop       object     temp     firstarg
    */

   object = gen_expression(SETL_SYSTEM left_ptr->ast_child.ast_child_ast,NULL);

   emitssi(q_slotof,
           operand[0],
           ((left_ptr->ast_child.ast_child_ast)->ast_next)->
               ast_child.ast_symtab_ptr,
           arg_count,
           &(root->ast_file_pos));

   get_temp(operand[1]);

   emit(q_noop,
        object,
        operand[1],
        first_arg,
        &(root->ast_file_pos));

   /*
    *  We follow the 'slotof' with an opcode which will be executed iff
    *  the slot turns out to be an instance variable rather than a
    *  method.
    */

   if (root->ast_type == ast_slotof) {

      if (arg_count == 1) {

         emit(q_of1,
              operand[0],
              operand[1],
              first_arg,
              &(root->ast_file_pos));

      }
      else {

         emitssi(q_of,
                 operand[0],
                 operand[1],
                 arg_count,
                 &(root->ast_file_pos));

      }
   }
   else {

      emitssi(q_call,
              operand[0],
              operand[1],
              arg_count,
              &(root->ast_file_pos));

   }

   /* we might have to replace a left hand side expression */

   if ((left_ptr->ast_child.ast_child_ast)->ast_type != ast_symtab) {
      gen_lhs(SETL_SYSTEM left_ptr->ast_child.ast_child_ast,object);
   }

   /* free temporaries */

   free_temp(operand[1]);
   if (object->st_is_temp) {
      free_temp(object);
   }
   if (first_arg != NULL && first_arg->st_is_temp) {
      free_temp(first_arg);
   }

   return operand[0];

}

/*\
 *  \ast{ast\_menviron}{method with environment}
 *     {\astnode{ast\_menviron}
 *        {\astnode{{\em method}}
 *           {\etcast}
 *           {\nullast}
 *        }
 *        {\etcast}
 *     }
 *
 *  This node saves the current {\em self}, along with the environment.
 *  It is generated only within a class when a method is used in a
 *  value-yielding context.  It is like \verb"ast_penviron", but we are
 *  forced to use it more often, so that {\em self} is saved.
\*/

symtab_ptr_type gen_expr_menviron(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
ast_ptr_type left_ptr;                 /* left child node                   */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* emit the instruction */

   emit(q_menviron,
        operand[0],
        left_ptr->ast_child.ast_symtab_ptr,
        NULL,
        &(root->ast_file_pos));

   return operand[0];

}

/*\
 *  \astnotree{ast\_self}{self copy}
 *
 *  Self is a nullary operator, which just returns the current self.  We
 *  maintain value semantics here, so self is not a pointer, but the
 *  current value (a copy is always made).
\*/

symtab_ptr_type gen_expr_self(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type target)             /* target operand                    */

{
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"EXPR : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* allocate a temporary if we did't get a target */

   if (target == NULL) {
      get_temp(operand[0]);
   }
   else {
      operand[0] = target;
   }

   /* emit the instruction */

   emit(q_self,
        operand[0],
        NULL,
        NULL,
        &(root->ast_file_pos));

   return operand[0];

}

