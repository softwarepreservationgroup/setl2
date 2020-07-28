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
 *  \package{The Left Hand Side Code Generator}
 *
 *  This package handles code generation for sinister assignments.  There
 *  are only a few node types we need to handle here, but since we
 *  overload node types which are also used on the right hand side, we
 *  have to separate this from the other code generation functions.
 *
 *  \texify{genlhs.h}
 *
 *  \packagebody{Left Hand Side Code Generator}
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
#include "genquads.h"                  /* quadruple generator               */
#include "genexpr.h"                   /* expression code generator         */
#include "genlhs.h"                    /* left hand side code generator     */


#ifdef PLUGIN
#define fprintf plugin_fprintf
#endif


/*\
 *  \function{new\_gen\_lhs()}
\*/

void new_gen_lhs(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type source)             /* value to be picked apart          */
{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type arg_ptr;                  /* pointer to argument               */
int32 arg_count;                       /* number of arguments               */
char arg_count_string[20];             /* character version for symtab      */
namtab_ptr_type namtab_ptr;            /* name pointer to argument count    */
symtab_ptr_type operand[3];            /* array of operands                 */
int opnd_num;                          /* temporary looping variable        */
int copied_source;                     /* YES if we copied the source       */
int lvof;                              /* Number of ast_of nestings         */
int numq;
quad_ptr_type *q;
quad_ptr_type *qold;
quad_ptr_type r,s,snew;
symtab_ptr_type final_dest;
int i,optok;

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"NLHS : %s\n",ast_desc[root->ast_type]);

   }
#endif

   if (((root->ast_type==ast_of)||(root->ast_type==ast_ofa))&&
       (!USE_INTERMEDIATE_FILES)&&(OPTIMIZE_OF)) {

      q = emit_quad_tail;  /* This will point to the next pointer of the last
                            * quadruple generated so far...
                            */
      
      /* Now compute the number of nested ast_ofs */

      lvof = 0;
      for (left_ptr = root;
           ((left_ptr->ast_type == ast_of)||( left_ptr->ast_type == ast_ofa));
            left_ptr = left_ptr->ast_child.ast_child_ast) 

         lvof++; 

      /* if the source is our final left hand side, copy it */

      for (left_ptr = root;
           left_ptr->ast_type != ast_symtab;
           left_ptr = left_ptr->ast_child.ast_child_ast);

         final_dest = left_ptr->ast_child.ast_symtab_ptr;

      if (left_ptr->ast_child.ast_symtab_ptr == source) {

         copied_source = YES;
         get_temp(operand[0]);
         emit(q_assign,
              operand[0],
              source,
              NULL,
              &(root->ast_file_pos));

         source = operand[0];
         copied_source = YES;

      }
      else {

         copied_source = NO;

      }

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

      /* if there is more than one argument we make them into a tuple */

      if (arg_count > 1) {

         /* push the arguments on the stack */

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
            emit(q_push1,
                 operand[0],
                 NULL,
                 NULL,
                 &(root->ast_file_pos));
         }
         else if (opnd_num == 2) {
            emit(q_push2,
                 operand[0],
                 operand[1],
                 NULL,
                 &(root->ast_file_pos));
         }
         else if (opnd_num == 3) {
            emit(q_push3,
                 operand[0],
                 operand[1],
                 operand[2],
                 &(root->ast_file_pos));
         }

         /* free any temporaries allocated for arguments */

         while (opnd_num--) {

            if (operand[opnd_num]->st_is_temp)
               free_temp(operand[opnd_num]);

         }

         /* we generate a temporary for the domain element */

         get_temp(operand[1]);

         /* we need to have a symbol table pointer as second argument */

         sprintf(arg_count_string,"%ld",arg_count);
         namtab_ptr = get_namtab(SETL_SYSTEM arg_count_string);

         /* if we don't find it, make a literal item */

         if (namtab_ptr->nt_symtab_ptr == NULL) {

            operand[2] = enter_symbol(SETL_SYSTEM
                                      namtab_ptr,
                                      unit_proctab_ptr,
                                      &(root->ast_file_pos));

            operand[2]->st_has_rvalue = YES;
            operand[2]->st_is_initialized = YES;
            operand[2]->st_type = sym_integer;
            operand[2]->st_aux.st_integer_ptr = char_to_int(SETL_SYSTEM arg_count_string);

         }
         else {

            operand[2] = namtab_ptr->nt_symtab_ptr;

         }

         /* form the tuple */

         emit(q_tuple,
              operand[1],
              operand[2],
              NULL,
              &(root->ast_file_pos));

      }

      /* we have only one argument, so use it as key */

      else {

         operand[1] = gen_expression(SETL_SYSTEM
                                     right_ptr->ast_child.ast_child_ast,NULL);

      }

      /* evaluate the left hand side */

      operand[0] = gen_expression(SETL_SYSTEM
                                  left_ptr,NULL);

      /* emit the sinister assignment */

      if (root->ast_type == ast_of) {

         emit(q_sof,
              operand[0],
              operand[1],
              source,
              &(root->ast_file_pos));

      }
      else {

         emit(q_sofa,
              operand[0],
              operand[1],
              source,
              &(root->ast_file_pos));

      }

      /* free temporaries */

      if (operand[1]->st_is_temp) {
         free_temp(operand[1]);
      }

      /* So far everything has been almost identical to the standard
       * case, in the gen_lhs function. Now we start to check to see
       * if we can fix the code that has been already generated...
       *
       * First we find out if we have generated something, and we 
       * count the number of quadruples. If we didn't emit what we
       * expect here, we set optok to NO...
       *
       * We hope to find object code that looks like this:
       * (q_of could also be q_ofa)
       *
       * (1)  q_of tempxxx,final_dest,X    -> means tempxxx:=final_dest(X)
       *      .............
       *      q_of or q_ofa
       *      q_sof or q_sofa
       *
       */

      if ((*q!=NULL)&&(lvof>1)) {
         numq = 1;
		 /* TOTO look here */
         for (r=*q; r=r->q_next; r!=NULL) numq++;

         /* Now advance to the first q_of q_ofa (see (1) above) */

         r = *q;
         for (i=1;i<=(numq-lvof);i++) r=r->q_next; 

         s = r;
         optok = YES;
         for (i=1;i<=lvof;i++) {
 
            if ((i==1)&&((final_dest!=s->q_operand[1].q_symtab_ptr))) 
               optok = NO;

            if ((i<lvof)&&(s->q_opcode!=q_of1)&&(s->q_opcode!=q_ofa)) 
               optok = NO;
       
            s = s->q_next;
         }

      } else optok = NO; 

      /* At his point we signal that we are analyzing a statement
       * for possible optimization
       */

      if ((COMPILER_OPTIONS & VERBOSE_OPTIMIZER)>0) {
        fprintf(stdout,"[%d:%d]",root->ast_file_pos.fp_line,
                                root->ast_file_pos.fp_column);
      }

      if (optok==NO) {
         if ((COMPILER_OPTIONS & VERBOSE_OPTIMIZER)>0) 
            fprintf(stdout,"     No LHS optimization possible\n");
      } else { 

         /* Generate the new quadruples */

         if ((COMPILER_OPTIONS & VERBOSE_OPTIMIZER)>0) 
            fprintf(stdout,"     Optimized (L=%d)\n",lvof);

         if ((operand[0]->st_is_temp)&&(!copied_source)) {

            qold=emit_quad_tail;  /* Here we save the current
                                   * pointer to the next quadruple 
                                   */
      
            /* Here from the code
             *
             * (1)    q_of tempxxx,final_dest,X 
             *        .............
             * (i-1)  q_of or q_ofa
             * (i)    q_sof or q_sofa
             *
             * We generate 
             *
             * (i-1)  q_sof or q_sofa
             *        .............
             * (1)    q_sof final_dest,X,tempxxx
             *
             *  and the code above is transformed into 
             *
             * (1)    q_kof tempxxx,final_dest,X 
             *        .............
             * (i-1)  q_kof or q_kofa
             * (i)    q_kof or q_kofa
             *
             */

            s = r;
            for (i=1;i<lvof;i++) {
               emit_quad_tail = qold;
               snew = (*qold);
               if (s->q_opcode==q_of1) {
                  emit(q_sof,
                       s->q_operand[1].q_symtab_ptr,
                       s->q_operand[2].q_symtab_ptr,
                       s->q_operand[0].q_symtab_ptr,
                       &(root->ast_file_pos));
               } else  {
                  emit(q_sofa,
                       s->q_operand[1].q_symtab_ptr,
                       s->q_operand[2].q_symtab_ptr,
                       s->q_operand[0].q_symtab_ptr,
                       &(root->ast_file_pos));
               }

               (*qold)->q_next = snew; 

               /* Generate the KILL instruction */

               snew = s->q_next;
               if (s->q_opcode==q_of1) s->q_opcode=q_kof1;
               else if (s->q_opcode==q_ofa) s->q_opcode=q_kofa; 
      
               s = snew;
            }

            /* Now we clear the temporaries used before */
   
            for (s=r;s->q_next!=NULL;s=s->q_next);
            emit_quad_tail = &s->q_next;
            s = r;

            for (i=1;i<lvof;i++) {
               if (operand[0]->st_is_temp) {
                  emit(q_assign,
                       s->q_operand[0].q_symtab_ptr,
                       sym_omega,
                       NULL,
                       &(root->ast_file_pos));
               }
            }
         }
         return;
      }

      /*  Here we continue with the standard code for the 
       *  non-optimized case (optok==NO)
       *
       *  If the target is a temporary, generate another sinister
       *  assignment.
       */

      if (operand[0]->st_is_temp) {

         gen_lhs(SETL_SYSTEM left_ptr,operand[0]);
         free_temp(operand[0]);

      }

      /* if we copied the source, free it */

      if (copied_source) {

         /* assign omega to copied source */

         emit(q_assign,
              source,
              sym_omega,
              NULL,
              &(root->ast_file_pos));
   
         free_temp(source);

      }

      return;

   } else gen_lhs(SETL_SYSTEM root,source);

}


/*\
 *  \function{gen\_lhs()}
\*/

void gen_lhs(
   SETL_SYSTEM_PROTO
   ast_ptr_type root,                  /* AST to be encoded                 */
   symtab_ptr_type source)             /* value to be picked apart          */

{

#ifdef DEBUG

   if (CODE_DEBUG) {

      fprintf(DEBUG_FILE,"LHS : %s\n",ast_desc[root->ast_type]);

   }

#endif

   switch (root->ast_type) {

/*\
 *  \ast{ast\_enum\_tup}{tuple assignment}
 *     {\astnode{ast\_enum\_tup}
 *        {\astnode{ast\_list}
 *           {\astnode{{\em element 1}}
 *              {\etcast}
 *              {\astnode{{\em element 2}}
 *                 {\etcast}
 *                 {\etcast}
 *              }
 *           }
 *           {\nullast}
 *        }
 *        {\nullast}
 *     }
 *
 *  This case handles enumerated tuples which appear in left hand side
 *  contexts.  We extract the elements of the source specifier into the
 *  various variables appearing in the tuple.
\*/

case ast_enum_tup :

{
ast_ptr_type elem_ptr;                 /* pointer to element                */
int32 elem_count;                      /* number of elements                */
char elem_count_string[20];            /* character version for symtab      */
namtab_ptr_type namtab_ptr;            /* name pointer to element count     */
symtab_ptr_type operand[3];            /* array of operands                 */

   /* copy the source (unpleasant, but safe) */

   get_temp(operand[1]);
   emit(q_assign,
        operand[1],
        source,
        NULL,
        &(root->ast_file_pos));

   /* loop over the elements of the tuple */

   elem_count = 0;
   for (elem_ptr = root->ast_child.ast_child_ast;
        elem_ptr != NULL;
        elem_ptr = elem_ptr->ast_next) {

      elem_count++;

      /* skip placeholders */

      if (elem_ptr->ast_type == ast_placeholder)
         continue;

      /* load symbols directly */

      if (elem_ptr->ast_type == ast_symtab) {
         operand[0] = elem_ptr->ast_child.ast_symtab_ptr;
      }

      /* otherwise we load a temporary, and pick it apart later */

      else {
         get_temp(operand[0]);
      }

      /* we need to have a symbol table pointer as third argument */

      sprintf(elem_count_string,"%ld",elem_count);
      namtab_ptr = get_namtab(SETL_SYSTEM elem_count_string);

      /* if we don't find it, make a literal item */

      if (namtab_ptr->nt_symtab_ptr == NULL) {

         operand[2] = enter_symbol(SETL_SYSTEM
                                   namtab_ptr,
                                   unit_proctab_ptr,
                                   &(root->ast_file_pos));

         operand[2]->st_has_rvalue = YES;
         operand[2]->st_is_initialized = YES;
         operand[2]->st_type = sym_integer;
         operand[2]->st_aux.st_integer_ptr = char_to_int(SETL_SYSTEM elem_count_string);

      }
      else {

         operand[2] = namtab_ptr->nt_symtab_ptr;

      }

      /* extract the tuple element */

      emit(q_tupof,
           operand[0],
           operand[1],
           operand[2],
           &(root->ast_file_pos));

      /* if we read into a temporary we have to pick it apart further */

      if (elem_ptr->ast_type != ast_symtab) {
         /* If we can, we try to optimize this case */
         new_gen_lhs(SETL_SYSTEM elem_ptr,operand[0]);
      }

      if (operand[0]->st_is_temp) {
         free_temp(operand[0]);
      }
   }

   /* assign omega to copied source (costs an opcode, avoids a real copy) */

   emit(q_assign,
        operand[1],
        sym_omega,
        NULL,
        &(root->ast_file_pos));

   free_temp(operand[1]);

   return;

}

/*\
 *  \ast{ast\_of}{map, tuple, and string assignment}
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
 *        {\nullast}
 *     }
 *
 *  This case generates sinister assignment code for a map, tuple, or
 *  string.  The major complication here is that we allow map assignments
 *  to have multiple keys.  When we find such a situation, we form the
 *  keys into a tuple and use the tuple as a key.
\*/

case ast_of :           case ast_ofa :

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
ast_ptr_type arg_ptr;                  /* pointer to argument               */
int32 arg_count;                       /* number of arguments               */
char arg_count_string[20];             /* character version for symtab      */
namtab_ptr_type namtab_ptr;            /* name pointer to argument count    */
symtab_ptr_type operand[3];            /* array of operands                 */
int opnd_num;                          /* temporary looping variable        */
int copied_source;                     /* YES if we copied the source       */

   /* if the source is our final left hand side, copy it */

   for (left_ptr = root;
        left_ptr->ast_type != ast_symtab;
        left_ptr = left_ptr->ast_child.ast_child_ast);

   if (left_ptr->ast_child.ast_symtab_ptr == source) {

      copied_source = YES;
      get_temp(operand[0]);
      emit(q_assign,
           operand[0],
           source,
           NULL,
           &(root->ast_file_pos));

      source = operand[0];
      copied_source = YES;

   }
   else {

      copied_source = NO;

   }

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

   /* if there is more than one argument we make them into a tuple */

   if (arg_count > 1) {

      /* push the arguments on the stack */

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

      /* we generate a temporary for the domain element */

      get_temp(operand[1]);

      /* we need to have a symbol table pointer as second argument */

      sprintf(arg_count_string,"%ld",arg_count);
      namtab_ptr = get_namtab(SETL_SYSTEM arg_count_string);

      /* if we don't find it, make a literal item */

      if (namtab_ptr->nt_symtab_ptr == NULL) {

         operand[2] = enter_symbol(SETL_SYSTEM
                                   namtab_ptr,
                                   unit_proctab_ptr,
                                   &(root->ast_file_pos));

         operand[2]->st_has_rvalue = YES;
         operand[2]->st_is_initialized = YES;
         operand[2]->st_type = sym_integer;
         operand[2]->st_aux.st_integer_ptr = char_to_int(SETL_SYSTEM arg_count_string);

      }
      else {

         operand[2] = namtab_ptr->nt_symtab_ptr;

      }

      /* form the tuple */

      emit(q_tuple,
           operand[1],
           operand[2],
           NULL,
           &(root->ast_file_pos));

   }

   /* we have only one argument, so use it as key */

   else {

      operand[1] = gen_expression(SETL_SYSTEM right_ptr->ast_child.ast_child_ast,NULL);

   }

   /* evaluate the left hand side */

   operand[0] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   /* emit the sinister assignment */

   if (root->ast_type == ast_of) {

      emit(q_sof,
           operand[0],
           operand[1],
           source,
           &(root->ast_file_pos));

   }
   else {

      emit(q_sofa,
           operand[0],
           operand[1],
           source,
           &(root->ast_file_pos));

   }

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }

   /*
    *  If the target is a temporary, generate another sinister
    *  assignment.
    */

   if (operand[0]->st_is_temp) {

      gen_lhs(SETL_SYSTEM left_ptr,operand[0]);
      free_temp(operand[0]);

   }

   /* if we copied the source, free it */

   if (copied_source) {

      /* assign omega to copied source */

      emit(q_assign,
           source,
           sym_omega,
           NULL,
           &(root->ast_file_pos));

      free_temp(source);

   }

   return;

}

/*\
 *  \ast{ast\_slice}{string or tuple slice assignment}
 *     {\astnode{ast\_slice}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{{\em beginning}}
 *              {\etcast}
 *              {\astnode{{\em end}}
 *                 {\etcast}
 *                 {\nullast}
 *              }
 *           }
 *        }
 *        {\nullast}
 *     }
 *
 *  This case generates sinister assignment code for a slice of a string
 *  or tuple.
\*/

case ast_slice :

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type begin_ptr;                /* beginning of slice                */
ast_ptr_type end_ptr;                  /* end of slice                      */
symtab_ptr_type operand[3];            /* array of operands                 */
int copied_source;                     /* YES if we copied the source       */

   /* if the source is our final left hand side, copy it */

   for (left_ptr = root;
        left_ptr->ast_type != ast_symtab;
        left_ptr = left_ptr->ast_child.ast_child_ast);

   if (left_ptr->ast_child.ast_symtab_ptr == source) {

      copied_source = YES;
      get_temp(operand[0]);
      emit(q_assign,
           operand[0],
           source,
           NULL,
           &(root->ast_file_pos));

      source = operand[0];
      copied_source = YES;

   }
   else {

      copied_source = NO;

   }

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   begin_ptr = left_ptr->ast_next;
   end_ptr = begin_ptr->ast_next;

   /* evaluate the left hand side */

   operand[0] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   /* evaluate operands */

   operand[1] = gen_expression(SETL_SYSTEM begin_ptr,NULL);
   operand[2] = gen_expression(SETL_SYSTEM end_ptr,NULL);

   /* emit the sinister assignment */

   emit(q_sslice,
        operand[0],
        operand[1],
        operand[2],
        &(root->ast_file_pos));

   /* we use a no-operation for the extra operand */

   emit(q_noop,
        source,
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

   /*
    *  If the target is a temporary, generate another sinister
    *  assignment.
    */

   if (operand[0]->st_is_temp) {

      gen_lhs(SETL_SYSTEM left_ptr,operand[0]);
      free_temp(operand[0]);

   }

   /* if we copied the source, free it */

   if (copied_source) {

      /* assign omega to copied source */

      emit(q_assign,
           source,
           sym_omega,
           NULL,
           &(root->ast_file_pos));

      free_temp(source);

   }

   return;

}

/*\
 *  \ast{ast\_end}{end of string or tuple assignment}
 *     {\astnode{ast\_end}
 *        {\astnode{{\em expression}}
 *           {\etcast}
 *           {\astnode{{\em beginning}}
 *              {\etcast}
 *              {\nullast}
 *           }
 *        }
 *        {\nullast}
 *     }
 *
 *  This case handles sinister assignment for the tail of a string or
 *  tuple.
\*/

case ast_end :

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */
int copied_source;                     /* YES if we copied the source       */

   /* if the source is our final left hand side, copy it */

   for (left_ptr = root;
        left_ptr->ast_type != ast_symtab;
        left_ptr = left_ptr->ast_child.ast_child_ast);

   if (left_ptr->ast_child.ast_symtab_ptr == source) {

      copied_source = YES;
      get_temp(operand[0]);
      emit(q_assign,
           operand[0],
           source,
           NULL,
           &(root->ast_file_pos));

      source = operand[0];
      copied_source = YES;

   }
   else {

      copied_source = NO;

   }

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* evaluate the left hand side */

   operand[0] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   /* evaluate operands */

   operand[1] = gen_expression(SETL_SYSTEM right_ptr,NULL);

   /* emit the sinister assignment */

   emit(q_send,
        operand[0],
        operand[1],
        source,
        &(root->ast_file_pos));

   /* free temporaries */

   if (operand[1]->st_is_temp) {
      free_temp(operand[1]);
   }

   /*
    *  If the target is a temporary, generate another sinister
    *  assignment.
    */

   if (operand[0]->st_is_temp) {

      gen_lhs(SETL_SYSTEM left_ptr,operand[0]);
      free_temp(operand[0]);

   }

   /* if we copied the source, free it */

   if (copied_source) {

      /* assign omega to copied source */

      emit(q_assign,
           source,
           sym_omega,
           NULL,
           &(root->ast_file_pos));

      free_temp(source);

   }

   return;

}

/*\
 *  \ast{ast\_slot}{slot assignment}
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
 *  This node corresponds to a slot reference on the left.  At run time
 *  we will have to verify that the slot name does not refer to a method.
\*/

case ast_slot :

{
ast_ptr_type left_ptr;                 /* left child node                   */
ast_ptr_type right_ptr;                /* right child node                  */
symtab_ptr_type operand[3];            /* array of operands                 */
int copied_source;                     /* YES if we copied the source       */

   /* if the source is our final left hand side, copy it */

   for (left_ptr = root;
        left_ptr->ast_type != ast_symtab;
        left_ptr = left_ptr->ast_child.ast_child_ast);

   if (left_ptr->ast_child.ast_symtab_ptr == source) {

      copied_source = YES;
      get_temp(operand[0]);
      emit(q_assign,
           operand[0],
           source,
           NULL,
           &(root->ast_file_pos));

      source = operand[0];
      copied_source = YES;

   }
   else {

      copied_source = NO;

   }

   /* pick out child pointers, for readability */

   left_ptr = root->ast_child.ast_child_ast;
   right_ptr = left_ptr->ast_next;

   /* evaluate the left hand side */

   operand[0] = gen_expression(SETL_SYSTEM left_ptr,NULL);

   /* emit the sinister assignment */

   emit(q_sslot,
        operand[0],
        right_ptr->ast_child.ast_symtab_ptr,
        source,
        &(root->ast_file_pos));

   /*
    *  If the target is a temporary, generate another sinister
    *  assignment.
    */

   if (operand[0]->st_is_temp) {

      gen_lhs(SETL_SYSTEM left_ptr,operand[0]);
      free_temp(operand[0]);

   }

   /* if we copied the source, free it */

   if (copied_source) {

      /* assign omega to copied source */

      emit(q_assign,
           source,
           sym_omega,
           NULL,
           &(root->ast_file_pos));

      free_temp(source);

   }

   return;

}

/*\
 *  \astnotree{ast\_placeholder}{dummy targets}
 *
 *  We allow place holders (\verb"-") in tuples used on the left.  We
 *  generate no code for those.
\*/

case ast_placeholder :

{

   return;

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

   trap(__FILE__,__LINE__,msg_bad_ast_node,root->ast_type);

#endif

   break;
}

/* at this point we return to normal indentation */

}}
