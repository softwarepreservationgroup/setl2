/*\
 *  \packagespec{Abstract Syntax Tree}
\*/

#ifndef AST_LOADED

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "quads.h"                     /* quadruple types                   */

/* ast node structure */

struct ast_item {
   int ast_type;                       /* node type                         */
   struct namtab_item *ast_extension;  /* Normally NULL. Used for .op syntax*/
   union {
      struct namtab_item *ast_namtab_ptr;
                                       /* name table pointer (first pass)   */
      struct symtab_item *ast_symtab_ptr;
                                       /* symbol table pointer              */
      struct ast_item *ast_child_ast;  /* child ast                         */
   } ast_child;
   struct ast_item *ast_next;          /* next in list (if list)            */
   file_pos_type ast_file_pos;         /* file position                     */
};

typedef struct ast_item *ast_ptr_type; /* node pointer                      */

/* clear an ast item */

#define clear_ast(a) { \
   (a)->ast_type = -1;                 (a)->ast_child.ast_namtab_ptr = NULL; \
   (a)->ast_file_pos.fp_line = -1;     (a)->ast_file_pos.fp_column = -1; \
   (a)->ast_extension = NULL; \
   (a)->ast_next = NULL; \
}

/*\
 *  \table{AST node types}
\*/

/* ## begin ast_types */
#define ast_null               0       /* null tree                         */
#define ast_list               1       /* statement or expression list      */
#define ast_namtab             2       /* name table pointer                */
#define ast_symtab             3       /* symbol table pointer              */
#define ast_dot                4       /* name qualifier                    */
#define ast_add                5       /* +                                 */
#define ast_sub                6       /* -                                 */
#define ast_mult               7       /* *                                 */
#define ast_div                8       /* /                                 */
#define ast_expon              9       /* **                                */
#define ast_mod               10       /* MOD                               */
#define ast_min               11       /* MIN                               */
#define ast_max               12       /* MAX                               */
#define ast_question          13       /* ?                                 */
#define ast_with              14       /* with operator                     */
#define ast_less              15       /* less operator                     */
#define ast_lessf             16       /* lessf operator                    */
#define ast_npow              17       /* npow operator                     */
#define ast_uminus            18       /* unary minus                       */
#define ast_ufrom             19       /* unary from                        */
#define ast_domain            20       /* map domain                        */
#define ast_range             21       /* map range                         */
#define ast_not               22       /* not                               */
#define ast_arb               23       /* arb                               */
#define ast_pow               24       /* pow                               */
#define ast_nelt              25       /* #                                 */
#define ast_of                26       /* string, map, or tuple component   */
#define ast_ofa               27       /* multi-valued map `of'             */
#define ast_kof               28       /* string, map, or tuple component   */
                                       /* kill temp after assignment        */
#define ast_kofa              29       /* multi-valued map `of'             */
                                       /* kill temp after assignment        */
#define ast_slice             30       /* string or tuple slice             */
#define ast_end               31       /* string or tuple tail              */
#define ast_assign            32       /* general assignment                */
#define ast_assignop          33       /* assignment operators              */
#define ast_penviron          34       /* procedure with environment        */
#define ast_cassign           35       /* constant initialization           */
#define ast_placeholder       36       /* place holder in tuple lhs         */
#define ast_from              37       /* from operator                     */
#define ast_fromb             38       /* fromb operator                    */
#define ast_frome             39       /* frome operator                    */
#define ast_eq                40       /* =                                 */
#define ast_ne                41       /* /=                                */
#define ast_lt                42       /* <                                 */
#define ast_le                43       /* <=                                */
#define ast_gt                44       /* >                                 */
#define ast_ge                45       /* >=                                */
#define ast_in                46       /* in                                */
#define ast_notin             47       /* notin                             */
#define ast_incs              48       /* in                                */
#define ast_subset            49       /* subset                            */
#define ast_or                50       /* or operator                       */
#define ast_and               51       /* and operator                      */
#define ast_enum_set          52       /* enumerated set                    */
#define ast_enum_tup          53       /* enumerated tuple                  */
#define ast_genset            54       /* general set former                */
#define ast_gentup            55       /* general tuple former              */
#define ast_genset_noexp      56       /* general set former without        */
                                       /* expression                        */
#define ast_gentup_noexp      57       /* general tuple former without      */
                                       /* expression                        */
#define ast_arith_set         58       /* arithmetic set former             */
#define ast_arith_tup         59       /* arithmetic tuple former           */
#define ast_exists            60       /* exists expression                 */
#define ast_forall            61       /* forall expression                 */
#define ast_apply             62       /* application over set              */
#define ast_binapply          63       /* binary application over set       */
#define ast_iter_list         64       /* iterator list                     */
#define ast_ex_iter           65       /* exists iterator list              */
#define ast_if_stmt           66       /* if statement                      */
#define ast_if_expr           67       /* if expression                     */
#define ast_loop              68       /* loop statement                    */
#define ast_while             69       /* while statement                   */
#define ast_until             70       /* until statement                   */
#define ast_for               71       /* for statement                     */
#define ast_case_stmt         72       /* case statement                    */
#define ast_case_expr         73       /* case expression                   */
#define ast_guard_stmt        74       /* guard statement                   */
#define ast_guard_expr        75       /* guard expression                  */
#define ast_when              76       /* when clause                       */
#define ast_call              77       /* procedure call                    */
#define ast_return            78       /* return statement                  */
#define ast_stop              79       /* stop statement                    */
#define ast_exit              80       /* break out of loop                 */
#define ast_continue          81       /* continue loop                     */
#define ast_assert            82       /* assert expressions                */
#define ast_initobj           83       /* initialize object                 */
#define ast_slot              84       /* slot reference                    */
#define ast_slotof            85       /* call slot reference               */
#define ast_slotcall          86       /* call slot reference               */
#define ast_menviron          87       /* method with environment           */
                                       /* or instance                       */
#define ast_self              88       /* self reference                    */
/* ## end ast_types */

/*\
 *  \table{default opcodes}
 *
 *  We try to merge the handling of various AST types into a single case
 *  in the code generation packages.  This and the following two tables
 *  give opcodes used for that purpose.
\*/

#ifdef SHARED

char ast_default_opcode[] = {          /* default qcode for ast type        */
/* ## begin ast_default_opcodes */
   q_noop,                             /* null tree                         */
   q_noop,                             /* statement or expression list      */
   q_noop,                             /* name table pointer                */
   q_noop,                             /* symbol table pointer              */
   q_noop,                             /* name qualifier                    */
   q_add,                              /* +                                 */
   q_sub,                              /* -                                 */
   q_mult,                             /* *                                 */
   q_div,                              /* /                                 */
   q_exp,                              /* **                                */
   q_mod,                              /* MOD                               */
   q_min,                              /* MIN                               */
   q_max,                              /* MAX                               */
   q_noop,                             /* ?                                 */
   q_with,                             /* with operator                     */
   q_less,                             /* less operator                     */
   q_lessf,                            /* lessf operator                    */
   q_npow,                             /* npow operator                     */
   q_uminus,                           /* unary minus                       */
   q_ufrom,                            /* unary from                        */
   q_domain,                           /* map domain                        */
   q_range,                            /* map range                         */
   q_not,                              /* not                               */
   q_arb,                              /* arb                               */
   q_pow,                              /* pow                               */
   q_nelt,                             /* #                                 */
   q_of,                               /* string, map, or tuple component   */
   q_ofa,                              /* multi-valued map `of'             */
   q_kof,                              /* string, map, or tuple component   */
                                       /* kill temp after assignment        */
   q_kofa,                             /* multi-valued map `of'             */
                                       /* kill temp after assignment        */
   q_slice,                            /* string or tuple slice             */
   q_end,                              /* string or tuple tail              */
   q_assign,                           /* general assignment                */
   q_assign,                           /* assignment operators              */
   q_penviron,                         /* procedure with environment        */
   q_assign,                           /* constant initialization           */
   q_noop,                             /* place holder in tuple lhs         */
   q_from,                             /* from operator                     */
   q_fromb,                            /* fromb operator                    */
   q_frome,                            /* frome operator                    */
   q_eq,                               /* =                                 */
   q_ne,                               /* /=                                */
   q_lt,                               /* <                                 */
   q_le,                               /* <=                                */
   q_lt,                               /* >                                 */
   q_le,                               /* >=                                */
   q_in,                               /* in                                */
   q_notin,                            /* notin                             */
   q_incs,                             /* in                                */
   q_incs,                             /* subset                            */
   q_or,                               /* or operator                       */
   q_and,                              /* and operator                      */
   q_set,                              /* enumerated set                    */
   q_tuple,                            /* enumerated tuple                  */
   q_set,                              /* general set former                */
   q_tuple,                            /* general tuple former              */
   q_set,                              /* general set former without        */
                                       /* expression                        */
   q_tuple,                            /* general tuple former without      */
                                       /* expression                        */
   q_set,                              /* arithmetic set former             */
   q_tuple,                            /* arithmetic tuple former           */
   q_noop,                             /* exists expression                 */
   q_noop,                             /* forall expression                 */
   q_noop,                             /* application over set              */
   q_noop,                             /* binary application over set       */
   q_noop,                             /* iterator list                     */
   q_noop,                             /* exists iterator list              */
   q_noop,                             /* if statement                      */
   q_noop,                             /* if expression                     */
   q_noop,                             /* loop statement                    */
   q_noop,                             /* while statement                   */
   q_noop,                             /* until statement                   */
   q_noop,                             /* for statement                     */
   q_noop,                             /* case statement                    */
   q_noop,                             /* case expression                   */
   q_noop,                             /* guard statement                   */
   q_noop,                             /* guard expression                  */
   q_noop,                             /* when clause                       */
   q_call,                             /* procedure call                    */
   q_return,                           /* return statement                  */
   q_noop,                             /* stop statement                    */
   q_noop,                             /* break out of loop                 */
   q_noop,                             /* continue loop                     */
   q_noop,                             /* assert expressions                */
   q_initobj,                          /* initialize object                 */
   q_slot,                             /* slot reference                    */
   q_slotof,                           /* call slot reference               */
   q_slotof,                           /* call slot reference               */
   q_menviron,                         /* method with environment           */
                                       /* or instance                       */
   q_self,                             /* self reference                    */
/* ## end ast_default_opcodes */
   -1};

#else

extern char ast_default_opcode[];      /* default qcode for ast type        */

#endif

/*\
 *  \table{true branch opcodes}
\*/

#ifdef SHARED

char ast_true_opcode[] = {             /* true branch qcode for ast type    */
/* ## begin ast_true_opcodes */
   q_noop,                             /* null tree                         */
   q_noop,                             /* statement or expression list      */
   q_noop,                             /* name table pointer                */
   q_noop,                             /* symbol table pointer              */
   q_noop,                             /* name qualifier                    */
   q_noop,                             /* +                                 */
   q_noop,                             /* -                                 */
   q_noop,                             /* *                                 */
   q_noop,                             /* /                                 */
   q_noop,                             /* **                                */
   q_noop,                             /* MOD                               */
   q_noop,                             /* MIN                               */
   q_noop,                             /* MAX                               */
   q_noop,                             /* ?                                 */
   q_noop,                             /* with operator                     */
   q_noop,                             /* less operator                     */
   q_noop,                             /* lessf operator                    */
   q_noop,                             /* npow operator                     */
   q_noop,                             /* unary minus                       */
   q_noop,                             /* unary from                        */
   q_noop,                             /* map domain                        */
   q_noop,                             /* map range                         */
   q_noop,                             /* not                               */
   q_noop,                             /* arb                               */
   q_noop,                             /* pow                               */
   q_noop,                             /* #                                 */
   q_noop,                             /* string, map, or tuple component   */
   q_noop,                             /* multi-valued map `of'             */
   q_noop,                             /* string, map, or tuple component   */
                                       /* kill temp after assignment        */
   q_noop,                             /* multi-valued map `of'             */
                                       /* kill temp after assignment        */
   q_noop,                             /* string or tuple slice             */
   q_noop,                             /* string or tuple tail              */
   q_noop,                             /* general assignment                */
   q_noop,                             /* assignment operators              */
   q_noop,                             /* procedure with environment        */
   q_noop,                             /* constant initialization           */
   q_noop,                             /* place holder in tuple lhs         */
   q_noop,                             /* from operator                     */
   q_noop,                             /* fromb operator                    */
   q_noop,                             /* frome operator                    */
   q_goeq,                             /* =                                 */
   q_gone,                             /* /=                                */
   q_golt,                             /* <                                 */
   q_gole,                             /* <=                                */
   q_golt,                             /* >                                 */
   q_gole,                             /* >=                                */
   q_goin,                             /* in                                */
   q_gonotin,                          /* notin                             */
   q_goincs,                           /* in                                */
   q_goincs,                           /* subset                            */
   q_noop,                             /* or operator                       */
   q_noop,                             /* and operator                      */
   q_noop,                             /* enumerated set                    */
   q_noop,                             /* enumerated tuple                  */
   q_noop,                             /* general set former                */
   q_noop,                             /* general tuple former              */
   q_noop,                             /* general set former without        */
                                       /* expression                        */
   q_noop,                             /* general tuple former without      */
                                       /* expression                        */
   q_noop,                             /* arithmetic set former             */
   q_noop,                             /* arithmetic tuple former           */
   q_noop,                             /* exists expression                 */
   q_noop,                             /* forall expression                 */
   q_noop,                             /* application over set              */
   q_noop,                             /* binary application over set       */
   q_noop,                             /* iterator list                     */
   q_noop,                             /* exists iterator list              */
   q_noop,                             /* if statement                      */
   q_noop,                             /* if expression                     */
   q_noop,                             /* loop statement                    */
   q_noop,                             /* while statement                   */
   q_noop,                             /* until statement                   */
   q_noop,                             /* for statement                     */
   q_noop,                             /* case statement                    */
   q_noop,                             /* case expression                   */
   q_noop,                             /* guard statement                   */
   q_noop,                             /* guard expression                  */
   q_noop,                             /* when clause                       */
   q_noop,                             /* procedure call                    */
   q_noop,                             /* return statement                  */
   q_noop,                             /* stop statement                    */
   q_noop,                             /* break out of loop                 */
   q_noop,                             /* continue loop                     */
   q_noop,                             /* assert expressions                */
   q_noop,                             /* initialize object                 */
   q_noop,                             /* slot reference                    */
   q_noop,                             /* call slot reference               */
   q_noop,                             /* call slot reference               */
   q_noop,                             /* method with environment           */
                                       /* or instance                       */
   q_noop,                             /* self reference                    */
/* ## end ast_true_opcodes */
   -1};

#else

extern char ast_true_opcode[];         /* true branch qcode for ast type    */

#endif

/*\
 *  \table{false branch opcodes}
\*/

#ifdef SHARED

char ast_false_opcode[] = {            /* false branch qcode for ast type   */
/* ## begin ast_false_opcodes */
   q_noop,                             /* null tree                         */
   q_noop,                             /* statement or expression list      */
   q_noop,                             /* name table pointer                */
   q_noop,                             /* symbol table pointer              */
   q_noop,                             /* name qualifier                    */
   q_noop,                             /* +                                 */
   q_noop,                             /* -                                 */
   q_noop,                             /* *                                 */
   q_noop,                             /* /                                 */
   q_noop,                             /* **                                */
   q_noop,                             /* MOD                               */
   q_noop,                             /* MIN                               */
   q_noop,                             /* MAX                               */
   q_noop,                             /* ?                                 */
   q_noop,                             /* with operator                     */
   q_noop,                             /* less operator                     */
   q_noop,                             /* lessf operator                    */
   q_noop,                             /* npow operator                     */
   q_noop,                             /* unary minus                       */
   q_noop,                             /* unary from                        */
   q_noop,                             /* map domain                        */
   q_noop,                             /* map range                         */
   q_noop,                             /* not                               */
   q_noop,                             /* arb                               */
   q_noop,                             /* pow                               */
   q_noop,                             /* #                                 */
   q_noop,                             /* string, map, or tuple component   */
   q_noop,                             /* multi-valued map `of'             */
   q_noop,                             /* string, map, or tuple component   */
                                       /* kill temp after assignment        */
   q_noop,                             /* multi-valued map `of'             */
                                       /* kill temp after assignment        */
   q_noop,                             /* string or tuple slice             */
   q_noop,                             /* string or tuple tail              */
   q_noop,                             /* general assignment                */
   q_noop,                             /* assignment operators              */
   q_noop,                             /* procedure with environment        */
   q_noop,                             /* constant initialization           */
   q_noop,                             /* place holder in tuple lhs         */
   q_noop,                             /* from operator                     */
   q_noop,                             /* fromb operator                    */
   q_noop,                             /* frome operator                    */
   q_gone,                             /* =                                 */
   q_goeq,                             /* /=                                */
   q_gonlt,                            /* <                                 */
   q_gonle,                            /* <=                                */
   q_gonlt,                            /* >                                 */
   q_gonle,                            /* >=                                */
   q_gonotin,                          /* in                                */
   q_goin,                             /* notin                             */
   q_gonincs,                          /* in                                */
   q_gonincs,                          /* subset                            */
   q_noop,                             /* or operator                       */
   q_noop,                             /* and operator                      */
   q_noop,                             /* enumerated set                    */
   q_noop,                             /* enumerated tuple                  */
   q_noop,                             /* general set former                */
   q_noop,                             /* general tuple former              */
   q_noop,                             /* general set former without        */
                                       /* expression                        */
   q_noop,                             /* general tuple former without      */
                                       /* expression                        */
   q_noop,                             /* arithmetic set former             */
   q_noop,                             /* arithmetic tuple former           */
   q_noop,                             /* exists expression                 */
   q_noop,                             /* forall expression                 */
   q_noop,                             /* application over set              */
   q_noop,                             /* binary application over set       */
   q_noop,                             /* iterator list                     */
   q_noop,                             /* exists iterator list              */
   q_noop,                             /* if statement                      */
   q_noop,                             /* if expression                     */
   q_noop,                             /* loop statement                    */
   q_noop,                             /* while statement                   */
   q_noop,                             /* until statement                   */
   q_noop,                             /* for statement                     */
   q_noop,                             /* case statement                    */
   q_noop,                             /* case expression                   */
   q_noop,                             /* guard statement                   */
   q_noop,                             /* guard expression                  */
   q_noop,                             /* when clause                       */
   q_noop,                             /* procedure call                    */
   q_noop,                             /* return statement                  */
   q_noop,                             /* stop statement                    */
   q_noop,                             /* break out of loop                 */
   q_noop,                             /* continue loop                     */
   q_noop,                             /* assert expressions                */
   q_noop,                             /* initialize object                 */
   q_noop,                             /* slot reference                    */
   q_noop,                             /* call slot reference               */
   q_noop,                             /* call slot reference               */
   q_noop,                             /* method with environment           */
                                       /* or instance                       */
   q_noop,                             /* self reference                    */
/* ## end ast_false_opcodes */
   -1};

#else

extern char ast_false_opcode[];        /* false branch qcode for ast type   */

#endif

/*\
 *  \table{flip operands table}
 *
 *  To avoid superfluous opcodes, we require some operators to flip their
 *  operands.  For example: instead of a {\it branch on less than}
 *  instruction, we use {\it branch on greater or equal} instruction with
 *  operands reversed.
\*/

#ifdef SHARED

char ast_flip_operands[] = {           /* YES if operands should be flipped */
/* ## begin ast_flip_operands */
   0,                                  /* null tree                         */
   0,                                  /* statement or expression list      */
   0,                                  /* name table pointer                */
   0,                                  /* symbol table pointer              */
   0,                                  /* name qualifier                    */
   0,                                  /* +                                 */
   0,                                  /* -                                 */
   0,                                  /* *                                 */
   0,                                  /* /                                 */
   0,                                  /* **                                */
   0,                                  /* MOD                               */
   0,                                  /* MIN                               */
   0,                                  /* MAX                               */
   0,                                  /* ?                                 */
   0,                                  /* with operator                     */
   0,                                  /* less operator                     */
   0,                                  /* lessf operator                    */
   0,                                  /* npow operator                     */
   0,                                  /* unary minus                       */
   0,                                  /* unary from                        */
   0,                                  /* map domain                        */
   0,                                  /* map range                         */
   0,                                  /* not                               */
   0,                                  /* arb                               */
   0,                                  /* pow                               */
   0,                                  /* #                                 */
   0,                                  /* string, map, or tuple component   */
   0,                                  /* multi-valued map `of'             */
   0,                                  /* string, map, or tuple component   */
                                       /* kill temp after assignment        */
   0,                                  /* multi-valued map `of'             */
                                       /* kill temp after assignment        */
   0,                                  /* string or tuple slice             */
   0,                                  /* string or tuple tail              */
   0,                                  /* general assignment                */
   0,                                  /* assignment operators              */
   0,                                  /* procedure with environment        */
   0,                                  /* constant initialization           */
   0,                                  /* place holder in tuple lhs         */
   0,                                  /* from operator                     */
   0,                                  /* fromb operator                    */
   0,                                  /* frome operator                    */
   0,                                  /* =                                 */
   0,                                  /* /=                                */
   0,                                  /* <                                 */
   0,                                  /* <=                                */
   1,                                  /* >                                 */
   1,                                  /* >=                                */
   0,                                  /* in                                */
   0,                                  /* notin                             */
   0,                                  /* in                                */
   1,                                  /* subset                            */
   0,                                  /* or operator                       */
   0,                                  /* and operator                      */
   0,                                  /* enumerated set                    */
   0,                                  /* enumerated tuple                  */
   0,                                  /* general set former                */
   0,                                  /* general tuple former              */
   0,                                  /* general set former without        */
                                       /* expression                        */
   0,                                  /* general tuple former without      */
                                       /* expression                        */
   0,                                  /* arithmetic set former             */
   0,                                  /* arithmetic tuple former           */
   0,                                  /* exists expression                 */
   0,                                  /* forall expression                 */
   0,                                  /* application over set              */
   0,                                  /* binary application over set       */
   0,                                  /* iterator list                     */
   0,                                  /* exists iterator list              */
   0,                                  /* if statement                      */
   0,                                  /* if expression                     */
   0,                                  /* loop statement                    */
   0,                                  /* while statement                   */
   0,                                  /* until statement                   */
   0,                                  /* for statement                     */
   0,                                  /* case statement                    */
   0,                                  /* case expression                   */
   0,                                  /* guard statement                   */
   0,                                  /* guard expression                  */
   0,                                  /* when clause                       */
   0,                                  /* procedure call                    */
   0,                                  /* return statement                  */
   0,                                  /* stop statement                    */
   0,                                  /* break out of loop                 */
   0,                                  /* continue loop                     */
   0,                                  /* assert expressions                */
   0,                                  /* initialize object                 */
   0,                                  /* slot reference                    */
   0,                                  /* call slot reference               */
   0,                                  /* call slot reference               */
   0,                                  /* method with environment           */
                                       /* or instance                       */
   0,                                  /* self reference                    */
/* ## end ast_flip_operands */
   -1};

#else

extern char ast_flip_operands[];       /* YES if operands should be flipped */

#endif

/*\
 *  \table{AST description table}
 *
 *  During debugging we print out the AST.  This table gives us printable
 *  node names.
\*/

#ifdef DEBUG
#ifdef SHARED

char *ast_desc[] = {                   /* print string for AST types        */
/* ## begin ast_desc */
   "ast_null",                         /* null tree                         */
   "ast_list",                         /* statement or expression list      */
   "ast_namtab",                       /* name table pointer                */
   "ast_symtab",                       /* symbol table pointer              */
   "ast_dot",                          /* name qualifier                    */
   "ast_add",                          /* +                                 */
   "ast_sub",                          /* -                                 */
   "ast_mult",                         /* *                                 */
   "ast_div",                          /* /                                 */
   "ast_expon",                        /* **                                */
   "ast_mod",                          /* MOD                               */
   "ast_min",                          /* MIN                               */
   "ast_max",                          /* MAX                               */
   "ast_question",                     /* ?                                 */
   "ast_with",                         /* with operator                     */
   "ast_less",                         /* less operator                     */
   "ast_lessf",                        /* lessf operator                    */
   "ast_npow",                         /* npow operator                     */
   "ast_uminus",                       /* unary minus                       */
   "ast_ufrom",                        /* unary from                        */
   "ast_domain",                       /* map domain                        */
   "ast_range",                        /* map range                         */
   "ast_not",                          /* not                               */
   "ast_arb",                          /* arb                               */
   "ast_pow",                          /* pow                               */
   "ast_nelt",                         /* #                                 */
   "ast_of",                           /* string, map, or tuple component   */
   "ast_ofa",                          /* multi-valued map `of'             */
   "ast_kof",                          /* string, map, or tuple component   */
                                       /* kill temp after assignment        */
   "ast_kofa",                         /* multi-valued map `of'             */
                                       /* kill temp after assignment        */
   "ast_slice",                        /* string or tuple slice             */
   "ast_end",                          /* string or tuple tail              */
   "ast_assign",                       /* general assignment                */
   "ast_assignop",                     /* assignment operators              */
   "ast_penviron",                     /* procedure with environment        */
   "ast_cassign",                      /* constant initialization           */
   "ast_placeholder",                  /* place holder in tuple lhs         */
   "ast_from",                         /* from operator                     */
   "ast_fromb",                        /* fromb operator                    */
   "ast_frome",                        /* frome operator                    */
   "ast_eq",                           /* =                                 */
   "ast_ne",                           /* /=                                */
   "ast_lt",                           /* <                                 */
   "ast_le",                           /* <=                                */
   "ast_gt",                           /* >                                 */
   "ast_ge",                           /* >=                                */
   "ast_in",                           /* in                                */
   "ast_notin",                        /* notin                             */
   "ast_incs",                         /* in                                */
   "ast_subset",                       /* subset                            */
   "ast_or",                           /* or operator                       */
   "ast_and",                          /* and operator                      */
   "ast_enum_set",                     /* enumerated set                    */
   "ast_enum_tup",                     /* enumerated tuple                  */
   "ast_genset",                       /* general set former                */
   "ast_gentup",                       /* general tuple former              */
   "ast_genset_noexp",                 /* general set former without        */
                                       /* expression                        */
   "ast_gentup_noexp",                 /* general tuple former without      */
                                       /* expression                        */
   "ast_arith_set",                    /* arithmetic set former             */
   "ast_arith_tup",                    /* arithmetic tuple former           */
   "ast_exists",                       /* exists expression                 */
   "ast_forall",                       /* forall expression                 */
   "ast_apply",                        /* application over set              */
   "ast_binapply",                     /* binary application over set       */
   "ast_iter_list",                    /* iterator list                     */
   "ast_ex_iter",                      /* exists iterator list              */
   "ast_if_stmt",                      /* if statement                      */
   "ast_if_expr",                      /* if expression                     */
   "ast_loop",                         /* loop statement                    */
   "ast_while",                        /* while statement                   */
   "ast_until",                        /* until statement                   */
   "ast_for",                          /* for statement                     */
   "ast_case_stmt",                    /* case statement                    */
   "ast_case_expr",                    /* case expression                   */
   "ast_guard_stmt",                   /* guard statement                   */
   "ast_guard_expr",                   /* guard expression                  */
   "ast_when",                         /* when clause                       */
   "ast_call",                         /* procedure call                    */
   "ast_return",                       /* return statement                  */
   "ast_stop",                         /* stop statement                    */
   "ast_exit",                         /* break out of loop                 */
   "ast_continue",                     /* continue loop                     */
   "ast_assert",                       /* assert expressions                */
   "ast_initobj",                      /* initialize object                 */
   "ast_slot",                         /* slot reference                    */
   "ast_slotof",                       /* call slot reference               */
   "ast_slotcall",                     /* call slot reference               */
   "ast_menviron",                     /* method with environment           */
                                       /* or instance                       */
   "ast_self",                         /* self reference                    */
/* ## end ast_desc */
   NULL};

#else

EXTERNAL char *ast_desc[];               /* print string for AST types        */

#endif
#endif

/*\
 *  \nobar\newpage
\*/

/* public function declarations */

void init_ast(void);                   /* clear abstract syntax tree        */
ast_ptr_type get_ast(SETL_SYSTEM_PROTO_VOID);
                                       /* allocate a new item               */
void free_ast(ast_ptr_type);           /* return an item to the free pool   */
void store_ast(SETL_SYSTEM_PROTO struct storage_location_item *, 
               struct ast_item *);
                                       /* save an ast in intermediate file  */
struct ast_item *load_ast(SETL_SYSTEM_PROTO struct storage_location_item *);
                                       /* load ast from intermediate file   */
void kill_ast(struct ast_item *);      /* release memory used by ast        */
#ifdef DEBUG
void print_ast(SETL_SYSTEM_PROTO ast_ptr_type,char *);   
                                       /* print AST                         */
#endif

#define AST_LOADED 1
#endif
