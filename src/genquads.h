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
 *  \packagespec{Quadruple Generator}
\*/

#ifndef GENQUADS_LOADED

/* loop stack structure */

struct loop_stack_item {
   struct symtab_item *ls_return;      /* return value, if any              */
   int ls_exit_label;                  /* current quit location             */
   int ls_continue_label;              /* current continue location         */
};

#ifdef SHARED

struct symtab_item *next_temp = NULL;  /* free temporary list               */
int next_label = 0;                    /* next label to be allocated        */
int lstack_top = -1;                   /* top of loop stack                 */
int lstack_max = 0;                    /* size of loop stack                */
struct loop_stack_item *lstack = NULL;
                                       /* loop stack                        */

#else

extern struct symtab_item *next_temp;  /* free temporary list               */
extern int next_label;                 /* next label to be allocated        */
extern int lstack_top;                 /* top of loop stack                 */
extern int lstack_max;                 /* size of loop stack                */
extern struct loop_stack_item *lstack;
                                       /* loop stack                        */

#endif

/* public function declarations */

void gen_quads(SETL_SYSTEM_PROTO struct proctab_item *); 
                                       /* main code generator function      */
int get_lstack(SETL_SYSTEM_PROTO_VOID);
                                       /* push the loop stack               */

/*\
 *  \function{get\_temp()}
 *
 *  These macros allocate and deallocate temporary variables.  They are
 *  implemented as macros because they are fairly small and used
 *  frequently.
\*/

#define get_temp(t) { \
   if (next_temp == NULL) { \
      t = enter_symbol(SETL_SYSTEM \
                       NULL,curr_proctab_ptr,NULL); \
      (t)->st_type = sym_id; \
      (t)->st_has_lvalue = 1; \
      (t)->st_has_rvalue = 1; \
      (t)->st_is_temp = 1; \
   } else { \
      t = next_temp; \
      next_temp = next_temp->st_name_link; \
   } \
}

#define free_temp(t) { \
   (t)->st_name_link = next_temp;      next_temp = t; \
}

#define GENQUADS_LOADED 1
#endif
