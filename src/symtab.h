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
 *  \packagespec{Symbol Table}
\*/

#ifndef SYMTAB_LOADED

/* symbol table structure */

struct symtab_item {

   struct symtab_item *st_name_link;   /* stack of symbols with common name */
   struct symtab_item *st_thread;      /* list of symbols in procedure      */
   struct namtab_item *st_namtab_ptr;  /* pointer to name table item        */
   struct proctab_item *st_owner_proc; /* procedure which 'owns' symbol     */
   struct proctab_item *st_class;      /* class to which symbol belongs     */
   int st_unit_num;                    /* unit which 'owns' symbol          */
   int32 st_offset;                    /* symbol offset within block        */
   int st_slot_num;                    /* local slot number (compile only)  */
   struct file_pos_item st_file_pos;   /* position symbol declared          */

   /* flags */

   unsigned st_type : 4;               /* symbol class                      */
   unsigned st_is_name_attached : 1;   /* YES if symbol is attached to name */
   unsigned st_is_hidden : 1;          /* YES if symbol is hidden           */
   unsigned st_has_lvalue : 1;         /* YES if symbol has an lhs value    */
   unsigned st_has_rvalue : 1;         /* YES if symbol has an rhs value    */
   unsigned st_is_rparam : 1;          /* YES if symbol is readable param   */
   unsigned st_is_wparam : 1;          /* YES if symbol is writable param   */
   unsigned st_is_temp : 1;            /* YES if symbol is temporary        */
   unsigned st_needs_stored : 1;       /* YES if symbol requires storage    */
   unsigned st_is_alloced : 1;         /* YES if symbol has been allocated  */
   unsigned st_is_initialized : 1;     /* YES if symbol has been            */
                                       /* initialized                       */
   unsigned st_in_spec : 1;            /* YES if in specification           */
   unsigned st_is_declared : 1;        /* YES if symbol is declared         */
   unsigned st_is_public : 1;          /* YES if symbol is in specification */
   unsigned st_is_visible_slot : 1;    /* YES if active built-in method     */
   unsigned st_global_var : 1;         /* YES if global variable (in a prog)*/


   /* auxiliary pointer (st_type determines which applies) */

   union {
      struct proctab_item *st_proctab_ptr;
                                       /* procedure pointer                 */
      struct import_item *st_import_ptr;
                                       /* imported package pointer          */
      struct c_real_item *st_real_ptr; /* real literal value                */
      struct integer_item *st_integer_ptr;
                                       /* integer literal value             */
      struct string_item *st_string_ptr;
                                       /* string literal value              */
      struct symtab_item *st_selector_ptr;
                                       /* selector symbol table entry       */
      int32 st_label_num;              /* label number                      */
      int32 st_label_offset;           /* label location (offset in unit)   */
   } st_aux;

};

typedef struct symtab_item *symtab_ptr_type;
                                       /* symbol table pointer              */

/* symbol classes */

/* ## begin symtab_types */
#define sym_id                 0       /* identifier                        */
#define sym_slot               1       /* instance variable                 */
#define sym_selector           2       /* selector                          */
#define sym_real               3       /* real literal                      */
#define sym_integer            4       /* integer literal                   */
#define sym_string             5       /* string literal                    */
#define sym_package            6       /* package name                      */
#define sym_class              7       /* class name                        */
#define sym_process            8       /* process name                      */
#define sym_procedure          9       /* procedure name                    */
#define sym_method            10       /* method name                       */
#define sym_program           11       /* program name                      */
#define sym_use               12       /* imported package                  */
#define sym_inherit           13       /* inherited class                   */
#define sym_label             14       /* label location                    */
/* ## end symtab_types */

/* symbol type descriptions */

#ifdef SHARED

char *symtab_desc[] = {                /* print string for symbol types     */
/* ## begin symtab_desc */
   "identifier",                       /* identifier                        */
   "slot",                             /* instance variable                 */
   "selector",                         /* selector                          */
   "real",                             /* real literal                      */
   "integer",                          /* integer literal                   */
   "string",                           /* string literal                    */
   "package",                          /* package name                      */
   "class",                            /* class name                        */
   "process",                          /* process name                      */
   "procedure",                        /* procedure name                    */
   "method",                           /* method name                       */
   "program",                          /* program name                      */
   "use",                              /* imported package                  */
   "inherit",                          /* inherited class                   */
   "label",                            /* label location                    */
/* ## end symtab_desc */
   NULL};

#else

EXTERNAL char *symtab_desc[];            /* print string for symbol types     */

#endif

/* public function declarations */

void init_symtab(void);                /* clear symbol table                */
symtab_ptr_type get_symtab(SETL_SYSTEM_PROTO_VOID);      
                                       /* allocate a new item               */
void clear_symtab(symtab_ptr_type);    /* clear a single symtab item        */
void free_symtab(symtab_ptr_type);     /* free a symbol table item          */
symtab_ptr_type enter_symbol(SETL_SYSTEM_PROTO
                             struct namtab_item *,
                             struct proctab_item *,
                             struct file_pos_item *);
                                       /* declare a symbol, place it in     */
                                       /* the symbol table                  */
void detach_symtab(struct symtab_item *);
                                       /* detach list of symbols from name  */
                                       /* table                             */
#ifdef DEBUG
void print_symtab(SETL_SYSTEM_PROTO struct proctab_item *);
                                       /* print symbol table                */
#endif

#define SYMTAB_LOADED 1
#endif
