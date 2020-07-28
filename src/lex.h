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
 *  \packagespec{Lexical Analyzer Declarations}
\*/

#ifndef LEX_LOADED

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "mcode.h"                     /* method codes                      */

/* token structure */

typedef struct {
   int tk_token_class;                 /* token class                       */
   int tk_token_subclass;              /* token subclass                    */
   file_pos_type tk_file_pos;          /* file position of token            */
   struct namtab_item *tk_namtab_ptr;  /* name table pointer                */
} token_type;

/*\
 *  \table{token classes}
 *
 *  We merge token classes and token subclasses into a single sequence of
 *  numbers.  Token classes appear first in this sequence, to accommodate
 *  the parser.  Subclasses start with the literals.
 *
 *  Although the parser works with token classes, most of the tables are
 *  by subclass.
\*/

/* ## begin token_classes */
#define tok_eof                0       /* end of file                       */
#define tok_error              1       /* error token                       */
#define tok_id                 2       /* identifier                        */
#define tok_literal            3       /* literal                           */
#define tok_and                4       /* keyword => AND                    */
#define tok_assert             5       /* keyword => ASSERT                 */
#define tok_body               6       /* keyword => BODY                   */
#define tok_case               7       /* keyword => CASE                   */
#define tok_class              8       /* keyword => CLASS                  */
#define tok_const              9       /* keyword => CONST                  */
#define tok_continue          10       /* keyword => CONTINUE               */
#define tok_else              11       /* keyword => ELSE                   */
#define tok_elseif            12       /* keyword => ELSEIF                 */
#define tok_end               13       /* keyword => END                    */
#define tok_exit              14       /* keyword => EXIT                   */
#define tok_for               15       /* keyword => FOR                    */
#define tok_if                16       /* keyword => IF                     */
#define tok_inherit           17       /* keyword => INHERIT                */
#define tok_lambda            18       /* keyword => LAMBDA                 */
#define tok_loop              19       /* keyword => LOOP                   */
#define tok_not               20       /* keyword => NOT                    */
#define tok_null              21       /* keyword => NULL                   */
#define tok_or                22       /* keyword => OR                     */
#define tok_otherwise         23       /* keyword => OTHERWISE              */
#define tok_package           24       /* keyword => PACKAGE                */
#define tok_procedure         25       /* keyword => PROCEDURE              */
#define tok_process           26       /* keyword => PROCESS                */
#define tok_program           27       /* keyword => PROGRAM                */
#define tok_rd                28       /* keyword => RD                     */
#define tok_return            29       /* keyword => RETURN                 */
#define tok_rw                30       /* keyword => RW                     */
#define tok_sel               31       /* keyword => SEL                    */
#define tok_self              32       /* keyword => SELF                   */
#define tok_stop              33       /* keyword => STOP                   */
#define tok_then              34       /* keyword => THEN                   */
#define tok_until             35       /* keyword => UNTIL                  */
#define tok_use               36       /* keyword => USE                    */
#define tok_var               37       /* keyword => VAR                    */
#define tok_when              38       /* keyword => WHEN                   */
#define tok_while             39       /* keyword => WHILE                  */
#define tok_wr                40       /* keyword => WR                     */
#define tok_semi              41       /* ;                                 */
#define tok_comma             42       /* ,                                 */
#define tok_colon             43       /* :                                 */
#define tok_lparen            44       /* (                                 */
#define tok_rparen            45       /* )                                 */
#define tok_lbracket          46       /* [                                 */
#define tok_rbracket          47       /* ]                                 */
#define tok_lbrace            48       /* {                                 */
#define tok_rbrace            49       /* }                                 */
#define tok_dot               50       /* .                                 */
#define tok_dotdot            51       /* ..                                */
#define tok_assign            52       /* :=                                */
#define tok_suchthat          53       /* |                                 */
#define tok_rarrow            54       /* =>                                */
#define tok_assignop          55       /* assignment operator               */
#define tok_applyop           56       /* assignment operator               */
#define tok_unop              57       /* unary operator                    */
#define tok_caret             58       /* pointer reference                 */
#define tok_addop             59       /* addop                             */
#define tok_dash              60       /* -                                 */
#define tok_mulop             61       /* mulop                             */
#define tok_expon             62       /* **                                */
#define tok_relop             63       /* relop                             */
#define tok_fromop            64       /* fromop                            */
#define tok_quantifier        65       /* quantifier                        */
#define tok_native            66       /* keyword => NATIVE                 */
#define tok_integer           67       /* ;                                 */
#define tok_real              68       /* .                                 */
#define tok_string            69       /* (                                 */
#define tok_nelt              70       /* #                                 */
#define tok_pow               71       /* POW                               */
#define tok_arb               72       /* ARB                               */
#define tok_dom               73       /* DOMAIN                            */
#define tok_range             74       /* RANGE                             */
#define tok_plus              75       /* +                                 */
#define tok_asnplus           76       /* +:=                               */
#define tok_appplus           77       /* +/                                */
#define tok_asnsub            78       /* -:=                               */
#define tok_appsub            79       /* -/                                */
#define tok_question          80       /* ?                                 */
#define tok_asnquestion       81       /* ?:=                               */
#define tok_appquestion       82       /* ?/                                */
#define tok_mult              83       /* *                                 */
#define tok_asnmult           84       /* *:=                               */
#define tok_appmult           85       /* * /                               */
#define tok_slash             86       /* /                                 */
#define tok_asnslash          87       /* /:=                               */
#define tok_appslash          88       /* //                                */
#define tok_mod               89       /* MOD                               */
#define tok_asnmod            90       /* MOD:=                             */
#define tok_appmod            91       /* MOD/                              */
#define tok_min               92       /* MIN                               */
#define tok_asnmin            93       /* MIN:=                             */
#define tok_appmin            94       /* MIN/                              */
#define tok_max               95       /* MAX                               */
#define tok_asnmax            96       /* MAX:=                             */
#define tok_appmax            97       /* MAX/                              */
#define tok_with              98       /* WITH                              */
#define tok_asnwith           99       /* WITH:=                            */
#define tok_appwith          100       /* WITH/                             */
#define tok_less             101       /* LESS                              */
#define tok_asnless          102       /* LESS:=                            */
#define tok_appless          103       /* LESS/                             */
#define tok_lessf            104       /* LESSF                             */
#define tok_asnlessf         105       /* LESSF:=                           */
#define tok_applessf         106       /* LESSF/                            */
#define tok_npow             107       /* NPOW                              */
#define tok_asnnpow          108       /* NPOW:=                            */
#define tok_appnpow          109       /* NPOW/                             */
#define tok_eq               110       /* =                                 */
#define tok_asneq            111       /* =:=                               */
#define tok_appeq            112       /* =/                                */
#define tok_ne               113       /* /=                                */
#define tok_asnne            114       /* /=:=                              */
#define tok_appne            115       /* /=/                               */
#define tok_lt               116       /* <                                 */
#define tok_asnlt            117       /* <:=                               */
#define tok_applt            118       /* </                                */
#define tok_le               119       /* <=                                */
#define tok_asnle            120       /* <=:=                              */
#define tok_apple            121       /* <=/                               */
#define tok_gt               122       /* >                                 */
#define tok_asngt            123       /* >:=                               */
#define tok_appgt            124       /* >/                                */
#define tok_ge               125       /* >=                                */
#define tok_asnge            126       /* >=:=                              */
#define tok_appge            127       /* >=/                               */
#define tok_in               128       /* IN                                */
#define tok_asnin            129       /* IN:=                              */
#define tok_appin            130       /* IN/                               */
#define tok_notin            131       /* NOTIN                             */
#define tok_asnnotin         132       /* NOTIN:=                           */
#define tok_appnotin         133       /* NOTIN/                            */
#define tok_subset           134       /* SUBSET                            */
#define tok_asnsubset        135       /* SUBSET:=                          */
#define tok_appsubset        136       /* SUBSET/                           */
#define tok_incs             137       /* INCS                              */
#define tok_asnincs          138       /* INCS:=                            */
#define tok_appincs          139       /* INCS/                             */
#define tok_asnand           140       /* AND:=                             */
#define tok_appand           141       /* AND/                              */
#define tok_asnor            142       /* OR:=                              */
#define tok_appor            143       /* OR/                               */
#define tok_from             144       /* FROM                              */
#define tok_fromb            145       /* FROMB                             */
#define tok_frome            146       /* FROME                             */
#define tok_exists           147       /* EXISTS                            */
#define tok_forall           148       /* FORALL                            */
/* ## end token_classes */

/*\
 *  \table{default AST types}
 *
 *  We keep a table of default AST types for the semantic actions.  In
 *  many places we merge many token subclasses into a single class, and
 *  use this table to determine the type of node to create.
\*/

#ifdef SHARED

char tok_ast_type[] = {                /* ast node type for each token      */
/* ## begin default_ast */
   ast_null,                           /* end of file                       */
   ast_null,                           /* error token                       */
   ast_null,                           /* identifier                        */
   ast_null,                           /* literal                           */
   ast_and,                            /* keyword => AND                    */
   ast_null,                           /* keyword => ASSERT                 */
   ast_null,                           /* keyword => BODY                   */
   ast_null,                           /* keyword => CASE                   */
   ast_null,                           /* keyword => CLASS                  */
   ast_null,                           /* keyword => CONST                  */
   ast_null,                           /* keyword => CONTINUE               */
   ast_null,                           /* keyword => ELSE                   */
   ast_null,                           /* keyword => ELSEIF                 */
   ast_null,                           /* keyword => END                    */
   ast_null,                           /* keyword => EXIT                   */
   ast_null,                           /* keyword => FOR                    */
   ast_null,                           /* keyword => IF                     */
   ast_null,                           /* keyword => INHERIT                */
   ast_null,                           /* keyword => LAMBDA                 */
   ast_null,                           /* keyword => LOOP                   */
   ast_null,                           /* keyword => NOT                    */
   ast_null,                           /* keyword => NULL                   */
   ast_or,                             /* keyword => OR                     */
   ast_null,                           /* keyword => OTHERWISE              */
   ast_null,                           /* keyword => PACKAGE                */
   ast_null,                           /* keyword => PROCEDURE              */
   ast_null,                           /* keyword => PROCESS                */
   ast_null,                           /* keyword => PROGRAM                */
   ast_null,                           /* keyword => RD                     */
   ast_return,                         /* keyword => RETURN                 */
   ast_null,                           /* keyword => RW                     */
   ast_null,                           /* keyword => SEL                    */
   ast_null,                           /* keyword => SELF                   */
   ast_null,                           /* keyword => STOP                   */
   ast_null,                           /* keyword => THEN                   */
   ast_null,                           /* keyword => UNTIL                  */
   ast_null,                           /* keyword => USE                    */
   ast_null,                           /* keyword => VAR                    */
   ast_null,                           /* keyword => WHEN                   */
   ast_null,                           /* keyword => WHILE                  */
   ast_null,                           /* keyword => WR                     */
   ast_null,                           /* ;                                 */
   ast_null,                           /* ,                                 */
   ast_null,                           /* :                                 */
   ast_null,                           /* (                                 */
   ast_null,                           /* )                                 */
   ast_null,                           /* [                                 */
   ast_null,                           /* ]                                 */
   ast_null,                           /* {                                 */
   ast_null,                           /* }                                 */
   ast_null,                           /* .                                 */
   ast_null,                           /* ..                                */
   ast_assign,                         /* :=                                */
   ast_null,                           /* |                                 */
   ast_null,                           /* =>                                */
   ast_null,                           /* assignment operator               */
   ast_null,                           /* assignment operator               */
   ast_null,                           /* unary operator                    */
   ast_of,                             /* pointer reference                 */
   ast_null,                           /* addop                             */
   ast_sub,                            /* -                                 */
   ast_null,                           /* mulop                             */
   ast_expon,                          /* **                                */
   ast_null,                           /* relop                             */
   ast_null,                           /* fromop                            */
   ast_null,                           /* quantifier                        */
   ast_null,                           /* keyword => NATIVE                 */
   ast_null,                           /* ;                                 */
   ast_null,                           /* .                                 */
   ast_null,                           /* (                                 */
   ast_nelt,                           /* #                                 */
   ast_pow,                            /* POW                               */
   ast_arb,                            /* ARB                               */
   ast_domain,                         /* DOMAIN                            */
   ast_range,                          /* RANGE                             */
   ast_add,                            /* +                                 */
   ast_add,                            /* +:=                               */
   ast_add,                            /* +/                                */
   ast_sub,                            /* -:=                               */
   ast_sub,                            /* -/                                */
   ast_question,                       /* ?                                 */
   ast_question,                       /* ?:=                               */
   ast_question,                       /* ?/                                */
   ast_mult,                           /* *                                 */
   ast_mult,                           /* *:=                               */
   ast_mult,                           /* * /                               */
   ast_div,                            /* /                                 */
   ast_div,                            /* /:=                               */
   ast_div,                            /* //                                */
   ast_mod,                            /* MOD                               */
   ast_mod,                            /* MOD:=                             */
   ast_mod,                            /* MOD/                              */
   ast_min,                            /* MIN                               */
   ast_min,                            /* MIN:=                             */
   ast_min,                            /* MIN/                              */
   ast_max,                            /* MAX                               */
   ast_max,                            /* MAX:=                             */
   ast_max,                            /* MAX/                              */
   ast_with,                           /* WITH                              */
   ast_with,                           /* WITH:=                            */
   ast_with,                           /* WITH/                             */
   ast_less,                           /* LESS                              */
   ast_less,                           /* LESS:=                            */
   ast_less,                           /* LESS/                             */
   ast_lessf,                          /* LESSF                             */
   ast_lessf,                          /* LESSF:=                           */
   ast_lessf,                          /* LESSF/                            */
   ast_npow,                           /* NPOW                              */
   ast_npow,                           /* NPOW:=                            */
   ast_npow,                           /* NPOW/                             */
   ast_eq,                             /* =                                 */
   ast_eq,                             /* =:=                               */
   ast_eq,                             /* =/                                */
   ast_ne,                             /* /=                                */
   ast_ne,                             /* /=:=                              */
   ast_ne,                             /* /=/                               */
   ast_lt,                             /* <                                 */
   ast_lt,                             /* <:=                               */
   ast_lt,                             /* </                                */
   ast_le,                             /* <=                                */
   ast_le,                             /* <=:=                              */
   ast_le,                             /* <=/                               */
   ast_gt,                             /* >                                 */
   ast_gt,                             /* >:=                               */
   ast_gt,                             /* >/                                */
   ast_ge,                             /* >=                                */
   ast_ge,                             /* >=:=                              */
   ast_ge,                             /* >=/                               */
   ast_in,                             /* IN                                */
   ast_in,                             /* IN:=                              */
   ast_in,                             /* IN/                               */
   ast_notin,                          /* NOTIN                             */
   ast_notin,                          /* NOTIN:=                           */
   ast_notin,                          /* NOTIN/                            */
   ast_subset,                         /* SUBSET                            */
   ast_subset,                         /* SUBSET:=                          */
   ast_subset,                         /* SUBSET/                           */
   ast_incs,                           /* INCS                              */
   ast_incs,                           /* INCS:=                            */
   ast_incs,                           /* INCS/                             */
   ast_and,                            /* AND:=                             */
   ast_and,                            /* AND/                              */
   ast_or,                             /* OR:=                              */
   ast_or,                             /* OR/                               */
   ast_from,                           /* FROM                              */
   ast_fromb,                          /* FROMB                             */
   ast_frome,                          /* FROME                             */
   ast_exists,                         /* EXISTS                            */
   ast_forall,                         /* FORALL                            */
/* ## end default_ast */
   -1};

#else

extern char tok_ast_type[];            /* ast node type for each token      */

#endif

/*\
 *  \table{default method names}
 *
 *  We keep a table of default method names for the semantic actions.  In
 *  many places we merge many token subclasses into a single class, and
 *  use this table to determine the type of method to create.
\*/

#ifdef SHARED

int tok_mcode[] = {                    /* method code type                  */
/* ## begin default_meth */
   -1,                                 /* end of file                       */
   -1,                                 /* error token                       */
   -1,                                 /* identifier                        */
   -1,                                 /* literal                           */
   -1,                                 /* keyword => AND                    */
   -1,                                 /* keyword => ASSERT                 */
   -1,                                 /* keyword => BODY                   */
   -1,                                 /* keyword => CASE                   */
   -1,                                 /* keyword => CLASS                  */
   -1,                                 /* keyword => CONST                  */
   -1,                                 /* keyword => CONTINUE               */
   -1,                                 /* keyword => ELSE                   */
   -1,                                 /* keyword => ELSEIF                 */
   -1,                                 /* keyword => END                    */
   -1,                                 /* keyword => EXIT                   */
   -1,                                 /* keyword => FOR                    */
   -1,                                 /* keyword => IF                     */
   -1,                                 /* keyword => INHERIT                */
   -1,                                 /* keyword => LAMBDA                 */
   -1,                                 /* keyword => LOOP                   */
   -1,                                 /* keyword => NOT                    */
   -1,                                 /* keyword => NULL                   */
   -1,                                 /* keyword => OR                     */
   -1,                                 /* keyword => OTHERWISE              */
   -1,                                 /* keyword => PACKAGE                */
   -1,                                 /* keyword => PROCEDURE              */
   -1,                                 /* keyword => PROCESS                */
   -1,                                 /* keyword => PROGRAM                */
   -1,                                 /* keyword => RD                     */
   -1,                                 /* keyword => RETURN                 */
   -1,                                 /* keyword => RW                     */
   -1,                                 /* keyword => SEL                    */
   -1,                                 /* keyword => SELF                   */
   -1,                                 /* keyword => STOP                   */
   -1,                                 /* keyword => THEN                   */
   -1,                                 /* keyword => UNTIL                  */
   -1,                                 /* keyword => USE                    */
   -1,                                 /* keyword => VAR                    */
   -1,                                 /* keyword => WHEN                   */
   -1,                                 /* keyword => WHILE                  */
   -1,                                 /* keyword => WR                     */
   -1,                                 /* ;                                 */
   -1,                                 /* ,                                 */
   -1,                                 /* :                                 */
   -1,                                 /* (                                 */
   -1,                                 /* )                                 */
   -1,                                 /* [                                 */
   -1,                                 /* ]                                 */
   -1,                                 /* {                                 */
   -1,                                 /* }                                 */
   -1,                                 /* .                                 */
   -1,                                 /* ..                                */
   -1,                                 /* :=                                */
   -1,                                 /* |                                 */
   -1,                                 /* =>                                */
   -1,                                 /* assignment operator               */
   -1,                                 /* assignment operator               */
   -1,                                 /* unary operator                    */
   -1,                                 /* pointer reference                 */
   -1,                                 /* addop                             */
   -1,                                 /* -                                 */
   -1,                                 /* mulop                             */
   m_exp,                              /* **                                */
   -1,                                 /* relop                             */
   -1,                                 /* fromop                            */
   -1,                                 /* quantifier                        */
   -1,                                 /* keyword => NATIVE                 */
   -1,                                 /* ;                                 */
   -1,                                 /* .                                 */
   -1,                                 /* (                                 */
   m_nelt,                             /* #                                 */
   m_pow,                              /* POW                               */
   m_arb,                              /* ARB                               */
   m_domain,                           /* DOMAIN                            */
   m_range,                            /* RANGE                             */
   m_add,                              /* +                                 */
   -1,                                 /* +:=                               */
   -1,                                 /* +/                                */
   -1,                                 /* -:=                               */
   -1,                                 /* -/                                */
   -1,                                 /* ?                                 */
   -1,                                 /* ?:=                               */
   -1,                                 /* ?/                                */
   m_mult,                             /* *                                 */
   -1,                                 /* *:=                               */
   -1,                                 /* * /                               */
   m_div,                              /* /                                 */
   -1,                                 /* /:=                               */
   -1,                                 /* //                                */
   m_mod,                              /* MOD                               */
   -1,                                 /* MOD:=                             */
   -1,                                 /* MOD/                              */
   m_min,                              /* MIN                               */
   -1,                                 /* MIN:=                             */
   -1,                                 /* MIN/                              */
   m_max,                              /* MAX                               */
   -1,                                 /* MAX:=                             */
   -1,                                 /* MAX/                              */
   m_with,                             /* WITH                              */
   -1,                                 /* WITH:=                            */
   -1,                                 /* WITH/                             */
   m_less,                             /* LESS                              */
   -1,                                 /* LESS:=                            */
   -1,                                 /* LESS/                             */
   m_lessf,                            /* LESSF                             */
   -1,                                 /* LESSF:=                           */
   -1,                                 /* LESSF/                            */
   m_npow,                             /* NPOW                              */
   -1,                                 /* NPOW:=                            */
   -1,                                 /* NPOW/                             */
   -1,                                 /* =                                 */
   -1,                                 /* =:=                               */
   -1,                                 /* =/                                */
   -1,                                 /* /=                                */
   -1,                                 /* /=:=                              */
   -1,                                 /* /=/                               */
   m_lt,                               /* <                                 */
   -1,                                 /* <:=                               */
   -1,                                 /* </                                */
   -1,                                 /* <=                                */
   -1,                                 /* <=:=                              */
   -1,                                 /* <=/                               */
   -1,                                 /* >                                 */
   -1,                                 /* >:=                               */
   -1,                                 /* >/                                */
   -1,                                 /* >=                                */
   -1,                                 /* >=:=                              */
   -1,                                 /* >=/                               */
   m_in,                               /* IN                                */
   -1,                                 /* IN:=                              */
   -1,                                 /* IN/                               */
   -1,                                 /* NOTIN                             */
   -1,                                 /* NOTIN:=                           */
   -1,                                 /* NOTIN/                            */
   -1,                                 /* SUBSET                            */
   -1,                                 /* SUBSET:=                          */
   -1,                                 /* SUBSET/                           */
   -1,                                 /* INCS                              */
   -1,                                 /* INCS:=                            */
   -1,                                 /* INCS/                             */
   -1,                                 /* AND:=                             */
   -1,                                 /* AND/                              */
   -1,                                 /* OR:=                              */
   -1,                                 /* OR/                               */
   m_from,                             /* FROM                              */
   m_fromb,                            /* FROMB                             */
   m_frome,                            /* FROME                             */
   -1,                                 /* EXISTS                            */
   -1,                                 /* FORALL                            */
/* ## end default_meth */
   -1};

#else

extern int tok_mcode[];                /* method code type                  */

#endif

/*\
 *  \nobar\newpage
\*/

/* function declarations */

void init_lex(SETL_SYSTEM_PROTO_VOID); /* initialize lexical analyzer       */
void get_token(SETL_SYSTEM_PROTO token_type *);          
                                       /* return next token                 */
void close_lex(SETL_SYSTEM_PROTO_VOID);/* close the source file             */

#define LEX_LOADED 1

#endif
