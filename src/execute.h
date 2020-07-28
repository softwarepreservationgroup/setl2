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
 *  \packagespec{Interpreter Core}
\*/

#ifndef EXECUTE_LOADED

/* execute code types */

#define EX_INIT_CODE 0                 /* initialization code               */
#define EX_BODY_CODE 1                 /* body code                         */

/* procedure call stack */

struct call_stack_item {
   struct unittab_item *cs_unittab_ptr;
                                       /* currently executing unit          */
   struct proc_item *cs_proc_ptr;      /* current procedure                 */
   struct instruction_item *cs_pc;     /* program counter                   */
   struct specifier_item *cs_return_value;
                                       /* return value location             */
   struct specifier_item *cs_self_ptr;
                                       /* self for call                     */
   struct unittab_item *cs_class_ptr;  /* saved class pointer               */
   struct process_item *cs_process_ptr;
   int32 cs_pstack_top;                /* saved pstack top                  */
   unsigned cs_C_return : 2;           /* YES if we execute a C return      */
   unsigned cs_literal_proc : 1;       /* YES if a literal procedure call   */
   unsigned cs_code_type : 2;          /* initialization or body code       */
   unsigned cs_extra_code : 3;         /* To be executed after p_return     */
};

/* public data */

#ifdef SHARED

int32 critical_section = 0;
int32 opcodes_until_switch = 2000;
struct process_item *process_head = NULL;
struct specifier_item *ex_wait_target;

struct instruction_item *ip;           /* executing instruction             */
struct instruction_item *pc;           /* next instruction                  */
int32 pstack_top = -1;                 /* top of program stack              */
int32 pstack_max = 0;                  /* size of program stack             */
struct specifier_item *pstack = NULL;  /* program stack                     */
int32 cstack_top = -1;                 /* top of call stack                 */
int32 cstack_max = 0;                  /* size of call stack                */
struct call_stack_item *cstack = NULL; /* call stack                        */

#ifdef DEBUG

char x_source_name[PATH_LENGTH + 1];   /* source source file name           */
int32 source_line = 0;                 /* current line number               */
int32 source_column = 0;               /* current column                    */
struct unittab_item *source_unittab;   /* current unittab                   */
struct profiler_item *profi=NULL;      /* temporary looping variable        */
struct unittab_item *head_unittab;     /* points to the 1st unittab loaded  */
struct unittab_item *last_unittab;     /* points to the last unittab loaded */

#endif

#else

extern int32 critical_section;
extern int32 opcodes_until_switch;
extern struct process_item *process_head;
extern struct specifier_item *ex_wait_target;

extern struct instruction_item *ip;    /* executing instruction             */
extern struct instruction_item *pc;    /* next instruction                  */
EXTERNAL int32 pstack_top;               /* top of program stack              */
EXTERNAL int32 pstack_max;               /* size of program stack             */
EXTERNAL struct specifier_item *pstack;  /* program stack                     */
extern int32 cstack_top;               /* top of call stack                 */
extern int32 cstack_max;               /* size of call stack                */
extern struct call_stack_item *cstack; /* call stack                        */

#ifdef DEBUG

extern char x_source_name[PATH_LENGTH + 1];   
				       /* source source file name           */
extern int32 source_line;              /* current line number               */
extern int32 source_column;            /* current column                    */
extern struct unittab_item *source_unittab;   
                                       /* current unittab                   */
extern struct profiler_item *profi;    /* temporary looping variable        */
extern struct unittab_item *head_unittab;   
                                       /* points to the 1st unittab loaded  */
extern struct unittab_item *last_unittab;   
                                       /* points to the last unittab loaded */


#endif
#endif

/*
 *  Stack access macros.
 */

/* program stack */

#define push_pstack(s) {\
   mark_specifier(s); \
   if (++pstack_top == pstack_max) \
      alloc_pstack(SETL_SYSTEM_VOID); \
   pstack[pstack_top].sp_form = (s)->sp_form; \
   pstack[pstack_top].sp_val.sp_biggest = (s)->sp_val.sp_biggest; \
}

#define pop_pstack {\
   unmark_specifier((pstack + pstack_top)); \
   pstack_top--; \
}

/* call stack */

#define push_cstack(p,pr,r,ss,cl,s,c,l,u,t,po,ct) { \
if (++cstack_top == cstack_max) \
alloc_cstack(SETL_SYSTEM_VOID); \
cstack[cstack_top].cs_pc = p; \
cstack[cstack_top].cs_proc_ptr = pr; \
cstack[cstack_top].cs_return_value = r; \
cstack[cstack_top].cs_self_ptr = ss; \
cstack[cstack_top].cs_class_ptr = cl; \
cstack[cstack_top].cs_pstack_top = s; \
cstack[cstack_top].cs_C_return = c; \
cstack[cstack_top].cs_literal_proc = l; \
cstack[cstack_top].cs_unittab_ptr = u; \
cstack[cstack_top].cs_code_type = t; \
cstack[cstack_top].cs_process_ptr = po; \
cstack[cstack_top].cs_extra_code = ct; \
}

#define pop_cstack { \
   current_class = cstack[cstack_top].cs_class_ptr; \
   cstack_top--; \
}

/* public function declarations */

void execute_setup(SETL_SYSTEM_PROTO struct unittab_item *, int);
                                       /* interpreter function              */
int execute_go(SETL_SYSTEM_PROTO int);
                                       /* start executing                   */
void alloc_pstack(SETL_SYSTEM_PROTO_VOID);
                                       /* enlarge the program stack         */
void call_procedure(SETL_SYSTEM_PROTO
                    struct specifier_item *, struct specifier_item *,
                    struct specifier_item *, int32, int, int, int);
                                       /* call a SETL2 procedure            */

#define EXECUTE_LOADED 1
#endif
