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
 *  \package{Built-In Symbols}
 *
 *  This package provides a description of built-in symbols to both the
 *  compiler and the interpreter.  Most of it is a table of symbols and
 *  attributes.
\*/

/* standard C header files */

#include <stdio.h>                     /* standard I/O library              */

/* SETL2 system header files */

#include "form.h"                      /* form codes                        */

/*\
 *  \function{Compiler Definitions}
\*/

#ifdef COMPILER

/* built-in symbol table item */

typedef struct {
   int bi_form;                        /* form code                         */
   char *bi_name;                      /* symbol name                       */
   struct symtab_item **bi_symtab_ptr; /* symbol table pointer to be set    */
   int bi_formal_count;                /* minimum formal parameters         */
   unsigned bi_var_args : 1;           /* YES if variable number of         */
                                       /* arguments are allowed             */
   char *bi_arg_mode;                  /* parameter passing modes           */
} c_built_in_sym;

/* compiler symbols to be initialized */

#ifdef SHARED

struct symtab_item *sym_omega;         /* OM                                */
struct symtab_item *sym_false;         /* FALSE                             */
struct symtab_item *sym_true;          /* TRUE                              */
struct symtab_item *sym_zero;          /* literal 0                         */
struct symtab_item *sym_one;           /* literal 1                         */
struct symtab_item *sym_two;           /* literal 2                         */
struct symtab_item *sym_nullset;       /* empty set                         */
struct symtab_item *sym_nulltup;       /* empty tuple                       */
struct symtab_item *sym_memory;        /* pointer memory                    */
struct symtab_item *sym_abendtrap;     /* pointer memory                    */

#else

extern struct symtab_item *sym_omega;  /* OM                                */
extern struct symtab_item *sym_false;  /* FALSE                             */
extern struct symtab_item *sym_true;   /* TRUE                              */
extern struct symtab_item *sym_zero;   /* literal 0                         */
extern struct symtab_item *sym_one;    /* literal 1                         */
extern struct symtab_item *sym_two;    /* literal 2                         */
extern struct symtab_item *sym_nullset;
                                       /* empty set                         */
extern struct symtab_item *sym_nulltup;
                                       /* empty tuple                       */
extern struct symtab_item *sym_memory; /* pointer memory                    */
extern struct symtab_item *sym_abendtrap;

#endif
#endif

/*\
 *  \function{Interpreter Definitions}
\*/

#ifdef INTERP

/* built-in symbol table item */

typedef struct {
   int bi_form;                        /* form code                         */
   struct specifier_item **bi_spec_ptr;
                                       /* pointer to allocated specifier    */
   int bi_int_value;                   /* integer value                     */
   void (*bi_func_ptr)(SETL_SYSTEM_PROTO 
                       int, struct specifier_item *, struct specifier_item *);
                                       /* built-in function pointer         */
   int bi_formal_count;                /* minimum formal parameters         */
   unsigned bi_var_args : 1;           /* YES if variable number of         */
                                       /* arguments are allowed             */
} i_built_in_sym;

/* built-in functions */

void open_io(SETL_SYSTEM_PROTO_VOID);  /* open the I/O subsystem            */
void close_io(SETL_SYSTEM_PROTO_VOID); /* close the I/O system              */

/*
 *  Miscellaneous procedures
 */

void setl2_newat(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 newat procedure             */
void setl2_date(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 date procedure              */
void setl2_time(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 time procedure              */

/*
 *  Type checking procedures
 */

void setl2_type(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 type procedure              */
void setl2_is_atom(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 is_atom procedure           */
void setl2_is_boolean(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 is_boolean procedure        */
void setl2_is_integer(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 is_integer procedure        */
void setl2_is_real(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 is_real procedure           */
void setl2_is_string(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 is_string procedure         */
void setl2_is_set(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 is_set procedure            */
void setl2_is_map(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 is_map procedure            */
void setl2_is_tuple(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 is_tuple procedure          */
void setl2_is_procedure(SETL_SYSTEM_PROTO
                 int,struct specifier_item *, struct specifier_item *);
                                       /* SETL2 is_procedure procedure      */

/*
 *  Math procedures
 */

void setl2_abs(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 abs procedure               */
void setl2_even(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 even procedure              */
void setl2_odd(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 odd procedure               */
void setl2_float(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 float procedure             */
void setl2_atan2(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 atan2 procedure             */
void setl2_exp(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 exp procedure               */
void setl2_log(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 log procedure               */
void setl2_fix(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 fix procedure               */
void setl2_floor(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 floor procedure             */
void setl2_ceil(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 ceil procedure              */
void setl2_str(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 str procedure               */
void setl2_char(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 char procedure              */
void setl2_cos(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 cos procedure               */
void setl2_sin(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 sin procedure               */
void setl2_tan(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 tan procedure               */
void setl2_acos(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 acos procedure              */
void setl2_asin(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 asin procedure              */
void setl2_atan(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 atan procedure              */
void setl2_tanh(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 tanh procedure              */
void setl2_sqrt(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 sqrt procedure              */
void setl2_sign(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 sign procedure              */
/*
 *  String scanning primitives
 */

void setl2_any(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 any procedure               */
void setl2_break(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 break procedure             */
void setl2_len(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 len procedure               */
void setl2_match(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 match procedure             */
void setl2_notany(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 notany procedure            */
void setl2_span(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 span procedure              */
void setl2_lpad(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 lpad procedure              */
void setl2_rany(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 rany procedure              */
void setl2_rbreak(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 rbreak procedure            */
void setl2_rlen(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 rlen procedure              */
void setl2_rmatch(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 rmatch procedure            */
void setl2_rnotany(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 rnotany procedure           */
void setl2_rspan(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 rspan procedure             */
void setl2_rpad(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 rpad procedure              */

/*
 *  Input / Output procedures
 */

void setl2_open(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 open procedure              */
void setl2_close(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 close procedure             */
void setl2_get(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 get procedure               */
void setl2_geta(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 geta procedure              */
void setl2_read(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 read procedure              */
void setl2_reada(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 reada procedure             */
void setl2_reads(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 reads procedure             */
void setl2_unstr(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 unstr procedure             */
void setl2_binstr(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 binstr procedure            */
void setl2_unbinstr(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 unbinstr procedure          */
void setl2_print(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 print procedure             */
void setl2_nprint(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 nprint procedure            */
void setl2_printa(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 printa procedure            */
void setl2_nprinta(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 nprinta procedure           */
void setl2_getb(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 getb procedure              */
void setl2_putb(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 putb procedure              */
void setl2_gets(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 getb procedure              */
void setl2_puts(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 getb procedure              */
void setl2_fsize(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 getb procedure              */
void setl2_eof(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 eof procedure               */

/*
 *  System procedures
 */

void setl2_fexists(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 exists procedure            */
void setl2_system(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 system procedure            */
void setl2_abort(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 abort procedure             */
void setl2_Ccallout(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 callout procedure           */
void setl2_Ccallout2(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 callout2 procedure          */
void setl2_getenv(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 callout2 procedure          */
void setl2_open_lib(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Open DLL                          */
void setl2_close_lib(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Close DLL                         */
void setl2_get_symbol(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_get_symbol_name(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_find_symbol(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_get_symbol(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_num_symbols(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);

void setl2_call_function(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Call a function defined in a DLL  */
void setl2_library_file(SETL_SYSTEM_PROTO
                 int, struct specifier_item *,
                             struct specifier_item *);
                                       /* load file unit from library       */
void setl2_library_package(SETL_SYSTEM_PROTO
                 int, struct specifier_item *,
                                struct specifier_item *);
                                       /* load package from library         */
void setl2_opcode_count(SETL_SYSTEM_PROTO
                 int, struct specifier_item *,
                             struct specifier_item *);
                                       /* SETL2 callout2 procedure          */
void setl2_malloc(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 interface to malloc         */
void setl2_dispose(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 interface to free           */
void setl2_javascript(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Give a JAVASCRIPT command         */
void setl2_geturl(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Get a URL                         */
void setl2_posturl(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Post a URL                        */
void setl2_wait(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Wait for a browser event          */
void setl2_pass_symtab(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Pass the error ext. procs to R/T.*/
void setl2_eval(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 dynamic compilation         */

void setl2_popen(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 popen procedure             */
void setl2_getchar(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 getchar procedure           */
void setl2_fflush(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 fflush procedure            */
void setl2_user_time(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 user time procedure         */
void setl2_trace(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* SETL2 trace procedure             */
void setl2_ref_count(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Reference count of an object      */

/*
 *  Process stuff
 */

void setl2_suspend(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_resume(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_kill(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_newmbox(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_await(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_acheck(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_pass(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);

/*
 * Extra memory management routines
 */

void setl2_bpeek(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_speek(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_ipeek(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_bpoke(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_spoke(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
void setl2_ipoke(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);

/*
 * Extra memory management routines
 */

void setl2_host_get(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);

void setl2_host_put(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);

void setl2_host_call(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);

void setl2_create_activexobject(SETL_SYSTEM_PROTO
                 int, struct specifier_item *, struct specifier_item *);
                                       /* Give a JAVASCRIPT command         */


/* interpreter symbols to be initialized */

#ifdef SHARED

struct specifier_item *spec_omega;     /* OM                                */
struct specifier_item *spec_false;     /* FALSE                             */
struct specifier_item *spec_true;      /* TRUE                              */
struct specifier_item *spec_zero;      /* literal 0                         */
struct specifier_item *spec_one;       /* literal 1                         */
struct specifier_item *spec_two;       /* literal 2                         */
struct specifier_item *spec_cline;     /* command line tuple                */
struct specifier_item *spec_nullset;   /* empty set                         */
struct specifier_item *spec_nulltup;   /* empty tuple                       */
struct specifier_item *spec_memory;    /* pointer memory                    */
struct specifier_item *spec_abendtrap; /* pointer memory                    */

/* Misc specifiers used to directly call builtin procedures  */

struct specifier_item *spec_printa;    /* setl2_printa                      */
struct specifier_item *spec_reada;     /* setl2_reada                       */
struct specifier_item *spec_nprinta;   /* setl2_nprinta                     */
struct specifier_item *spec_fsize;     /* setl2_fsize                       */

#else

extern struct specifier_item *spec_omega;
                                       /* OM                                */
EXTERNAL struct specifier_item *spec_false;
                                       /* FALSE                             */
EXTERNAL struct specifier_item *spec_true;
                                       /* TRUE                              */
extern struct specifier_item *spec_zero;
                                       /* literal 0                         */
extern struct specifier_item *spec_one;
                                       /* literal 1                         */
extern struct specifier_item *spec_two;
                                       /* literal 2                         */
EXTERNAL struct specifier_item *spec_cline;
                                       /* command line tuple                */
extern struct specifier_item *spec_nullset;
                                       /* empty set                         */
extern struct specifier_item *spec_nulltup;
                                       /* empty tuple                       */
extern struct specifier_item *spec_memory;
                                       /* pointer memory                    */
extern struct specifier_item *spec_abendtrap;
                                       /* pointer memory                    */
extern struct specifier_item *spec_printa;
                                       /* setl2_printa                      */
extern struct specifier_item *spec_reada;
                                       /* setl2_reada                       */
extern struct specifier_item *spec_nprinta;
                                       /* setl2_nprinta                     */
extern struct specifier_item *spec_fsize;
                                       /* setl2_fsize                       */

#endif
#endif

/*\
 *  \function{Table of Built-In Symbols}
\*/

#ifdef SHARED

#ifdef COMPILER
#define SYM_COMPILER 1
#endif

#ifdef INTERP
#define SYM_INTERP 1
#endif

#ifdef SYM_COMPILER
#define COMPILER 1
#undef INTERP
c_built_in_sym c_built_in_tab[] = {

#include "builtsym.h"

};
#endif

#ifdef SYM_INTERP
#define INTERP 1
#undef COMPILER
i_built_in_sym i_built_in_tab[] = {

#include "builtsym.h"

};
#endif

#undef INTERP
#undef COMPILER

#ifdef SYM_INTERP
#define INTERP 1
#endif

#ifdef SYM_COMPILER
#define COMPILER 1
#endif

#else /* not SHARED */

#ifdef COMPILER
extern c_built_in_sym c_built_in_tab[];
#endif
#ifdef INTERP
extern i_built_in_sym i_built_in_tab[];
#endif

#endif
