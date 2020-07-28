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
 *  \package{Pseudo-Code}
 *
 *  This package contains definitions and tables for pseudo-code
 *  instructions.
\*/

#ifndef PCODE_LOADED

/* operand type definitions */

/* ## begin pcode_types */
#define p_noop                 0       /* no operation                      */
#define p_push1                1       /* push one operand                  */
#define p_push2                2       /* push two operands                 */
#define p_push3                3       /* push three operands               */
#define p_pop1                 4       /* pop one operand                   */
#define p_pop2                 5       /* pop two operands                  */
#define p_pop3                 6       /* pop three operands                */
#define p_add                  7       /* +                                 */
#define p_sub                  8       /* -                                 */
#define p_mult                 9       /* *                                 */
#define p_div                 10       /* /                                 */
#define p_exp                 11       /* **                                */
#define p_mod                 12       /* mod                               */
#define p_min                 13       /* min                               */
#define p_max                 14       /* max                               */
#define p_with                15       /* with                              */
#define p_less                16       /* with                              */
#define p_lessf               17       /* lessf                             */
#define p_from                18       /* from                              */
#define p_fromb               19       /* fromb                             */
#define p_frome               20       /* frome                             */
#define p_npow                21       /* npow                              */
#define p_uminus              22       /* unary minus                       */
#define p_domain              23       /* domain                            */
#define p_range               24       /* range                             */
#define p_pow                 25       /* pow                               */
#define p_arb                 26       /* arb                               */
#define p_nelt                27       /* #                                 */
#define p_not                 28       /* not                               */
#define p_smap                29       /* convert set to smap               */
#define p_tupof               30       /* tuple `of'                        */
#define p_of1                 31       /* one argument `of'                 */
#define p_of                  32       /* map, tuple, or string             */
#define p_ofa                 33       /* multi-valued map                  */
#define p_kof1                34       /* one argument `of'                 */
                                       /* kill temp after assignment        */
#define p_kof                 35       /* map, tuple, or string             */
                                       /* kill temp after assignment        */
#define p_kofa                36       /* multi-valued map                  */
                                       /* kill temp after assignment        */
#define p_erase               37       /* kill temp                         */
#define p_slice               38       /* slice                             */
#define p_end                 39       /* string end                        */
#define p_assign              40       /* general assignment                */
#define p_penviron            41       /* procedure with environment        */
#define p_sof                 42       /* map, tuple, or string assignment  */
#define p_sofa                43       /* mmap sinister assignment          */
#define p_sslice              44       /* slice assignment                  */
#define p_send                45       /* string end assignment             */
#define p_eq                  46       /* =                                 */
#define p_ne                  47       /* /=                                */
#define p_lt                  48       /* <                                 */
#define p_nlt                 49       /* not <                             */
#define p_le                  50       /* <=                                */
#define p_nle                 51       /* not <=                            */
#define p_in                  52       /* in                                */
#define p_notin               53       /* notin                             */
#define p_incs                54       /* incs                              */
#define p_and                 55       /* and                               */
#define p_or                  56       /* or                                */
#define p_go                  57       /* branch unconditionally            */
#define p_goind               58       /* branch indirect                   */
#define p_gotrue              59       /* branch if value is true           */
#define p_gofalse             60       /* branch if value is false          */
#define p_goeq                61       /* branch if =                       */
#define p_gone                62       /* branch if /=                      */
#define p_golt                63       /* branch if <                       */
#define p_gonlt               64       /* branch if not <                   */
#define p_gole                65       /* branch if <=                      */
#define p_gonle               66       /* branch if not <=                  */
#define p_goin                67       /* branch if element                 */
#define p_gonotin             68       /* branch if not element             */
#define p_goincs              69       /* branch if includes                */
#define p_gonincs             70       /* branch if not includes            */
#define p_set                 71       /* { ... }                           */
#define p_tuple               72       /* [ ... ]                           */
#define p_iter                73       /* general iterator                  */
#define p_inext               74       /* iterator next                     */
#define p_lcall               75       /* literal procedure call            */
#define p_call                76       /* procedure call                    */
#define p_return              77       /* return from procedure             */
#define p_stop                78       /* stop executing                    */
#define p_stopall             79       /* stop everything                   */
#define p_assert              80       /* assert message                    */
#define p_initobj             81       /* create new object                 */
#define p_initend             82       /* finish creation                   */
#define p_slot                83       /* reference slot                    */
#define p_sslot               84       /* assign slot                       */
#define p_slotof              85       /* call slot reference               */
#define p_menviron            86       /* method with environment           */
                                       /* or instance                       */
#define p_self                87       /* make self                         */
#define p_initproc            88       /* create new process                */
#define p_initpend            89       /* finish creation                   */
#define p_intcheck            90       /* null instruction                  */
#define p_filepos             91       /* file position flag                */
#define p_ufrom               92       /* unary minus                       */
/* ## end pcode_types */

#define pcode_length          92       /* Number of operands used           */
/*\
 *  \table{operand type table}
 *
 *  Each pseudo-code instruction has three operands, the types of which
 *  vary according to the opcode.  This table gives the types for each
 *  opcode.
\*/

#define PCODE_INTEGER_OP       0       /* integer operand                   */
#define PCODE_SPEC_OP          1       /* specifier operand                 */
#define PCODE_INST_OP          2       /* label operand                     */
#define PCODE_SLOT_OP          3       /* slot pointer                      */
#define PCODE_CLASS_OP         4       /* class identifier                  */
#define PCODE_PROCESS_OP       5       /* class identifier                  */

/* operand type table */

#ifdef SHARED

char pcode_optype[][3] = {             /* operand types by opcode           */
/* ## begin pcode_optypes */
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
   {  1,  1,  1 },                     /* null instruction                  */
   {  4,  0,  0 },                     /* file position flag                */
   {  1,  1,  1 },                     /* unary from                        */
/* ## end pcode_optypes */
   { -1, -1, -1 }};

#else

extern char pcode_optype[][3];         /* operand types by opcode           */

#endif

/*\
 *  \table{opcode description table}
 *
 *  We need a table of opcode names for single-step execution, when
 *  debugging.
\*/

#ifdef SHARED

char *pcode_desc[] = {                 /* print string for opcodes          */
/* ## begin pcode_desc */
   "p_noop",                           /* no operation                      */
   "p_push1",                          /* push one operand                  */
   "p_push2",                          /* push two operands                 */
   "p_push3",                          /* push three operands               */
   "p_pop1",                           /* pop one operand                   */
   "p_pop2",                           /* pop two operands                  */
   "p_pop3",                           /* pop three operands                */
   "p_add",                            /* +                                 */
   "p_sub",                            /* -                                 */
   "p_mult",                           /* *                                 */
   "p_div",                            /* /                                 */
   "p_exp",                            /* **                                */
   "p_mod",                            /* mod                               */
   "p_min",                            /* min                               */
   "p_max",                            /* max                               */
   "p_with",                           /* with                              */
   "p_less",                           /* with                              */
   "p_lessf",                          /* lessf                             */
   "p_from",                           /* from                              */
   "p_fromb",                          /* fromb                             */
   "p_frome",                          /* frome                             */
   "p_npow",                           /* npow                              */
   "p_uminus",                         /* unary minus                       */
   "p_domain",                         /* domain                            */
   "p_range",                          /* range                             */
   "p_pow",                            /* pow                               */
   "p_arb",                            /* arb                               */
   "p_nelt",                           /* #                                 */
   "p_not",                            /* not                               */
   "p_smap",                           /* convert set to smap               */
   "p_tupof",                          /* tuple `of'                        */
   "p_of1",                            /* one argument `of'                 */
   "p_of",                             /* map, tuple, or string             */
   "p_ofa",                            /* multi-valued map                  */
   "p_kof1",                           /* one argument `of'                 */
                                       /* kill temp after assignment        */
   "p_kof",                            /* map, tuple, or string             */
                                       /* kill temp after assignment        */
   "p_kofa",                           /* multi-valued map                  */
                                       /* kill temp after assignment        */
   "p_erase",                          /* kill temp                         */
   "p_slice",                          /* slice                             */
   "p_end",                            /* string end                        */
   "p_assign",                         /* general assignment                */
   "p_penviron",                       /* procedure with environment        */
   "p_sof",                            /* map, tuple, or string assignment  */
   "p_sofa",                           /* mmap sinister assignment          */
   "p_sslice",                         /* slice assignment                  */
   "p_send",                           /* string end assignment             */
   "p_eq",                             /* =                                 */
   "p_ne",                             /* /=                                */
   "p_lt",                             /* <                                 */
   "p_nlt",                            /* not <                             */
   "p_le",                             /* <=                                */
   "p_nle",                            /* not <=                            */
   "p_in",                             /* in                                */
   "p_notin",                          /* notin                             */
   "p_incs",                           /* incs                              */
   "p_and",                            /* and                               */
   "p_or",                             /* or                                */
   "p_go",                             /* branch unconditionally            */
   "p_goind",                          /* branch indirect                   */
   "p_gotrue",                         /* branch if value is true           */
   "p_gofalse",                        /* branch if value is false          */
   "p_goeq",                           /* branch if =                       */
   "p_gone",                           /* branch if /=                      */
   "p_golt",                           /* branch if <                       */
   "p_gonlt",                          /* branch if not <                   */
   "p_gole",                           /* branch if <=                      */
   "p_gonle",                          /* branch if not <=                  */
   "p_goin",                           /* branch if element                 */
   "p_gonotin",                        /* branch if not element             */
   "p_goincs",                         /* branch if includes                */
   "p_gonincs",                        /* branch if not includes            */
   "p_set",                            /* { ... }                           */
   "p_tuple",                          /* [ ... ]                           */
   "p_iter",                           /* general iterator                  */
   "p_inext",                          /* iterator next                     */
   "p_lcall",                          /* literal procedure call            */
   "p_call",                           /* procedure call                    */
   "p_return",                         /* return from procedure             */
   "p_stop",                           /* stop executing                    */
   "p_stopall",                        /* stop everything                   */
   "p_assert",                         /* assert message                    */
   "p_initobj",                        /* create new object                 */
   "p_initend",                        /* finish creation                   */
   "p_slot",                           /* reference slot                    */
   "p_sslot",                          /* assign slot                       */
   "p_slotof",                         /* call slot reference               */
   "p_menviron",                       /* method with environment           */
                                       /* or instance                       */
   "p_self",                           /* make self                         */
   "p_initproc",                       /* create new process                */
   "p_initpend",                       /* finish creation                   */
   "p_intcheck",                       /* null instruction                  */
   "p_filepos",                        /* file position flag                */
   "p_ufrom",                          /* unary from                        */
/* ## end pcode_desc */
    NULL};

#ifdef DEBUG
long pcode_operations[128];
long copy_operations[128];
#endif

#else

extern char *pcode_desc[];             /* print string for opcodes          */
#ifdef DEBUG
extern long pcode_operations[128];
extern long copy_operations[128];
#endif

#endif

#define PCODE_LOADED 1
#endif

