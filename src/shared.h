#ifndef SHARED_H_LOADED
#define SHARED_H_LOADED

#ifdef TSAFE

#include "system.h"


typedef struct plugin_item *plugin_item_ptr_type;

#if COMPILER || DYNAMIC_COMP
#include "compiler.h"

#endif

#if INTERP || DYNAMIC_COMP
#include "interp.h"

#include "unittab.h"
#include "specs.h"
#include "procs.h"
#include "process.h"
#include "mailbox.h"
#include "x_files.h"
#include "slots.h"
#include "iters.h"
#include "objects.h"
#include "sets.h"
#include "maps.h"
#include "tuples.h"
#include "x_integers.h"
#include "x_reals.h"
#include "x_strngs.h"
#include "libman.h"
#endif

struct plugin_item {

 /* Common variables */

   char *default_library;
   char *library_path;
   FILE *debug_file;
   int verbose_mode;
   int markup_source;
   long numeval;
   int defining_proc;
   
#if INTERP || DYNAMIC_COMP
   /* interp.h variables */

   char x_source_name[PATH_LENGTH + 1];
   int assert_mode;
   int ex_debug;
   int alloc_debug;
   int prof_debug;
   int copy_debug;
   int step_debug;
   int tracing_on;
   int trace_copies;
   int opcode_executed;
   int32 opcode_count;
   int32 process_slice;
   int eval_package;
   char abend_message[8000];
   int nested_calls;
   int32 wait_flag;
   specifier symbol_map;

   file_ptr_type file_next_free;
   integer_h_ptr_type integer_h_next_free;
   integer_c_ptr_type integer_c_next_free;
   string_h_ptr_type string_h_next_free;
   string_c_ptr_type string_c_next_free;
   mailbox_h_ptr_type mailbox_h_next_free;
   mailbox_c_ptr_type mailbox_c_next_free;
   process_ptr_type process_next_free;
   request_ptr_type request_next_free;
   int32 total_slot_count;
   iter_ptr_type iter_next_free;
   set_h_ptr_type set_h_next_free;
   set_c_ptr_type set_c_next_free;
   map_h_ptr_type map_h_next_free;
   map_c_ptr_type map_c_next_free;
   tuple_h_ptr_type tuple_h_next_free;
   tuple_c_ptr_type tuple_c_next_free;
   i_real_ptr_type real_next_free;
   proc_ptr_type proc_next_free;
   object_h_ptr_type object_h_next_free;
   object_c_ptr_type object_c_next_free;
   self_stack_ptr_type self_stack_next_free;
#endif

#if COMPILER || DYNAMIC_COMP

   /* compiler.h variables */


   int implicit_decls;
   int generate_listing;
   int safety_check;

   int use_intermediate_files;
   int tab_width;
   char c_source_name[PATH_LENGTH + 1];
   char list_fname[PATH_LENGTH + 1];
   FILE *source_file;
   char i1_fname[PATH_LENGTH + 1];
   FILE *i1_file; 
   char i2_fname[PATH_LENGTH + 1];
   struct libfile_item *i2_file;
   struct libfile_item *default_libfile;
   int unit_error_count;
   int file_error_count;
   int total_error_count;
   int unit_warning_count;
   int file_warning_count;
   int total_warning_count;
   int total_global_symbols;
   int compiling_eval;
   int optimize_of;
   int optimize_assop;
   int compiler_options;
   int compiler_symtab; 
 
#if DYNAMIC_COMP 
   char *program_fragment;
   global_ptr_type global_head;
#endif

#ifdef DEBUG
   int prs_debug;
   int lex_debug;
   int sym_debug;
   int ast_debug;
   int proctab_debug;
   int quads_debug;
   int code_debug;
#endif

#endif

 /* Static variables */
#if INTERP
  char abend__source_name[PATH_LENGTH + 1];
  char giveup_message[8000];
  int  already_called;
  char vp_buffer[1024];
  label_record label;
  int first_time;
  char *arg_buffer;
  char *arg_ptr;
  int carg_num;
  struct table_block *table_block_head;
  struct table_item *table_next_free;
  slot_ptr_type hash_table[SLOTS__HASH_TABLE_SIZE];
  struct string_block *string_block_head;                                    
  char *string_block_eos;
  char *string_next_free;
  struct table_block *unittab__table_block_head;
  struct table_item *unittab__table_next_free;
  unittab_ptr_type unittab__hash_table[UNITTAB__HASH_TABLE_SIZE];
  struct string_block *unittab__string_block_head;
  char *unittab__string_block_eos;
  char *unittab__string_next_free;
  setl_destructor *reg_types;
  int32 num_reg_types;
  int32 safe_mode;
  char *safe_prefix;
#endif
};


#endif
#endif
