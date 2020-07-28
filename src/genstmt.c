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
 *  \package{The Statement Code Generator}
 *
 *  The functions in this file handle code generation for statements.
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
 *  \texify{genstmt.h}
 *
 *  \packagebody{Statement Code Generator}
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
#include "c_strngs.h"                    /* string literals                   */
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
 *  \astnotree{ast\_null}{null subtree}
 *
 *  Null subtrees can sometimes be found where we would expect a
 *  statement.  We don't generate any code here.
\*/

void gen_stmt_null(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   return;

}

/*\
 *  \ast{ast\_list}{statement lists}{
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
 *  statement lists.  All we do is loop over the children, generating
 *  code for each statement we find.
\*/

void gen_stmt_list(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type ast_ptr;                  /* used to loop over list            */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* loop over statement list, generating code for each statement */

   for (ast_ptr = root->ast_child.ast_child_ast;
        ast_ptr != NULL;
        ast_ptr = ast_ptr->ast_next) {

      gen_statement(SETL_SYSTEM ast_ptr);

   }

   return;

}

/*\
 *  \astnotree{ast\_symtab}{symbol table pointer}
 *
 *  A symbol used as a statement must be a procedure call, where the
 *  procedure called does not have any parameters.  The semantic check
 *  module does not make a distinction between statements and
 *  expressions, so is unable to recognize this.  We can check here
 *  whether the procedure should have no actual parameters, and generate
 *  a procedure call.
\*/

void gen_stmt_symtab(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
proctab_ptr_type proctab_ptr;          /* procedure table item              */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* we can check the number of parameters if the procedure is a literal */

   if ((root->ast_child.ast_symtab_ptr)->st_type == sym_procedure ||
       (root->ast_child.ast_symtab_ptr)->st_type == sym_method) {

      /* pick out the procedure record */

      proctab_ptr =
         (root->ast_child.ast_symtab_ptr)->st_aux.st_proctab_ptr;

      /* make sure the actual parameters are compatible with the formal */

      if (proctab_ptr->pr_formal_count > 0) {

         error_message(SETL_SYSTEM &(root->ast_file_pos),
                       msg_bad_proc_call);

      }

      /* emit the call */

      emitssi(q_lcall,
              NULL,
              root->ast_child.ast_symtab_ptr,
              0,
              &(root->ast_file_pos));

   }
   else {

      /* emit the call */

      emitssi(q_call,
              NULL,
              root->ast_child.ast_symtab_ptr,
              0,
              &(root->ast_file_pos));

   }

   return;

}

/*\
 *  \ast{ast\_assign}{assignment statements}{
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
 *  Assignment statements are a little unusual.  One easy optimization we
 *  can make here is to look for instances in which the left hand side is
 *  a simple identifier, and use that identifier as the target when
 *  evaluating the expression.
\*/

void gen_stmt_assign(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

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

   /* otherwise emit an assignment */

   else {

      operand[1] = gen_expression(SETL_SYSTEM right_ptr,NULL);

      new_gen_lhs(SETL_SYSTEM left_ptr,operand[1]);

      if (operand[1]->st_is_temp) {
         free_temp(operand[1]);
      }
   }

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
 *  The big problem with assignment operators is that if the target is
 *  indexed, we don't want to evaluate the indices twice.  The case
 *  statements here insure that.
\*/

void gen_stmt_assignop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type left_ptr;                 /* left operand                      */
ast_ptr_type right_ptr;                /* right operand                     */
ast_ptr_type index_ptr;                /* assignment index                  */
ast_ptr_type *index_place;             /* index position                    */
symtab_ptr_type operand[5];            /* array of operands                 */
symtab_ptr_type temp_list, new_temp;   /* list of generated temporaries     */
quad_ptr_type *qold;
quad_ptr_type r,s,t,t2;
symtab_ptr_type final_dest;
ast_ptr_type save1,save2;
int lvof;
int numq,i,optok,opcode_save;

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif
  
   optok = NO; /* We don't optimize by default */

   /* pick out child pointers, for readability */

   right_ptr = root->ast_child.ast_child_ast;

   /* compute the nesting level */

   lvof = 0;
   for (left_ptr = right_ptr->ast_child.ast_child_ast;
        left_ptr != NULL &&
           (left_ptr->ast_type == ast_of ||
            left_ptr->ast_type == ast_ofa);
        left_ptr = left_ptr->ast_child.ast_child_ast) lvof++;

   for (left_ptr = right_ptr->ast_child.ast_child_ast;
        left_ptr->ast_type != ast_symtab;
        left_ptr = left_ptr->ast_child.ast_child_ast);

   final_dest = left_ptr->ast_child.ast_symtab_ptr;

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

      /* We have something like LHS := LHS binop RHS 
       * where RHS is an expression...
       */

      optok=NO;
      qold=emit_quad_tail;

      save1 = right_ptr->ast_child.ast_child_ast;
      save2 = save1->ast_next;

      /* See if we can swap the 2 operands, and generate the RHS first */
      
      if ((OPTIMIZE_ASSOP)&&(save2!=NULL)&&(save2->ast_next==NULL)) {
         right_ptr->ast_child.ast_child_ast=save2;
         save2->ast_next=save1;
         save1->ast_next=NULL;
         operand[0] = gen_expression(SETL_SYSTEM right_ptr,NULL);
         save1->ast_next=save2;
	 save2->ast_next=NULL; 
         right_ptr->ast_child.ast_child_ast=save1;

         /* Now swap also the operands in the binop statement */

         for (r=*qold;r->q_next!=NULL; r=r->q_next);

         operand[3]=r->q_operand[2].q_symtab_ptr;
         r->q_operand[2]=r->q_operand[1];
         r->q_operand[1].q_symtab_ptr=operand[3];

         optok=YES;

      } else {

          optok=NO; 
          operand[0] = gen_expression(SETL_SYSTEM right_ptr,NULL);
      }

      if ((optok==YES)&&(OPTIMIZE_ASSOP)) { 

         optok = NO; 
         if ((*qold!=NULL)&&(lvof>=1)) { 

            numq=1;
            for (r=*qold;r->q_next!=NULL; r=r->q_next) numq++;

            r=*qold;
            for (i=1;i<=(numq-lvof-1);i++) r=r->q_next; 
 
            /* r is now pointing to the first q_of or q_ofa */

            s=r;optok=YES;
            if (final_dest!=s->q_operand[1].q_symtab_ptr)
               optok=NO;
            else {
               for (i=1;i<=lvof;i++) {
                  if ((i<lvof)&&(s->q_opcode!=q_of1)&&(s->q_opcode!=q_ofa)) 
                     optok=NO;
                  s=s->q_next;
               }
            }

            t=s; /* Save a pointer (to the binop instruction) */

            if ((optok==YES)&&(lvof>1)) {
               s=r;
               for (i=1;i<=lvof;i++) {
                  emit(q_assign,
                       s->q_operand[0].q_symtab_ptr,
                       sym_omega,
                       NULL,
                       &(root->ast_file_pos));
  
                  s=s->q_next;
               }


               /* Keep a copy of the RHS because t->q_operand[2] has
                * been freed at a lower level 
                */

	       if (operand[0]->st_is_temp) {
                  get_temp(operand[3]);
                  emit(q_assign,
                       operand[3],
                       t->q_operand[2].q_symtab_ptr,
                       NULL,
                       &(root->ast_file_pos));

               } else operand[3]=t->q_operand[2].q_symtab_ptr;

               /* Now we copy the LHS */

               t->q_operand[2].q_symtab_ptr=NULL;
               opcode_save=t->q_opcode;
               t->q_opcode=q_assign;
  
            }

         }
      }

      if (optok) {
        
         /* Call the optimized sinister assignment */

         new_gen_lhs(SETL_SYSTEM left_ptr,operand[0]);
         if ((COMPILER_OPTIONS & VERBOSE_OPTIMIZER)>0) {
            fprintf(stdout,"[%d:%d]",root->ast_file_pos.fp_line,
                                root->ast_file_pos.fp_column);
            fprintf(stdout,"     Optimized ASSIGNOP\n");
         }

         if (lvof==1) { 
            s=t->q_next;
            /* s points to q_sofa or q_sof */
            /* t points to the binop instruction */

            /* Save the temporary (is present) so that we
             * can clear it later
             */

            operand[4] = s->q_operand[2].q_symtab_ptr;
            if (!operand[4]->st_is_temp)
               operand[4] = NULL;


            if (operand[4]!=NULL) {
                emit(q_assign,
                     operand[4],
                     sym_omega,
                     NULL,
                     &(root->ast_file_pos));
            }

         } else {
            /* Now look for the last kof1 or kofa */
            s=t; 
            t2=NULL;
            while (s!=NULL)  {
               if ((s->q_opcode==q_kof1)||(s->q_opcode==q_kofa))
                  t2=s;
               s=s->q_next;
            }
            /* The t2->next statement should be a q_sof or q_sofa */

            qold=emit_quad_tail;
            emit_quad_tail=&t2->q_next;
            t2=t2->q_next;
            if (t2->q_opcode==q_sof)  {
               emit(q_erase,
                    t2->q_operand[0].q_symtab_ptr,
                    t2->q_operand[1].q_symtab_ptr,
                    sym_omega,
                    &(root->ast_file_pos));
            } else {
               emit(q_erase,
                    t2->q_operand[0].q_symtab_ptr,
                    t2->q_operand[1].q_symtab_ptr,
                    sym_omega,
                    &(root->ast_file_pos));
            }

            emit(opcode_save,
                 t->q_operand[0].q_symtab_ptr,
                 t->q_operand[0].q_symtab_ptr,
                 operand[3],
                 &(root->ast_file_pos));

            if (operand[0]->st_is_temp) {
               free_temp(operand[3]);
            }
      
            *emit_quad_tail=t2;
            emit_quad_tail=qold;
         }

      } else  { /* if optok==NO */

         if ((OPTIMIZE_ASSOP) && ((COMPILER_OPTIONS & VERBOSE_OPTIMIZER)>0)) {
            fprintf(stdout,"[%d:%d]",root->ast_file_pos.fp_line,
                                 root->ast_file_pos.fp_column);
            fprintf(stdout,"     No optimization possible\n");
         }
         gen_lhs(SETL_SYSTEM left_ptr,operand[0]);
      }

      /* free temporaries */

      if (operand[0]->st_is_temp) {
         free_temp(operand[0]);
      }

   }

   /* if we created any temporaries, free them */

   while (temp_list != NULL) {

      new_temp = temp_list;
      temp_list = new_temp->st_name_link;
      new_temp->st_is_temp = YES;
      free_temp(new_temp);

   }

   return;

}

/*\
 *  \ast{ast\_if}{if statement}
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
 *  This function handles if statements.
\*/

void gen_stmt_if(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type true_ptr, false_ptr;      /* right child node                  */
int true_label;                        /* start next iteration              */
int false_label;                       /* follows condition check           */
int done_label;                        /* break out of loop                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   true_ptr = left_ptr->ast_next;
   false_ptr = true_ptr->ast_next;

   /* we need three labels associated with the statement */

   true_label = next_label++;
   false_label = next_label++;
   done_label = next_label++;

   /* evaluate the condition */

   gen_boolean(SETL_SYSTEM left_ptr,true_label,false_label,true_label);

   /* generate the true code */

   emitiss(q_label,true_label,NULL,NULL,&(root->ast_file_pos));
   gen_statement(SETL_SYSTEM true_ptr);
   emitiss(q_go,done_label,NULL,NULL,&(root->ast_file_pos));

   /* generate the false code */

   emitiss(q_label,false_label,NULL,NULL,&(root->ast_file_pos));
   gen_statement(SETL_SYSTEM false_ptr);
   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   return;

}

/*\
 *  \ast{ast\_while}{while statement}
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
 *  This function generates code for a while statement.
\*/

void gen_stmt_while(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
int loop_label;                        /* start next iteration              */
int start_label;                       /* follows condition check           */
int quit_label;                        /* break out of loop                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* we need three labels associated with the loop */

   loop_label = next_label++;
   start_label = next_label++;
   quit_label = next_label++;

   /* set the top of the loop */

   emitiss(q_label,loop_label,NULL,NULL,&(root->ast_file_pos));

   /* evaluate the condition */

   gen_boolean(SETL_SYSTEM left_ptr,start_label,quit_label,start_label);

   emitiss(q_label,start_label,NULL,NULL,&(root->ast_file_pos));

   /* push the break and cycle label on the loop stack */

   get_lstack(SETL_SYSTEM_VOID);
   lstack[lstack_top].ls_exit_label = quit_label;
   lstack[lstack_top].ls_continue_label = loop_label;
   lstack[lstack_top].ls_return = NULL;

   /* generate code for the body */

   gen_statement(SETL_SYSTEM right_ptr);

   /* branch to top of loop */

   emitiss(q_go,loop_label,NULL,NULL,&(root->ast_file_pos));

   /* generate the end of loop label */

   emitiss(q_label,quit_label,NULL,NULL,&(root->ast_file_pos));

   /* pop the loop stack */

   lstack_top--;

   return;

}

/*\
 *  \ast{ast\_until}{until statement}
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
 *  This function generates code for a until statement.
\*/

void gen_stmt_until(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
int loop_label;                        /* start next iteration              */
int start_label;                       /* follows condition check           */
int quit_label;                        /* break out of loop                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* we need three labels associated with the loop */

   loop_label = next_label++;
   start_label = next_label++;
   quit_label = next_label++;

   /* set the top of the loop */

   emitiss(q_label,loop_label,NULL,NULL,&(root->ast_file_pos));

   emitiss(q_label,start_label,NULL,NULL,&(root->ast_file_pos));

   /* push the break and cycle label on the loop stack */

   get_lstack(SETL_SYSTEM_VOID);
   lstack[lstack_top].ls_exit_label = quit_label;
   lstack[lstack_top].ls_continue_label = loop_label;
   lstack[lstack_top].ls_return = NULL;

   /* generate code for the body */

   gen_statement(SETL_SYSTEM right_ptr);

   /* evaluate the condition */

   gen_boolean(SETL_SYSTEM left_ptr,quit_label,start_label,quit_label);

   /* generate the end of loop label */

   emitiss(q_label,quit_label,NULL,NULL,&(root->ast_file_pos));

   /* pop the loop stack */

   lstack_top--;

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

void gen_stmt_loop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
int loop_label;                        /* start next iteration              */
int quit_label;                        /* break out of loop                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* we need two labels associated with the loop */

   loop_label = next_label++;
   quit_label = next_label++;

   /* set the top of the loop */

   emitiss(q_label,loop_label,NULL,NULL,&(root->ast_file_pos));

   /* push the break and cycle label on the loop stack */

   get_lstack(SETL_SYSTEM_VOID);
   lstack[lstack_top].ls_exit_label = quit_label;
   lstack[lstack_top].ls_continue_label = loop_label;
   lstack[lstack_top].ls_return = NULL;

   /* generate code for the body */

   gen_statement(SETL_SYSTEM root->ast_child.ast_child_ast);

   /* branch to top of loop */

   emitiss(q_go,loop_label,NULL,NULL,&(root->ast_file_pos));

   /* generate the end of loop label */

   emitiss(q_label,quit_label,NULL,NULL,&(root->ast_file_pos));

   /* pop the loop stack */

   lstack_top--;

   return;

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

void gen_stmt_for(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type iter_list_ptr;            /* list of iterators                 */
ast_ptr_type cond_ptr;                 /* inclusion condition               */
ast_ptr_type stmt_list_ptr;            /* loop body list                    */
c_iter_ptr_type iter_ptr;              /* iterator control record           */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   iter_list_ptr = root->ast_child.ast_child_ast;
   cond_ptr = iter_list_ptr->ast_next;
   stmt_list_ptr = cond_ptr->ast_next;

   /* generate loop initialization */

   iter_ptr = gen_iter_varvals(SETL_SYSTEM iter_list_ptr,cond_ptr);
   get_lstack(SETL_SYSTEM_VOID);
   lstack[lstack_top].ls_exit_label = iter_ptr->it_fail_label;
   lstack[lstack_top].ls_continue_label = iter_ptr->it_loop_label;
   lstack[lstack_top].ls_return = NULL;

   /* generate code for the body */

   gen_statement(SETL_SYSTEM stmt_list_ptr);

   /* set the bottom of the loop */

   gen_iter_bottom(SETL_SYSTEM iter_ptr);

   /* pop the loop stack */

   lstack_top--;

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
 *  Case and select expressions are probably the most complex statements
 *  to encode.  Generally, we would like to build a map with value /
 *  label pairs.  Then we evaluate the discriminant, and use a map
 *  reference to branch to its corresponding label.  Since we allow
 *  non-constant expressions in cases, we must form the map on the fly.
 *  If we happen to have constant expressions, we only form the map once.
\*/

void gen_stmt_case(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

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
int32 map_card;                        /* cardinality of map                */
char map_card_string[20];              /* string representation of above    */
namtab_ptr_type namtab_ptr;            /* name pointer to map size          */
int can_bypass;                        /* YES if can bypass map building    */
symtab_ptr_type map;                   /* case map                          */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

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
        operand[1],
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

   /* loop over the when clauses */

   for (when_ptr = ((root->ast_child.ast_child_ast)->ast_next)->
                                                ast_child.ast_child_ast;
        when_ptr != NULL;
        when_ptr = when_ptr->ast_next) {

      /* set the clause's label */

      emitiss(q_label,when_label,NULL,NULL,&(root->ast_file_pos));
      when_label++;

      /* generate the list of expressions */

      gen_statement(SETL_SYSTEM (when_ptr->ast_child.ast_child_ast)->ast_next);

      /* by default, go past the end of the case */

      emitiss(q_go,done_label,NULL,NULL,&(root->ast_file_pos));

   }

   /* generate code for the default clause */

   emitiss(q_label,default_label,NULL,NULL,&(root->ast_file_pos));

   gen_statement(SETL_SYSTEM root->ast_child.ast_child_ast->ast_next->ast_next);

   /* finally, set the end of case label */

   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   return;

}

#if MPWC
#pragma segment genstmt1
#endif

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

void gen_stmt_guard(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

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
int32 set_card;                        /* cardinality of set                */
char set_card_string[20];              /* string representation of above    */
namtab_ptr_type namtab_ptr;            /* name pointer to set size          */
symtab_ptr_type set;                   /* case set                          */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

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

      gen_statement(SETL_SYSTEM (when_ptr->ast_child.ast_child_ast)->ast_next);

      /* by default, go past the end of the case */

      emitiss(q_go,done_label,NULL,NULL,&(root->ast_file_pos));

   }

   /* generate code for the default clause */

   emitiss(q_label,default_label,NULL,NULL,&(root->ast_file_pos));

   gen_statement(SETL_SYSTEM root->ast_child.ast_child_ast->ast_next);

   /* finally, set the end of case label */

   emitiss(q_label,done_label,NULL,NULL,&(root->ast_file_pos));

   /* free any temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }
   if (operand[2]->st_is_temp) {
      free_temp(operand[2]);
   }

   return;

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

void gen_stmt_call(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type arg_ptr;                  /* pointer to argument               */
int arg_count;                         /* number of arguments               */
proctab_ptr_type proctab_ptr;          /* procedure being called            */
symtab_ptr_type formal_ptr;            /* formal parameter pointer          */
int formal_num;                        /* formal number                     */
ast_ptr_type optree[3];                /* operand trees                     */
symtab_ptr_type operand[3];            /* array of operands                 */
int opnd_num;                          /* temporary looping variable        */
static ast_ptr_type *arg_stack = NULL; /* argument stack                    */
ast_ptr_type *old_arg_stack;           /* previous argument stack           */
static int arg_stack_size = 0;         /* size of argument stack            */
static int arg_stack_top = -1;         /* top of argument stack             */
int arg_stack_base = arg_stack_top;    /* base for this nesting level       */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

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

      /* operand 1 is the procedure to call */

      operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);

      emitssi(q_lcall,
              NULL,
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

      return;

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

   /* generate procedure call */

   operand[1] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   emitssi(q_call,
           NULL,
           operand[1],
           arg_count,
           &(root->ast_file_pos));

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }

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
 *  This case handles return statements.  If we do not have a value to
 *  return, we return OM.
\*/

void gen_stmt_return(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
symtab_ptr_type return_val;            /* value returned from procedure     */
symtab_ptr_type operand[3];            /* array of operands                 */
symtab_ptr_type formal_ptr;            /* formal parameter pointer          */
int formal_num;                        /* formal number                     */
int opnd_num;                          /* temporary looping variable        */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* evaluate the return value */

   if (root->ast_child.ast_child_ast != NULL) {
      return_val = gen_expression(SETL_SYSTEM root->ast_child.ast_child_ast,NULL);
   }
   else {
      return_val = sym_omega;
   }

   /* push write parameters */

   opnd_num = 0;
   for (formal_ptr = curr_proctab_ptr->pr_symtab_head, formal_num = 0;
        formal_num < curr_proctab_ptr->pr_formal_count;
        formal_ptr = formal_ptr->st_thread, formal_num++) {

      /* we push at most three arguments per instruction */

      if (opnd_num == 3) {

         emit(q_push3,
              operand[0],
              operand[1],
              operand[2],
              &(root->ast_file_pos));

         opnd_num = 0;

      }

      if (formal_ptr->st_is_wparam) {

         operand[opnd_num] = formal_ptr;
         opnd_num++;

      }
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

   emit(q_return,return_val,NULL,NULL,&(root->ast_file_pos));

   if (root->ast_child.ast_child_ast != NULL && return_val->st_is_temp)
      free_temp(return_val);

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
 *  This case handles return statements.  If we do not have a value to
 *  return, we return OM.
\*/

void gen_stmt_stop(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   emit(q_stopall,NULL,NULL,NULL,&(root->ast_file_pos));

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
 *  This case handles return statements.  If we do not have a value to
 *  return, we return OM.
\*/

void gen_stmt_exit(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   if (lstack_top >= 0 && lstack[lstack_top].ls_return != NULL) {

      if (root->ast_child.ast_child_ast != NULL) {

         lstack[lstack_top].ls_return =
               gen_expression(SETL_SYSTEM root->ast_child.ast_child_ast,
                              lstack[lstack_top].ls_return);
      }
      else {

         emit(q_assign,
              lstack[lstack_top].ls_return,
              sym_omega,
              NULL,
              &(root->ast_file_pos));

      }
   }

   emitiss(q_go,
           lstack[lstack_top].ls_exit_label,
           NULL,
           NULL,
           &(root->ast_file_pos));

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
 *  This case handles return statements.  If we do not have a value to
 *  return, we return OM.
\*/

void gen_stmt_continue(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   emitiss(q_go,
           lstack[lstack_top].ls_continue_label,
           NULL,
           NULL,
           &(root->ast_file_pos));

   return;

}

/*\
 *  \ast{ast\_assert}{assert statements}
 *     {\astnode{ast\_assert}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\etcast}
 *        }
 *        {\etcast}
 *     }
 *
 *  This function handles assert statements.
\*/

void gen_stmt_assert(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
symtab_ptr_type operand[3];            /* array of operands                 */
char proc_name[MAX_TOK_LEN + 1];       /* procedure name string             */
namtab_ptr_type namtab_ptr;            /* procedure name pointer            */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   operand[0] = gen_expression(SETL_SYSTEM root->ast_child.ast_child_ast,NULL);

   /* we need to have a symbol table pointer for procedure name */

   sprintf(proc_name,"\"%s\"",(curr_proctab_ptr->pr_namtab_ptr)->nt_name);
   namtab_ptr = get_namtab(SETL_SYSTEM proc_name);

   /* if we don't find it, make a literal item */

   if (namtab_ptr->nt_symtab_ptr == NULL) {

      namtab_ptr->nt_token_class = tok_literal;
      namtab_ptr->nt_token_subclass = tok_string;
      operand[1] = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                unit_proctab_ptr,
                                &(root->ast_file_pos));

      operand[1]->st_has_rvalue = YES;
      operand[1]->st_is_initialized = YES;
      operand[1]->st_type = sym_string;
      operand[1]->st_aux.st_string_ptr = char_to_string(SETL_SYSTEM proc_name);

   }
   else {

      operand[1] = namtab_ptr->nt_symtab_ptr;

   }

   /* emit the assertion */

   emitssi(q_assert,
           operand[0],
           operand[1],
           root->ast_file_pos.fp_line,
           &(root->ast_file_pos));

   return;

}

/*\
 *  \astnotree{{\em error}}{error node}
 *
 *  This function is invoked when we find an AST type which should not
 *  occur.
\*/

void gen_stmt_error(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

#ifdef TRAPS

   trap(__FILE__,__LINE__,msg_bad_ast_node,root->ast_type);

#endif

   return;

}

/*\
 *  \astnotree{{\em error}}{error node}
 *
 *  This function is invoked when we find an AST type which should not
 *  occur.
\*/

void gen_stmt_from(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

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
        NULL,
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
 *  A slot reference in a statement {\em must} be a method call.  We
 *  generate code similar to a \verb"ast_slotof", but forcing a call.
\*/

void gen_stmt_slot(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"STMT : %s\n",ast_desc[root->ast_type]);

   }

#endif

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /*
    *  First we generate a 'slotof' operation, which looks like this:
    *
    *     q_slotof     target     slot     #args
    *     q_noop       object     temp     firstarg
    */

   operand[0] = gen_expression(SETL_SYSTEM left_ptr->ast_child.ast_child_ast,NULL);

   emitssi(q_slotof,
           NULL,
           right_ptr->ast_child.ast_symtab_ptr,
           0,
           &(root->ast_file_pos));

   get_temp(operand[1]);

   emit(q_noop,
        operand[0],
        operand[1],
        NULL,
        &(root->ast_file_pos));
   
   /*
    *  We follow the 'slotcall' with the actual call.  The call will be
    *  executed if the slot name turns out to be an instance variable.
    */

   emitssi(q_call,
           NULL,
           operand[1],
           0,
           &(root->ast_file_pos));

   /* we might have to replace a left hand side expression */

   if ((left_ptr->ast_child.ast_child_ast)->ast_type != ast_symtab) {
      gen_lhs(SETL_SYSTEM left_ptr->ast_child.ast_child_ast,operand[0]);
   }

   /* free temporaries */

   free_temp(operand[1]);
   if (operand[0]->st_is_temp) {
      free_temp(operand[0]);
   }

   return;

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
 *  procedure-valued instance variable We will generate the following
 *  strange sequence of instructions:
 *
 *  \begin{verbatim}
 *     q_slotof     target     slotname     n
 *     q_noop       object     temp         firstarg
 *     q_call       target     temp         n
 *  \end{verbatim}
 *
 *  At run time, the interpreter will check whether slotname is an
 *  instance variable or method.  If a method, it will call the method
 *  and skip the following instruction.  If an instance variable, it will
 *  copy the value into temp and execute the following instruction.
\*/

void gen_stmt_slotof(
   SETL_SYSTEM_PROTO
   ast_ptr_type root)                  /* AST to be encoded                 */

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

   /*
    *  First we generate a 'slotof' operation, which looks like this:
    *
    *     q_slotof     target     slot     #args
    *     q_noop       object     temp     firstarg
    */

   object = gen_expression(SETL_SYSTEM left_ptr->ast_child.ast_child_ast,NULL);

   emitssi(q_slotof,
           NULL,
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

   emitssi(q_call,
           NULL,
           operand[1],
           arg_count,
           &(root->ast_file_pos));

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

   return;

}

