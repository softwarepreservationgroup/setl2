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
 *  \packagespec{Procedure Table}
\*/

#ifndef PROCTAB_LOADED

/* procedure table item structure */

struct proctab_item {
   struct namtab_item *pr_namtab_ptr;  /* procedure name                    */
   struct symtab_item *pr_symtab_ptr;  /* procedure symbol                  */
   struct file_pos_item pr_file_pos;   /* file position of declaration      */
   struct proctab_item *pr_child;      /* first child pointer               */
   struct proctab_item *pr_parent;     /* parent pointer                    */
   struct proctab_item *pr_next;       /* next child                        */
   struct proctab_item **pr_tail;      /* next child to be added            */
   struct symtab_item *pr_symtab_head; /* list of symbols                   */
   struct symtab_item **pr_symtab_tail;
                                       /* tail of symbol list               */
   int32 pr_symtab_count;              /* number of used symbols            */
   struct storage_location_item pr_init_code;
                                       /* initialization code               */
   struct storage_location_item pr_slot_code;
                                       /* initialization code               */
   struct storage_location_item pr_body_code;
                                       /* body code                         */
   int pr_label_count;                 /* number of labels in procedure     */
   int32 pr_init_count;                /* number of init quadruples         */
   int32 pr_sinit_count;               /* number of slot init quadruples    */
   int32 pr_body_count;                /* number of body quadruples         */
   int32 pr_init_offset;               /* offset of procedure in init code  */
   int32 pr_entry_offset;              /* offset of procedure entry point   */
   int32 pr_body_offset;               /* offset of procedure in body code  */
   int32 pr_spec_offset;               /* offset of specifiers              */
   struct import_item *pr_import_list; /* list of imported packages         */
   struct import_item *pr_inherit_list;
                                       /* list of inherited classes         */
   int pr_unit_count;                  /* number of units referenced        */
   unsigned pr_type : 4;               /* procedure type                    */
   unsigned pr_method_code : 8;        /* method code                       */
   unsigned pr_var_args : 1;           /* YES if procedure accepts          */
                                       /* variable number of arguments      */
   int pr_formal_count;                /* number of formal parameters       */
};

typedef struct proctab_item *proctab_ptr_type;
                                       /* node pointer                      */

/* table item types */

/* ## begin proctab_types */
#define pr_package_spec        0       /* package specification             */
#define pr_package_body        1       /* package body                      */
#define pr_class_spec          2       /* class specification               */
#define pr_class_body          3       /* class body                        */
#define pr_process_spec        4       /* process specification             */
#define pr_process_body        5       /* process body                      */
#define pr_program             6       /* program                           */
#define pr_procedure           7       /* procedure                         */
#define pr_method              8       /* method                            */
#define pr_native_package      9       /* native package specification      */

/* ## end proctab_types */

/* type description strings */

#ifdef DEBUG
#ifdef SHARED

char *proctab_desc[] = {               /* names of procedure types          */
/* ## begin proctab_desc */
   "package spec",                     /* package specification             */
   "package body",                     /* package body                      */
   "class spec",                       /* class specification               */
   "class body",                       /* class body                        */
   "process spec",                     /* process specification             */
   "process body",                     /* process body                      */
   "program",                          /* program                           */
   "procedure",                        /* procedure                         */
   "method",                           /* method                            */
/* ## end proctab_desc */
   NULL};

#else

extern char *proctab_desc[];           /* names of procedure types          */

#endif
#endif

/* public data */

#ifdef SHARED

proctab_ptr_type predef_proctab_ptr = NULL;
                                       /* root of procedure tree            */
proctab_ptr_type unit_proctab_ptr = NULL;
                                       /* dummy procedure owning literals   */
struct proctab_item *curr_proctab_ptr = NULL;
                                       /* current procedure                 */

#else

EXTERNAL proctab_ptr_type predef_proctab_ptr;
                                       /* root of procedure tree            */
EXTERNAL proctab_ptr_type unit_proctab_ptr;
                                       /* dummy procedure owning literals   */
EXTERNAL struct proctab_item *curr_proctab_ptr;
                                       /* current procedure                 */

#endif

/* public function declarations */

void init_proctab(SETL_SYSTEM_PROTO_VOID);
                                       /* initialize the procedure table    */
proctab_ptr_type get_proctab(SETL_SYSTEM_PROTO_VOID);
                                       /* allocate a new item               */
void clear_proctab(proctab_ptr_type);  /* clear one table item              */
void free_proctab(proctab_ptr_type);   /* free a procedure table item       */
#ifdef DEBUG
void print_proctab(SETL_SYSTEM_PROTO_VOID);
                                       /* print procedure table             */
#endif

#define PROCTAB_LOADED 1
#endif
