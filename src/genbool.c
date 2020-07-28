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
 *  \package{The Boolean Expression Code Generator}
 *
 *  This function handles generation of code for boolean expressions.
 *  For most expressions that simply involves calling the more general
 *  expression code generator and then generating a conditional branch
 *  based on the result of that expression.  For the comparison and
 *  logical expression operators (which should make up the bulk of
 *  expressions handled here) we generate conditional branch code.
 *
 *  \texify{genbool.h}
 *
 *  \packagebody{Boolean Expression Code Generator}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "symtab.h"                    /* symbol table                      */
#include "ast.h"                       /* abstract syntax tree              */
#include "quads.h"                     /* quadruples                        */
#include "genquads.h"                  /* quadruple generator               */
#include "genstmt.h"                   /* statement code generator          */
#include "genexpr.h"                   /* expression code generator         */
#include "genbool.h"                   /* boolean expression code generator */

/*\
 *  \function{gen\_boolean()}
\*/

void gen_boolean(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   int true_label,                     /* branch location if condition true */
   int false_label,                    /* branch location if condition      */
                                       /* false                             */
   int follow_label)                   /* label following condition         */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"BOOL : %s\n",ast_desc[root->ast_type]);

   }

#endif

   switch (root->ast_type) {

/*\
 *  \astnotree{ast\_null}{null tree}
 *
 *  We sometimes have null conditions, which we consider to be always
 *  satisfied.
\*/

case ast_null :

{

   /* branch to true code */

   if (follow_label != true_label) {

      emitiss(q_go,true_label,NULL,NULL,&(root->ast_file_pos));

   }

   return;

}

/*\
 *  \ast{ast\_lt}{binary predicates}
 *     {\astnode{operator}
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
 *  Binary predicates do have corresponding conditional branch
 *  instructions, so we try to use those to generate efficient code.
\*/

case ast_eq :           case ast_ne :           case ast_lt :
case ast_le :           case ast_gt :           case ast_ge :
case ast_in :           case ast_notin :        case ast_incs :
case ast_subset :

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */
int opcode;                            /* opcode                            */
int label_operand;                     /* conditional branch target         */

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = (root->ast_child.ast_child_ast)->ast_next;

   /* decide whether to branch on true or false */

   if (follow_label == true_label) {

      opcode = ast_false_opcode[root->ast_type];
      label_operand = false_label;

   }
   else {

      opcode = ast_true_opcode[root->ast_type];
      label_operand = true_label;

   }

   /* some operands only have inverted opcodes */

   if (ast_flip_operands[root->ast_type]) {

      operand[2] = gen_expression(SETL_SYSTEM left_ptr,NULL);
      operand[1] = gen_expression(SETL_SYSTEM right_ptr,NULL);

   }
   else {

      operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);
      operand[2] = gen_expression(SETL_SYSTEM right_ptr,NULL);

   }

   /* emit the conditional branch */

   emitiss(opcode,
           label_operand,
           operand[1],
           operand[2],
           &(root->ast_file_pos));

   /* branch over the true code, if the result is false */

   if (follow_label != true_label && follow_label != false_label) {

      emitiss(q_go,false_label,NULL,NULL,&(root->ast_file_pos));

   }

   /* free temporaries */

   if (operand[1]->st_is_temp)
      free_temp(operand[1]);
   if (operand[2]->st_is_temp)
      free_temp(operand[2]);

   return;

}

/*\
 *  \ast{ast\_and}{logical and}
 *     {\astnode{ast\_and}
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
 *  We try to short circuit evaluation of \verb"AND" expressions.  We
 *  evaluate the left hand side first, and if it is false we don't bother
 *  with the right.
\*/

case ast_and :

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
int skip_label;                        /* intermediate label                */

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* generate a label */

   skip_label = next_label++;

   /* evaluate the left expression, quit if it's false */

   gen_boolean(SETL_SYSTEM left_ptr,skip_label,false_label,skip_label);

   /* if we reach this label we have to evaluate the right */

   emitiss(q_label,skip_label,NULL,NULL,&(root->ast_file_pos));

   gen_boolean(SETL_SYSTEM right_ptr,true_label,false_label,follow_label);

   return;

}

/*\
 *  \ast{ast\_or}{logical or}
 *     {\astnode{ast\_or}
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
 *  We try to short circuit evaluation of \verb"OR" expressions.  We
 *  evaluate the left hand side first, and if it is true we don't bother
 *  with the right.
\*/

case ast_or :

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
int skip_label;                        /* intermediate label                */

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* generate a label */

   skip_label = next_label++;

   /* evalueate the left side, quit if it's true */

   gen_boolean(SETL_SYSTEM left_ptr,true_label,skip_label,skip_label);

   /* if we reach this label we have to evaluate the right */

   emitiss(q_label,skip_label,NULL,NULL,&(root->ast_file_pos));

   gen_boolean(SETL_SYSTEM right_ptr,true_label,false_label,follow_label);

   return;

}

/*\
 *  \ast{ast\_not}{logical not}
 *     {\astnode{ast\_not}
 *        {\etcast}
 *        {\etcast}
 *     }
 *
 *  This case handles logical not operations.  We just call this function
 *  recursively reversing the branch labels.
\*/

case ast_not :

{
   gen_boolean(SETL_SYSTEM root->ast_child.ast_child_ast,
               false_label,
               true_label,
               follow_label);

   return;

}

/*\
 *  \astnotree{miscellaneous}{value-returning nodes}
 *
 *  We have a variety of nodes which return a value, but do not have
 *  conditional branch opcodes.  First, we get the value of the
 *  expression.  Then we test the result, and branch according to that
 *  result.
\*/

default :

{
symtab_ptr_type logical_value;         /* return operand                    */

   /* get the value of the expression */

   logical_value = gen_expression(SETL_SYSTEM root,NULL);

   /* branch to true/false code */

   if (follow_label == true_label) {

      emitiss(q_gofalse,
              false_label,
              logical_value,
              NULL,
              &(root->ast_file_pos));

   }
   else {

      emitiss(q_gotrue,
              true_label,
              logical_value,
              NULL,
              &(root->ast_file_pos));

   }

   /* branch over the true code, if the result is false */

   if (follow_label != true_label && follow_label != false_label) {

      emitiss(q_go,false_label,NULL,NULL,&(root->ast_file_pos));

   }

   /* free temporaries */

   if (logical_value->st_is_temp) {
      free_temp(logical_value);
   }

   return;

}

/* at this point we return to normal indentation */

}}
