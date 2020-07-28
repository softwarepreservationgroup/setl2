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
 *
 *  \package{The Interpreter Core}
 *
 *  This package contains the core of the interpreter.  The structure is
 *  a single large case statement, unless the symbol \verb"SHORT_FUNCS"
 *  is defined, in which case the interpreter is broken up into many
 *  smaller functions.  The breakup is necessary on quite a few
 *  compilers, since otherwise this function is very large.  We keep it
 *  large anyway since the performance improvement is significant if we
 *  do manage to find a compiler which will handle it.
 *
 *  From outside this package, we call \verb"execute_setup()", and pass it
 *  the unit we want to interpret.  \verb"execute_setup()" will set up
 *  a program unit to be executed.  Then \verb"execute_go()" should be
 *  called to interpret the stream of instructions.
 *
 *  If a C function wants to call a \setl\ procedure, it uses the
 *  function \verb"call_procedure", with a flag set indicating that a C
 *  return is expected.  Then when that procedure returns it will return
 *  from the main procedure, rather than continuing the interpretater.
 *
 *  \texify{execute.h}
 *
 *  \packagebody{Interpreter Core}
\*/


/* standard C header files */

#include <math.h>                      /* math functions                    */

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "mcode.h"                     /* method codes                      */
#include "pcode.h"                     /* pseudo code                       */
#include "builtins.h"                  /* built-in symbols                  */
#include "libman.h"                    /* library manager                   */
#include "unittab.h"                   /* unit table                        */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "procs.h"                     /* procedures                        */
#include "objects.h"                   /* objects                           */
#include "process.h"                   /* objects                           */
#include "mailbox.h"                   /* objects                           */
#include "slots.h"                     /* procedures                        */
#include "iters.h"                     /* iterators                         */
#include "axobj.h"
#include <setjmp.h>
 

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define printf plugin_printf
#define fputs plugin_fputs
#define exit plugin_exit
#endif

void runtime_cleanup(SETL_SYSTEM_PROTO_VOID);
                                       /* Will be execute before a stopall  */

#ifdef HAVE_GETRUSAGE
#include "timeval.h"
#endif

/*
 *  Some compilers, notably Lattice, can not handle a very long text
 *  segment.  This stuff is used to break up this file, so we can run
 *  through it several times.
 */

#ifdef SHORT_TEXT

#define PRTYPE                         /* prototype class                   */

#ifdef ROOT
#define PCLASS                         /* data storage class                */
#else
#define PCLASS extern                  /* data storage class                */
#endif

#ifndef ROOT
#define ROOT 0
#endif

#ifndef PARTA
#define PARTA 0
#endif

#ifndef PARTB
#define PARTB 0
#endif

#ifndef PARTC
#define PARTC 0
#endif

#ifndef PARTD
#define PARTD 0
#endif

#ifndef PARTF
#define PARTF 0
#endif

#ifndef PARTG
#define PARTG 0
#endif

#ifndef PARTH
#define PARTH 0
#endif

#ifndef PARTI
#define PARTI 0
#endif

#ifndef PARTJ
#define PARTJ 0
#endif

#ifndef PARTK
#define PARTK 0
#endif

#ifndef PARTL
#define PARTL 0
#endif

#ifndef PARTM
#define PARTM 0
#endif

#else
#define PCLASS static                  /* data storage class                */
#define PRTYPE static                  /* prototype class                   */
#endif

/* constants */

#define EXTRA  2                       /* Used in call_procedure and return */

#define CONTINUE    -1                 /* continue execution                */

/* performance tuning constants */

#define PSTACK_BLOCK_SIZE    400       /* program stack block size          */
#define CSTACK_BLOCK_SIZE     40       /* call stack block size             */

/* global data */
jmp_buf abend_env;

PCLASS int32 save_pstack_top;          /* saved stack top                   */
PCLASS int32 return_pstack_top;        /* stack top on return               */

/*
 *  The following variables should most properly appear within individual
 *  case items below, and with more descriptive names.  While this is
 *  great for clarity, it is faster to make them static than to allocate
 *  and deallocate them from the stack when cases are invoked.  We
 *  therefore think of them as registers of various types.  This is a
 *  little ugly, but not too bad since each case is relatively short and
 *  the cases are independent.
 */

PCLASS specifier *target;              /* target operand pointer            */
PCLASS specifier *left;                /* left operand pointer              */
PCLASS specifier *right;               /* right operand pointer             */
PCLASS specifier *end;                 /* slice end operand pointer         */
PCLASS specifier *domain_element;      /* domain element in map             */
PCLASS specifier *range_element;       /* range element in map              */
PCLASS specifier *target_element;      /* target specifier                  */

PCLASS integer_h_ptr_type integer_hdr; /* integer header pointer            */
PCLASS int32 short_value;              /* short integer value               */
PCLASS int32 short_hi_bits;            /* high order bits of short          */
PCLASS i_real_ptr_type real_ptr;       /* real pointer                      */
PCLASS double real_number;             /* real value                        */
PCLASS string_h_ptr_type target_string_hdr, left_string_hdr, right_string_hdr;
                                       /* string header pointers            */
PCLASS string_c_ptr_type target_string_cell, left_string_cell,
                         right_string_cell;
                                       /* string cell pointers              */
PCLASS char *target_char_ptr, *left_char_ptr, *right_char_ptr;
                                       /* character pointers                */
PCLASS char *target_char_end, *left_char_end, *right_char_end;
                                       /* character pointers                */
PCLASS int32 target_string_length, left_string_length, right_string_length;
                                       /* string lengths                    */
PCLASS set_h_ptr_type set_root, set_work_hdr, new_set_hdr;
                                       /* set header pointers               */
PCLASS set_c_ptr_type set_cell, *set_cell_tail, new_set_cell;
                                       /* set cell pointer                  */
PCLASS map_h_ptr_type map_root, map_work_hdr, new_map_hdr;
                                       /* map header pointers               */
PCLASS map_c_ptr_type map_cell, *map_cell_tail, new_map_cell;
                                       /* map cell pointer                  */
PCLASS tuple_h_ptr_type tuple_root, tuple_work_hdr, new_tuple_hdr;
                                       /* tuple header pointers             */
PCLASS tuple_c_ptr_type tuple_cell;    /* tuple cell pointer                */
PCLASS int source_index, source_height;
                                       /* used to descend header trees      */
PCLASS int32 source_number;            /* tuple element number              */
PCLASS int target_index, target_height;
                                       /* used to descend header trees      */
PCLASS int32 work_length;              /* working length (destroyed)        */
PCLASS int32 source_hash_code, work_hash_code;
                                       /* work hash codes                   */
PCLASS int32 domain_hash_code, range_hash_code;
                                       /* work hash codes                   */
PCLASS int32 target_number;            /* tuple element number              */
PCLASS int32 expansion_trigger;        /* size which triggers header        */
                                       /* expansion                         */
PCLASS int32 contraction_trigger;      /* size which triggers header        */
                                       /* contraction                       */
PCLASS int32 slice_start, slice_end;   /* slice limits                      */
PCLASS proc_ptr_type proc_ptr;         /* procedure pointer                 */
PCLASS proc_ptr_type new_proc_ptr;     /* created procedure pointer         */
PCLASS iter_ptr_type iter_ptr;         /* iterator pointer                  */

PCLASS unittab_ptr_type class_ptr;     /* class pointer                     */
PCLASS object_h_ptr_type object_root, object_work_hdr, new_object_hdr;
                                       /* object header pointers            */
PCLASS object_c_ptr_type object_cell;  /* object cell pointer               */
PCLASS struct slot_info_item *slot_info;
                                       /* slot information record           */
PCLASS int32 slot_number;              /* index within instance             */
PCLASS self_stack_ptr_type self_stack_ptr;
PCLASS object_h_ptr_type self_root;    /* root of current or new self       */
PCLASS specifier *self_target;         /* self to be copied                 */
PCLASS unittab_ptr_type current_class; /* currently executing class         */
PCLASS process_ptr_type process_ptr;   /* work process pointer              */
PCLASS request_ptr_type request_ptr;   /* work request pointer              */
PCLASS mailbox_h_ptr_type mailbox_ptr; /* work mailbox pointer              */
PCLASS mailbox_c_ptr_type mailbox_cell;
                                       /* work mailbox pointer              */

/* spare specifiers (used for type conversions) */

PCLASS specifier spare,spare1;

/* call and return work variables */

PCLASS specifier *stack_ptr;           /* work stack pointer, usually for   */
                                       /* unmarking stack items             */
PCLASS specifier *stack_pos;           /* stack position at start of call   */
PCLASS specifier *arg_ptr;             /* work argument pointer             */

/* miscellaneous */

PCLASS int condition_true;             /* YES if tested condition is true   */
PCLASS int is_equal;                   /* YES if tested specifiers are      */
                                       /* equal                             */
PCLASS int32 i;                        /* temporary looping variable        */

/* Profiler */

#ifdef HAVE_GETRUSAGE
struct timeval prf_time;
#endif

/* forward declarations */

PRTYPE void call_binop_method(SETL_SYSTEM_PROTO
                              specifier *, specifier *, specifier *,
                              int, char *,int);
                                       /* call a binary operator method     */
/*
 *  For many compilers we have to break up the interpreter into a bunch
 *  of small functions.  These are the small functions.
 */

#ifdef SHORT_FUNCS

PRTYPE void arith_ops1(SETL_SYSTEM_PROTO_VOID);          
                                       /* arithmetic operators              */
PRTYPE void arith_ops2(SETL_SYSTEM_PROTO_VOID);          
                                       /* arithmetic operators              */
PRTYPE void arith_ops3(SETL_SYSTEM_PROTO_VOID);          
                                       /* arithmetic operators              */
PRTYPE void arith_ops4(SETL_SYSTEM_PROTO_VOID);          
                                       /* arithmetic operators              */
PRTYPE void set_ops1(SETL_SYSTEM_PROTO_VOID);            
                                       /* set and tuple operators           */
PRTYPE void set_ops2(SETL_SYSTEM_PROTO_VOID);            
                                       /* set and tuple operators           */
PRTYPE void set_ops3(SETL_SYSTEM_PROTO_VOID);            
                                       /* set and tuple operators           */
PRTYPE void unary_ops(SETL_SYSTEM_PROTO_VOID);           
                                       /* unary operators                   */
PRTYPE void extract_ops1(SETL_SYSTEM_PROTO_VOID);        
                                       /* extraction operations             */
PRTYPE void extract_ops2(SETL_SYSTEM_PROTO_VOID);        
                                       /* extraction operations             */
PRTYPE void extract_ops3(SETL_SYSTEM_PROTO_VOID);        
                                       
                                       /* extraction operations             */
PRTYPE void extract_ops4(SETL_SYSTEM_PROTO_VOID);        
                                       /* extraction operations             */
PRTYPE void assign_ops1(SETL_SYSTEM_PROTO_VOID);         
                                       /* assignment operations             */
PRTYPE void assign_ops2(SETL_SYSTEM_PROTO_VOID);         
                                       /* assignment operations             */
PRTYPE void assign_ops3(SETL_SYSTEM_PROTO_VOID);         
                                       /* assignment operations             */
PRTYPE void assign_ops4(SETL_SYSTEM_PROTO_VOID);         
                                       /* assignment operations             */
PRTYPE void cond_ops1(SETL_SYSTEM_PROTO_VOID);           
                                       /* condition operations              */
PRTYPE void cond_ops2(SETL_SYSTEM_PROTO_VOID);           
                                       /* condition operations              */
PRTYPE void cond_ops3(SETL_SYSTEM_PROTO_VOID);           
                                       /* condition operations              */
PRTYPE void cond_ops4(SETL_SYSTEM_PROTO_VOID);           
                                       /* condition operations              */
PRTYPE void cond_ops5(SETL_SYSTEM_PROTO_VOID);           
                                       /* condition operations              */
PRTYPE void iter_ops1(SETL_SYSTEM_PROTO_VOID);           
                                       /* set formers and iterators         */
PRTYPE void iter_ops2(SETL_SYSTEM_PROTO_VOID);           
                                       /* set formers and iterators         */
PRTYPE void object_ops(SETL_SYSTEM_PROTO_VOID);          
                                       /* object operations                 */
PRTYPE void switch_process(SETL_SYSTEM_PROTO_VOID);      
                                       /* change processes                  */

#endif

PRTYPE void alloc_cstack(SETL_SYSTEM_PROTO_VOID);
                                       /* enlarge the call stack            */

#ifdef DEBUG

#define PC         (pc+EX_DEBUG)
#define BUMPPC(n)  {pc += (n*(EX_DEBUG+1));}

#else

#define PC         (pc)
#define BUMPPC(n)  {pc += n;}

#endif

int hard_stop;
int abend_initialized;


#if !SHORT_TEXT | ROOT

/*\
 *  \function{execute\_setup()}
 *
 *  This is the central function of the interpreter.  It executes pseudo
 *  code instructions until it finds a \verb"p_stop" instruction.
\*/

void execute_setup(
   SETL_SYSTEM_PROTO
   unittab_ptr_type unittab_ptr,       /* unit to be executed               */
   int code_type)                      /* init or body code                 */

{
   /* Clear the profiler timers */

#ifdef DEBUG
   profi=NULL;
#ifdef HAVE_GETRUSAGE
   prf_time.tv_sec=0;
   prf_time.tv_usec=0;
#endif
#endif

   /* initialize spare specifiers */

   spare.sp_form = ft_omega;
   spare1.sp_form = ft_omega;

   SYMBOL_MAP.sp_form = ft_omega; /* Global error extension map */

   /* push something on the program stack, to avoid NULL problems */

   push_pstack(&spare);

   /* push the unit and stream on the execute stack */

   push_cstack(NULL,NULL,NULL,NULL,NULL,-1,0,0,unittab_ptr,code_type,NULL,0);

   if (code_type == EX_BODY_CODE)
      pc = unittab_ptr->ut_body_code;
   else
      pc = unittab_ptr->ut_init_code;

   if (process_head == NULL) {
      get_process(process_head);
      process_head->pc_prev = process_head->pc_next = process_head;
      process_head->pc_type = ROOT_PROCESS;
      process_head->pc_object_ptr = NULL;
      process_head->pc_suspended = NO;
      process_head->pc_idle = NO;
      process_head->pc_waiting = NO;
      process_head->pc_checking = NO;
   }
   hard_stop=0;
   abend_initialized=0;
}

void get_setl_string(
   SETL_SYSTEM_PROTO
   char *s,
   specifier *target) 
{
string_h_ptr_type string_hdr;          /* root of string value              */
string_c_ptr_type string_cell;         /* string cell                       */
char *string_char_ptr, *string_char_end;



   /* first we make a SETL2 string out of the C string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = NULL;
   string_char_ptr = string_char_end = NULL;

   /* copy the source string */

   while (*s) {

      if (string_char_ptr == string_char_end) {

         get_string_cell(string_cell);
         if (string_hdr->s_tail != NULL)
            (string_hdr->s_tail)->s_next = string_cell;
         string_cell->s_prev = string_hdr->s_tail;
         string_cell->s_next = NULL;
         string_hdr->s_tail = string_cell;
         if (string_hdr->s_head == NULL)
            string_hdr->s_head = string_cell;
         string_char_ptr = string_cell->s_cell_value;
         string_char_end = string_char_ptr + STR_CELL_WIDTH;

      }

      *string_char_ptr++ = *s++;
      string_hdr->s_length++;

   }

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;

}

/* Left is the string, right is the map, target is the destination */

void get_from_symmap(
   SETL_SYSTEM_PROTO
   specifier *right,
   specifier *left, 
   specifier *target)
{
         spec_hash_code(work_hash_code,right);
         source_hash_code = work_hash_code;

         /* look up the map component */

         map_work_hdr = left->sp_val.sp_map_ptr;

         for (source_height = map_work_hdr->m_ntype.m_root.m_height;
              source_height && map_work_hdr != NULL;
              source_height--) {

            /* extract the element's index at this level */

            source_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

            map_work_hdr = map_work_hdr->m_child[source_index].m_header;

         }

         /* if we can't get to a leaf, there is no matching element */

         if (map_work_hdr == NULL) {

            unmark_specifier(target);
            target->sp_form = ft_omega;

            return;

         }

         /*
          *  At this point, map_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */

         source_index = work_hash_code & MAP_HASH_MASK;
         for (map_cell = map_work_hdr->m_child[source_index].m_cell;
              map_cell != NULL &&
                 map_cell->m_hash_code < source_hash_code;
              map_cell = map_cell->m_next);

         /* try to find the domain element */

         is_equal = NO;
         while (map_cell != NULL &&
                map_cell->m_hash_code == source_hash_code) {

            spec_equal(is_equal,&(map_cell->m_domain_spec),right);

            if (is_equal)
               break;

            map_cell = map_cell->m_next;

         }

         /* if we did't find the domain element, return omega */

         if (!is_equal) {

            unmark_specifier(target);
            target->sp_form = ft_omega;

            return;

         }

         /* if the map has multiple values here, return omega */

         if (map_cell->m_is_multi_val) {

            unmark_specifier(target);
            target->sp_form = ft_omega;

            return;

         }

         /* otherwise, we found it */

         mark_specifier(&(map_cell->m_range_spec));
         unmark_specifier(target);
         target->sp_form = map_cell->m_range_spec.sp_form;
         target->sp_val.sp_biggest =
            map_cell->m_range_spec.sp_val.sp_biggest;

}

void triop_abend(
   SETL_SYSTEM_PROTO
   char *message,             
   char *s,
   char *l,
   char *r,
   char *e
)    

{
int mcode;
char function[64];

  mcode=0;
  while ((mcode_desc[mcode]!=NULL)&&(strcmp(s,mcode_desc[mcode])!=0)) mcode++;

  if (mcode_desc[mcode]==NULL) {
        abend(SETL_SYSTEM message,s,l);
  }


  sprintf(function,"$ERR_EXT%d",mcode);

  get_setl_string(SETL_SYSTEM function,&spare);
  get_from_symmap(SETL_SYSTEM &spare,
                  cstack[cstack_top].cs_unittab_ptr->ut_err_ext_map,
                  &spare1);
  if (spare1.sp_form==ft_omega)  {
        abend(SETL_SYSTEM message,s,l,r);
  }

  push_pstack(left);
  push_pstack(right);
  push_pstack(end);
  call_procedure(SETL_SYSTEM target,
                 &spare1,
                 NULL,
                 3,NO,YES,0);

}

void binop_abend(
   SETL_SYSTEM_PROTO
   char *message,             
   char *s,
   char *l,
   char *r
)    

{
int mcode;
char function[64];

  mcode=0;
  while ((mcode_desc[mcode]!=NULL)&&(strcmp(s,mcode_desc[mcode])!=0)) mcode++;

  if (mcode_desc[mcode]==NULL) {
        abend(SETL_SYSTEM message,s,l,r);
  }

  sprintf(function,"$ERR_EXT%d",mcode);

  get_setl_string(SETL_SYSTEM function,&spare);
  get_from_symmap(SETL_SYSTEM &spare,
                  cstack[cstack_top].cs_unittab_ptr->ut_err_ext_map,
                  &spare1);
  if (spare1.sp_form==ft_omega)  {
        abend(SETL_SYSTEM message,s,l,r);
  }

  push_pstack(left);
  push_pstack(right);
  call_procedure(SETL_SYSTEM target,
                 &spare1,
                 NULL,
                  2,NO,YES,0);

}

void unop_abend(
   SETL_SYSTEM_PROTO
   char *message,             
   char *s,
   char *l
)    

{
int mcode;
char function[64];

  mcode=0;
  while ((mcode_desc[mcode]!=NULL)&&(strcmp(s,mcode_desc[mcode])!=0)) mcode++;

  if (mcode_desc[mcode]==NULL) {
        abend(SETL_SYSTEM message,s,l);
  }


  sprintf(function,"$ERR_EXT%d",mcode);

  get_setl_string(SETL_SYSTEM function,&spare);
  get_from_symmap(SETL_SYSTEM &spare,
                  cstack[cstack_top].cs_unittab_ptr->ut_err_ext_map,
                  &spare1);
  if (spare1.sp_form==ft_omega)  {
        abend(SETL_SYSTEM message,s,l);
  }

  push_pstack(left);
  call_procedure(SETL_SYSTEM target,
                 &spare1,
                 NULL,
                 1,NO,YES,0);

}


int execute_go(
   SETL_SYSTEM_PROTO
   int forever)

{
#ifdef HAVE_GETRUSAGE
struct timeval tvspam;
struct timezone tzspam;
#endif

   WAIT_FLAG=0;
   //printf("EXECUTE_GO!!\n");
   if (abend_initialized==0) { 

   if (setjmp(abend_env)==1) {
     //  while (cstack_top >= 0 && cstack[cstack_top].cs_pc != NULL) {
      //   pop_cstack;
      // }
      // if (cstack_top >= 0) {
      //    pop_cstack;
      // }
      // if (cstack_top < -1)
      //    cstack_top = -1;
//critical_section = 0;
// opcodes_until_switch = 2000;
// pstack_top = -1;                 /* top of program stack              */
// pstack_max = 0;                  /* size of program stack             */
// cstack_top = -1;                 /* top of call stack                 */
//cstack_max = 0;                  /* size of call stack                */
		abend_initialized=0;
		hard_stop=1;
       return NO;
   
   }
   abend_initialized=1;
   }

   execute_start:

      OPCODE_COUNT++;
       
      if (hard_stop>0) {
      	 return NO;
      }
#ifdef PROCESSES

      if ((--opcodes_until_switch) <= 0)
         switch_process(SETL_SYSTEM_VOID);

#endif

#ifdef DEBUG

      ip = pc++;

      if ((PROF_DEBUG) && ip->i_opcode) {
         OPCODE_EXECUTED=ip->i_opcode;
         pcode_operations[OPCODE_EXECUTED]++;

      }

      if ((TRACING_ON) && ip->i_opcode != p_filepos) {

         /* trace mode */

       if (!PROF_DEBUG) {
         fprintf(DEBUG_FILE,"PCODE => %-13s %4ld %4ld",
                 pcode_desc[ip->i_opcode],
                 source_line,
                 source_column);
         fflush(DEBUG_FILE);

         if (STEP_DEBUG) {
            getchar();
         }
         fputs("\n",DEBUG_FILE);
       } else {

         if (source_unittab!=NULL) {
#ifdef HAVE_GETRUSAGE
            gettimeofday(&tvspam,&tzspam);
            if (profi!=NULL) {
               profi->time.tv_sec +=(tvspam.tv_sec-prf_time.tv_sec);
               profi->time.tv_usec+=(tvspam.tv_usec-prf_time.tv_usec);
               while (profi->time.tv_usec<0) {
                  profi->time.tv_sec--;
                  profi->time.tv_usec+=1000000;
               }
               while (profi->time.tv_usec>=1000000) {
                  profi->time.tv_sec++;
                  profi->time.tv_usec-=1000000;
               }
            } 
#endif
            profi=&source_unittab->ut_prof_table[source_line]; 
            profi->count++;
#ifdef HAVE_GETRUSAGE
            prf_time.tv_sec = tvspam.tv_sec;
            prf_time.tv_usec = tvspam.tv_usec;
#endif
         }
       }

      }

      switch (ip->i_opcode) {

#else

      switch ((ip = pc++)->i_opcode) {

#endif

/*\
 *  \pcode{p\_noop}{no operation}
 *        {}{unused}
 *        {}{unused}
 *        {}{unused}
 *
 *  This is just a null operation.  The compiler should only generate
 *  noop's when an instruction needs more than three operands, in which
 *  case it will generate a noop for the extra operands.  Those
 *  instructions should automatically skip the following noop.  We
 *  include a noop here primarily to document its existence.
\*/

case p_noop :

#ifdef TRAPS

   trap(__FILE__,__LINE__,msg_noop_executed);

#endif

   break;

/*\
 *  \pcode{p\_noop}{no operation}
 *        {}{unused}
 *        {}{unused}
 *        {}{unused}
 *
 *  This is just a null operation.  The compiler should only generate
 *  noop's when an instruction needs more than three operands, in which
 *  case it will generate a noop for the extra operands.  Those
 *  instructions should automatically skip the following noop.  We
 *  include a noop here primarily to document its existence.
\*/

#ifdef DEBUG

case p_filepos :

   if ((!EX_DEBUG)&&(!PROF_DEBUG))
      break;


   if (strcmp((ip->i_operand[0].i_class_ptr)->ut_source_name,
              X_SOURCE_NAME) != 0) {

      strcpy(X_SOURCE_NAME,(ip->i_operand[0].i_class_ptr)->ut_source_name);
      if (!PROF_DEBUG)
         fprintf(DEBUG_FILE,"\nSETL2 source file => %s\n\n",X_SOURCE_NAME);

   }

   if (ip->i_operand[1].i_integer > 0) {

      source_line = ip->i_operand[1].i_integer;
      source_column = ip->i_operand[2].i_integer;
      source_unittab=ip->i_operand[0].i_class_ptr;

   }

   break;

#endif

/*\
 *  \pcode{p\_erase}
 *
 *  Identical to p_sof, but has effect only on sets, tuples and maps
 *
\*/

case p_erase :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the target operand */

   switch (target->sp_form) {

      /*
       *  We modify strings in-line, since this should be very fast.
       */

      case ft_string :
         
		break;

      case ft_set :

         if (!set_to_map(SETL_SYSTEM target,target,NO))
            abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM target));

      case ft_map :
      	 if (left->sp_form==ft_omega)
      	 	abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         /* we would like to use the target destructively */

         map_root = target->sp_val.sp_map_ptr;
         if (map_root->m_use_count != 1) {

            map_root->m_use_count--;
            map_root = copy_map(SETL_SYSTEM map_root);
            target->sp_val.sp_map_ptr = map_root;

         }

         /* look up the domain element in the map */

         map_work_hdr = map_root;
         spec_hash_code(work_hash_code,right);
         range_hash_code = work_hash_code;
         spec_hash_code(work_hash_code,left);
         domain_hash_code = work_hash_code;

         for (source_height = map_work_hdr->m_ntype.m_root.m_height;
              source_height;
              source_height--) {

            /* extract the element's index at this level */

            source_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

            /* if we're missing a header record, insert it */

            if (map_work_hdr->m_child[source_index].m_header == NULL) {

               get_map_header(new_map_hdr);
               new_map_hdr->m_ntype.m_intern.m_parent = map_work_hdr;
               new_map_hdr->m_ntype.m_intern.m_child_index = source_index;
               for (i = 0;
                    i < MAP_HASH_SIZE;
                    new_map_hdr->m_child[i++].m_cell = NULL);
               map_work_hdr->m_child[source_index].m_header = new_map_hdr;
               map_work_hdr = new_map_hdr;

            }
            else {

               map_work_hdr = map_work_hdr->m_child[source_index].m_header;

            }
         }

         /*
          *  At this point, map_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */

         source_index = work_hash_code & MAP_HASH_MASK;
         map_cell_tail = &(map_work_hdr->m_child[source_index].m_cell);
         for (map_cell = map_work_hdr->m_child[source_index].m_cell;
              map_cell != NULL &&
                 map_cell->m_hash_code < domain_hash_code;
              map_cell = map_cell->m_next) {

            map_cell_tail = &(map_cell->m_next);

         }

         /* try to find the domain element */

         is_equal = NO;
         while (map_cell != NULL &&
                map_cell->m_hash_code == domain_hash_code) {

            spec_equal(is_equal,&(map_cell->m_domain_spec),left);

            if (is_equal)
               break;

            map_cell_tail = &(map_cell->m_next);
            map_cell = map_cell->m_next;

         }

         /* if the source operand is omega, remove the cell */

         if (right->sp_form == ft_omega) {

            if (!is_equal)
               break;

            spec_hash_code(work_hash_code,&(map_cell->m_range_spec));
            map_root->m_hash_code ^= work_hash_code;

            if (map_cell->m_is_multi_val) {

               set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
               map_root->m_ntype.m_root.m_cardinality -=
                     set_root->s_ntype.s_root.s_cardinality;

               if ((set_root->s_ntype.s_root.s_cardinality%2)==0) 
                  map_root->m_hash_code ^= map_cell->m_hash_code;

            }
            else {

               map_root->m_ntype.m_root.m_cardinality--;

            }

            map_root->m_hash_code ^= map_cell->m_hash_code;
            *map_cell_tail = map_cell->m_next;
            unmark_specifier(&(map_cell->m_domain_spec));
            unmark_specifier(&(map_cell->m_range_spec));
            map_root->m_ntype.m_root.m_cell_count--;
            free_map_cell(map_cell);

            /*
             *  We've reduced the number of cells, so we may have to
             *  contract the map header.
             */

            contraction_trigger =
                  (1 << ((map_root->m_ntype.m_root.m_height)
                          * MAP_SHIFT_DIST));
            if (contraction_trigger == 1)
               contraction_trigger = 0;

            if (map_root->m_ntype.m_root.m_cell_count <
                contraction_trigger) {

               map_root = map_contract_header(SETL_SYSTEM map_root);
               target->sp_val.sp_map_ptr = map_root;

            }

            break;

         }

         /*
          *  The source is not omega, so we add or change a cell.
          */

         /* if we don't find the domain element, add a cell */

         if (!is_equal) {

            get_map_cell(new_map_cell);
            mark_specifier(left);
            mark_specifier(right);
            new_map_cell->m_domain_spec.sp_form = left->sp_form;
            new_map_cell->m_domain_spec.sp_val.sp_biggest =
                  left->sp_val.sp_biggest;
            new_map_cell->m_range_spec.sp_form = right->sp_form;
            new_map_cell->m_range_spec.sp_val.sp_biggest =
                  right->sp_val.sp_biggest;
            new_map_cell->m_is_multi_val = NO;
            new_map_cell->m_hash_code = domain_hash_code;
            new_map_cell->m_next = *map_cell_tail;
            *map_cell_tail = new_map_cell;
            map_root->m_ntype.m_root.m_cardinality++;
            map_root->m_ntype.m_root.m_cell_count++;
            map_root->m_hash_code ^= domain_hash_code;
            map_root->m_hash_code ^= range_hash_code;

            /* expand the map header if necessary */

            expansion_trigger =
                  (1 << ((map_root->m_ntype.m_root.m_height + 1)
                         * MAP_SHIFT_DIST)) * 2;
            if (map_root->m_ntype.m_root.m_cardinality >
                expansion_trigger) {

               map_root = map_expand_header(SETL_SYSTEM map_root);
               target->sp_val.sp_map_ptr = map_root;

            }

            break;

         }

         /* if we had a multi-value cell, decrease the cardinality */

         if (map_cell->m_is_multi_val) {

            set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
            map_root->m_ntype.m_root.m_cardinality -=
                  (set_root->s_ntype.s_root.s_cardinality - 1);

            if ((set_root->s_ntype.s_root.s_cardinality%2)==0) 
               map_root->m_hash_code ^= map_cell->m_hash_code;
         }

         /* set the range element */


         spec_hash_code(work_hash_code,&(map_cell->m_range_spec));
         map_root->m_hash_code ^= work_hash_code;

         mark_specifier(right);
         unmark_specifier(&(map_cell->m_range_spec));
         map_cell->m_range_spec.sp_form = right->sp_form;
         map_cell->m_range_spec.sp_val.sp_biggest =
               right->sp_val.sp_biggest;
         map_cell->m_is_multi_val = NO;
         map_root->m_hash_code ^= range_hash_code;

         break;

      /*
       *  We modify tuple elements in-line, since this should be a very
       *  fast operation.
       */

      case ft_tuple :

         /* we convert the tuple index to a C long */

         if (left->sp_form == ft_short) {

            short_value = left->sp_val.sp_short_value;
            if (short_value < 0)
               short_value =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  short_value + 1;
            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else if (left->sp_form == ft_long) {

            short_value = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
            if (short_value < 0)
               short_value =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  short_value + 1;
            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }

         tuple_root = target->sp_val.sp_tuple_ptr;

         /* we would like to use the target destructively */

         if (tuple_root->t_use_count != 1) {

            tuple_root->t_use_count--;
            tuple_root = copy_tuple(SETL_SYSTEM tuple_root);
            target->sp_val.sp_tuple_ptr = tuple_root;

         }

         /* our indices are zero-based */

         short_value--;

         /* if the item is past the end of the tuple, extend the tuple */

         if (short_value >= tuple_root->t_ntype.t_root.t_length) {

            expansion_trigger =
                  1 << ((tuple_root->t_ntype.t_root.t_height + 1)
                        * TUP_SHIFT_DIST);

            /* expand the target tree if necessary */

            while (short_value >= expansion_trigger) {

               tuple_work_hdr = tuple_root;

               get_tuple_header(tuple_root);

               tuple_root->t_use_count = 1;
               tuple_root->t_hash_code =
                  tuple_work_hdr->t_hash_code;
               tuple_root->t_ntype.t_root.t_length =
                  tuple_work_hdr->t_ntype.t_root.t_length;
               tuple_root->t_ntype.t_root.t_height =
                  tuple_work_hdr->t_ntype.t_root.t_height + 1;

               for (i = 1;
                    i < TUP_HEADER_SIZE;
                    tuple_root->t_child[i++].t_header = NULL);

               tuple_root->t_child[0].t_header = tuple_work_hdr;

               tuple_work_hdr->t_ntype.t_intern.t_parent = tuple_root;
               tuple_work_hdr->t_ntype.t_intern.t_child_index = 0;

               expansion_trigger *= TUP_HEADER_SIZE;

            }

            tuple_root->t_ntype.t_root.t_length = short_value + 1;
            target->sp_val.sp_tuple_ptr = tuple_root;

         }

         /* look up the tuple component */

         tuple_work_hdr = tuple_root;

         for (source_height = tuple_work_hdr->t_ntype.t_root.t_height;
              source_height;
              source_height--) {

            /* extract the element's index at this level */

            source_index = (short_value >>
                                 (source_height * TUP_SHIFT_DIST)) &
                              TUP_SHIFT_MASK;

            /* if we're missing a header record, allocate one */

            if (tuple_work_hdr->t_child[source_index].t_header == NULL) {

               get_tuple_header(new_tuple_hdr);
               new_tuple_hdr->t_ntype.t_intern.t_parent = tuple_work_hdr;
               new_tuple_hdr->t_ntype.t_intern.t_child_index = source_index;
               for (i = 0;
                    i < TUP_HEADER_SIZE;
                    new_tuple_hdr->t_child[i++].t_cell = NULL);
               tuple_work_hdr->t_child[source_index].t_header =
                     new_tuple_hdr;
               tuple_work_hdr = new_tuple_hdr;

            }
            else {

               tuple_work_hdr =
                  tuple_work_hdr->t_child[source_index].t_header;

            }
         }

         /*
          *  At this point, tuple_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */

         source_index = short_value & TUP_SHIFT_MASK;
         tuple_cell = tuple_work_hdr->t_child[source_index].t_cell;

         /* if we assign omega to the last cell of a tuple, we shorten it */

         if (right->sp_form == ft_omega &&
             short_value == (tuple_root->t_ntype.t_root.t_length - 1)) {

            /* first free the tuple cell */

            if (tuple_cell != NULL) {

               tuple_root->t_hash_code ^=
                     tuple_cell->t_hash_code;
               unmark_specifier(&(tuple_cell->t_spec));
               free_tuple_cell(tuple_cell);
               tuple_work_hdr->t_child[source_index].t_cell = NULL;

            }

            /* now shorten the tuple while the rightmost element is omega */

            for (;;) {

               if (!source_height && source_index >= 0) {

                  if (tuple_work_hdr->t_child[source_index].t_cell != NULL)
                     break;

                  tuple_root->t_ntype.t_root.t_length--;
                  source_index--;

                  continue;

               }

               /* move up if we're at the end of a node */

               if (source_index < 0) {

                  if (tuple_work_hdr == tuple_root)
                     break;

                  source_height++;
                  source_index =
                     tuple_work_hdr->t_ntype.t_intern.t_child_index;
                  tuple_work_hdr =
                     tuple_work_hdr->t_ntype.t_intern.t_parent;
                  free_tuple_header(
                     tuple_work_hdr->t_child[source_index].t_header);
                  tuple_work_hdr->t_child[source_index].t_header = NULL;
                  source_index--;

                  continue;

               }

               /* skip over null nodes */

               if (tuple_work_hdr->t_child[source_index].t_header == NULL) {

                  tuple_root->t_ntype.t_root.t_length -=
                        1 << (source_height * TUP_SHIFT_DIST);
                  source_index--;

                  continue;

               }

               /* otherwise drop down a level */

               tuple_work_hdr =
                  tuple_work_hdr->t_child[source_index].t_header;
               source_index = TUP_HEADER_SIZE - 1;

               source_height--;

            }

            /* we've shortened the tuple -- now reduce the height */

            while (tuple_root->t_ntype.t_root.t_height > 0 &&
                   tuple_root->t_ntype.t_root.t_length <=
                      (int32)(1L << (tuple_root->t_ntype.t_root.t_height *
                                    TUP_SHIFT_DIST))) {

               tuple_work_hdr = tuple_root->t_child[0].t_header;

               /* it's possible that we deleted internal headers */

               if (tuple_work_hdr == NULL) {

                  tuple_root->t_ntype.t_root.t_height--;
                  continue;

               }

               /* delete the root node */

               tuple_work_hdr->t_use_count = tuple_root->t_use_count;
               tuple_work_hdr->t_hash_code =
                     tuple_root->t_hash_code;
               tuple_work_hdr->t_ntype.t_root.t_length =
                     tuple_root->t_ntype.t_root.t_length;
               tuple_work_hdr->t_ntype.t_root.t_height =
                     tuple_root->t_ntype.t_root.t_height - 1;

               free_tuple_header(tuple_root);
               tuple_root = tuple_work_hdr;
               target->sp_val.sp_tuple_ptr = tuple_root;

            }

            break;

         }

         /* we aren't assigning omega, or we're not at the end of a tuple */

         if (tuple_cell == NULL) {

            if (right->sp_form == ft_omega)
               break;

            get_tuple_cell(tuple_cell);
            tuple_work_hdr->t_child[source_index].t_cell = tuple_cell;
            mark_specifier(right);
            tuple_cell->t_spec.sp_form = right->sp_form;
            tuple_cell->t_spec.sp_val.sp_biggest = right->sp_val.sp_biggest;
            spec_hash_code(work_hash_code,right);
            tuple_root->t_hash_code ^= work_hash_code;
            tuple_cell->t_hash_code = work_hash_code;

            break;

         }

         if (right->sp_form == ft_omega) {

            unmark_specifier(&(tuple_cell->t_spec));
            tuple_root->t_hash_code ^=
                  tuple_cell->t_hash_code;
            free_tuple_cell(tuple_cell);
            tuple_work_hdr->t_child[source_index].t_cell = NULL;

            break;

         }

         mark_specifier(right);
         unmark_specifier(&(tuple_cell->t_spec));
         tuple_root->t_hash_code ^= tuple_cell->t_hash_code;
         tuple_cell->t_spec.sp_form = right->sp_form;
         tuple_cell->t_spec.sp_val.sp_biggest = right->sp_val.sp_biggest;
         spec_hash_code(work_hash_code,right);
         tuple_root->t_hash_code ^= work_hash_code;
         tuple_cell->t_hash_code = work_hash_code;

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         break;

   }

   break;

}

/*\
 *  \pcode{p\_smap}{convert set to single-valued map}
 *        {specifier}{set}
 *        {}{unused}
 *        {}{unused}
 *
 *  This opcode converts a set to a single-valued map.  It is used for
 *  run-time prevention of duplicate case labels.
\*/

case p_smap :

   set_to_smap(SETL_SYSTEM ip->i_operand[0].i_spec_ptr,
               ip->i_operand[0].i_spec_ptr);

   break;

/*\
 *  \pcode{p\_push1}{push one operand}
 *        {specifier}{value to be pushed}
 *        {}{unused}
 *        {}{unused}
 *
 *  This case pushes one operand on the program stack.  We have three
 *  simple push operands, which push one, two, or three operands on the
 *  stack.  This is just an attempt to make better use of the pseudo-code
 *  instruction width -- we cut the number of instructions necessary to
 *  push a set of operands on the stack.
\*/

case p_push1 :

   push_pstack(ip->i_operand[0].i_spec_ptr);

   break;

/*\
 *  \pcode{p\_push2}{push two operands}
 *        {specifier}{first value to be pushed}
 *        {specifier}{second value to be pushed}
 *        {}{unused}
\*/

case p_push2 :

   push_pstack(ip->i_operand[0].i_spec_ptr);
   push_pstack(ip->i_operand[1].i_spec_ptr);

   break;

/*\
 *  \pcode{p\_push3}{push three operands}
 *        {specifier}{first value to be pushed}
 *        {specifier}{second value to be pushed}
 *        {specifier}{third value to be pushed}
\*/

case p_push3 :

   push_pstack(ip->i_operand[0].i_spec_ptr);
   push_pstack(ip->i_operand[1].i_spec_ptr);
   push_pstack(ip->i_operand[2].i_spec_ptr);

   break;

/*\
 *  \pcode{p\_pop1}{pop one operand}
 *        {specifier}{specifier to be set}
 *        {}{unused}
 *        {}{unused}
 *
 *  This case pops one operand on the program stack.  We have three
 *  simple pop operands, which pop one, two, or three operands from the
 *  stack.  This is just an attempt to make better use of the pseudo-code
 *  instruction width -- we cut the number of instructions necessary to
 *  pop a set of operands from the stack.
\*/

case p_pop1 :

   target = ip->i_operand[0].i_spec_ptr;
   unmark_specifier(target);
   target->sp_form = pstack[pstack_top].sp_form;
   target->sp_val.sp_biggest = pstack[pstack_top].sp_val.sp_biggest;
   pstack_top--;

   break;

/*\
 *  \pcode{p\_pop2}{pop two operands}
 *        {specifier}{first specifier to be popped}
 *        {specifier}{second specifier to be popped}
 *        {}{unused}
\*/

case p_pop2 :

   target = ip->i_operand[0].i_spec_ptr;
   unmark_specifier(target);
   target->sp_form = pstack[pstack_top].sp_form;
   target->sp_val.sp_biggest = pstack[pstack_top].sp_val.sp_biggest;
   pstack_top--;

   target = ip->i_operand[1].i_spec_ptr;
   unmark_specifier(target);
   target->sp_form = pstack[pstack_top].sp_form;
   target->sp_val.sp_biggest = pstack[pstack_top].sp_val.sp_biggest;
   pstack_top--;

   break;

/*\
 *  \pcode{p\_pop3}{pop three operands}
 *        {specifier}{first specifier to be popped}
 *        {specifier}{second specifier to be popped}
 *        {specifier}{third specifier to be popped}
\*/

case p_pop3 :

   target = ip->i_operand[0].i_spec_ptr;
   unmark_specifier(target);
   target->sp_form = pstack[pstack_top].sp_form;
   target->sp_val.sp_biggest = pstack[pstack_top].sp_val.sp_biggest;
   pstack_top--;

   target = ip->i_operand[1].i_spec_ptr;
   unmark_specifier(target);
   target->sp_form = pstack[pstack_top].sp_form;
   target->sp_val.sp_biggest = pstack[pstack_top].sp_val.sp_biggest;
   pstack_top--;

   target = ip->i_operand[2].i_spec_ptr;
   unmark_specifier(target);
   target->sp_form = pstack[pstack_top].sp_form;
   target->sp_val.sp_biggest = pstack[pstack_top].sp_val.sp_biggest;
   pstack_top--;

   break;

/*\
 *  \pcode{p\_lcall}{literal procedure call}
 *        {specifier}{returned value}
 *        {specifier}{procedure}
 *        {integer}{number of actual parameters}
 *
 *  This case handles procedure calls.  There are actually three opcodes
 *  which handle calls, this one is invoked for calls to literal
 *  procedures.
\*/

case p_lcall :

{

   /* pick out the operand pointers, to save some expression evaluations */

   left = ip->i_operand[1].i_spec_ptr;

   /* at this point we expect to find a procedure */

   if (left->sp_form != ft_proc)
      abend(SETL_SYSTEM msg_expected_proc,
            abend_opnd_str(SETL_SYSTEM left));

   call_procedure(SETL_SYSTEM ip->i_operand[0].i_spec_ptr,
                  left,
                  NULL,
                  ip->i_operand[2].i_integer,NO,YES,0);

   if ((WAIT_FLAG<0)&&(forever == NO))
      return WAIT_FLAG;

   break;

}

/*\
 *  \pcode{p\_call}{procedure call}
 *        {specifier}{returned value}
 *        {specifier}{procedure}
 *        {integer}{number of actual parameters}
 *
 *  This case handles procedure calls.  There are actually three opcodes
 *  which handle calls, this one is invoked for calls to literal
 *  procedures.
\*/

case p_call :

{

   /* pick out the operand pointers, to save some expression evaluations */

   left = ip->i_operand[1].i_spec_ptr;

   /* at this point we expect to find a procedure */

   if (left->sp_form != ft_proc) {
      abend(SETL_SYSTEM msg_expected_proc,
            abend_opnd_str(SETL_SYSTEM left));
   }

   call_procedure(SETL_SYSTEM ip->i_operand[0].i_spec_ptr,
                  left,
                  NULL,
                  ip->i_operand[2].i_integer,NO,NO,0);

   break;

}

/*\
 *  \pcode{p\_return}{procedure return}
 *        {specifier}{returned value}
 *        {}{unused}
 *        {}{unused}
 *
 *  This instruction returns from a user-defined procedure.  First we
 *  copy the return value to the location desired, then we pop all the
 *  local variables and actual parameters from the stack, and reset the
 *  program counter.
 *
 *  If the environment of the current procedure is to be saved, we save
 *  it.  Similarly, we copy the current instance if we have one.
\*/

case p_return :

{

   /* pick out the operand pointer, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;

   /* save the result in spare */

   spare.sp_form = target->sp_form;
   spare.sp_val.sp_biggest = target->sp_val.sp_biggest;
   mark_specifier(&spare);

   /* save the new pstack pointer */

   return_pstack_top = pstack_top;
   save_pstack_top = cstack[cstack_top].cs_pstack_top;
   pstack_top = save_pstack_top;

   /* pick out the procedure pointer */

   proc_ptr = cstack[cstack_top].cs_proc_ptr;
   process_ptr = cstack[cstack_top].cs_process_ptr;

   /* check for a self to be restored */

   class_ptr = current_class;
   self_target = cstack[cstack_top].cs_self_ptr;
   if (self_target != NULL) {
      self_root = (class_ptr->ut_self)->ss_object;
      self_root->o_hash_code = (int32)class_ptr;
   }
   else {
      self_root = proc_ptr->p_self_ptr;
   }

   /*
    *  The procedure for returning from a process method is fairly different
    *  from that of returning from other procedures.  We handle that case
    *  here.
    */

   if (process_ptr != NULL) {

      object_root = process_ptr->pc_object_ptr;
      object_work_hdr = object_root;
      target_height = class_ptr->ut_obj_height;

      /* loop over the instance variables */

      for (slot_info = class_ptr->ut_first_var, target_number = 0;
           slot_info != NULL;
           slot_info = slot_info->si_next_var, target_number++) {

         /* drop down to a leaf */

         while (target_height) {

            /* extract the element's index at this level */

            target_index = (target_number >>
                                 (target_height * OBJ_SHIFT_DIST)) &
                              OBJ_SHIFT_MASK;

            /* if the header is missing, allocate one */

            if (object_work_hdr->o_child[target_index].o_header ==
                  NULL) {

               get_object_header(new_object_hdr);
               new_object_hdr->o_ntype.o_intern.o_parent =
                  object_work_hdr;
               new_object_hdr->o_ntype.o_intern.o_child_index =
                  target_index;

               for (i = 0;
                    i < OBJ_HEADER_SIZE;
                    new_object_hdr->o_child[i++].o_cell = NULL);

               object_work_hdr->o_child[target_index].o_header =
                     new_object_hdr;
               object_work_hdr = new_object_hdr;

            }

            /* otherwise just drop down a level */

            else {

               object_work_hdr =
                  object_work_hdr->o_child[target_index].o_header;

            }

            target_height--;

         }

         /*
          *  At this point, object_work_hdr points to the lowest
          *  level header record.  We insert the new element in the
          *  appropriate slot.
          */

         target_index = target_number & OBJ_SHIFT_MASK;
         object_cell = object_work_hdr->o_child[target_index].o_cell;
         if (object_cell == NULL) {
            get_object_cell(object_cell);
            object_work_hdr->o_child[target_index].o_cell = object_cell;
         }
         target_element = slot_info->si_spec;
         object_cell->o_spec.sp_form = target_element->sp_form;
         object_cell->o_spec.sp_val.sp_biggest =
            target_element->sp_val.sp_biggest;

         /*
          *  We move back up the header tree at this point, if it is
          *  necessary.
          */

         target_index++;
         while (target_index >= OBJ_HEADER_SIZE) {

            target_height++;
            target_index =
               object_work_hdr->o_ntype.o_intern.o_child_index + 1;
            object_work_hdr =
               object_work_hdr->o_ntype.o_intern.o_parent;

         }
      }
      
      self_root = NULL;

   }

   /* pop a saved 'self', if we have one */

   if (self_root != NULL) {

      self_stack_ptr = (class_ptr->ut_self)->ss_next;

      /* if the current instance is not already loaded, push it */

      if (self_stack_ptr == NULL ||
          self_stack_ptr->ss_object != self_root) {

         /*
          *  The following block of code saves the current 'self' for
          *  a class.  We have to worry about hash codes here iff there
          *  is a target self we will later assign.
          */

         object_root = self_root;
         object_work_hdr = object_root;
         target_height = class_ptr->ut_obj_height;

         /* loop over the instance variables */

         for (slot_info = class_ptr->ut_first_var, target_number = 0;
              slot_info != NULL;
              slot_info = slot_info->si_next_var, target_number++) {

            /* drop down to a leaf */

            while (target_height) {

               /* extract the element's index at this level */

               target_index = (target_number >>
                                    (target_height * OBJ_SHIFT_DIST)) &
                                 OBJ_SHIFT_MASK;

               /* if the header is missing, allocate one */

               if (object_work_hdr->o_child[target_index].o_header ==
                     NULL) {

                  get_object_header(new_object_hdr);
                  new_object_hdr->o_ntype.o_intern.o_parent =
                     object_work_hdr;
                  new_object_hdr->o_ntype.o_intern.o_child_index =
                     target_index;

                  for (i = 0;
                       i < OBJ_HEADER_SIZE;
                       new_object_hdr->o_child[i++].o_cell = NULL);

                  object_work_hdr->o_child[target_index].o_header =
                        new_object_hdr;
                  object_work_hdr = new_object_hdr;

               }

               /* otherwise just drop down a level */

               else {

                  object_work_hdr =
                     object_work_hdr->o_child[target_index].o_header;

               }

               target_height--;

            }

            /*
             *  At this point, object_work_hdr points to the lowest
             *  level header record.  We insert the new element in the
             *  appropriate slot.
             */

            target_index = target_number & OBJ_SHIFT_MASK;
            object_cell = object_work_hdr->o_child[target_index].o_cell;
            if (object_cell == NULL) {
               get_object_cell(object_cell);
               object_work_hdr->o_child[target_index].o_cell = object_cell;
            }
            target_element = slot_info->si_spec;
            object_cell->o_spec.sp_form = target_element->sp_form;
            object_cell->o_spec.sp_val.sp_biggest =
               target_element->sp_val.sp_biggest;
            if (self_target != NULL) {
               spec_hash_code(object_cell->o_hash_code,target_element);
               object_root->o_hash_code ^= object_cell->o_hash_code;
            }

            /*
             *  We move back up the header tree at this point, if it is
             *  necessary.
             */

            target_index++;
            while (target_index >= OBJ_HEADER_SIZE) {

               target_height++;
               target_index =
                  object_work_hdr->o_ntype.o_intern.o_child_index + 1;
               object_work_hdr =
                  object_work_hdr->o_ntype.o_intern.o_parent;

            }
         }
      }

      /* if the old self is not already there, install it */

      if (self_stack_ptr != NULL &&
          self_stack_ptr->ss_object != self_root) {

         object_root = self_stack_ptr->ss_object;
         object_work_hdr = object_root;
         target_height = class_ptr->ut_obj_height;

         /* loop over the instance variables */

         for (slot_info = class_ptr->ut_first_var, target_number = 0;
              slot_info != NULL;
              slot_info = slot_info->si_next_var, target_number++) {

            /* drop down to a leaf */

            while (target_height) {

               /* extract the element's index at this level */

               target_index = (target_number >>
                                    (target_height * OBJ_SHIFT_DIST)) &
                                 OBJ_SHIFT_MASK;

               /* we'll always have all internal nodes in this situation */

               object_work_hdr =
                  object_work_hdr->o_child[target_index].o_header;
               target_height--;

            }

            /*
             *  At this point, object_work_hdr points to the lowest
             *  level header record.  We copy the appropriate element
             *  to the instance variable.
             */

            target_index = target_number & OBJ_SHIFT_MASK;
            object_cell = object_work_hdr->o_child[target_index].o_cell;
            target_element = slot_info->si_spec;
            target_element->sp_form = object_cell->o_spec.sp_form;
            target_element->sp_val.sp_biggest =
               object_cell->o_spec.sp_val.sp_biggest;

            /*
             *  We move back up the header tree at this point, if it is
             *  necessary.
             */

            target_index++;
            while (target_index >= OBJ_HEADER_SIZE) {

               target_height++;
               target_index =
                  object_work_hdr->o_ntype.o_intern.o_child_index + 1;
               object_work_hdr =
                  object_work_hdr->o_ntype.o_intern.o_parent;

            }
         }
      }

      /* pop the old 'self' */

      free_self_stack(class_ptr->ut_self);
      class_ptr->ut_self = self_stack_ptr;

   }

   /* if we've copied the procedure, save the current variables */

   if (proc_ptr->p_copy != NULL) {

      new_proc_ptr = proc_ptr->p_copy;
      if (!(--(new_proc_ptr->p_use_count))) {
         free_procedure(SETL_SYSTEM new_proc_ptr);
         proc_ptr->p_copy = NULL;
      }
      else {
         new_proc_ptr->p_save_specs =
            get_specifiers(SETL_SYSTEM new_proc_ptr->p_spec_count);

         for (arg_ptr = proc_ptr->p_spec_ptr,
                 stack_ptr = new_proc_ptr->p_save_specs;
              arg_ptr < proc_ptr->p_spec_ptr + proc_ptr->p_spec_count;
              arg_ptr++, stack_ptr++) {

            stack_ptr->sp_form = arg_ptr->sp_form;
            stack_ptr->sp_val.sp_biggest = arg_ptr->sp_val.sp_biggest;
            mark_specifier(stack_ptr);

         }

         proc_ptr->p_copy = NULL;
         new_proc_ptr->p_active_use_count = 0;

      }
   }

   /*
    * If this wasn't a call to a process method, we have to restore
    *  local variables.
    */

   if (process_ptr == NULL) {

      /* replace the procedure's local variables */

      stack_pos = pstack + (pstack_top + 1) - proc_ptr->p_spec_count;

      for (arg_ptr = proc_ptr->p_spec_ptr;
           arg_ptr < proc_ptr->p_spec_ptr + proc_ptr->p_spec_count;
           arg_ptr++, stack_pos++) {

         unmark_specifier(arg_ptr);
         arg_ptr->sp_form = stack_pos->sp_form;
         arg_ptr->sp_val.sp_biggest = stack_pos->sp_val.sp_biggest;

      }

      pstack_top -= proc_ptr->p_spec_count;

      /* pop the actual parameters */

      pstack_top -= proc_ptr->p_formal_count;

      /* copy self, if desired */

      if (self_target != NULL) {

         unmark_specifier(self_target);
         self_target->sp_form = ft_object;
         self_target->sp_val.sp_object_ptr = self_root;

      }
      else if (self_root != NULL) {

         if (!(--(self_root->o_use_count))) {
            free_object(SETL_SYSTEM self_root);
         }
      }

      /* slide down any pushed write parameters */

      if (return_pstack_top != save_pstack_top) {

         memcpy((void *)(pstack + pstack_top + 1),
                (void *)(pstack + save_pstack_top + 1),
                (size_t)((return_pstack_top -
                          save_pstack_top) * sizeof(specifier)));

         pstack_top += (return_pstack_top - save_pstack_top);

      }
   }

   /*
    *  We have to free the local variables for process calls.
    */

   else {

      for (arg_ptr = proc_ptr->p_spec_ptr;
           arg_ptr < proc_ptr->p_spec_ptr + proc_ptr->p_spec_count;
           arg_ptr++) {

         unmark_specifier(arg_ptr);
         arg_ptr->sp_form = ft_omega;

      }
   }

   /* decrement the parent's usage count, and unload unused procedures */

   if (!cstack[cstack_top].cs_literal_proc) {

      /*
       *  Now that we've installed the relevant 'self', we have to load
       *  the saved environment.
       */

      new_proc_ptr = proc_ptr->p_parent;
      while (new_proc_ptr != NULL && !(--new_proc_ptr->p_active_use_count)) {

         critical_section--;

         /* swap saved and active data */

         for (arg_ptr = new_proc_ptr->p_spec_ptr,
                 stack_ptr = new_proc_ptr->p_save_specs;
              arg_ptr < new_proc_ptr->p_spec_ptr + new_proc_ptr->p_spec_count;
              arg_ptr++, stack_ptr++) {

            memcpy((void *)&spare1,
                   (void *)arg_ptr,
                   sizeof(struct specifier_item));

            memcpy((void *)arg_ptr,
                   (void *)stack_ptr,
                   sizeof(struct specifier_item));

            memcpy((void *)stack_ptr,
                   (void *)&spare1,
                   sizeof(struct specifier_item));

            spare1.sp_form = ft_omega;

         }

         new_proc_ptr = new_proc_ptr->p_parent;

      }
   }

   /* copy the return pointer to the return value pointer, if desired */

   if (process_ptr == NULL) {

      if (cstack[cstack_top].cs_return_value != NULL) {

         unmark_specifier(cstack[cstack_top].cs_return_value);
         cstack[cstack_top].cs_return_value->sp_form = spare.sp_form;
         cstack[cstack_top].cs_return_value->sp_val.sp_biggest =
            spare.sp_val.sp_biggest;

      }
      else {
         unmark_specifier(&spare);
      }

      spare.sp_form = ft_omega;

   }
   else {
   
      request_ptr = process_head->pc_request_head;

      /* check for a mailbox for the return value */

      mailbox_ptr = request_ptr->rq_mailbox_ptr;
      if (mailbox_ptr != NULL) {
      
         get_mailbox_cell(mailbox_cell);
         *(mailbox_ptr->mb_tail) = mailbox_cell;
         mailbox_ptr->mb_tail = &(mailbox_cell->mb_next);
         mailbox_cell->mb_next = NULL;

         mailbox_ptr->mb_cell_count++;

         mailbox_cell->mb_next = NULL;
         mailbox_cell->mb_spec.sp_form = spare.sp_form;
         mailbox_cell->mb_spec.sp_val.sp_biggest = spare.sp_val.sp_biggest;
 
      }
      else {
         unmark_specifier(&spare);
      }
      spare.sp_form = ft_omega;

      /* remove the request record */

      process_head->pc_request_head = request_ptr->rq_next;
      if (process_head->pc_request_head == NULL)
         process_head->pc_request_tail = &(process_head->pc_request_head);         
      free((void *)(request_ptr->rq_args));
      free_request(request_ptr);

      if (!(--(process_head->pc_object_ptr->o_use_count)))
         free_object(SETL_SYSTEM process_head->pc_object_ptr);

      /* make sure we switch on the next cycle */

      opcodes_until_switch = 0;
      process_head->pc_idle = YES;

   }

   /* restore the program counter */

   pc = cstack[cstack_top].cs_pc;

   /* decrement the use count and free if necessary */

   proc_ptr->p_active_use_count--;
   if (!(--(proc_ptr->p_use_count))) {
      free_procedure(SETL_SYSTEM proc_ptr);
   }

   pop_cstack;

   /* if we should return from C, do it */

   if (cstack[cstack_top + 1].cs_C_return==0)
      break;

   if (cstack[cstack_top + 1].cs_C_return==1)
      return 0; /* End the execution of the current program */

 
   if (cstack[cstack_top + 1].cs_C_return==EXTRA)  {

      ip = pc - 1;
      switch(cstack[cstack_top + 1].cs_extra_code) {
      
         case 1:
            target = ip->i_operand[0].i_spec_ptr;
            left = ip->i_operand[1].i_spec_ptr;

            /* if used in expression context, copy the result */

            if (target != NULL) {

               mark_specifier(left);
               unmark_specifier(target);
               target->sp_form = left->sp_form;
               target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            }

            break;

         case 2:
         
               target = ip->i_operand[0].i_spec_ptr;

               /* now we have the real base set */

               switch (spare1.sp_form) {

                  case ft_set :

                     start_set_iterator(SETL_SYSTEM target,&spare1);
                     unmark_specifier(&spare1);
                     spare1.sp_form = ft_omega;

                     break;

                  case ft_map :

                     start_map_iterator(SETL_SYSTEM target,&spare1);
                     unmark_specifier(&spare1);
                     spare1.sp_form = ft_omega;

                     break;

                  case ft_tuple :

                     start_tuple_iterator(SETL_SYSTEM target,&spare1);
                     unmark_specifier(&spare1);
                     spare1.sp_form = ft_omega;

                     break;

                  case ft_string :

                     start_string_iterator(SETL_SYSTEM target,&spare1);
                     unmark_specifier(&spare1);
                     spare1.sp_form = ft_omega;

                     break;

                  case ft_object :

                     start_object_iterator(SETL_SYSTEM target,&spare1);
                     unmark_specifier(&spare1);
                     spare1.sp_form = ft_omega;

                     break;

                  /*
                   *  Anything else is a run-time error.
                   */

                  default :

                     abend(SETL_SYSTEM "Can not iterate over source:\nSource => %s",
                           abend_opnd_str(SETL_SYSTEM &spare1));

                     break;

               }
	       break;

         case 3:
               /* set the condition based on the return value */

               if (spare1.sp_form != ft_atom)
                  abend(SETL_SYSTEM "Return value from < method must be true or false");

               if (spare1.sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num)
                  condition_true = YES;
               else if (spare1.sp_val.sp_atom_num ==
                        spec_false->sp_val.sp_atom_num)
                  condition_true = NO;
               else
                  abend(SETL_SYSTEM "Return value from < method must be true or false");

               spare1.sp_form = ft_omega;

               switch (ip->i_opcode) {

                  case p_gole :

                  if (condition_true)
                     pc = ip->i_operand[0].i_inst_ptr;

                  break;

               case p_gonle :

                  if (!condition_true)
                     pc = ip->i_operand[0].i_inst_ptr;

                  break;

               case p_le :

                  target = ip->i_operand[0].i_spec_ptr;

                  if (condition_true) {

                     unmark_specifier(target);
                     target->sp_form = spec_true->sp_form;
                     target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

                  }
                  else {

                     unmark_specifier(target);
                     target->sp_form = spec_false->sp_form;
                     target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

                  }

                  break;

               case p_nle :

                  target = ip->i_operand[0].i_spec_ptr;

                  if (!condition_true) {

                     unmark_specifier(target);
                     target->sp_form = spec_true->sp_form;
                     target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

                  }
                  else {

                     unmark_specifier(target);
                     target->sp_form = spec_false->sp_form;
                     target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

                  }

                  break;
               case p_golt :

                  if (condition_true)
                     pc = ip->i_operand[0].i_inst_ptr;

                  break;

               case p_gonlt :

                  if (!condition_true)
                     pc = ip->i_operand[0].i_inst_ptr;

                  break;

               case p_lt :

                  target = ip->i_operand[0].i_spec_ptr;

                  if (condition_true) {

                     unmark_specifier(target);
                     target->sp_form = spec_true->sp_form;
                     target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

                  }
                  else {

                     unmark_specifier(target);
                     target->sp_form = spec_false->sp_form;
                     target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

                  }

                  break;

               case p_nlt :

                  target = ip->i_operand[0].i_spec_ptr;

                  if (!condition_true) {

                     unmark_specifier(target);
                     target->sp_form = spec_true->sp_form;
                     target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

                  }
                  else {

                     unmark_specifier(target);
                     target->sp_form = spec_false->sp_form;
                     target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

                  }

                  break;


               }

               break;
         case 4:
               /* set the condition based on the return value */

               if (spare1.sp_form != ft_atom)
                  abend(SETL_SYSTEM "Return value from IN method must be true or false");
      
               if (spare1.sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num)
                  condition_true = YES;
               else if (spare1.sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num)
                  condition_true = NO;
               else
                  abend(SETL_SYSTEM "Return value from IN method must be true or false");

               spare1.sp_form = ft_omega;

               switch (ip->i_opcode) {

                  case p_goin :

                     if (condition_true)
                        pc = ip->i_operand[0].i_inst_ptr;

                     break;

                  case p_gonotin :

                     if (!condition_true)
                        pc = ip->i_operand[0].i_inst_ptr;

                     break;

                  case p_in :

                     target = ip->i_operand[0].i_spec_ptr;

                     if (condition_true) {

                        unmark_specifier(target);
                        target->sp_form = spec_true->sp_form;
                        target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

                     }
                     else {

                        unmark_specifier(target);
                        target->sp_form = spec_false->sp_form;
                        target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

                     }

                     break;

                  case p_notin :

                     target = ip->i_operand[0].i_spec_ptr;

                     if (!condition_true) {

                        unmark_specifier(target);
                        target->sp_form = spec_true->sp_form;
                        target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

                     }
                     else {

                        unmark_specifier(target);
                        target->sp_form = spec_false->sp_form;
                        target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

                     }

                     break;

               }

               break;

         default:

            break;
  
      }
   }

   break;

}

/*\
 *  \pcode{p\_penviron}{save procedure environment}
 *        {specifier}{copy of procedure}
 *        {specifier}{original procedure}
 *        {}{unused}
 *
 *  This instruction copies a procedure's environment, where for our
 *  purposes that is the local data of any enclosing procedures.
\*/

case p_penviron :

{

   /* pick out the operand pointers, to save some expression evaluations */

   left = ip->i_operand[1].i_spec_ptr;
   target = ip->i_operand[0].i_spec_ptr;

   /* we copy the procedure */

   proc_ptr = left->sp_val.sp_proc_ptr;
   get_proc(new_proc_ptr);
   memcpy((void *)new_proc_ptr,
          (void *)proc_ptr,
          sizeof(struct proc_item));
   new_proc_ptr->p_copy = NULL;
   new_proc_ptr->p_save_specs = NULL;
   new_proc_ptr->p_use_count = 1;
   new_proc_ptr->p_active_use_count = 0;
   new_proc_ptr->p_self_ptr = NULL;
   new_proc_ptr->p_is_const = NO;

   /* set the target */

   unmark_specifier(target);
   target->sp_form = ft_proc;
   target->sp_val.sp_proc_ptr = new_proc_ptr;

   /* copy enclosing procedures */

   for (proc_ptr = new_proc_ptr;
        proc_ptr->p_parent != NULL;
        proc_ptr = proc_ptr->p_parent) {

      /*
       *  This is an expensive-looking test to find an active procedure
       *  with the desired signature.  In fact, it isn't so expensive
       *  since it's really rare to have deeply nested procedures, and
       *  rarer still to embed procedures used as first-class objects!
       */

      for (i = cstack_top; i >= 0; i--) {

         for (new_proc_ptr = cstack[i].cs_proc_ptr;
              new_proc_ptr == NULL ||
                 new_proc_ptr->p_signature != proc_ptr->p_parent;
              new_proc_ptr = new_proc_ptr->p_parent);

         if (new_proc_ptr != NULL)
            break;

      }

#ifdef TRAPS

      if (i < 0)
         trap(__FILE__,__LINE__,"Missing procedure on call stack");

#endif

      if (new_proc_ptr->p_copy != NULL) {

         new_proc_ptr = new_proc_ptr->p_copy;
         proc_ptr->p_parent = new_proc_ptr;
         new_proc_ptr->p_use_count++;
         break;

      }

      /* copy the current procedure's parent */

      proc_ptr->p_parent = new_proc_ptr;
      get_proc(new_proc_ptr);
      memcpy((void *)new_proc_ptr,
             (void *)(proc_ptr->p_parent),
             sizeof(struct proc_item));
      (proc_ptr->p_parent)->p_copy = new_proc_ptr;
      proc_ptr->p_parent = new_proc_ptr;
      new_proc_ptr->p_use_count = 2;
      new_proc_ptr->p_active_use_count = 1;
      new_proc_ptr->p_is_const = NO;
      if (new_proc_ptr->p_self_ptr != NULL)
         (new_proc_ptr->p_self_ptr)->o_use_count++;

   }
 
   break;

}

/*\
 *  \pcode{p\_stop}{stop interpreter}
 *        {}{unused}
 *        {}{unused}
 *        {}{unused}
 *
 *  This instruction stops the interpreter and returns to the caller.  It
 *  is a normal termination.
\*/

case p_stop :

   /* decrement the call stack until we find a null program counter */

   while (cstack_top >= 0 && cstack[cstack_top].cs_pc != NULL) {
      pop_cstack;
   }
   if (cstack_top >= 0) {
      pop_cstack;
   }
   if (cstack_top < -1)
      cstack_top = -1;

   abend_initialized=0;
   return NO;

/*\
 *  \pcode{p\_stopall}{stop everything}
 *        {}{unused}
 *        {}{unused}
 *        {}{unused}
 *
 *  This instruction stops everything.  It is only generated by a
 *  \verb"stop" instruction.
\*/

case p_stopall :
   hard_stop=1;
   if (abend_initialized)
          longjmp(abend_env,1);
  
   abend_initialized=0;
   return NO; // 
   runtime_cleanup(SETL_SYSTEM_VOID);
   exit(SUCCESS_EXIT);

/*\
 *  \pcode{p\_assert}{assertions}
 *        {specifier}{boolean}
 *        {specifier}{procedure name}
 *        {integer}{line number}
 *
 *  This instruction is generated by assert statements.  The boolean
 *  should be either true or false, and the action we take depends on a
 *  command line switch.
\*/

case p_assert :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* if failing asserts should cause aborts ... */

   if (ASSERT_MODE == ASSERT_FAIL) {

      if (target->sp_form != ft_atom)
         abend(SETL_SYSTEM msg_bad_assert_arg,
               abend_opnd_str(SETL_SYSTEM target));

      if (target->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num)
         break;

      if (target->sp_val.sp_atom_num != spec_false->sp_val.sp_atom_num)
         abend(SETL_SYSTEM msg_bad_assert_arg,
               abend_opnd_str(SETL_SYSTEM target));

      /* at this point an assertion has failed */

      fputs("Assert failed in ",stderr);
      left_string_hdr = left->sp_val.sp_string_ptr;
      left_string_length = left_string_hdr->s_length;
      for (left_string_cell = left_string_hdr->s_head;
           left_string_cell != NULL;
           left_string_cell = left_string_cell->s_next) {

         for (left_char_ptr = left_string_cell->s_cell_value;
              left_string_length-- &&
                    left_char_ptr <
                       left_string_cell->s_cell_value + STR_CELL_WIDTH;
              fputc(*left_char_ptr++,stderr));

      }

      fprintf(stderr," line %ld\n",ip->i_operand[2].i_integer);

      exit(ASSERT_EXIT);

   }

   /* we log passes, but abort on failures */

   else if (ASSERT_MODE == ASSERT_LOG) {

      if (target->sp_form != ft_atom)
         abend(SETL_SYSTEM msg_bad_assert_arg,
               abend_opnd_str(SETL_SYSTEM target));

      if (target->sp_val.sp_atom_num != spec_false->sp_val.sp_atom_num &&
          target->sp_val.sp_atom_num != spec_true->sp_val.sp_atom_num)
         abend(SETL_SYSTEM msg_bad_assert_arg,
               abend_opnd_str(SETL_SYSTEM target));

      if (target->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num)
         fputs(msg_assert_passed,stderr);
      else
         fputs("Assert failed in ",stderr);

      left_string_hdr = left->sp_val.sp_string_ptr;
      left_string_length = left_string_hdr->s_length;
      for (left_string_cell = left_string_hdr->s_head;
           left_string_cell != NULL;
           left_string_cell = left_string_cell->s_next) {

         for (left_char_ptr = left_string_cell->s_cell_value;
              left_string_length-- &&
                    left_char_ptr <
                       left_string_cell->s_cell_value + STR_CELL_WIDTH;
              fputc(*left_char_ptr++,stderr));

      }

      fprintf(stderr," line %ld\n",ip->i_operand[2].i_integer);

   }

   break;

}

/*\
 *  \pcode{p\_intcheck}{integer check}
 *        {specifier}{integer}
 *        {specifier}{integer}
 *        {specifier}{integer}
 *
 *  This instruction is generated for arithmetic iterators.  We use it to
 *  verify that the bounds are integers.
\*/

case p_intcheck :

   for (i = 0;
        i < 3 && (left = ip->i_operand[i].i_spec_ptr) != NULL;
        i++) {

      if (left->sp_form != ft_short && left->sp_form != ft_long)
         abend(SETL_SYSTEM msg_expected_integer,
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

/*\
 *  \case{arithmetic opcodes}
 *
 *  This case handles arithmetic operations if we have broken up the
 *  interpreter.
\*/

#ifdef SHORT_FUNCS

case p_add :            case p_sub :

   arith_ops1(SETL_SYSTEM_VOID);

   break;

case p_mult :           case p_div :

   arith_ops2(SETL_SYSTEM_VOID);

   break;

case p_exp :            case p_mod :

   arith_ops3(SETL_SYSTEM_VOID);

   break;

case p_min :            case p_max :

   arith_ops4(SETL_SYSTEM_VOID);

   break;

#endif

/*\
 *  \case{set and tuple opcodes}
 *
 *  This case handles set and tuple operations if we have broken up the
 *  interpreter.
\*/

#ifdef SHORT_FUNCS

case p_with :

   set_ops1(SETL_SYSTEM_VOID);

   break;

case p_less :

   set_ops2(SETL_SYSTEM_VOID);

   break;

case p_lessf :          case p_from :           case p_fromb :
case p_frome :          case p_npow :           case p_ufrom :

   set_ops3(SETL_SYSTEM_VOID);

   break;

#endif

/*\
 *  \case{unary opcodes}
 *
 *  This case handles unary operations if we have broken up the
 *  interpreter.
\*/

#ifdef SHORT_FUNCS

case p_uminus :         case p_domain :         case p_range :
case p_pow :            case p_arb :            case p_nelt :
case p_not :

   unary_ops(SETL_SYSTEM_VOID);

   break;

#endif

/*\
 *  \case{extraction opcodes}
 *
 *  This case handles extraction operations if we have broken up the
 *  interpreter.
\*/

#ifdef SHORT_FUNCS

case p_tupof :          case p_of1 :     case p_kof1 :      

   extract_ops1(SETL_SYSTEM_VOID);

   break;

case p_of :             case p_ofa :     case p_kofa :

   extract_ops2(SETL_SYSTEM_VOID);

   break;

case p_slice :

   extract_ops3(SETL_SYSTEM_VOID);

   break;

case p_end :

   extract_ops4(SETL_SYSTEM_VOID);

   break;

#endif

/*\
 *  \case{assignment opcodes}
 *
 *  This case handles assignment operations if we have broken up the
 *  interpreter.
\*/

#ifdef SHORT_FUNCS

case p_assign :         case p_sof :        

   assign_ops1(SETL_SYSTEM_VOID);

   break;

case p_sofa :           

   assign_ops2(SETL_SYSTEM_VOID);

   break;

case p_sslice :

   assign_ops3(SETL_SYSTEM_VOID);

   break;

case p_send :

   assign_ops4(SETL_SYSTEM_VOID);

   break;

#endif

/*\
 *  \case{condition opcodes}
 *
 *  This case handles condition operations if we have broken up the
 *  interpreter.
\*/

#ifdef SHORT_FUNCS

case p_eq :             case p_ne :             case p_goeq :
case p_gone :

   cond_ops1(SETL_SYSTEM_VOID);

   break;

case p_lt :             case p_nlt :            case p_golt :
case p_gonlt :

   cond_ops2(SETL_SYSTEM_VOID);

   break;

case p_le :             case p_nle :            case p_gole :
case p_gonle :

   cond_ops3(SETL_SYSTEM_VOID);

   break;

case p_in :             case p_notin :          case p_goin :
case p_gonotin :

   cond_ops4(SETL_SYSTEM_VOID);

   break;

case p_incs :
case p_go :             case p_goind :          case p_gotrue :
case p_gofalse :
case p_goincs :         case p_gonincs :
case p_and :            case p_or :

   cond_ops5(SETL_SYSTEM_VOID);

   break;

#endif

/*\
 *  \case{iterator opcodes}
 *
 *  This case handles iterator operations if we have broken up the
 *  interpreter.
\*/

#ifdef SHORT_FUNCS

case p_iter :

   iter_ops1(SETL_SYSTEM_VOID);

   break;

case p_set :            case p_tuple :          case p_inext :

   iter_ops2(SETL_SYSTEM_VOID);

   break;

#endif

/*\
 *  \case{object opcodes}
 *
 *  This case handles object operations if we have broken up the
 *  interpreter.
\*/

#ifdef SHORT_FUNCS

case p_initobj :        case p_initend :        case p_slot :
case p_sslot :          case p_slotof :         case p_self :
case p_menviron :

   object_ops(SETL_SYSTEM_VOID);

   break;

#endif

/*\
 *  \case{system error}
 *
 *  If we ever get here, we screwed up in the compiler.  It means we
 *  encountered an invalid op code.
\*/

#ifdef SHORT_FUNCS

#ifdef TRAPS

default :

   giveup(SETL_SYSTEM "System error -- Invalid opcode");

#endif

   /* return to normal indentation */
      }

   /* Loop back */
   
   if (forever == YES)
      goto execute_start;
   else
      return CONTINUE;
}

#endif

#endif                                 /* ROOT                              */
#if !SHORT_TEXT | PARTA

/*\
 *  \function{arith\_ops1()}
 *
 *  This function handles arithmetic operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS


PRTYPE void arith_ops1(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_add}{addition, concatenation, and set union}
 *        {specifier}{target operand}
 *        {specifier}{left hand operand}
 *        {specifier}{right hand operand}
 *
 *  The plus sign is very heavily overloaded.  It might mean integer or
 *  real addition, set union, string concatenation, or tuple
 *  concatenation.  We try to do simple arithmetic operations in-line,
 *  but call other functions to handle long objects.
\*/

case p_add :

{
specifier spareplus;                       /* spare specifier                   */

 

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can add short integers to other short integers or to long
       *  integers.
       */

      case ft_short :

         /*
          *  The easy case is when both arguments are short.  We add the
          *  two short integers together, and if we overflow the maximum
          *  value of a short, convert the result to a long integer.
          */

         if (right->sp_form == ft_short) {

            short_value = left->sp_val.sp_short_value +
                          right->sp_val.sp_short_value;

            /* check whether the sum remains short */

            if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
                short_hi_bits == INT_HIGH_BITS) {

               unmark_specifier(target);
               target->sp_form = ft_short;
               target->sp_val.sp_short_value = short_value;

               break;

            }

            /* if we exceed the maximum short, convert to long */

            short_to_long(SETL_SYSTEM target,short_value);

            break;

         }

         /*
          *  We use a general function which adds integers to handle the
          *  case in which the right operand is long.
          */

         else if (right->sp_form == ft_long) {

            integer_add(SETL_SYSTEM target,left,right);

            break;

         } else if (right->sp_form == ft_real) {
         
         	real_number = left->sp_val.sp_short_value +
         	              (right->sp_val.sp_real_ptr)->r_value;
         	
         	goto real_add;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_add_r,
                              "+",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"+",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can add long integers to short integers or other long
       *  integers.
       */

      case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which adds any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            integer_add(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_add_r,
                              "+",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"+",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can only add real numbers to other real numbers.
       */

      case ft_real :
		 /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_add_r,
                              "+",0);

            break;

         }
       
         if (right->sp_form == ft_real) {

            real_number = (left->sp_val.sp_real_ptr)->r_value +
                          (right->sp_val.sp_real_ptr)->r_value;
real_add:
#if INFNAN

            if (NANorINF(real_number))
               abend(SETL_SYSTEM "Floating point error -- Not a number");

#endif

            /* allocate a real */

            unmark_specifier(target);
            i_get_real(real_ptr);
            target->sp_form = ft_real;
            target->sp_val.sp_real_ptr = real_ptr;
            real_ptr->r_use_count = 1;
            real_ptr->r_value = real_number;

            break;

         }
 		 if (right->sp_form == ft_short) {
				real_number = (left->sp_val.sp_real_ptr)->r_value+
						(double)(right->sp_val.sp_short_value);
				goto real_add;
		 }
		  if (right->sp_form == ft_long) {
				real_number = (left->sp_val.sp_real_ptr)->r_value+
						(double)(long_to_double(SETL_SYSTEM right));
				goto real_add;
		 }

         /* that's all we accept */

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"+",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         break;

      /*
       *  Strings can be concatenated with other strings.
       */

      case ft_string :
      
     	 /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

        if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_add_r,
                              "+",0);

            break;

         }


         if (right->sp_form == ft_omega) {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"+",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));
             
            break;

         }
         
         spareplus.sp_form = ft_omega;
         
         if (right->sp_form != ft_string) {
         		/* convert to string */
		
				setl2_str(SETL_SYSTEM 1L,right,&spareplus);
				right = &spareplus;

		 }

         if (right->sp_form == ft_string) {

            /* we would like to use the left operand destructively */

            if (target == left && target != right &&
                (target->sp_val.sp_string_ptr)->s_use_count == 1) {

               target_string_hdr = target->sp_val.sp_string_ptr;
               target->sp_form = ft_omega;

            }
            else {

               target_string_hdr = copy_string(SETL_SYSTEM left->sp_val.sp_string_ptr);

            }

            right_string_hdr = right->sp_val.sp_string_ptr;

            /* we just attach the right string to the target */

            target_string_cell = target_string_hdr->s_tail;
            right_string_cell = right_string_hdr->s_head;

            /* set up cell limit pointers */

            if (target_string_cell == NULL) {

               target_char_ptr = target_char_end = NULL;

            }
            else {

               target_char_ptr = target_string_cell->s_cell_value +
                              (target_string_hdr->s_length % STR_CELL_WIDTH);
               target_char_end = target_string_cell->s_cell_value +
                                 STR_CELL_WIDTH;
               if (target_char_ptr == target_string_cell->s_cell_value)
                  target_char_ptr = target_char_end;

            }

            if (right_string_cell == NULL) {

               right_char_ptr = right_char_end = NULL;

            }
            else {

               right_char_ptr = right_string_cell->s_cell_value;
               right_char_end = right_char_ptr + STR_CELL_WIDTH;

            }

            /* loop over the right string attaching it to the target */

            right_string_length = right_string_hdr->s_length;

            while (right_string_length--) {

               if (right_char_ptr == right_char_end) {

                  right_string_cell = right_string_cell->s_next;
                  right_char_ptr = right_string_cell->s_cell_value;
                  right_char_end = right_char_ptr + STR_CELL_WIDTH;

               }

               if (target_char_ptr == target_char_end) {

                  get_string_cell(target_string_cell);
                  if (target_string_hdr->s_tail != NULL)
                     (target_string_hdr->s_tail)->s_next = target_string_cell;
                  target_string_cell->s_prev = target_string_hdr->s_tail;
                  target_string_cell->s_next = NULL;
                  target_string_hdr->s_tail = target_string_cell;
                  if (target_string_hdr->s_head == NULL)
                     target_string_hdr->s_head = target_string_cell;
                  target_char_ptr = target_string_cell->s_cell_value;
                  target_char_end = target_char_ptr + STR_CELL_WIDTH;

               }

               *target_char_ptr++ = *right_char_ptr++;

            }

            /* note that we've killed the string's hash code */

            target_string_hdr->s_length += right_string_hdr->s_length;
            target_string_hdr->s_hash_code = -1;

            /* assign our result to the target */

            unmark_specifier(target);
            target->sp_form = ft_string;
            target->sp_val.sp_string_ptr = target_string_hdr;
            
            if (spareplus.sp_form!=ft_omega) /* implicit conversion done */
            
            	unmark_specifier(&spareplus);

            break;

         }

         /* Otherwise we have an error */

         binop_abend(SETL_SYSTEM msg_bad_binop_forms,"+",
              abend_opnd_str(SETL_SYSTEM left),
              abend_opnd_str(SETL_SYSTEM right));
          
        
         break;

      /*
       *  Sets can be unioned with other sets or with maps.
       */

      case ft_map :

         map_to_set(SETL_SYSTEM left,left);

      case ft_set :

         /* if the right operand is a map, convert it */

         if (right->sp_form == ft_map)
            map_to_set(SETL_SYSTEM right,right);

         if (right->sp_form == ft_set) {

            set_union(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_add_r,
                              "+",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"+",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  Tuples can be concatenated with other tuples.
       */

      case ft_tuple :

         if (right->sp_form == ft_tuple) {

            tuple_concat(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_add_r,
                              "+",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"+",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_add,
                           "+",0);

         break;

      case ft_atom :
         push_pstack(ip->i_operand[1].i_spec_ptr);
         push_pstack(ip->i_operand[2].i_spec_ptr);
         target = NULL;
         call_procedure(SETL_SYSTEM target,
                        spec_nprinta,
                        NULL,
                        2,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_add_r,
                              "+",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"+",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

/*\
 *  \pcode{p\_sub}{subtraction and set difference}
 *        {specifier}{target operand}
 *        {specifier}{left hand operand}
 *        {specifier}{right hand operand}
 *
 *  The minus sign is overloaded, but not quite as much as plus.  We can
 *  subtract integers and reals, or form the difference between two sets.
 *  It is undefined on strings or tuples.
\*/

case p_sub :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can subtract short integers with other short integers or
       *  long integers.
       */

      case ft_short :

         /*
          *  The easy case is when both arguments are short.  We subtract
          *  the two short integers, and if we overflow the maximum value
          *  of a short, convert the result to a long integer.
          */

         if (right->sp_form == ft_short) {

            short_value = left->sp_val.sp_short_value -
                          right->sp_val.sp_short_value;

            /* check whether the sum remains short */

            if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
                short_hi_bits == INT_HIGH_BITS) {

               unmark_specifier(target);
               target->sp_form = ft_short;
               target->sp_val.sp_short_value = short_value;

               break;

            }

            /* if we exceed the maximum short, convert to long */

            short_to_long(SETL_SYSTEM target,short_value);

            break;

         }

         /*
          *  We use a general function which subtracts integers to handle
          *  the case in which the right operand is long.
          */

         else if (right->sp_form == ft_long) {

            integer_subtract(SETL_SYSTEM target,left,right);

            break;

         } else if (right->sp_form == ft_real) {
         
         	real_number = left->sp_val.sp_short_value -
         	              (right->sp_val.sp_real_ptr)->r_value;
         	
         	goto real_sub;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_sub_r,
                              "-",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"-",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can subtract long integers and short integers or other long
       *  integers.
       */

      case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which subtracts any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            integer_subtract(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_sub_r,
                              "-",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"-",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can only subtract real numbers and other real numbers.
       */

      case ft_real :
      
		


         if (right->sp_form == ft_real) {

            real_number = (left->sp_val.sp_real_ptr)->r_value -
                          (right->sp_val.sp_real_ptr)->r_value;
                   
real_sub:               

#if INFNAN

            if (NANorINF(real_number))
               abend(SETL_SYSTEM "Floating point error -- Not a number");

#endif

            /* allocate a real */

            unmark_specifier(target);
            i_get_real(real_ptr);
            target->sp_form = ft_real;
            target->sp_val.sp_real_ptr = real_ptr;
            real_ptr->r_use_count = 1;
            real_ptr->r_value = real_number;

            break;

         }
		 else if (right->sp_form == ft_short) {
			real_number = (left->sp_val.sp_real_ptr)->r_value-
					(double)(right->sp_val.sp_short_value);
			goto real_sub;
		 } 
		 else if (right->sp_form == ft_long) {
			real_number = (left->sp_val.sp_real_ptr)->r_value-
					(double)(long_to_double(SETL_SYSTEM right));
			goto real_sub;
		 }
         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_sub_r,
                              "-",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"-",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  Sets can be differenced with other sets or with maps.
       */

      case ft_map :

         map_to_set(SETL_SYSTEM left,left);

      case ft_set :

         /* if the right operand is a map, convert it */

         if (right->sp_form == ft_map)
            map_to_set(SETL_SYSTEM right,right);

         /* make sure the right hand side is another set */

         if (right->sp_form == ft_set) {

            set_difference(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_sub_r,
                              "-",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"-",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_sub,
                           "-",0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_sub_r,
                              "-",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"-",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{arith\_ops2()}
 *
 *  This function handles arithmetic operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void arith_ops2(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_mult}{multiplication and set intersection}
 *        {specifier}{target operand}
 *        {specifier}{left hand operand}
 *        {specifier}{right hand operand}
 *
 *  A star is used for multiplication, set intersection, and multiple
 *  concatenation on tuples and strings.
\*/

case p_mult :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can multiply short integers with other short integers or
       *  long integers.
       */

      case ft_short :

         /*
          *  The easy case is when both arguments are short.  We multiply
          *  the two short integers, and if we overflow the maximum value
          *  of a short, convert the result to a long integer.
          */

         if (right->sp_form == ft_short) {

            short_value = left->sp_val.sp_short_value *
                          right->sp_val.sp_short_value;

            /* check whether the sum remains short */

            if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
                short_hi_bits == INT_HIGH_BITS) {

               unmark_specifier(target);
               target->sp_form = ft_short;
               target->sp_val.sp_short_value = short_value;

               break;

            }

            /* if we exceed the maximum short, convert to long */

            short_to_long(SETL_SYSTEM target,short_value);

            break;

         }

         /*
          *  We use a general function which multiplies integers to handle
          *  the case in which the right operand is long.
          */

         else if (right->sp_form == ft_long) {

            integer_multiply(SETL_SYSTEM target,left,right);

            break;

         }  else if (right->sp_form == ft_real) {
         
         	real_number = left->sp_val.sp_short_value *
         	              (right->sp_val.sp_real_ptr)->r_value;
         	
         	goto real_mul;

         }

         /*
          *  If the right operand is a string we make many copies of the
          *  string.
          */

         else if (right->sp_form == ft_string) {

            if (left->sp_val.sp_short_value < 0) {
               binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                     abend_opnd_str(SETL_SYSTEM left),
                     abend_opnd_str(SETL_SYSTEM right));
               break;
            }

            string_multiply(SETL_SYSTEM target,right,left->sp_val.sp_short_value);

            break;

         }

         /*
          *  If the right operand is a tuple we make many copies of the
          *  tuple.
          */

         else if (right->sp_form == ft_tuple) {

            if (left->sp_val.sp_short_value < 0) {
               binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                     abend_opnd_str(SETL_SYSTEM left),
                     abend_opnd_str(SETL_SYSTEM right));
               break;
            }

            tuple_multiply(SETL_SYSTEM target,right,left->sp_val.sp_short_value);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mult_r,
                              "*",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can multiply long integers and short integers or other long
       *  integers.
       */

      case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which multiplies any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            integer_multiply(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  If the right operand is a string we make many copies of the
          *  string.
          */

         else if (right->sp_form == ft_string) {

            if ((left->sp_val.sp_long_ptr)->i_is_negative < 0) {
               binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                     abend_opnd_str(SETL_SYSTEM left),
                     abend_opnd_str(SETL_SYSTEM right));
               break;
            }

            short_value = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);

            string_multiply(SETL_SYSTEM target,right,short_value);

            break;

         }

         /*
          *  If the right operand is a tuple we make many copies of the
          *  tuple.
          */

         else if (right->sp_form == ft_tuple) {

            if ((left->sp_val.sp_long_ptr)->i_is_negative < 0) {
               binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                     abend_opnd_str(SETL_SYSTEM left),
                     abend_opnd_str(SETL_SYSTEM right));
               break;
            }

            short_value = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);

            tuple_multiply(SETL_SYSTEM target,right,short_value);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mult_r,
                              "*",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can only multiply real numbers and other real numbers.
       */

      case ft_real :

         if (right->sp_form == ft_real) {

            real_number = (left->sp_val.sp_real_ptr)->r_value *
                          (right->sp_val.sp_real_ptr)->r_value;
                          
real_mul:

#if INFNAN

            if (NANorINF(real_number))
               abend(SETL_SYSTEM "Floating point error -- Not a number");

#endif

            /* allocate a real */

            unmark_specifier(target);
            i_get_real(real_ptr);
            target->sp_form = ft_real;
            target->sp_val.sp_real_ptr = real_ptr;
            real_ptr->r_use_count = 1;
            real_ptr->r_value = real_number;

            break;

         }
 		 else if (right->sp_form == ft_short) {
			real_number = (left->sp_val.sp_real_ptr)->r_value*
					(double)(right->sp_val.sp_short_value);
			goto real_mul;
		 } 
		 else if (right->sp_form == ft_long) {
			real_number = (left->sp_val.sp_real_ptr)->r_value*
					(double)(long_to_double(SETL_SYSTEM right));
			goto real_mul;
		 }
         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mult_r,
                              "*",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  String multiplication is multiple concatenation.
       */

      case ft_string :

         /*
          *  We have to convert the right operand to a C long.
          */

         if (right->sp_form == ft_short) {

            short_value = right->sp_val.sp_short_value;
            string_multiply(SETL_SYSTEM target,left,short_value);

            break;

         }
         else if (right->sp_form == ft_long) {

            short_value = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);
            string_multiply(SETL_SYSTEM target,left,short_value);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mult_r,
                              "*",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  Sets can be intersected with other sets or with maps.
       */

      case ft_map :

         map_to_set(SETL_SYSTEM left,left);

      case ft_set :

         /* if the right operand is a map, convert it */

         if (right->sp_form == ft_map)
            map_to_set(SETL_SYSTEM right,right);

         /* make sure the right hand side is another set */

         if (right->sp_form == ft_set) {

            set_intersection(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mult_r,
                              "*",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  Tuple multiplication is multiple concatenation.
       */

      case ft_tuple :

         /*
          *  We have to convert the right operand to a C long.
          */

         if (right->sp_form == ft_short) {

            tuple_multiply(SETL_SYSTEM target,left,right->sp_val.sp_short_value);

            break;

         }
         else if (right->sp_form == ft_long) {

            short_value = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);

            tuple_multiply(SETL_SYSTEM target,left,short_value);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mult_r,
                              "*",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_mult,
                           "*",0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mult_r,
                              "*",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"*",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

/*\
 *  \pcode{p\_div}{division}
 *        {specifier}{target operand}
 *        {specifier}{left hand operand}
 *        {specifier}{right hand operand}
 *
 *  The slash is only defined on integers and reals, where it is used for
 *  division.
\*/

case p_div :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can divide short integers with other short integers or
       *  long integers.
       */

      case ft_short :

         /*
          *  The easy case is when both arguments are short.  We divide
          *  the two short integers, and if we overflow the maximum value
          *  of a short, convert the result to a long integer.
          */

         if (right->sp_form == ft_short) {

            if (right->sp_val.sp_short_value == 0)
               abend(SETL_SYSTEM msg_zero_divide);

            short_value = left->sp_val.sp_short_value /
                          right->sp_val.sp_short_value;

            /* check whether the sum remains short */

            if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
                short_hi_bits == INT_HIGH_BITS) {

               unmark_specifier(target);
               target->sp_form = ft_short;
               target->sp_val.sp_short_value = short_value;

               break;

            }

            /* if we exceed the maximum short, convert to long */

            short_to_long(SETL_SYSTEM target,short_value);

            break;

         }

         /*
          *  We use a general function which divides integers to handle
          *  the case in which the right operand is long.
          */

         else if (right->sp_form == ft_long) {

            integer_divide(SETL_SYSTEM target,left,right);

            break;

         } else if (right->sp_form == ft_real) {
         
         	real_number = (double)(left->sp_val.sp_short_value) /
         	              (right->sp_val.sp_real_ptr)->r_value;
         	
         	goto real_div;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_div_r,
                              "/",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"/",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can divide long integers and short integers or other long
       *  integers.
       */

      case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which divides any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            integer_divide(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_div_r,
                              "/",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"/",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can only divide real numbers and other real numbers.
       */

      case ft_real :

         if (right->sp_form == ft_real) {

            real_number = (left->sp_val.sp_real_ptr)->r_value /
                          (right->sp_val.sp_real_ptr)->r_value;

real_div:

#if INFNAN

            if (NANorINF(real_number))
               abend(SETL_SYSTEM "Floating point error -- Not a number");

#endif

            /* allocate a real */

            unmark_specifier(target);
            i_get_real(real_ptr);
            target->sp_form = ft_real;
            target->sp_val.sp_real_ptr = real_ptr;
            real_ptr->r_use_count = 1;
            real_ptr->r_value = real_number;

            break;

         }
         else if (right->sp_form == ft_short) {
			real_number = (left->sp_val.sp_real_ptr)->r_value /
					(double)(right->sp_val.sp_short_value);
			goto real_div;
		 } 
		 else if (right->sp_form == ft_long) {
			real_number = (left->sp_val.sp_real_ptr)->r_value /
					(double)(long_to_double(SETL_SYSTEM right));
			goto real_div;
		 }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_div_r,
                              "/",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"/",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_div,
                           "/",0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_div_r,
                              "/",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"/",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{arith\_ops3()}
 *
 *  This function handles arithmetic operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void arith_ops3(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_exp}{exponentiation}
 *        {specifier}{target operand}
 *        {specifier}{left hand operand}
 *        {specifier}{right hand operand}
 *
 *  The double star operator is only defined on integers and reals, where
 *  it is used for exponentiation.
\*/

case p_exp :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can produce powers of short integers with other short
       *  integers or long integers.
       */

      case ft_short :           case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which finds powers of any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            integer_power(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_exp_r,
                              "**",0);

            break;

         } else if (right->sp_form == ft_real) {
			real_number = pow((double)(left->sp_val.sp_short_value),
							  (right->sp_val.sp_real_ptr)->r_value);
							  
			goto real_pow;
		 } 

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"**",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can only find powers of real numbers.
       */

      case ft_real :

         if (right->sp_form == ft_real) {

            real_number = pow((left->sp_val.sp_real_ptr)->r_value,
                              (right->sp_val.sp_real_ptr)->r_value);

real_pow:
#if INFNAN

            if (NANorINF(real_number))
               abend(SETL_SYSTEM "Floating point error -- Not a number");

#endif

            /* allocate a real */

            unmark_specifier(target);
            i_get_real(real_ptr);
            target->sp_form = ft_real;
            target->sp_val.sp_real_ptr = real_ptr;
            real_ptr->r_use_count = 1;
            real_ptr->r_value = real_number;

            break;

         }
         else if (right->sp_form == ft_short) {
			real_number = pow((left->sp_val.sp_real_ptr)->r_value,
					(double)(right->sp_val.sp_short_value));
			goto real_pow;
		 } 
		 else if (right->sp_form == ft_long) {
			real_number = pow((left->sp_val.sp_real_ptr)->r_value,
					(double)(long_to_double(SETL_SYSTEM right)));
			goto real_pow;
		 }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_exp_r,
                              "**",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"**",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_exp,
                           "**",0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_exp_r,
                              "**",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"**",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

/*\
 *  \pcode{p\_mod}{modulo (remainder)}
 *        {specifier}{target operand}
 *        {specifier}{left hand operand}
 *        {specifier}{right hand operand}
 *
 *  The mod operation is defined on integers and sets.  On integers it is
 *  the remainder operation, and on sets it is symmetric difference.
\*/

case p_mod :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can find remainders of short integers divided by short
       *  or long integers.
       */

      case ft_short :

         /*
          *  The easy case is when both arguments are short.  We find the
          *  remainder and return (we can not overflow).
          */

         if (right->sp_form == ft_short) {

            if (right->sp_val.sp_short_value == 0)
               abend(SETL_SYSTEM msg_zero_divide);

            short_value = labs(left->sp_val.sp_short_value) %
                          labs(right->sp_val.sp_short_value);

            /* make sure mod is positive */

            if (short_value != 0) {

               if (left->sp_val.sp_short_value < 0 &&
                   right->sp_val.sp_short_value > 0)
                  short_value = right->sp_val.sp_short_value - short_value;

               if (left->sp_val.sp_short_value >= 0 &&
                   right->sp_val.sp_short_value < 0)
                  short_value = -right->sp_val.sp_short_value - short_value;

            }

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_value;

            break;

         }

         /*
          *  We use a general function which divides integers to handle
          *  the case in which the right operand is long.
          */

         else if (right->sp_form == ft_long) {

            integer_mod(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mod_r,
                              "MOD",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MOD",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can find the remainder of a long integer divided by either a
       *  short or long integer.
       */

      case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which divides any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            integer_mod(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mod_r,
                              "MOD",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MOD",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can find the symmetric difference between sets.
       */

      case ft_map :

         map_to_set(SETL_SYSTEM left,left);

      case ft_set :

         /* if the right operand is a map, convert it */

         if (right->sp_form == ft_map)
            map_to_set(SETL_SYSTEM right,right);

         /* make sure the right hand side is another set */

         if (right->sp_form == ft_set) {

            set_symdiff(SETL_SYSTEM target,left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mod_r,
                              "MOD",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MOD",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_mod,
                           "MOD",0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_mod_r,
                              "MOD",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MOD",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

#endif                                 /* PARTA                             */
#if !SHORT_TEXT | PARTB

/*\
 *  \function{arith\_ops4()}
 *
 *  This function handles arithmetic operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void arith_ops4(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {
    
 

#endif

/*\
 *  \pcode{p\_min}{minimum}
 *        {instruction}{target operand}
 *        {specifier}{left operand}
 *        {specifier}{right operand}
 *
 *  This case handles the MIN operator.  The comparison is similar to the
 *  less than operation.
\*/

case p_min :

{
  double real1,real2;
   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can compare short integers with other short integers and long
       *  integers.
       */

      case ft_short :

         /*
          *  If both operands are short we just compare the values.
          */

         if (right->sp_form == ft_short) {

            if (right->sp_val.sp_short_value < left->sp_val.sp_short_value) {

               unmark_specifier(target);
               target->sp_form = right->sp_form;
               target->sp_val.sp_biggest = right->sp_val.sp_biggest;

            }
            else {

               unmark_specifier(target);
               target->sp_form = left->sp_form;
               target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            }

            break;

         }

         /*
          *  We use a general function which compares integers to handle
          *  the case in which the second operand is long.
          */

         else if (right->sp_form == ft_long) {

            if (integer_lt(SETL_SYSTEM right,left)) {

               mark_specifier(right);
               unmark_specifier(target);
               target->sp_form = right->sp_form;
               target->sp_val.sp_biggest = right->sp_val.sp_biggest;

            }
            else {

               unmark_specifier(target);
               target->sp_form = left->sp_form;
               target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            }

            break;

         } else if (right->sp_form == ft_real) {
			real2 = (double)(left->sp_val.sp_short_value);
			real1 = (right->sp_val.sp_real_ptr)->r_value;	
			
			goto real_min2;
		 } 

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_min_r,
                              "MIN",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MIN",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare integers with short integers and other
       *  long integers.
       */

      case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which adds any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            if (integer_lt(SETL_SYSTEM right,left)) {

               mark_specifier(right);
               unmark_specifier(target);
               target->sp_form = right->sp_form;
               target->sp_val.sp_biggest = right->sp_val.sp_biggest;

            }
            else {

               mark_specifier(left);
               unmark_specifier(target);
               target->sp_form = left->sp_form;
               target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            }

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_min_r,
                              "MIN",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MIN",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare reals with other reals.
       */

      case ft_real :

         if (right->sp_form == ft_real) {

			real1 = (right->sp_val.sp_real_ptr)->r_value;	
			
real_min:	
			real2 = (left->sp_val.sp_real_ptr)->r_value;	
real_min2:
            if (real1<real2) {
            
               mark_specifier(right);
               unmark_specifier(target);
               target->sp_form = right->sp_form;
               target->sp_val.sp_biggest = right->sp_val.sp_biggest;

            }
            else {

               mark_specifier(left);
               unmark_specifier(target);
               target->sp_form = left->sp_form;
               target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            }
         }
		 else if (right->sp_form == ft_short) {
			real1 = (double)(right->sp_val.sp_short_value);
			goto real_min;
		 } 
		 else if (right->sp_form == ft_long) {
			real1 = (double)(long_to_double(SETL_SYSTEM right));
			goto real_min;
		 }
         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_min_r,
                              "MIN",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MIN",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare strings with other strings.
       */

      case ft_string :

         /* make sure the right operand is a string */

         if (right->sp_form == ft_string) {

            /* pick out the string headers */

            left_string_hdr = left->sp_val.sp_string_ptr;
            right_string_hdr = right->sp_val.sp_string_ptr;

            /* we just traverse the two strings until we find a mismatch */

            left_string_cell = left_string_hdr->s_head;
            right_string_cell = right_string_hdr->s_head;

            /* set up cell limit pointers */

            if (right_string_cell == NULL) {

               right_char_ptr = right_char_end = NULL;

            }
            else {

               right_char_ptr = right_string_cell->s_cell_value;
               right_char_end = right_char_ptr + STR_CELL_WIDTH;

            }

            if (left_string_cell == NULL) {

               left_char_ptr = left_char_end = NULL;

            }
            else {

               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            /* loop over the strings in parallel */

            target_string_length = min(left_string_hdr->s_length,
                                       right_string_hdr->s_length);

            while (target_string_length--) {

               if (left_char_ptr == left_char_end) {

                  left_string_cell = left_string_cell->s_next;
                  left_char_ptr = left_string_cell->s_cell_value;
                  left_char_end = left_char_ptr + STR_CELL_WIDTH;

               }

               if (right_char_ptr == right_char_end) {

                  right_string_cell = right_string_cell->s_next;
                  right_char_ptr = right_string_cell->s_cell_value;
                  right_char_end = right_char_ptr + STR_CELL_WIDTH;

               }

               /* if we find a mismatch, break */

               if (*left_char_ptr != *right_char_ptr)
                  break;

               left_char_ptr++;
               right_char_ptr++;

            }

            /* if we found a mismatch, compare characters */

            if (target_string_length >= 0) {

               if (*right_char_ptr < *left_char_ptr) {

                  mark_specifier(right);
                  unmark_specifier(target);
                  target->sp_form = right->sp_form;
                  target->sp_val.sp_biggest = right->sp_val.sp_biggest;

               }
               else {

                  mark_specifier(left);
                  unmark_specifier(target);
                  target->sp_form = left->sp_form;
                  target->sp_val.sp_biggest = left->sp_val.sp_biggest;

               }
            }

            /* otherwise the shorter string is less */

            else {

               if (right_string_hdr->s_length < left_string_hdr->s_length) {

                  mark_specifier(right);
                  unmark_specifier(target);
                  target->sp_form = right->sp_form;
                  target->sp_val.sp_biggest = right->sp_val.sp_biggest;

               }
               else {

                  mark_specifier(left);
                  unmark_specifier(target);
                  target->sp_form = left->sp_form;
                  target->sp_val.sp_biggest = left->sp_val.sp_biggest;

               }
            }

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_min_r,
                              "MIN",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MIN",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_min,
                           "MIN",0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_min_r,
                              "MIN",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MIN",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

/*\
 *  \pcode{p\_max}{maximum}
 *        {instruction}{target operand}
 *        {specifier}{left operand}
 *        {specifier}{right operand}
 *
 *  This case handles the MAX operator.  The comparison is similar to the
 *  less than operation.
\*/

case p_max :

{
  double real1,real2;

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can compare short integers with other short integers and long
       *  integers.
       */

      case ft_short :

         /*
          *  If both operands are short we just compare the values.
          */

         if (right->sp_form == ft_short) {

            if (left->sp_val.sp_short_value < right->sp_val.sp_short_value) {

               unmark_specifier(target);
               target->sp_form = right->sp_form;
               target->sp_val.sp_biggest = right->sp_val.sp_biggest;

            }
            else {

               unmark_specifier(target);
               target->sp_form = left->sp_form;
               target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            }

            break;

         }

         /*
          *  We use a general function which compares integers to handle
          *  the case in which the second operand is long.
          */

         else if (right->sp_form == ft_long) {

            if (integer_lt(SETL_SYSTEM left,right)) {

               mark_specifier(right);
               unmark_specifier(target);
               target->sp_form = right->sp_form;
               target->sp_val.sp_biggest = right->sp_val.sp_biggest;

            }
            else {

               unmark_specifier(target);
               target->sp_form = left->sp_form;
               target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            }

            break;

         }  else if (right->sp_form == ft_real) {
			real2 = (double)(left->sp_val.sp_short_value);
			real1 = (right->sp_val.sp_real_ptr)->r_value;	
			
			goto real_max2;
		 } 
		 
         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_max_r,
                              "MAX",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MAX",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare integers with short integers and other
       *  long integers.
       */

      case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which adds any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            if (integer_lt(SETL_SYSTEM left,right)) {

               mark_specifier(right);
               unmark_specifier(target);
               target->sp_form = right->sp_form;
               target->sp_val.sp_biggest = right->sp_val.sp_biggest;

            }
            else {

               mark_specifier(left);
               unmark_specifier(target);
               target->sp_form = left->sp_form;
               target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            }

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_max_r,
                              "MAX",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MAX",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare reals with other reals.
       */

      case ft_real :

         if (right->sp_form == ft_real) {


			real1 = (right->sp_val.sp_real_ptr)->r_value;			
			
real_max:	
			real2 = (left->sp_val.sp_real_ptr)->r_value;	
real_max2:
            if (real2<real1) {
            

               mark_specifier(right);
               unmark_specifier(target);
               target->sp_form = right->sp_form;
               target->sp_val.sp_biggest = right->sp_val.sp_biggest;

            }
            else {

               mark_specifier(left);
               unmark_specifier(target);
               target->sp_form = left->sp_form;
               target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            }
         }
         else if (right->sp_form == ft_short) {
			real1 = (double)(right->sp_val.sp_short_value);
			goto real_max;
		 } 
		 else if (right->sp_form == ft_long) {
			real1 = (double)(long_to_double(SETL_SYSTEM right));
			goto real_max;
		 }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_max_r,
                              "MAX",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MAX",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare strings with other strings.
       */

      case ft_string :

         /* make sure the right operand is a string */

         if (right->sp_form == ft_string) {

            /* pick out the string headers */

            left_string_hdr = left->sp_val.sp_string_ptr;
            right_string_hdr = right->sp_val.sp_string_ptr;

            /* we just traverse the two strings until we find a mismatch */

            left_string_cell = left_string_hdr->s_head;
            right_string_cell = right_string_hdr->s_head;

            /* set up cell limit pointers */

            if (right_string_cell == NULL) {

               right_char_ptr = right_char_end = NULL;

            }
            else {

               right_char_ptr = right_string_cell->s_cell_value;
               right_char_end = right_char_ptr + STR_CELL_WIDTH;

            }

            if (left_string_cell == NULL) {

               left_char_ptr = left_char_end = NULL;

            }
            else {

               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            /* loop over the strings in parallel */

            target_string_length = min(left_string_hdr->s_length,
                                       right_string_hdr->s_length);

            while (target_string_length--) {

               if (left_char_ptr == left_char_end) {

                  left_string_cell = left_string_cell->s_next;
                  left_char_ptr = left_string_cell->s_cell_value;
                  left_char_end = left_char_ptr + STR_CELL_WIDTH;

               }

               if (right_char_ptr == right_char_end) {

                  right_string_cell = right_string_cell->s_next;
                  right_char_ptr = right_string_cell->s_cell_value;
                  right_char_end = right_char_ptr + STR_CELL_WIDTH;

               }

               /* if we find a mismatch, break */

               if (*left_char_ptr != *right_char_ptr)
                  break;

               left_char_ptr++;
               right_char_ptr++;

            }

            /* if we found a mismatch, compare characters */

            if (target_string_length >= 0) {

               if (*left_char_ptr < *right_char_ptr) {

                  mark_specifier(right);
                  unmark_specifier(target);
                  target->sp_form = right->sp_form;
                  target->sp_val.sp_biggest = right->sp_val.sp_biggest;

               }
               else {

                  mark_specifier(left);
                  unmark_specifier(target);
                  target->sp_form = left->sp_form;
                  target->sp_val.sp_biggest = left->sp_val.sp_biggest;

               }
            }

            /* otherwise the shorter string is less */

            else {

               if (left_string_hdr->s_length < right_string_hdr->s_length) {

                  mark_specifier(right);
                  unmark_specifier(target);
                  target->sp_form = right->sp_form;
                  target->sp_val.sp_biggest = right->sp_val.sp_biggest;

               }
               else {

                  mark_specifier(left);
                  unmark_specifier(target);
                  target->sp_form = left->sp_form;
                  target->sp_val.sp_biggest = left->sp_val.sp_biggest;

               }
            }

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_max_r,
                              "MAX",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MAX",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_max,
                           "MAX",0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_max_r,
                              "MAX",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"MAX",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{set\_ops1()}
 *
 *  This function handles set and tuple operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void set_ops1(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_with}{set and tuple insertion}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The with operator inserts an element into a set or tuple.  If the
 *  target is a tuple, we append the element on to the tuple.
\*/

case p_with :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the first operand */

   switch (left->sp_form) {

      /*
       *  Maps can have items inserted.
       */

      case ft_map :

         /*
          *  If the value being inserted is not a tuple of length 2, fall
          *  through to the set routine.
          */

         if (right->sp_form == ft_tuple &&
             (right->sp_val.sp_tuple_ptr)->t_ntype.t_root.t_length == 2) {

            /* pick out the domain and range elements */

            tuple_root = right->sp_val.sp_tuple_ptr;

            /* we need the left-most child */

            source_height = tuple_root->t_ntype.t_root.t_height;
            while (source_height--) {

               tuple_root = tuple_root->t_child[0].t_header;

#ifdef TRAPS

               if (tuple_root == NULL)
                  giveup(SETL_SYSTEM msg_corrupted_tuple);

#endif

            }

            /* if the domain element is omega, convert to set */

			if ((tuple_root->t_child[0].t_cell)==NULL)
				domain_element=NULL;
			else
            domain_element = &((tuple_root->t_child[0].t_cell)->t_spec);
            
            
            if ((domain_element!=NULL)&&(domain_element->sp_form != ft_omega)) {
            
				domain_hash_code = (tuple_root->t_child[0].t_cell)->t_hash_code;
	            range_element = &((tuple_root->t_child[1].t_cell)->t_spec);
	            range_hash_code = (tuple_root->t_child[1].t_cell)->t_hash_code;

               /*
                *  Now we know we have an acceptable tuple, so we can
                *  perform the insertion as a map.  First we set up a
                *  target map.  We would like to use the left operand
                *  destructively if possible.
                */

               if (target == left && target != right &&
                   (target->sp_val.sp_map_ptr)->m_use_count == 1) {

                  map_root = target->sp_val.sp_map_ptr;
                  target->sp_form = ft_omega;

               }
               else {

                  map_root = copy_map(SETL_SYSTEM left->sp_val.sp_map_ptr);
				  target->sp_form = ft_omega;

               }

               /*
                *  We now have a valid pair, so we search for it in the
                *  map.
                */

               map_work_hdr = map_root;
               work_hash_code = domain_hash_code;
               target_height = map_root->m_ntype.m_root.m_height;
               while (target_height--) {

                  /* extract the element's index at this level */

                  target_index = work_hash_code & MAP_HASH_MASK;
                  work_hash_code >>= MAP_SHIFT_DIST;

                  /* if we're missing a header record, insert it */

                  if (map_work_hdr->m_child[target_index].m_header == NULL) {

                     get_map_header(new_map_hdr);
                     new_map_hdr->m_ntype.m_intern.m_parent = map_work_hdr;
                     new_map_hdr->m_ntype.m_intern.m_child_index =
                        target_index;
                     for (i = 0;
                          i < MAP_HASH_SIZE;
                          new_map_hdr->m_child[i++].m_cell = NULL);
                     map_work_hdr->m_child[target_index].m_header =
                        new_map_hdr;
                     map_work_hdr = new_map_hdr;

                  }
                  else {

                     map_work_hdr =
                        map_work_hdr->m_child[target_index].m_header;

                  }
               }

               /*
                *  At this point, map_work_hdr points to the lowest level
                *  header record.  The next problem is to determine if the
                *  domain element is already in the map.  We compare the
                *  element with the clash list.
                */

               target_index = work_hash_code & MAP_HASH_MASK;
               map_cell_tail = &(map_work_hdr->m_child[target_index].m_cell);
               for (map_cell = *map_cell_tail;
                    map_cell != NULL &&
                       map_cell->m_hash_code < domain_hash_code;
                    map_cell = map_cell->m_next) {

                  map_cell_tail = &(map_cell->m_next);

               }

               /* try to find the domain element */

               is_equal = NO;
               while (map_cell != NULL &&
                      map_cell->m_hash_code == domain_hash_code) {

                  spec_equal(is_equal,
                             &(map_cell->m_domain_spec),domain_element);

                  if (is_equal)
                     break;

                  map_cell_tail = &(map_cell->m_next);
                  map_cell = map_cell->m_next;

               }

               /* if we don't find the domain element, add a cell */

               if (!is_equal) {

                  get_map_cell(new_map_cell);
                  mark_specifier(domain_element);
                  mark_specifier(range_element);
                  new_map_cell->m_domain_spec.sp_form =
                     domain_element->sp_form;
                  new_map_cell->m_domain_spec.sp_val.sp_biggest =
                     domain_element->sp_val.sp_biggest;
                  new_map_cell->m_range_spec.sp_form =
                     range_element->sp_form;
                  new_map_cell->m_range_spec.sp_val.sp_biggest =
                     range_element->sp_val.sp_biggest;
                  new_map_cell->m_is_multi_val = NO;
                  new_map_cell->m_hash_code = domain_hash_code;
                  new_map_cell->m_next = *map_cell_tail;
                  *map_cell_tail = new_map_cell;
                  map_root->m_ntype.m_root.m_cardinality++;
                  map_root->m_ntype.m_root.m_cell_count++;

                  map_root->m_hash_code ^= domain_hash_code;
                  map_root->m_hash_code ^= range_hash_code;

                  unmark_specifier(target);
                  target->sp_form = ft_map;
                  target->sp_val.sp_map_ptr = map_root;

                  break;

               }

               /*
                *  Here we know that there is already a domain element for
                *  this pair.  We either have to convert the range
                *  element to a set, or insert our new range element into
                *  a set.
                */

               if (!(map_cell->m_is_multi_val)) {

                  /* if the range element is already in the range, break */

                  spec_equal(is_equal,
                             &(map_cell->m_range_spec),range_element);

                  if (is_equal) {

                     unmark_specifier(target);
                     target->sp_form = ft_map;
                     target->sp_val.sp_map_ptr = map_root;

                     break;

                  }

                  /* create a new singleton set for values */

                  get_set_header(set_root);
                  set_root->s_use_count = 1;
                  set_root->s_ntype.s_root.s_cardinality = 1;
                  set_root->s_ntype.s_root.s_height = 0;
                  for (i = 0;
                       i < SET_HASH_SIZE;
                       set_root->s_child[i++].s_cell = NULL);

                  spec_hash_code(source_hash_code,&map_cell->m_range_spec);
                  set_root->s_hash_code = source_hash_code;
                  get_set_cell(new_set_cell);
                  new_set_cell->s_spec.sp_form =
                     map_cell->m_range_spec.sp_form;
                  new_set_cell->s_spec.sp_val.sp_biggest =
                        map_cell->m_range_spec.sp_val.sp_biggest;
                  new_set_cell->s_hash_code = source_hash_code;
                  new_set_cell->s_next = NULL;
                  set_root->s_child[source_hash_code &
                                      SET_HASH_MASK].s_cell = new_set_cell;

                  map_cell->m_is_multi_val = YES;
                  map_cell->m_range_spec.sp_form = ft_omega;

               }
               else {

                  /*
                   *  We have to modify the value set.  First we copy it
                   *  so we can use it destructively.
                   */

                  set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
                  if (set_root->s_use_count == 1) 
                     map_cell->m_range_spec.sp_form = ft_omega;
                  else
                     set_root = copy_set(SETL_SYSTEM set_root);

               }

               /*
                *  Now we have a value set, and we must insert our range
                *  element into that set.
                */

               set_work_hdr = set_root;

               /* get the element's hash code */

               work_hash_code = source_hash_code = range_hash_code;

               /* descend the header tree until we get to a leaf */

               source_height = set_root->s_ntype.s_root.s_height;
               while (source_height--) {

                  /* extract the element's index at this level */

                  source_index = work_hash_code & SET_HASH_MASK;
                  work_hash_code >>= SET_SHIFT_DIST;

                  /* if we're missing a header record, insert it */

                  if (set_work_hdr->s_child[source_index].s_header ==
                      NULL) {

                     get_set_header(new_set_hdr);
                     new_set_hdr->s_ntype.s_intern.s_parent =
                        set_work_hdr;
                     new_set_hdr->s_ntype.s_intern.s_child_index =
                        source_index;
                     for (i = 0;
                          i < SET_HASH_SIZE;
                          new_set_hdr->s_child[i++].s_cell = NULL);
                     set_work_hdr->s_child[source_index].s_header =
                        new_set_hdr;
                     set_work_hdr = new_set_hdr;

                  }
                  else {

                     set_work_hdr =
                        set_work_hdr->s_child[source_index].s_header;

                  }
               }

               /*
                *  At this point, set_work_hdr points to the lowest
                *  level header record.  The next problem is to determine
                *  if the element is already in the set.  We compare the
                *  element with the clash list.
                */

               source_index = work_hash_code & SET_HASH_MASK;
               set_cell_tail = &(set_work_hdr->s_child[source_index].s_cell);
               for (set_cell = *set_cell_tail;
                    set_cell != NULL &&
                       set_cell->s_hash_code < source_hash_code;
                    set_cell = set_cell->s_next) {

                  set_cell_tail = &(set_cell->s_next);

               }

               /* check for a duplicate element */

               is_equal = NO;
               while (set_cell != NULL &&
                      set_cell->s_hash_code == source_hash_code) {

                  spec_equal(is_equal,&(set_cell->s_spec),range_element);

                  if (is_equal)
                     break;

                  set_cell_tail = &(set_cell->s_next);
                  set_cell = set_cell->s_next;

               }

               /* if we reach this point we didn't find the element */

               if (is_equal) {

                  unmark_specifier(&(map_cell->m_range_spec));
                  map_cell->m_range_spec.sp_form = ft_set;
                  map_cell->m_range_spec.sp_val.sp_set_ptr = set_root;

                  unmark_specifier(target);
                  target->sp_form = ft_map;
                  target->sp_val.sp_map_ptr = map_root;

                  break;

               }

               map_root->m_hash_code ^= domain_hash_code;
               map_root->m_hash_code ^= range_hash_code;

               get_set_cell(new_set_cell);
               mark_specifier(range_element);
               new_set_cell->s_spec.sp_form = range_element->sp_form;
               new_set_cell->s_spec.sp_val.sp_biggest =
                  range_element->sp_val.sp_biggest;
               new_set_cell->s_hash_code = source_hash_code;
               new_set_cell->s_next = *set_cell_tail;
               *set_cell_tail = new_set_cell;
               set_root->s_ntype.s_root.s_cardinality++;
               map_root->m_ntype.m_root.m_cardinality++;
               set_root->s_hash_code ^= source_hash_code;

               /* we may have to expand the header, so find the trigger */

               expansion_trigger =
                     (1 << ((set_root->s_ntype.s_root.s_height + 1)
                             * SET_SHIFT_DIST)) * SET_CLASH_SIZE;

               /* expand the set header if necessary */

               if (set_root->s_ntype.s_root.s_cardinality >
                   expansion_trigger) {

                  set_root = set_expand_header(SETL_SYSTEM set_root);
				  // What is this line doing here??
                  //map_cell->m_range_spec.sp_val.sp_set_ptr = set_root;

               }

               /* expand the map header if necessary */

               expansion_trigger =
                     (1 << ((map_root->m_ntype.m_root.m_height + 1)
                            * MAP_SHIFT_DIST)) * 2;
               if (map_root->m_ntype.m_root.m_cardinality >
                   expansion_trigger) {

                  map_root = map_expand_header(SETL_SYSTEM map_root);
				  // What is this line doing here??
                  //target->sp_val.sp_map_ptr = map_root;

               }

               /* finally, set the target and return */

               unmark_specifier(&(map_cell->m_range_spec));
               map_cell->m_range_spec.sp_form = ft_set;
               map_cell->m_range_spec.sp_val.sp_set_ptr = set_root;

               unmark_specifier(target);
               target->sp_form = ft_map;
               target->sp_val.sp_map_ptr = map_root;

               break;

            }
         }

         map_to_set(SETL_SYSTEM left,left);

      /*
       *  Sets can have items inserted.
       */

      case ft_set :

         /* we never insert omegas */

         if (right->sp_form == ft_omega) {

            mark_specifier(left);
            unmark_specifier(target);
            target->sp_form = left->sp_form;
            target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            break;

         }

         /*
          *  First we set up a target set.  We would like to use the left
          *  operand destructively if possible.
          */
                  
         if (target == left && target != right &&
             (target->sp_val.sp_set_ptr)->s_use_count == 1) {

            set_root = target->sp_val.sp_set_ptr;
            target->sp_form = ft_omega;

         }
         else {

            set_root = copy_set(SETL_SYSTEM left->sp_val.sp_set_ptr);

         }

         set_work_hdr = set_root;

         /* get the element's hash code */

         spec_hash_code(work_hash_code,right);
         source_hash_code = work_hash_code;

         /* descend the header tree until we get to a leaf */

         target_height = set_root->s_ntype.s_root.s_height;
         while (target_height--) {

            /* extract the element's index at this level */

            target_index = work_hash_code & SET_HASH_MASK;
            work_hash_code = work_hash_code >> SET_SHIFT_DIST;

            /* if we're missing a header record, insert it */

            if (set_work_hdr->s_child[target_index].s_header == NULL) {

               get_set_header(new_set_hdr);
               new_set_hdr->s_ntype.s_intern.s_parent = set_work_hdr;
               new_set_hdr->s_ntype.s_intern.s_child_index = target_index;
               for (i = 0;
                    i < SET_HASH_SIZE;
                    new_set_hdr->s_child[i++].s_cell = NULL);
               set_work_hdr->s_child[target_index].s_header = new_set_hdr;
               set_work_hdr = new_set_hdr;

            }
            else {

               set_work_hdr =
                  set_work_hdr->s_child[target_index].s_header;

            }
         }

         /*
          *  At this point, set_work_hdr points to the lowest level header
          *  record.  The next problem is to determine if the element is
          *  already in the set.  We compare the element with the clash
          *  list.
          */

         target_index = work_hash_code & SET_HASH_MASK;
         set_cell_tail = &(set_work_hdr->s_child[target_index].s_cell);
         for (set_cell = *set_cell_tail;
              set_cell != NULL &&
                 set_cell->s_hash_code < source_hash_code;
              set_cell = set_cell->s_next) {

            set_cell_tail = &(set_cell->s_next);

         }

         /* check for a duplicate element */

         is_equal = NO;
         while (set_cell != NULL &&
                set_cell->s_hash_code == source_hash_code) {

            spec_equal(is_equal,&(set_cell->s_spec),right);

            if (is_equal)
               break;

            set_cell_tail = &(set_cell->s_next);
            set_cell = set_cell->s_next;

         }

         /* if we reach this point we didn't find the element */

         if (!is_equal) {

            get_set_cell(new_set_cell);
            mark_specifier(right);
            new_set_cell->s_spec.sp_form = right->sp_form;
            new_set_cell->s_spec.sp_val.sp_biggest =
               right->sp_val.sp_biggest;
            new_set_cell->s_hash_code = source_hash_code;
            new_set_cell->s_next = *set_cell_tail;
            *set_cell_tail = new_set_cell;
            set_root->s_ntype.s_root.s_cardinality++;
            set_root->s_hash_code ^= source_hash_code;

            /* we may have to expand the size of the header */

            expansion_trigger =
                 (1 << ((set_root->s_ntype.s_root.s_height + 1)
                  * SET_SHIFT_DIST)) * SET_CLASH_SIZE;

            /* expand the set header if necessary */

            if (set_root->s_ntype.s_root.s_cardinality > expansion_trigger) {

               set_root = set_expand_header(SETL_SYSTEM set_root);

            }
         }

         /* finally, set the target value */

         unmark_specifier(target);
         target->sp_form = ft_set;
         target->sp_val.sp_set_ptr = set_root;

         break;

      /*
       *  Tuples can have items appended.
       */

      case ft_tuple :

         /* we don't append omega's */

         if (right->sp_form == ft_omega) {

            mark_specifier(left);
            unmark_specifier(target);
            target->sp_form = left->sp_form;
            target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            break;

         }

         /* we would like to use the left operand destructively */

         if (target == left && target != right &&
             (target->sp_val.sp_tuple_ptr)->t_use_count == 1) {

            tuple_root = target->sp_val.sp_tuple_ptr;
            target->sp_form = ft_omega;

         }
         else {

            tuple_root = copy_tuple(SETL_SYSTEM left->sp_val.sp_tuple_ptr);

         }

         /* we insert at the end of the tuple */

         short_value = tuple_root->t_ntype.t_root.t_length;

         /* expand the target tree if necessary */

         expansion_trigger = 1 << ((tuple_root->t_ntype.t_root.t_height + 1)
                                    * TUP_SHIFT_DIST);

         if (short_value >= expansion_trigger) {

            tuple_work_hdr = tuple_root;

            get_tuple_header(tuple_root);

            tuple_root->t_use_count = 1;
            tuple_root->t_hash_code =
               tuple_work_hdr->t_hash_code;
            tuple_root->t_ntype.t_root.t_length =
               tuple_work_hdr->t_ntype.t_root.t_length;
            tuple_root->t_ntype.t_root.t_height =
               tuple_work_hdr->t_ntype.t_root.t_height + 1;

            for (i = 1;
                 i < TUP_HEADER_SIZE;
                 tuple_root->t_child[i++].t_header = NULL);

            tuple_root->t_child[0].t_header = tuple_work_hdr;

            tuple_work_hdr->t_ntype.t_intern.t_parent = tuple_root;
            tuple_work_hdr->t_ntype.t_intern.t_child_index = 0;

         }

         tuple_root->t_ntype.t_root.t_length = short_value + 1;

         /* look up the tuple component */

         tuple_work_hdr = tuple_root;

         for (source_height = tuple_root->t_ntype.t_root.t_height;
              source_height;
              source_height--) {

            /* extract the element's index at this level */

            source_index = (short_value >>
                                 (source_height * TUP_SHIFT_DIST)) &
                              TUP_SHIFT_MASK;

            /* if we're missing a header record, allocate one */

            if (tuple_work_hdr->t_child[source_index].t_header == NULL) {

               get_tuple_header(new_tuple_hdr);
               new_tuple_hdr->t_ntype.t_intern.t_parent = tuple_work_hdr;
               new_tuple_hdr->t_ntype.t_intern.t_child_index = source_index;
               for (i = 0;
                    i < TUP_HEADER_SIZE;
                    new_tuple_hdr->t_child[i++].t_cell = NULL);
               tuple_work_hdr->t_child[source_index].t_header =
                     new_tuple_hdr;
               tuple_work_hdr = new_tuple_hdr;

            }
            else {

               tuple_work_hdr =
                  tuple_work_hdr->t_child[source_index].t_header;

            }
         }

         /*
          *  At this point, tuple_work_hdr points to the lowest level header
          *  record.  We insert the new element.
          */

         source_index = short_value & TUP_SHIFT_MASK;

         get_tuple_cell(tuple_cell);
         tuple_work_hdr->t_child[source_index].t_cell = tuple_cell;
         mark_specifier(right);
         tuple_cell->t_spec.sp_form = right->sp_form;
         tuple_cell->t_spec.sp_val.sp_biggest = right->sp_val.sp_biggest;
         spec_hash_code(work_hash_code,right);
         tuple_root->t_hash_code ^= work_hash_code;
         tuple_cell->t_hash_code = work_hash_code;

         /* finally, assign our result to the target */

         unmark_specifier(target);
         target->sp_form = ft_tuple;
         target->sp_val.sp_tuple_ptr = tuple_root;

         break;

      /*
       *  With is one of the very few operations we allow on mailboxes.
       *  We append a value on the list.
       */

      case ft_mailbox :

         mailbox_ptr = left->sp_val.sp_mailbox_ptr;

         get_mailbox_cell(mailbox_cell);
         *(mailbox_ptr->mb_tail) = mailbox_cell;
         mailbox_ptr->mb_tail = &(mailbox_cell->mb_next);
         mailbox_cell->mb_next = NULL;

         mailbox_ptr->mb_cell_count++;

         mark_specifier(right);
         mailbox_cell->mb_spec.sp_form = right->sp_form;
         mailbox_cell->mb_spec.sp_val.sp_biggest = right->sp_val.sp_biggest;
 
         mailbox_ptr->mb_use_count++;
         unmark_specifier(target);
         target->sp_form = ft_mailbox;
         target->sp_val.sp_mailbox_ptr = mailbox_ptr;

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_with,
                           "WITH",0);

         break;

      case ft_atom :
         push_pstack(ip->i_operand[1].i_spec_ptr);
         push_pstack(ip->i_operand[2].i_spec_ptr);
         target = NULL;
         call_procedure(SETL_SYSTEM target,
                        spec_printa,
                        NULL,
                        2,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_with_r,
                              "WITH",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"WITH",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

#endif                                 /* PARTB                             */
#if !SHORT_TEXT | PARTC

/*\
 *  \function{set\_ops2()}
 *
 *  This function handles set and tuple operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void set_ops2(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_less}{remove an item from a set}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The less operator removes an element from a set.
\*/

case p_less :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /*
    *  Sets can have items deleted.
    */

   switch (left->sp_form) {

      case ft_set :

         /* we never delete omegas */

         if (right->sp_form == ft_omega) {

            mark_specifier(left);
            unmark_specifier(target);
            target->sp_form = left->sp_form;
            target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            break;

         }

         /*
          *  First we set up a target set.  We would like to use the left
          *  operand destructively if possible.
          */

         if (target == left && target != right &&
             (target->sp_val.sp_set_ptr)->s_use_count == 1) {

            set_root = target->sp_val.sp_set_ptr;
            target->sp_form = ft_omega;

         }
         else {

            set_root = copy_set(SETL_SYSTEM left->sp_val.sp_set_ptr);

         }

         set_work_hdr = set_root;

         /* get the element's hash code */

         spec_hash_code(work_hash_code,right);
         source_hash_code = work_hash_code;

         /* descend the header tree until we get to a leaf */

         target_height = set_root->s_ntype.s_root.s_height;
         while (set_work_hdr != NULL && target_height--) {

            /* extract the element's index at this level */

            target_index = work_hash_code & SET_HASH_MASK;
            work_hash_code = work_hash_code >> SET_SHIFT_DIST;

            set_work_hdr =
               set_work_hdr->s_child[target_index].s_header;

         }

         if (set_work_hdr == NULL) {

            unmark_specifier(target);
            target->sp_form = ft_set;
            target->sp_val.sp_set_ptr = set_root;

            break;

         }

         /*
          *  At this point, set_work_hdr points to the lowest level header
          *  record.  The next problem is to determine if the element is in
          *  the set.  We compare the element with the clash list.
          */

         target_index = work_hash_code & SET_HASH_MASK;
         set_cell_tail = &(set_work_hdr->s_child[target_index].s_cell);
         for (set_cell = *set_cell_tail;
              set_cell != NULL &&
                 set_cell->s_hash_code < source_hash_code;
              set_cell = set_cell->s_next) {

            set_cell_tail = &(set_cell->s_next);

         }

         /* search the list of elements with the same hash code */

         is_equal = NO;
         while (set_cell != NULL &&
                set_cell->s_hash_code == source_hash_code) {

            spec_equal(is_equal,&(set_cell->s_spec),right);

            if (is_equal)
               break;

            set_cell_tail = &(set_cell->s_next);
            set_cell = set_cell->s_next;

         }

         /* if we found the element, delete it */

         if (is_equal) {

            unmark_specifier(&(set_cell->s_spec));
            *set_cell_tail = set_cell->s_next;
            set_root->s_ntype.s_root.s_cardinality--;
            set_root->s_hash_code ^= source_hash_code;
            free_set_cell(set_cell);

            /* delete any null headers on this subtree */

            for (;;) {

               if (set_work_hdr == set_root)
                  break;

               for (i = 0;
                    i < SET_HASH_SIZE &&
                       set_work_hdr->s_child[i].s_header == NULL;
                    i++);

               if (i < SET_HASH_SIZE)
                  break;

               target_index =
                  set_work_hdr->s_ntype.s_intern.s_child_index + 1;
               set_work_hdr =
                  set_work_hdr->s_ntype.s_intern.s_parent;

            }

            /* we may have to reduce the height of the set */

            contraction_trigger = (1 << ((set_root->s_ntype.s_root.s_height)
                                          * SET_SHIFT_DIST));
            if (contraction_trigger == 1)
               contraction_trigger = 0;

            if (set_root->s_ntype.s_root.s_cardinality <
                contraction_trigger) {

               set_root = set_contract_header(SETL_SYSTEM set_root);

            }
         }

         /* finally, set the target value */

         unmark_specifier(target);
         target->sp_form = ft_set;
         target->sp_val.sp_set_ptr = set_root;

         break;

      /*
       *  Maps can have items deleted.
       */

      case ft_map :

         /*
          *  If the value being removed is not a tuple of length 2, it
          *  can't be in a map.
          */

         if (right->sp_form != ft_tuple) {

            mark_specifier(left);
            unmark_specifier(target);
            target->sp_form = left->sp_form;
            target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            break;

         }

         /* pick out the domain and range elements */

         tuple_root = right->sp_val.sp_tuple_ptr;
         if (tuple_root->t_ntype.t_root.t_length != 2) {

            mark_specifier(left);
            unmark_specifier(target);
            target->sp_form = left->sp_form;
            target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            break;

         }

         /* we need the left-most child */

         source_height = tuple_root->t_ntype.t_root.t_height;
         while (source_height--) {

            tuple_root = tuple_root->t_child[0].t_header;

#ifdef TRAPS

            if (tuple_root == NULL)
               giveup(SETL_SYSTEM msg_corrupted_tuple);

#endif

         }

         /* if the domain or range is omega, break */

         if (tuple_root->t_child[0].t_cell == NULL) {

            mark_specifier(left);
            unmark_specifier(target);
            target->sp_form = left->sp_form;
            target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            break;

         }

         domain_element = &((tuple_root->t_child[0].t_cell)->t_spec);
         domain_hash_code = (tuple_root->t_child[0].t_cell)->t_hash_code;
         range_element = &((tuple_root->t_child[1].t_cell)->t_spec);
         range_hash_code = (tuple_root->t_child[1].t_cell)->t_hash_code;

         if (domain_element->sp_form == ft_omega) {

            mark_specifier(left);
            unmark_specifier(target);
            target->sp_form = left->sp_form;
            target->sp_val.sp_biggest = left->sp_val.sp_biggest;

            break;

         }

         /*
          *  Now we know we have an acceptable tuple, so we can perform the
          *  deletion as a map.  First we set up a target map.  We would
          *  like to use the left operand destructively if possible.
          */

         if (target == left && target != right &&
             (target->sp_val.sp_map_ptr)->m_use_count == 1) {

            map_root = target->sp_val.sp_map_ptr;
            target->sp_form = ft_omega;

         }
         else {

            map_root = copy_map(SETL_SYSTEM left->sp_val.sp_map_ptr);

         }

         /*
          *  We now have a valid pair, so we search for it in the
          *  map.
          */

         map_work_hdr = map_root;
         work_hash_code = domain_hash_code;
         target_height = map_root->m_ntype.m_root.m_height;
         while (target_height-- && map_work_hdr != NULL) {

            /* extract the element's index at this level */

            target_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code >>= MAP_SHIFT_DIST;

            map_work_hdr =
               map_work_hdr->m_child[target_index].m_header;

         }

         if (map_work_hdr == NULL) {

            unmark_specifier(target);
            target->sp_form = ft_map;
            target->sp_val.sp_map_ptr = map_root;

            break;

         }

         /*
          *  At this point, map_work_hdr points to the lowest level header
          *  record.  The next problem is to determine if the domain element
          *  is in the map.  We compare the element with the clash list.
          */

         target_index = work_hash_code & MAP_HASH_MASK;
         map_cell_tail = &(map_work_hdr->m_child[target_index].m_cell);
         for (map_cell = *map_cell_tail;
              map_cell != NULL &&
                 map_cell->m_hash_code < domain_hash_code;
              map_cell = map_cell->m_next) {

            map_cell_tail = &(map_cell->m_next);

         }

         /* try to find the domain element */

         is_equal = NO;
         while (map_cell != NULL &&
                map_cell->m_hash_code == domain_hash_code) {

            spec_equal(is_equal,&(map_cell->m_domain_spec),domain_element);

            if (is_equal)
               break;

            map_cell_tail = &(map_cell->m_next);
            map_cell = map_cell->m_next;

         }

         if (!is_equal) {

            unmark_specifier(target);
            target->sp_form = ft_map;
            target->sp_val.sp_map_ptr = map_root;

            break;

         }

         /* if we don't have a multi-value cell, just delete the cell */

         if (!(map_cell->m_is_multi_val)) {

            /* if the range element is already in the range, delete */

            spec_equal(is_equal,&(map_cell->m_range_spec),range_element);

            if (is_equal) {

               map_root->m_ntype.m_root.m_cardinality--;
               map_root->m_ntype.m_root.m_cell_count--;
               map_root->m_hash_code ^= map_cell->m_hash_code;
               map_root->m_hash_code ^= range_hash_code;
               *map_cell_tail = map_cell->m_next;
               unmark_specifier(&(map_cell->m_domain_spec));
               unmark_specifier(&(map_cell->m_range_spec));
               free_map_cell(map_cell);

            }

            unmark_specifier(target);
            target->sp_form = ft_map;
            target->sp_val.sp_map_ptr = map_root;

            break;

         }

         /*
          *  We have to modify the value set.  First we copy it so we can
          *  use it destructively.
          */

         set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
         if (set_root->s_use_count == 1)
            map_cell->m_range_spec.sp_form = ft_omega;
         else
            set_root = copy_set(SETL_SYSTEM set_root);

         set_work_hdr = set_root;

         /* get the element's hash code */

         work_hash_code = range_hash_code;

         /* descend the header tree until we get to a leaf */

         source_height = set_root->s_ntype.s_root.s_height;
         while (source_height-- && set_work_hdr != NULL) {

            /* extract the element's index at this level */

            source_index = work_hash_code & SET_HASH_MASK;
            work_hash_code >>= SET_SHIFT_DIST;

            set_work_hdr =
               set_work_hdr->s_child[source_index].s_header;

         }

         if (set_work_hdr == NULL) {

            unmark_specifier(&(map_cell->m_range_spec));
            map_cell->m_range_spec.sp_form = ft_set;
            map_cell->m_range_spec.sp_val.sp_set_ptr = set_root;

            unmark_specifier(target);
            target->sp_form = ft_map;
            target->sp_val.sp_map_ptr = map_root;

            break;

         }

         /*
          *  At this point, set_work_hdr points to the lowest level header
          *  record.  The next problem is to determine if the element is in
          *  the set.  We compare the element with the clash list.
          */

         source_index = work_hash_code & SET_HASH_MASK;
         set_cell_tail = &(set_work_hdr->s_child[source_index].s_cell);
         for (set_cell = *set_cell_tail;
              set_cell != NULL &&
                 set_cell->s_hash_code < range_hash_code;
              set_cell = set_cell->s_next) {

            set_cell_tail = &(set_cell->s_next);

         }

         /* find the range element in the clash list */

         is_equal = NO;
         while (set_cell != NULL &&
                set_cell->s_hash_code == range_hash_code) {

            spec_equal(is_equal,&(set_cell->s_spec),range_element);

            if (is_equal)
               break;

            set_cell_tail = &(set_cell->s_next);
            set_cell = set_cell->s_next;

         }

         if (!is_equal) {

            unmark_specifier(&(map_cell->m_range_spec));
            map_cell->m_range_spec.sp_form = ft_set;
            map_cell->m_range_spec.sp_val.sp_set_ptr = set_root;

            unmark_specifier(target);
            target->sp_form = ft_map;
            target->sp_val.sp_map_ptr = map_root;

            break;

         }

         /*
          *  If the value set has more than two elements, we remove the
          *  element from the cell.
          */

         map_root->m_hash_code ^= range_hash_code;
         unmark_specifier(&(set_cell->s_spec));
         *set_cell_tail = set_cell->s_next;
         set_root->s_ntype.s_root.s_cardinality--;
         set_root->s_hash_code ^= range_hash_code;
         free_set_cell(set_cell);

         /* delete any null headers on this subtree */

         for (;;) {

            if (set_work_hdr == set_root)
               break;

            for (i = 0;
                 i < SET_HASH_SIZE &&
                    set_work_hdr->s_child[i].s_header == NULL;
                 i++);

            if (i < SET_HASH_SIZE)
               break;

            target_index =
               set_work_hdr->s_ntype.s_intern.s_child_index + 1;
            set_work_hdr =
               set_work_hdr->s_ntype.s_intern.s_parent;

         }

         /* we may have to reduce the height of the set */

         contraction_trigger = (1 << ((set_root->s_ntype.s_root.s_height)
                                       * SET_SHIFT_DIST));
         if (contraction_trigger == 1)
            contraction_trigger = 0;

         if (set_root->s_ntype.s_root.s_cardinality <
             contraction_trigger) {

            set_root = set_contract_header(SETL_SYSTEM set_root);

         }

         if (set_root->s_ntype.s_root.s_cardinality > 1) {

            unmark_specifier(&(map_cell->m_range_spec));
            map_cell->m_range_spec.sp_form = ft_set;
            map_cell->m_range_spec.sp_val.sp_set_ptr = set_root;

            unmark_specifier(target);
            target->sp_form = ft_map;
            target->sp_val.sp_map_ptr = map_root;

            break;

         }

         /*
          *  We have to convert the value set to a single element.
          */

         set_work_hdr = set_root;
         source_height = set_root->s_ntype.s_root.s_height;
         source_index = 0;
         set_cell = NULL;

         for (;;) {

            /* if we're at a leaf, look for the cell */

            if (!source_height) {

               for (source_index = 0;
                    source_index < SET_HASH_SIZE && set_cell == NULL;
                    source_index++) {

                  set_cell = set_work_hdr->s_child[source_index].s_cell;

               }

               if (set_cell != NULL)
                  break;

            }

            /* move up if we're at the end of a node */

            if (source_index >= SET_HASH_SIZE) {

               /* if we return to the root the set is exhausted */

#ifdef TRAPS

               if (set_work_hdr == set_root)
                  trap(__FILE__,__LINE__,msg_missing_set_element);

#endif

               source_height++;
               source_index =
                   set_work_hdr->s_ntype.s_intern.s_child_index + 1;
               set_work_hdr = set_work_hdr->s_ntype.s_intern.s_parent;

               continue;

            }

            /* skip over null nodes */

            if (set_work_hdr->s_child[source_index].s_header == NULL) {

               source_index++;
               continue;

            }

            /* otherwise drop down a level */

            set_work_hdr = set_work_hdr->s_child[source_index].s_header;
            source_index = 0;
            source_height--;

         }

         /* now we have the set element in set_cell */

         mark_specifier(&(set_cell->s_spec));
         map_cell->m_range_spec.sp_form = set_cell->s_spec.sp_form;
         map_cell->m_range_spec.sp_val.sp_biggest =
            set_cell->s_spec.sp_val.sp_biggest;
         map_cell->m_is_multi_val = NO;
         free_set(SETL_SYSTEM set_root);

         /* set the target */

         unmark_specifier(target);
         target->sp_form = ft_map;
         target->sp_val.sp_map_ptr = map_root;

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_less,
                           "LESS",0);

         break;

      /*
       *  That's all we know how to handle.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_less_r,
                              "LESS",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"LESS",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{set\_ops3()}
 *
 *  This function handles set and tuple operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void set_ops3(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_lessf}{remove an item from the domain of a map}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The lessf operator removes an element from the domain of a map, and
 *  all pairs corresponding to that element.
\*/

case p_lessf :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   switch (left->sp_form) {

      case ft_set :

         if (!set_to_map(SETL_SYSTEM left,left,NO))
            abend(SETL_SYSTEM msg_invalid_set_map,
                  abend_opnd_str(SETL_SYSTEM left));

      case ft_map :

         map_lessf(SETL_SYSTEM target,left,right);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_lessf,
                           "LESSF",0);

         break;

      /*
       *  That's all we know how to handle.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_lessf_r,
                              "LESSF",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"LESSF",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

/*\
 *  \pcode{p\_ufrom}{remove an arbitrary element from a set}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The from operator removes an arbitrary element from a set, assigning
 *  it to the target operand.
\*/

case p_ufrom : 

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = NULL;
   left = ip->i_operand[0].i_spec_ptr;
   right = ip->i_operand[1].i_spec_ptr;

   /* the action depends on the form of the right operand */

   switch (right->sp_form) {

      case ft_map :

         map_to_set(SETL_SYSTEM right,right);

      case ft_set :

         set_from(SETL_SYSTEM target,left,right);

         break;

      case ft_tuple :

         tuple_frome(SETL_SYSTEM target,left,right);

         break;

      case ft_string :

         string_frome(SETL_SYSTEM target,left,right);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :
         object_root = right->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_from;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"FROM",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM left,
                        slot_info->si_spec,
                        right,
                        0L,NO,YES,0);
         break;

      case ft_atom :
         push_pstack(ip->i_operand[1].i_spec_ptr);
         push_pstack(spec_omega);
         target = NULL;
         call_procedure(SETL_SYSTEM target,
                        spec_reada,
                        NULL,
                        2,NO,YES,0);
         target = ip->i_operand[0].i_spec_ptr;
         unmark_specifier(target);
         target->sp_form = pstack[pstack_top].sp_form;
         target->sp_val.sp_biggest = pstack[pstack_top].sp_val.sp_biggest;
         pstack_top--;

         break;

      /*
       *  That's all we know how to handle.
       */

      default :

         binop_abend(SETL_SYSTEM msg_bad_binop_forms,"FROM",
               abend_opnd_str(SETL_SYSTEM left),
               abend_opnd_str(SETL_SYSTEM right));

   }

   break;

}

/*\
 *  \pcode{p\_from}{remove an arbitrary element from a set}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The from operator removes an arbitrary element from a set, assigning
 *  it to the target operand.
\*/

case p_from : 

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action depends on the form of the right operand */

   switch (right->sp_form) {

      case ft_map :

         map_to_set(SETL_SYSTEM right,right);

      case ft_set :

         set_from(SETL_SYSTEM target,left,right);

         break;

      case ft_tuple :

         tuple_frome(SETL_SYSTEM target,left,right);

         break;

      case ft_string :

         string_frome(SETL_SYSTEM target,left,right);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = right->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_from;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"FROM",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM left,
                        slot_info->si_spec,
                        right,
                        0L,EXTRA,YES,1);
         break;

      /*
       *  That's all we know how to handle.
       */

      default :

         binop_abend(SETL_SYSTEM msg_bad_binop_forms,"FROM",
               abend_opnd_str(SETL_SYSTEM left),
               abend_opnd_str(SETL_SYSTEM right));

   }

   break;

}

/*\
 *  \pcode{p\_fromb}{remove the first element from a tuple}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The fromb operator removes the first element of a tuple, assigning it
 *  to the target operand.
\*/

case p_fromb :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action depends on the form of the right operand */

   switch (right->sp_form) {

      case ft_tuple :

         tuple_fromb(SETL_SYSTEM target,left,right);

         break;

      case ft_string :

         string_fromb(SETL_SYSTEM target,left,right);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = right->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_fromb;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"FROMB",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM left,
                        slot_info->si_spec,
                        right,
                        0L,EXTRA,YES,1);

         break;

      /*
       *  That's all we know how to handle.
       */

      default :

         binop_abend(SETL_SYSTEM msg_bad_binop_forms,"FROMB",
               abend_opnd_str(SETL_SYSTEM left),
               abend_opnd_str(SETL_SYSTEM right));

   }

   break;

}

/*\
 *  \pcode{p\_frome}{remove the last element from a tuple}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The frome operator removes the last element of a tuple, assigning it
 *  to the target operand.
\*/

case p_frome :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action depends on the form of the right operand */

   switch (right->sp_form) {

      case ft_tuple :

         tuple_frome(SETL_SYSTEM target,left,right);

         break;

      case ft_string :

         string_frome(SETL_SYSTEM target,left,right);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = right->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_frome;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"FROME",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM left,
                        slot_info->si_spec,
                        right,
                        0L,EXTRA,YES,1);

         break;

      /*
       *  That's all we know how to handle.
       */

      default :

         binop_abend(SETL_SYSTEM msg_bad_binop_forms,"FROME",
               abend_opnd_str(SETL_SYSTEM left),
               abend_opnd_str(SETL_SYSTEM right));

   }

   break;

}

/*\
 *  \pcode{p\_npow}{generate all subsets of fixed size}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The npow operator generates all subsets of a source set with a fixed
 *  cardinality.
\*/

case p_npow :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  If we have an integer on the left we look for a set on the
       *  right.
       */

      case ft_short :

         if (right->sp_form == ft_map)
            map_to_set(SETL_SYSTEM right,right);

         if (right->sp_form == ft_set) {

            if (left->sp_val.sp_short_value < 0)
               abend(SETL_SYSTEM msg_negative_npow,
                     abend_opnd_str(SETL_SYSTEM left));

            set_npow(SETL_SYSTEM target,right,left->sp_val.sp_short_value);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_npow_r,
                              "NPOW",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"NPOW",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      case ft_long :

         if (right->sp_form == ft_map)
            map_to_set(SETL_SYSTEM right,right);

         if (right->sp_form == ft_set) {

            short_value = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
            if (short_value < 0)
               abend(SETL_SYSTEM msg_negative_npow,
                     abend_opnd_str(SETL_SYSTEM left));

            set_npow(SETL_SYSTEM target,right,short_value);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_npow_r,
                              "NPOW",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"NPOW",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  If we have a set on the left, we look for an integer on the right.
       */

      case ft_map :

         map_to_set(SETL_SYSTEM left,left);

      case ft_set :

         /*
          *  We have to convert the right operand to a C long.
          */

         if (right->sp_form == ft_short) {

            if (right->sp_val.sp_short_value < 0)
               abend(SETL_SYSTEM msg_negative_npow,
                     abend_opnd_str(SETL_SYSTEM right));

            set_npow(SETL_SYSTEM target,left,right->sp_val.sp_short_value);

            break;

         }
         else if (right->sp_form == ft_long) {

            short_value = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);

            if (short_value < 0)
               abend(SETL_SYSTEM msg_negative_npow,
                     abend_opnd_str(SETL_SYSTEM right));

            set_npow(SETL_SYSTEM target,left,short_value);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_npow_r,
                              "NPOW",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"NPOW",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         call_binop_method(SETL_SYSTEM target,
                           left,
                           right,
                           m_npow,
                           "NPOW",0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            call_binop_method(SETL_SYSTEM target,
                              right,
                              left,
                              m_npow_r,
                              "NPOW",0);

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"NPOW",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{unary\_ops()}
 *
 *  This function handles unary operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void unary_ops(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_uminus}{unary minus operation}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  A unary minus is only defined on integers and reals.  We have nothing
 *  comparable to a set inverse, since there is no relevant universal
 *  set.
\*/

case p_uminus :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* the action we take depends on the form of the first operand */

   switch (left->sp_form) {

      /*
       *  If the source is short, we reverse the sign and make sure the
       *  result remains short.
       */

      case ft_short :

         short_value = -(left->sp_val.sp_short_value);

         /* check whether the sum remains short */

         if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
             short_hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_value;

            break;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_value);

         break;

      /*
       *  If the source is long, we reverse the sign and try to make it
       *  into a short.  This is necessary since the acceptable
       *  magnitudes of positive and negative shorts are not the same.
       */

      case ft_long :

         integer_hdr = left->sp_val.sp_long_ptr;

         /* try to convert to short */

         if (integer_hdr->i_cell_count < 3) {

            short_value = -long_to_short(SETL_SYSTEM integer_hdr);

            /* check whether it will fit in a short */

            if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
                short_hi_bits == INT_HIGH_BITS) {

               unmark_specifier(target);
               target->sp_form = ft_short;
               target->sp_val.sp_short_value = short_value;

               break;

            }
         }

         /* we just flip the sign of the long */

         if (target != left || integer_hdr->i_use_count != 1) {

            integer_hdr = copy_integer(SETL_SYSTEM integer_hdr);
            integer_hdr->i_is_negative = !(integer_hdr->i_is_negative);
            unmark_specifier(target);
            target->sp_form = ft_long;
            target->sp_val.sp_long_ptr = integer_hdr;

            break;

         }

         integer_hdr->i_is_negative = !(integer_hdr->i_is_negative);

         break;

      /*
       *  Reals: we copy the source to the target and reverse the sign.
       */

      case ft_real :

         real_number = -((left->sp_val.sp_real_ptr)->r_value);

#if INFNAN

         if (NANorINF(real_number))
            abend(SETL_SYSTEM "Floating point error -- Not a number");

#endif

         /* allocate a real */

         unmark_specifier(target);
         i_get_real(real_ptr);
         target->sp_form = ft_real;
         target->sp_val.sp_real_ptr = real_ptr;
         real_ptr->r_use_count = 1;
         real_ptr->r_value = real_number;

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_uminus;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"Unary minus",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        0L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         unop_abend(SETL_SYSTEM msg_bad_unop_form,"Unary minus",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

/*\
 *  \pcode{p\_domain}{domain of a map}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The domain operator finds the domain of a map.
\*/

case p_domain :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* the action we take depends on the form of the first operand */

   switch (left->sp_form) {

      case ft_set :

         if (!set_to_map(SETL_SYSTEM left,left,NO))
            abend(SETL_SYSTEM msg_invalid_set_map,
                  abend_opnd_str(SETL_SYSTEM left));

      case ft_map :

         map_domain(SETL_SYSTEM target,left);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_domain;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"DOMAIN",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        0L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         unop_abend(SETL_SYSTEM msg_bad_unop_form,"DOMAIN",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

/*\
 *  \pcode{p\_range}{range of a map}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The range operator finds the range of a map.
\*/

case p_range :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* the action we take depends on the form of the first operand */

   switch (left->sp_form) {

      case ft_set :

         if (!set_to_map(SETL_SYSTEM left,left,NO))
            abend(SETL_SYSTEM msg_invalid_set_map,
                  abend_opnd_str(SETL_SYSTEM left));

      case ft_map :

         map_range(SETL_SYSTEM target,left);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_range;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"RANGE",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        0L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         unop_abend(SETL_SYSTEM msg_bad_unop_form,"RANGE",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

/*\
 *  \pcode{p\_pow}{power set}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The pow operator finds the power set of a set.
\*/

case p_pow :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* the action we take depends on the form of the first operand */

   switch (left->sp_form) {

      case ft_map :

         map_to_set(SETL_SYSTEM left,left);

      case ft_set :

         set_pow(SETL_SYSTEM target,left);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_pow;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"POW",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        0L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         unop_abend(SETL_SYSTEM msg_bad_unop_form,"POW",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

/*\
 *  \pcode{p\_arb}{arbitrary element}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The arb operator finds an arbitrary element of a set.
\*/

case p_arb :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* the action we take depends on the form of the first operand */

   switch (left->sp_form) {

      case ft_map :

         map_to_set(SETL_SYSTEM left,left);

      case ft_set :

         set_arb(SETL_SYSTEM target,left);

         break;

      case ft_tuple :

         tuple_arb(SETL_SYSTEM target,left);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_arb;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"ARB",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        0L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         unop_abend(SETL_SYSTEM msg_bad_unop_form,"ARB",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

/*\
 *  \pcode{p\_nelt}{number of elements}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The sharp sign finds the number of elements in a set, map, tuple, or
 *  string.
\*/

case p_nelt :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* the action we take depends on the form of the first operand */

   switch (left->sp_form) {

      case ft_string :

         short_value = (left->sp_val.sp_string_ptr)->s_length;

         /* check whether the length is short */

         if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
             short_hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_value;

            break;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_value);

         break;

      case ft_set :

         short_value =
               (left->sp_val.sp_set_ptr)->s_ntype.s_root.s_cardinality;

         /* check whether the length is short */

         if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
             short_hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_value;

            break;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_value);

         break;

      case ft_map :

         short_value =
               (left->sp_val.sp_map_ptr)->m_ntype.m_root.m_cardinality;

         /* check whether the length is short */

         if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
             short_hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_value;

            break;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_value);

         break;

      case ft_tuple :

         short_value =
               (left->sp_val.sp_tuple_ptr)->t_ntype.t_root.t_length;

         /* check whether the length is short */

         if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
             short_hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_value;

            break;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_value);

         break;

      case ft_mailbox :

         short_value =
               (left->sp_val.sp_mailbox_ptr)->mb_cell_count;

         /* check whether the length is short */

         if (!(short_hi_bits = short_value & INT_HIGH_BITS) ||
             short_hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_value;

            break;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_value);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_nelt;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"NELT",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        0L,NO,YES,0);

         break;

      case ft_atom:
         push_pstack(ip->i_operand[1].i_spec_ptr);
         call_procedure(SETL_SYSTEM target,
                        spec_fsize,
                        NULL,
                        1,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         unop_abend(SETL_SYSTEM msg_bad_unop_form,"NELT",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

/*\
 *  \pcode{p\_not}{logical not}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  The not operator reverses a logical value.
\*/

case p_not :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* make sure the source is an atom */

   if (left->sp_form != ft_atom) {

      abend(SETL_SYSTEM msg_bad_unop_form,"NOT",
            abend_opnd_str(SETL_SYSTEM left));

   }

   /* reverse the logical value */

   if (left->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num) {

      unmark_specifier(target);
      target->sp_form = ft_atom;
      target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

   }
   else if (left->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {

      unmark_specifier(target);
      target->sp_form = ft_atom;
      target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

   }
   else {

      abend(SETL_SYSTEM msg_bad_unop_form,"NOT",
            abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

#endif                                 /* PARTC                             */
#if !SHORT_TEXT | PARTD

/*\
 *  \function{extract\_ops1()}
 *
 *  This function handles extraction operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void extract_ops1(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif
/* ######################## start here with abend's ####################### */
/*\
 *  \pcode{p\_tupof}{tuple extraction}
 *        {specifier}{returned value}
 *        {specifier}{tuple}
 *        {integer}{index}
 *
 *  This instruction extracts an item from a tuple.  It is used in
 *  situations in which the right hand argument {\em must} be a tuple,
 *  such as the source of a tuple assignment instruction.
\*/

case p_tupof :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the left operand must be a tuple */

   if (left->sp_form != ft_tuple) {

      abend(SETL_SYSTEM msg_expected_tuple,
            abend_opnd_str(SETL_SYSTEM left));

   }

   /*
    *  We look up tuple elements in-line, since this should be a very
    *  fast operation.
    */

   /* we convert the tuple index to a C long */

   if (right->sp_form == ft_short) {

      short_value = right->sp_val.sp_short_value;

   }
   else if (right->sp_form == ft_long) {

      short_value = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);

   }

   /* look up the tuple component */

   short_value--;
   tuple_work_hdr = left->sp_val.sp_tuple_ptr;

   for (source_height = tuple_work_hdr->t_ntype.t_root.t_height;
        source_height && tuple_work_hdr != NULL;
        source_height--) {

      /* extract the element's index at this level */

      source_index = (short_value >>
                           (source_height * TUP_SHIFT_DIST)) &
                        TUP_SHIFT_MASK;

      tuple_work_hdr = tuple_work_hdr->t_child[source_index].t_header;

   }

   /*
    *  At this point, tuple_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   source_index = short_value & TUP_SHIFT_MASK;

   if (tuple_work_hdr != NULL &&
       (tuple_cell =
           tuple_work_hdr->t_child[source_index].t_cell) != NULL) {

      mark_specifier(&(tuple_cell->t_spec));
      unmark_specifier(target);
      target->sp_form = tuple_cell->t_spec.sp_form;
      target->sp_val.sp_biggest =
            tuple_cell->t_spec.sp_val.sp_biggest;

   }
   else {

      unmark_specifier(target);
      target->sp_form = ft_omega;

   }

   break;

}

/*\
 *  \pcode{p\_of1}{procedure call, map or tuple reference}
 *        {specifier}{returned value}
 *        {specifier}{procedure, map, string, or tuple}
 *        {specifier}{index or parameter}
 *
 *  We have two forms of an \verb"of": this one in which we have only one
 *  argument (which we expect to be used most frequently), and the
 *  following version which allows multiple arguments.
\*/

case p_of1 :   case p_kof1 :           

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the left operand */

   switch (left->sp_form) {

      /*
       *  We reference strings in-line, since this should be very fast.
       */

      case ft_string :

         /* we convert the string index to a C long */
         if (right->sp_form == ft_short) {

            short_value = right->sp_val.sp_short_value;
            if (short_value < 0)
               short_value = left->sp_val.sp_string_ptr->s_length +
                             short_value + 1;

            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else if (right->sp_form == ft_long) {

            short_value = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);
            if (short_value < 0)
               short_value = left->sp_val.sp_string_ptr->s_length +
                             short_value + 1;
            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }

         /* if we reference past the end of the string, return omega */

         left_string_hdr = left->sp_val.sp_string_ptr;
         if (left_string_hdr->s_length < short_value) {

            unmark_specifier(target);
            target->sp_form = ft_omega;

            break;

         }

         /* traverse the cells until we find the character we want */

         short_value--;
         left_string_cell = left_string_hdr->s_head;
         while (short_value >= STR_CELL_WIDTH) {

            left_string_cell = left_string_cell->s_next;
            short_value -= STR_CELL_WIDTH;

         }

         /* we found the cell, pull the character */

         get_string_header(right_string_hdr);

         right_string_hdr->s_use_count = 1;
         right_string_hdr->s_hash_code = -1;
         right_string_hdr->s_length = 1;

         get_string_cell(right_string_cell);
         right_string_hdr->s_head = right_string_hdr->s_tail =
            right_string_cell;
         right_string_cell->s_next = NULL;
         right_string_cell->s_prev = NULL;
         right_string_cell->s_cell_value[0] =
            left_string_cell->s_cell_value[short_value];

         /* assign it to the target */

         unmark_specifier(target);
         target->sp_form = ft_string;
         target->sp_val.sp_string_ptr = right_string_hdr;

         break;

      /*
       *  We reference maps in-line, since this should be very fast.
       */

      case ft_set :

         if (!set_to_map(SETL_SYSTEM left,left,NO))
            abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM left));

      case ft_map :

         spec_hash_code(work_hash_code,right);
         source_hash_code = work_hash_code;

         /* look up the map component */

         map_root = left->sp_val.sp_map_ptr;
         if (ip->i_opcode==p_kof1) { 
            if (map_root->m_use_count != 1) {

              map_root->m_use_count--;
              map_root = copy_map(SETL_SYSTEM map_root);
              left->sp_val.sp_map_ptr = map_root;

            }
         }
         map_work_hdr = map_root;

         for (source_height = map_work_hdr->m_ntype.m_root.m_height;
              source_height && map_work_hdr != NULL;
              source_height--) {

            /* extract the element's index at this level */

            source_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

            map_work_hdr = map_work_hdr->m_child[source_index].m_header;

         }

         /* if we can't get to a leaf, there is no matching element */

         if (map_work_hdr == NULL) {

            unmark_specifier(target);
            target->sp_form = ft_omega;

            break;

         }

         /*
          *  At this point, map_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */

         source_index = work_hash_code & MAP_HASH_MASK;
         map_cell_tail = &(map_work_hdr->m_child[source_index].m_cell);

         for (map_cell = map_work_hdr->m_child[source_index].m_cell;
              map_cell != NULL &&
                 map_cell->m_hash_code < source_hash_code;
              map_cell = map_cell->m_next) {

              map_cell_tail = &(map_cell->m_next);

         }

         /* try to find the domain element */

         is_equal = NO;
         while (map_cell != NULL &&
                map_cell->m_hash_code == source_hash_code) {

            spec_equal(is_equal,&(map_cell->m_domain_spec),right);

            if (is_equal)
               break;

            map_cell_tail = &(map_cell->m_next);
            map_cell = map_cell->m_next;

         }

         /* if we did't find the domain element, return omega */

         if (!is_equal) {

            unmark_specifier(target);
            target->sp_form = ft_omega;

            break;

         }

         /* if the map has multiple values here, return omega */

         if (map_cell->m_is_multi_val) {

            unmark_specifier(target);
            target->sp_form = ft_omega;


         } else {

         /* otherwise, we found it */

         mark_specifier(&(map_cell->m_range_spec));
         unmark_specifier(target);
         target->sp_form = map_cell->m_range_spec.sp_form;
         target->sp_val.sp_biggest =
            map_cell->m_range_spec.sp_val.sp_biggest;

         }

         if (ip->i_opcode==p_kof1) { /* Kill this value...*/

            spec_hash_code(work_hash_code,&(map_cell->m_range_spec));
            map_root->m_hash_code ^= work_hash_code;

            if (map_cell->m_is_multi_val) {

               set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
               map_root->m_ntype.m_root.m_cardinality -=
                     set_root->s_ntype.s_root.s_cardinality;

               if ((set_root->s_ntype.s_root.s_cardinality%2)==0) 
                  map_root->m_hash_code ^= map_cell->m_hash_code;

            }
            else {

               map_root->m_ntype.m_root.m_cardinality--;

            }

            map_root->m_hash_code ^= map_cell->m_hash_code;
            *map_cell_tail = map_cell->m_next;
            unmark_specifier(&(map_cell->m_domain_spec));
            unmark_specifier(&(map_cell->m_range_spec));
            map_root->m_ntype.m_root.m_cell_count--;
            free_map_cell(map_cell);

            /*
             *  We've reduced the number of cells, so we may have to
             *  contract the map header.
             */

            contraction_trigger =
                  (1 << ((map_root->m_ntype.m_root.m_height)
                          * MAP_SHIFT_DIST));
            if (contraction_trigger == 1)
               contraction_trigger = 0;

            if (map_root->m_ntype.m_root.m_cell_count <
                contraction_trigger) {

               map_root = map_contract_header(SETL_SYSTEM map_root);
               left->sp_val.sp_map_ptr = map_root;

            }

         }

         break;

      /*
       *  We look up tuple elements in-line, since this should be a very
       *  fast operation.
       */

      case ft_tuple :

         /* we convert the tuple index to a C long */

         if (right->sp_form == ft_short) {

            short_value = right->sp_val.sp_short_value;
            if (short_value < 0)
               short_value =
                  left->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  short_value + 1;
            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else if (right->sp_form == ft_long) {

            short_value = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);
            if (short_value < 0)
               short_value =
                  left->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  short_value + 1;
            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }

         /* look up the tuple component */

         short_value--;

         tuple_root = left->sp_val.sp_tuple_ptr;
         if (ip->i_opcode==p_kof1) { 
            if (tuple_root->t_use_count != 1) {

              tuple_root->t_use_count--;
              tuple_root = copy_tuple(SETL_SYSTEM tuple_root);
              left->sp_val.sp_tuple_ptr = tuple_root;

            }
         }
         tuple_work_hdr = tuple_root;

         /* if we reference past the end of the tuple, return omega */

         if (tuple_work_hdr->t_ntype.t_root.t_length <= short_value) {

            unmark_specifier(target);
            target->sp_form = ft_omega;

            break;

         }

         for (source_height = tuple_work_hdr->t_ntype.t_root.t_height;
              source_height && tuple_work_hdr != NULL;
              source_height--) {

            /* extract the element's index at this level */

            source_index = (short_value >>
                                 (source_height * TUP_SHIFT_DIST)) &
                              TUP_SHIFT_MASK;

            tuple_work_hdr = tuple_work_hdr->t_child[source_index].t_header;

         }

         /*
          *  At this point, tuple_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */

         source_index = short_value & TUP_SHIFT_MASK;

         if (tuple_work_hdr != NULL &&
             (tuple_cell =
                 tuple_work_hdr->t_child[source_index].t_cell) != NULL) {

            mark_specifier(&(tuple_cell->t_spec));
            unmark_specifier(target);
            target->sp_form = tuple_cell->t_spec.sp_form;
            target->sp_val.sp_biggest =
               tuple_cell->t_spec.sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = ft_omega;

         }
         if (ip->i_opcode==p_kof1) {


         if  (short_value == (tuple_root->t_ntype.t_root.t_length - 1)) {

            /* first free the tuple cell */

            if (tuple_cell != NULL) {

               tuple_root->t_hash_code ^=
                     tuple_cell->t_hash_code;
               unmark_specifier(&(tuple_cell->t_spec));
               free_tuple_cell(tuple_cell);
               tuple_work_hdr->t_child[source_index].t_cell = NULL;

            }

            /* now shorten the tuple while the rightmost element is omega */

            for (;;) {

               if (!source_height && source_index >= 0) {

                  if (tuple_work_hdr->t_child[source_index].t_cell != NULL)
                     break;

                  tuple_root->t_ntype.t_root.t_length--;
                  source_index--;

                  continue;

               }

               /* move up if we're at the end of a node */

               if (source_index < 0) {

                  if (tuple_work_hdr == tuple_root)
                     break;

                  source_height++;
                  source_index =
                     tuple_work_hdr->t_ntype.t_intern.t_child_index;
                  tuple_work_hdr =
                     tuple_work_hdr->t_ntype.t_intern.t_parent;
                  free_tuple_header(
                     tuple_work_hdr->t_child[source_index].t_header);
                  tuple_work_hdr->t_child[source_index].t_header = NULL;
                  source_index--;

                  continue;

               }

               /* skip over null nodes */

               if (tuple_work_hdr->t_child[source_index].t_header == NULL) {

                  tuple_root->t_ntype.t_root.t_length -=
                        1 << (source_height * TUP_SHIFT_DIST);
                  source_index--;

                  continue;

               }

               /* otherwise drop down a level */

               tuple_work_hdr =
                  tuple_work_hdr->t_child[source_index].t_header;
               source_index = TUP_HEADER_SIZE - 1;

               source_height--;

            }

            /* we've shortened the tuple -- now reduce the height */

            while (tuple_root->t_ntype.t_root.t_height > 0 &&
                   tuple_root->t_ntype.t_root.t_length <=
                      (int32)(1L << (tuple_root->t_ntype.t_root.t_height *
                                    TUP_SHIFT_DIST))) {

               tuple_work_hdr = tuple_root->t_child[0].t_header;

               /* it's possible that we deleted internal headers */

               if (tuple_work_hdr == NULL) {

                  tuple_root->t_ntype.t_root.t_height--;
                  continue;

               }

               /* delete the root node */

               tuple_work_hdr->t_use_count = tuple_root->t_use_count;
               tuple_work_hdr->t_hash_code =
                     tuple_root->t_hash_code;
               tuple_work_hdr->t_ntype.t_root.t_length =
                     tuple_root->t_ntype.t_root.t_length;
               tuple_work_hdr->t_ntype.t_root.t_height =
                     tuple_root->t_ntype.t_root.t_height - 1;

               free_tuple_header(tuple_root);
               tuple_root = tuple_work_hdr;
               left->sp_val.sp_tuple_ptr = tuple_root;

            }

            break;

         }

         /* we aren't assigning omega, or we're not at the end of a tuple */

         if (tuple_cell == NULL) {

	    break;

         } else {

            unmark_specifier(&(tuple_cell->t_spec));
            tuple_root->t_hash_code ^=
                  tuple_cell->t_hash_code;
            free_tuple_cell(tuple_cell);
            tuple_work_hdr->t_child[source_index].t_cell = NULL;
            break;

         }

         }

         break;

      /*
       *  Normally, procedure arguments are already on the stack.  In
       *  this case, we must push it.
       */

      case ft_proc :

         /* we push our argument on to the stack */

         push_pstack(right);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,left,NULL,(int32)1,NO,NO,0);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_of;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"F(X)",
                  class_ptr->ut_name);

         /* we push our argument on to the stack */

         push_pstack(right);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        1L,NO,YES,0);

         break;
#ifdef WIN32
      case ft_opaque :
		
		 if (left->sp_val.sp_opaque_ptr->type!=ax_type) 
            abend(SETL_SYSTEM "Invalid opaque object");
                  
	
		 if (right->sp_form!=ft_string)
			 abend(SETL_SYSTEM "Invalid property of method");


         access_property(SETL_SYSTEM target,
                        left,
						right);
                        

         break;

#endif

      /*
       *  Anything else is a run-time error.
       */

      default :

         binop_abend(SETL_SYSTEM msg_bad_binop_forms,"F(X)",
               abend_opnd_str(SETL_SYSTEM left),
               abend_opnd_str(SETL_SYSTEM right));

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{extract\_ops2()}
 *
 *  This function handles extraction operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void extract_ops2(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_of}{procedure call or map reference}
 *        {specifier}{returned value}
 *        {specifier}{procedure, map, string, or tuple}
 *        {integer}{number of actual parameters}
 *
 *  This case handles the less common version of \verb"of", in which
 *  there is more than one argument.  It is only used for map references
 *  (in which case we form a tuple to use as an index) or for procedures,
 *  in which the procedure is not a literal.
\*/

case p_of :             

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* the action we take depends on the form of the left operand */

   switch (left->sp_form) {

      /*
       *  Strings and tuples are special errors.
       */

      case ft_string :

         abend(SETL_SYSTEM msg_tuple_indexes_string);

         break;

      case ft_tuple :

         abend(SETL_SYSTEM msg_tuple_indexes_tuple);

         break;

      /*
       *  We reference maps in-line, since this should be very fast.
       */

      case ft_set :

         if (!set_to_map(SETL_SYSTEM left,left,NO))
            abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM left));

      case ft_map :

         /* our argument should be a tuple, with elements on the stack */

         short_value = ip->i_operand[2].i_integer;

         /* skip trailing omegas */

         for (;
              short_value > 0 && pstack[pstack_top].sp_form == ft_omega;
              short_value--) {

            pop_pstack;

         }

         /* first we calculate the height of the new header tree */

         target_height = 0;
         work_length = short_value;
         while (work_length = work_length >> TUP_SHIFT_DIST)
            target_height++;

         /* allocate and initialize the root header item */

         get_tuple_header(tuple_root);
         tuple_root->t_use_count = 1;
         tuple_root->t_hash_code = 0;
         tuple_root->t_ntype.t_root.t_length = short_value;
         tuple_root->t_ntype.t_root.t_height = target_height;

         for (i = 0;
              i < TUP_HEADER_SIZE;
              tuple_root->t_child[i++].t_cell = NULL);

         /* insert each element in the tuple */

         target_number = 0;
         for (target_element = pstack + (pstack_top + 1 - short_value);
              target_number < short_value;
              target_element++, target_number++) {

            /* don't insert omegas */

            if (target_element->sp_form == ft_omega)
               continue;

            /* descend the header tree until we get to a leaf */

            tuple_work_hdr = tuple_root;
            for (target_height = tuple_root->t_ntype.t_root.t_height;
                 target_height;
                 target_height--) {

               /* extract the element's index at this level */

               target_index = (target_number >>
                                    (target_height * TUP_SHIFT_DIST)) &
                                 TUP_SHIFT_MASK;

               /* if we're missing a header record, allocate one */

               if (tuple_work_hdr->t_child[target_index].t_header == NULL) {

                  get_tuple_header(new_tuple_hdr);
                  new_tuple_hdr->t_ntype.t_intern.t_parent = tuple_work_hdr;
                  new_tuple_hdr->t_ntype.t_intern.t_child_index =
                     target_index;

                  for (i = 0;
                       i < TUP_HEADER_SIZE;
                       new_tuple_hdr->t_child[i++].t_cell = NULL);

                  tuple_work_hdr->t_child[target_index].t_header =
                     new_tuple_hdr;
                  tuple_work_hdr = new_tuple_hdr;

               }
               else {

                  tuple_work_hdr =
                     tuple_work_hdr->t_child[target_index].t_header;

               }
            }

            /*
             *  At this point, tuple_work_hdr points to the lowest level
             *  header record.  We insert the new element in the
             *  appropriate slot.
             */

            mark_specifier(target_element);
            get_tuple_cell(tuple_cell);
            tuple_cell->t_spec.sp_form = target_element->sp_form;
            tuple_cell->t_spec.sp_val.sp_biggest =
               target_element->sp_val.sp_biggest;
            spec_hash_code(tuple_cell->t_hash_code,target_element);
            target_index = target_number & TUP_SHIFT_MASK;
            tuple_work_hdr->t_child[target_index].t_cell = tuple_cell;
            tuple_root->t_hash_code ^= tuple_cell->t_hash_code;

         }

         /* we pop the tuple elements from the stack */

         while (short_value--) {
            pop_pstack;
         }

         /* set the target value */

         spare.sp_form = ft_tuple;
         spare.sp_val.sp_tuple_ptr = tuple_root;

         /* spare is now our key */

         spec_hash_code(work_hash_code,&spare);
         source_hash_code = work_hash_code;

         /* look up the map component */

         map_work_hdr = left->sp_val.sp_map_ptr;

         for (source_height = map_work_hdr->m_ntype.m_root.m_height;
              source_height && map_work_hdr != NULL;
              source_height--) {

            /* extract the element's index at this level */

            source_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code >>= MAP_SHIFT_DIST;

            map_work_hdr = map_work_hdr->m_child[source_index].m_header;

         }

         /* if we can't get to a leaf, there is no matching element */

         if (map_work_hdr == NULL) {

            unmark_specifier(target);
            target->sp_form = ft_omega;
            unmark_specifier(&spare);
            spare.sp_form = ft_omega;

            break;

         }

         /*
          *  At this point, map_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */

         source_index = work_hash_code & MAP_HASH_MASK;
         for (map_cell = map_work_hdr->m_child[source_index].m_cell;
              map_cell != NULL &&
                 map_cell->m_hash_code < source_hash_code;
              map_cell = map_cell->m_next);

         /* try to find the domain element */

         is_equal = NO;
         while (map_cell != NULL &&
                map_cell->m_hash_code == source_hash_code) {

            spec_equal(is_equal,&(map_cell->m_domain_spec),&spare);

            if (is_equal)
               break;

            map_cell = map_cell->m_next;

         }

         /* if we did't find the domain element, return omega */

         if (!is_equal) {

            unmark_specifier(target);
            target->sp_form = ft_omega;
            unmark_specifier(&spare);
            spare.sp_form = ft_omega;

            break;

         }

         /* if the map has multiple values here, return omega */

         if (map_cell->m_is_multi_val) {

            unmark_specifier(target);
            target->sp_form = ft_omega;
            unmark_specifier(&spare);
            spare.sp_form = ft_omega;

            break;

         }

         /* otherwise, we found it */

         mark_specifier(&(map_cell->m_range_spec));
         unmark_specifier(target);
         target->sp_form = map_cell->m_range_spec.sp_form;
         target->sp_val.sp_biggest =
            map_cell->m_range_spec.sp_val.sp_biggest;
         unmark_specifier(&spare);
         spare.sp_form = ft_omega;

         break;

      /*
       *  Procedure arguments are already on the stack.  We just call the
       *  procedure.
       */

      case ft_proc :

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
			left,NULL,ip->i_operand[2].i_integer,NO,NO,0);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         /* our argument should be a tuple, with elements on the stack */

         short_value = ip->i_operand[2].i_integer;

         /* skip trailing omegas */

         for (;
              short_value > 0 && pstack[pstack_top].sp_form == ft_omega;
              short_value--) {

            pop_pstack;

         }

         /* first we calculate the height of the new header tree */

         target_height = 0;
         work_length = short_value;
         while (work_length = work_length >> TUP_SHIFT_DIST)
            target_height++;

         /* allocate and initialize the root header item */

         get_tuple_header(tuple_root);
         tuple_root->t_use_count = 1;
         tuple_root->t_hash_code = 0;
         tuple_root->t_ntype.t_root.t_length = short_value;
         tuple_root->t_ntype.t_root.t_height = target_height;

         for (i = 0;
              i < TUP_HEADER_SIZE;
              tuple_root->t_child[i++].t_cell = NULL);

         /* insert each element in the tuple */

         target_number = 0;
         for (target_element = pstack + (pstack_top + 1 - short_value);
              target_number < short_value;
              target_element++, target_number++) {

            /* don't insert omegas */

            if (target_element->sp_form == ft_omega)
               continue;

            /* descend the header tree until we get to a leaf */

            tuple_work_hdr = tuple_root;
            for (target_height = tuple_root->t_ntype.t_root.t_height;
                 target_height;
                 target_height--) {

               /* extract the element's index at this level */

               target_index = (target_number >>
                                    (target_height * TUP_SHIFT_DIST)) &
                                 TUP_SHIFT_MASK;

               /* if we're missing a header record, allocate one */

               if (tuple_work_hdr->t_child[target_index].t_header == NULL) {

                  get_tuple_header(new_tuple_hdr);
                  new_tuple_hdr->t_ntype.t_intern.t_parent = tuple_work_hdr;
                  new_tuple_hdr->t_ntype.t_intern.t_child_index =
                     target_index;

                  for (i = 0;
                       i < TUP_HEADER_SIZE;
                       new_tuple_hdr->t_child[i++].t_cell = NULL);

                  tuple_work_hdr->t_child[target_index].t_header =
                     new_tuple_hdr;
                  tuple_work_hdr = new_tuple_hdr;

               }
               else {

                  tuple_work_hdr =
                     tuple_work_hdr->t_child[target_index].t_header;

               }
            }

            /*
             *  At this point, tuple_work_hdr points to the lowest level
             *  header record.  We insert the new element in the
             *  appropriate slot.
             */

            mark_specifier(target_element);
            get_tuple_cell(tuple_cell);
            tuple_cell->t_spec.sp_form = target_element->sp_form;
            tuple_cell->t_spec.sp_val.sp_biggest =
               target_element->sp_val.sp_biggest;
            spec_hash_code(tuple_cell->t_hash_code,target_element);
            target_index = target_number & TUP_SHIFT_MASK;
            tuple_work_hdr->t_child[target_index].t_cell = tuple_cell;
            tuple_root->t_hash_code ^= tuple_cell->t_hash_code;

         }

         /* we pop the tuple elements from the stack */

         while (short_value--) {
            pop_pstack;
         }

         /* set spare to the tuple */

         spare.sp_form = ft_tuple;
         spare.sp_val.sp_tuple_ptr = tuple_root;

         /* we push our argument on to the stack */

         push_pstack(&spare);
         unmark_specifier(&spare);
         spare.sp_form = ft_omega;

         /* pick out the class pointer */

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_of;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"F(X)",
                  class_ptr->ut_name);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        1L,
                        NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         abend(SETL_SYSTEM msg_bad_unop_form,"F(I,J)",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

/*\
 *  \pcode{p\_ofa}{multi-valued map reference}
 *        {specifier}{returned value}
 *        {specifier}{map}
 *        {specifier}{index}
 *
 *  This case handles mult-valued map references (\verb"y = f{x}").  It
 *  is almost identical to the map reference in \verb"p_of1".
\*/

case p_ofa :          case p_kofa : 

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   switch (left->sp_form) {

      case ft_set :

         if (!set_to_map(SETL_SYSTEM left,left,NO))
            abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM left));

      case ft_map :

         spec_hash_code(work_hash_code,right);
         source_hash_code = work_hash_code;

         /* look up the map component */

         map_root = left->sp_val.sp_map_ptr;
         if (ip->i_opcode==p_kofa) { 
            if (map_root->m_use_count != 1) {

               map_root->m_use_count--;
               map_root = copy_map(SETL_SYSTEM map_root);
               left->sp_val.sp_map_ptr = map_root;

            }
         }

         map_work_hdr = map_root;

         for (source_height = map_work_hdr->m_ntype.m_root.m_height;
              source_height && map_work_hdr != NULL;
              source_height--) {

            /* extract the element's index at this level */

            source_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

            map_work_hdr = map_work_hdr->m_child[source_index].m_header;

         }

         /* if we can't get to a leaf, there is no matching element */

         if (map_work_hdr == NULL) {

            mark_specifier(spec_nullset);
            unmark_specifier(target);
            target->sp_form = ft_set;
            target->sp_val.sp_set_ptr = spec_nullset->sp_val.sp_set_ptr;

            break;

         }

         /*
          *  At this point, map_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */


         source_index = work_hash_code & MAP_HASH_MASK;
         map_cell_tail = &(map_work_hdr->m_child[source_index].m_cell);
         for (map_cell = map_work_hdr->m_child[source_index].m_cell;
              map_cell != NULL &&
                 map_cell->m_hash_code < source_hash_code;
              map_cell = map_cell->m_next) {

              map_cell_tail = &(map_cell->m_next);
         }

         /* try to find the domain element */

         is_equal = NO;
         while (map_cell != NULL &&
                map_cell->m_hash_code == source_hash_code) {

            spec_equal(is_equal,&(map_cell->m_domain_spec),right);

            if (is_equal)
               break;

            map_cell_tail = &(map_cell->m_next);
            map_cell = map_cell->m_next;

         }

         /* if we did't find the domain element, return an empty set */

         if (!is_equal) {

            mark_specifier(spec_nullset);
            unmark_specifier(target);
            target->sp_form = ft_set;
            target->sp_val.sp_set_ptr = spec_nullset->sp_val.sp_set_ptr;

            break;

         }

         /* if the map has multiple values here, return the set */

         if (map_cell->m_is_multi_val) {

            mark_specifier(&(map_cell->m_range_spec));
            unmark_specifier(target);
            target->sp_form = ft_set;
            target->sp_val.sp_set_ptr =
               map_cell->m_range_spec.sp_val.sp_set_ptr;


         } else {

         /* otherwise, we must make a singleton set */

         get_set_header(set_root);
         set_root->s_use_count = 1;
         set_root->s_ntype.s_root.s_cardinality = 1;
         set_root->s_ntype.s_root.s_height = 0;
         for (i = 0;
              i < SET_HASH_SIZE;
              set_root->s_child[i++].s_cell = NULL);

         spec_hash_code(work_hash_code,&(map_cell->m_range_spec));
         set_root->s_hash_code = work_hash_code;
         get_set_cell(set_cell);
         set_cell->s_spec.sp_form = map_cell->m_range_spec.sp_form;
         set_cell->s_spec.sp_val.sp_biggest =
               map_cell->m_range_spec.sp_val.sp_biggest;
         mark_specifier(&(set_cell->s_spec));
         set_cell->s_hash_code = work_hash_code;
         set_cell->s_next = NULL;
         set_root->s_child[work_hash_code & SET_HASH_MASK].s_cell = set_cell;

         /* assign the singleton set to the target */

         unmark_specifier(target);
         target->sp_form = ft_set;
         target->sp_val.sp_set_ptr = set_root;

         }

         if (ip->i_opcode==p_kofa) { /* Kill this value...*/

            spec_hash_code(work_hash_code,&(map_cell->m_range_spec));
            map_root->m_hash_code ^= work_hash_code;

            if (map_cell->m_is_multi_val) {

               set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
               map_root->m_ntype.m_root.m_cardinality -=
                     set_root->s_ntype.s_root.s_cardinality;

               if ((set_root->s_ntype.s_root.s_cardinality%2)==0) 
                  map_root->m_hash_code ^= map_cell->m_hash_code;

            }
            else {

               map_root->m_ntype.m_root.m_cardinality--;

            }

            map_root->m_hash_code ^= map_cell->m_hash_code;
            *map_cell_tail = map_cell->m_next;
            unmark_specifier(&(map_cell->m_domain_spec));
            unmark_specifier(&(map_cell->m_range_spec));
            map_root->m_ntype.m_root.m_cell_count--;
            free_map_cell(map_cell);

            /*
             *  We've reduced the number of cells, so we may have to
             *  contract the map header.
             */

            contraction_trigger =
                  (1 << ((map_root->m_ntype.m_root.m_height)
                          * MAP_SHIFT_DIST));
            if (contraction_trigger == 1)
               contraction_trigger = 0;

            if (map_root->m_ntype.m_root.m_cell_count <
                contraction_trigger) {

               map_root = map_contract_header(SETL_SYSTEM map_root);
               left->sp_val.sp_map_ptr = map_root;

            }

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_ofa;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"F{X}",
                  class_ptr->ut_name);

         /* we push our argument on to the stack */

         push_pstack(right);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        1L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         binop_abend(SETL_SYSTEM msg_bad_binop_forms,"F{X}",
               abend_opnd_str(SETL_SYSTEM left),
               abend_opnd_str(SETL_SYSTEM right));

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{extract\_ops3()}
 *
 *  This function handles extraction operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void extract_ops3(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_slice}{string or tuple slice}
 *        {specifier}{returned value}
 *        {specifier}{string or tuple}
 *        {specifier}{beginning of slice}
 *
 *  This case handles slice references.  We need four operands for this
 *  instruction, so the end of slice operand is the first operand of the
 *  following instruction, which will be a noop.
\*/

case p_slice :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;
   end = PC->i_operand[0].i_spec_ptr;
   BUMPPC(1);

   switch (left->sp_form) {

      case ft_string :

         /* we convert the slice front to a C long */

         if (right->sp_form == ft_short) {

            slice_start = right->sp_val.sp_short_value;
            if (slice_start <= 0)
               slice_start = left->sp_val.sp_string_ptr->s_length +
                             slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else if (right->sp_form == ft_long) {

            slice_start = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);
            if (slice_start <= 0)
               slice_start = left->sp_val.sp_string_ptr->s_length +
                             slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }

         /* we convert the slice end to a C long */

         if (end->sp_form == ft_short) {

            slice_end = end->sp_val.sp_short_value;
            if (slice_end < 0)
               slice_end = left->sp_val.sp_string_ptr->s_length +
                           slice_end + 1;
            if (slice_end < 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM end));

         }
         else if (end->sp_form == ft_long) {

            slice_end = long_to_short(SETL_SYSTEM end->sp_val.sp_long_ptr);
            if (slice_end < 0)
               slice_end = left->sp_val.sp_string_ptr->s_length +
                           slice_end + 1;
            if (slice_end < 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM end));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM end));

         }

         /* make sure start <= end + 1 */

         if (slice_start > slice_end + 1)
            abend(SETL_SYSTEM msg_invalid_slice_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right),
                  abend_opnd_str(SETL_SYSTEM end));

         /* if we reference past the end of the string, abend */

         left_string_hdr = left->sp_val.sp_string_ptr;
         if (left_string_hdr->s_length < slice_end)
            abend(SETL_SYSTEM msg_invalid_slice_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right),
                  abend_opnd_str(SETL_SYSTEM end));

         /* we start with a null string */

         get_string_header(target_string_hdr);
         target_string_hdr->s_use_count = 1;
         target_string_hdr->s_hash_code = -1;
         target_string_hdr->s_length = 0;
         target_string_hdr->s_head = target_string_hdr->s_tail = NULL;

         /* if start = end + 1, return NULL */

         if (slice_start == slice_end + 1) {

            unmark_specifier(target);
            target->sp_form = ft_string;
            target->sp_val.sp_string_ptr = target_string_hdr;

            break;

         }

         /* otherwise copy the slice */

         target_string_length = slice_end - slice_start + 1;
         target_string_hdr->s_length = target_string_length;

         /* traverse the cells until we find the character we want */

         slice_start--;
         left_string_cell = left_string_hdr->s_head;
         while (slice_start >= STR_CELL_WIDTH) {

            left_string_cell = left_string_cell->s_next;
            slice_start -= STR_CELL_WIDTH;

         }

         /* we just attach the left string to the target */

         left_char_ptr = left_string_cell->s_cell_value + slice_start;
         left_char_end = left_string_cell->s_cell_value + STR_CELL_WIDTH;
         target_char_ptr = target_char_end = NULL;

         while (target_string_length--) {

            if (left_char_ptr == left_char_end) {

               left_string_cell = left_string_cell->s_next;
               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            if (target_char_ptr == target_char_end) {

               get_string_cell(target_string_cell);
               if (target_string_hdr->s_tail != NULL)
                  (target_string_hdr->s_tail)->s_next = target_string_cell;
               target_string_cell->s_prev = target_string_hdr->s_tail;
               target_string_cell->s_next = NULL;
               target_string_hdr->s_tail = target_string_cell;
               if (target_string_hdr->s_head == NULL)
                  target_string_hdr->s_head = target_string_cell;
               target_char_ptr = target_string_cell->s_cell_value;
               target_char_end = target_char_ptr + STR_CELL_WIDTH;

            }

            *target_char_ptr++ = *left_char_ptr++;

         }

         /* assign our result to the target */

         unmark_specifier(target);
         target->sp_form = ft_string;
         target->sp_val.sp_string_ptr = target_string_hdr;

         break;

      /*
       *  We produce tuple slices in the tuple package, and hope that it
       *  will not be used much.
       */

      case ft_tuple :

         /* we convert the slice front to a C long */

         if (right->sp_form == ft_short) {

            slice_start = right->sp_val.sp_short_value;
            if (slice_start <= 0)
               slice_start = 
                  left->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else if (right->sp_form == ft_long) {

            slice_start = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);
            if (slice_start <= 0)
               slice_start = 
                  left->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }

         /* we convert the slice end to a C long */

         if (end->sp_form == ft_short) {

            slice_end = end->sp_val.sp_short_value;
            if (slice_end < 0)
               slice_end = 
                  left->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_end + 1;
            if (slice_end < 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else if (end->sp_form == ft_long) {

            slice_end = long_to_short(SETL_SYSTEM end->sp_val.sp_long_ptr);
            if (slice_end < 0)
               slice_end = 
                  left->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_end + 1;
            if (slice_end < 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }

         /* make sure start <= end + 1 */

         if (slice_start > slice_end + 1)
            abend(SETL_SYSTEM msg_invalid_slice_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right),
                  abend_opnd_str(SETL_SYSTEM end));

         /* if we reference past the end of the tuple, abend */

         tuple_root = left->sp_val.sp_tuple_ptr;
         if (tuple_root->t_ntype.t_root.t_length < slice_end)
            abend(SETL_SYSTEM msg_invalid_slice_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right),
                  abend_opnd_str(SETL_SYSTEM end));

         tuple_slice(SETL_SYSTEM target,left,slice_start,slice_end);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_slice;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"F(I..J)",
                  class_ptr->ut_name);

         /* we push our arguments on to the stack */

         push_pstack(right);
         push_pstack(end);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        2L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         triop_abend(SETL_SYSTEM msg_bad_unop_form,"F(I..J)",
               abend_opnd_str(SETL_SYSTEM left),NULL,NULL);

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{extract\_ops4()}
 *
 *  This function handles extraction operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void extract_ops4(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_end}{string or tuple tail}
 *        {specifier}{returned value}
 *        {specifier}{string or tuple}
 *        {specifier}{beginning of slice}
 *
 *  This case handles slice references which extend to the end of the
 *  string or tuple.  It is a copy of the slice operation, with the end
 *  being the length of the tuple or string.
\*/

case p_end :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   switch (left->sp_form) {

      case ft_string :

         /* we convert the slice front to a C long */

         if (right->sp_form == ft_short) {

            slice_start = right->sp_val.sp_short_value;
            if (slice_start <= 0)
               slice_start = left->sp_val.sp_string_ptr->s_length +
                             slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else if (right->sp_form == ft_long) {

            slice_start = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);
            if (slice_start <= 0)
               slice_start = left->sp_val.sp_string_ptr->s_length +
                             slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }

         left_string_hdr = left->sp_val.sp_string_ptr;
         slice_end = left_string_hdr->s_length;

         /* make sure start <= end + 1 */

         if (slice_start > slice_end + 1)
            abend(SETL_SYSTEM msg_invalid_tail_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         /* we start with a null string */

         get_string_header(target_string_hdr);
         target_string_hdr->s_use_count = 1;
         target_string_hdr->s_hash_code = -1;
         target_string_hdr->s_length = 0;
         target_string_hdr->s_head = target_string_hdr->s_tail = NULL;

         /* if start = end + 1, return NULL */

         if (slice_start == slice_end + 1) {

            unmark_specifier(target);
            target->sp_form = ft_string;
            target->sp_val.sp_string_ptr = target_string_hdr;

            break;

         }

         /* otherwise copy the slice */

         target_string_length = slice_end - slice_start + 1;
         target_string_hdr->s_length = target_string_length;

         /* traverse the cells until we find the character we want */

         slice_start--;
         left_string_cell = left_string_hdr->s_head;
         while (slice_start >= STR_CELL_WIDTH) {

            left_string_cell = left_string_cell->s_next;
            slice_start -= STR_CELL_WIDTH;

         }

         /* we just attach the left string to the target */

         left_char_ptr = left_string_cell->s_cell_value + slice_start;
         left_char_end = left_string_cell->s_cell_value + STR_CELL_WIDTH;
         target_char_ptr = target_char_end = NULL;

         while (target_string_length--) {

            if (left_char_ptr == left_char_end) {

               left_string_cell = left_string_cell->s_next;
               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            if (target_char_ptr == target_char_end) {

               get_string_cell(target_string_cell);
               if (target_string_hdr->s_tail != NULL)
                  (target_string_hdr->s_tail)->s_next = target_string_cell;
               target_string_cell->s_prev = target_string_hdr->s_tail;
               target_string_cell->s_next = NULL;
               target_string_hdr->s_tail = target_string_cell;
               if (target_string_hdr->s_head == NULL)
                  target_string_hdr->s_head = target_string_cell;
               target_char_ptr = target_string_cell->s_cell_value;
               target_char_end = target_char_ptr + STR_CELL_WIDTH;

            }

            *target_char_ptr++ = *left_char_ptr++;

         }

         /* assign our result to the target */

         unmark_specifier(target);
         target->sp_form = ft_string;
         target->sp_val.sp_string_ptr = target_string_hdr;

         break;

      /*
       *  We produce tuple slices in the tuple package, and hope that it
       *  will not be used much.
       */

      case ft_tuple :

         /* we convert the slice front to a C long */

         if (right->sp_form == ft_short) {

            slice_start = right->sp_val.sp_short_value;
            if (slice_start <= 0)
               slice_start = 
                  left->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else if (right->sp_form == ft_long) {

            slice_start = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);
            if (slice_start <= 0)
               slice_start = 
                  left->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM right));

         }

         tuple_work_hdr = left->sp_val.sp_tuple_ptr;
         slice_end = tuple_work_hdr->t_ntype.t_root.t_length;

         /* make sure start <= end + 1 */

         if (slice_start > slice_end + 1)
            abend(SETL_SYSTEM msg_invalid_tail_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         tuple_slice(SETL_SYSTEM target,left,slice_start,slice_end);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_end;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"F(I..)",
                  class_ptr->ut_name);

         /* we push our arguments on to the stack */

         push_pstack(right);

         /* call the procedure */

         call_procedure(SETL_SYSTEM target,
                        slot_info->si_spec,
                        left,
                        1L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         binop_abend(SETL_SYSTEM msg_bad_unop_form,"F(I..)",
               abend_opnd_str(SETL_SYSTEM left),NULL);

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

#endif                                 /* PARTD                             */
#if !SHORT_TEXT | PARTE

/*\
 *  \function{assign\_ops1()}
 *
 *  This function handles assignment operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void assign_ops1(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_assign}{assignment}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {}{unused}
 *
 *  An assignment just copies one specifier to another, unmarking the
 *  target and marking the source.
\*/

case p_assign :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* mark the source, unmark the target */

   mark_specifier(left);
   unmark_specifier(target);

   /* copy the specifier */

   target->sp_form = left->sp_form;
   target->sp_val.sp_biggest = left->sp_val.sp_biggest;

   break;

}

/*\
 *  \pcode{p\_sof}{sinister assignment}
 *        {specifier}{target}
 *        {specifier}{index or parameter}
 *        {specifier}{source}
 *
 *  This case handles the most general form of sinister assignment.  The
 *  aggregate may be a map, tuple, or string.
\*/

case p_sof :  

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the target operand */

   switch (target->sp_form) {

      /*
       *  We modify strings in-line, since this should be very fast.
       */

      case ft_string :
         
         /* we convert the string index to a C long */

         if (left->sp_form == ft_short) {

            short_value = left->sp_val.sp_short_value;
            if (short_value < 0)
               short_value = target->sp_val.sp_string_ptr->s_length +
                             short_value + 1;
            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else if (left->sp_form == ft_long) {

            short_value = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
            if (short_value < 0)
               short_value = target->sp_val.sp_string_ptr->s_length +
                             short_value + 1;
            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }

         left_string_hdr = target->sp_val.sp_string_ptr;

         /* if we reference past the end of the string, abend */

         if (left_string_hdr->s_length < short_value)
            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         /* the source must be a single character */

         if (right->sp_form != ft_string)
            abend(SETL_SYSTEM msg_source_too_long);

         right_string_hdr = right->sp_val.sp_string_ptr;
         if (right_string_hdr->s_length != 1)
            abend(SETL_SYSTEM msg_source_too_long);

         /* we would like to use the target destructively */

         if (left_string_hdr->s_use_count != 1) {

            left_string_hdr->s_use_count--;
            left_string_hdr = copy_string(SETL_SYSTEM left_string_hdr);
            target->sp_val.sp_string_ptr = left_string_hdr;

         }

         /* traverse the cells until we find the character we want */

         short_value--;
         left_string_cell = left_string_hdr->s_head;
         while (short_value >= STR_CELL_WIDTH) {

            left_string_cell = left_string_cell->s_next;
            short_value -= STR_CELL_WIDTH;

         }

         /* set the target character */

         right_string_cell = right_string_hdr->s_head;
         left_string_cell->s_cell_value[short_value] =
            right_string_cell->s_cell_value[0];

         break;

      /*
       *  We modify maps in-line, since this should be very fast.
       */

      case ft_set :

         if (!set_to_map(SETL_SYSTEM target,target,NO))
            abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM target));

      case ft_map :

         /* we would like to use the target destructively */
		 if (left->sp_form==ft_omega)
		 	abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));


         map_root = target->sp_val.sp_map_ptr;
         if (map_root->m_use_count != 1) {

            map_root->m_use_count--;
            map_root = copy_map(SETL_SYSTEM map_root);
            target->sp_val.sp_map_ptr = map_root;

         }

         /* look up the domain element in the map */

         map_work_hdr = map_root;
         spec_hash_code(work_hash_code,right);
         range_hash_code = work_hash_code;
         spec_hash_code(work_hash_code,left);
         domain_hash_code = work_hash_code;

         for (source_height = map_work_hdr->m_ntype.m_root.m_height;
              source_height;
              source_height--) {

            /* extract the element's index at this level */

            source_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

            /* if we're missing a header record, insert it */

            if (map_work_hdr->m_child[source_index].m_header == NULL) {

               get_map_header(new_map_hdr);
               new_map_hdr->m_ntype.m_intern.m_parent = map_work_hdr;
               new_map_hdr->m_ntype.m_intern.m_child_index = source_index;
               for (i = 0;
                    i < MAP_HASH_SIZE;
                    new_map_hdr->m_child[i++].m_cell = NULL);
               map_work_hdr->m_child[source_index].m_header = new_map_hdr;
               map_work_hdr = new_map_hdr;

            }
            else {

               map_work_hdr = map_work_hdr->m_child[source_index].m_header;

            }
         }

         /*
          *  At this point, map_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */

         source_index = work_hash_code & MAP_HASH_MASK;
         map_cell_tail = &(map_work_hdr->m_child[source_index].m_cell);
         for (map_cell = map_work_hdr->m_child[source_index].m_cell;
              map_cell != NULL &&
                 map_cell->m_hash_code < domain_hash_code;
              map_cell = map_cell->m_next) {

            map_cell_tail = &(map_cell->m_next);

         }

         /* try to find the domain element */

         is_equal = NO;
         while (map_cell != NULL &&
                map_cell->m_hash_code == domain_hash_code) {

            spec_equal(is_equal,&(map_cell->m_domain_spec),left);

            if (is_equal)
               break;

            map_cell_tail = &(map_cell->m_next);
            map_cell = map_cell->m_next;

         }

         /* if the source operand is omega, remove the cell */

         if (right->sp_form == ft_omega) {

            if (!is_equal)
               break;

            spec_hash_code(work_hash_code,&(map_cell->m_range_spec));
            map_root->m_hash_code ^= work_hash_code;

            if (map_cell->m_is_multi_val) {

               set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
               map_root->m_ntype.m_root.m_cardinality -=
                     set_root->s_ntype.s_root.s_cardinality;

               if ((set_root->s_ntype.s_root.s_cardinality%2)==0) 
                  map_root->m_hash_code ^= map_cell->m_hash_code;

            }
            else {

               map_root->m_ntype.m_root.m_cardinality--;

            }

            map_root->m_hash_code ^= map_cell->m_hash_code;
            *map_cell_tail = map_cell->m_next;
            unmark_specifier(&(map_cell->m_domain_spec));
            unmark_specifier(&(map_cell->m_range_spec));
            map_root->m_ntype.m_root.m_cell_count--;
            free_map_cell(map_cell);

            /*
             *  We've reduced the number of cells, so we may have to
             *  contract the map header.
             */

            contraction_trigger =
                  (1 << ((map_root->m_ntype.m_root.m_height)
                          * MAP_SHIFT_DIST));
            if (contraction_trigger == 1)
               contraction_trigger = 0;

            if (map_root->m_ntype.m_root.m_cell_count <
                contraction_trigger) {

               map_root = map_contract_header(SETL_SYSTEM map_root);
               target->sp_val.sp_map_ptr = map_root;

            }

            break;

         }

         /*
          *  The source is not omega, so we add or change a cell.
          */

         /* if we don't find the domain element, add a cell */

         if (!is_equal) {

            get_map_cell(new_map_cell);
            mark_specifier(left);
            mark_specifier(right);
            new_map_cell->m_domain_spec.sp_form = left->sp_form;
            new_map_cell->m_domain_spec.sp_val.sp_biggest =
                  left->sp_val.sp_biggest;
            new_map_cell->m_range_spec.sp_form = right->sp_form;
            new_map_cell->m_range_spec.sp_val.sp_biggest =
                  right->sp_val.sp_biggest;
            new_map_cell->m_is_multi_val = NO;
            new_map_cell->m_hash_code = domain_hash_code;
            new_map_cell->m_next = *map_cell_tail;
            *map_cell_tail = new_map_cell;
            map_root->m_ntype.m_root.m_cardinality++;
            map_root->m_ntype.m_root.m_cell_count++;
            map_root->m_hash_code ^= domain_hash_code;
            map_root->m_hash_code ^= range_hash_code;

            /* expand the map header if necessary */

            expansion_trigger =
                  (1 << ((map_root->m_ntype.m_root.m_height + 1)
                         * MAP_SHIFT_DIST)) * 2;
            if (map_root->m_ntype.m_root.m_cardinality >
                expansion_trigger) {

               map_root = map_expand_header(SETL_SYSTEM map_root);
               target->sp_val.sp_map_ptr = map_root;

            }

            break;

         }

         /* if we had a multi-value cell, decrease the cardinality */

         if (map_cell->m_is_multi_val) {

            set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
            map_root->m_ntype.m_root.m_cardinality -=
                  (set_root->s_ntype.s_root.s_cardinality - 1);

            if ((set_root->s_ntype.s_root.s_cardinality%2)==0) 
               map_root->m_hash_code ^= map_cell->m_hash_code;
         }

         /* set the range element */


         spec_hash_code(work_hash_code,&(map_cell->m_range_spec));
         map_root->m_hash_code ^= work_hash_code;

         mark_specifier(right);
         unmark_specifier(&(map_cell->m_range_spec));
         map_cell->m_range_spec.sp_form = right->sp_form;
         map_cell->m_range_spec.sp_val.sp_biggest =
               right->sp_val.sp_biggest;
         map_cell->m_is_multi_val = NO;
         map_root->m_hash_code ^= range_hash_code;

         break;

      /*
       *  We modify tuple elements in-line, since this should be a very
       *  fast operation.
       */

      case ft_tuple :

         /* we convert the tuple index to a C long */

         if (left->sp_form == ft_short) {

            short_value = left->sp_val.sp_short_value;
            if (short_value < 0)
               short_value =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  short_value + 1;
            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else if (left->sp_form == ft_long) {

            short_value = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
            if (short_value < 0)
               short_value =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  short_value + 1;
            if (short_value <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }

         tuple_root = target->sp_val.sp_tuple_ptr;

         /* we would like to use the target destructively */

         if (tuple_root->t_use_count != 1) {

            tuple_root->t_use_count--;
            tuple_root = copy_tuple(SETL_SYSTEM tuple_root);
            target->sp_val.sp_tuple_ptr = tuple_root;

         }

         /* our indices are zero-based */

         short_value--;

         /* if the item is past the end of the tuple, extend the tuple */

         if (short_value >= tuple_root->t_ntype.t_root.t_length) {

            expansion_trigger =
                  1 << ((tuple_root->t_ntype.t_root.t_height + 1)
                        * TUP_SHIFT_DIST);

            /* expand the target tree if necessary */

            while (short_value >= expansion_trigger) {

               tuple_work_hdr = tuple_root;

               get_tuple_header(tuple_root);

               tuple_root->t_use_count = 1;
               tuple_root->t_hash_code =
                  tuple_work_hdr->t_hash_code;
               tuple_root->t_ntype.t_root.t_length =
                  tuple_work_hdr->t_ntype.t_root.t_length;
               tuple_root->t_ntype.t_root.t_height =
                  tuple_work_hdr->t_ntype.t_root.t_height + 1;

               for (i = 1;
                    i < TUP_HEADER_SIZE;
                    tuple_root->t_child[i++].t_header = NULL);

               tuple_root->t_child[0].t_header = tuple_work_hdr;

               tuple_work_hdr->t_ntype.t_intern.t_parent = tuple_root;
               tuple_work_hdr->t_ntype.t_intern.t_child_index = 0;

               expansion_trigger *= TUP_HEADER_SIZE;

            }

            tuple_root->t_ntype.t_root.t_length = short_value + 1;
            target->sp_val.sp_tuple_ptr = tuple_root;

         }

         /* look up the tuple component */

         tuple_work_hdr = tuple_root;

         for (source_height = tuple_work_hdr->t_ntype.t_root.t_height;
              source_height;
              source_height--) {

            /* extract the element's index at this level */

            source_index = (short_value >>
                                 (source_height * TUP_SHIFT_DIST)) &
                              TUP_SHIFT_MASK;

            /* if we're missing a header record, allocate one */

            if (tuple_work_hdr->t_child[source_index].t_header == NULL) {

               get_tuple_header(new_tuple_hdr);
               new_tuple_hdr->t_ntype.t_intern.t_parent = tuple_work_hdr;
               new_tuple_hdr->t_ntype.t_intern.t_child_index = source_index;
               for (i = 0;
                    i < TUP_HEADER_SIZE;
                    new_tuple_hdr->t_child[i++].t_cell = NULL);
               tuple_work_hdr->t_child[source_index].t_header =
                     new_tuple_hdr;
               tuple_work_hdr = new_tuple_hdr;

            }
            else {

               tuple_work_hdr =
                  tuple_work_hdr->t_child[source_index].t_header;

            }
         }

         /*
          *  At this point, tuple_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */

         source_index = short_value & TUP_SHIFT_MASK;
         tuple_cell = tuple_work_hdr->t_child[source_index].t_cell;

         /* if we assign omega to the last cell of a tuple, we shorten it */

         if (right->sp_form == ft_omega &&
             short_value == (tuple_root->t_ntype.t_root.t_length - 1)) {

            /* first free the tuple cell */

            if (tuple_cell != NULL) {

               tuple_root->t_hash_code ^=
                     tuple_cell->t_hash_code;
               unmark_specifier(&(tuple_cell->t_spec));
               free_tuple_cell(tuple_cell);
               tuple_work_hdr->t_child[source_index].t_cell = NULL;

            }

            /* now shorten the tuple while the rightmost element is omega */

            for (;;) {

               if (!source_height && source_index >= 0) {

                  if (tuple_work_hdr->t_child[source_index].t_cell != NULL)
                     break;

                  tuple_root->t_ntype.t_root.t_length--;
                  source_index--;

                  continue;

               }

               /* move up if we're at the end of a node */

               if (source_index < 0) {

                  if (tuple_work_hdr == tuple_root)
                     break;

                  source_height++;
                  source_index =
                     tuple_work_hdr->t_ntype.t_intern.t_child_index;
                  tuple_work_hdr =
                     tuple_work_hdr->t_ntype.t_intern.t_parent;
                  free_tuple_header(
                     tuple_work_hdr->t_child[source_index].t_header);
                  tuple_work_hdr->t_child[source_index].t_header = NULL;
                  source_index--;

                  continue;

               }

               /* skip over null nodes */

               if (tuple_work_hdr->t_child[source_index].t_header == NULL) {

                  tuple_root->t_ntype.t_root.t_length -=
                        1 << (source_height * TUP_SHIFT_DIST);
                  source_index--;

                  continue;

               }

               /* otherwise drop down a level */

               tuple_work_hdr =
                  tuple_work_hdr->t_child[source_index].t_header;
               source_index = TUP_HEADER_SIZE - 1;

               source_height--;

            }

            /* we've shortened the tuple -- now reduce the height */

            while (tuple_root->t_ntype.t_root.t_height > 0 &&
                   tuple_root->t_ntype.t_root.t_length <=
                      (int32)(1L << (tuple_root->t_ntype.t_root.t_height *
                                    TUP_SHIFT_DIST))) {

               tuple_work_hdr = tuple_root->t_child[0].t_header;

               /* it's possible that we deleted internal headers */

               if (tuple_work_hdr == NULL) {

                  tuple_root->t_ntype.t_root.t_height--;
                  continue;

               }

               /* delete the root node */

               tuple_work_hdr->t_use_count = tuple_root->t_use_count;
               tuple_work_hdr->t_hash_code =
                     tuple_root->t_hash_code;
               tuple_work_hdr->t_ntype.t_root.t_length =
                     tuple_root->t_ntype.t_root.t_length;
               tuple_work_hdr->t_ntype.t_root.t_height =
                     tuple_root->t_ntype.t_root.t_height - 1;

               free_tuple_header(tuple_root);
               tuple_root = tuple_work_hdr;
               target->sp_val.sp_tuple_ptr = tuple_root;

            }

            break;

         }

         /* we aren't assigning omega, or we're not at the end of a tuple */

         if (tuple_cell == NULL) {

            if (right->sp_form == ft_omega)
               break;

            get_tuple_cell(tuple_cell);
            tuple_work_hdr->t_child[source_index].t_cell = tuple_cell;
            mark_specifier(right);
            tuple_cell->t_spec.sp_form = right->sp_form;
            tuple_cell->t_spec.sp_val.sp_biggest = right->sp_val.sp_biggest;
            spec_hash_code(work_hash_code,right);
            tuple_root->t_hash_code ^= work_hash_code;
            tuple_cell->t_hash_code = work_hash_code;

            break;

         }

         if (right->sp_form == ft_omega) {

            unmark_specifier(&(tuple_cell->t_spec));
            tuple_root->t_hash_code ^=
                  tuple_cell->t_hash_code;
            free_tuple_cell(tuple_cell);
            tuple_work_hdr->t_child[source_index].t_cell = NULL;

            break;

         }

         mark_specifier(right);
         unmark_specifier(&(tuple_cell->t_spec));
         tuple_root->t_hash_code ^= tuple_cell->t_hash_code;
         tuple_cell->t_spec.sp_form = right->sp_form;
         tuple_cell->t_spec.sp_val.sp_biggest = right->sp_val.sp_biggest;
         spec_hash_code(work_hash_code,right);
         tuple_root->t_hash_code ^= work_hash_code;
         tuple_cell->t_hash_code = work_hash_code;

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = target->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_sof;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"F(X) :=",
                  class_ptr->ut_name);

         /* we push our arguments on to the stack */

         push_pstack(left);
         push_pstack(right);

         /* call the procedure */

         call_procedure(SETL_SYSTEM NULL,
                        slot_info->si_spec,
                        target,
                        2L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         abend(SETL_SYSTEM msg_bad_unop_form,"F(X) :=",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{assign\_ops2()}
 *
 *  This function handles assignment operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void assign_ops2(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_sofa}{multi-valued map assignment}
 *        {specifier}{target}
 *        {specifier}{parameter}
 *        {specifier}{source}
 *
 *  This case handles multi-valued map assignments.
\*/

case p_sofa :  

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   switch (target->sp_form) {

      case ft_set :

         if (!set_to_map(SETL_SYSTEM target,target,NO))
            abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM target));

      case ft_map :

         /* we would like to use the target destructively */

		 if (left->sp_form==ft_omega)
		 	abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         map_root = target->sp_val.sp_map_ptr;
         if (map_root->m_use_count != 1) {

            map_root->m_use_count--;
            map_root = copy_map(SETL_SYSTEM map_root);
            target->sp_val.sp_map_ptr = map_root;

         }

         /* look up the domain element in the map */

         map_work_hdr = map_root;
         spec_hash_code(work_hash_code,right);
         range_hash_code = work_hash_code;
         spec_hash_code(work_hash_code,left);
         source_hash_code = work_hash_code;

         for (source_height = map_work_hdr->m_ntype.m_root.m_height;
              source_height;
              source_height--) {

            /* extract the element's index at this level */

            source_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

            /* if we're missing a header record, insert it */

            if (map_work_hdr->m_child[source_index].m_header == NULL) {

               get_map_header(new_map_hdr);
               new_map_hdr->m_ntype.m_intern.m_parent = map_work_hdr;
               new_map_hdr->m_ntype.m_intern.m_child_index = source_index;
               for (i = 0;
                    i < MAP_HASH_SIZE;
                    new_map_hdr->m_child[i++].m_cell = NULL);
               map_work_hdr->m_child[source_index].m_header = new_map_hdr;
               map_work_hdr = new_map_hdr;

            }
            else {

               map_work_hdr = map_work_hdr->m_child[source_index].m_header;

            }
         }

         /*
          *  At this point, map_work_hdr points to the lowest level header
          *  record.  We look for an element.
          */

         source_index = work_hash_code & MAP_HASH_MASK;
         map_cell_tail = &(map_work_hdr->m_child[source_index].m_cell);
         for (map_cell = *map_cell_tail;
              map_cell != NULL &&
                 map_cell->m_hash_code < source_hash_code;
              map_cell = map_cell->m_next) {

            map_cell_tail = &(map_cell->m_next);

         }

         /* try to find the domain element */

         is_equal = NO;
         while (map_cell != NULL &&
                map_cell->m_hash_code == source_hash_code) {

            spec_equal(is_equal,&(map_cell->m_domain_spec),left);

            if (is_equal)
               break;

            map_cell_tail = &(map_cell->m_next);
            map_cell = map_cell->m_next;

         }

         /* make sure we have a set to insert */

         if (right->sp_form == ft_map)
            map_to_set(SETL_SYSTEM right,right);

         if (right->sp_form != ft_set)
            abend(SETL_SYSTEM msg_expected_set,
                  abend_opnd_str(SETL_SYSTEM right));

         /* if we have an empty set, remove the domain element */

         set_root = right->sp_val.sp_set_ptr;
         if (set_root->s_ntype.s_root.s_cardinality == 0) {

            if (!is_equal)
               break;

            spec_hash_code(work_hash_code,&(map_cell->m_range_spec));
            map_root->m_hash_code ^= work_hash_code;

            if (map_cell->m_is_multi_val) {

               set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
               map_root->m_ntype.m_root.m_cardinality -=
                     set_root->s_ntype.s_root.s_cardinality;
               if ((set_root->s_ntype.s_root.s_cardinality%2)==0) 
                  map_root->m_hash_code ^= map_cell->m_hash_code;

            }
            else {

               map_root->m_ntype.m_root.m_cardinality--;

            }

            map_root->m_ntype.m_root.m_cell_count--;
            map_root->m_hash_code ^= map_cell->m_hash_code;
            *map_cell_tail = map_cell->m_next;
            unmark_specifier(&(map_cell->m_domain_spec));
            unmark_specifier(&(map_cell->m_range_spec));
            free_map_cell(map_cell);

            /*
             *  We've reduced the number of cells, so we may have to
             *  contract the map header.
             */

            contraction_trigger = (1 << ((map_root->m_ntype.m_root.m_height)
                                         * MAP_SHIFT_DIST));
            if (contraction_trigger == 1)
               contraction_trigger = 0;

            if (map_root->m_ntype.m_root.m_cell_count < contraction_trigger) {

               map_root = map_contract_header(SETL_SYSTEM map_root);
               target->sp_val.sp_map_ptr = map_root;

            }

            break;

         }

         /*
          *  The source is not empty, so we add or change a cell.
          */

         /* if we don't find the domain element, add a cell */

         if (!is_equal) {

            get_map_cell(new_map_cell);
            mark_specifier(left);
            new_map_cell->m_domain_spec.sp_form = left->sp_form;
            new_map_cell->m_domain_spec.sp_val.sp_biggest =
                  left->sp_val.sp_biggest;
            new_map_cell->m_range_spec.sp_form = ft_omega;
            new_map_cell->m_hash_code = source_hash_code;
            new_map_cell->m_next = map_cell;
            *map_cell_tail = new_map_cell;
            map_root->m_ntype.m_root.m_cell_count++;
            map_root->m_hash_code ^= source_hash_code;
            map_cell = new_map_cell;

            /* expand the map header if necessary */

            expansion_trigger = (1 << ((map_root->m_ntype.m_root.m_height + 1)
                                       * MAP_SHIFT_DIST)) * 2;
            if (map_root->m_ntype.m_root.m_cell_count >
                expansion_trigger) {

               map_root = map_expand_header(SETL_SYSTEM map_root);
               target->sp_val.sp_map_ptr = map_root;

            }
         }

         /* if we had a multi-value cell, decrease the cardinality */

         else {

            spec_hash_code(work_hash_code,&(map_cell->m_range_spec));
            map_root->m_hash_code ^= work_hash_code;

            if (map_cell->m_is_multi_val) {

               set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
               map_root->m_ntype.m_root.m_cardinality -=
                     set_root->s_ntype.s_root.s_cardinality;

               if ((set_root->s_ntype.s_root.s_cardinality%2)==0) 
                  map_root->m_hash_code ^= map_cell->m_hash_code;

            }
            else {

               map_root->m_ntype.m_root.m_cardinality--;

            }
         }

         /*
          *  Now we're set up to modify the range value.  If we have a
          *  singleton set, we pick out the element.
          */

         set_root = right->sp_val.sp_set_ptr;
         if (set_root->s_ntype.s_root.s_cardinality == 1) {

            set_work_hdr = set_root;
            source_height = set_root->s_ntype.s_root.s_height;
            source_index = 0;
            set_cell = NULL;

            for (;;) {

               /* if we're at a leaf, look for the cell */

               if (!source_height) {

                  for (source_index = 0;
                       source_index < SET_HASH_SIZE && set_cell == NULL;
                       source_index++) {

                     set_cell = set_work_hdr->s_child[source_index].s_cell;

                  }

                  if (set_cell != NULL)
                     break;

               }

               /* move up if we're at the end of a node */

               if (source_index >= SET_HASH_SIZE) {

                  /* if we return to the root the set is exhausted */

#ifdef TRAPS

                  if (set_work_hdr == set_root)
                     trap(__FILE__,__LINE__,msg_missing_set_element);

#endif

                  source_height++;
                  source_index =
                      set_work_hdr->s_ntype.s_intern.s_child_index + 1;
                  set_work_hdr = set_work_hdr->s_ntype.s_intern.s_parent;

                  continue;

               }

               /* skip over null nodes */

               if (set_work_hdr->s_child[source_index].s_header == NULL) {

                  source_index++;
                  continue;

               }

               /* otherwise drop down a level */

               set_work_hdr = set_work_hdr->s_child[source_index].s_header;
               source_index = 0;
               source_height--;

            }

            /* now we have the set element in set_cell */

            mark_specifier(&(set_cell->s_spec));
            unmark_specifier(&(map_cell->m_range_spec));
            map_cell->m_range_spec.sp_form = set_cell->s_spec.sp_form;
            map_cell->m_range_spec.sp_val.sp_biggest =
               set_cell->s_spec.sp_val.sp_biggest;
            map_root->m_ntype.m_root.m_cardinality++;
            map_cell->m_is_multi_val = NO;

            map_root->m_hash_code ^= range_hash_code;

            break;

         }

         set_root->s_use_count++;
         unmark_specifier(&(map_cell->m_range_spec));
         map_cell->m_range_spec.sp_form = right->sp_form;
         map_cell->m_range_spec.sp_val.sp_set_ptr = set_root;
         map_cell->m_is_multi_val = YES;
         map_root->m_ntype.m_root.m_cardinality +=
               set_root->s_ntype.s_root.s_cardinality;

         if ((set_root->s_ntype.s_root.s_cardinality%2)==0) 
            map_root->m_hash_code ^= map_cell->m_hash_code;

         map_root->m_hash_code ^= range_hash_code;

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = target->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_sofa;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"F{X} :=",
                  class_ptr->ut_name);

         /* we push our arguments on to the stack */

         push_pstack(left);
         push_pstack(right);

         /* call the procedure */

         call_procedure(SETL_SYSTEM NULL,
                        slot_info->si_spec,
                        target,
                        2L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         abend(SETL_SYSTEM msg_bad_unop_form,"F{X} :=",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{assign\_ops3()}
 *
 *  This function handles assignment operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void assign_ops3(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_sslice}{string or tuple slice assignment}
 *        {specifier}{target string or tuple}
 *        {specifier}{beginning of slice}
 *        {specifier}{end}
 *
 *  This case handles slice assignment.  We need four operands for this
 *  instruction, so the source operand is the first operand of the
 *  following instruction, which will be a noop.
\*/

case p_sslice :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   end = ip->i_operand[2].i_spec_ptr;
   right = PC->i_operand[0].i_spec_ptr;
   BUMPPC(1);

   switch (target->sp_form) {

      case ft_string :

         /* we convert the slice front to a C long */

         if (left->sp_form == ft_short) {

            slice_start = left->sp_val.sp_short_value;
            if (slice_start <= 0)
               slice_start = target->sp_val.sp_string_ptr->s_length +
                             slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else if (left->sp_form == ft_long) {

            slice_start = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
            if (slice_start <= 0)
               slice_start = target->sp_val.sp_string_ptr->s_length +
                             slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }

         /* we convert the slice end to a C long */

         if (end->sp_form == ft_short) {

            slice_end = end->sp_val.sp_short_value;
            if (slice_end < 0)
               slice_end = target->sp_val.sp_string_ptr->s_length +
                           slice_end + 1;
            if (slice_end < 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM end));

         }
         else if (end->sp_form == ft_long) {

            slice_end = long_to_short(SETL_SYSTEM end->sp_val.sp_long_ptr);
            if (slice_end < 0)
               slice_end = target->sp_val.sp_string_ptr->s_length +
                           slice_end + 1;
            if (slice_end < 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM end));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM end));

         }

         /* make sure start <= end + 1 */

         if (slice_start > slice_end + 1)
            abend(SETL_SYSTEM msg_invalid_slice_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right),
                  abend_opnd_str(SETL_SYSTEM end));

         /* the source must also be a string */

         if (right->sp_form != ft_string)
            abend(SETL_SYSTEM "Expected string, but found %s",
                  abend_opnd_str(SETL_SYSTEM right));

         /* if we reference past the end of the string, abend */

         left_string_hdr = target->sp_val.sp_string_ptr;
         if (left_string_hdr->s_length < slice_end)
            abend(SETL_SYSTEM msg_invalid_slice_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right),
                  abend_opnd_str(SETL_SYSTEM end));

         /* make a target string */

         get_string_header(target_string_hdr);
         target_string_hdr->s_use_count = 1;
         target_string_hdr->s_hash_code = -1;
         target_string_hdr->s_length = 0;
         target_string_hdr->s_head = target_string_hdr->s_tail = NULL;
         target_char_ptr = target_char_end = NULL;

         /* copy the target operand up to the beginning of the slice */

         left_string_length = left_string_hdr->s_length - slice_end;
         left_string_cell = left_string_hdr->s_head;

         if (left_string_cell == NULL) {

            left_char_ptr = left_char_end = NULL;

         }
         else {

            left_char_ptr = left_string_cell->s_cell_value;
            left_char_end = left_char_ptr + STR_CELL_WIDTH;

         }

         slice_start--;
         target_string_hdr->s_length = slice_start;
         slice_end -= slice_start;

         while (slice_start--) {

            if (left_char_ptr == left_char_end) {

               left_string_cell = left_string_cell->s_next;
               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            if (target_char_ptr == target_char_end) {

               get_string_cell(target_string_cell);
               if (target_string_hdr->s_tail != NULL)
                  (target_string_hdr->s_tail)->s_next = target_string_cell;
               target_string_cell->s_prev = target_string_hdr->s_tail;
               target_string_cell->s_next = NULL;
               target_string_hdr->s_tail = target_string_cell;
               if (target_string_hdr->s_head == NULL)
                  target_string_hdr->s_head = target_string_cell;
               target_char_ptr = target_string_cell->s_cell_value;
               target_char_end = target_char_ptr + STR_CELL_WIDTH;

            }

            *target_char_ptr++ = *left_char_ptr++;

         }

         /* next copy the source operand */

         right_string_hdr = right->sp_val.sp_string_ptr;
         right_string_cell = right_string_hdr->s_head;

         if (right_string_cell == NULL) {

            right_char_ptr = right_char_end = NULL;

         }
         else {

            right_char_ptr = right_string_cell->s_cell_value;
            right_char_end = right_char_ptr + STR_CELL_WIDTH;

         }

         right_string_length = right_string_hdr->s_length;
         target_string_hdr->s_length += right_string_length;

         while (right_string_length--) {

            if (right_char_ptr == right_char_end) {

               right_string_cell = right_string_cell->s_next;
               right_char_ptr = right_string_cell->s_cell_value;
               right_char_end = right_char_ptr + STR_CELL_WIDTH;

            }

            if (target_char_ptr == target_char_end) {

               get_string_cell(target_string_cell);
               if (target_string_hdr->s_tail != NULL)
                  (target_string_hdr->s_tail)->s_next = target_string_cell;
               target_string_cell->s_prev = target_string_hdr->s_tail;
               target_string_cell->s_next = NULL;
               target_string_hdr->s_tail = target_string_cell;
               if (target_string_hdr->s_head == NULL)
                  target_string_hdr->s_head = target_string_cell;
               target_char_ptr = target_string_cell->s_cell_value;
               target_char_end = target_char_ptr + STR_CELL_WIDTH;

            }

            *target_char_ptr++ = *right_char_ptr++;

         }

         /* position the left operand past the slice */

         slice_end += (left_char_ptr - left_string_cell->s_cell_value);
         while (slice_end >= STR_CELL_WIDTH) {

            left_string_cell = left_string_cell->s_next;
            slice_end -= STR_CELL_WIDTH;

         }

         /* copy the rest of the left string */

         target_string_hdr->s_length += left_string_length;

         if (left_string_cell == NULL) {

            left_char_ptr = left_char_end = NULL;

         }
         else {

            left_char_ptr = left_string_cell->s_cell_value + slice_end;
            left_char_end = left_string_cell->s_cell_value + STR_CELL_WIDTH;

         }

         while (left_string_length--) {

            if (left_char_ptr == left_char_end) {

               left_string_cell = left_string_cell->s_next;
               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            if (target_char_ptr == target_char_end) {

               get_string_cell(target_string_cell);
               if (target_string_hdr->s_tail != NULL)
                  (target_string_hdr->s_tail)->s_next = target_string_cell;
               target_string_cell->s_prev = target_string_hdr->s_tail;
               target_string_cell->s_next = NULL;
               target_string_hdr->s_tail = target_string_cell;
               if (target_string_hdr->s_head == NULL)
                  target_string_hdr->s_head = target_string_cell;
               target_char_ptr = target_string_cell->s_cell_value;
               target_char_end = target_char_ptr + STR_CELL_WIDTH;

            }

            *target_char_ptr++ = *left_char_ptr++;

         }

         /* assign our result to the target */

         unmark_specifier(target);
         target->sp_form = ft_string;
         target->sp_val.sp_string_ptr = target_string_hdr;

         break;

      /*
       *  We produce tuple slices in the tuple package, and hope that it
       *  will not be used much.
       */

      case ft_tuple :

         /* we convert the slice front to a C long */

         if (left->sp_form == ft_short) {

            slice_start = left->sp_val.sp_short_value;
            if (slice_start <= 0)
               slice_start =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else if (left->sp_form == ft_long) {

            slice_start = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
            if (slice_start <= 0)
               slice_start =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }

         /* we convert the slice end to a C long */

         if (end->sp_form == ft_short) {

            slice_end = end->sp_val.sp_short_value;
            if (slice_end < 0)
               slice_end =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_end + 1;
            if (slice_end < 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM end));

         }
         else if (end->sp_form == ft_long) {

            slice_end = long_to_short(SETL_SYSTEM end->sp_val.sp_long_ptr);
            if (slice_end < 0)
               slice_end =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_end + 1;
            if (slice_end < 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM end));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM end));

         }

         /* make sure start <= end + 1 */

         if (slice_start > slice_end + 1)
            abend(SETL_SYSTEM msg_invalid_slice_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right),
                  abend_opnd_str(SETL_SYSTEM end));

         /* the source must also be a tuple */

         if (right->sp_form != ft_tuple)
            abend(SETL_SYSTEM "Expected tuple, but found %s",
                  abend_opnd_str(SETL_SYSTEM right));

         tuple_work_hdr = target->sp_val.sp_tuple_ptr;

         /* make sure end <= length */

         if (slice_end > tuple_work_hdr->t_ntype.t_root.t_length)
            abend(SETL_SYSTEM msg_invalid_slice_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right),
                  abend_opnd_str(SETL_SYSTEM end));

         tuple_sslice(SETL_SYSTEM target,right,slice_start,slice_end);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = target->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_sslice;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"F(I..J) :=",
                  class_ptr->ut_name);

         /* we push our arguments on to the stack */

         push_pstack(left);
         push_pstack(end);
         push_pstack(right);

         /* call the procedure */

         call_procedure(SETL_SYSTEM NULL,
                        slot_info->si_spec,
                        target,
                        3L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

        abend(SETL_SYSTEM msg_bad_unop_form,"F(I..J) :=",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{assign\_ops4()}
 *
 *  This function handles assignment operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void assign_ops4(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_send}{string or tuple tail}
 *        {specifier}{target string or tuple}
 *        {specifier}{beginning of slice}
 *        {specifier}{source}
 *
 *  This case handles tail slice assignment.
\*/

case p_send :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   switch (target->sp_form) {

      case ft_string :

         /* we convert the slice front to a C long */

         if (left->sp_form == ft_short) {

            slice_start = left->sp_val.sp_short_value;
            if (slice_start <= 0)
               slice_start = target->sp_val.sp_string_ptr->s_length +
                             slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else if (left->sp_form == ft_long) {

            slice_start = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
            if (slice_start <= 0)
               slice_start = target->sp_val.sp_string_ptr->s_length +
                             slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }

         left_string_hdr = target->sp_val.sp_string_ptr;
         slice_end = left_string_hdr->s_length;

         /* make sure start <= end + 1 */

         if (slice_start > slice_end + 1)
            abend(SETL_SYSTEM msg_invalid_tail_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         /* make a target string */

         get_string_header(target_string_hdr);
         target_string_hdr->s_use_count = 1;
         target_string_hdr->s_hash_code = -1;
         target_string_hdr->s_length = 0;
         target_string_hdr->s_head = target_string_hdr->s_tail = NULL;
         target_char_ptr = target_char_end = NULL;

         /* copy the target operand up to the beginning of the slice */

         left_string_hdr = target->sp_val.sp_string_ptr;
         left_string_length = left_string_hdr->s_length - slice_end;
         left_string_cell = left_string_hdr->s_head;

         if (left_string_cell == NULL) {

            left_char_ptr = left_char_end = NULL;

         }
         else {

            left_char_ptr = left_string_cell->s_cell_value;
            left_char_end = left_char_ptr + STR_CELL_WIDTH;

         }

         slice_start--;
         target_string_hdr->s_length = slice_start;
         slice_end -= slice_start;

         while (slice_start--) {

            if (left_char_ptr == left_char_end) {

               left_string_cell = left_string_cell->s_next;
               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            if (target_char_ptr == target_char_end) {

               get_string_cell(target_string_cell);
               if (target_string_hdr->s_tail != NULL)
                  (target_string_hdr->s_tail)->s_next = target_string_cell;
               target_string_cell->s_prev = target_string_hdr->s_tail;
               target_string_cell->s_next = NULL;
               target_string_hdr->s_tail = target_string_cell;
               if (target_string_hdr->s_head == NULL)
                  target_string_hdr->s_head = target_string_cell;
               target_char_ptr = target_string_cell->s_cell_value;
               target_char_end = target_char_ptr + STR_CELL_WIDTH;

            }

            *target_char_ptr++ = *left_char_ptr++;

         }

         /* next copy the source operand */

         right_string_hdr = right->sp_val.sp_string_ptr;
         right_string_cell = right_string_hdr->s_head;

         if (right_string_cell == NULL) {

            right_char_ptr = right_char_end = NULL;

         }
         else {

            right_char_ptr = right_string_cell->s_cell_value;
            right_char_end = right_char_ptr + STR_CELL_WIDTH;

         }

         right_string_length = right_string_hdr->s_length;
         target_string_hdr->s_length += right_string_length;

         while (right_string_length--) {

            if (right_char_ptr == right_char_end) {

               right_string_cell = right_string_cell->s_next;
               right_char_ptr = right_string_cell->s_cell_value;
               right_char_end = right_char_ptr + STR_CELL_WIDTH;

            }

            if (target_char_ptr == target_char_end) {

               get_string_cell(target_string_cell);
               if (target_string_hdr->s_tail != NULL)
                  (target_string_hdr->s_tail)->s_next = target_string_cell;
               target_string_cell->s_prev = target_string_hdr->s_tail;
               target_string_cell->s_next = NULL;
               target_string_hdr->s_tail = target_string_cell;
               if (target_string_hdr->s_head == NULL)
                  target_string_hdr->s_head = target_string_cell;
               target_char_ptr = target_string_cell->s_cell_value;
               target_char_end = target_char_ptr + STR_CELL_WIDTH;

            }

            *target_char_ptr++ = *right_char_ptr++;

         }

         /* assign our result to the target */

         unmark_specifier(target);
         target->sp_form = ft_string;
         target->sp_val.sp_string_ptr = target_string_hdr;

         break;

      /*
       *  We produce tuple slices in the tuple package, and hope that it
       *  will not be used much.
       */

      case ft_tuple :

         /* we convert the slice front to a C long */

         if (left->sp_form == ft_short) {

            slice_start = left->sp_val.sp_short_value;
            if (slice_start <= 0)
               slice_start =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else if (left->sp_form == ft_long) {

            slice_start = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
            if (slice_start <= 0)
               slice_start =
                  target->sp_val.sp_tuple_ptr->t_ntype.t_root.t_length +
                  slice_start + 1;
            if (slice_start <= 0)
               abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }
         else {

            abend(SETL_SYSTEM msg_invalid_index,abend_opnd_str(SETL_SYSTEM left));

         }

         tuple_work_hdr = target->sp_val.sp_tuple_ptr;
         slice_end = tuple_work_hdr->t_ntype.t_root.t_length;

         /* make sure start <= end + 1 */

         if (slice_start > slice_end + 1)
            abend(SETL_SYSTEM msg_invalid_tail_limits,
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         tuple_sslice(SETL_SYSTEM target,right,slice_start,slice_end);

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = target->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_send;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"F(I..) :=",
                  class_ptr->ut_name);

         /* we push our arguments on to the stack */

         push_pstack(left);
         push_pstack(right);

         /* call the procedure */

         call_procedure(SETL_SYSTEM NULL,
                        slot_info->si_spec,
                        target,
                        2L,NO,YES,0);

         break;

      /*
       *  Anything else is a run-time error.
       */

      default :

         abend(SETL_SYSTEM msg_bad_unop_form,"F(I..) :=",
               abend_opnd_str(SETL_SYSTEM left));

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{cond\_ops1()}
 *
 *  This function handles condition operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void cond_ops1(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_eq}{equality tests}
 *        {specifier}{target operand}
 *        {specifier}{left operand}
 *        {specifier}{right operand}
 *
 *  This case handles a variety of operands all of which depend on an
 *  equality test.  First we determine whether the two operands are
 *  equal, then we perform some operation dependent upon the opcode.
\*/

case p_goeq :           case p_gone :           case p_eq :
case p_ne :

{

   /* pick out the operand pointers, to save some expression evaluations */

   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;


   spec_equal(condition_true,left,right);

   /*
    *  At this point condition_true is either YES or NO.  The action we
    *  take depends on the opcode.
    */

   switch (ip->i_opcode) {

      case p_goeq :

         if (condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_gone :

         if (!condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_eq :

         target = ip->i_operand[0].i_spec_ptr;

         if (condition_true) {

            unmark_specifier(target);
            target->sp_form = spec_true->sp_form;
            target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = spec_false->sp_form;
            target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

         }

         break;

      case p_ne :

         target = ip->i_operand[0].i_spec_ptr;

         if (!condition_true) {

            unmark_specifier(target);
            target->sp_form = spec_true->sp_form;
            target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = spec_false->sp_form;
            target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

         }

         break;

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

#endif                                 /* PARTE                             */
#if !SHORT_TEXT | PARTF

/*\
 *  \function{cond\_ops2()}
 *
 *  This function handles condition operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void cond_ops2(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_lt}{inequality tests}
 *        {instruction}{target operand}
 *        {specifier}{left operand}
 *        {specifier}{right operand}
 *
 *  This case handles a variety of operands all of which depend on a less
 *  than test.  First we determine whether the less than condition holds,
 *  then we perform some operation dependant upon the opcode.
\*/

case p_lt :             case p_nlt :            case p_golt :
case p_gonlt :

{

   /*
    *  We will set condition_true to YES if the left operand is less than
    *  the right.
    */

   condition_true = NO;

   /* pick out the operand pointers, to save some expression evaluations */

   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can compare short integers with other short integers and long
       *  integers.
       */

      case ft_short :

         /*
          *  If both operands are short we just compare the values.
          */

         if (right->sp_form == ft_short) {

            condition_true =
               (left->sp_val.sp_short_value < right->sp_val.sp_short_value);

            break;

         } else if (right->sp_form == ft_real) {
			
			condition_true = 
               (left->sp_val.sp_short_value < (right->sp_val.sp_real_ptr)->r_value);

            break;

         }

         /*
          *  We use a general function which compares integers to handle
          *  the case in which the second operand is long.
          */

         else if (right->sp_form == ft_long) {

            condition_true = integer_lt(SETL_SYSTEM left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            spare1.sp_form = ft_omega;
            call_binop_method(SETL_SYSTEM &spare1,
                              right,
                              left,
                              m_lt_r,
                              "<",3);

            goto jump2_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare integers with short integers and other
       *  long integers.
       */

      case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which adds any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            condition_true = integer_lt(SETL_SYSTEM left,right);

            break;

         
         } else if (right->sp_form == ft_real) {
			
            condition_true = ((double)(long_to_double(SETL_SYSTEM left)) <
            			 (right->sp_val.sp_real_ptr)->r_value);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            spare1.sp_form = ft_omega;
            call_binop_method(SETL_SYSTEM &spare1,
                              right,
                              left,
                              m_lt_r,
                              "<",3);

            goto jump2_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare reals with other reals.
       */

      case ft_real :

         if (right->sp_form == ft_real) {

            condition_true = ((left->sp_val.sp_real_ptr)->r_value <
                              (right->sp_val.sp_real_ptr)->r_value);

         } else if (right->sp_form == ft_short) {
         
            condition_true = ((left->sp_val.sp_real_ptr)->r_value <
                              (double)(right->sp_val.sp_short_value));
		 } 
		 else if (right->sp_form == ft_long) {
		 
		    condition_true = ((left->sp_val.sp_real_ptr)->r_value <
                               (double)(long_to_double(SETL_SYSTEM right)));
                               
		 }


         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            spare1.sp_form = ft_omega;
            call_binop_method(SETL_SYSTEM &spare1,
                              right,
                              left,
                              m_lt_r,
                              "<",3);

            goto jump2_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare strings with other strings.
       */

      case ft_string :

         /* make sure the right operand is a string */

         if (right->sp_form == ft_string) {

            /* pick out the string headers */

            left_string_hdr = left->sp_val.sp_string_ptr;
            right_string_hdr = right->sp_val.sp_string_ptr;

            /* we just traverse the two strings until we find a mismatch */

            left_string_cell = left_string_hdr->s_head;
            right_string_cell = right_string_hdr->s_head;

            /* set up cell limit pointers */

            if (right_string_cell == NULL) {

               right_char_ptr = right_char_end = NULL;

            }
            else {

               right_char_ptr = right_string_cell->s_cell_value;
               right_char_end = right_char_ptr + STR_CELL_WIDTH;

            }

            if (left_string_cell == NULL) {

               left_char_ptr = left_char_end = NULL;

            }
            else {

               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            /* loop over the strings in parallel */

            target_string_length = min(left_string_hdr->s_length,
                                       right_string_hdr->s_length);

            while (target_string_length--) {

               if (left_char_ptr == left_char_end) {

                  left_string_cell = left_string_cell->s_next;
                  left_char_ptr = left_string_cell->s_cell_value;
                  left_char_end = left_char_ptr + STR_CELL_WIDTH;

               }

               if (right_char_ptr == right_char_end) {

                  right_string_cell = right_string_cell->s_next;
                  right_char_ptr = right_string_cell->s_cell_value;
                  right_char_end = right_char_ptr + STR_CELL_WIDTH;

               }

               /* if we find a mismatch, break */

               if (*left_char_ptr != *right_char_ptr)
                  break;

               left_char_ptr++;
               right_char_ptr++;

            }

            /* if we found a mismatch, compare characters */

            if (target_string_length >= 0) {

               condition_true = *left_char_ptr < *right_char_ptr;

            }

            /* otherwise the shorter string is less */

            else {

               condition_true =
                  left_string_hdr->s_length < right_string_hdr->s_length;

            }

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            spare1.sp_form = ft_omega;
            call_binop_method(SETL_SYSTEM &spare1,
                              right,
                              left,
                              m_lt_r,
                              "<",3);

            goto jump2_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  Less than for sets is subset.
       */

      case ft_map :

         map_to_set(SETL_SYSTEM left,left);

      case ft_set :

         if (right->sp_form == ft_map)
            map_to_set(SETL_SYSTEM right,right);

         if (right->sp_form == ft_set) {

            condition_true = (set_subset(SETL_SYSTEM left,right) &&
                  ((left->sp_val.sp_set_ptr)->s_ntype.s_root.s_cardinality <
                   (right->sp_val.sp_set_ptr)->s_ntype.s_root.s_cardinality));

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            spare1.sp_form = ft_omega;
            call_binop_method(SETL_SYSTEM &spare1,
                              right,
                              left,
                              m_lt_r,
                              "<",3);

            goto jump2_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         spare1.sp_form = ft_omega;
         call_binop_method(SETL_SYSTEM &spare1,
                           left,
                           right,
                           m_lt,
                           "<",3);

         goto jump2_end;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            spare1.sp_form = ft_omega;
            call_binop_method(SETL_SYSTEM &spare1,
                              right,
                              left,
                              m_lt_r,
                              "<",3);

            goto jump2_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   /*
    *  At this point condition_true is either YES or NO.  The action we
    *  take depends on the opcode.
    */

   switch (ip->i_opcode) {

      case p_golt :

         if (condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_gonlt :

         if (!condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_lt :

         target = ip->i_operand[0].i_spec_ptr;

         if (condition_true) {

            unmark_specifier(target);
            target->sp_form = spec_true->sp_form;
            target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = spec_false->sp_form;
            target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

         }

         break;

      case p_nlt :

         target = ip->i_operand[0].i_spec_ptr;

         if (!condition_true) {

            unmark_specifier(target);
            target->sp_form = spec_true->sp_form;
            target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = spec_false->sp_form;
            target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

         }

         break;

   }

   jump2_end:
   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{cond\_ops3()}
 *
 *  This function handles condition operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void cond_ops3(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_le}{inequality tests}
 *        {instruction}{target operand}
 *        {specifier}{left operand}
 *        {specifier}{right operand}
 *
 *  This case handles a variety of operands all of which depend on a less
 *  than or equal to test.  First we determine whether the less than
 *  condition holds, then we perform some operation dependant upon the
 *  opcode.
\*/

case p_le :             case p_nle :            case p_gole :
case p_gonle :

{

   /*
    *  We will set condition_true to YES if the left operand is less than
    *  or equal to the right.
    */

   condition_true = NO;

   /* pick out the operand pointers, to save some expression evaluations */

   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (left->sp_form) {

      /*
       *  We can compare short integers with other short integers and long
       *  integers.
       */

      case ft_short :

         /*
          *  If both operands are short we just compare the values.
          */

         if (right->sp_form == ft_short) {

            condition_true =
               (left->sp_val.sp_short_value <= right->sp_val.sp_short_value);

            break;

         } else if (right->sp_form == ft_real) {
			
			condition_true = 
               (left->sp_val.sp_short_value <= (right->sp_val.sp_real_ptr)->r_value);

            break;

         }

         /*
          *  We use a general function which compares integers to handle
          *  the case in which the second operand is long.
          */

         else if (right->sp_form == ft_long) {

            condition_true = integer_le(SETL_SYSTEM left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            object_root = right->sp_val.sp_object_ptr;
            class_ptr = object_root->o_ntype.o_root.o_class;

            /* get information about this slot in the object */

            slot_info = class_ptr->ut_slot_info + m_lt_r;

            /* make sure we can access this slot */

            if (!slot_info->si_in_class)
               abend(SETL_SYSTEM msg_missing_method,"<",
                     class_ptr->ut_name);

            /* accept equal operands */

            spec_equal(condition_true,left,right);
            if (condition_true)
               break;

            /* push the left operand */

            push_pstack(left);

            /* call the procedure */

            spare1.sp_form = ft_omega;
            call_procedure(SETL_SYSTEM &spare1,
                           slot_info->si_spec,
                           right,
                           1L,EXTRA,YES,3);

	    goto jump1_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare integers with short integers and other
       *  long integers.
       */

      case ft_long :

         /*
          *  If the right operand is either type of integer we call a
          *  general function which adds any type of integers.
          */

         if (right->sp_form == ft_short || right->sp_form == ft_long) {

            condition_true = integer_le(SETL_SYSTEM left,right);

            break;

         } else if (right->sp_form == ft_real) {
			
            condition_true = ((double)(long_to_double(SETL_SYSTEM left)) <=
            			 (right->sp_val.sp_real_ptr)->r_value);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            object_root = right->sp_val.sp_object_ptr;
            class_ptr = object_root->o_ntype.o_root.o_class;

            /* get information about this slot in the object */

            slot_info = class_ptr->ut_slot_info + m_lt_r;

            /* make sure we can access this slot */

            if (!slot_info->si_in_class)
               abend(SETL_SYSTEM msg_missing_method,"<",
                     class_ptr->ut_name);

            /* accept equal operands */

            spec_equal(condition_true,left,right);
            if (condition_true)
               break;

            /* push the left operand */

            push_pstack(left);

            /* call the procedure */

            spare1.sp_form = ft_omega;
            call_procedure(SETL_SYSTEM &spare1,
                           slot_info->si_spec,
                           right,
                           1L,EXTRA,YES,3);

	    goto jump1_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare reals with other reals.
       */

      case ft_real :

         if (right->sp_form == ft_real) {

            condition_true = ((left->sp_val.sp_real_ptr)->r_value <=
                              (right->sp_val.sp_real_ptr)->r_value);

         } else if (right->sp_form == ft_short) {
         
            condition_true = ((left->sp_val.sp_real_ptr)->r_value <=
                              (double)(right->sp_val.sp_short_value));
		 } 
		 else if (right->sp_form == ft_long) {
		 
		    condition_true = ((left->sp_val.sp_real_ptr)->r_value <=
                               (double)(long_to_double(SETL_SYSTEM right)));
                               
		 }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            object_root = right->sp_val.sp_object_ptr;
            class_ptr = object_root->o_ntype.o_root.o_class;

            /* get information about this slot in the object */

            slot_info = class_ptr->ut_slot_info + m_lt_r;

            /* make sure we can access this slot */

            if (!slot_info->si_in_class)
               abend(SETL_SYSTEM msg_missing_method,"<",
                     class_ptr->ut_name);

            /* accept equal operands */

            spec_equal(condition_true,left,right);
            if (condition_true)
               break;

            /* push the left operand */

            push_pstack(left);

            /* call the procedure */

            spare1.sp_form = ft_omega;
            call_procedure(SETL_SYSTEM &spare1,
                           slot_info->si_spec,
                           right,
                           1L,EXTRA,YES,3);

	    goto jump1_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can compare strings with other strings.
       */

      case ft_string :

         /* make sure the right operand is a string */

         if (right->sp_form == ft_string) {

            /* pick out the string headers */

            left_string_hdr = left->sp_val.sp_string_ptr;
            right_string_hdr = right->sp_val.sp_string_ptr;

            /* we just traverse the two strings until we find a mismatch */

            left_string_cell = left_string_hdr->s_head;
            right_string_cell = right_string_hdr->s_head;

            /* set up cell limit pointers */

            if (right_string_cell == NULL) {

               right_char_ptr = right_char_end = NULL;

            }
            else {

               right_char_ptr = right_string_cell->s_cell_value;
               right_char_end = right_char_ptr + STR_CELL_WIDTH;

            }

            if (left_string_cell == NULL) {

               left_char_ptr = left_char_end = NULL;

            }
            else {

               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            /* loop over the strings in parallel */

            target_string_length = min(left_string_hdr->s_length,
                                       right_string_hdr->s_length);

            while (target_string_length--) {

               if (left_char_ptr == left_char_end) {

                  left_string_cell = left_string_cell->s_next;
                  left_char_ptr = left_string_cell->s_cell_value;
                  left_char_end = left_char_ptr + STR_CELL_WIDTH;

               }

               if (right_char_ptr == right_char_end) {

                  right_string_cell = right_string_cell->s_next;
                  right_char_ptr = right_string_cell->s_cell_value;
                  right_char_end = right_char_ptr + STR_CELL_WIDTH;

               }

               /* if we find a mismatch, break */

               if (*left_char_ptr != *right_char_ptr)
                  break;

               left_char_ptr++;
               right_char_ptr++;

            }

            /* if we found a mismatch, compare characters */

            if (target_string_length >= 0) {

               condition_true = *left_char_ptr < *right_char_ptr;

            }

            /* otherwise the shorter string is less */

            else {

               condition_true =
                  left_string_hdr->s_length <= right_string_hdr->s_length;

            }

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            object_root = right->sp_val.sp_object_ptr;
            class_ptr = object_root->o_ntype.o_root.o_class;

            /* get information about this slot in the object */

            slot_info = class_ptr->ut_slot_info + m_lt_r;

            /* make sure we can access this slot */

            if (!slot_info->si_in_class)
               abend(SETL_SYSTEM msg_missing_method,"<",
                     class_ptr->ut_name);

            /* accept equal operands */

            spec_equal(condition_true,left,right);
            if (condition_true)
               break;

            /* push the left operand */

            push_pstack(left);

            /* call the procedure */

            spare1.sp_form = ft_omega;
            call_procedure(SETL_SYSTEM &spare1,
                           slot_info->si_spec,
                           right,
                           1L,EXTRA,YES,3);

            goto jump1_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  Less than for sets is subset.
       */

      case ft_map :

         map_to_set(SETL_SYSTEM left,left);

      case ft_set :

         if (right->sp_form == ft_map)
            map_to_set(SETL_SYSTEM right,right);

         if (right->sp_form == ft_set) {

            condition_true = set_subset(SETL_SYSTEM left,right);

            break;

         }

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         else if (right->sp_form == ft_object) {

            object_root = right->sp_val.sp_object_ptr;
            class_ptr = object_root->o_ntype.o_root.o_class;

            /* get information about this slot in the object */

            slot_info = class_ptr->ut_slot_info + m_lt_r;

            /* make sure we can access this slot */

            if (!slot_info->si_in_class)
               abend(SETL_SYSTEM msg_missing_method,"<",
                     class_ptr->ut_name);

            /* accept equal operands */

            spec_equal(condition_true,left,right);
            if (condition_true)
               break;

            /* push the left operand */

            push_pstack(left);

            /* call the procedure */

            spare1.sp_form = ft_omega;
            call_procedure(SETL_SYSTEM &spare1,
                           slot_info->si_spec,
                           right,
                           1L,EXTRA,YES,3);

            goto jump1_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = left->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_lt;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"<",
                  class_ptr->ut_name);

         /* accept equal operands */

         spec_equal(condition_true,left,right);
         if (condition_true)
            break;

         /* push the right operand */

         push_pstack(right);

         /* call the procedure */

         spare1.sp_form = ft_omega;
         call_procedure(SETL_SYSTEM &spare1,
                        slot_info->si_spec,
                        left,
                        1L,EXTRA,YES,3);

         goto jump1_end;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the right.
          */

         if (right->sp_form == ft_object) {

            object_root = right->sp_val.sp_object_ptr;
            class_ptr = object_root->o_ntype.o_root.o_class;

            /* get information about this slot in the object */

            slot_info = class_ptr->ut_slot_info + m_lt_r;

            /* make sure we can access this slot */

            if (!slot_info->si_in_class)
               abend(SETL_SYSTEM msg_missing_method,"<",
                     class_ptr->ut_name);

            /* accept equal operands */

            spec_equal(condition_true,left,right);
            if (condition_true)
               break;

            /* push the left operand */

            push_pstack(left);

            /* call the procedure */

            spare1.sp_form = ft_omega;
            call_procedure(SETL_SYSTEM &spare1,
                           slot_info->si_spec,
                           right,
                           1L,EXTRA,YES,3);
	    goto jump1_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"<",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   /*
    *  At this point condition_true is either YES or NO.  The action we
    *  take depends on the opcode.
    */

   switch (ip->i_opcode) {

      case p_gole :

         if (condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_gonle :

         if (!condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_le :

         target = ip->i_operand[0].i_spec_ptr;

         if (condition_true) {

            unmark_specifier(target);
            target->sp_form = spec_true->sp_form;
            target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = spec_false->sp_form;
            target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

         }

         break;

      case p_nle :

         target = ip->i_operand[0].i_spec_ptr;

         if (!condition_true) {

            unmark_specifier(target);
            target->sp_form = spec_true->sp_form;
            target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = spec_false->sp_form;
            target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

         }

         break;

   }
   jump1_end:
   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

#endif                                 /* PARTF                             */
#if !SHORT_TEXT | PARTG

/*\
 *  \function{cond\_ops4()}
 *
 *  This function handles condition operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void cond_ops4(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_in}{element operations}
 *        {specifier}{target operand}
 *        {specifier}{left operand}
 *        {specifier}{right operand}
 *
 *  This case handles operations dependent on the \verb"IN" and
 *  \verb"NOTIN" operations.
\*/

case p_goin :           case p_gonotin :        case p_in :
case p_notin :

{

   /* pick out the operand pointers, to save some expression evaluations */

   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* the action we take depends on the form of the operands */

   switch (right->sp_form) {

      /*
       *  This is a completely simple, stupid substring test.  It needs to
       *  be improved.
       */

      case ft_string :

         /* make sure the right operand is a string */

         if (left->sp_form == ft_string) {

            /* pick out the string headers */

            left_string_hdr = left->sp_val.sp_string_ptr;
            right_string_hdr = right->sp_val.sp_string_ptr;

            /* if the left string is null, return TRUE */

            if (left_string_hdr->s_length == 0) {

               condition_true = YES;
               break;

            }

            /* if the left string is longer, return FALSE */

            if (left_string_hdr->s_length > right_string_hdr->s_length) {

               condition_true = NO;
               break;

            }

            /* we traverse the right string searching for the left */

            right_string_cell = right_string_hdr->s_head;
            left_string_cell = left_string_hdr->s_head;

            /* set up cell limit pointers */

            if (right_string_cell == NULL) {

               right_char_ptr = right_char_end = NULL;

            }
            else {

               right_char_ptr = right_string_cell->s_cell_value;
               right_char_end = right_char_ptr + STR_CELL_WIDTH;

            }

            left_char_ptr = left_string_cell->s_cell_value;
            left_char_end = left_char_ptr + STR_CELL_WIDTH;

            condition_true = NO;
            right_string_length = right_string_hdr->s_length -
                                  left_string_hdr->s_length + 1;

            while (right_string_length--) {

               if (right_char_ptr == right_char_end) {

                  right_string_cell = right_string_cell->s_next;
                  right_char_ptr = right_string_cell->s_cell_value;
                  right_char_end = right_char_ptr + STR_CELL_WIDTH;

               }

               if (*left_char_ptr != *right_char_ptr) {

                  right_char_ptr++;
                  continue;

               }

               /*
                *  We found the first character of the target string.  We
                *  compare the right string with the left.
                */

               left_string_length = left_string_hdr->s_length;
               target_string_cell = right_string_cell;
               target_char_ptr = right_char_ptr;
               target_char_end = right_char_end;
               right_char_ptr++;

               while (left_string_length--) {

                  if (left_char_ptr == left_char_end) {

                     left_string_cell = left_string_cell->s_next;
                     left_char_ptr = left_string_cell->s_cell_value;
                     left_char_end = left_char_ptr + STR_CELL_WIDTH;

                  }

                  if (target_char_ptr == target_char_end) {

                     target_string_cell = target_string_cell->s_next;
                     target_char_ptr = target_string_cell->s_cell_value;
                     target_char_end = target_char_ptr + STR_CELL_WIDTH;

                  }

                  if (*target_char_ptr != *left_char_ptr)
                     break;

                  left_char_ptr++;
                  target_char_ptr++;

               }

               /* if we fell off the end of left string we found the target */

               if (left_string_length < 0) {

                  condition_true = YES;
                  break;

               }

               /* we didn't match -- reset the left string */

               left_string_cell = left_string_hdr->s_head;

               /* set up cell limit pointers */

               left_char_ptr = left_string_cell->s_cell_value;
               left_char_end = left_char_ptr + STR_CELL_WIDTH;

            }

            break;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"IN",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  Test whether an element is a member of a set.
       */

      case ft_map :

         /*
          *  If the value being checked is not a tuple of length 2, it
          *  can't be in a map.
          */

         if (left->sp_form != ft_tuple) {

            condition_true = NO;
            break;

         }

         /* pick out the domain and range elements */

         tuple_root = left->sp_val.sp_tuple_ptr;
         if (tuple_root->t_ntype.t_root.t_length != 2) {

            condition_true = NO;
            break;

         }

         /* we need the left-most child */

         source_height = tuple_root->t_ntype.t_root.t_height;
         while (source_height--) {

            tuple_root = tuple_root->t_child[0].t_header;

#ifdef TRAPS

            if (tuple_root == NULL)
               giveup(SETL_SYSTEM msg_corrupted_tuple);

#endif

         }

         /* if the domain or range is omega, break */

         domain_element = &((tuple_root->t_child[0].t_cell)->t_spec);
         domain_hash_code = (tuple_root->t_child[0].t_cell)->t_hash_code;
         range_element = &((tuple_root->t_child[1].t_cell)->t_spec);
         range_hash_code = (tuple_root->t_child[1].t_cell)->t_hash_code;

         if (domain_element->sp_form == ft_omega) {

            condition_true = NO;
            break;

         }

         /*
          *  We now have a valid pair, so we search for it in the
          *  map.
          */

         map_root = right->sp_val.sp_map_ptr;
         map_work_hdr = map_root;
         work_hash_code = domain_hash_code;
         target_height = map_root->m_ntype.m_root.m_height;
         while (target_height-- && map_work_hdr != NULL) {

            /* extract the element's index at this level */

            target_index = work_hash_code & MAP_HASH_MASK;
            work_hash_code >>= MAP_SHIFT_DIST;

            map_work_hdr =
               map_work_hdr->m_child[target_index].m_header;

         }

         if (map_work_hdr == NULL) {

            condition_true = NO;
            break;

         }

         /*
          *  At this point, map_work_hdr points to the lowest level header
          *  record.  The next problem is to determine if the domain element
          *  is in the map.  We compare the element with the clash list.
          */

         target_index = work_hash_code & MAP_HASH_MASK;
         for (map_cell = map_work_hdr->m_child[target_index].m_cell;
              map_cell != NULL &&
                 map_cell->m_hash_code < domain_hash_code;
              map_cell = map_cell->m_next);

         /* try to find the domain element */

         is_equal = NO;
         while (map_cell != NULL &&
                map_cell->m_hash_code == domain_hash_code) {

            spec_equal(is_equal,&(map_cell->m_domain_spec),domain_element);

            if (is_equal)
               break;

            map_cell = map_cell->m_next;

         }

         if (!is_equal) {

            condition_true = NO;
            break;

         }

         /* if we don't have a multi-value cell, just check the range */

         if (!(map_cell->m_is_multi_val)) {

            spec_equal(condition_true,
                       &(map_cell->m_range_spec),range_element);
            break;

         }

         /*
          *  We have to check the value set.
          */

         set_root = map_cell->m_range_spec.sp_val.sp_set_ptr;
         set_work_hdr = set_root;

         /* get the element's hash code */

         work_hash_code = range_hash_code;

         /* descend the header tree until we get to a leaf */

         source_height = set_root->s_ntype.s_root.s_height;
         while (source_height-- && set_work_hdr != NULL) {

            /* extract the element's index at this level */

            source_index = work_hash_code & SET_HASH_MASK;
            work_hash_code >>= SET_SHIFT_DIST;

            set_work_hdr =
               set_work_hdr->s_child[source_index].s_header;

         }

         if (set_work_hdr == NULL) {

            condition_true = NO;
            break;

         }

         /*
          *  At this point, set_work_hdr points to the lowest level header
          *  record.  The next problem is to determine if the element is in
          *  the set.  We compare the element with the clash list.
          */

         source_index = work_hash_code & SET_HASH_MASK;
         for (set_cell = set_work_hdr->s_child[source_index].s_cell;
              set_cell != NULL &&
                 set_cell->s_hash_code < range_hash_code;
              set_cell = set_cell->s_next);

         /* check for a duplicate element */

         condition_true = NO;
         while (set_cell != NULL &&
                set_cell->s_hash_code == range_hash_code) {

            spec_equal(condition_true,&(set_cell->s_spec),range_element);

            if (condition_true)
               break;

            set_cell = set_cell->s_next;

         }

         break;

      case ft_set :

         set_root = right->sp_val.sp_set_ptr;

         /* get the test element's hash code */

         spec_hash_code(work_hash_code,left);
         source_hash_code = work_hash_code;

         /* descend the header tree until we get to a leaf */

         source_height = set_root->s_ntype.s_root.s_height;
         set_work_hdr = set_root;
         condition_true = YES;

         while (source_height-- && condition_true) {

            /* extract the element's index at this level */

            source_index = work_hash_code & SET_HASH_MASK;
            work_hash_code = work_hash_code >> SET_SHIFT_DIST;

            /* if we're missing a header record, we can quit */

            if (set_work_hdr->s_child[source_index].s_header == NULL) {

               condition_true = NO;
               break;

            }

            set_work_hdr = set_work_hdr->s_child[source_index].s_header;

         }

         /*
          *  At this point, set_work_hdr points to the lowest level header
          *  record.  The next problem is to determine if the element is
          *  in the clash list.
          */

         if (condition_true) {

            source_index = work_hash_code & SET_HASH_MASK;
            for (set_cell = set_work_hdr->s_child[source_index].s_cell;
                 set_cell != NULL &&
                    set_cell->s_hash_code < source_hash_code;
                 set_cell = set_cell->s_next);

            /* check for a duplicate element */

            condition_true = NO;
            while (set_cell != NULL &&
                   set_cell->s_hash_code == source_hash_code) {

               spec_equal(condition_true,&(set_cell->s_spec),left);

               if (condition_true)
                  break;

               set_cell = set_cell->s_next;

            }
         }

         break;

      /*
       *  Test whether a specifier is a component of a tuple.
       */

      case ft_tuple :

         /* set up to iterate over the source */

         tuple_root = right->sp_val.sp_tuple_ptr;
         tuple_work_hdr = tuple_root;
         source_height = tuple_root->t_ntype.t_root.t_height;
         target_number = 0;
         source_index = 0;
         condition_true = NO;

         while (target_number < tuple_root->t_ntype.t_root.t_length) {

            /* if we have an element already, check it it */

            if (!source_height && source_index < TUP_HEADER_SIZE) {

               if (tuple_work_hdr->t_child[source_index].t_cell == NULL) {

                  if (target_number < tuple_root->t_ntype.t_root.t_length &&
                      left->sp_form == ft_omega) {

                     condition_true = YES;

                     break;

                  }

                  target_number++;
                  source_index++;

                  continue;

               }

               tuple_cell = tuple_work_hdr->t_child[source_index].t_cell;
               spec_equal(is_equal,&(tuple_cell->t_spec),left);
               if (is_equal) {

                  condition_true = YES;

                  break;

               }

               target_number++;
               source_index++;

               continue;

            }

            /* the current header node is exhausted -- find the next one */

            if (source_index >= TUP_HEADER_SIZE) {

               /* break if we've exhausted the source */

               if (tuple_work_hdr == tuple_root) {

                  condition_true = left->sp_form == ft_omega;
                  break;

               }

               source_height++;
               source_index =
                  tuple_work_hdr->t_ntype.t_intern.t_child_index + 1;
               tuple_work_hdr =
                  tuple_work_hdr->t_ntype.t_intern.t_parent;

               continue;

            }

            /* skip over null nodes */

            if (tuple_work_hdr->t_child[source_index].t_header == NULL) {

               if (target_number < tuple_root->t_ntype.t_root.t_length &&
                   left->sp_form == ft_omega) {

                  condition_true = YES;
                  break;

               }

               target_number += 1L << (source_height * TUP_SHIFT_DIST);
               source_index++;

               continue;

            }

            /* otherwise drop down a level */

            tuple_work_hdr =
               tuple_work_hdr->t_child[source_index].t_header;
            source_index = 0;
            source_height--;

         }

         break;

      /*
       *  We can't do much with objects, since the user is allowed to
       *  define his own operation.  We look for such a user-defined
       *  operation and call it.
       */

      case ft_object :

         object_root = right->sp_val.sp_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;

         /* get information about this slot in the object */

         slot_info = class_ptr->ut_slot_info + m_in_r;

         /* make sure we can access this slot */

         if (!slot_info->si_in_class)
            abend(SETL_SYSTEM msg_missing_method,"IN",
                  class_ptr->ut_name);

         /* push the left operand */

         push_pstack(left);

         /* call the procedure */

         spare1.sp_form = ft_omega;
         call_procedure(SETL_SYSTEM &spare1,
                        slot_info->si_spec,
                        right,
                        1L,EXTRA,YES,4);

         goto in_end;

      /*
       *  Anything else is a run-time error.
       */

      default :

         /*
          *  We don't expect this to happen very often, but we do allow
          *  objects on the left.
          */

         if (left->sp_form == ft_object) {

            object_root = left->sp_val.sp_object_ptr;
            class_ptr = object_root->o_ntype.o_root.o_class;

            /* get information about this slot in the object */

            slot_info = class_ptr->ut_slot_info + m_in;

            /* make sure we can access this slot */

            if (!slot_info->si_in_class)
               abend(SETL_SYSTEM msg_missing_method,"IN",
                     class_ptr->ut_name);

            /* push the right operand */

            push_pstack(right);

            /* call the procedure */

            spare1.sp_form = ft_omega;
            call_procedure(SETL_SYSTEM &spare1,
                           slot_info->si_spec,
                           left,
                           1L,EXTRA,YES,4);

            goto in_end;

         }

         /* that's all we accept */

         else {

            binop_abend(SETL_SYSTEM msg_bad_binop_forms,"IN",
                  abend_opnd_str(SETL_SYSTEM left),
                  abend_opnd_str(SETL_SYSTEM right));

         }

         break;

   }

   /*
    *  At this point condition_true is either YES or NO.  The action we
    *  take depends on the opcode.
    */

   switch (ip->i_opcode) {

      case p_goin :

         if (condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_gonotin :

         if (!condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_in :

         target = ip->i_operand[0].i_spec_ptr;

         if (condition_true) {

            unmark_specifier(target);
            target->sp_form = spec_true->sp_form;
            target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = spec_false->sp_form;
            target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

         }

         break;

      case p_notin :

         target = ip->i_operand[0].i_spec_ptr;

         if (!condition_true) {

            unmark_specifier(target);
            target->sp_form = spec_true->sp_form;
            target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = spec_false->sp_form;
            target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

         }

         break;

   }
   in_end:

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{cond\_ops5()}
 *
 *  This function handles condition operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void cond_ops5(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_incs}{subset operations}
 *        {specifier}{target operand}
 *        {specifier}{left operand}
 *        {specifier}{right operand}
 *
 *  This case handles operations dependent on the \verb"SUBSET"
 *  operation.
\*/

case p_incs :           case p_goincs :        case p_gonincs :

{

   /* pick out the operand pointers, to save some expression evaluations */

   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   if (left->sp_form == ft_map)
      map_to_set(SETL_SYSTEM left,left);

   if (right->sp_form == ft_map)
      map_to_set(SETL_SYSTEM right,right);

   if (left->sp_form != ft_set || right->sp_form != ft_set) {

      abend(SETL_SYSTEM msg_incompatible_opnds,
            abend_opnd_str(SETL_SYSTEM left),
            abend_opnd_str(SETL_SYSTEM right));

   }

   condition_true = set_subset(SETL_SYSTEM right,left);

   /*
    *  At this point condition_true is either YES or NO.  The action we
    *  take depends on the opcode.
    */

   switch (ip->i_opcode) {

      case p_goincs :

         if (condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_gonincs :

         if (!condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_incs :

         target = ip->i_operand[0].i_spec_ptr;

         if (condition_true) {

            unmark_specifier(target);
            target->sp_form = spec_true->sp_form;
            target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

         }
         else {

            unmark_specifier(target);
            target->sp_form = spec_false->sp_form;
            target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

         }

         break;

   }

   break;

}

/*\
 *  \pcode{p\_gotrue}{go if true of false}
 *        {specifier}{target operand}
 *        {specifier}{left operand}
 *        {}{unused}
 *
 *  This case handles branches on logical true or false.
\*/

case p_gotrue :         case p_gofalse :

{

   /* pick out the operand pointers, to save some expression evaluations */

   left = ip->i_operand[1].i_spec_ptr;

   if (left->sp_form != ft_atom) {

      abend(SETL_SYSTEM "Expected TRUE or FALSE");

   }
   else if (left->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num) {

      condition_true = YES;

   }
   else if (left->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {

      condition_true = NO;

   }
   else {

      abend(SETL_SYSTEM "Expected TRUE or FALSE");

   }

   /*
    *  At this point condition_true is either YES or NO.  The action we
    *  take depends on the opcode.
    */

   switch (ip->i_opcode) {

      case p_gotrue :

         if (condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

      case p_gofalse :

         if (!condition_true)
            pc = ip->i_operand[0].i_inst_ptr;

         break;

   }

   break;

}

/*\
 *  \pcode{p\_go}{branch unconditionally}
 *        {instruction}{branch target}
 *        {}{unused}
 *        {}{unused}
 *
 *  This instruction makes an unconditional branch.
\*/

case p_go :

   pc = ip->i_operand[0].i_inst_ptr;

   break;

/*\
 *  \pcode{p\_goind}{indirect branch}
 *        {instruction}{branch target}
 *        {}{unused}
 *        {}{unused}
 *
 *  This instruction makes an unconditional indirect branch.
\*/

case p_goind :

   left = ip->i_operand[0].i_spec_ptr;

#ifdef TRAPS

   if (left->sp_form != ft_label)
      trap(__FILE__,__LINE__,msg_bad_indirect_goto);

#endif

   pc = left->sp_val.sp_label_ptr;

   break;

/*\
 *  \pcode{p\_and}{logical and}
 *        {specifier}{target operand}
 *        {specifier}{left operand}
 *        {specifier}{right operand}
 *
 *  This case handles a logical and operation.
\*/

case p_and :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   if (left->sp_form != ft_atom || right->sp_form != ft_atom) {

      abend(SETL_SYSTEM "Expected TRUE or FALSE");

   }
   else if (left->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num) {
      if (right->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num) {

         unmark_specifier(target);
         target->sp_form = ft_atom;
         target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

      }
      else if (right->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {

         unmark_specifier(target);
         target->sp_form = ft_atom;
         target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

      }
      else {

         abend(SETL_SYSTEM "Expected TRUE or FALSE");

      }
   }
   else if (left->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {
      if (right->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num ||
          right->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {

         unmark_specifier(target);
         target->sp_form = ft_atom;
         target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

      }
      else {

         abend(SETL_SYSTEM "Expected TRUE or FALSE");

      }
   }
   else {

      abend(SETL_SYSTEM "Expected TRUE or FALSE");

   }

   break;

}

/*\
 *  \pcode{p\_and}{logical or}
 *        {specifier}{target operand}
 *        {specifier}{left operand}
 *        {specifier}{right operand}
 *
 *  This case handles a logical or operation.
\*/

case p_or :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   if (left->sp_form != ft_atom || right->sp_form != ft_atom) {

      abend(SETL_SYSTEM "Expected TRUE or FALSE");

   }
   else if (left->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {
      if (right->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num) {

         unmark_specifier(target);
         target->sp_form = ft_atom;
         target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

      }
      else if (right->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {

         unmark_specifier(target);
         target->sp_form = ft_atom;
         target->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

      }
      else {

         abend(SETL_SYSTEM "Expected TRUE or FALSE");

      }
   }
   else if (left->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num) {
      if (right->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num ||
          right->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num) {

         unmark_specifier(target);
         target->sp_form = ft_atom;
         target->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

      }
      else {

         abend(SETL_SYSTEM "Expected TRUE or FALSE");

      }
   }
   else {

      abend(SETL_SYSTEM "Expected TRUE or FALSE");

   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

#endif                                 /* PARTG                             */
#if !SHORT_TEXT | PARTH

/*\
 *  \function{iter\_ops1()}
 *
 *  This function handles iterator operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void iter_ops1(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_iter}{iterator initialization}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {integer}{iteration type}
 *
 *  This opcode initializes an iterator.  Iterators are internal types
 *  created with this opcode.
\*/

case p_iter :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* the action we take depends on the iteration type */

   switch ((int)(ip->i_operand[2].i_integer)) {

      /*
       *  Iterate over a power set.
       */

      case it_pow :

         switch (left->sp_form) {

            case ft_map :

               spare.sp_form = ft_omega;
               map_to_set(SETL_SYSTEM &spare,left);
               start_pow_iterator(SETL_SYSTEM target,&spare);
               unmark_specifier(&spare);
               spare.sp_form = ft_omega;
               break;

            case ft_set :

               start_pow_iterator(SETL_SYSTEM target,left);
               break;

            /*
             *  We can't avoid computing this function if the source is
             *  an object.
             */

            case ft_object :

               object_root = left->sp_val.sp_object_ptr;
               class_ptr = object_root->o_ntype.o_root.o_class;

               /* get information about this slot in the object */

               slot_info = class_ptr->ut_slot_info + m_pow;

               /* make sure we can access this slot */

               if (!slot_info->si_in_class)
                  abend(SETL_SYSTEM msg_missing_method,"POW",
                        class_ptr->ut_name);

               /* call the procedure */

               spare1.sp_form = ft_omega;
               call_procedure(SETL_SYSTEM &spare1,
                              slot_info->si_spec,
                              left,
                              0L,EXTRA,YES,2);
	       break;

            /*
             *  Anything else is a run-time error.
             */

            default :

               unop_abend(SETL_SYSTEM msg_bad_unop_form,"POW",
                     abend_opnd_str(SETL_SYSTEM left));

         }

         break;

      /*
       *  Iterate over an npower set.
       */

      case it_npow :

         right = PC->i_operand[0].i_spec_ptr;
         BUMPPC(1);

         switch (left->sp_form) {

            /*
             *  If we have an integer on the left we look for a set on
             *  the right.
             */

            case ft_short :

               short_value = left->sp_val.sp_short_value;

               if (short_value < 0)
                  abend(SETL_SYSTEM msg_negative_npow);

               switch (right->sp_form) {

                  case ft_map :

                     spare.sp_form = ft_omega;
                     map_to_set(SETL_SYSTEM &spare,right);

                     start_npow_iterator(SETL_SYSTEM target,&spare,short_value);
                     unmark_specifier(&spare);
                     spare.sp_form = ft_omega;

                     break;

                  case ft_set :

                     start_npow_iterator(SETL_SYSTEM target,right,short_value);

                     break;

                  /*
                   *  Last resort: check for a right method for an
                   *  object.
                   */

                  case ft_object :

                     object_root = right->sp_val.sp_object_ptr;
                     class_ptr = object_root->o_ntype.o_root.o_class;

                     /* get information about this slot in the object */

                     slot_info = class_ptr->ut_slot_info + m_npow_r;

                     /* make sure we can access this slot */

                     if (!slot_info->si_in_class)
                        abend(SETL_SYSTEM msg_missing_method,"NPOW",
                              class_ptr->ut_name);

                     /* push the left operand */

                     push_pstack(left);

                     /* call the procedure */

                     spare1.sp_form = ft_omega;
                     call_procedure(SETL_SYSTEM &spare1,
                                    slot_info->si_spec,
                                    left,
                                    1L,EXTRA,YES,2);

                     break;

                  /*
                   *  Anything else is a run-time error.
                   */

                  default :

                     binop_abend(SETL_SYSTEM msg_bad_binop_forms,"NPOW",
                           abend_opnd_str(SETL_SYSTEM left),
                           abend_opnd_str(SETL_SYSTEM right));

               }

               break;

            case ft_long :

               short_value = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);

               if (short_value < 0)
                  abend(SETL_SYSTEM msg_negative_npow);

               switch (right->sp_form) {

                  case ft_map :

                     spare.sp_form = ft_omega;
                     map_to_set(SETL_SYSTEM &spare,right);

                     start_npow_iterator(SETL_SYSTEM target,&spare,short_value);
                     unmark_specifier(&spare);
                     spare.sp_form = ft_omega;

                     break;

                  case ft_set :

                     start_npow_iterator(SETL_SYSTEM target,right,short_value);

                     break;

                  /*
                   *  Last resort: check for a right method for an
                   *  object.
                   */

                  case ft_object :

                     object_root = right->sp_val.sp_object_ptr;
                     class_ptr = object_root->o_ntype.o_root.o_class;

                     /* get information about this slot in the object */

                     slot_info = class_ptr->ut_slot_info + m_npow_r;

                     /* make sure we can access this slot */

                     if (!slot_info->si_in_class)
                        abend(SETL_SYSTEM msg_missing_method,"NPOW",
                              class_ptr->ut_name);

                     /* push the left operand */

                     push_pstack(left);

                     /* call the procedure */

                     spare1.sp_form = ft_omega;
                     call_procedure(SETL_SYSTEM &spare1,
                                    slot_info->si_spec,
                                    left,
                                    1L,EXTRA,YES,2);

                     break;

                  /*
                   *  Anything else is a run-time error.
                   */

                  default :

                     binop_abend(SETL_SYSTEM msg_bad_binop_forms,"NPOW",
                           abend_opnd_str(SETL_SYSTEM left),
                           abend_opnd_str(SETL_SYSTEM right));

               }

               break;

            /*
             *  If we have a set on the left, we look for an integer on
             *  the right.
             */

            case ft_map :

               map_to_set(SETL_SYSTEM left,left);

            case ft_set :

               switch (right->sp_form) {

                  case ft_short :

                     short_value = right->sp_val.sp_short_value;
                     start_npow_iterator(SETL_SYSTEM target,left,short_value);

                     break;

                  case ft_long :

                     short_value = long_to_short(SETL_SYSTEM right->sp_val.sp_long_ptr);
                     start_npow_iterator(SETL_SYSTEM target,left,short_value);

                     break;

                  /*
                   *  Last resort: check for a right method for an
                   *  object.
                   */

                  case ft_object :

                     object_root = right->sp_val.sp_object_ptr;
                     class_ptr = object_root->o_ntype.o_root.o_class;

                     /* get information about this slot in the object */

                     slot_info = class_ptr->ut_slot_info + m_npow_r;

                     /* make sure we can access this slot */

                     if (!slot_info->si_in_class)
                        abend(SETL_SYSTEM msg_missing_method,"NPOW",
                              class_ptr->ut_name);

                     /* push the left operand */

                     push_pstack(left);

                     /* call the procedure */

                     spare1.sp_form = ft_omega;
                     call_procedure(SETL_SYSTEM &spare1,
                                    slot_info->si_spec,
                                    left,
                                    1L,EXTRA,YES,2);
                     break;

                  /*
                   *  Anything else is a run-time error.
                   */

                  default :

                     binop_abend(SETL_SYSTEM msg_bad_binop_forms,"NPOW",
                           abend_opnd_str(SETL_SYSTEM left),
                           abend_opnd_str(SETL_SYSTEM right));

               }

               break;

            /*
             *  We can't avoid computing this function if the source is
             *  an object.
             */

            case ft_object :

               object_root = left->sp_val.sp_object_ptr;
               class_ptr = object_root->o_ntype.o_root.o_class;

               /* get information about this slot in the object */

               slot_info = class_ptr->ut_slot_info + m_npow;

               /* make sure we can access this slot */

               if (!slot_info->si_in_class)
                  abend(SETL_SYSTEM msg_missing_method,"NPOW",
                        class_ptr->ut_name);

               /* push the right operand */

               push_pstack(right);

               /* call the procedure */

               spare1.sp_form = ft_omega;
               call_procedure(SETL_SYSTEM &spare1,
                              slot_info->si_spec,
                              left,
                              1L,EXTRA,YES,2);

	       break;
            /*
             *  Anything else is a run-time error.
             */

            default :

               binop_abend(SETL_SYSTEM msg_bad_binop_forms,"NPOW",
                     abend_opnd_str(SETL_SYSTEM left),
                     abend_opnd_str(SETL_SYSTEM right));

         }

         break;

      /*
       *  Iterate over a domain set.
       */

      case it_domain :

         switch (left->sp_form) {

            case ft_set :

               spare.sp_form = ft_omega;
               if (!set_to_map(SETL_SYSTEM &spare,left,NO))
                  abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM left));

               start_domain_iterator(SETL_SYSTEM target,&spare);
               unmark_specifier(&spare);
               spare.sp_form = ft_omega;

               break;

            case ft_map :

               start_domain_iterator(SETL_SYSTEM target,left);

               break;

            /*
             *  We can't avoid computing this function if the source is
             *  an object.
             */

            case ft_object :

               object_root = left->sp_val.sp_object_ptr;
               class_ptr = object_root->o_ntype.o_root.o_class;

               /* get information about this slot in the object */

               slot_info = class_ptr->ut_slot_info + m_domain;

               /* make sure we can access this slot */

               if (!slot_info->si_in_class)
                  abend(SETL_SYSTEM msg_missing_method,"DOMAIN",
                        class_ptr->ut_name);

               /* call the procedure */

               spare1.sp_form = ft_omega;
               call_procedure(SETL_SYSTEM &spare1,
                              slot_info->si_spec,
                              left,
                              0L,EXTRA,YES,2);
	       break;

            /*
             *  Anything else is a run-time error.
             */

            default :

               unop_abend(SETL_SYSTEM msg_bad_unop_form,"DOMAIN",
                     abend_opnd_str(SETL_SYSTEM left));

         }

         break;

      /*
       *  This is the most general, most frequently used kind of
       *  iteration.  We found the syntax \verb"x in S", and we just
       *  iterate over some kind of value.
       */

      case it_single :

         switch (left->sp_form) {

            case ft_set :

               start_set_iterator(SETL_SYSTEM target,left);

               break;

            case ft_map :

               start_map_iterator(SETL_SYSTEM target,left);

               break;

            case ft_tuple :

               start_tuple_iterator(SETL_SYSTEM target,left);

               break;

            case ft_string :

               start_string_iterator(SETL_SYSTEM target,left);

               break;

            case ft_object :

               start_object_iterator(SETL_SYSTEM target,left);

               break;

            /*
             *  Anything else is a run-time error.
             */

            default :

               abend(SETL_SYSTEM "Can not iterate over source:\nSource => %s",
                     abend_opnd_str(SETL_SYSTEM &spare1));

               break;

         }

         break;

      /*
       *  This probably corresponds to a map iteration.  It arises from
       *  the syntax \verb"[x,y] in f".
       */

      case it_pair :

         switch (left->sp_form) {

            case ft_set :

               spare.sp_form = ft_omega;
               if (!set_to_map(SETL_SYSTEM &spare,left,YES))
                  abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM left));

               start_map_pair_iterator(SETL_SYSTEM target,&spare);
               unmark_specifier(&spare);
               spare.sp_form = ft_omega;

               break;

            case ft_map :

               start_map_pair_iterator(SETL_SYSTEM target,left);

               break;

            case ft_tuple :

               start_alt_tuple_pair_iterator(SETL_SYSTEM target,left);

               break;

            case ft_object :

               start_object_pair_iterator(SETL_SYSTEM target,left);

               break;

            /*
             *  Anything else is a run-time error.
             */

            default :

               abend(SETL_SYSTEM "Can not iterate over source:\nSource => %s",
                     abend_opnd_str(SETL_SYSTEM left));

               break;

         }

         break;

      /*
       *  This probably corresponds to a map iteration.  It arises from
       *  the syntax \verb"y = f(x)".
       */

      case it_map_pair :

         switch (left->sp_form) {

            case ft_set :

               spare.sp_form = ft_omega;
               if (!set_to_map(SETL_SYSTEM &spare,left,NO))
                  abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM left));

               start_map_pair_iterator(SETL_SYSTEM target,&spare);
               unmark_specifier(&spare);
               spare.sp_form = ft_omega;

               break;

            case ft_map :

               start_map_pair_iterator(SETL_SYSTEM target,left);

               break;

            case ft_tuple :

               start_tuple_pair_iterator(SETL_SYSTEM target,left);

               break;

            case ft_string :

               start_string_pair_iterator(SETL_SYSTEM target,left);

               break;

            case ft_object :

               start_object_pair_iterator(SETL_SYSTEM target,left);

               break;

            /*
             *  Anything else is a run-time error.
             */

            default :

               abend(SETL_SYSTEM "Can not iterate over source:\nSource => %s",
                     abend_opnd_str(SETL_SYSTEM left));

               break;

         }

         break;

      /*
       *  This definitely corresponds to a map iteration.  It arises from
       *  the syntax \verb"y = f{x}".
       */

      case it_multi :

         switch (left->sp_form) {

            case ft_set :

               spare.sp_form = ft_omega;
               if (!set_to_map(SETL_SYSTEM &spare,left,NO))
                  abend(SETL_SYSTEM msg_invalid_set_map,abend_opnd_str(SETL_SYSTEM left));

               start_map_multi_iterator(SETL_SYSTEM target,&spare);
               unmark_specifier(&spare);
               spare.sp_form = ft_omega;

               break;

            case ft_map :

               start_map_multi_iterator(SETL_SYSTEM target,left);

               break;

            case ft_object :

               start_object_multi_iterator(SETL_SYSTEM target,left);

               break;

            /*
             *  Anything else is a run-time error.
             */

            default :

               abend(SETL_SYSTEM "Can not iterate over source:\nSource => %s",
                     abend_opnd_str(SETL_SYSTEM left));

               break;

         }
   }

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

#endif                                 /* PARTH                             */
#if !SHORT_TEXT | PARTI

/*\
 *  \function{iter\_ops2()}
 *
 *  This function handles iterator operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void iter_ops2(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_inext}{next in iterator}
 *        {specifier}{target operand}
 *        {specifier}{source operand}
 *        {label}{fail label}
 *
 *  This opcode picks the next element from an iterator.
\*/

case p_inext :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   iter_ptr = left->sp_val.sp_iter_ptr;

   switch(iter_ptr->it_type) {

      case it_set :

         if (!set_iterator_next(SETL_SYSTEM target,left))
            pc = ip->i_operand[2].i_inst_ptr;

         break;

      case it_map :

         if (!map_iterator_next(SETL_SYSTEM target,left))
            pc = ip->i_operand[2].i_inst_ptr;

         break;

      case it_tuple :

         if (!tuple_iterator_next(SETL_SYSTEM target,left))
            pc = ip->i_operand[2].i_inst_ptr;

         break;

      case it_string :

         if (!string_iterator_next(SETL_SYSTEM target,left))
            pc = ip->i_operand[2].i_inst_ptr;

         break;

      case it_pow :

         if (!pow_iterator_next(SETL_SYSTEM target,left))
            pc = ip->i_operand[2].i_inst_ptr;

         break;

      case it_npow :

         if (!npow_iterator_next(SETL_SYSTEM target,left))
            pc = ip->i_operand[2].i_inst_ptr;

         break;

      case it_domain :

         if (!domain_iterator_next(SETL_SYSTEM target,left))
            pc = ip->i_operand[2].i_inst_ptr;

         break;

      case it_map_pair :

         right = PC->i_operand[0].i_spec_ptr;
         if (!map_pair_iterator_next(SETL_SYSTEM target,right,left))
            pc = ip->i_operand[2].i_inst_ptr;
         else
            BUMPPC(1);

         break;

      case it_tuple_pair :

         right = PC->i_operand[0].i_spec_ptr;
         if (!tuple_pair_iterator_next(SETL_SYSTEM target,right,left))
            pc = ip->i_operand[2].i_inst_ptr;
         else
            BUMPPC(1);

         break;

      case it_alt_tuple_pair :

         right = PC->i_operand[0].i_spec_ptr;
         if (!alt_tuple_pair_iterator_next(SETL_SYSTEM target,right,left))
            pc = ip->i_operand[2].i_inst_ptr;
         else
            BUMPPC(1);

         break;

      case it_string_pair :

         right = PC->i_operand[0].i_spec_ptr;
         if (!string_pair_iterator_next(SETL_SYSTEM target,right,left))
            pc = ip->i_operand[2].i_inst_ptr;
         else
            BUMPPC(1);

         break;

      case it_map_multi :

         right = PC->i_operand[0].i_spec_ptr;
         if (!map_multi_iterator_next(SETL_SYSTEM target,right,left))
            pc = ip->i_operand[2].i_inst_ptr;
         else
            BUMPPC(1);

         break;

      case it_object :

         if (!object_iterator_next(SETL_SYSTEM target,left))
            pc = ip->i_operand[2].i_inst_ptr;

         break;

      case it_object_pair :

         right = PC->i_operand[0].i_spec_ptr;
         if (!object_pair_iterator_next(SETL_SYSTEM target,right,left))
            pc = ip->i_operand[2].i_inst_ptr;
         else
            BUMPPC(1);

         break;

      case it_object_multi :

         right = PC->i_operand[0].i_spec_ptr;
         if (!object_multi_iterator_next(SETL_SYSTEM target,right,left))
            pc = ip->i_operand[2].i_inst_ptr;
         else
            BUMPPC(1);

         break;

   }

   break;

}

/*\
 *  \pcode{p\_set}{create set}
 *        {specifier}{target specifier}
 *        {specifier}{cardinality (always an integer)}
 *        {}{unused}
 *
 *  This instruction creates a set from elements on the stack.  This is
 *  fairly straightforward, the only thing we need to point out is that
 *  the cardinality is a specifier, not an integer literal, so we must
 *  convert that to a \verb"long" before creating the set.
\*/

case p_set :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /*
    *  The set cardinality is generated internally, not directly by the
    *  user, so generally we can assume that it is an integer.  We do set
    *  a trap for debugging, however.
    */

#ifdef TRAPS

   if (left->sp_form != ft_long &&
       left->sp_form != ft_short)
      trap(__FILE__,__LINE__,msg_non_int_card);

#endif

   /* short's have the cardinality directly */

   if (left->sp_form == ft_short) {
      short_value = left->sp_val.sp_short_value;
   }
   else {
      short_value = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
   }

   /* first we calculate the height of the new header tree */

   target_height = 0;
   work_length = (short_value / 2) / SET_CLASH_SIZE + 1;
   while (work_length = work_length >> SET_SHIFT_DIST)
      target_height++;

   /* allocate and initialize the root header item */

   get_set_header(set_root);
   set_root->s_use_count = 1;
   set_root->s_hash_code = 0;
   set_root->s_ntype.s_root.s_cardinality = 0;
   set_root->s_ntype.s_root.s_height = target_height;
   for (i = 0;
        i < SET_HASH_SIZE;
        set_root->s_child[i++].s_cell = NULL);

   /* insert each element in the set */

   for (target_element = pstack + (pstack_top + 1 - short_value);
        target_element < pstack + (pstack_top + 1);
        target_element++) {

      if (target_element->sp_form == ft_omega)
         continue;

      set_work_hdr = set_root;

      /* get the element's hash code */

      spec_hash_code(work_hash_code,target_element);
      source_hash_code = work_hash_code;

      /* descend the header tree until we get to a leaf */

      target_height = set_root->s_ntype.s_root.s_height;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & SET_HASH_MASK;
         work_hash_code = work_hash_code >> SET_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (set_work_hdr->s_child[target_index].s_header == NULL) {

            get_set_header(new_set_hdr);
            new_set_hdr->s_ntype.s_intern.s_parent = set_work_hdr;
            new_set_hdr->s_ntype.s_intern.s_child_index = target_index;
            for (i = 0;
                 i < SET_HASH_SIZE;
                 new_set_hdr->s_child[i++].s_cell = NULL);
            set_work_hdr->s_child[target_index].s_header = new_set_hdr;
            set_work_hdr = new_set_hdr;

         }
         else {

            set_work_hdr =
               set_work_hdr->s_child[target_index].s_header;

         }
      }

      /*
       *  At this point, set_work_hdr points to the lowest level header
       *  record.  The next problem is to determine if the element is
       *  already in the set.  We compare the element with the clash
       *  list.
       */

      target_index = work_hash_code & SET_HASH_MASK;
      set_cell_tail = &(set_work_hdr->s_child[target_index].s_cell);
      for (set_cell = *set_cell_tail;
           set_cell != NULL &&
              set_cell->s_hash_code < source_hash_code;
           set_cell = set_cell->s_next) {

         set_cell_tail = &(set_cell->s_next);

      }

      /* check for a duplicate element */

      is_equal = NO;
      while (set_cell != NULL &&
             set_cell->s_hash_code == source_hash_code) {

         spec_equal(is_equal,&(set_cell->s_spec),target_element);

         if (is_equal)
            break;

         set_cell_tail = &(set_cell->s_next);
         set_cell = set_cell->s_next;

      }

      /* if we found the element, continue */

      if (is_equal)
         continue;

      /* if we reach this point we didn't find the element, so we insert it */

      get_set_cell(new_set_cell);
      mark_specifier(target_element);
      new_set_cell->s_spec.sp_form = target_element->sp_form;
      new_set_cell->s_spec.sp_val.sp_biggest =
         target_element->sp_val.sp_biggest;
      new_set_cell->s_hash_code = source_hash_code;
      new_set_cell->s_next = *set_cell_tail;
      *set_cell_tail = new_set_cell;
      set_root->s_ntype.s_root.s_cardinality++;
      set_root->s_hash_code ^= source_hash_code;

   }

   /* if our estimate of the header height was too large, compress it */

   contraction_trigger = (1 << ((set_root->s_ntype.s_root.s_height)
                                 * SET_SHIFT_DIST));
   if (contraction_trigger == 1)
      contraction_trigger = 0;

   while (set_root->s_ntype.s_root.s_cardinality < contraction_trigger) {

      set_root = set_contract_header(SETL_SYSTEM set_root);
      contraction_trigger /= SET_HASH_SIZE;

   }

   /* we pop the set elements from the stack */

   while (short_value--) {
      pop_pstack;
   }

   /* finally, we set our result */

   unmark_specifier(target);
   target->sp_form = ft_set;
   target->sp_val.sp_set_ptr = set_root;

   break;

}

/*\
 *  \pcode{p\_tuple}{create tuple}
 *        {specifier}{target specifier}
 *        {specifier}{length (always an integer)}
 *        {}{unused}
 *
 *  This instruction creates a tuple from elements on the stack.  This is
 *  fairly straightforward, the only thing we need to point out is that
 *  the length is a specifier, not an integer literal, so we must
 *  convert that to a \verb"long" before creating the tuple.
\*/

case p_tuple :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /*
    *  The tuple length is generated internally, not directly by the
    *  user, so generally we can assume that it is an integer.  We do set
    *  a trap for debugging, however.
    */

#ifdef TRAPS

   if (left->sp_form != ft_long &&
       left->sp_form != ft_short)
      trap(__FILE__,__LINE__,msg_non_int_length);

#endif

   /* short's have the length directly */

   if (left->sp_form == ft_short) {
      short_value = left->sp_val.sp_short_value;
   }
   else {
      short_value = long_to_short(SETL_SYSTEM left->sp_val.sp_long_ptr);
   }

   /* skip trailing omegas */

   for (;
        short_value > 0 && pstack[pstack_top].sp_form == ft_omega;
        short_value--) {

      pop_pstack;

   }

   /* first we calculate the height of the new header tree */

   target_height = 0;
   work_length = short_value;
   while (work_length = (work_length >> TUP_SHIFT_DIST))
      target_height++;

   /* allocate and initialize the root header item */

   get_tuple_header(tuple_root);
   tuple_root->t_use_count = 1;
   tuple_root->t_hash_code = 0;
   tuple_root->t_ntype.t_root.t_length = short_value;
   tuple_root->t_ntype.t_root.t_height = target_height;

   for (i = 0;
        i < TUP_HEADER_SIZE;
        tuple_root->t_child[i++].t_cell = NULL);

   /* insert each element in the tuple */

   target_number = 0;
   for (target_element = pstack + (pstack_top + 1 - short_value);
        target_number < short_value;
        target_element++, target_number++) {

      /* don't insert omegas */

      if (target_element->sp_form == ft_omega)
         continue;

      /* descend the header tree until we get to a leaf */

      tuple_work_hdr = tuple_root;
      for (target_height = tuple_root->t_ntype.t_root.t_height;
           target_height;
           target_height--) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (tuple_work_hdr->t_child[target_index].t_header == NULL) {

            get_tuple_header(new_tuple_hdr);
            new_tuple_hdr->t_ntype.t_intern.t_parent = tuple_work_hdr;
            new_tuple_hdr->t_ntype.t_intern.t_child_index = target_index;

            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_tuple_hdr->t_child[i++].t_cell = NULL);

            tuple_work_hdr->t_child[target_index].t_header = new_tuple_hdr;
            tuple_work_hdr = new_tuple_hdr;

         }
         else {

            tuple_work_hdr =
               tuple_work_hdr->t_child[target_index].t_header;

         }
      }

      /*
       *  At this point, tuple_work_hdr points to the lowest level header
       *  record.  We insert the new element in the appropriate slot.
       */

      mark_specifier(target_element);
      get_tuple_cell(tuple_cell);
      tuple_cell->t_spec.sp_form = target_element->sp_form;
      tuple_cell->t_spec.sp_val.sp_biggest =
         target_element->sp_val.sp_biggest;
      spec_hash_code(tuple_cell->t_hash_code,target_element);
      target_index = target_number & TUP_SHIFT_MASK;
      tuple_work_hdr->t_child[target_index].t_cell = tuple_cell;
      tuple_root->t_hash_code ^= tuple_cell->t_hash_code;

   }

   /* we pop the tuple elements from the stack */

   while (short_value--) {
      pop_pstack;
   }

   /* set the target value */

   unmark_specifier(target);
   target->sp_form = ft_tuple;
   target->sp_val.sp_tuple_ptr = tuple_root;

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \function{object\_ops()}
 *
 *  This function handles object operations if we have broken up
 *  the interpreter.
\*/

#ifdef SHORT_FUNCS

PRTYPE void object_ops(SETL_SYSTEM_PROTO_VOID)

{

   switch (ip->i_opcode) {

#endif

/*\
 *  \pcode{p\_initobj}{initialize object}
 *        {class}{object class}
 *        {specifier}{temporary storage}
 *        {}{unused}
 *
 *  Initializing an instance of a class (an object) generates a sequence
 *  of three or four instructions, of which this is the first.  The
 *  sequence looks like this:
 *
 *  \begin{verbatim}
 *     p_initobj     <class>   NULL         NULL
 *     p_lcall       NULL      InitObj      0
 *     p_lcall       NULL      CREATE       #args
 *     p_initend     <target>  NULL         NULL
 *  \end{verbatim}
 *
 *  The call to \verb"CREATE" will occur iff there is a creation
 *  procedure for the class.
 *
 *  The purpose of \verb"p_initobj" is to save all the instance variables
 *  of the current instance of the class in the current self for the
 *  class, and initialize those instance variables to $\Omega$.  The
 *  following two opcodes will initialize those instance variables, and
 *  \verb"p_initend" will save the created object and reload the previous
 *  self.
\*/

case p_initobj :

{

   /* stop process switches temporarily */

   critical_section++;

   /* pick out the operand pointer, to save some expression evaluations */

   class_ptr = ip->i_operand[0].i_class_ptr;

   /* if there is a current 'self' for the class, push it */

   if (class_ptr->ut_self != NULL) {

      /*
       *  The following block of code pushes the current 'self' for a
       *  class.  Notice that we don't have to worry about hash codes
       *  here:  We can't do anything with the object while it's on the
       *  stack, and there can be no other pointers to the object.  The
       *  copy is quicker if we ignore the hash code.
       */

      object_root = (class_ptr->ut_self)->ss_object;
      object_work_hdr = object_root;
      target_height = class_ptr->ut_obj_height;

      /* loop over the instance variables */

      for (slot_info = class_ptr->ut_first_var, target_number = 0;
           slot_info != NULL;
           slot_info = slot_info->si_next_var, target_number++) {

         /* drop down to a leaf */

         while (target_height) {

            /* extract the element's index at this level */

            target_index = (target_number >>
                                 (target_height * OBJ_SHIFT_DIST)) &
                              OBJ_SHIFT_MASK;

            /* if the header is missing, allocate one */

            if (object_work_hdr->o_child[target_index].o_header == NULL) {

               get_object_header(new_object_hdr);
               new_object_hdr->o_ntype.o_intern.o_parent = object_work_hdr;
               new_object_hdr->o_ntype.o_intern.o_child_index = target_index;

               for (i = 0;
                    i < OBJ_HEADER_SIZE;
                    new_object_hdr->o_child[i++].o_cell = NULL);

               object_work_hdr->o_child[target_index].o_header =
                     new_object_hdr;
               object_work_hdr = new_object_hdr;

            }

            /* otherwise just drop down a level */

            else {

               object_work_hdr =
                  object_work_hdr->o_child[target_index].o_header;

            }

            target_height--;

         }

         /*
          *  At this point, object_work_hdr points to the lowest level
          *  header record.  We insert the new element in the appropriate
          *  slot.
          */

         target_index = target_number & OBJ_SHIFT_MASK;
         object_cell = object_work_hdr->o_child[target_index].o_cell;
         if (object_cell == NULL) {
            get_object_cell(object_cell);
            object_work_hdr->o_child[target_index].o_cell = object_cell;
         }
         target_element = slot_info->si_spec;
         object_cell->o_spec.sp_form = target_element->sp_form;
         object_cell->o_spec.sp_val.sp_biggest =
            target_element->sp_val.sp_biggest;

         /*
          *  We move back up the header tree at this point, if it is
          *  necessary.
          */

         target_index++;
         while (target_index >= OBJ_HEADER_SIZE) {

            target_height++;
            target_index =
               object_work_hdr->o_ntype.o_intern.o_child_index + 1;
            object_work_hdr = object_work_hdr->o_ntype.o_intern.o_parent;

         }
      }
   }

   /*
    *  Whether or not we had to save 'self' we set the instance variables
    *  to omega.
    */

   for (slot_info = class_ptr->ut_first_var;
        slot_info != NULL;
        slot_info = slot_info->si_next_var) {

      (slot_info->si_spec)->sp_form = ft_omega;

   }

   /*
    *  We're going to install a new 'self' for this class.  We create an
    *  object header, but we don't need to install any data yet.
    */

   get_object_header(object_root);
   object_root->o_ntype.o_root.o_class = class_ptr;
   object_root->o_use_count = 1;
   object_root->o_process_ptr = NULL;

   for (i = 0;
        i < OBJ_HEADER_SIZE;
        object_root->o_child[i++].o_cell = NULL);

   get_self_stack(self_stack_ptr);
   self_stack_ptr->ss_object = object_root;
   self_stack_ptr->ss_next = class_ptr->ut_self;
   class_ptr->ut_self = self_stack_ptr;

   /* push the current class */

   push_cstack(pc,NULL,NULL,NULL,current_class,-1,0,0,NULL,EX_BODY_CODE,NULL,0);
   current_class = class_ptr;

   break;

}

/*\
 *  \pcode{p\_initend}{finish the initialization process}
 *        {specifier}{target specifier}
 *        {}{unused}
 *        {}{unused}
 *
 *  This instruction finishes up the initialization process.  We pop the
 *  current self from the class's self stack.
\*/

case p_initend :

{

   /* enable process switches */

   critical_section--;

   /* pick out the operand pointer, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   class_ptr = ip->i_operand[1].i_class_ptr;

   /*
    *  The following block of code loads the current 'self' to the
    *  object on the top of the 'self' stack.  We do have to set the
    *  hash code correctly at this time.
    */

   object_root = (class_ptr->ut_self)->ss_object;
   object_root->o_hash_code = (int32)class_ptr;
   object_work_hdr = object_root;
   target_height = class_ptr->ut_obj_height;

   /* loop over the instance variables */

   for (slot_info = class_ptr->ut_first_var, target_number = 0;
        slot_info != NULL;
        slot_info = slot_info->si_next_var, target_number++) {

      /* drop down to a leaf */

      while (target_height) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * OBJ_SHIFT_DIST)) &
                           OBJ_SHIFT_MASK;

         /* if the header is missing, allocate one */

         if (object_work_hdr->o_child[target_index].o_header == NULL) {

            get_object_header(new_object_hdr);
            new_object_hdr->o_ntype.o_intern.o_parent = object_work_hdr;
            new_object_hdr->o_ntype.o_intern.o_child_index = target_index;

            for (i = 0;
                 i < OBJ_HEADER_SIZE;
                 new_object_hdr->o_child[i++].o_cell = NULL);

            object_work_hdr->o_child[target_index].o_header =
                  new_object_hdr;
            object_work_hdr = new_object_hdr;

         }

         /* otherwise just drop down a level */

         else {

            object_work_hdr =
               object_work_hdr->o_child[target_index].o_header;

         }

         target_height--;

      }

      /*
       *  At this point, object_work_hdr points to the lowest level
       *  header record.  We insert the new element in the appropriate
       *  slot.
       */

      target_element = slot_info->si_spec;
      target_index = target_number & OBJ_SHIFT_MASK;
      object_cell = object_work_hdr->o_child[target_index].o_cell;
      if (object_cell == NULL) {
         get_object_cell(object_cell);
         object_work_hdr->o_child[target_index].o_cell = object_cell;
      }
      object_cell->o_spec.sp_form = target_element->sp_form;
      object_cell->o_spec.sp_val.sp_biggest =
         target_element->sp_val.sp_biggest;
      spec_hash_code(object_cell->o_hash_code,target_element);
      object_root->o_hash_code ^= object_cell->o_hash_code;

      /*
       *  We move back up the header tree at this point, if it is
       *  necessary.
       */

      target_index++;
      while (target_index >= OBJ_HEADER_SIZE) {

         target_height++;
         target_index =
            object_work_hdr->o_ntype.o_intern.o_child_index + 1;
         object_work_hdr = object_work_hdr->o_ntype.o_intern.o_parent;

      }
   }

   /* set the target */

   unmark_specifier(target);
   target->sp_form = ft_object;
   target->sp_val.sp_object_ptr = object_root;

   /* pop the 'self' stack */

   self_stack_ptr = class_ptr->ut_self;
   class_ptr->ut_self = self_stack_ptr->ss_next;
   free_self_stack(self_stack_ptr);

   /*
    *  If there is something on the top of the 'self' stack for this
    *  class, we have to reload it into the instance variables.
    */

   if (class_ptr->ut_self != NULL) {

      object_root = (class_ptr->ut_self)->ss_object;
      object_work_hdr = object_root;
      target_height = class_ptr->ut_obj_height;

      /* loop over the instance variables */

      for (slot_info = class_ptr->ut_first_var, target_number = 0;
           slot_info != NULL;
           slot_info = slot_info->si_next_var, target_number++) {

         /* drop down to a leaf */

         while (target_height) {

            /* extract the element's index at this level */

            target_index = (target_number >>
                                 (target_height * OBJ_SHIFT_DIST)) &
                              OBJ_SHIFT_MASK;

            /* we'll always have all internal nodes in this situation */

            object_work_hdr =
               object_work_hdr->o_child[target_index].o_header;
            target_height--;

         }

         /*
          *  At this point, object_work_hdr points to the lowest level
          *  header record.  We copy the appropriat element to the
          *  instance variable.
          */

         target_index = target_number & OBJ_SHIFT_MASK;
         object_cell = object_work_hdr->o_child[target_index].o_cell;
         target_element = slot_info->si_spec;
         target_element->sp_form = object_cell->o_spec.sp_form;
         target_element->sp_val.sp_biggest =
            object_cell->o_spec.sp_val.sp_biggest;

         /*
          *  We move back up the header tree at this point, if it is
          *  necessary.
          */

         target_index++;
         while (target_index >= OBJ_HEADER_SIZE) {

            target_height++;
            target_index =
               object_work_hdr->o_ntype.o_intern.o_child_index + 1;
            object_work_hdr = object_work_hdr->o_ntype.o_intern.o_parent;

         }
      }
   }

   /*
    *  If the class type is a process, establish a process record and put
    *  it in the pool.
    */

   if (class_ptr->ut_type == PROCESS_UNIT) {
      
      target->sp_form = ft_process;
      get_process(process_ptr);
      process_ptr->pc_next = process_head->pc_next;
      process_ptr->pc_next->pc_prev = process_ptr;
      process_ptr->pc_prev = process_head;
      process_head->pc_next = process_ptr;

      process_ptr->pc_type = CHILD_PROCESS;
      process_ptr->pc_idle = YES;
      process_ptr->pc_suspended = NO;
      process_ptr->pc_waiting = NO;
      process_ptr->pc_checking = NO;

      process_ptr->pc_object_ptr = target->sp_val.sp_object_ptr;
      process_ptr->pc_object_ptr->o_process_ptr = process_ptr;

      process_ptr->pc_request_head = NULL;
      process_ptr->pc_request_tail = &(process_ptr->pc_request_head);

      process_ptr->pc_pstack = (specifier *)malloc((size_t)(
                   PSTACK_BLOCK_SIZE * sizeof(specifier)));
      if (process_ptr->pc_pstack == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      process_ptr->pc_pstack_max = PSTACK_BLOCK_SIZE;
      process_ptr->pc_pstack_top = 0;
      process_ptr->pc_pstack_base = 0;
      process_ptr->pc_pstack[0].sp_form = ft_omega;
      
      process_ptr->pc_cstack = (struct call_stack_item *)malloc((size_t)(
                   CSTACK_BLOCK_SIZE * sizeof(struct call_stack_item)));
      if (process_ptr->pc_cstack == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      process_ptr->pc_cstack_max = CSTACK_BLOCK_SIZE;
      process_ptr->pc_cstack_top = 0;
      process_ptr->pc_current_class = class_ptr;

      process_ptr->pc_cstack[0].cs_unittab_ptr = class_ptr;
      process_ptr->pc_cstack[0].cs_proc_ptr = NULL;
      process_ptr->pc_cstack[0].cs_pc = 0;
      process_ptr->pc_cstack[0].cs_return_value = NULL;
      process_ptr->pc_cstack[0].cs_self_ptr = NULL;
      process_ptr->pc_cstack[0].cs_class_ptr = NULL;
      process_ptr->pc_cstack[0].cs_pstack_top = 1;
      process_ptr->pc_cstack[0].cs_C_return = 0;
      process_ptr->pc_cstack[0].cs_literal_proc = 0;
      process_ptr->pc_cstack[0].cs_code_type = EX_BODY_CODE;

   }

   /* pop the current class */

   pop_cstack;

   break;

}

/*\
 *  \pcode{p\_slot}{instance variable value}
 *        {specifier}{target specifier}
 *        {specifier}{object}
 *        {slot}{slot}
 *
 *  This instruction returns the value of a slot.  It is
 *  not generated in method calling situations.  Most of the time the
 *  slot will be an instance variable, and all we do is return its value.
 *  If the slot corresponds to a method, we have to create a procedure
 *  with the object attached as {\em self}.
\*/

case p_slot :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;

   /* left must be an object */

   if (left->sp_form != ft_object &&
       left->sp_form != ft_process)
      abend(SETL_SYSTEM "Expected class instance");

   object_root = left->sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;
   slot_number = ip->i_operand[2].i_slot;

   /* make sure the slot we're given is valid for the object */

   if (slot_number >= class_ptr->ut_slot_count)
      abend(SETL_SYSTEM "Instance variable not in class %s",class_ptr->ut_name);

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + slot_number;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM "Instance variable not in class %s",
            class_ptr->ut_name);

   if (class_ptr != current_class &&
       !(slot_info->si_is_public))
      abend(SETL_SYSTEM "Instance variable %s is not visible outside class %s",
            (slot_info->si_slot_ptr)->sl_name,
            class_ptr->ut_name);

   /* if we don't have a method, pick out the instance variable */

   if (!slot_info->si_is_method) {

      /*
       *  A process complication.  Processes follow pointer semantics, so
       *  if the object is currently active we don't change it on the
       *  record, we go directly to the workspace.
       */

      if (class_ptr->ut_type == PROCESS_UNIT &&
          process_head->pc_type == CHILD_PROCESS &&
          process_head->pc_object_ptr == object_root) {

         target_element = slot_info->si_spec;
         mark_specifier(target_element);
         unmark_specifier(target);
         target->sp_form = target_element->sp_form;
         target->sp_val.sp_biggest = target_element->sp_val.sp_biggest;
         
         break;

      }

      target_number = slot_info->si_index;
      target_height = class_ptr->ut_obj_height;
      object_work_hdr = object_root;

      /* drop down to a leaf */

      while (target_height) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * OBJ_SHIFT_DIST)) &
                           OBJ_SHIFT_MASK;

         /* drop down one level */

         object_work_hdr = object_work_hdr->o_child[target_index].o_header;
         target_height--;

      }

      /*
       *  At this point, object_work_hdr points to the lowest level header
       *  record.  We pick out the element.
       */

      target_index = target_number & OBJ_SHIFT_MASK;
      target_element =
         &((object_work_hdr->o_child[target_index].o_cell)->o_spec);

      /* copy the value to our target */

      mark_specifier(target_element);
      unmark_specifier(target);
      target->sp_form = target_element->sp_form;
      target->sp_val.sp_biggest = target_element->sp_val.sp_biggest;

      break;

   }

   /*
    *  At this point, the slot we found must be a method, so what we
    *  found is a reference something like:
    *
    *     target := object.method;
    *
    *  We have to convert this into a procedure value.  We must always
    *  copy the procedure, so that we can associate the object with that
    *  procedure.  The logic here is almost identical to p_penviron and
    *  p_menviron.  We save the procedure, the object, and the current
    *  environment.
    */

   /* we copy the procedure */

   proc_ptr = (slot_info->si_spec)->sp_val.sp_proc_ptr;
   get_proc(new_proc_ptr);
   memcpy((void *)new_proc_ptr,
          (void *)proc_ptr,
          sizeof(struct proc_item));
   new_proc_ptr->p_copy = NULL;
   new_proc_ptr->p_save_specs = NULL;
   new_proc_ptr->p_use_count = 1;
   new_proc_ptr->p_active_use_count = 0;
   new_proc_ptr->p_is_const = NO;
   new_proc_ptr->p_self_ptr = left->sp_val.sp_object_ptr;
   (left->sp_val.sp_object_ptr)->o_use_count++;

   /* set the target */

   unmark_specifier(target);
   target->sp_form = ft_proc;
   target->sp_val.sp_proc_ptr = new_proc_ptr;

   /* copy enclosing procedures */

   for (proc_ptr = new_proc_ptr;
        proc_ptr->p_parent != NULL;
        proc_ptr = proc_ptr->p_parent) {

      /*
       *  This is an expensive-looking test to find an active procedure
       *  with the desired signature.  In fact, it isn't so expensive
       *  since it's really rare to have deeply nested procedures, and
       *  rarer still to embed procedures used as first-class objects!
       */

      for (i = cstack_top; i >= 0; i--) {

         for (new_proc_ptr = cstack[i].cs_proc_ptr;
              new_proc_ptr == NULL ||
                 new_proc_ptr->p_signature != proc_ptr->p_parent;
              new_proc_ptr = new_proc_ptr->p_parent);

         if (new_proc_ptr != NULL)
            break;

      }

#ifdef TRAPS

      if (i < 0)
         trap(__FILE__,__LINE__,"Missing procedure on call stack");

#endif

      if (new_proc_ptr->p_copy != NULL) {

         new_proc_ptr = new_proc_ptr->p_copy;
         proc_ptr->p_parent = new_proc_ptr;
         new_proc_ptr->p_use_count++;
         break;

      }

      if (new_proc_ptr->p_save_specs != NULL) {

         new_proc_ptr = new_proc_ptr->p_copy;
         new_proc_ptr->p_use_count++;
         break;

      }

      /* copy the current procedure's parent */

      proc_ptr->p_parent = new_proc_ptr;
      get_proc(new_proc_ptr);
      memcpy((void *)new_proc_ptr,
             (void *)(proc_ptr->p_parent),
             sizeof(struct proc_item));
      (proc_ptr->p_parent)->p_copy = new_proc_ptr;
      proc_ptr->p_parent = new_proc_ptr;
      new_proc_ptr->p_use_count = 1;
      new_proc_ptr->p_active_use_count = 1;
      new_proc_ptr->p_is_const = NO;
      if (new_proc_ptr->p_self_ptr != NULL)
         (new_proc_ptr->p_self_ptr)->o_use_count++;

   }

   break;

}

/*\
 *  \pcode{p\_sslot}{assign slot value}
 *        {specifier}{object}
 *        {slot}{slot name}
 *        {specifier}{source operand}
 *
 *  This instruction assigns a value to an instance variable {\em from
 *  outside the class}.  Inside the class instance variables are ordinary
 *  variables.
 *
 *  We make sure the slot is visible and is a variable, then set it.
\*/

case p_sslot :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   right = ip->i_operand[2].i_spec_ptr;

   /* target must be an object */

   if (target->sp_form != ft_object &&
       target->sp_form != ft_process)
      abend(SETL_SYSTEM "Expected class instance");

   class_ptr = (target->sp_val.sp_object_ptr)->o_ntype.o_root.o_class;
   slot_number = ip->i_operand[1].i_slot;

   /* make sure the slot we're given is valid for the object */

   if (slot_number >= class_ptr->ut_slot_count)
      abend(SETL_SYSTEM "Instance variable not in class %s",class_ptr->ut_name);

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + slot_number;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM "Instance variable not in class %s",
            class_ptr->ut_name);

   if (class_ptr != current_class &&
       !(slot_info->si_is_public))
      abend(SETL_SYSTEM "Instance variable %s is not visible outside class %s",
            (slot_info->si_slot_ptr)->sl_name,
            class_ptr->ut_name);

   /* we can't assign methods */

   if (slot_info->si_is_method)
      abend(SETL_SYSTEM "Assignment to procedure %s in class %s is not allowed",
            (slot_info->si_slot_ptr)->sl_name,
            class_ptr->ut_name);

   /*
    *  A process complication.  Processes follow pointer semantics, so
    *  if the object is currently active we don't change it on the
    *  record, we go directly to the workspace.
    */

   if (class_ptr->ut_type == PROCESS_UNIT &&
       process_head->pc_type == CHILD_PROCESS &&
       process_head->pc_object_ptr == object_root) {

      target_element = slot_info->si_spec;
      mark_specifier(right);
      unmark_specifier(target_element);
      target_element->sp_form = right->sp_form;
      target_element->sp_val.sp_biggest = right->sp_val.sp_biggest;
      
      break;

   }

   /* we would like to use the target destructively */

   object_root = target->sp_val.sp_object_ptr;
   if (class_ptr->ut_type != PROCESS_UNIT && 
       object_root->o_use_count != 1) {

      object_root->o_use_count--;
      object_root = copy_object(SETL_SYSTEM object_root);
      target->sp_val.sp_object_ptr = object_root;

   }

   /* now we know we have a variable */

   target_number = slot_info->si_index;
   target_height = class_ptr->ut_obj_height;
   object_work_hdr = object_root;

   /* drop down to a leaf */

   while (target_height) {

      /* extract the element's index at this level */

      target_index = (target_number >>
                           (target_height * OBJ_SHIFT_DIST)) &
                        OBJ_SHIFT_MASK;

      /* drop down one level */

      object_work_hdr = object_work_hdr->o_child[target_index].o_header;
      target_height--;

   }

   /*
    *  At this point, object_work_hdr points to the lowest level header
    *  record.  We pick out the element.
    */

   target_index = target_number & OBJ_SHIFT_MASK;
   object_cell = object_work_hdr->o_child[target_index].o_cell;
   target_element =
      &((object_work_hdr->o_child[target_index].o_cell)->o_spec);

   /* reset the instance variable */

   object_root->o_hash_code ^= object_cell->o_hash_code;
   mark_specifier(right);
   unmark_specifier(target_element);
   target_element->sp_form = right->sp_form;
   target_element->sp_val.sp_biggest = right->sp_val.sp_biggest;
   spec_hash_code(object_cell->o_hash_code,right);
   object_root->o_hash_code ^= object_cell->o_hash_code;

   break;

}

/*\
 *  \pcode{p\_slotof}{general `of' for slots}
 *        {specifier}{target specifier}
 *        {slot}{slot name}
 *        {integer}{number of arguments}
 *
 *  This is probably a call to a method, but we don't know for sure.  It
 *  corresponds to the syntactic construct:
 *
 *  \begin{verbatim}
 *     target := object.slot(<args>);
 *  \end{verbatim}
 *
 *  \noindent
 *  We always find a three-operand sequence for this construct, of which
 *  this is the first (but it is followed by a noop, which is used here).
 *  The sequence looks like this:
 *
 *  \begin{verbatim}
 *     p_slotof       target      slot      #args
 *     p_noop         object      temp      arg1
 *     p_of1          target      temp      arg1    --
 *     p_of           target      temp      #args    |  one of these
 *     p_call         target      temp      #args   --
 *  \end{verbatim}
 *
 *  If the slot is a method, we call the method and skip the final
 *  instruction.  If it is a variable, we copy the value into temp and
 *  continue with the next instruction.
\*/

case p_slotof :

{

   /* pick out the object */

   left = PC->i_operand[0].i_spec_ptr;
   if (left->sp_form != ft_object &&
       left->sp_form != ft_process)
      abend(SETL_SYSTEM "Expected class instance");
   object_root = left->sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* make sure the slot we're given is valid for the object */

   slot_number = ip->i_operand[1].i_slot;
   if (slot_number >= class_ptr->ut_slot_count)
      abend(SETL_SYSTEM "Instance variable not in class %s",class_ptr->ut_name);

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + slot_number;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM "Instance variable not in class %s",
            class_ptr->ut_name);

   if (class_ptr != current_class &&
       !(slot_info->si_is_public))
      abend(SETL_SYSTEM "Instance variable %s is not visible outside class %s",
            (slot_info->si_slot_ptr)->sl_name,
            class_ptr->ut_name);

   /* if we have a method, call it */

   if (slot_info->si_is_method) {

      /* if we have one argument, push it */

      if (ip->i_operand[2].i_integer == 1) {
         push_pstack(PC->i_operand[2].i_spec_ptr);
      }

      /* bump the program counter past the dummy args */

      BUMPPC(2);

      /* call the procedure */

      call_procedure(SETL_SYSTEM ip->i_operand[0].i_spec_ptr,
                     slot_info->si_spec,
                     left,
                     ip->i_operand[2].i_integer,
                     NO,YES,0);

      break;

   }

   /* we have an instance variable, not a method */

   target = PC->i_operand[1].i_spec_ptr;

   /*
    *  A process complication.  Processes follow pointer semantics, so
    *  if the object is currently active we don't change it on the
    *  record, we go directly to the workspace.
    */

   if (class_ptr->ut_type == PROCESS_UNIT &&
       process_head->pc_type == CHILD_PROCESS &&
       process_head->pc_object_ptr == object_root) {

      target_element = slot_info->si_spec;
      mark_specifier(target_element);
      unmark_specifier(target);
      target->sp_form = target_element->sp_form;
      target->sp_val.sp_biggest = target_element->sp_val.sp_biggest;
      
      break;

   }

   target_number = slot_info->si_index;
   target_height = class_ptr->ut_obj_height;
   object_work_hdr = object_root;

   /* drop down to a leaf */

   while (target_height) {

      /* extract the element's index at this level */

      target_index = (target_number >>
                           (target_height * OBJ_SHIFT_DIST)) &
                        OBJ_SHIFT_MASK;

      /* drop down one level */

      object_work_hdr = object_work_hdr->o_child[target_index].o_header;
      target_height--;

   }

   /*
    *  At this point, object_work_hdr points to the lowest level header
    *  record.  We pick out the element.
    */

   target_index = target_number & OBJ_SHIFT_MASK;
   target_element =
      &((object_work_hdr->o_child[target_index].o_cell)->o_spec);

   /* copy the value to our target */

   mark_specifier(target_element);
   unmark_specifier(target);
   target->sp_form = target_element->sp_form;
   target->sp_val.sp_biggest = target_element->sp_val.sp_biggest;

   /* bump the program counter past the noop */

   BUMPPC(1);

   break;

}

/*\
 *  \pcode{p\_menviron}{copy a method with environment}
 *        {specifier}{target specifier}
 *        {specifier}{procedure}
 *        {}{unused}
 *
 *  This instruction copies a method with it's environment.  It is like
 *  \verb"p_penviron", but for methods we must save `self'.
\*/

case p_menviron :

{

   /* pick out the operand pointers, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   left = ip->i_operand[1].i_spec_ptr;
   class_ptr = current_class;

   /* we copy the procedure */

   proc_ptr = left->sp_val.sp_proc_ptr;
   get_proc(new_proc_ptr);
   memcpy((void *)new_proc_ptr,
          (void *)proc_ptr,
          sizeof(struct proc_item));
   new_proc_ptr->p_copy = NULL;
   new_proc_ptr->p_save_specs = NULL;
   new_proc_ptr->p_use_count = 1;
   new_proc_ptr->p_active_use_count = 0;
   new_proc_ptr->p_is_const = NO;
   new_proc_ptr->p_self_ptr = (class_ptr->ut_self)->ss_object;
   (new_proc_ptr->p_self_ptr)->o_use_count++;

   /* set the target */

   unmark_specifier(target);
   target->sp_form = ft_proc;
   target->sp_val.sp_proc_ptr = new_proc_ptr;

   /* copy enclosing procedures */

   for (proc_ptr = new_proc_ptr;
        proc_ptr->p_parent != NULL;
        proc_ptr = proc_ptr->p_parent) {

      /*
       *  This is an expensive-looking test to find an active procedure
       *  with the desired signature.  In fact, it isn't so expensive
       *  since it's really rare to have deeply nested procedures, and
       *  rarer still to embed procedures used as first-class objects!
       */

      for (i = cstack_top; i >= 0; i--) {

         for (new_proc_ptr = cstack[i].cs_proc_ptr;
              new_proc_ptr == NULL ||
                 new_proc_ptr->p_signature != proc_ptr->p_parent;
              new_proc_ptr = new_proc_ptr->p_parent);

         if (new_proc_ptr != NULL)
            break;

      }

#ifdef TRAPS

      if (i < 0)
         trap(__FILE__,__LINE__,"Missing procedure on call stack");

#endif

      if (new_proc_ptr->p_copy != NULL) {

         new_proc_ptr = new_proc_ptr->p_copy;
         proc_ptr->p_parent = new_proc_ptr;
         new_proc_ptr->p_use_count++;
         break;

      }

      /* copy the current procedure's parent */

      proc_ptr->p_parent = new_proc_ptr;
      get_proc(new_proc_ptr);
      memcpy((void *)new_proc_ptr,
             (void *)(proc_ptr->p_parent),
             sizeof(struct proc_item));
      (proc_ptr->p_parent)->p_copy = new_proc_ptr;
      proc_ptr->p_parent = new_proc_ptr;
      new_proc_ptr->p_use_count = 2;
      new_proc_ptr->p_active_use_count = 1;
      new_proc_ptr->p_is_const = NO;
      if (new_proc_ptr->p_self_ptr != NULL)
         (new_proc_ptr->p_self_ptr)->o_use_count++;

   }

   break;

}

/*\
 *  \pcode{p\_self}{copy self}
 *        {specifier}{target specifier}
 *        {}{unused}
 *        {}{unused}
 *
 *  This instruction saves the current self.  It is a nullary operation,
 *  valid only with class bodies.
\*/

case p_self :

{

   /* pick out the operand pointer, to save some expression evaluations */

   target = ip->i_operand[0].i_spec_ptr;
   class_ptr = current_class;

   /*
    *  A process complication.  Processes follow pointer semantics, so
    *  if the object is currently active we don't change it on the
    *  record, we go directly to the workspace.
    */

   if (class_ptr->ut_type == PROCESS_UNIT) {

      process_head->pc_object_ptr->o_use_count++;
      unmark_specifier(target);
      target->sp_form = ft_process;
      target->sp_val.sp_object_ptr = process_head->pc_object_ptr;
      
      break;
   }

   /*
    *  The following block of code loads the current 'self' into a new
    *  object.  We do have to set the hash code correctly at this time.
    */

   get_object_header(object_root);
   object_root->o_use_count = 1;
   object_root->o_ntype.o_root.o_class = class_ptr;
   object_root->o_process_ptr = NULL;

   for (i = 0;
        i < OBJ_HEADER_SIZE;
        object_root->o_child[i++].o_cell = NULL);

   object_root->o_hash_code = (int32)class_ptr;
   object_work_hdr = object_root;
   target_height = class_ptr->ut_obj_height;

   /* loop over the instance variables */

   for (slot_info = class_ptr->ut_first_var, target_number = 0;
        slot_info != NULL;
        slot_info = slot_info->si_next_var, target_number++) {

      /* drop down to a leaf */

      while (target_height) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * OBJ_SHIFT_DIST)) &
                           OBJ_SHIFT_MASK;

         /* if the header is missing, allocate one */

         if (object_work_hdr->o_child[target_index].o_header == NULL) {

            get_object_header(new_object_hdr);
            new_object_hdr->o_ntype.o_intern.o_parent = object_work_hdr;
            new_object_hdr->o_ntype.o_intern.o_child_index = target_index;

            for (i = 0;
                 i < OBJ_HEADER_SIZE;
                 new_object_hdr->o_child[i++].o_cell = NULL);

            object_work_hdr->o_child[target_index].o_header =
                  new_object_hdr;
            object_work_hdr = new_object_hdr;

         }

         /* otherwise just drop down a level */

         else {

            object_work_hdr =
               object_work_hdr->o_child[target_index].o_header;

         }

         target_height--;

      }

      /*
       *  At this point, object_work_hdr points to the lowest level
       *  header record.  We insert the new element in the appropriate
       *  slot.
       */

      target_element = slot_info->si_spec;
      target_index = target_number & OBJ_SHIFT_MASK;
      get_object_cell(object_cell);
      object_work_hdr->o_child[target_index].o_cell = object_cell;
      mark_specifier(target_element);
      object_cell->o_spec.sp_form = target_element->sp_form;
      object_cell->o_spec.sp_val.sp_biggest =
         target_element->sp_val.sp_biggest;
      spec_hash_code(object_cell->o_hash_code,target_element);
      object_root->o_hash_code ^= object_cell->o_hash_code;

      /*
       *  We move back up the header tree at this point, if it is
       *  necessary.
       */

      target_index++;
      while (target_index >= OBJ_HEADER_SIZE) {

         target_height++;
         target_index =
            object_work_hdr->o_ntype.o_intern.o_child_index + 1;
         object_work_hdr = object_work_hdr->o_ntype.o_intern.o_parent;

      }
   }

   /* set the target */

   unmark_specifier(target);
   target->sp_form = ft_object;
   target->sp_val.sp_object_ptr = object_root;

   break;

}

#ifdef SHORT_FUNCS

/* return to normal indentation */

   }
}

#endif

/*\
 *  \case{system error}
 *
 *  If we ever get here, we screwed up in the compiler.  It means we
 *  encountered an invalid op code.
\*/

#ifndef SHORT_FUNCS

#ifdef TRAPS

default :

   giveup(SETL_SYSTEM "System error -- Invalid opcode");

#endif

   /* return to normal indentation */

      }

   if (forever)
      goto execute_start;
   else
      return CONTINUE;

}

#endif

#endif                                 /* PARTI                             */
#if !SHORT_TEXT | PARTJ

/*\
 *  \function{call\_procedure()}
 *
 *  This instruction calls a procedure, either built-in, user-defined, or
 *  method.  First, we find the procedure table record corresponding to
 *  the procedure called.  If we have found a built-in procedure, we
 *  just check that the actual parameter count is correct, call the
 *  procedure, and pop the parameters from the stack.
 *
 *  If the procedure being called is user-defined, we have to worry about
 *  restoring the environment.  We require a flag, \verb"literal_proc" to
 *  help us decide whether we have to worry about restoring the
 *  environment.  Literal procedures do not have saved environments.
 *
 *  Another compilication is the possiblity of a method call.  If we get
 *  a method call, we have to save the current instance on the class'
 *  stack, then push the new instance on that stack and load it.
\*/

void call_procedure(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* return value                      */
   specifier *left,                    /* procedure operand                 */
   specifier *self,                    /* new self                          */
   int32 arg_count,                    /* number of arguments on stack      */
   int C_return,                       /* YES if C procedure                */
   int literal_proc,                   /* YES if a literal call             */
   int extra_code)                     /* Code type executed after a return */

{
int32 save_pstack_top;                 /* saved pstack_top                  */
specifier return_value;                /* return value specifier            */
instruction *save_ip;                  /* saved instruction pointer         */

   /* pick out the procedure pointer */

   proc_ptr = left->sp_val.sp_proc_ptr;

   /*
    *  Built-in procedures are relatively easy.  We just check the
    *  arguments and perform a C call.  There is no corresponding return
    *  from built-in procedures, it is a C return.
    */

   if ((proc_ptr->p_type == BUILTIN_PROC)||
       (proc_ptr->p_type == NATIVE_PROC)) {

      /* check the number of actual arguments */

      if (proc_ptr->p_var_args) {

         if (arg_count < proc_ptr->p_formal_count)
            abend(SETL_SYSTEM msg_missing_args,arg_count);

      }
      else {

         if (arg_count != proc_ptr->p_formal_count)
            abend(SETL_SYSTEM msg_actarg_ne_formal,arg_count);

      }

      /*
       *  Call the procedure, passing the number of arguments, a pointer
       *  to the argument array, and a pointer to a return location.
       */

      return_value.sp_form = ft_omega;
      save_pstack_top = pstack_top;

      ex_wait_target = target;
      (*(proc_ptr->p_func_ptr))(SETL_SYSTEM (int)arg_count,
                                pstack + (pstack_top + 1) -
                                   arg_count,
                                &return_value);

      if (target != NULL) {

         unmark_specifier(target);
         target->sp_form = return_value.sp_form;
         target->sp_val.sp_biggest = return_value.sp_val.sp_biggest;

      }
      else {

         unmark_specifier(&return_value);

      }
      return_value.sp_form = ft_omega;

      /* pop the arguments from the program stack */

      return_pstack_top = pstack_top;
      pstack_top = save_pstack_top;

      while (arg_count--) {
         pop_pstack;
      }

      /* slide down any pushed write parameters */

      if (return_pstack_top != save_pstack_top) {

         memcpy((void *)(pstack + pstack_top + 1),
                (void *)(pstack + save_pstack_top + 1),
                (size_t)((return_pstack_top -
                          save_pstack_top) * sizeof(specifier)));

         pstack_top += (return_pstack_top - save_pstack_top);

      }

      return;

   }

   /*
    *  We have lots more work for user-defined procedures.  We have to
    *  worry about saving and restoring environments, saving and
    *  restoring class instances, and saving and restoring local
    *  variables and parameters.
    */

#ifdef TRAPS

   if (proc_ptr->p_type != USERDEF_PROC)
      trap(__FILE__,__LINE__,msg_invalid_proc_type,
           proc_ptr->p_type);

#endif

   if (arg_count != proc_ptr->p_formal_count)
      abend(SETL_SYSTEM msg_actarg_ne_formal,arg_count);

   /* save the root of 'self' if we have one */

   if (self != NULL)
      self_root = self->sp_val.sp_object_ptr;
   else
      self_root = proc_ptr->p_self_ptr;

   /*
    *  If the procedure is really a process, we don't actually branch,
    *  but rather enqueue the request.
    */

   if (self_root != NULL &&
       self_root->o_process_ptr != NULL) {

      self_root->o_use_count++;
      proc_ptr->p_use_count++;

      process_ptr = self_root->o_process_ptr;
      get_request(request_ptr);
      request_ptr->rq_next = NULL;
      *(process_ptr->pc_request_tail) = request_ptr;
      process_ptr->pc_request_tail = &(request_ptr->rq_next);

      request_ptr->rq_proc = proc_ptr;
      request_ptr->rq_args = get_specifiers(SETL_SYSTEM arg_count);
      if (target != NULL) {

         get_mailbox_header(mailbox_ptr);
         request_ptr->rq_mailbox_ptr = mailbox_ptr;
         mailbox_ptr->mb_use_count = 1;
         mailbox_ptr->mb_cell_count = 0;
         mailbox_ptr->mb_head = NULL;
         mailbox_ptr->mb_tail = &(mailbox_ptr->mb_head);

         unmark_specifier(target);
         target->sp_form = ft_mailbox;
         target->sp_val.sp_mailbox_ptr = mailbox_ptr;

      }
      else {
         request_ptr->rq_mailbox_ptr = mailbox_ptr;
      }

      /* copy the actual arguments from the stack, to the formal arguments */

      for (stack_pos = pstack + (pstack_top + 1) - arg_count,
              arg_ptr = request_ptr->rq_args;
           arg_ptr < request_ptr->rq_args + arg_count;
           arg_ptr++, stack_pos++) {

         arg_ptr->sp_form = stack_pos->sp_form;
         arg_ptr->sp_val.sp_biggest = stack_pos->sp_val.sp_biggest;

      }

      pstack_top -= arg_count;

      return;

   }

   /* move the local variables to the program stack */

   save_pstack_top = pstack_top;

   //pstack_pos = pstack + (pstack_top + 1) - arg_count;
   //printf("stack pos %d spec count %d\n",stack_pos,proc_ptr->p_spec_count);
   for (arg_ptr = proc_ptr->p_spec_ptr;
        arg_ptr < proc_ptr->p_spec_ptr + proc_ptr->p_spec_count;
        arg_ptr++) {

      if (++pstack_top == pstack_max)
         alloc_pstack(SETL_SYSTEM_VOID);
      pstack[pstack_top].sp_form = arg_ptr->sp_form;
      pstack[pstack_top].sp_val.sp_biggest = arg_ptr->sp_val.sp_biggest;

      arg_ptr->sp_form = ft_omega;

   }

   /* copy the actual arguments from the stack, to the formal arguments */
   stack_pos = pstack + (save_pstack_top + 1) - arg_count;

   for (arg_ptr = proc_ptr->p_spec_ptr;
        arg_ptr < proc_ptr->p_spec_ptr + arg_count;
        arg_ptr++, stack_pos++) {

      arg_ptr->sp_form = stack_pos->sp_form;
      arg_ptr->sp_val.sp_biggest = stack_pos->sp_val.sp_biggest;

   }

   /* push the return address */

   push_cstack(pc,proc_ptr,target,self,current_class,
               pstack_top,C_return,literal_proc,
               proc_ptr->p_unittab_ptr,EX_BODY_CODE,NULL,extra_code);

   /* branch to the start of the called procedure */

   pc = (proc_ptr->p_unittab_ptr)->ut_body_code +
        proc_ptr->p_offset;

   /* bump this procedure's usage count */

   proc_ptr->p_use_count++;
   proc_ptr->p_active_use_count++;

   /*
    *  We hope most procedure calls will be literal calls, and in fact
    *  experience has shown that to be the case.  In the unfortunate
    *  event that we find a call to a procedure-valued variable, we have
    *  to restore its environment.
    */

   if (!literal_proc) {

      new_proc_ptr = proc_ptr->p_parent;
      while (new_proc_ptr != NULL &&
             (++(new_proc_ptr->p_active_use_count)) == 1) {

         critical_section++;

         /* swap saved and active data */

         for (arg_ptr = new_proc_ptr->p_spec_ptr,
                 stack_ptr = new_proc_ptr->p_save_specs;
              arg_ptr < new_proc_ptr->p_spec_ptr +
                        new_proc_ptr->p_spec_count;
              arg_ptr++, stack_ptr++) {

            memcpy((void *)&spare,
                   (void *)arg_ptr,
                   sizeof(struct specifier_item));

            memcpy((void *)arg_ptr,
                   (void *)stack_ptr,
                   sizeof(struct specifier_item));

            memcpy((void *)stack_ptr,
                   (void *)&spare,
                   sizeof(struct specifier_item));

            spare.sp_form = ft_omega;

         }

         new_proc_ptr = new_proc_ptr->p_parent;

      }
   }

   /*
    *  There are two situations in which we have to load a new 'self'.
    *  The first is in a method call, and the second is in a call to a
    *  procedure which has a previously bound 'self'.
    */

   if (self_root != NULL) {

      class_ptr = self_root->o_ntype.o_root.o_class;
      current_class = class_ptr;

      /* if the current instance is not already loaded, push it */

      if (class_ptr->ut_self != NULL &&
          (class_ptr->ut_self)->ss_object != self_root) {

         /*
          *  The following block of code pushes the current 'self' for
          *  a class.  Notice that we don't have to worry about hash
          *  codes here:  We can't do anything with the object while
          *  it's on the stack, and there can be no other pointers to
          *  the object.  The copy is quicker if we ignore the hash
          *  code.
          */

         object_root = (class_ptr->ut_self)->ss_object;
         object_work_hdr = object_root;
         target_height = class_ptr->ut_obj_height;

         /* loop over the instance variables */

         for (slot_info = class_ptr->ut_first_var, target_number = 0;
              slot_info != NULL;
              slot_info = slot_info->si_next_var, target_number++) {

            /* drop down to a leaf */

            while (target_height) {

               /* extract the element's index at this level */

               target_index = (target_number >>
                                    (target_height * OBJ_SHIFT_DIST)) &
                                 OBJ_SHIFT_MASK;

               /* if the header is missing, allocate one */

               if (object_work_hdr->o_child[target_index].o_header ==
                     NULL) {

                  get_object_header(new_object_hdr);
                  new_object_hdr->o_ntype.o_intern.o_parent =
                     object_work_hdr;
                  new_object_hdr->o_ntype.o_intern.o_child_index =
                     target_index;

                  for (i = 0;
                       i < OBJ_HEADER_SIZE;
                       new_object_hdr->o_child[i++].o_cell = NULL);

                  object_work_hdr->o_child[target_index].o_header =
                        new_object_hdr;
                  object_work_hdr = new_object_hdr;

               }

               /* otherwise just drop down a level */

               else {

                  object_work_hdr =
                     object_work_hdr->o_child[target_index].o_header;

               }

               target_height--;

            }

            /*
             *  At this point, object_work_hdr points to the lowest
             *  level header record.  We insert the new element in the
             *  appropriate slot.
             */

            target_index = target_number & OBJ_SHIFT_MASK;
            object_cell = object_work_hdr->o_child[target_index].o_cell;
            if (object_cell == NULL) {
               get_object_cell(object_cell);
               object_work_hdr->o_child[target_index].o_cell = object_cell;
            }
            target_element = slot_info->si_spec;
            object_cell->o_spec.sp_form = target_element->sp_form;
            object_cell->o_spec.sp_val.sp_biggest =
               target_element->sp_val.sp_biggest;

            /*
             *  We move back up the header tree at this point, if it is
             *  necessary.
             */

            target_index++;
            while (target_index >= OBJ_HEADER_SIZE) {

               target_height++;
               target_index =
                  object_work_hdr->o_ntype.o_intern.o_child_index + 1;
               object_work_hdr =
                  object_work_hdr->o_ntype.o_intern.o_parent;

            }
         }
      }

      /* if the new self is not already there, push it */

      if (class_ptr->ut_self == NULL ||
          (class_ptr->ut_self)->ss_object != self_root) {

         object_root = self_root;
         object_work_hdr = object_root;
         target_height = class_ptr->ut_obj_height;

         /* loop over the instance variables */

         for (slot_info = class_ptr->ut_first_var, target_number = 0;
              slot_info != NULL;
              slot_info = slot_info->si_next_var, target_number++) {

            /* drop down to a leaf */

            while (target_height) {

               /* extract the element's index at this level */

               target_index = (target_number >>
                                    (target_height * OBJ_SHIFT_DIST)) &
                                 OBJ_SHIFT_MASK;

               /* we'll always have all internal nodes in this situation */

               object_work_hdr =
                  object_work_hdr->o_child[target_index].o_header;
               target_height--;

            }

            /*
             *  At this point, object_work_hdr points to the lowest
             *  level header record.  We copy the appropriate element
             *  to the instance variable.
             */

            target_index = target_number & OBJ_SHIFT_MASK;
            object_cell = object_work_hdr->o_child[target_index].o_cell;
            target_element = slot_info->si_spec;
            target_element->sp_form = object_cell->o_spec.sp_form;
            target_element->sp_val.sp_biggest =
               object_cell->o_spec.sp_val.sp_biggest;
            if (self != NULL) {
               mark_specifier(target_element);
            }

            /*
             *  We move back up the header tree at this point, if it is
             *  necessary.
             */

            target_index++;
            while (target_index >= OBJ_HEADER_SIZE) {

               target_height++;
               target_index =
                  object_work_hdr->o_ntype.o_intern.o_child_index + 1;
               object_work_hdr =
                  object_work_hdr->o_ntype.o_intern.o_parent;

            }
         }
      }

      /* if we were passed a self, push an empty root */

      if (self != NULL) {

         get_object_header(self_root);
         self_root->o_use_count = 1;
         self_root->o_ntype.o_root.o_class = class_ptr;
         self_root->o_process_ptr = NULL;

         for (i = 0;
              i < OBJ_HEADER_SIZE;
              self_root->o_child[i++].o_cell = NULL);

      }
      else {

         self_root->o_use_count++;

      }

      /* push the new 'self' */

      get_self_stack(self_stack_ptr);
      self_stack_ptr->ss_object = self_root;
      self_stack_ptr->ss_next = class_ptr->ut_self;
      class_ptr->ut_self = self_stack_ptr;

   }

   /* if we're calling from C, execute */

   if (C_return==YES) {

      critical_section++;
      save_ip = ip;
      execute_go(SETL_SYSTEM YES);
      ip = save_ip;
      critical_section--;

   }

   return;

}

/*\
 *  \function{switch\_process}
\*/

PRTYPE void switch_process(SETL_SYSTEM_PROTO_VOID)

{
int32 arg_count;                       /* number of arguments in request    */

   /* first make sure we're not in a critical section */

   if (critical_section) {
      opcodes_until_switch = 200;
      return;
   }

   /* find an unblocked process */

   for (process_ptr = process_head->pc_next;
        process_ptr != process_head;
        process_ptr = process_ptr->pc_next) {

      if (process_ptr->pc_suspended)
         continue;
      
      if (process_ptr->pc_waiting ||
          process_ptr->pc_checking) {
      
         if (process_unblock(SETL_SYSTEM process_ptr)) 
            break;
         else
            continue;

      }

      if (process_ptr->pc_idle) {
    
         if (process_ptr->pc_request_head != NULL)
            break;
         else
            continue;

      }

      break;
      
   }

   /* if the only unblocked process is the current one, reactivate it */

   if (process_ptr == process_head) {

      if (process_ptr->pc_suspended ||
          ((process_ptr->pc_waiting ||
                process_ptr->pc_checking) &&
            !process_unblock(SETL_SYSTEM process_ptr)) ||
          (process_ptr->pc_idle &&
             process_ptr->pc_request_head == NULL)) {

         abend(SETL_SYSTEM "Deadlock!  No processes can proceed");

      }
   }

   if (process_ptr != process_head) {

      /*
       *  We'll make one pass over the call stack to clear the current flag
       *  on each procedure.  Notice that excess clearing doesn't hurt a
       *  thing.
       */

      for (i = cstack_top; i >= 0; i--) {
         if (cstack[i].cs_proc_ptr != NULL)
            (cstack[i].cs_proc_ptr)->p_current_saved = NO;
         if (cstack[i].cs_self_ptr != NULL)
            (cstack[i].cs_unittab_ptr)->ut_current_saved = NO;
      }

      /*
       *  Now we make a second pass saving local variables.
       */

      process_head->pc_pstack_base = pstack_top;
      for (i = cstack_top; i >= 0; i--) {

         /*
          *  First we save local variables for each procedure.
          */

         if (cstack[i].cs_proc_ptr != NULL &&
             !(cstack[i].cs_proc_ptr)->p_current_saved) {
            
            proc_ptr = cstack[i].cs_proc_ptr;
            proc_ptr->p_current_saved = YES;

            /* save local variables on the stack */

            for (arg_ptr = proc_ptr->p_spec_ptr;
                 arg_ptr < proc_ptr->p_spec_ptr + proc_ptr->p_spec_count;
                 arg_ptr++) {

               if (++pstack_top == pstack_max)
                  alloc_pstack(SETL_SYSTEM_VOID);
               pstack[pstack_top].sp_form = arg_ptr->sp_form;
               pstack[pstack_top].sp_val.sp_biggest =
                   arg_ptr->sp_val.sp_biggest;

               arg_ptr->sp_form = ft_omega;

            }
         }

         /*
          *  Damned nuisance.  For reasons I can't remember, each class
          *  maintains a stack of current instances.  Why didn't I just push
          *  them on the stack?  I have no idea.  However, as I write this
          *  code I'm unwilling to risk taking it out.  Therefore I save
          *  each of those stacks.
          */

         if (cstack[i].cs_self_ptr != NULL &&
             !(cstack[i].cs_unittab_ptr)->ut_current_saved) {

            class_ptr = cstack[i].cs_unittab_ptr;
            class_ptr->ut_current_saved = YES;

            if (++pstack_top == pstack_max)
               alloc_pstack(SETL_SYSTEM_VOID);
            pstack[pstack_top].sp_val.sp_object_ptr =
               class_ptr->ut_self->ss_object;
            class_ptr->ut_self = NULL;

            /* loop over the instance variables */

            for (slot_info = class_ptr->ut_first_var;
                 slot_info != NULL;
                 slot_info = slot_info->si_next_var) {

               if (++pstack_top == pstack_max)
                  alloc_pstack(SETL_SYSTEM_VOID);
               pstack[pstack_top].sp_form =
                  slot_info->si_spec->sp_form;
               pstack[pstack_top].sp_val.sp_biggest =
                  slot_info->si_spec->sp_val.sp_biggest;

               (slot_info->si_spec)->sp_form = ft_omega;

            }
         }
      }

      /*
       *  If we're not saving the root, we have to save the instance
       *  variables.
       */

      if (process_head->pc_type != ROOT_PROCESS) {

         object_root = process_head->pc_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;
         object_work_hdr = object_root;
         target_height = class_ptr->ut_obj_height;

         /* loop over the instance variables */

         for (slot_info = class_ptr->ut_first_var, target_number = 0;
              slot_info != NULL;
              slot_info = slot_info->si_next_var, target_number++) {

            /* drop down to a leaf */

            while (target_height) {

               /* extract the element's index at this level */

               target_index = (target_number >>
                                    (target_height * OBJ_SHIFT_DIST)) &
                                 OBJ_SHIFT_MASK;

               /* if the header is missing, allocate one */

               if (object_work_hdr->o_child[target_index].o_header == NULL) {

                  get_object_header(new_object_hdr);
                  new_object_hdr->o_ntype.o_intern.o_parent =
                     object_work_hdr;
                  new_object_hdr->o_ntype.o_intern.o_child_index =
                     target_index;

                  for (i = 0;
                       i < OBJ_HEADER_SIZE;
                       new_object_hdr->o_child[i++].o_cell = NULL);

                  object_work_hdr->o_child[target_index].o_header =
                        new_object_hdr;
                  object_work_hdr = new_object_hdr;

               }

               /* otherwise just drop down a level */

               else {

                  object_work_hdr =
                     object_work_hdr->o_child[target_index].o_header;

               }

               target_height--;

            }

            /*
             *  At this point, object_work_hdr points to the lowest
             *  level header record.  We insert the new element in the
             *  appropriate slot.
             */

            target_index = target_number & OBJ_SHIFT_MASK;
            object_cell = object_work_hdr->o_child[target_index].o_cell;
            if (object_cell == NULL) {
               get_object_cell(object_cell);
               object_work_hdr->o_child[target_index].o_cell = object_cell;
            }
            target_element = slot_info->si_spec;
            object_cell->o_spec.sp_form = target_element->sp_form;
            object_cell->o_spec.sp_val.sp_biggest =
               target_element->sp_val.sp_biggest;

            /*
             *  We move back up the header tree at this point, if it is
             *  necessary.
             */

            target_index++;
            while (target_index >= OBJ_HEADER_SIZE) {

               target_height++;
               target_index =
                  object_work_hdr->o_ntype.o_intern.o_child_index + 1;
               object_work_hdr =
                  object_work_hdr->o_ntype.o_intern.o_parent;

            }
         }
      }

      /*
       *  OK, all the data from the current process should be safe, so
       *  install the stacks from the new process.
       */

      process_head->pc_pstack_top = pstack_top;
      process_head->pc_pstack_max = pstack_max;
      process_head->pc_pstack = pstack;
      process_head->pc_cstack_top = cstack_top;
      process_head->pc_cstack_max = cstack_max;
      process_head->pc_cstack = cstack;
      process_head->pc_pc = pc;
      process_head->pc_ip = ip;
      process_head->pc_current_class = current_class;

      if (process_ptr->pc_type == CHILD_PROCESS)
         process_ptr->pc_object_ptr->o_use_count++;
      if (process_head->pc_type == CHILD_PROCESS &&
          --process_head->pc_object_ptr->o_use_count <= 0)
         free_object(SETL_SYSTEM process_head->pc_object_ptr);

      pstack_top = process_ptr->pc_pstack_top;
      pstack_max = process_ptr->pc_pstack_max;
      pstack = process_ptr->pc_pstack;
      cstack_top = process_ptr->pc_cstack_top;
      cstack_max = process_ptr->pc_cstack_max;
      cstack = process_ptr->pc_cstack;
      pc = process_ptr->pc_pc;
      ip = process_ptr->pc_ip;
      current_class = process_ptr->pc_current_class;

      /*
       *  Re-install the process object.
       */

      if (process_ptr->pc_type != ROOT_PROCESS) {

         object_root = process_ptr->pc_object_ptr;
         class_ptr = object_root->o_ntype.o_root.o_class;
         object_work_hdr = object_root;
         target_height = class_ptr->ut_obj_height;

         /* loop over the instance variables */

         for (slot_info = class_ptr->ut_first_var, target_number = 0;
              slot_info != NULL;
              slot_info = slot_info->si_next_var, target_number++) {

            /* drop down to a leaf */

            while (target_height) {

               /* extract the element's index at this level */

               target_index = (target_number >>
                                    (target_height * OBJ_SHIFT_DIST)) &
                                 OBJ_SHIFT_MASK;

               /* we'll always have all internal nodes in this situation */

               object_work_hdr =
                  object_work_hdr->o_child[target_index].o_header;
               target_height--;

            }

            /*
             *  At this point, object_work_hdr points to the lowest
             *  level header record.  We copy the appropriate element
             *  to the instance variable.
             */

            target_index = target_number & OBJ_SHIFT_MASK;
            object_cell = object_work_hdr->o_child[target_index].o_cell;
            target_element = slot_info->si_spec;
            target_element->sp_form = object_cell->o_spec.sp_form;
            target_element->sp_val.sp_biggest =
               object_cell->o_spec.sp_val.sp_biggest;

            /*
             *  We move back up the header tree at this point, if it is
             *  necessary.
             */

            target_index++;
            while (target_index >= OBJ_HEADER_SIZE) {

               target_height++;
               target_index =
                  object_work_hdr->o_ntype.o_intern.o_child_index + 1;
               object_work_hdr =
                  object_work_hdr->o_ntype.o_intern.o_parent;

            }
         }
      }

      /*
       *  We'll make one pass over the call stack to clear the current flag
       *  on each procedure.  Notice that excess clearing doesn't hurt a
       *  thing.
       */

      for (i = cstack_top; i >= 0; i--) {
         if (cstack[i].cs_proc_ptr != NULL)
            (cstack[i].cs_proc_ptr)->p_current_saved = NO;
         if (cstack[i].cs_self_ptr != NULL)
            (cstack[i].cs_unittab_ptr)->ut_current_saved = NO;
      }

      /*
       *  Now we make a second pass saving local variables.
       */

      pstack_top = process_ptr->pc_pstack_base;
      for (i = cstack_top; i >= 0; i--) {

         /*
          *  Restore the local variables for each procedure.
          */

         if (cstack[i].cs_proc_ptr != NULL &&
             !(cstack[i].cs_proc_ptr)->p_current_saved) {
            
            proc_ptr = cstack[i].cs_proc_ptr;
            proc_ptr->p_current_saved = YES;

            /* save local variables on the stack */

            for (arg_ptr = proc_ptr->p_spec_ptr;
                 arg_ptr < proc_ptr->p_spec_ptr + proc_ptr->p_spec_count;
                 arg_ptr++) {

               pstack_top++;
               arg_ptr->sp_form = pstack[pstack_top].sp_form;
               arg_ptr->sp_val.sp_biggest =
                  pstack[pstack_top].sp_val.sp_biggest;

            }
         }

         /*
          *  Damned nuisance.  For reasons I can't remember, each class
          *  maintains a stack of current instances.  Why didn't I just push
          *  them on the stack?  I have no idea.  However, as I write this
          *  code I'm unwilling to risk taking it out.  Therefore I save
          *  each of those stacks.
          */

         if (cstack[i].cs_self_ptr != NULL &&
             !(cstack[i].cs_unittab_ptr)->ut_current_saved) {

            class_ptr = cstack[i].cs_unittab_ptr;
            class_ptr->ut_current_saved = YES;

            pstack_top++;
            class_ptr->ut_self->ss_object =
               pstack[pstack_top].sp_val.sp_object_ptr;

            /* loop over the instance variables */

            for (slot_info = class_ptr->ut_first_var;
                 slot_info != NULL;
                 slot_info = slot_info->si_next_var) {

               pstack_top++;
               (slot_info->si_spec)->sp_form = pstack[pstack_top].sp_form;
               (slot_info->si_spec)->sp_val.sp_biggest =
                  pstack[pstack_top].sp_val.sp_biggest;

            }
         }
      }

      pstack_top = process_ptr->pc_pstack_base;

   }

   /*
    *  At this point the switch is finished, if it was necessary.  We
    *  have to handle new requests and wait terminations.
    */

   if (process_ptr->pc_idle) {

      process_ptr->pc_idle = NO;
      request_ptr = process_ptr->pc_request_head;
      proc_ptr = request_ptr->rq_proc;
      arg_count = proc_ptr->p_formal_count;

      /* move the arguments to the formal parameters */

      for (i = 0, arg_ptr = proc_ptr->p_spec_ptr;
           i < arg_count;
           arg_ptr++, i++) {

         arg_ptr->sp_form = (request_ptr->rq_args)[i].sp_form;
         arg_ptr->sp_val.sp_biggest = 
            (request_ptr->rq_args)[i].sp_val.sp_biggest;
         (request_ptr->rq_args)[i].sp_form = ft_omega;

      }

      /* clear any other local variables */

      for (;
           arg_ptr < proc_ptr->p_spec_ptr + proc_ptr->p_spec_count;
           arg_ptr++) {

         arg_ptr->sp_form = ft_omega;

      }


      /* push the return address */

      push_cstack(pc,proc_ptr,NULL,NULL,current_class,
                  pstack_top,NO,YES,
                  proc_ptr->p_unittab_ptr,EX_BODY_CODE,
                  process_ptr,0);

      /* branch to the start of the called procedure */

      pc = (proc_ptr->p_unittab_ptr)->ut_body_code +
           proc_ptr->p_offset;

      /* bump this procedure's usage count */

      proc_ptr->p_use_count++;
      proc_ptr->p_active_use_count++;

   }

   /* if we've just finished waiting, install the return value */

   if (process_ptr->pc_waiting ||
       process_ptr->pc_checking) {

      target = process_ptr->pc_wait_target;
      if (target != NULL) {
         unmark_specifier(target);
         target->sp_form = process_ptr->pc_wait_return.sp_form;
         target->sp_val.sp_biggest =
            process_ptr->pc_wait_return.sp_val.sp_biggest;
      }

      process_ptr->pc_waiting = process_ptr->pc_checking = NO;

   }

   process_head = process_ptr;
   opcodes_until_switch = PROCESS_SLICE;
   

}

/*\
 *  \function{alloc\_pstack()}
 *
 *  This function expands the program stack.
\*/

void alloc_pstack(SETL_SYSTEM_PROTO_VOID)

{
specifier *old_pstack;                 /* temporary program stack           */

   /* expand the table */

   old_pstack = pstack;
   pstack = (specifier *)malloc((size_t)(
                   (pstack_max + PSTACK_BLOCK_SIZE) * sizeof(specifier)));
   if (pstack == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* copy the old table to the new table, and free the old table */

   if (pstack_max > 0) {

      memcpy((void *)pstack,
             (void *)old_pstack,
             (size_t)(pstack_max * sizeof(specifier)));

      free(old_pstack);

   }

   pstack_max += PSTACK_BLOCK_SIZE;

}

/*\
 *  \function{alloc\_cstack()}
 *
 *  This function expands the call stack.
\*/

PRTYPE void alloc_cstack(SETL_SYSTEM_PROTO_VOID)

{
struct call_stack_item *old_cstack;    /* temporary call stack              */

   /* expand the table */

   old_cstack = cstack;
   cstack = (struct call_stack_item *)malloc((size_t)(
      (cstack_max + CSTACK_BLOCK_SIZE) * sizeof(struct call_stack_item)));
   if (cstack == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* copy the old table to the new table, and free the old table */

   if (cstack_max > 0) {

      memcpy((void *)cstack,
             (void *)old_cstack,
             (size_t)(cstack_max * sizeof(struct call_stack_item)));

      free(old_cstack);

   }

   cstack_max += CSTACK_BLOCK_SIZE;

}

/*\
 *  \function{call\_binop\_method()}
 *
 *  This function calls a binary operator method.  Putting it in line
 *  exploded the interpreter.
\*/

PRTYPE void call_binop_method(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target specifier                  */
   specifier *self,                    /* self in call                      */
   specifier *arg,                     /* other operand                     */
   int method_code,                    /* method code                       */
   char *operator,                     /* printable operator string         */
   int extra_code)                     /* extra code to be executed after   */
                                       /* the procedure return              */

{

   object_root = self->sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* get information about this slot in the object */

   slot_info = class_ptr->ut_slot_info + method_code;

   /* make sure we can access this slot */

   if (!slot_info->si_in_class)
      abend(SETL_SYSTEM msg_missing_method,operator,
            class_ptr->ut_name);

   /* push the argument */

   push_pstack(arg);

   /* call the procedure */

   if (extra_code==0) 
      call_procedure(SETL_SYSTEM target,
                     slot_info->si_spec,
                     self,
                     1L,NO,YES,0);
   else 
      call_procedure(SETL_SYSTEM target,
                     slot_info->si_spec,
                     self,
                     1L,EXTRA,YES,extra_code);

   return;

}

#endif                                 /* PARTJ                             */
