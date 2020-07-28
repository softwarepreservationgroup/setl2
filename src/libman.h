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
 *  \packagespec{Library Manager}
 *
 *  The specification of the library manager is really split into two
 *  header files.  This header file contains anything used by files
 *  outside the library manager which access it, and another file which
 *  contains anything used exclusively within the library manager, but in
 *  more than one file of the library manager.  It is one of those
 *  unusual situations in which we would like to have the full block
 *  structure of Ada, rather than the two level name space provided by C.
 */

#ifndef LIBMAN_LOADED

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#ifdef COMPILER
#include "symtab.h"                    /* symbol table                      */
#endif

/* special library name used for dynamic compilation */

#define MEM_LIB_NAME           "<mem_lib>"

/* unit access modes */

#define LIB_READ_UNIT          0       /* unit read mode                    */
#define LIB_WRITE_UNIT         1       /* unit write mode                   */

/* unit types */

#define PACKAGE_UNIT           0       /* packages (specification and body) */
#define CLASS_UNIT             1       /* classes (specification and body)  */
#define PROCESS_UNIT           2       /* process (similar to class)        */
#define PROGRAM_UNIT           3       /* program                           */
#define FILE_UNIT              4       /* file as tuple                     */
#define NATIVE_UNIT            5       /* native package specs              */

/* stream indices */

#define LIB_CONTROL_STREAM     0       /* unit control record               */
#define LIB_IMPORT_STREAM      1       /* imported packages list            */
#define LIB_INHERIT_STREAM     2       /* inherited classes list            */
#define LIB_SYMTAB_STREAM      3       /* symbol table                      */
#define LIB_INIT_STREAM        4       /* initialization code               */
#define LIB_SLOT_STREAM        5       /* initialization code               */
#define LIB_PCODE_STREAM       6       /* body code                         */
#define LIB_INTEGER_STREAM     7       /* integer values                    */
#define LIB_REAL_STREAM        8       /* real values                       */
#define LIB_STRING_STREAM      9       /* string values                     */
#define LIB_PROCEDURE_STREAM  10       /* procedure information             */
#define LIB_LABEL_STREAM      11       /* label values                      */
#define LIB_DSLOT_STREAM      12       /* defined slots                     */
#define LIB_PUBLIC_STREAM     13       /* public symbols                    */
#define LIB_TEXT_STREAM        1       /* text of file unit                 */
#define LIB_LENGTH_STREAM      2       /* line lengths of file unit         */

/*\
 *  \function{Record Formats}
\*/

/* unit control record */

struct unit_control_item {
   int uc_type;                        /* program or package                */
   char uc_spec_source_name[PATH_LENGTH + 1];
                                       /* source file name                  */
   char uc_body_source_name[PATH_LENGTH + 1];
                                       /* body source name                  */
   time_t uc_time_stamp;               /* time stamp                        */
   time_t uc_body_time_stamp;          /* body time stamp                   */
   int uc_needs_body;                  /* YES if body not compiled          */
   int32 uc_import_count;              /* number of imported packages       */
   int32 uc_inherit_count;             /* number of inherited classes       */
   int32 uc_unit_count;                /* total referenced units            */
   int32 uc_symtab_count;              /* number of public symbols          */
   int32 uc_spec_count;                /* number of specifiers              */
   int32 uc_ipcode_count;              /* initialization pseudo-code        */
   int32 uc_bpcode_count;              /* body pseudo-code                  */
   int32 uc_integer_count;             /* number of integer literals        */
   int32 uc_real_count;                /* number of real literals           */
   int32 uc_string_count;              /* number of string literals         */
   int32 uc_proc_count;                /* number of procedure constants     */
   int32 uc_label_count;               /* number of label values            */
   int32 uc_sspec_count;               /* specification spec count          */
   int32 uc_csipcode_count;            /* class specification init          */
                                       /* pseudo-code                       */
   int32 uc_sipcode_count;             /* specification initialization      */
                                       /* pseudo-code                       */
   int32 uc_slot_count;                /* number of slots                   */
   int32 uc_max_slot;                  /* highest slot number               */
   int32 uc_line_count;                /* line count for file unit          */
};

typedef struct unit_control_item unit_control_record;
                                       /* unit control record               */

/* imported package record */

struct import_record_item {
   char ir_name[MAX_UNIT_NAME+1];      /* package name                      */
   char ir_source_name[PATH_LENGTH + 1];
                                       /* package source file               */
   time_t ir_time_stamp;               /* time package compiled             */
};

typedef struct import_record_item import_record;
                                       /* imported package record           */

#ifdef COMPILER

/* symbol table record (for package specifications) */

struct symtab_record_item {
   struct symtab_item sr_symtab_item;  /* symtab item from symbol table     */
   int sr_name_length;                 /* symbol name length                */
   int sr_param_count;                 /* number of parameters, if proc     */
};

typedef struct symtab_record_item symtab_record;
                                       /* symbol table record               */

#endif

/* integer literal value record */

struct integer_record_item {
   unsigned int32 ir_offset;           /* specifier to get integer value    */
   unsigned ir_cell_count;             /* number of cells in integer        */
};

typedef struct integer_record_item integer_record;
                                       /* integer literal record            */

/* real literal value record */

struct real_record_item {
   unsigned int32 rr_offset;           /* specifier to get real value       */
   double rr_value;                    /* literal value                     */
};

typedef struct real_record_item real_record;
                                       /* real literal record               */

/* string literal value record */

struct string_record_item {
   unsigned int32 sr_offset;           /* specifier to get string value     */
   int sr_length;                      /* string length                     */
};

typedef struct string_record_item string_record;
                                       /* integer literal record            */

/* procedure record */

struct proc_record_item {
   int32 pr_symtab_offset;             /* offset of procedure (symbol)      */
   int32 pr_proc_offset;               /* offset of procedure (code)        */
   int32 pr_spec_offset;               /* procedure data in unit            */
   int32 pr_parent_offset;             /* parent's specifier                */
   int pr_formal_count;                /* number of formal paramenters      */
   int32 pr_spec_count;                /* number of specifiers in proc      */
};

typedef struct proc_record_item proc_record;
                                       /* procedure record                  */

/* label record */

struct label_record_item {
   int32 lr_symtab_offset;             /* offset of label (symbol)          */
   int32 lr_label_offset;              /* offset of label (code)            */
};

typedef struct label_record_item label_record;
                                       /* label record                      */

/* pseudo code instruction */

struct pcode_item {
   int pr_opcode;                      /* opcode                            */
   int pr_unit_num[3];                 /* segment part of operand address   */
   int32 pr_offset[3];                 /* offset part of operand address    */
   struct file_pos_item pr_file_pos;   /* file position for ABENDS          */
};

typedef struct pcode_item pcode_record;
                                       /* pseudo-code record                */

/* slot name record */

struct slot_record_item {
   int32 sl_number;                    /* slot value number                 */
   unsigned sl_in_class : 1;           /* YES if in this class              */
   unsigned sl_is_method : 1;          /* YES if method                     */
   unsigned sl_is_public : 1;          /* YES if exported                   */
   int sl_unit_num;                    /* owning unit number                */
   int32 sl_offset;                    /* offset within unit                */
   int sl_name_length;                 /* length of name                    */
};

typedef struct slot_record_item slot_record;
                                       /* slot record                       */

struct public_record_item {
   int32 pu_offset;                    /* location of procedure             */
   int32 pu_name_length;               /* length of symbol name             */
};

typedef struct public_record_item public_record;

/*\
 *  \nobar\newpage
\*/

/* typedefs used in public function declarations */

typedef struct libfile_item *libfile_ptr_type;
                                       /* pointer to library file           */

typedef struct libunit_item *libunit_ptr_type;
                                       /* compilation unit pointer          */

typedef struct libstr_item *libstr_ptr_type;
                                       /* library stream pointer            */

/* public function declarations */

void open_lib(void);                   /* initialize the library manager    */
void close_lib(SETL_SYSTEM_PROTO_VOID);                  /* close the library manager         */
#ifdef LIBWRITE
void create_lib_file(SETL_SYSTEM_PROTO char *);          
			               /* create a new library file         */
#endif
libfile_ptr_type add_lib_file(SETL_SYSTEM_PROTO char *, int);
                                       /* add a library file to the         */
                                       /* search path                       */
void close_lib_file(SETL_SYSTEM_PROTO libfile_ptr_type); /* close a library file              */
void add_lib_path(SETL_SYSTEM_PROTO char *);             
				       /* add a path to the library         */
                                       /* search path                       */
libunit_ptr_type open_libunit(SETL_SYSTEM_PROTO char *, libfile_ptr_type, int);
                                       /* open a new library unit           */
void close_libunit(SETL_SYSTEM_PROTO libunit_ptr_type);  
				       /* close an open unit                */
#ifdef LIBWRITE
void copy_libunit(SETL_SYSTEM_PROTO char *, libfile_ptr_type, libfile_ptr_type);
				       /* copy a unit between libraries     */
#endif
libstr_ptr_type open_libstr(SETL_SYSTEM_PROTO libunit_ptr_type, int);
                                       /* open a library stream             */
void close_libstr(SETL_SYSTEM_PROTO libstr_ptr_type);    /* close an open library stream      */
unsigned read_libstr(SETL_SYSTEM_PROTO libstr_ptr_type, char *, unsigned);
                                       /* read data from a library stream   */
#ifdef LIBWRITE
void write_libstr(SETL_SYSTEM_PROTO libstr_ptr_type, char *, unsigned);
                                       /* write data to a library stream    */
#endif

#define LIBMAN_LOADED 1
#endif
