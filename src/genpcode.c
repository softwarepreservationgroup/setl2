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
 *  \package{The Pseudo Code Generator}
 *
 *  The pseudo-code generator is the final phase of the compilation
 *  process.  At this point, we have a list of quadruples for each
 *  procedure, with one quadruple for each pseudo-code instruction we
 *  output.  All we have to do now is translate symbol table pointers to
 *  memory locations, and write the instructions.
 *
 *  \texify{genpcode.h}
 *
 *  \packagebody{Pseudo-Code Generator}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "libman.h"                    /* library manager                   */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "ast.h"                       /* abstract syntax tree              */
#include "quads.h"                     /* quadruples                        */
#include "c_integers.h"                  /* integer literals                  */
#include "c_reals.h"                     /* real literals                     */
#include "c_strngs.h"                    /* string literals                   */
#include "import.h"                    /* imported packages and classes     */

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define fputs   plugin_fputs
#endif

/* standard C header files */

#include <ctype.h>                     /* character macros                  */
#if UNIX_VARARGS
#include <varargs.h>                   /* variable argument macros          */
#else
#include <stdarg.h>                    /* variable argument macros          */
#endif

/* package-global data */

static unit_control_record unit_control;
                                       /* unit control record               */
static libunit_ptr_type libunit_ptr;   /* current unit                      */
static libunit_ptr_type libunit_in;    /* input unit                        */
static libstr_ptr_type bpcode_stream;  /* body pseudo-code stream           */
static libstr_ptr_type ipcode_stream;  /* initialization code unit          */
static libstr_ptr_type spcode_stream;  /* slot initialization code unit     */
static libstr_ptr_type integer_stream; /* integer library stream            */
static libstr_ptr_type real_stream;    /* real number library stream        */
static libstr_ptr_type string_stream;  /* string library stream             */
static libstr_ptr_type procedure_stream;
                                       /* procedure library stream          */
static libstr_ptr_type label_stream;   /* label library stream              */

/* forward declarations */

static void alloc_specifiers(proctab_ptr_type);
                                       /* allocate specifiers from the      */
                                       /* symbol table                      */
static void write_symtab(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* write the symbol table for        */
                                       /* package specifications            */
static void write_public(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* write the symbol table for        */
                                       /* public symbols                    */
static void write_literals(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* write literal values              */
static void write_slots(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* write slot data                   */
static void write_imports(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* write imported package list       */
static void gen_procedure(SETL_SYSTEM_PROTO proctab_ptr_type);
                                       /* generate code for a procedure     */
static void transform_quads(SETL_SYSTEM_PROTO libstr_ptr_type, quad_ptr_type, int32);
                                       /* generate code for quadruple list  */
#if UNIX_VARARGS
static int get_yes_no();               /* query user                        */
#else
static int get_yes_no(char *, ...);    /* query user                        */
#endif

/*\
 *  \function{gen\_pcode()}
 *
 *  This is the entry function for the pseudo-code generation module.  It
 *  is really a driver function for compilation units, calling other
 *  functions to perform all the various duties necessary to write a
 *  compilation unit to the library.
\*/

void gen_pcode(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* package or program pointer        */

{
libstr_ptr_type libstr_ptr;            /* library stream pointer            */
static pcode_record pcode;             /* pseudo code record                */
quad_ptr_type quad_head;               /* head of quadruple list            */
int32 spec_init_offset;                /* to skip package spec offset       */
int32 i;                               /* temporary looping variable        */
symtab_ptr_type symtab_ptr;            /* used to loop over the symbol      */
int32 j;                               /* temporary looping variable        */

#ifdef DYNAMIC_COMP
global_ptr_type temp_global_head,global_ptr;
char *temp_symtab_name;
#endif

#ifdef DEBUG

   if (SYM_DEBUG || QUADS_DEBUG) {

      fprintf(DEBUG_FILE,"\nPSEUDO-CODE GENERATION PHASE\n");
      fprintf(DEBUG_FILE,"============================\n");

   }

#endif

   /* save the compilation unit procedure table pointer */

   unit_proctab_ptr = proctab_ptr;

   /*
    *  If we're compiling a package or class body, we need to get the
    *  specification from the library and load in the control
    *  information.
    */

   if (proctab_ptr->pr_type == pr_package_body ||
       proctab_ptr->pr_type == pr_class_body ||
       proctab_ptr->pr_type == pr_process_body) {

      if ((libunit_in = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                     I2_FILE,LIB_READ_UNIT)) == NULL &&
          (libunit_in = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                                     DEFAULT_LIBFILE,LIB_READ_UNIT)) == NULL)
         giveup(SETL_SYSTEM msg_no_spec,
                (proctab_ptr->pr_namtab_ptr)->nt_name);

      /* read the unit control record */

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_in,LIB_CONTROL_STREAM);
      read_libstr(SETL_SYSTEM libstr_ptr,
                  (char *)&unit_control,
                  sizeof(unit_control_record));
      close_libstr(SETL_SYSTEM libstr_ptr);

      /* initialize body-specific fields */

      strcpy(unit_control.uc_body_source_name,C_SOURCE_NAME);
      time(&unit_control.uc_body_time_stamp);
      unit_control.uc_needs_body = NO;
      unit_control.uc_spec_count = unit_control.uc_sspec_count;

   }

   /*
    *  Other types of compilation units must clear the entire unit
    *  control record, including information about specifications.
    */

   else {

      if (proctab_ptr->pr_type == pr_package_spec)
         unit_control.uc_type = PACKAGE_UNIT;
      else if (proctab_ptr->pr_type == pr_class_spec)
         unit_control.uc_type = CLASS_UNIT;
      else if (proctab_ptr->pr_type == pr_native_package)
         unit_control.uc_type = NATIVE_UNIT;
      else if (proctab_ptr->pr_type == pr_process_spec)
         unit_control.uc_type = PROCESS_UNIT;
      else
         unit_control.uc_type = PROGRAM_UNIT;

      strcpy(unit_control.uc_spec_source_name,C_SOURCE_NAME);
      strcpy(unit_control.uc_body_source_name,C_SOURCE_NAME);
      time(&unit_control.uc_time_stamp);

      if ((proctab_ptr->pr_type == pr_package_spec ||
              proctab_ptr->pr_type == pr_class_spec ||
              proctab_ptr->pr_type == pr_process_spec) &&
          proctab_ptr->pr_child != NULL)
         unit_control.uc_needs_body = YES;
      else
         unit_control.uc_needs_body = NO;

      unit_control.uc_spec_count = 0;
      unit_control.uc_sspec_count = 0;
      unit_control.uc_sipcode_count = 0;
      unit_control.uc_csipcode_count = 0;

   }

   /* common initialization */

   unit_control.uc_import_count = 0;
   unit_control.uc_inherit_count = 0;
   unit_control.uc_unit_count = proctab_ptr->pr_unit_count;
   unit_control.uc_symtab_count = 0;
   unit_control.uc_ipcode_count = 0;
   unit_control.uc_bpcode_count = 0;
   unit_control.uc_integer_count = 0;
   unit_control.uc_real_count = 0;
   unit_control.uc_string_count = 0;
   unit_control.uc_proc_count = 0;
   unit_control.uc_label_count = 0;
   unit_control.uc_slot_count = 0;
   unit_control.uc_max_slot = 0;
   unit_control.uc_line_count = 0;

   /* open the output unit */

   libunit_ptr = open_libunit(SETL_SYSTEM (proctab_ptr->pr_namtab_ptr)->nt_name,
                              I2_FILE,LIB_WRITE_UNIT);

   /* open all the library streams we'll need */

   integer_stream = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INTEGER_STREAM);
   real_stream = open_libstr(SETL_SYSTEM libunit_ptr,LIB_REAL_STREAM);
   string_stream = open_libstr(SETL_SYSTEM libunit_ptr,LIB_STRING_STREAM);
   procedure_stream = open_libstr(SETL_SYSTEM libunit_ptr,LIB_PROCEDURE_STREAM);
   label_stream = open_libstr(SETL_SYSTEM libunit_ptr,LIB_LABEL_STREAM);
   ipcode_stream = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INIT_STREAM);
   spcode_stream = open_libstr(SETL_SYSTEM libunit_ptr,LIB_SLOT_STREAM);
   bpcode_stream = open_libstr(SETL_SYSTEM libunit_ptr,LIB_PCODE_STREAM);

   /*
    *  Find the offset locations of specifiers and instructions.  After
    *  that is complete, find slot numbers and write the slots.
    */

   alloc_specifiers(proctab_ptr);
   write_slots(SETL_SYSTEM proctab_ptr);

   /* 
    * In case of dynamic compilation, identify the global variables 
    * and the global procedures and add them to the global list...
    *
    */

#ifdef DYNAMIC_COMP
   if ((proctab_ptr->pr_type == pr_program)&&(COMPILING_EVAL==YES))   {

      for (symtab_ptr = proctab_ptr->pr_symtab_head;
         symtab_ptr != NULL; symtab_ptr = symtab_ptr->st_thread) {

	 if (((symtab_ptr->st_type == sym_procedure)||
	      (symtab_ptr->st_type == sym_id)) &&
	     (symtab_ptr->st_namtab_ptr != NULL) &&
	     (symtab_ptr->st_unit_num == 1))  {
	 
            global_ptr = GLOBAL_HEAD;
            while (global_ptr != NULL ) {
               if (strcmp(symtab_ptr->st_namtab_ptr->nt_name,
                         global_ptr->gl_name)==0)  {
                  if (symtab_ptr->st_type == sym_id) { /* Only ids for now.. */
                     symtab_ptr->st_unit_num=2;
                     symtab_ptr->st_offset=global_ptr->gl_number;
                  } else {
                     global_ptr->gl_offset=symtab_ptr->st_offset;
                     DEFINING_PROC = YES;
	          }
		  global_ptr->gl_type=symtab_ptr->st_type;
                  break;
               }
               global_ptr=global_ptr->gl_next_ptr;
            }

            if (global_ptr==NULL) {

               if (symtab_ptr->st_type == sym_id) { /* Only ids for now.. */
 	           symtab_ptr->st_unit_num=2;
 	           symtab_ptr->st_offset=(TOTAL_GLOBAL_SYMBOLS++)+1;
               } 

               /* If the symbol is a procedure, inform the run time */
  
               if (symtab_ptr->st_type == sym_procedure) 
                  DEFINING_PROC = YES;
	
               /* Now add the symbol to the global variables list */
	
               temp_global_head = (global_ptr_type)
                         malloc(sizeof(struct global_item));
               if (temp_global_head == NULL)
                  giveup(SETL_SYSTEM msg_malloc_error);
 
               temp_global_head->gl_offset=-1;
               temp_global_head->gl_number=symtab_ptr->st_offset;

               if (symtab_ptr->st_type == sym_procedure) { 
                   temp_global_head->gl_offset=symtab_ptr->st_offset;
                   temp_global_head->gl_number=(TOTAL_GLOBAL_SYMBOLS++)+1;
               } 
		
               temp_global_head->gl_global=symtab_ptr->st_global_var;
               temp_global_head->gl_type=symtab_ptr->st_type;

               temp_symtab_name = symtab_ptr->st_namtab_ptr->nt_name;
               temp_global_head->gl_name =
                      (char *) malloc(strlen(temp_symtab_name)+1);

               if (temp_global_head->gl_name == NULL)
                  giveup(SETL_SYSTEM msg_malloc_error);
      
               strcpy(temp_global_head->gl_name,temp_symtab_name);
               temp_global_head->gl_next_ptr = GLOBAL_HEAD;
               GLOBAL_HEAD = temp_global_head;

	    }
         }
      }
   }
#endif

   if (proctab_ptr->pr_type == pr_package_spec ||
       proctab_ptr->pr_type == pr_native_package ||
       proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      unit_control.uc_sspec_count = unit_control.uc_spec_count;

   }

   /*
    *  If we are compiling a class or package, we have to save the symbol
    *  table.
    */

   if (proctab_ptr->pr_type == pr_package_spec ||
       proctab_ptr->pr_type == pr_native_package ||
       proctab_ptr->pr_type == pr_package_body ||
       proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_class_body ||
       proctab_ptr->pr_type == pr_process_spec ||
       proctab_ptr->pr_type == pr_process_body) {
      write_symtab(SETL_SYSTEM proctab_ptr);
   }
   if (proctab_ptr->pr_type == pr_package_spec ||
       proctab_ptr->pr_type == pr_native_package ||
       proctab_ptr->pr_type == pr_package_body) {
      if (!COMPILER_SYMTAB)
         write_public(SETL_SYSTEM proctab_ptr);
   }
   if (COMPILER_SYMTAB)
          write_public(SETL_SYSTEM proctab_ptr);


   /*
    *  If we're compiling a package or class body, we need to copy the
    *  intialization quadruples from the specification.
    */

   if (proctab_ptr->pr_type == pr_package_body ||
       proctab_ptr->pr_type == pr_class_body ||
       proctab_ptr->pr_type == pr_process_body) {

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_in,LIB_INIT_STREAM);

      spec_init_offset = unit_control.uc_sipcode_count;
      for (i = 0; i < unit_control.uc_sipcode_count; i++) {

         read_libstr(SETL_SYSTEM libstr_ptr,(char *)&pcode,sizeof(pcode_record));
         write_libstr(SETL_SYSTEM ipcode_stream,(char *)&pcode,sizeof(pcode_record));

      }

      unit_control.uc_ipcode_count+= unit_control.uc_sipcode_count;
      close_libstr(SETL_SYSTEM libstr_ptr);

   }
   else {

      spec_init_offset = 0;

   }

   /* load the initialization quadruples */

   quad_head = load_quads(SETL_SYSTEM &(proctab_ptr->pr_init_code));

#ifdef DEBUG

   /* print the quadruples and symbol table if desired */

   if (SYM_DEBUG || QUADS_DEBUG) {

      fprintf(DEBUG_FILE,"\n%s : %s\n",
              (proctab_ptr->pr_namtab_ptr)->nt_name,
              proctab_desc[proctab_ptr->pr_type]);

      if (SYM_DEBUG)
         print_symtab(SETL_SYSTEM proctab_ptr);

      if (QUADS_DEBUG) {
         print_quads(SETL_SYSTEM quad_head,"Initialization Code");
      }
   }

#endif

   /* generate initialization code */

   transform_quads(SETL_SYSTEM ipcode_stream,
                   quad_head,
                   proctab_ptr->pr_init_offset + spec_init_offset);

   /* save a count of the initialization code, for later copying */

   if (proctab_ptr->pr_type == pr_package_spec ||
       proctab_ptr->pr_type == pr_native_package ||
       proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_process_spec) {

      unit_control.uc_sipcode_count = unit_control.uc_ipcode_count;

   }

   /* we're through with the quadruples */

   kill_quads(quad_head);

   /*
    *  If we're compiling a class specification, we have to save the slot
    *  initialization code.  This will eventually become part of the
    *  procedure 'InitObj' for the class.
    */

   if (proctab_ptr->pr_type == pr_class_spec) {

      quad_head = load_quads(SETL_SYSTEM &(proctab_ptr->pr_slot_code));

#ifdef DEBUG

      /* print the quadruples if desired */

      if (QUADS_DEBUG) {

         if (QUADS_DEBUG)
            print_quads(SETL_SYSTEM quad_head,"Slot Initialization Code");

      }

#endif

      /* generate code for slot initialization */

      transform_quads(SETL_SYSTEM spcode_stream,
                      quad_head,
                      0L);
      unit_control.uc_csipcode_count = proctab_ptr->pr_sinit_count;

      /* we're through with the quadruples */

      kill_quads(quad_head);

   }

   /*
    *  If we're compiling a class specification, we have to save the slot
    *  initialization code.  This will eventually become part of the
    *  procedure 'InitObj' for the class.
    */

   if (proctab_ptr->pr_type == pr_process_spec) {

      quad_head = load_quads(SETL_SYSTEM &(proctab_ptr->pr_slot_code));

#ifdef DEBUG

      /* print the quadruples if desired */

      if (QUADS_DEBUG) {

         if (QUADS_DEBUG)
            print_quads(SETL_SYSTEM quad_head,"Slot Initialization Code");

      }

#endif

      /* generate code for slot initialization */

      transform_quads(SETL_SYSTEM spcode_stream,
                      quad_head,
                      0L);
      unit_control.uc_csipcode_count = proctab_ptr->pr_sinit_count;

      /* we're through with the quadruples */

      kill_quads(quad_head);

   }

   /*
    *  If we are generating code for a program, we have to transform the
    *  main program body.
    */

   if (proctab_ptr->pr_type == pr_program) {

      /* load the body code */

      quad_head = load_quads(SETL_SYSTEM &(proctab_ptr->pr_body_code));

#ifdef DEBUG

      if (QUADS_DEBUG)
         print_quads(SETL_SYSTEM quad_head,"Body Code");

#endif

      transform_quads(SETL_SYSTEM bpcode_stream,
                      quad_head,
                      proctab_ptr->pr_body_offset);

      /* we're through with the quadruples */

      kill_quads(quad_head);

   }

   /*
    *  If we are compiling a package body, a class body, or a program we
    *  need to generate and save the code for embedded procedures.
    */

   if (proctab_ptr->pr_type == pr_package_body ||
       proctab_ptr->pr_type == pr_class_body ||
       proctab_ptr->pr_type == pr_process_body ||
       proctab_ptr->pr_type == pr_program) {

      gen_procedure(SETL_SYSTEM proctab_ptr->pr_child);

   }

   /*
    *  Package bodies, class specifications, class bodies, and programs
    *  have lists of used and inherited units which must be saved.
    */

   if (proctab_ptr->pr_type == pr_package_body ||
       proctab_ptr->pr_type == pr_class_spec ||
       proctab_ptr->pr_type == pr_class_body ||
       proctab_ptr->pr_type == pr_process_spec ||
       proctab_ptr->pr_type == pr_process_body ||
       proctab_ptr->pr_type == pr_program) {

      write_imports(SETL_SYSTEM proctab_ptr);

   }

   /* write the literal values */

   write_literals(SETL_SYSTEM proctab_ptr);

   /* write the unit control record */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_CONTROL_STREAM);
   write_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
                sizeof(unit_control_record));
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* we're done with the compilation unit */

   close_libstr(SETL_SYSTEM integer_stream);
   close_libstr(SETL_SYSTEM real_stream);
   close_libstr(SETL_SYSTEM string_stream);
   close_libstr(SETL_SYSTEM procedure_stream);
   close_libstr(SETL_SYSTEM label_stream);
   close_libstr(SETL_SYSTEM ipcode_stream);
   close_libstr(SETL_SYSTEM spcode_stream);
   close_libstr(SETL_SYSTEM bpcode_stream);

   close_libunit(SETL_SYSTEM libunit_ptr);

   if (proctab_ptr->pr_type == pr_package_body ||
       proctab_ptr->pr_type == pr_class_body ||
       proctab_ptr->pr_type == pr_process_body) {

      close_libunit(SETL_SYSTEM libunit_in);

   }

   return;

}

/*\
 *  \function{alloc\_specifiers()}
 *
 *  This recursive function allocates all the specifiers for a unit and
 *  computes the starting address for each procedure. It scans the
 *  symbols for each procedure, and sets the offset for each symbol
 *  requiring storage.
 *
 *  We should remind the reader that the symbol table for each procedure
 *  was built so that any parameters appear first in the symbol table
 *  list, which is necessary for the procedure entry and exit opcodes to
 *  work properly.
\*/

static void alloc_specifiers(
   proctab_ptr_type proctab_ptr)       /* unit to be processed              */

{
symtab_ptr_type symtab_ptr;            /* used to loop over the symbol      */
                                       /* table                             */

   /* loop over compilation units */

   while (proctab_ptr != NULL) {

      /* set the starting address for the procedure */

      proctab_ptr->pr_body_offset = unit_control.uc_bpcode_count;
      proctab_ptr->pr_entry_offset = unit_control.uc_bpcode_count;
      proctab_ptr->pr_init_offset = unit_control.uc_ipcode_count;
      unit_control.uc_bpcode_count += proctab_ptr->pr_body_count;
      unit_control.uc_ipcode_count += proctab_ptr->pr_init_count;

      if (proctab_ptr->pr_type == pr_method &&
          proctab_ptr->pr_method_code == m_initobj) {
         proctab_ptr->pr_body_offset += unit_control.uc_csipcode_count;
         unit_control.uc_bpcode_count += unit_control.uc_csipcode_count;
      }

      /*
       *  loop over the symbol table, allocating labels, procedures, and
       *  constants
       */

      for (symtab_ptr = proctab_ptr->pr_symtab_head;
           symtab_ptr != NULL;
           symtab_ptr = symtab_ptr->st_thread) {

         /* allocate labels */

         if (symtab_ptr->st_type == sym_label &&
             symtab_ptr->st_needs_stored &&
             !(symtab_ptr->st_is_alloced)) {

            symtab_ptr->st_unit_num = 1;
            symtab_ptr->st_offset = unit_control.uc_spec_count++;
            symtab_ptr->st_is_alloced = YES;

            continue;

         }

         /* allocate procedures */

         if (symtab_ptr->st_type == sym_procedure &&
             symtab_ptr->st_needs_stored &&
             !(symtab_ptr->st_is_alloced)) {

            symtab_ptr->st_unit_num = 1;
            symtab_ptr->st_offset = unit_control.uc_spec_count++;
            symtab_ptr->st_is_alloced = YES;

            continue;

         }

         /* allocate constants */

         if (symtab_ptr->st_has_rvalue &&
             !symtab_ptr->st_has_lvalue &&
             !symtab_ptr->st_is_rparam &&
             symtab_ptr->st_needs_stored &&
             !(symtab_ptr->st_is_alloced)) {

            symtab_ptr->st_unit_num = 1;
            symtab_ptr->st_offset = unit_control.uc_spec_count++;
            symtab_ptr->st_is_alloced = YES;

            continue;

         }
      }

      /* loop over the symbol table, allocating variables */

      for (symtab_ptr = proctab_ptr->pr_symtab_head;
           symtab_ptr != NULL;
           symtab_ptr = symtab_ptr->st_thread) {

         if (symtab_ptr->st_needs_stored && !(symtab_ptr->st_is_alloced)) {

            if (proctab_ptr->pr_spec_offset == -1)
               proctab_ptr->pr_spec_offset = unit_control.uc_spec_count;

            proctab_ptr->pr_symtab_count++;
            symtab_ptr->st_unit_num = 1;
            symtab_ptr->st_offset = unit_control.uc_spec_count++;
            symtab_ptr->st_is_alloced = YES;

         }
      }

      /* allocate space for children */

      if ((proctab_ptr->pr_type != pr_package_spec) &&
          (proctab_ptr->pr_type != pr_native_package))
         alloc_specifiers(proctab_ptr->pr_child);

      /* set up for next procedure */

      if (proctab_ptr->pr_type == pr_procedure ||
          proctab_ptr->pr_type == pr_method)
         proctab_ptr = proctab_ptr->pr_next;
      else
         proctab_ptr = NULL;

   }
}

/*\
 *  \function{write\_literals()}
 *
 *  This function stores specifier values where necessary.
\*/

static void write_literals(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* compilation unit                  */

{
symtab_ptr_type symtab_ptr;            /* used to loop over symbol table    */
proctab_ptr_type parent_ptr;           /* parent procedure                  */
static integer_record integer;         /* integer literal record            */
integer_ptr_type integer_ptr;          /* integer cell pointer              */
static real_record real;               /* real literal record               */
static string_record string;           /* string literal record             */
static proc_record proc;               /* procedure address record          */
static label_record label;             /* procedure address record          */

   /* loop over compilation units */

   while (proctab_ptr != NULL) {

      for (symtab_ptr = proctab_ptr->pr_symtab_head;
           symtab_ptr != NULL;
           symtab_ptr = symtab_ptr->st_thread) {

         if (!(symtab_ptr->st_is_alloced))
            continue;
         if (symtab_ptr->st_unit_num != 1)
            continue;

         switch (symtab_ptr->st_type) {

            case sym_integer :

               integer.ir_offset = symtab_ptr->st_offset;
               integer.ir_cell_count = 0;

               integer_ptr = symtab_ptr->st_aux.st_integer_ptr;
               do {
                  integer.ir_cell_count++;
                  integer_ptr = integer_ptr->i_next;
               } while (integer_ptr != symtab_ptr->st_aux.st_integer_ptr);

               write_libstr(SETL_SYSTEM integer_stream,
                            (char *)&integer,
                            sizeof(integer_record));

               integer_ptr = symtab_ptr->st_aux.st_integer_ptr;
               do {

                  write_libstr(SETL_SYSTEM integer_stream,
                               (char *)(&(integer_ptr->i_value)),
                               sizeof(int32));

                  integer_ptr = integer_ptr->i_next;

               } while (integer_ptr != symtab_ptr->st_aux.st_integer_ptr);

               unit_control.uc_integer_count++;

               break;

            case sym_real :

               real.rr_offset = symtab_ptr->st_offset;
               real.rr_value = (symtab_ptr->st_aux.st_real_ptr)->r_value;

               write_libstr(SETL_SYSTEM real_stream,
                            (char *)&real,
                            sizeof(real_record));

               unit_control.uc_real_count++;

               break;

            case sym_string :

               string.sr_offset = symtab_ptr->st_offset;
               string.sr_length =
                   (symtab_ptr->st_aux.st_string_ptr)->s_length;

               write_libstr(SETL_SYSTEM string_stream,
                            (char *)&string,
                            sizeof(string_record));
               write_libstr(SETL_SYSTEM string_stream,
                            (symtab_ptr->st_aux.st_string_ptr)->s_value,
                            string.sr_length);

               unit_control.uc_string_count++;

               break;

            case sym_procedure :

               proc.pr_symtab_offset = symtab_ptr->st_offset;
               parent_ptr =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_parent;
               if (parent_ptr->pr_type == pr_procedure ||
                   parent_ptr->pr_type == pr_method)
                  proc.pr_parent_offset =
                     (parent_ptr->pr_symtab_ptr)->st_offset;
               else
                  proc.pr_parent_offset = -1;
               proc.pr_proc_offset =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_entry_offset;
               proc.pr_spec_offset =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_spec_offset;
               if (proc.pr_spec_offset == -1)
                  proc.pr_spec_offset = 0;
               proc.pr_formal_count =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_formal_count;
               proc.pr_spec_count =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_symtab_count;

               write_libstr(SETL_SYSTEM procedure_stream,
                            (char *)&proc,
                            sizeof(proc_record));
               unit_control.uc_proc_count++;

               break;

            case sym_method :

               proc.pr_symtab_offset = symtab_ptr->st_offset;
               parent_ptr =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_parent;
               if (parent_ptr->pr_type == pr_procedure ||
                   parent_ptr->pr_type == pr_method)
                  proc.pr_parent_offset =
                     (parent_ptr->pr_symtab_ptr)->st_offset;
               else
                  proc.pr_parent_offset = -1;
               proc.pr_proc_offset =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_entry_offset;
               proc.pr_spec_offset =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_spec_offset;
               if (proc.pr_spec_offset == -1)
                  proc.pr_spec_offset = 0;
               proc.pr_formal_count =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_formal_count;
               proc.pr_spec_count =
                  (symtab_ptr->st_aux.st_proctab_ptr)->pr_symtab_count;

               write_libstr(SETL_SYSTEM procedure_stream,
                            (char *)&proc,
                            sizeof(proc_record));
               unit_control.uc_proc_count++;

               break;

            case sym_label :

               label.lr_symtab_offset = symtab_ptr->st_offset;
               label.lr_label_offset = symtab_ptr->st_aux.st_label_offset;
               if (label.lr_label_offset < 0)
                  label.lr_label_offset -= proctab_ptr->pr_init_offset;
               else
                  label.lr_label_offset += proctab_ptr->pr_body_offset;

               write_libstr(SETL_SYSTEM label_stream,
                            (char *)&label,
                            sizeof(label_record));
               unit_control.uc_label_count++;

               break;

         }
      }

      /* process children */

      if ((proctab_ptr->pr_type != pr_package_spec) &&
          (proctab_ptr->pr_type != pr_native_package))
         write_literals(SETL_SYSTEM proctab_ptr->pr_child);

      /* set up for next procedure */

      if (proctab_ptr->pr_type == pr_procedure ||
          proctab_ptr->pr_type == pr_method)
         proctab_ptr = proctab_ptr->pr_next;
      else
         proctab_ptr = NULL;

   }
}

/*\
 *  \function{write\_slots()}
 *
 *  This function numbers all the slots used in the compilation unit with
 *  numbers unique and uniform for each name.  These numbers will be used
 *  in the pseudo-code instructions, and translated into other unique
 *  numbers when the unit is loaded.
\*/

static void write_slots(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* compilation unit                  */

{
symtab_ptr_type symtab_ptr;            /* used to loop over symbol table    */
libstr_ptr_type libstr_ptr;            /* library stream pointer            */
static slot_record slot;               /* slot record                       */

   /* open up the slot stream */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_DSLOT_STREAM);
   unit_control.uc_max_slot = m_user + 1;

   /* now we write each active slot */

   for (symtab_ptr = proctab_ptr->pr_symtab_head;
        symtab_ptr != NULL;
        symtab_ptr = symtab_ptr->st_thread) {

      if (symtab_ptr->st_type != sym_slot &&
          symtab_ptr->st_type != sym_method)
         continue;

      if (!(symtab_ptr->st_is_visible_slot))
         continue;

      /* we're ready to write out the slot */

      slot.sl_number = symtab_ptr->st_slot_num;
      slot.sl_in_class = (symtab_ptr->st_class == proctab_ptr);
      slot.sl_is_method = (symtab_ptr->st_type == sym_method);
      slot.sl_is_public = symtab_ptr->st_is_public;
      slot.sl_unit_num = symtab_ptr->st_unit_num;
      slot.sl_offset = symtab_ptr->st_offset;
      slot.sl_name_length =
            strlen((symtab_ptr->st_namtab_ptr)->nt_name);

      write_libstr(SETL_SYSTEM libstr_ptr,
                   (char *)&slot,
                   sizeof(slot_record));

      write_libstr(SETL_SYSTEM libstr_ptr,
                   (symtab_ptr->st_namtab_ptr)->nt_name,
                   (size_t)slot.sl_name_length);

      unit_control.uc_slot_count++;
      unit_control.uc_max_slot = max(unit_control.uc_max_slot,
                                     symtab_ptr->st_slot_num);

   }

   close_libstr(SETL_SYSTEM libstr_ptr);

}

/*\
 *  \function{write\_imports)}
 *
 *  This function writes the import list.
\*/

static void write_imports(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* compilation unit pointer          */

{
libstr_ptr_type libstr_ptr;            /* library stream pointer            */
import_ptr_type import_ptr;            /* used to loop over import list     */
static import_record import;           /* imported package record           */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_IMPORT_STREAM);

   for (import_ptr = proctab_ptr->pr_import_list;
        import_ptr != NULL;
        import_ptr = import_ptr->im_next) {

      strcpy(import.ir_name,(import_ptr->im_namtab_ptr)->nt_name);
      strcpy(import.ir_source_name,import_ptr->im_source_name);
      import.ir_time_stamp = import_ptr->im_time_stamp;

      write_libstr(SETL_SYSTEM libstr_ptr,(char *)&import,
                   sizeof(import_record));
      unit_control.uc_import_count++;

   }

   close_libstr(SETL_SYSTEM libstr_ptr);

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_INHERIT_STREAM);

   for (import_ptr = proctab_ptr->pr_inherit_list;
        import_ptr != NULL;
        import_ptr = import_ptr->im_next) {

      strcpy(import.ir_name,(import_ptr->im_namtab_ptr)->nt_name);
      strcpy(import.ir_source_name,import_ptr->im_source_name);
      import.ir_time_stamp = import_ptr->im_time_stamp;

      write_libstr(SETL_SYSTEM libstr_ptr,(char *)&import,
                   sizeof(import_record));
      unit_control.uc_inherit_count++;

   }

   close_libstr(SETL_SYSTEM libstr_ptr);

}

/*\
 *  \function{write\_symtab()}
 *
 *  This function writes the public symbols and the formal parameter
 *  names of public functions for package specifications.  We save the
 *  formal parameters only to check for errors on function definitions,
 *  they are not visible to users of the package.
\*/

static void write_symtab(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* package specification             */

{
namtab_ptr_type namtab_ptr;            /* name of symbol to be written      */
symtab_ptr_type symtab_ptr;            /* used to loop over symbol table    */
symtab_ptr_type proc_ptr;              /* procedure symbol table pointer    */
int param_count;                       /* formals left to be written        */
libstr_ptr_type libstr_ptr;            /* symbol table stream               */
symtab_record symtab;                  /* symbol table record               */
int selector_length;                   /* selector name length              */

   /* open the symbol table stream */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_SYMTAB_STREAM);

   /* start out at the top level -- compilation unit level */

   proc_ptr = NULL;
   param_count = 0;
   symtab_ptr = proctab_ptr->pr_symtab_head;

   while (symtab_ptr != NULL) {

      /* skip anything imported */

      if (symtab_ptr->st_unit_num > 1) {

         symtab_ptr = symtab_ptr->st_thread;
         continue;

      }

      /* flag specification symbols */

      if (proctab_ptr->pr_type == pr_package_spec ||
          proctab_ptr->pr_type == pr_native_package ||
          proctab_ptr->pr_type == pr_class_spec ||
          proctab_ptr->pr_type == pr_process_spec) {

         symtab_ptr->st_in_spec = YES;

      }

      if (!(symtab_ptr->st_needs_stored) ||
          symtab_ptr->st_type == sym_class ||
          symtab_ptr->st_type == sym_process) {

         symtab_ptr = symtab_ptr->st_thread;
         continue;

      }

      /* write the symbol */

      memcpy((void *)&(symtab.sr_symtab_item),
             (void *)symtab_ptr,
             sizeof(struct symtab_item));
      namtab_ptr = symtab_ptr->st_namtab_ptr;
      if (namtab_ptr != NULL)
         symtab.sr_name_length = strlen(namtab_ptr->nt_name);
      else
         symtab.sr_name_length = 0;

      /*
       *  if the parameter count is greater than zero, we're writing
       *  formal parameters
       */

      if (param_count > 0) {

         /* write the symbol table record and the name string */

         write_libstr(SETL_SYSTEM libstr_ptr,
                      (char *)&symtab,
                      sizeof(symtab_record));

         if (namtab_ptr != NULL) {

            write_libstr(SETL_SYSTEM libstr_ptr,
                         (char *)(namtab_ptr->nt_name),
                         symtab.sr_name_length);

         }

         unit_control.uc_symtab_count++;

         /* when the formal count reaches zero, return to package spec */

         if (!(--param_count))
            symtab_ptr = proc_ptr->st_thread;
         else
            symtab_ptr = symtab_ptr->st_thread;

      }

      /* if we find a procedure, store its formal parameters */

      else if (symtab_ptr->st_type == sym_procedure ||
               symtab_ptr->st_type == sym_method) {

         param_count =
            (symtab_ptr->st_aux.st_proctab_ptr)->pr_formal_count;
         symtab.sr_param_count = param_count;

         /* write the symbol table record and the name string */

         write_libstr(SETL_SYSTEM libstr_ptr,
                      (char *)&symtab,
                      sizeof(symtab_record));

         if (namtab_ptr != NULL) {

            write_libstr(SETL_SYSTEM libstr_ptr,
                         (char *)(namtab_ptr->nt_name),
                         symtab.sr_name_length);

         }

         unit_control.uc_symtab_count++;

         if (param_count > 0) {

            proc_ptr = symtab_ptr;
            symtab_ptr =
               (symtab_ptr->st_aux.st_proctab_ptr)->pr_symtab_head;

         }
         else {

            symtab_ptr = symtab_ptr->st_thread;

         }
      }

      /* if we find a selector, store its key */

      else if (symtab_ptr->st_type == sym_selector) {

         /* write the symbol table record and the name string */

         write_libstr(SETL_SYSTEM libstr_ptr,
                      (char *)&symtab,
                      sizeof(symtab_record));

         if (namtab_ptr != NULL) {

            write_libstr(SETL_SYSTEM libstr_ptr,
                         (char *)(namtab_ptr->nt_name),
                         symtab.sr_name_length);

         }
        
         unit_control.uc_symtab_count++;

         namtab_ptr = (symtab_ptr->st_aux.st_selector_ptr)->st_namtab_ptr;
         selector_length = strlen(namtab_ptr->nt_name);

         write_libstr(SETL_SYSTEM libstr_ptr,
                      (char *)&selector_length,
                      sizeof(int));

         if (namtab_ptr != NULL) {

            write_libstr(SETL_SYSTEM libstr_ptr,
                         (char *)(namtab_ptr->nt_name),
                         selector_length);

         }

         symtab_ptr = symtab_ptr->st_thread;

      }
      else {

         /* write the symbol table record and the name string */

         write_libstr(SETL_SYSTEM libstr_ptr,
                      (char *)&symtab,
                      sizeof(symtab_record));

         if (namtab_ptr != NULL) {

            write_libstr(SETL_SYSTEM libstr_ptr,
                         (char *)(namtab_ptr->nt_name),
                         symtab.sr_name_length);

         }

         unit_control.uc_symtab_count++;

         symtab_ptr = symtab_ptr->st_thread;

      }
   }

   close_libstr(SETL_SYSTEM libstr_ptr);

}

/*\
 *  \function{write\_public()}
 *
 *  This function writes the names of public functions for package
 *  specifications.
\*/

static void write_public(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* package specification             */

{
namtab_ptr_type namtab_ptr;            /* name of symbol to be written      */
symtab_ptr_type symtab_ptr;            /* used to loop over symbol table    */
libstr_ptr_type libstr_ptr;            /* symbol table stream               */
public_record pub;                     /* public symbol record              */
char print_symbol[16];
char *name;

   /* open the symbol table stream */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr,LIB_PUBLIC_STREAM);

   /* loop over symbols at the top level */

   symtab_ptr = proctab_ptr->pr_symtab_head;
   while (symtab_ptr != NULL) {

      if (!COMPILER_SYMTAB) {
         if (symtab_ptr->st_unit_num > 1 ||
             symtab_ptr->st_type != sym_procedure ||
             !(symtab_ptr->st_in_spec) ||
             symtab_ptr->st_is_temp) {
            symtab_ptr = symtab_ptr->st_thread;
            continue;
         }
         namtab_ptr = symtab_ptr->st_namtab_ptr;
         if (namtab_ptr != NULL) {
            pub.pu_name_length = strlen(namtab_ptr->nt_name);
         } else {
            symtab_ptr = symtab_ptr->st_thread;
            continue;
         }
         name = (char *)(namtab_ptr->nt_name);
      } else {

         if (symtab_ptr->st_namtab_ptr == NULL) {

            if (symtab_ptr->st_type == sym_label)
               sprintf(print_symbol,"$L%ld ",(long)symtab_ptr);
            else
               sprintf(print_symbol,"$T%ld ",(long)symtab_ptr);

            pub.pu_name_length = strlen(print_symbol);
            name = print_symbol;

         } else {
            namtab_ptr = symtab_ptr->st_namtab_ptr;
            name = (char *)(namtab_ptr->nt_name);
            pub.pu_name_length = strlen(namtab_ptr->nt_name);

         }
      }

      pub.pu_offset = symtab_ptr->st_offset;

      write_libstr(SETL_SYSTEM libstr_ptr,
                   (char *)&pub,
                   sizeof(public_record));
      write_libstr(SETL_SYSTEM libstr_ptr,
                   (char *)(name),
                   pub.pu_name_length);

      symtab_ptr = symtab_ptr->st_thread;


   }

   close_libstr(SETL_SYSTEM libstr_ptr);

}

/*\
 *  \function{gen\_procedure()}
 *
 *  This function generates code for a procedure, and calls itself
 *  recursively to generate code for its children.
\*/

static void gen_procedure(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* unit to be processed              */

{
libstr_ptr_type libstr_in;             /* input stream                      */
pcode_record pcode;                    /* pseudo code record                */
quad_ptr_type quad_head;               /* quadruple list                    */
int operand;                           /* used to loop over operands        */
int32 i;                               /* temporary looping variable        */

   /* loop over procedures */

   while (proctab_ptr != NULL) {

      /* load the symbol table and quadruples for this procedure */

      quad_head = load_quads(SETL_SYSTEM &(proctab_ptr->pr_init_code));

#ifdef DEBUG

      /* print the quadruples and symbol table if desired */

      if (SYM_DEBUG || QUADS_DEBUG) {

         fprintf(DEBUG_FILE,"\n%s : %s\n",
                 (proctab_ptr->pr_namtab_ptr)->nt_name,
                 proctab_desc[proctab_ptr->pr_type]);

         if (SYM_DEBUG)
            print_symtab(SETL_SYSTEM proctab_ptr);

         if (QUADS_DEBUG)
            print_quads(SETL_SYSTEM quad_head,"Initialization Code");

      }

#endif

      /* generate code for each quadruple list */

      transform_quads(SETL_SYSTEM ipcode_stream,
                      quad_head,
                      proctab_ptr->pr_init_offset);
      kill_quads(quad_head);

      /*
       *  Copy the 'initobj' procedure initialization code from the
       *  package spec.
       */

      if (proctab_ptr->pr_type == pr_method &&
          proctab_ptr->pr_method_code == m_initobj) {

         libstr_in = open_libstr(SETL_SYSTEM libunit_in,LIB_SLOT_STREAM);

         for (i = 0; i < unit_control.uc_csipcode_count; i++) {

            read_libstr(SETL_SYSTEM libstr_in,(char *)&pcode,sizeof(pcode_record));

            for (operand = 0; operand < 3; operand++) {

               if (pcode_optype[pcode.pr_opcode][operand] == PCODE_INST_OP) {
                  pcode.pr_offset[operand] += proctab_ptr->pr_entry_offset;
               }
            }

            write_libstr(SETL_SYSTEM bpcode_stream,(char *)&pcode,sizeof(pcode_record));

         }

         close_libstr(SETL_SYSTEM libstr_in);

      }

      /* load the body code */

      quad_head = load_quads(SETL_SYSTEM &(proctab_ptr->pr_body_code));

#ifdef DEBUG

      if (QUADS_DEBUG)
         print_quads(SETL_SYSTEM quad_head,"Body Code");

#endif

      transform_quads(SETL_SYSTEM bpcode_stream,
                      quad_head,
                      proctab_ptr->pr_body_offset);
      kill_quads(quad_head);

      /* generate code for children */

      gen_procedure(SETL_SYSTEM proctab_ptr->pr_child);

      /* set up for next procedure */

      proctab_ptr = proctab_ptr->pr_next;

   }
}

/*\
 *  \function{transform\_quads()}
 *
 *  This function traverses a list of quadruples into pseudo-code, and
 *  writes it to the library.
\*/

static void transform_quads(
   SETL_SYSTEM_PROTO
   libstr_ptr_type libstr_ptr,         /* code stream                       */
   quad_ptr_type quad_ptr,             /* head of quadruple list            */
   int32 label_base)                   /* base address for labels           */

{
symtab_ptr_type opnd_symtab_ptr;       /* symbol table operand              */
pcode_record pcode;                    /* pseudo code record                */
int operand;                           /* used to loop over operands        */

   /* loop over the quadruple list */

   while (quad_ptr != NULL) {

      pcode.pr_opcode = pcode_opcode[quad_ptr->q_opcode];
      copy_file_pos(&pcode.pr_file_pos,&(quad_ptr->q_file_pos));

      for (operand = 0; operand < 3; operand++) {

         switch (quad_optype[quad_ptr->q_opcode][operand]) {

            case QUAD_INTEGER_OP :

               pcode.pr_unit_num[operand] = 0;
               pcode.pr_offset[operand] =
                     quad_ptr->q_operand[operand].q_integer;

               break;

            case QUAD_SPEC_OP :

               opnd_symtab_ptr =
                     quad_ptr->q_operand[operand].q_symtab_ptr;

               if (opnd_symtab_ptr == NULL) {
                  pcode.pr_unit_num[operand] = -1;
                  pcode.pr_offset[operand] = 0;
               }
               else {
                  pcode.pr_unit_num[operand] =
                     opnd_symtab_ptr->st_unit_num;
                  pcode.pr_offset[operand] =
                     opnd_symtab_ptr->st_offset;
               }

               break;

            case QUAD_LABEL_OP :

               pcode.pr_unit_num[operand] = 0;
               if (quad_ptr->q_operand[operand].q_integer < 0) {
                  pcode.pr_offset[operand] = -label_base +
                        quad_ptr->q_operand[operand].q_integer;
               }
               else {
                  pcode.pr_offset[operand] = label_base +
                        quad_ptr->q_operand[operand].q_integer;
               }

               break;

            case QUAD_SLOT_OP :

               opnd_symtab_ptr =
                     quad_ptr->q_operand[operand].q_symtab_ptr;

               if (opnd_symtab_ptr == NULL) {
                  pcode.pr_unit_num[operand] = -1;
                  pcode.pr_offset[operand] = 0;
               }
               else {
                  pcode.pr_unit_num[operand] =
                     opnd_symtab_ptr->st_unit_num;
                  pcode.pr_offset[operand] =
                     opnd_symtab_ptr->st_slot_num;
               }

               break;

            case QUAD_CLASS_OP :

               opnd_symtab_ptr =
                     quad_ptr->q_operand[operand].q_symtab_ptr;

               if (opnd_symtab_ptr == NULL) {
                  pcode.pr_unit_num[operand] = -1;
                  pcode.pr_offset[operand] = 0;
               }
               else {
                  pcode.pr_unit_num[operand] =
                     opnd_symtab_ptr->st_unit_num;
                  pcode.pr_offset[operand] =
                     opnd_symtab_ptr->st_unit_num;
               }

               break;

            case QUAD_PROCESS_OP :

               opnd_symtab_ptr =
                     quad_ptr->q_operand[operand].q_symtab_ptr;

               if (opnd_symtab_ptr == NULL) {
                  pcode.pr_unit_num[operand] = -1;
                  pcode.pr_offset[operand] = 0;
               }
               else {
                  pcode.pr_unit_num[operand] =
                     opnd_symtab_ptr->st_unit_num;
                  pcode.pr_offset[operand] =
                     opnd_symtab_ptr->st_unit_num;
               }

               break;

         }
      }

      /* the record is built -- write it */

      write_libstr(SETL_SYSTEM libstr_ptr,(char *)&pcode,sizeof(pcode_record));

      quad_ptr = quad_ptr->q_next;

   }
}

/*\
 *  \function{add\_file\_unit()}
 *
 *  This function copies a file into a library.  I do this to reduce
 *  clutter on installation directories, by storing configuration files
 *  in libraries.
\*/

void add_file_unit(SETL_SYSTEM_PROTO char *file_name, char *unit_name)
{
libunit_ptr_type libunit_ptr;          /* work library unit pointer         */
libstr_ptr_type textstr_ptr, lenstr_ptr;
                                       /* text stream and length stream     */
libstr_ptr_type libstr_ptr;            /* library stream pointer            */
unit_control_record unit_control;      /* unit control record               */
static char buffer[256];               /* I/O buffer                        */
int32 length;                          /* length of current record          */
FILE *handle;                          /* handle of input file              */
char *p, *s;
int i;
int state;

   /* open the libraries */

   open_lib();
   I2_FILE = add_lib_file(SETL_SYSTEM I2_FNAME,YES);
   DEFAULT_LIBFILE = add_lib_file(SETL_SYSTEM DEFAULT_LIBRARY, YES);
   add_lib_path(SETL_SYSTEM LIBRARY_PATH);

   /*
    *  First we do some error checking.  We check if there is an existing
    *  unit in the library with the same name, and if so we check whether
    *  it can be replaced.
    */

   if ((libunit_ptr = open_libunit(SETL_SYSTEM unit_name, I2_FILE,
                                   LIB_READ_UNIT)) != NULL ||
       (libunit_ptr = open_libunit(SETL_SYSTEM unit_name, DEFAULT_LIBFILE,
                                   LIB_READ_UNIT)) != NULL) {

      /* read the unit control record */

      libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr, LIB_CONTROL_STREAM);
      read_libstr(SETL_SYSTEM libstr_ptr, (char *)&unit_control,
                  sizeof(unit_control_record));
      close_libstr(SETL_SYSTEM libstr_ptr);
      close_libunit(SETL_SYSTEM libunit_ptr);

      if (strcmp(unit_control.uc_spec_source_name, file_name) != 0) {

         if (SAFETY_CHECK &&
             !get_yes_no(msg_existing_unit, unit_name,
             unit_control.uc_spec_source_name)) {

            error_message(NULL, "Unit not added: %s", unit_name);
            return;

         }
      }
      else if (unit_control.uc_type != FILE_UNIT) {

         if (SAFETY_CHECK &&
             !get_yes_no("%s is not a file. Overwrite? ", unit_name)) {

            error_message(NULL, "Unit not added: %s", unit_name);
            return;

         }
      }

   }

   /*
    *  Even file units need a control record, but most of the fields are
    *  irrelevant.
    */

   unit_control.uc_type = FILE_UNIT;
   strcpy(unit_control.uc_spec_source_name, file_name);
   strcpy(unit_control.uc_body_source_name, file_name);
   time(&unit_control.uc_time_stamp);
   unit_control.uc_needs_body = NO;
   unit_control.uc_spec_count = 0;
   unit_control.uc_sspec_count = 0;
   unit_control.uc_sipcode_count = 0;
   unit_control.uc_csipcode_count = 0;
   unit_control.uc_import_count = 0;
   unit_control.uc_inherit_count = 0;
   unit_control.uc_unit_count = 0;
   unit_control.uc_symtab_count = 0;
   unit_control.uc_ipcode_count = 0;
   unit_control.uc_bpcode_count = 0;
   unit_control.uc_integer_count = 0;
   unit_control.uc_real_count = 0;
   unit_control.uc_string_count = 0;
   unit_control.uc_proc_count = 0;
   unit_control.uc_label_count = 0;
   unit_control.uc_slot_count = 0;
   unit_control.uc_max_slot = 0;
   unit_control.uc_line_count = 0;

   /* open the output unit */

   libunit_ptr = open_libunit(SETL_SYSTEM unit_name, I2_FILE, LIB_WRITE_UNIT);
   handle = fopen(file_name, BINARY_RD);
   if (handle == NULL) {
      giveup(SETL_SYSTEM "Can not open file %s", file_name);
   }

   /*
    *  We use two streams for the file, one for lengths and one for the
    *  text of lines.  We'll treat both newlines and carriage returns as
    *  line separators, but more than one of either means we have two
    *  blank lines.
    */

   textstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr, LIB_TEXT_STREAM);
   lenstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr, LIB_LENGTH_STREAM);

   length = 0;
   state = 0;
   while (state != 999) {

      switch (state) {

         case 0:
            i = fread((void *)buffer, (size_t)1, 256, handle);
            if (i == 0) {
               state = 999;
               break;
            }
            p = s = buffer;
            length = 0;
            state = 1;
            break;

         case 1:
            if (p >= buffer + i) {
               write_libstr(SETL_SYSTEM textstr_ptr, s, (int32)(p - s));
               i = fread((void *)buffer, (size_t)1, 256, handle);
               if (i == 0) {
                  write_libstr(SETL_SYSTEM lenstr_ptr, (char *)(&length), sizeof(int32));
                  state = 999;
                  break;
               }
               p = s = buffer;
               break;
            }

            if (*p == '\n' || *p == '\r') {
               write_libstr(SETL_SYSTEM textstr_ptr, s, (int32)(p - s));
               write_libstr(SETL_SYSTEM lenstr_ptr, (char *)(&length), sizeof(int32));
               length = 0;
               unit_control.uc_line_count++;
               if (*p == '\n')
                  state = 2;
               else
                  state = 3;
               p++;
               break;
            }

            p++;
            length++;
            break;

         case 2:
            if (p >= buffer + i) {
               i = fread((void *)buffer, (size_t)1, 256, handle);
               if (i == 0) {
                  state = 999;
                  break;
               }
               p = s = buffer;
               break;
            }

            if (*p == '\r') {
               p++;
               s = p;
               state = 1;
               break;
            }

            s = p;
            state = 1;
            break;

         case 3:
            if (p >= buffer + i) {
               i = fread((void *)buffer, (size_t)1, 256, handle);
               if (i == 0) {
                  state = 999;
                  break;
               }
               p = s = buffer;
               break;
            }

            if (*p == '\n') {
               p++;
               s = p;
               state = 1;
               break;
            }

            s = p;
            state = 1;
            break;

      }
   }

   fclose(handle);

   /* write the unit control record */

   libstr_ptr = open_libstr(SETL_SYSTEM libunit_ptr, LIB_CONTROL_STREAM);
   write_libstr(SETL_SYSTEM libstr_ptr,(char *)&unit_control,
                sizeof(unit_control_record));
   close_libstr(SETL_SYSTEM libstr_ptr);

   /* we're done with the compilation unit */

   close_libstr(SETL_SYSTEM textstr_ptr);
   close_libstr(SETL_SYSTEM lenstr_ptr);
   close_libunit(SETL_SYSTEM libunit_ptr);

   copy_libunit(SETL_SYSTEM unit_name, I2_FILE, DEFAULT_LIBFILE);

   close_lib(SETL_SYSTEM_VOID);
   I2_FILE = NULL;

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
