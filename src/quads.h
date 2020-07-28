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
 *  \packagespec{Quadruples}
\*/

#ifndef QUADS_LOADED

/* SETL2 system header files */

#include "pcode.h"                     /* pseudo-code instructions          */

/* quad node structure */

struct quad_item {
   struct quad_item *q_next;           /* forward and backward pointers     */
   int q_opcode;                       /* opcode (quad type)                */
   union {                             /* operand array                     */
      int32 q_integer;                 /* immediate integer operand         */
      struct symtab_item *q_symtab_ptr;
                                       /* symbol table operand              */
   } q_operand[3];
   int q_opt_data;                     /* used by the optimizer             */
   struct file_pos_item q_file_pos;    /* file position (saved for abends)  */
};

typedef struct quad_item *quad_ptr_type;
                                       /* node pointer                      */

/* clear a quad item */

#define clear_quad(q) { \
   (q)->q_next = NULL;                 (q)->q_opcode = -1; \
   (q)->q_opt_data = 0; \
   (q)->q_operand[0].q_symtab_ptr = NULL; \
   (q)->q_operand[1].q_symtab_ptr = NULL; \
   (q)->q_operand[2].q_symtab_ptr = NULL; \
   (q)->q_file_pos.fp_line = -1;       (q)->q_file_pos.fp_column = -1; \
}

/*\
 *  \table{quadruple opcodes}
\*/

/* ## begin quad_types */
#define q_noop                 0       /* no operation                      */
#define q_push1                1       /* push one operand                  */
#define q_push2                2       /* push two operands                 */
#define q_push3                3       /* push three operands               */
#define q_pop1                 4       /* pop one operand                   */
#define q_pop2                 5       /* pop two operands                  */
#define q_pop3                 6       /* pop three operands                */
#define q_add                  7       /* +                                 */
#define q_sub                  8       /* -                                 */
#define q_mult                 9       /* *                                 */
#define q_div                 10       /* /                                 */
#define q_exp                 11       /* **                                */
#define q_mod                 12       /* mod                               */
#define q_min                 13       /* min                               */
#define q_max                 14       /* max                               */
#define q_with                15       /* with                              */
#define q_less                16       /* with                              */
#define q_lessf               17       /* lessf                             */
#define q_from                18       /* from                              */
#define q_fromb               19       /* fromb                             */
#define q_frome               20       /* frome                             */
#define q_npow                21       /* npow                              */
#define q_uminus              22       /* unary minus                       */
#define q_domain              23       /* domain                            */
#define q_range               24       /* range                             */
#define q_pow                 25       /* pow                               */
#define q_arb                 26       /* arb                               */
#define q_nelt                27       /* #                                 */
#define q_not                 28       /* not                               */
#define q_smap                29       /* convert set to smap               */
#define q_tupof               30       /* tuple `of'                        */
#define q_of1                 31       /* one argument `of'                 */
#define q_of                  32       /* map, tuple, or string             */
#define q_ofa                 33       /* multi-valued map                  */
#define q_kof1                34       /* one argument `of'                 */
                                       /* kill temp after assignment        */
#define q_kof                 35       /* map, tuple, or string             */
                                       /* kill temp after assignment        */
#define q_kofa                36       /* multi-valued map                  */
                                       /* kill temp after assignment        */
#define q_erase               37       /* kill temp                         */
#define q_slice               38       /* slice                             */
#define q_end                 39       /* string end                        */
#define q_assign              40       /* general assignment                */
#define q_penviron            41       /* procedure with environment        */
#define q_sof                 42       /* map, tuple, or string assignment  */
#define q_sofa                43       /* mmap sinister assignment          */
#define q_sslice              44       /* slice assignment                  */
#define q_send                45       /* string end assignment             */
#define q_eq                  46       /* =                                 */
#define q_ne                  47       /* /=                                */
#define q_lt                  48       /* <                                 */
#define q_nlt                 49       /* not <                             */
#define q_le                  50       /* <=                                */
#define q_nle                 51       /* not <=                            */
#define q_in                  52       /* in                                */
#define q_notin               53       /* notin                             */
#define q_incs                54       /* incs                              */
#define q_and                 55       /* and                               */
#define q_or                  56       /* or                                */
#define q_go                  57       /* branch unconditionally            */
#define q_goind               58       /* branch indirect                   */
#define q_gotrue              59       /* branch if value is true           */
#define q_gofalse             60       /* branch if value is false          */
#define q_goeq                61       /* branch if =                       */
#define q_gone                62       /* branch if /=                      */
#define q_golt                63       /* branch if <                       */
#define q_gonlt               64       /* branch if not <                   */
#define q_gole                65       /* branch if <=                      */
#define q_gonle               66       /* branch if not <=                  */
#define q_goin                67       /* branch if element                 */
#define q_gonotin             68       /* branch if not element             */
#define q_goincs              69       /* branch if includes                */
#define q_gonincs             70       /* branch if not includes            */
#define q_set                 71       /* { ... }                           */
#define q_tuple               72       /* [ ... ]                           */
#define q_iter                73       /* general iterator                  */
#define q_inext               74       /* iterator next                     */
#define q_lcall               75       /* literal procedure call            */
#define q_call                76       /* procedure call                    */
#define q_return              77       /* return from procedure             */
#define q_stop                78       /* stop executing                    */
#define q_stopall             79       /* stop everything                   */
#define q_assert              80       /* assert message                    */
#define q_intcheck            81       /* integer check                     */
#define q_initobj             82       /* create new object                 */
#define q_initend             83       /* finish creation                   */
#define q_slot                84       /* reference slot                    */
#define q_sslot               85       /* assign slot                       */
#define q_slotof              86       /* call slot reference               */
#define q_menviron            87       /* method with environment           */
                                       /* or instance                       */
#define q_self                88       /* make self                         */
#define q_initproc            89       /* create new process                */
#define q_initpend            90       /* finish creation                   */
#define q_label               91       /* label                             */
#define q_ufrom               92       /* unary from                        */
/* ## end quad_types */

/*\
 *  \table{quadruple operand types}
 *
 *  Each quadruple instruction has three operands, the types of which
 *  vary according to the opcode.  This table gives the types for each
 *  opcode.
\*/

#define QUAD_INTEGER_OP        0       /* integer operand                   */
#define QUAD_SPEC_OP           1       /* specifier operand                 */
#define QUAD_LABEL_OP          2       /* label operand                     */
#define QUAD_SLOT_OP           3       /* slot pointer                      */
#define QUAD_CLASS_OP          4       /* class identifier                  */
#define QUAD_PROCESS_OP        5       /* class identifier                  */

#ifdef SHARED

char quad_optype[][3] = {              /* operand types by opcode           */
/* ## begin quad_optypes */
   {  1,  1,  1 },                     /* no operation                      */
   {  1,  1,  1 },                     /* push one operand                  */
   {  1,  1,  1 },                     /* push two operands                 */
   {  1,  1,  1 },                     /* push three operands               */
   {  1,  1,  1 },                     /* pop one operand                   */
   {  1,  1,  1 },                     /* pop two operands                  */
   {  1,  1,  1 },                     /* pop three operands                */
   {  1,  1,  1 },                     /* +                                 */
   {  1,  1,  1 },                     /* -                                 */
   {  1,  1,  1 },                     /* *                                 */
   {  1,  1,  1 },                     /* /                                 */
   {  1,  1,  1 },                     /* **                                */
   {  1,  1,  1 },                     /* mod                               */
   {  1,  1,  1 },                     /* min                               */
   {  1,  1,  1 },                     /* max                               */
   {  1,  1,  1 },                     /* with                              */
   {  1,  1,  1 },                     /* with                              */
   {  1,  1,  1 },                     /* lessf                             */
   {  1,  1,  1 },                     /* from                              */
   {  1,  1,  1 },                     /* fromb                             */
   {  1,  1,  1 },                     /* frome                             */
   {  1,  1,  1 },                     /* npow                              */
   {  1,  1,  1 },                     /* unary minus                       */
   {  1,  1,  1 },                     /* domain                            */
   {  1,  1,  1 },                     /* range                             */
   {  1,  1,  1 },                     /* pow                               */
   {  1,  1,  1 },                     /* arb                               */
   {  1,  1,  1 },                     /* #                                 */
   {  1,  1,  1 },                     /* not                               */
   {  1,  1,  1 },                     /* convert set to smap               */
   {  1,  1,  1 },                     /* tuple `of'                        */
   {  1,  1,  1 },                     /* one argument `of'                 */
   {  1,  1,  0 },                     /* map, tuple, or string             */
   {  1,  1,  1 },                     /* multi-valued map                  */
   {  1,  1,  1 },                     /* one argument `of'                 */
                                       /* kill temp after assignment        */
   {  1,  1,  1 },                     /* map, tuple, or string             */
                                       /* kill temp after assignment        */
   {  1,  1,  1 },                     /* multi-valued map                  */
                                       /* kill temp after assignment        */
   {  1,  1,  1 },                     /* kill temp                         */
   {  1,  1,  1 },                     /* slice                             */
   {  1,  1,  1 },                     /* string end                        */
   {  1,  1,  1 },                     /* general assignment                */
   {  1,  1,  1 },                     /* procedure with environment        */
   {  1,  1,  1 },                     /* map, tuple, or string assignment  */
   {  1,  1,  1 },                     /* mmap sinister assignment          */
   {  1,  1,  1 },                     /* slice assignment                  */
   {  1,  1,  1 },                     /* string end assignment             */
   {  1,  1,  1 },                     /* =                                 */
   {  1,  1,  1 },                     /* /=                                */
   {  1,  1,  1 },                     /* <                                 */
   {  1,  1,  1 },                     /* not <                             */
   {  1,  1,  1 },                     /* <=                                */
   {  1,  1,  1 },                     /* not <=                            */
   {  1,  1,  1 },                     /* in                                */
   {  1,  1,  1 },                     /* notin                             */
   {  1,  1,  1 },                     /* incs                              */
   {  1,  1,  1 },                     /* and                               */
   {  1,  1,  1 },                     /* or                                */
   {  2,  1,  1 },                     /* branch unconditionally            */
   {  1,  1,  1 },                     /* branch indirect                   */
   {  2,  1,  1 },                     /* branch if value is true           */
   {  2,  1,  1 },                     /* branch if value is false          */
   {  2,  1,  1 },                     /* branch if =                       */
   {  2,  1,  1 },                     /* branch if /=                      */
   {  2,  1,  1 },                     /* branch if <                       */
   {  2,  1,  1 },                     /* branch if not <                   */
   {  2,  1,  1 },                     /* branch if <=                      */
   {  2,  1,  1 },                     /* branch if not <=                  */
   {  2,  1,  1 },                     /* branch if element                 */
   {  2,  1,  1 },                     /* branch if not element             */
   {  2,  1,  1 },                     /* branch if includes                */
   {  2,  1,  1 },                     /* branch if not includes            */
   {  1,  1,  1 },                     /* { ... }                           */
   {  1,  1,  1 },                     /* [ ... ]                           */
   {  1,  1,  0 },                     /* general iterator                  */
   {  1,  1,  2 },                     /* iterator next                     */
   {  1,  1,  0 },                     /* literal procedure call            */
   {  1,  1,  0 },                     /* procedure call                    */
   {  1,  1,  1 },                     /* return from procedure             */
   {  1,  1,  1 },                     /* stop executing                    */
   {  1,  1,  1 },                     /* stop everything                   */
   {  1,  1,  0 },                     /* assert message                    */
   {  1,  1,  1 },                     /* integer check                     */
   {  4,  1,  1 },                     /* create new object                 */
   {  1,  4,  1 },                     /* finish creation                   */
   {  1,  1,  3 },                     /* reference slot                    */
   {  1,  3,  1 },                     /* assign slot                       */
   {  1,  3,  0 },                     /* call slot reference               */
   {  1,  1,  1 },                     /* method with environment           */
                                       /* or instance                       */
   {  1,  1,  1 },                     /* make self                         */
   {  5,  1,  1 },                     /* create new process                */
   {  1,  5,  1 },                     /* finish creation                   */
   {  0,  1,  1 },                     /* label                             */
   {  1,  1,  1 },                     /* unary from                        */
/* ## end quad_optypes */
   { -1, -1, -1 }};

#else

extern char quad_optype[][3];          /* operand types by opcode           */

#endif

/*\
 *  \table{default pseudo-opcodes}
 *
 *  This table maps quadruple opcodes to the corresponding pseudo-code
 *  opcode.
\*/

#ifdef SHARED

int pcode_opcode[] = {                 /* pcode for quad opcode             */
/* ## begin pcode_opcodes */
   p_noop,                             /* no operation                      */
   p_push1,                            /* push one operand                  */
   p_push2,                            /* push two operands                 */
   p_push3,                            /* push three operands               */
   p_pop1,                             /* pop one operand                   */
   p_pop2,                             /* pop two operands                  */
   p_pop3,                             /* pop three operands                */
   p_add,                              /* +                                 */
   p_sub,                              /* -                                 */
   p_mult,                             /* *                                 */
   p_div,                              /* /                                 */
   p_exp,                              /* **                                */
   p_mod,                              /* mod                               */
   p_min,                              /* min                               */
   p_max,                              /* max                               */
   p_with,                             /* with                              */
   p_less,                             /* with                              */
   p_lessf,                            /* lessf                             */
   p_from,                             /* from                              */
   p_fromb,                            /* fromb                             */
   p_frome,                            /* frome                             */
   p_npow,                             /* npow                              */
   p_uminus,                           /* unary minus                       */
   p_domain,                           /* domain                            */
   p_range,                            /* range                             */
   p_pow,                              /* pow                               */
   p_arb,                              /* arb                               */
   p_nelt,                             /* #                                 */
   p_not,                              /* not                               */
   p_smap,                             /* convert set to smap               */
   p_tupof,                            /* tuple `of'                        */
   p_of1,                              /* one argument `of'                 */
   p_of,                               /* map, tuple, or string             */
   p_ofa,                              /* multi-valued map                  */
   p_kof1,                             /* one argument `of'                 */
                                       /* kill temp after assignment        */
   p_kof,                              /* map, tuple, or string             */
                                       /* kill temp after assignment        */
   p_kofa,                             /* multi-valued map                  */
                                       /* kill temp after assignment        */
   p_erase,                            /* kill temp                         */
   p_slice,                            /* slice                             */
   p_end,                              /* string end                        */
   p_assign,                           /* general assignment                */
   p_penviron,                         /* procedure with environment        */
   p_sof,                              /* map, tuple, or string assignment  */
   p_sofa,                             /* mmap sinister assignment          */
   p_sslice,                           /* slice assignment                  */
   p_send,                             /* string end assignment             */
   p_eq,                               /* =                                 */
   p_ne,                               /* /=                                */
   p_lt,                               /* <                                 */
   p_nlt,                              /* not <                             */
   p_le,                               /* <=                                */
   p_nle,                              /* not <=                            */
   p_in,                               /* in                                */
   p_notin,                            /* notin                             */
   p_incs,                             /* incs                              */
   p_and,                              /* and                               */
   p_or,                               /* or                                */
   p_go,                               /* branch unconditionally            */
   p_goind,                            /* branch indirect                   */
   p_gotrue,                           /* branch if value is true           */
   p_gofalse,                          /* branch if value is false          */
   p_goeq,                             /* branch if =                       */
   p_gone,                             /* branch if /=                      */
   p_golt,                             /* branch if <                       */
   p_gonlt,                            /* branch if not <                   */
   p_gole,                             /* branch if <=                      */
   p_gonle,                            /* branch if not <=                  */
   p_goin,                             /* branch if element                 */
   p_gonotin,                          /* branch if not element             */
   p_goincs,                           /* branch if includes                */
   p_gonincs,                          /* branch if not includes            */
   p_set,                              /* { ... }                           */
   p_tuple,                            /* [ ... ]                           */
   p_iter,                             /* general iterator                  */
   p_inext,                            /* iterator next                     */
   p_lcall,                            /* literal procedure call            */
   p_call,                             /* procedure call                    */
   p_return,                           /* return from procedure             */
   p_stop,                             /* stop executing                    */
   p_stopall,                          /* stop everything                   */
   p_assert,                           /* assert message                    */
   p_intcheck,                         /* integer check                     */
   p_initobj,                          /* create new object                 */
   p_initend,                          /* finish creation                   */
   p_slot,                             /* reference slot                    */
   p_sslot,                            /* assign slot                       */
   p_slotof,                           /* call slot reference               */
   p_menviron,                         /* method with environment           */
                                       /* or instance                       */
   p_self,                             /* make self                         */
   p_initproc,                         /* create new process                */
   p_initpend,                         /* finish creation                   */
   q_noop,                             /* label                             */
   p_ufrom,                            /* unary from                        */
/* ## end pcode_opcodes */
   -1};

#else

extern int pcode_opcode[];             /* pcode for quad opcode             */

#endif

/*\
 *  \table{quadruple type descriptions}
 *
 *  We print out quadruple opcode names during debugging.  These strings
 *  just make that easy.
\*/

#ifdef DEBUG
#ifdef SHARED

char *quad_desc[] = {                  /* print string for opcodes          */
/* ## begin quad_desc */
   "q_noop",                           /* no operation                      */
   "q_push1",                          /* push one operand                  */
   "q_push2",                          /* push two operands                 */
   "q_push3",                          /* push three operands               */
   "q_pop1",                           /* pop one operand                   */
   "q_pop2",                           /* pop two operands                  */
   "q_pop3",                           /* pop three operands                */
   "q_add",                            /* +                                 */
   "q_sub",                            /* -                                 */
   "q_mult",                           /* *                                 */
   "q_div",                            /* /                                 */
   "q_exp",                            /* **                                */
   "q_mod",                            /* mod                               */
   "q_min",                            /* min                               */
   "q_max",                            /* max                               */
   "q_with",                           /* with                              */
   "q_less",                           /* with                              */
   "q_lessf",                          /* lessf                             */
   "q_from",                           /* from                              */
   "q_fromb",                          /* fromb                             */
   "q_frome",                          /* frome                             */
   "q_npow",                           /* npow                              */
   "q_uminus",                         /* unary minus                       */
   "q_domain",                         /* domain                            */
   "q_range",                          /* range                             */
   "q_pow",                            /* pow                               */
   "q_arb",                            /* arb                               */
   "q_nelt",                           /* #                                 */
   "q_not",                            /* not                               */
   "q_smap",                           /* convert set to smap               */
   "q_tupof",                          /* tuple `of'                        */
   "q_of1",                            /* one argument `of'                 */
   "q_of",                             /* map, tuple, or string             */
   "q_ofa",                            /* multi-valued map                  */
   "q_kof1",                           /* one argument `of'                 */
                                       /* kill temp after assignment        */
   "q_kof",                            /* map, tuple, or string             */
                                       /* kill temp after assignment        */
   "q_kofa",                           /* multi-valued map                  */
                                       /* kill temp after assignment        */
   "q_erase",                          /* kill temp                         */
   "q_slice",                          /* slice                             */
   "q_end",                            /* string end                        */
   "q_assign",                         /* general assignment                */
   "q_penviron",                       /* procedure with environment        */
   "q_sof",                            /* map, tuple, or string assignment  */
   "q_sofa",                           /* mmap sinister assignment          */
   "q_sslice",                         /* slice assignment                  */
   "q_send",                           /* string end assignment             */
   "q_eq",                             /* =                                 */
   "q_ne",                             /* /=                                */
   "q_lt",                             /* <                                 */
   "q_nlt",                            /* not <                             */
   "q_le",                             /* <=                                */
   "q_nle",                            /* not <=                            */
   "q_in",                             /* in                                */
   "q_notin",                          /* notin                             */
   "q_incs",                           /* incs                              */
   "q_and",                            /* and                               */
   "q_or",                             /* or                                */
   "q_go",                             /* branch unconditionally            */
   "q_goind",                          /* branch indirect                   */
   "q_gotrue",                         /* branch if value is true           */
   "q_gofalse",                        /* branch if value is false          */
   "q_goeq",                           /* branch if =                       */
   "q_gone",                           /* branch if /=                      */
   "q_golt",                           /* branch if <                       */
   "q_gonlt",                          /* branch if not <                   */
   "q_gole",                           /* branch if <=                      */
   "q_gonle",                          /* branch if not <=                  */
   "q_goin",                           /* branch if element                 */
   "q_gonotin",                        /* branch if not element             */
   "q_goincs",                         /* branch if includes                */
   "q_gonincs",                        /* branch if not includes            */
   "q_set",                            /* { ... }                           */
   "q_tuple",                          /* [ ... ]                           */
   "q_iter",                           /* general iterator                  */
   "q_inext",                          /* iterator next                     */
   "q_lcall",                          /* literal procedure call            */
   "q_call",                           /* procedure call                    */
   "q_return",                         /* return from procedure             */
   "q_stop",                           /* stop executing                    */
   "q_stopall",                        /* stop everything                   */
   "q_assert",                         /* assert message                    */
   "q_intcheck",                       /* integer check                     */
   "q_initobj",                        /* create new object                 */
   "q_initend",                        /* finish creation                   */
   "q_slot",                           /* reference slot                    */
   "q_sslot",                          /* assign slot                       */
   "q_slotof",                         /* call slot reference               */
   "q_menviron",                       /* method with environment           */
                                       /* or instance                       */
   "q_self",                           /* make self                         */
   "q_initproc",                       /* create new process                */
   "q_initpend",                       /* finish creation                   */
   "q_label",                          /* label                             */
   "q_ufrom",                          /* unary from                        */
/* ## end quad_desc */
   NULL};

#else

extern char *quad_desc[];              /* print string for opcodes          */

#endif
#endif

/*\
 *  \nobar\newpage
\*/

/* public data */

#ifdef SHARED

quad_ptr_type *emit_quad_tail = NULL;  /* append next quad here             */
quad_ptr_type emit_quad_ptr = NULL;    /* work pointer for emit macro       */

#else

extern quad_ptr_type *emit_quad_tail;  /* append next quad here             */
extern quad_ptr_type emit_quad_ptr;    /* work pointer for emit macro       */

#endif

/* public function declarations */

void init_quads(void);                 /* initialize quadruples             */
void open_emit(SETL_SYSTEM_PROTO struct storage_location_item *);
                                       /* initialize emit macros            */
#if SHORT_MACROS
void emit(int, symtab_ptr_type, symtab_ptr_type, symtab_ptr_type,
          file_pos_type *);            /* emit with three symtab operands   */
void emitiss(int, int, symtab_ptr_type, symtab_ptr_type,
             file_pos_type *);         /* emit with first operand integer   */
void emitssi(int, symtab_ptr_type, symtab_ptr_type, int,
             file_pos_type *);         /* emit with last operand integer    */
#endif
void close_emit(SETL_SYSTEM_PROTO_VOID);  
                                       /* finish current emit stream        */
quad_ptr_type get_quad(SETL_SYSTEM_PROTO_VOID);
                                       /* allocate a new item               */
void free_quad(SETL_SYSTEM_PROTO quad_ptr_type);
                                       /* return an item to the free pool   */
void store_quads(SETL_SYSTEM_PROTO
                 struct storage_location_item *, struct quad_item *);
                                       /* save quadruple list in            */
                                       /* intermediate file                 */
struct quad_item *load_quads(SETL_SYSTEM_PROTO
                             struct storage_location_item *);
                                       /* load quadruple list from          */
                                       /* intermediate file                 */
void kill_quads(struct quad_item *);   /* release memory used by quads      */
#ifdef DEBUG
void print_quads(SETL_SYSTEM_PROTO quad_ptr_type,char *);
                                       /* print quadruple list              */
#endif

#if !SHORT_MACROS
#include "emit.h"
#endif

#define QUADS_LOADED 1
#endif
