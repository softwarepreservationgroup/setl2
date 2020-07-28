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
 *  \packagespec{Name Table}
\*/

#ifndef NAMTAB_LOADED

/* SETL2 system header files */

#include "mcode.h"                     /* method codes                      */

/* name table item structure */

struct namtab_item {
   struct namtab_item *nt_hash_link;   /* next name with same hash value    */
   int nt_token_class;                 /* token class                       */
   int nt_token_subclass;              /* token subclass                    */
   int nt_method_code;                 /* built-in method code              */
   char *nt_name;                      /* name lexeme                       */
   struct symtab_item *nt_symtab_ptr;  /* list of symbols                   */
};

typedef struct namtab_item *namtab_ptr_type;
                                       /* name table type definition        */

/* clear a name table item */

#define clear_namtab(n) { \
   (n)->nt_hash_link = NULL;           (n)->nt_token_class = -1; \
   (n)->nt_token_subclass = -1;        (n)->nt_method_code = m_user; \
   (n)->nt_name = NULL;                (n)->nt_symtab_ptr = NULL; \
}

/* name table pointers for special characters */

#ifdef SHARED

/* ## begin shared_token_names */
namtab_ptr_type nam_eof;               /* end of file                       */
namtab_ptr_type nam_error;             /* error token                       */
namtab_ptr_type nam_id;                /* identifier                        */
namtab_ptr_type nam_literal;           /* literal                           */
namtab_ptr_type nam_inherit;           /* keyword => INHERIT                */
namtab_ptr_type nam_lambda;            /* keyword => LAMBDA                 */
namtab_ptr_type nam_semi;              /* ;                                 */
namtab_ptr_type nam_comma;             /* ,                                 */
namtab_ptr_type nam_colon;             /* :                                 */
namtab_ptr_type nam_lparen;            /* (                                 */
namtab_ptr_type nam_rparen;            /* )                                 */
namtab_ptr_type nam_lbracket;          /* [                                 */
namtab_ptr_type nam_rbracket;          /* ]                                 */
namtab_ptr_type nam_lbrace;            /* {                                 */
namtab_ptr_type nam_rbrace;            /* }                                 */
namtab_ptr_type nam_dot;               /* .                                 */
namtab_ptr_type nam_dotdot;            /* ..                                */
namtab_ptr_type nam_assign;            /* :=                                */
namtab_ptr_type nam_suchthat;          /* |                                 */
namtab_ptr_type nam_rarrow;            /* =>                                */
namtab_ptr_type nam_caret;             /* pointer reference                 */
namtab_ptr_type nam_dash;              /* -                                 */
namtab_ptr_type nam_expon;             /* **                                */
namtab_ptr_type nam_integer;           /* ;                                 */
namtab_ptr_type nam_real;              /* .                                 */
namtab_ptr_type nam_string;            /* (                                 */
namtab_ptr_type nam_nelt;              /* #                                 */
namtab_ptr_type nam_plus;              /* +                                 */
namtab_ptr_type nam_question;          /* ?                                 */
namtab_ptr_type nam_mult;              /* *                                 */
namtab_ptr_type nam_slash;             /* /                                 */
namtab_ptr_type nam_eq;                /* =                                 */
namtab_ptr_type nam_ne;                /* /=                                */
namtab_ptr_type nam_lt;                /* <                                 */
namtab_ptr_type nam_le;                /* <=                                */
namtab_ptr_type nam_gt;                /* >                                 */
namtab_ptr_type nam_ge;                /* >=                                */
/* ## end shared_token_names */
namtab_ptr_type method_name[m_user+2]; /* method names                      */

#else

/* ## begin extern_token_names */
extern namtab_ptr_type nam_eof;        /* end of file                       */
extern namtab_ptr_type nam_error;      /* error token                       */
extern namtab_ptr_type nam_id;         /* identifier                        */
extern namtab_ptr_type nam_literal;    /* literal                           */
extern namtab_ptr_type nam_inherit;    /* keyword => INHERIT                */
extern namtab_ptr_type nam_lambda;     /* keyword => LAMBDA                 */
extern namtab_ptr_type nam_semi;       /* ;                                 */
extern namtab_ptr_type nam_comma;      /* ,                                 */
extern namtab_ptr_type nam_colon;      /* :                                 */
extern namtab_ptr_type nam_lparen;     /* (                                 */
extern namtab_ptr_type nam_rparen;     /* )                                 */
extern namtab_ptr_type nam_lbracket;   /* [                                 */
extern namtab_ptr_type nam_rbracket;   /* ]                                 */
extern namtab_ptr_type nam_lbrace;     /* {                                 */
extern namtab_ptr_type nam_rbrace;     /* }                                 */
extern namtab_ptr_type nam_dot;        /* .                                 */
extern namtab_ptr_type nam_dotdot;     /* ..                                */
extern namtab_ptr_type nam_assign;     /* :=                                */
extern namtab_ptr_type nam_suchthat;   /* |                                 */
extern namtab_ptr_type nam_rarrow;     /* =>                                */
extern namtab_ptr_type nam_caret;      /* pointer reference                 */
extern namtab_ptr_type nam_dash;       /* -                                 */
extern namtab_ptr_type nam_expon;      /* **                                */
extern namtab_ptr_type nam_integer;    /* ;                                 */
extern namtab_ptr_type nam_real;       /* .                                 */
extern namtab_ptr_type nam_string;     /* (                                 */
extern namtab_ptr_type nam_nelt;       /* #                                 */
extern namtab_ptr_type nam_plus;       /* +                                 */
extern namtab_ptr_type nam_question;   /* ?                                 */
extern namtab_ptr_type nam_mult;       /* *                                 */
extern namtab_ptr_type nam_slash;      /* /                                 */
extern namtab_ptr_type nam_eq;         /* =                                 */
extern namtab_ptr_type nam_ne;         /* /=                                */
extern namtab_ptr_type nam_lt;         /* <                                 */
extern namtab_ptr_type nam_le;         /* <=                                */
extern namtab_ptr_type nam_gt;         /* >                                 */
extern namtab_ptr_type nam_ge;         /* >=                                */
/* ## end extern_token_names */
extern namtab_ptr_type method_name[];  /* method names                      */

#endif

/* public function declarations */

void init_namtab(SETL_SYSTEM_PROTO_VOID);   
                                       /* initialize name table             */
namtab_ptr_type get_namtab(SETL_SYSTEM_PROTO char *);  
                                       /* get name table item               */

#define NAMTAB_LOADED 1
#endif
