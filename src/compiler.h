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
 *  \package{Compiler-Specific Definitions}
 *
 *  This package contains constant and variable declarations for names
 *  which are used throughout the compiler, but not at all by the
 *  interpreter.
\*/

#ifndef COMPILER_LOADED
#define COMPILER_LOADED 1

/* SETL2 system header files */

#include "system.h"                    /* system constants                  */

/* storage location structure */

struct storage_location_item {
   void *sl_mem_ptr;                   /* memory pointer                    */
   int32 sl_file_ptr;                  /* file position pointer             */
};

typedef struct storage_location_item storage_location;
                                       /* object storage location           */

#define VERBOSE_FILES     1
#define VERBOSE_OPTIMIZER 2

/* This mess is temporary... */

#ifdef TSAFE

#ifdef TSAFE

#undef SETL_SYSTEM_PROTO
#undef SETL_SYSTEM_PROTO_VOID
#undef SETL_SYSTEM
#undef SETL_SYSTEM_VOID

#define SETL_SYSTEM_PROTO plugin_item_ptr_type plugin_instance,
#define SETL_SYSTEM_PROTO_VOID plugin_item_ptr_type plugin_instance
#define SETL_SYSTEM plugin_instance,
#define SETL_SYSTEM_VOID plugin_instance

#include "shared.h"

#endif

#define DEFAULT_LIBRARY      plugin_instance->default_library
#define LIBRARY_PATH         plugin_instance->library_path
#define VERBOSE_MODE         plugin_instance->verbose_mode
#define MARKUP_SOURCE        plugin_instance->markup_source
#define PROGRAM_FRAGMENT     plugin_instance->program_fragment
#define GLOBAL_HEAD          plugin_instance->global_head
#define IMPLICIT_DECLS       plugin_instance->implicit_decls
#define GENERATE_LISTING     plugin_instance->generate_listing
#define SAFETY_CHECK         plugin_instance->safety_check
#define USE_INTERMEDIATE_FILES plugin_instance->use_intermediate_files
#define TAB_WIDTH            plugin_instance->tab_width
#define C_SOURCE_NAME        plugin_instance->c_source_name
#define LIST_FNAME           plugin_instance->list_fname
#define SOURCE_FILE          plugin_instance->source_file
#define I1_FNAME             plugin_instance->i1_fname
#define I1_FILE              plugin_instance->i1_file
#define I2_FNAME             plugin_instance->i2_fname
#define I2_FILE              plugin_instance->i2_file
#define DEFAULT_LIBFILE      plugin_instance->default_libfile
#define UNIT_ERROR_COUNT     plugin_instance->unit_error_count
#define FILE_ERROR_COUNT     plugin_instance->file_error_count
#define UNIT_WARNING_COUNT   plugin_instance->unit_warning_count
#define FILE_WARNING_COUNT   plugin_instance->file_warning_count
#define TOTAL_ERROR_COUNT    plugin_instance->total_error_count
#define TOTAL_WARNING_COUNT  plugin_instance->total_warning_count
#define TOTAL_GLOBAL_SYMBOLS plugin_instance->total_global_symbols
#define COMPILING_EVAL       plugin_instance->compiling_eval
#define NUMEVAL              plugin_instance->numeval
#define DEFINING_PROC        plugin_instance->defining_proc
#define OPTIMIZE_OF          plugin_instance->optimize_of
#define OPTIMIZE_ASSOP       plugin_instance->optimize_assop
#define COMPILER_OPTIONS     plugin_instance->compiler_options
#define COMPILER_SYMTAB      plugin_instance->compiler_symtab

#ifdef DEBUG
#define DEBUG_FILE           plugin_instance->debug_file
#define PRS_DEBUG            plugin_instance->prs_debug
#define LEX_DEBUG            plugin_instance->lex_debug
#define SYM_DEBUG            plugin_instance->sym_debug
#define AST_DEBUG            plugin_instance->ast_debug
#define PROCTAB_DEBUG        plugin_instance->proctab_debug
#define QUADS_DEBUG          plugin_instance->quads_debug
#define CODE_DEBUG           plugin_instance->code_debug
#endif


#else /* TSAFE */

#define DEFAULT_LIBRARY  default_library
#define LIBRARY_PATH     library_path
#define VERBOSE_MODE     verbose_mode
#define MARKUP_SOURCE    markup_source
#define PROGRAM_FRAGMENT program_fragment
#define GLOBAL_HEAD      global_head
#define IMPLICIT_DECLS   implicit_decls
#define GENERATE_LISTING generate_listing
#define SAFETY_CHECK     safety_check
#define USE_INTERMEDIATE_FILES use_intermediate_files
#define TAB_WIDTH        tab_width
#define C_SOURCE_NAME    c_source_name
#define LIST_FNAME       list_fname
#define SOURCE_FILE      source_file
#define I1_FNAME         i1_fname
#define I1_FILE          i1_file
#define I2_FNAME         i2_fname
#define I2_FILE          i2_file
#define DEFAULT_LIBFILE  default_libfile
#define UNIT_ERROR_COUNT unit_error_count
#define FILE_ERROR_COUNT     file_error_count
#define UNIT_WARNING_COUNT   unit_warning_count
#define FILE_WARNING_COUNT   file_warning_count
#define TOTAL_ERROR_COUNT    total_error_count
#define TOTAL_WARNING_COUNT  total_warning_count
#define TOTAL_GLOBAL_SYMBOLS total_global_symbols
#define COMPILING_EVAL       compiling_eval
#define NUMEVAL              numeval
#define DEFINING_PROC        defining_proc
#define OPTIMIZE_OF          optimize_of
#define OPTIMIZE_ASSOP       optimize_assop
#define COMPILER_OPTIONS     compiler_options
#define COMPILER_SYMTAB      compiler_symtab

#ifdef DEBUG
#ifndef DEBUG_FILE
#define DEBUG_FILE stdout              /* debug list file                   */
#endif
#define PRS_DEBUG            prs_debug
#define LEX_DEBUG            lex_debug
#define SYM_DEBUG            sym_debug
#define AST_DEBUG            ast_debug
#define PROCTAB_DEBUG        proctab_debug
#define QUADS_DEBUG          quads_debug
#define CODE_DEBUG           code_debug
#endif

#ifdef SHARED

#ifndef INTERP
char *default_library = "setl2.lib";   /* default library name              */
char *library_path = "";               /* library search path               */
int markup_source = 0;                 /* markup source file                */
#endif


/* Global variables used in dynamic compilation mode */

#if DYNAMIC_COMP || PLUGIN

char *program_fragment;
global_ptr_type global_head=NULL;

#endif

int implicit_decls = 1;                /* do implicit variable declarations */
int generate_listing = 0;              /* generate a program listing        */
int safety_check = 1;                  /* check for duplicate units before  */
                                       /* storing                           */
int use_intermediate_files = 0;        /* use intermediate files for ast's  */
int tab_width = 8;                     /* tab width                         */
char c_source_name[PATH_LENGTH + 1];     /* source file name                  */
char list_fname[PATH_LENGTH + 1];      /* listing file name                 */
FILE *source_file = NULL;              /* source file handle                */
char i1_fname[PATH_LENGTH + 1];        /* intermediate library file name    */
FILE *i1_file = NULL;                  /* intermediate library file         */
char i2_fname[PATH_LENGTH + 1];        /* intermediate AST file name        */
struct libfile_item *i2_file = NULL;   /* intermediate AST file             */
struct libfile_item *default_libfile = NULL;
                                       /* default library                   */
int unit_error_count = 0;              /* errors in compilation unit        */
int file_error_count = 0;              /* errors in source file             */
int total_error_count = 0;             /* total error count                 */
int unit_warning_count = 0;            /* warnings in compilation unit      */
int file_warning_count = 0;            /* warnings in source file           */
int total_warning_count = 0;           /* total warning count               */
int total_global_symbols = 0;          /* Total number of global symbols    */
                                       /* defined in the EVALs              */
int compiling_eval = NO;               /* YES if we are compiling a fragment*/
long numeval=0;                        /* Incremented when a global proc.   */
                                       /* is defined in an EVAL             */
int defining_proc = NO;                /* YES if we defined a global proc.  */
int optimize_of = NO;
int optimize_assop = NO;
int compiler_options = 0;              /* Verbose modes                     */
int compiler_symtab = NO;

#ifdef DEBUG

#ifndef INTERP
#define debug_file stdout              /* debug list file                   */
#endif
int prs_debug = 0;                     /* debug parser flag                 */
int lex_debug = 0;                     /* debug lexical analyzer flag       */
int sym_debug = 0;                     /* debug symbol table flag           */
int ast_debug = 0;                     /* debug symbol table flag           */
int proctab_debug = 0;                 /* debug procedure table             */
int quads_debug = 0;                   /* debug quadruple lists             */
int code_debug = 0;                    /* debug code generator              */

#endif
#else

/* Global variables used in dynamic compilation mode */

#if DYNAMIC_COMP || PLUGIN

extern char *program_fragment;
extern global_ptr_type global_head;
#endif

extern char *default_library;          /* default library name              */
extern char *library_path;             /* library search path               */
extern int implicit_decls;             /* do implicit variable declarations */
extern int generate_listing;           /* generate a program listing        */
extern int markup_source;              /* markup source file                */
extern int safety_check;               /* check for duplicate units before  */
                                       /* storing                           */
extern int use_intermediate_files;     /* use intermediate files for ast's  */
extern int tab_width;                  /* tab width                         */
extern char c_source_name[PATH_LENGTH + 1];
                                       /* source file name                  */
extern char list_fname[PATH_LENGTH + 1];
                                       /* listing file name                 */
extern FILE *source_file;              /* source file handle                */
extern char i1_fname[PATH_LENGTH + 1];
                                       /* intermediate library file name    */
extern FILE *i1_file;                  /* intermediate library file         */
extern char i2_fname[PATH_LENGTH + 1];
                                       /* intermediate AST file name        */
extern struct libfile_item *i2_file;   /* intermediate AST file             */
extern struct libfile_item *default_libfile;
                                       /* default library                   */
extern int unit_error_count;           /* errors in compilation unit        */
extern int file_error_count;           /* errors in source file             */
extern int total_error_count;          /* total error count                 */
extern int unit_warning_count;         /* warnings in compilation unit      */
extern int file_warning_count;         /* warnings in source file           */
extern int total_warning_count;        /* total warning count               */
extern int total_global_symbols;       /* Total number of global symbols    */
                                       /* defined in the EVALs              */
extern int compiling_eval;
extern long numeval;                   /* Incremented when a global proc.   */
                                       /* is defined in an EVAL             */
extern int defining_proc;              /* YES if we defined a global proc.  */
extern int optimize_of;
extern int optimize_assop;
extern int compiler_options;           /* Verbose modes                     */
extern int compiler_symtab;            /* -g mode                           */

#ifdef DEBUG

#ifndef INTERP
#define debug_file stdout              /* debug list file                   */
#endif
extern int prs_debug;                  /* debug parser flag                 */
extern int lex_debug;                  /* debug lexical analyzer flag       */
extern int sym_debug;                  /* debug symbol table flag           */
extern int ast_debug;                  /* debug symbol table flag           */
extern int proctab_debug;              /* debug procedure table             */
extern int quads_debug;                /* debug quadruple lists             */
extern int code_debug;                 /* debug code generator              */

#endif
#endif
#endif

#endif
