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
 *  \package{The Code Optimizer}
 *
 *  At present we don't really have an optimization phase --- this is
 *  really just a stub.  However, there are a few things which should be
 *  performed by the last phase of the optimizer, so we do those things
 *  here.
 *
 *  \begin{itemize}
 *  \item
 *  We update a symbol table flag \verb"is_stored", which is true for any
 *  items which must be allocated storage.  We defer this until after
 *  optimization since the optimizer may be able to eliminate some items.
 *  \item
 *  We find addresses (segment offsets) for all labels.  Clearly this
 *  must be done after optimization.
 *  \end{itemize}
 *
 *  \texify{optimize.h}
 *
 *  \packagebody{Code Optimizer}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "quads.h"                     /* quadruples                        */

#ifdef PLUGIN
#define fprintf plugin_fprintf
#endif

/* forward declarations */

static void opt_procedure(SETL_SYSTEM_PROTO struct proctab_item *);
                                       /* get next program argument         */

/*\
 *  \function{optimize()}
 *
 *  This procedure loops over the procedures in a compilation unit and
 *  calls \verb"opt_procedure" to process each procedure.
\*/

void optimize(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* procedure / unit to optimize      */

{

   if (proctab_ptr == NULL)
      return;

#ifdef DEBUG

   if (SYM_DEBUG || QUADS_DEBUG) {

      fprintf(DEBUG_FILE,
              "\nCODE OPTIMIZATION PHASE\n=======================\n");

   }

#endif

   /* loop over procedures */

   while (proctab_ptr != NULL) {

      /* optimize one procedure */

      opt_procedure(SETL_SYSTEM proctab_ptr);

      /* optimize children */

      if (proctab_ptr->pr_type != pr_package_spec &&
          proctab_ptr->pr_type != pr_native_package &&
          proctab_ptr->pr_type != pr_class_spec &&
          proctab_ptr->pr_type != pr_process_spec)
         optimize(SETL_SYSTEM proctab_ptr->pr_child);

      /* set up for next procedure */

      if (proctab_ptr->pr_type == pr_procedure ||
          proctab_ptr->pr_type == pr_method)
         proctab_ptr = proctab_ptr->pr_next;
      else
         proctab_ptr = NULL;

   }
}

/*\
 *  \function{opt\_procedure()}
 *
 *  This function is a stub which performs some post-optimization fixup
 *  on a single procedure.
\*/

static void opt_procedure(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* procedure to be optimized         */

{
quad_ptr_type init_head;               /* head of initialization quads      */
quad_ptr_type slot_head;               /* head of slot initialization quads */
quad_ptr_type body_head;               /* head of body quads                */
int32 *offset;                         /* array of label offsets            */
int32 quad_num;                        /* quadruple number                  */
int operand;                           /* used to loop over operands        */
symtab_ptr_type symtab_ptr;            /* used to loop over symbols         */
quad_ptr_type quad_ptr;                /* used to loop over quadruples      */
quad_ptr_type quad_tmp;                /* quad lookahead pointer            */
quad_ptr_type *quad_tail;              /* place to attach quadruple         */
quad_ptr_type useless_quad;            /* deleted quadruple                 */
int last_was_goto;                     /* YES if last quad was branch       */
int32 i;                               /* temporary looping variable        */

   /* allocate an array of label locations */

   offset = (int32 *)malloc((size_t)
                    ((proctab_ptr->pr_label_count + 1) * sizeof(int32)));
   if (offset == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* load the quadruples code for this procedure */

   init_head = load_quads(SETL_SYSTEM &(proctab_ptr->pr_init_code));

   if (proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      slot_head = load_quads(SETL_SYSTEM &(proctab_ptr->pr_slot_code));

   }

   if (proctab_ptr->pr_type == pr_program ||
       proctab_ptr->pr_type == pr_procedure ||
       proctab_ptr->pr_type == pr_method) {

      body_head = load_quads(SETL_SYSTEM &(proctab_ptr->pr_body_code));

   }

#ifdef DEBUG

   /* print the quadruples and symbol table if desired */

   if (SYM_DEBUG || QUADS_DEBUG) {

      fprintf(DEBUG_FILE,"\n%s : %s\n",
              (proctab_ptr->pr_namtab_ptr)->nt_name,
              proctab_desc[proctab_ptr->pr_type]);

      if (SYM_DEBUG)
         print_symtab(SETL_SYSTEM proctab_ptr);

      if (QUADS_DEBUG) {
         print_quads(SETL_SYSTEM init_head,"Initialization Code");

         if (proctab_ptr->pr_type == pr_class_spec ||
             proctab_ptr->pr_type == pr_process_spec) {

            print_quads(SETL_SYSTEM slot_head,"Slot Initialization Code");

         }

         if (proctab_ptr->pr_type == pr_program ||
             proctab_ptr->pr_type == pr_procedure ||
             proctab_ptr->pr_type == pr_method) {

            print_quads(SETL_SYSTEM body_head,"Body Code");

         }
      }
   }

#endif

   /* flag symbols being used */

   for (quad_ptr = init_head;
        quad_ptr != NULL;
        quad_ptr = quad_ptr->q_next) {

      for (operand = 0; operand < 3; operand++) {

         if (quad_optype[quad_ptr->q_opcode][operand] == QUAD_SPEC_OP) {

            symtab_ptr = quad_ptr->q_operand[operand].q_symtab_ptr;
            if (symtab_ptr != NULL)
               symtab_ptr->st_needs_stored = YES;

         }
      }
   }

   if (proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      for (quad_ptr = slot_head;
           quad_ptr != NULL;
           quad_ptr = quad_ptr->q_next) {

         for (operand = 0; operand < 3; operand++) {

            if (quad_optype[quad_ptr->q_opcode][operand] == QUAD_SPEC_OP) {

               symtab_ptr = quad_ptr->q_operand[operand].q_symtab_ptr;
               if (symtab_ptr != NULL)
                  symtab_ptr->st_needs_stored = YES;

            }
         }
      }
   }

   if (proctab_ptr->pr_type == pr_program ||
       proctab_ptr->pr_type == pr_procedure ||
       proctab_ptr->pr_type == pr_method) {

      for (quad_ptr = body_head;
           quad_ptr != NULL;
           quad_ptr = quad_ptr->q_next) {

         for (operand = 0; operand < 3; operand++) {

            if (quad_optype[quad_ptr->q_opcode][operand] == QUAD_SPEC_OP) {

               symtab_ptr = quad_ptr->q_operand[operand].q_symtab_ptr;
               if (symtab_ptr != NULL)
                  symtab_ptr->st_needs_stored = YES;

            }
         }
      }
   }

   /*
    *  We collapse chains of goto's
    */

   /* initialize the offset array, each label maps to itself */

   for (i = 0;
        i < proctab_ptr->pr_label_count + 1;
        i++)
      offset[i] = i;

   /*
    *  Modify the offset array, so that any label followed by a goto maps
    *  to the target of that goto.
    */

   for (quad_ptr = init_head;
        quad_ptr != NULL;
        quad_ptr = quad_ptr->q_next) {

      /* if we found a label check if the following statement is a branch */

      if (quad_ptr->q_opcode == q_label) {

         for (quad_tmp = quad_ptr;
              quad_tmp != NULL && quad_tmp->q_opcode == q_label;
              quad_tmp = quad_tmp->q_next);

         if (quad_tmp != NULL && quad_tmp->q_opcode == q_go) {

            offset[quad_ptr->q_operand[0].q_integer] =
               quad_tmp->q_operand[0].q_integer;

         }
      }
   }

   if (proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      for (quad_ptr = slot_head;
           quad_ptr != NULL;
           quad_ptr = quad_ptr->q_next) {

         /* if we found a label check if followed by a branch */

         if (quad_ptr->q_opcode == q_label) {

            for (quad_tmp = quad_ptr;
                 quad_tmp != NULL && quad_tmp->q_opcode == q_label;
                 quad_tmp = quad_tmp->q_next);

            if (quad_tmp != NULL && quad_tmp->q_opcode == q_go) {

               offset[quad_ptr->q_operand[0].q_integer] =
                  quad_tmp->q_operand[0].q_integer;

            }
         }
      }
   }

   if (proctab_ptr->pr_type == pr_program ||
       proctab_ptr->pr_type == pr_procedure ||
       proctab_ptr->pr_type == pr_method) {

      for (quad_ptr = body_head;
           quad_ptr != NULL;
           quad_ptr = quad_ptr->q_next) {

         /* if we found a label check if followed by a branch */

         if (quad_ptr->q_opcode == q_label) {

            for (quad_tmp = quad_ptr;
                 quad_tmp != NULL && quad_tmp->q_opcode == q_label;
                 quad_tmp = quad_tmp->q_next);

            if (quad_tmp != NULL && quad_tmp->q_opcode == q_go) {

               offset[quad_ptr->q_operand[0].q_integer] =
                  quad_tmp->q_operand[0].q_integer;

            }
         }
      }
   }

   /* collapse chains of goto's */

   for (i = 0;
        i < proctab_ptr->pr_label_count + 1;
        i++) {

      /*
       *  This looks like a stupid way to code a while loop, but it ducks
       *  a bug in Microsoft C 5.1!
       */

      for (;;) {

         if (offset[i] == offset[offset[i]])
            break;

         offset[i] = offset[offset[i]];

      }
   }

   /* delete useless goto's */

   quad_ptr = init_head;
   quad_tail = &init_head;
   last_was_goto = NO;

   while (quad_ptr != NULL) {

      if (quad_ptr->q_opcode == q_go) {

         if (last_was_goto) {

            useless_quad = quad_ptr;
            quad_ptr = quad_ptr->q_next;
            free_quad(SETL_SYSTEM useless_quad);

            continue;

         }
      }

      /* anything else is saved */

      if (quad_ptr->q_opcode == q_go) {
         last_was_goto = YES;
      }
      else if (quad_ptr->q_opcode != q_label) {
         last_was_goto = NO;
      }
      *quad_tail = quad_ptr;
      quad_tail = &(quad_ptr->q_next);
      quad_ptr = quad_ptr->q_next;

   }

   *quad_tail = NULL;

   if (proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      quad_ptr = slot_head;
      quad_tail = &slot_head;
      last_was_goto = NO;

      while (quad_ptr != NULL) {

         if (quad_ptr->q_opcode == q_go) {

            if (last_was_goto) {

               useless_quad = quad_ptr;
               quad_ptr = quad_ptr->q_next;
               free_quad(SETL_SYSTEM useless_quad);

               continue;

            }
         }

         /* anything else is saved */

         if (quad_ptr->q_opcode == q_go) {
            last_was_goto = YES;
         }
         else if (quad_ptr->q_opcode != q_label) {
            last_was_goto = NO;
         }
         *quad_tail = quad_ptr;
         quad_tail = &(quad_ptr->q_next);
         quad_ptr = quad_ptr->q_next;

      }

      *quad_tail = NULL;

   }

   if (proctab_ptr->pr_type == pr_program ||
       proctab_ptr->pr_type == pr_procedure ||
       proctab_ptr->pr_type == pr_method) {

      quad_ptr = body_head;
      quad_tail = &body_head;
      last_was_goto = NO;

      while (quad_ptr != NULL) {

         if (quad_ptr->q_opcode == q_go) {

            if (last_was_goto) {

               useless_quad = quad_ptr;
               quad_ptr = quad_ptr->q_next;
               free_quad(SETL_SYSTEM useless_quad);

               continue;

            }
         }

         /* anything else is saved */

         if (quad_ptr->q_opcode == q_go) {
            last_was_goto = YES;
         }
         else if (quad_ptr->q_opcode != q_label) {
            last_was_goto = NO;
         }
         *quad_tail = quad_ptr;
         quad_tail = &(quad_ptr->q_next);
         quad_ptr = quad_ptr->q_next;

      }

      *quad_tail = NULL;

   }

   /* delete branches to the next instruction */

   quad_ptr = init_head;
   quad_tail = &init_head;

   while (quad_ptr != NULL) {

      if (quad_optype[quad_ptr->q_opcode][0] == QUAD_LABEL_OP) {

         for (quad_tmp = quad_ptr->q_next;
              quad_tmp != NULL &&
                 quad_tmp->q_opcode == q_label &&
                 quad_tmp->q_operand[0].q_integer !=
                    offset[quad_ptr->q_operand[0].q_integer];
              quad_tmp = quad_tmp->q_next);

         if (quad_tmp != NULL &&
             quad_tmp->q_opcode == q_label) {

            useless_quad = quad_ptr;
            quad_ptr = quad_ptr->q_next;
            free_quad(SETL_SYSTEM useless_quad);

            continue;
         }
      }

      *quad_tail = quad_ptr;
      quad_tail = &(quad_ptr->q_next);
      quad_ptr = quad_ptr->q_next;

   }

   *quad_tail = NULL;

   if (proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      quad_ptr = slot_head;
      quad_tail = &slot_head;

      while (quad_ptr != NULL) {

         if (quad_optype[quad_ptr->q_opcode][0] == QUAD_LABEL_OP) {

            for (quad_tmp = quad_ptr->q_next;
                 quad_tmp != NULL &&
                    quad_tmp->q_opcode == q_label &&
                    quad_tmp->q_operand[0].q_integer !=
                       offset[quad_ptr->q_operand[0].q_integer];
                 quad_tmp = quad_tmp->q_next);

            if (quad_tmp != NULL &&
                quad_tmp->q_opcode == q_label) {

               useless_quad = quad_ptr;
               quad_ptr = quad_ptr->q_next;
               free_quad(SETL_SYSTEM useless_quad);

               continue;
            }
         }

         *quad_tail = quad_ptr;
         quad_tail = &(quad_ptr->q_next);
         quad_ptr = quad_ptr->q_next;

      }

      *quad_tail = NULL;

   }

   if (proctab_ptr->pr_type == pr_program ||
       proctab_ptr->pr_type == pr_procedure ||
       proctab_ptr->pr_type == pr_method) {

      quad_ptr = body_head;
      quad_tail = &body_head;

      while (quad_ptr != NULL) {

         if (quad_optype[quad_ptr->q_opcode][0] == QUAD_LABEL_OP) {

            for (quad_tmp = quad_ptr->q_next;
                 quad_tmp != NULL &&
                    quad_tmp->q_opcode == q_label &&
                    quad_tmp->q_operand[0].q_integer !=
                       offset[quad_ptr->q_operand[0].q_integer];
                 quad_tmp = quad_tmp->q_next);

            if (quad_tmp != NULL &&
                quad_tmp->q_opcode == q_label) {

               useless_quad = quad_ptr;
               quad_ptr = quad_ptr->q_next;
               free_quad(SETL_SYSTEM useless_quad);

               continue;
            }
         }

         *quad_tail = quad_ptr;
         quad_tail = &(quad_ptr->q_next);
         quad_ptr = quad_ptr->q_next;

      }

      *quad_tail = NULL;

   }

   /* build up the offset array and remove labels */

   quad_num = 0;
   quad_ptr = init_head;
   quad_tail = &init_head;
   last_was_goto = NO;

   while (quad_ptr != NULL) {

      /* if we found a label, delete it */

      if (quad_ptr->q_opcode == q_label) {

         if (offset[quad_ptr->q_operand[0].q_integer] ==
             quad_ptr->q_operand[0].q_integer) {
            offset[quad_ptr->q_operand[0].q_integer] = -(quad_num + 1);
         }

         useless_quad = quad_ptr;
         quad_ptr = quad_ptr->q_next;
         free_quad(SETL_SYSTEM useless_quad);

         continue;

      }

      /* anything else is saved */

      else {

         *quad_tail = quad_ptr;
         quad_tail = &(quad_ptr->q_next);
         quad_ptr = quad_ptr->q_next;
         quad_num++;
         last_was_goto = NO;

      }
   }

   proctab_ptr->pr_init_count = quad_num;
   *quad_tail = NULL;

   if (proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      quad_num = 0;
      quad_ptr = slot_head;
      quad_tail = &slot_head;
      last_was_goto = NO;

      while (quad_ptr != NULL) {

         /* if we found a label, delete it */

         if (quad_ptr->q_opcode == q_label) {

            if (offset[quad_ptr->q_operand[0].q_integer] ==
                quad_ptr->q_operand[0].q_integer) {
               offset[quad_ptr->q_operand[0].q_integer] =
                  proctab_ptr->pr_label_count + quad_num + 2;
            }

            useless_quad = quad_ptr;
            quad_ptr = quad_ptr->q_next;
            free_quad(SETL_SYSTEM useless_quad);

            continue;

         }

         /* anything else is saved */

         else {

            *quad_tail = quad_ptr;
            quad_tail = &(quad_ptr->q_next);
            quad_ptr = quad_ptr->q_next;
            quad_num++;
            last_was_goto = NO;

         }
      }

      proctab_ptr->pr_sinit_count = quad_num;
      *quad_tail = NULL;

   }

   if (proctab_ptr->pr_type == pr_program ||
       proctab_ptr->pr_type == pr_procedure ||
       proctab_ptr->pr_type == pr_method) {

      quad_num = 0;
      quad_ptr = body_head;
      quad_tail = &body_head;
      last_was_goto = NO;

      while (quad_ptr != NULL) {

         /* if we found a label, delete it */

         if (quad_ptr->q_opcode == q_label) {

            if (offset[quad_ptr->q_operand[0].q_integer] ==
                quad_ptr->q_operand[0].q_integer) {
               offset[quad_ptr->q_operand[0].q_integer] =
                  proctab_ptr->pr_label_count + quad_num + 2;
            }

            useless_quad = quad_ptr;
            quad_ptr = quad_ptr->q_next;
            free_quad(SETL_SYSTEM useless_quad);

            continue;

         }

         /* anything else is saved */

         else {

            *quad_tail = quad_ptr;
            quad_tail = &(quad_ptr->q_next);
            quad_ptr = quad_ptr->q_next;
            quad_num++;
            last_was_goto = NO;

         }
      }

      proctab_ptr->pr_body_count = quad_num;
      *quad_tail = NULL;

   }

   /*
    *  Now the offset array contains either a code for the location of a
    *  label, or another label which replaces a given label.  We convert
    *  all of these to locations.
    */

   for (i = 0;
        i < proctab_ptr->pr_label_count + 1;
        i++) {

      if (offset[i] >= 0 && offset[i] < proctab_ptr->pr_label_count + 2)
         offset[i] = offset[offset[i]];

   }

   for (i = 0;
        i < proctab_ptr->pr_label_count;
        i++) {

      if (offset[i] > proctab_ptr->pr_label_count)
         offset[i] -= (proctab_ptr->pr_label_count + 2);

   }

   /*
    *  At this point, we know the locations of all the labels.  We loop
    *  over the quadruple lists again, and change any references to labels
    *  into procedure offsets.
    */

   for (quad_ptr = init_head;
        quad_ptr != NULL;
        quad_ptr = quad_ptr->q_next) {

      for (operand = 0; operand < 3; operand++) {

         if (quad_optype[quad_ptr->q_opcode][operand] == QUAD_LABEL_OP) {

            quad_ptr->q_operand[operand].q_integer =
               offset[quad_ptr->q_operand[operand].q_integer];

         }
      }
   }

   if (proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      for (quad_ptr = slot_head;
           quad_ptr != NULL;
           quad_ptr = quad_ptr->q_next) {

         for (operand = 0; operand < 3; operand++) {

            if (quad_optype[quad_ptr->q_opcode][operand] == QUAD_LABEL_OP) {

               quad_ptr->q_operand[operand].q_integer =
                  offset[quad_ptr->q_operand[operand].q_integer];

            }
         }
      }
   }

   if (proctab_ptr->pr_type == pr_program ||
       proctab_ptr->pr_type == pr_procedure ||
       proctab_ptr->pr_type == pr_method) {

      for (quad_ptr = body_head;
           quad_ptr != NULL;
           quad_ptr = quad_ptr->q_next) {

         for (operand = 0; operand < 3; operand++) {

            if (quad_optype[quad_ptr->q_opcode][operand] == QUAD_LABEL_OP) {

               quad_ptr->q_operand[operand].q_integer =
                  offset[quad_ptr->q_operand[operand].q_integer];

            }
         }
      }
   }

   /* save the updated quadruples */

   store_quads(SETL_SYSTEM &(proctab_ptr->pr_init_code),init_head);

   if (proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      store_quads(SETL_SYSTEM &(proctab_ptr->pr_slot_code),slot_head);

   }

   if (proctab_ptr->pr_type == pr_program ||
       proctab_ptr->pr_type == pr_procedure ||
       proctab_ptr->pr_type == pr_method) {

      store_quads(SETL_SYSTEM &(proctab_ptr->pr_body_code),body_head);

   }

   /*
    *  Now we set the location of any label variables (used by case and
    *  select statements).
    */

   for (symtab_ptr = proctab_ptr->pr_symtab_head;
        symtab_ptr != NULL;
        symtab_ptr = symtab_ptr->st_thread) {

      if (symtab_ptr->st_type == sym_label)
         symtab_ptr->st_aux.st_label_offset =
            offset[symtab_ptr->st_aux.st_label_num];

   }

   /* we're finished with the address array */

   free(offset);

}
