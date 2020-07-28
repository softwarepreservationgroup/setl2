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
 *  \package{Interpreter-Specific Definitions}
 *
 *  This package contains constant and variable declarations for names
 *  which are used throughout the interpreter, but not at all by the
 *  compiler.
\*/

#ifndef INTERP_LOADED
#define INTERP_LOADED 1

/* SETL2 system header files */

#include "system.h"                    /* system constants                  */

#ifndef TSAFE
#include "unittab.h"
#include "specs.h"
#endif

#ifdef HAVE_GETRUSAGE
#include "timeval.h"
#endif

/* destructors for registered types */

struct destructor_item {
   char *name;
   void *function;
};
typedef struct destructor_item setl_destructor;

/* instruction structure */

struct instruction_item {
   int i_opcode;
   union {
      struct specifier_item *i_spec_ptr;
      void (*i_func_ptr)(int, struct specifier_item *);
      struct instruction_item *i_inst_ptr;
      int32 i_integer;
      struct unittab_item *i_class_ptr;
      int32 i_slot;
   } i_operand[3];
};
typedef struct instruction_item instruction;

#ifdef DEBUG
struct profiler_item {
   long count;
   long copies;
#ifdef HAVE_GETRUSAGE
   struct timeval time;
   struct timeval timec;
#endif
};
typedef struct profiler_item profiler;
#endif

#define ASSERT_FAIL 1
#define ASSERT_LOG  2

#ifdef TSAFE


#include "shared.h"

plugin_item_ptr_type Setl_Initialize();

#define DEBUG_FILE 	plugin_instance->debug_file
#define ASSERT_MODE 	plugin_instance->assert_mode
#define EX_DEBUG 	plugin_instance->ex_debug
#define ALLOC_DEBUG 	plugin_instance->alloc_debug
#define PROF_DEBUG 	plugin_instance->prof_debug
#define COPY_DEBUG 	plugin_instance->copy_debug
#define STEP_DEBUG 	plugin_instance->step_debug
#define TRACING_ON 	plugin_instance->tracing_on
#define TRACE_COPIES 	plugin_instance->trace_copies
#define OPCODE_EXECUTED	plugin_instance->opcode_executed
#define MARKUP_SOURCE 	plugin_instance->markup_source
#define DEFAULT_LIBRARY	plugin_instance->default_library
#define LIBRARY_PATH 	plugin_instance->library_path
#define OPCODE_COUNT 	plugin_instance->opcode_count
#define PROCESS_SLICE 	plugin_instance->process_slice
#define NESTED_CALLS 	plugin_instance->nested_calls
#define ABEND_MESSAGE  	plugin_instance->abend_message
#define EVAL_PACKAGE   	plugin_instance->eval_package
#define VERBOSE_MODE  	plugin_instance->verbose_mode
#define WAIT_FLAG 	plugin_instance->wait_flag
#define SYMBOL_MAP	plugin_instance->symbol_map
#define X_SOURCE_NAME   plugin_instance->x_source_name
#define NUMEVAL         plugin_instance->numeval
#define DEFINING_PROC   plugin_instance->defining_proc
#define GLOBAL_HEAD     plugin_instance->global_head
#define REG_TYPES       plugin_instance->reg_types
#define NUM_REG_TYPES   plugin_instance->num_reg_types
#define SAFE_MODE 	plugin_instance->safe_mode
#define SAFE_PREFIX	plugin_instance->safe_prefix

#else 

void Setl_Initialize();


#define DEBUG_FILE 	debug_file
#define ASSERT_MODE 	assert_mode
#define EX_DEBUG 	ex_debug
#define ALLOC_DEBUG 	alloc_debug
#define PROF_DEBUG 	prof_debug
#define COPY_DEBUG 	copy_debug
#define STEP_DEBUG 	step_debug
#define TRACING_ON 	tracing_on
#define TRACE_COPIES 	trace_copies
#define OPCODE_EXECUTED opcode_executed
#define MARKUP_SOURCE 	markup_source
#define DEFAULT_LIBRARY default_library
#define LIBRARY_PATH 	library_path
#define OPCODE_COUNT 	opcode_count
#define PROCESS_SLICE 	process_slice
#define NESTED_CALLS 	nested_calls
#define ABEND_MESSAGE   abend_message
#define EVAL_PACKAGE    eval_package
#define VERBOSE_MODE  	verbose_mode
#define WAIT_FLAG 	wait_flag
#define SYMBOL_MAP	symbol_map
#define X_SOURCE_NAME   x_source_name
#define NUMEVAL         numeval
#define DEFINING_PROC   defining_proc
#define GLOBAL_HEAD     global_head
#define REG_TYPES       reg_types
#define NUM_REG_TYPES   num_reg_types
#define SAFE_MODE 	safe_mode
#define SAFE_PREFIX	safe_prefix

#ifdef SHARED

FILE *debug_file;                      /* debug list file                   */
int assert_mode = 0;                   /* assert mode                       */
int ex_debug = 0;                      /* debug execute flag                */
int alloc_debug = 0;                   /* debug alloc flag                  */
int prof_debug = 0;                    /* profiling mode                    */
int copy_debug = 0;                    /* debug copy operations             */
int step_debug = 0;                    /* single step mode                  */
int tracing_on = 0;                    /* YES if we're in trace mode        */
int trace_copies = 0;                  /* YES if we want to trace copies    */
int opcode_executed;                   /* Opcode being executed             */
int markup_source = 0;                 /* write abend information for       */
                                       /* source file markup                */
char *default_library = "setl2.lib";   /* default library name              */
char *library_path = "";               /* library search path               */
int32 opcode_count = 0;                /* number of opcodes executed        */
int32 process_slice = 2000;            /* number of opcodes in one slice    */
int nested_calls = 0;                  /* forbid recursive calls            */
char abend_message[8000];              /* error message                     */
int eval_package = 0;                  /* dummy package compiled?           */
int verbose_mode=0;
int32 wait_flag = 0;                   /* used by the wait statement to exit*/
specifier symbol_map;
setl_destructor *reg_types;
int32 num_reg_types;

#else

extern FILE *debug_file;               /* debug list file                   */
extern int assert_mode;                /* assert mode                       */
extern int ex_debug;                   /* debug execute flag                */
extern int alloc_debug;                /* debug alloc flag                  */
extern int prof_debug;                 /* profiling mode                    */
extern int copy_debug;                 /* debug copy operations             */
extern int step_debug;                 /* single step mode                  */
extern int tracing_on;                 /* YES if we're in trace mode        */
extern int trace_copies;               /* YES if we want to trace copies    */
extern int opcode_executed;            /* Opcode being executed             */
extern int markup_source;              /* write abend information for       */
                                       /* source file markup                */
extern char *default_library;          /* default library name              */
extern char *library_path;             /* library search path               */
extern int32 opcode_count;             /* number of opcodes executed        */
extern int32 process_slice;            /* number of opcodes in one slice    */
extern int eval_package;               /* dummy package compiled?           */
extern char abend_message[8000];       /* error message                     */
extern int nested_calls;
extern int verbose_mode;
extern int32 wait_flag;                /* used by the wait statement to exit*/
extern specifier symbol_map;
extern setl_destructor *reg_types;
extern int32 num_reg_types;
#endif
#endif

void Setl_SetVerboseMode(SETL_SYSTEM_PROTO int mode);
void Setl_InitInterpreter(SETL_SYSTEM_PROTO_VOID);
void Setl_PrintVersion(SETL_SYSTEM_PROTO_VOID);

#endif
