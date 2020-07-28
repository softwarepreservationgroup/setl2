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
 *  \package{The Name Table}
 *
 *  This name table is fairly straightforward, perhaps even a little
 *  simpler than typical name tables.  We only delete names from the name
 *  table between source files, as part of the initialization function,
 *  so there is no concept of scopes here.  Once a name is added it
 *  stays in the name table until we finish the source file in which it
 *  is found.
 *
 *  The character forms of names are stored in a separate string table,
 *  to make good use of space, as is a common practice.  We allocate the
 *  string table in fairly large blocks then do our own memory
 *  allocation within those blocks.  This combination gives us reasonably
 *  efficient space utilization without very many calls to
 *  \verb"malloc()", which may be expensive.
 *
 *  The name table proper is allocated in a slightly simpler but similar
 *  manner.  We allocate large blocks of name table items, then link them
 *  together on a free list and allocate them from that list.  When the
 *  list is empty we call \verb"malloc()" to get another one.
 *
 *  We use a hash table in an open hashing scheme to look up names
 *  quickly.  The hash table is somewhat large because we would like to
 *  have fast access even though we expect the name table to be large.
 *
 *  >texify namtab.h
 *
 *  \packagebody{Name Table}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "namtab.h"                    /* name table                        */
#include "lex.h"                       /* lexical analyzer                  */


/* performance tuning constants */

#define NAMTAB_BLOCK_SIZE    200       /* name table block size             */
#define HASH_TABLE_SIZE      151       /* size of hash table                */
#define STRING_BLOCK_SIZE   4096       /* string table block size           */

/* generic table item structure (name table use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct namtab_item ti_data;      /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[NAMTAB_BLOCK_SIZE];
                                       /* array of table items              */
};

/* string table block structure */

struct string_block {
   struct string_block *sb_next;       /* next block in list                */
   char sb_data[STRING_BLOCK_SIZE];    /* string table data                 */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */
static namtab_ptr_type hash_table[HASH_TABLE_SIZE];
                                       /* hash table                        */
static struct string_block *string_block_head = NULL;
                                       /* head of list of string blocks     */
static char *string_block_eos = NULL;  /* end of allocated block            */
static char *string_next_free = NULL;  /* next free character in block      */

/* forward declarations */

static char *get_strtab(SETL_SYSTEM_PROTO char *);  
                                       /* allocate string table             */
static unsigned hashpjw(char *);       /* string hash function              */

/*\
 *  \function{init\_namtab()}
 *
 *  This function initializes the name table.  First we delete the
 *  current name and string tables.  In most cases they will already be
 *  empty.  We only initialize the name table before scanning a source
 *  file and we expect there to be only one source file per execution
 *  in most cases.
 *
 *  Then we install all the keywords, special characters, and predefined
 *  functions.  Special characters are only necessary if we have to print
 *  out a token which happens to be a special character.
\*/

void init_namtab(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *tb_ptr;            /* work name table pointer           */
struct string_block *sb_ptr;           /* work string table pointer         */
static namtab_ptr_type dummy;          /* dummy name table pointer          */
namtab_ptr_type new_name;              /* allocated name table pointer      */
int i;                                 /* temporary looping variable        */

/* initialization table */

struct initial_tab {
   namtab_ptr_type *rsv_nam_ptr;       /* pointer to be set                 */
   char *rsv_name;                     /* name lexeme                       */
   int rsv_token_class;                /* token class                       */
   int rsv_token_subclass;             /* token subclass                    */
};
static struct initial_tab keyword_tab[] = {
                                       /* keywords                          */
/* ## begin token_names */
   {&nam_eof,        "end of file",       tok_eof,         tok_eof          },
   {&nam_error,      "error",             tok_error,       tok_error        },
   {&nam_id,         "identifier",        tok_id,          tok_id           },
   {&nam_literal,    "literal",           tok_literal,     tok_literal      },
   {&dummy,          "AND",               tok_and,         tok_and          },
   {&dummy,          "ASSERT",            tok_assert,      tok_assert       },
   {&dummy,          "BODY",              tok_body,        tok_body         },
   {&dummy,          "CASE",              tok_case,        tok_case         },
   {&dummy,          "CLASS",             tok_class,       tok_class        },
   {&dummy,          "CONST",             tok_const,       tok_const        },
   {&dummy,          "CONTINUE",          tok_continue,    tok_continue     },
   {&dummy,          "ELSE",              tok_else,        tok_else         },
   {&dummy,          "ELSEIF",            tok_elseif,      tok_elseif       },
   {&dummy,          "END",               tok_end,         tok_end          },
   {&dummy,          "EXIT",              tok_exit,        tok_exit         },
   {&dummy,          "FOR",               tok_for,         tok_for          },
   {&dummy,          "IF",                tok_if,          tok_if           },
   {&nam_inherit,    "INHERIT",           tok_inherit,     tok_inherit      },
   {&nam_lambda,     "LAMBDA",            tok_lambda,      tok_lambda       },
   {&dummy,          "LOOP",              tok_loop,        tok_loop         },
   {&dummy,          "NATIVE",            tok_native,      tok_native       },
   {&dummy,          "NOT",               tok_not,         tok_not          },
   {&dummy,          "NULL",              tok_null,        tok_null         },
   {&dummy,          "OR",                tok_or,          tok_or           },
   {&dummy,          "OTHERWISE",         tok_otherwise,   tok_otherwise    },
   {&dummy,          "PACKAGE",           tok_package,     tok_package      },
   {&dummy,          "PROCEDURE",         tok_procedure,   tok_procedure    },
   {&dummy,          "PROCESS",           tok_process,     tok_process      },
   {&dummy,          "PROGRAM",           tok_program,     tok_program      },
   {&dummy,          "RD",                tok_rd,          tok_rd           },
   {&dummy,          "RETURN",            tok_return,      tok_return       },
   {&dummy,          "RW",                tok_rw,          tok_rw           },
   {&dummy,          "SEL",               tok_sel,         tok_sel          },
   {&dummy,          "SELF",              tok_self,        tok_self         },
   {&dummy,          "STOP",              tok_stop,        tok_stop         },
   {&dummy,          "THEN",              tok_then,        tok_then         },
   {&dummy,          "UNTIL",             tok_until,       tok_until        },
   {&dummy,          "USE",               tok_use,         tok_use          },
   {&dummy,          "VAR",               tok_var,         tok_var          },
   {&dummy,          "WHEN",              tok_when,        tok_when         },
   {&dummy,          "WHILE",             tok_while,       tok_while        },
   {&dummy,          "WR",                tok_wr,          tok_wr           },
   {&nam_semi,       ";",                 tok_semi,        tok_semi         },
   {&nam_comma,      ",",                 tok_comma,       tok_comma        },
   {&nam_colon,      ":",                 tok_colon,       tok_colon        },
   {&nam_lparen,     "(",                 tok_lparen,      tok_lparen       },
   {&nam_rparen,     ")",                 tok_rparen,      tok_rparen       },
   {&nam_lbracket,   "[",                 tok_lbracket,    tok_lbracket     },
   {&nam_rbracket,   "]",                 tok_rbracket,    tok_rbracket     },
   {&nam_lbrace,     "{",                 tok_lbrace,      tok_lbrace       },
   {&nam_rbrace,     "}",                 tok_rbrace,      tok_rbrace       },
   {&nam_dot,        ".",                 tok_dot,         tok_dot          },
   {&nam_dotdot,     "..",                tok_dotdot,      tok_dotdot       },
   {&nam_assign,     ":=",                tok_assign,      tok_assign       },
   {&nam_suchthat,   "|",                 tok_suchthat,    tok_suchthat     },
   {&nam_rarrow,     "=>",                tok_rarrow,      tok_rarrow       },
   {&nam_caret,      "^",                 tok_caret,       tok_caret        },
   {&nam_dash,       "-",                 tok_dash,        tok_dash         },
   {&nam_expon,      "**",                tok_expon,       tok_expon        },
   {&nam_integer,    ";",                 tok_integer,     tok_integer      },
   {&nam_real,       ".",                 tok_real,        tok_real         },
   {&nam_string,     "(",                 tok_string,      tok_string       },
   {&nam_nelt,       "#",                 tok_unop,        tok_nelt         },
   {&dummy,          "POW",               tok_unop,        tok_pow          },
   {&dummy,          "ARB",               tok_unop,        tok_arb          },
   {&dummy,          "DOMAIN",            tok_unop,        tok_dom          },
   {&dummy,          "RANGE",             tok_unop,        tok_range        },
   {&nam_plus,       "+",                 tok_addop,       tok_plus         },
   {&dummy,          "+:=",               tok_assignop,    tok_asnplus      },
   {&dummy,          "+/",                tok_applyop,     tok_appplus      },
   {&dummy,          "-:=",               tok_assignop,    tok_asnsub       },
   {&dummy,          "-/",                tok_applyop,     tok_appsub       },
   {&nam_question,   "?",                 tok_mulop,       tok_question     },
   {&dummy,          "?:=",               tok_assignop,    tok_asnquestion  },
   {&dummy,          "?/",                tok_applyop,     tok_appquestion  },
   {&nam_mult,       "*",                 tok_mulop,       tok_mult         },
   {&dummy,          "*:=",               tok_assignop,    tok_asnmult      },
   {&dummy,          "*/",                tok_applyop,     tok_appmult      },
   {&nam_slash,      "/",                 tok_mulop,       tok_slash        },
   {&dummy,          "/:=",               tok_assignop,    tok_asnslash     },
   {&dummy,          "//",                tok_applyop,     tok_appslash     },
   {&dummy,          "MOD",               tok_mulop,       tok_mod          },
   {&dummy,          "MOD:=",             tok_assignop,    tok_asnmod       },
   {&dummy,          "MOD/",              tok_applyop,     tok_appmod       },
   {&dummy,          "MIN",               tok_mulop,       tok_min          },
   {&dummy,          "MIN:=",             tok_assignop,    tok_asnmin       },
   {&dummy,          "MIN/",              tok_applyop,     tok_appmin       },
   {&dummy,          "MAX",               tok_mulop,       tok_max          },
   {&dummy,          "MAX:=",             tok_assignop,    tok_asnmax       },
   {&dummy,          "MAX/",              tok_applyop,     tok_appmax       },
   {&dummy,          "WITH",              tok_mulop,       tok_with         },
   {&dummy,          "WITH:=",            tok_assignop,    tok_asnwith      },
   {&dummy,          "WITH/",             tok_applyop,     tok_appwith      },
   {&dummy,          "LESS",              tok_mulop,       tok_less         },
   {&dummy,          "LESS:=",            tok_assignop,    tok_asnless      },
   {&dummy,          "LESS/",             tok_applyop,     tok_appless      },
   {&dummy,          "LESSF",             tok_mulop,       tok_lessf        },
   {&dummy,          "LESSF:=",           tok_assignop,    tok_asnlessf     },
   {&dummy,          "LESSF/",            tok_applyop,     tok_applessf     },
   {&dummy,          "NPOW",              tok_mulop,       tok_npow         },
   {&dummy,          "NPOW:=",            tok_assignop,    tok_asnnpow      },
   {&dummy,          "NPOW/",             tok_applyop,     tok_appnpow      },
   {&nam_eq,         "=",                 tok_relop,       tok_eq           },
   {&dummy,          "=:=",               tok_assignop,    tok_asneq        },
   {&dummy,          "=/",                tok_applyop,     tok_appeq        },
   {&nam_ne,         "/=",                tok_relop,       tok_ne           },
   {&dummy,          "/=:=",              tok_assignop,    tok_asnne        },
   {&dummy,          "/=/",               tok_applyop,     tok_appne        },
   {&nam_lt,         "<",                 tok_relop,       tok_lt           },
   {&dummy,          "<:=",               tok_assignop,    tok_asnlt        },
   {&dummy,          "</",                tok_applyop,     tok_applt        },
   {&nam_le,         "<=",                tok_relop,       tok_le           },
   {&dummy,          "<=:=",              tok_assignop,    tok_asnle        },
   {&dummy,          "<=/",               tok_applyop,     tok_apple        },
   {&nam_gt,         ">",                 tok_relop,       tok_gt           },
   {&dummy,          ">:=",               tok_assignop,    tok_asngt        },
   {&dummy,          ">/",                tok_applyop,     tok_appgt        },
   {&nam_ge,         ">=",                tok_relop,       tok_ge           },
   {&dummy,          ">=:=",              tok_assignop,    tok_asnge        },
   {&dummy,          ">=/",               tok_applyop,     tok_appge        },
   {&dummy,          "IN",                tok_relop,       tok_in           },
   {&dummy,          "IN:=",              tok_assignop,    tok_asnin        },
   {&dummy,          "IN/",               tok_applyop,     tok_appin        },
   {&dummy,          "NOTIN",             tok_relop,       tok_notin        },
   {&dummy,          "NOTIN:=",           tok_assignop,    tok_asnnotin     },
   {&dummy,          "NOTIN/",            tok_applyop,     tok_appnotin     },
   {&dummy,          "SUBSET",            tok_relop,       tok_subset       },
   {&dummy,          "SUBSET:=",          tok_assignop,    tok_asnsubset    },
   {&dummy,          "SUBSET/",           tok_applyop,     tok_appsubset    },
   {&dummy,          "INCS",              tok_relop,       tok_incs         },
   {&dummy,          "INCS:=",            tok_assignop,    tok_asnincs      },
   {&dummy,          "INCS/",             tok_applyop,     tok_appincs      },
   {&dummy,          "AND:=",             tok_assignop,    tok_asnand       },
   {&dummy,          "AND/",              tok_applyop,     tok_appand       },
   {&dummy,          "OR:=",              tok_assignop,    tok_asnor        },
   {&dummy,          "OR/",               tok_applyop,     tok_appor        },
   {&dummy,          "FROM",              tok_fromop,      tok_from         },
   {&dummy,          "FROMB",             tok_fromop,      tok_fromb        },
   {&dummy,          "FROME",             tok_fromop,      tok_frome        },
   {&dummy,          "EXISTS",            tok_quantifier,  tok_exists       },
   {&dummy,          "FORALL",            tok_quantifier,  tok_forall       },
/* ## end token_names */
   {&dummy,          NULL,                -1,              -1               }
};
static struct initial_tab method_tab[] = {
                                       /* methods                           */
/* ## begin method_names */
   {method_name+0,   "InitObj",           tok_id,          tok_id           },
   {method_name+1,   "Add",               tok_id,          tok_id           },
   {method_name+2,   "Add Right",         tok_id,          tok_id           },
   {method_name+3,   "Subtract",          tok_id,          tok_id           },
   {method_name+4,   "Subtract Right",    tok_id,          tok_id           },
   {method_name+5,   "Multiply",          tok_id,          tok_id           },
   {method_name+6,   "Multiply Right",    tok_id,          tok_id           },
   {method_name+7,   "Divide",            tok_id,          tok_id           },
   {method_name+8,   "Divide Right",      tok_id,          tok_id           },
   {method_name+9,   "Exp",               tok_id,          tok_id           },
   {method_name+10,  "Exp Right",         tok_id,          tok_id           },
   {method_name+11,  "Mod",               tok_id,          tok_id           },
   {method_name+12,  "Mod Right",         tok_id,          tok_id           },
   {method_name+13,  "Min",               tok_id,          tok_id           },
   {method_name+14,  "Min Right",         tok_id,          tok_id           },
   {method_name+15,  "Max",               tok_id,          tok_id           },
   {method_name+16,  "Max Right",         tok_id,          tok_id           },
   {method_name+17,  "With",              tok_id,          tok_id           },
   {method_name+18,  "With Right",        tok_id,          tok_id           },
   {method_name+19,  "Less",              tok_id,          tok_id           },
   {method_name+20,  "Less Right",        tok_id,          tok_id           },
   {method_name+21,  "Lessf",             tok_id,          tok_id           },
   {method_name+22,  "Lessf Right",       tok_id,          tok_id           },
   {method_name+23,  "Npow",              tok_id,          tok_id           },
   {method_name+24,  "Npow Right",        tok_id,          tok_id           },
   {method_name+25,  "Uminus",            tok_id,          tok_id           },
   {method_name+26,  "Domain",            tok_id,          tok_id           },
   {method_name+27,  "Range",             tok_id,          tok_id           },
   {method_name+28,  "Pow",               tok_id,          tok_id           },
   {method_name+29,  "Arb",               tok_id,          tok_id           },
   {method_name+30,  "Nelt",              tok_id,          tok_id           },
   {method_name+31,  "From",              tok_id,          tok_id           },
   {method_name+32,  "Fromb",             tok_id,          tok_id           },
   {method_name+33,  "Frome",             tok_id,          tok_id           },
   {method_name+34,  "Of",                tok_id,          tok_id           },
   {method_name+35,  "Ofa",               tok_id,          tok_id           },
   {method_name+36,  "Slice",             tok_id,          tok_id           },
   {method_name+37,  "End",               tok_id,          tok_id           },
   {method_name+38,  "Sof",               tok_id,          tok_id           },
   {method_name+39,  "Sofa",              tok_id,          tok_id           },
   {method_name+40,  "Sslice",            tok_id,          tok_id           },
   {method_name+41,  "Send",              tok_id,          tok_id           },
   {method_name+42,  "Lt",                tok_id,          tok_id           },
   {method_name+43,  "Lt Right",          tok_id,          tok_id           },
   {method_name+44,  "In",                tok_id,          tok_id           },
   {method_name+45,  "In Right",          tok_id,          tok_id           },
   {method_name+46,  "CREATE",            tok_id,          tok_id           },
   {method_name+47,  "ITERATOR_START",    tok_id,          tok_id           },
   {method_name+48,  "ITERATOR_NEXT",     tok_id,          tok_id           },
   {method_name+49,  "SET_ITERATOR_START",                 tok_id,          tok_id           },
   {method_name+50,  "SET_ITERATOR_NEXT", tok_id,          tok_id           },
   {method_name+51,  "SELFSTR",           tok_id,          tok_id           },
   {method_name+52,  "User",              tok_id,          tok_id           },
/* ## end method_names */
   {&dummy,          NULL,                -1,              -1               }
};

   /* clear whatever might be in the name and string tables */

   while (table_block_head != NULL) {
      tb_ptr = table_block_head;
      table_block_head = table_block_head->tb_next;
      free((void *)tb_ptr);
   }
   table_next_free = NULL;

   /* clear the string table */

   while (string_block_head != NULL) {
      sb_ptr = string_block_head;
      string_block_head = string_block_head->sb_next;
      free((void *)sb_ptr);
   }

   /* clear the hash table */

   for (i = 0; i < HASH_TABLE_SIZE; i++)
      hash_table[i] = NULL;

   /*
    *  we allocate an empty string block here, to avoid checking for a
    *  null string block with every string allocation
    */

   string_block_head = (struct string_block *)
                       malloc(sizeof(struct string_block));
   if (string_block_head == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   string_next_free = string_block_head->sb_data;
   string_block_eos = string_block_head->sb_data + STRING_BLOCK_SIZE;
   string_block_head->sb_next = NULL;

   /* install the reserved words */

   for (i = 0; keyword_tab[i].rsv_name != NULL; i++) {

#ifndef PROCESSES
      
      if (strcmp(keyword_tab[i].rsv_name, "PROCESS") == 0)
         continue;

#endif

      new_name = get_namtab(SETL_SYSTEM keyword_tab[i].rsv_name);
      new_name->nt_token_class = keyword_tab[i].rsv_token_class;
      new_name->nt_token_subclass = keyword_tab[i].rsv_token_subclass;
      *(keyword_tab[i].rsv_nam_ptr) = new_name;

   }

   /* install the built-in methods */

   for (i = 0; method_tab[i].rsv_name != NULL; i++) {

      new_name = get_namtab(SETL_SYSTEM method_tab[i].rsv_name);
      new_name->nt_token_class = method_tab[i].rsv_token_class;
      new_name->nt_token_subclass = method_tab[i].rsv_token_subclass;
      new_name->nt_method_code = i;
      *(method_tab[i].rsv_nam_ptr) = new_name;

   }

   return;

}

/*\
 *  \function{get\_namtab()}
 *
 *  This function returns a name table entry for a symbol.  First we try
 *  to find the symbol in the current name table.  If a match is found,
 *  we just return a pointer to that item.  Otherwise we allocate a new
 *  item and enter it in the string, name, and hash tables.
\*/

namtab_ptr_type get_namtab(
   SETL_SYSTEM_PROTO
   char *string)                       /* string to be found / installed    */

{
int string_hash;                       /* string's hash code                */
namtab_ptr_type return_ptr;            /* return value                      */
struct table_block *old_head;          /* name table block list head        */

   /* first, look up the string in the current name table */

   string_hash = hashpjw(string);
   for (return_ptr = hash_table[string_hash];
        return_ptr != NULL && strcmp(return_ptr->nt_name,string) != 0;
        return_ptr = return_ptr->nt_hash_link);
   if (return_ptr != NULL)
      return return_ptr;

   /*
    *  We didn't find the string in the name table, so we'll have to
    *  install it.  If there are no free items left allocate a new block.
    */

   if (table_next_free == NULL) {

      /* allocate a new block */

      old_head = table_block_head;
      table_block_head = (struct table_block *)
                         malloc(sizeof(struct table_block));
      if (table_block_head == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      table_block_head->tb_next = old_head;

      /* link items on the free list */

      for (table_next_free = table_block_head->tb_data;
           table_next_free <=
              table_block_head->tb_data + NAMTAB_BLOCK_SIZE - 2;
           table_next_free++) {
         table_next_free->ti_union.ti_next = table_next_free + 1;
      }

      table_next_free->ti_union.ti_next = NULL;
      table_next_free = table_block_head->tb_data;
   }

   /* at this point, we know the free list is not empty */

   return_ptr = &(table_next_free->ti_union.ti_data);
   table_next_free = table_next_free->ti_union.ti_next;

   /* initialize the new entry */

   clear_namtab(return_ptr);
   return_ptr->nt_hash_link = hash_table[string_hash];
   hash_table[string_hash] = return_ptr;
   return_ptr->nt_name = get_strtab(SETL_SYSTEM string);

   return return_ptr;

}

/*\
 *
 *  \function{get\_strtab()}
 *
 *  This function installs a string in the string table, and returns a
 *  pointer to that string.
 *
\*/

static char *get_strtab(
   SETL_SYSTEM_PROTO
   char *string)                       /* string to be installed            */

{
int string_size;                       /* length of string                  */
char *return_ptr;                      /* return value                      */
struct string_block *old_head;         /* string block head before alloc    */

   string_size = strlen(string);

   /* allocate a new block, if necessary */

   if (string_next_free + string_size + 1 > string_block_eos) {

      old_head = string_block_head;
      string_block_head = (struct string_block *)
                          malloc(sizeof(struct string_block));
      if (string_block_head == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      string_block_head->sb_next = old_head;

      string_next_free = string_block_head->sb_data;
      string_block_eos = string_block_head->sb_data + STRING_BLOCK_SIZE - 1;

   }

   /* we know we have enough space, just install the string and return */

   return_ptr = string_next_free;
   strcpy(string_next_free,string);
   string_next_free += string_size + 1;

   return return_ptr;

}

/*\
 *  \function{hashpjw()}
 *
 *  This function is an implementation of a hash code function due to
 *  P.~J.~Weinberger taken from \cite{dragon}.  According to that text
 *  this hash function performs very well for a wide variety of strings.
 *
 *  We have not copied the code exactly, although we compute the same
 *  function.  The function in the text assumes \verb"unsigned" will be
 *  32 bits long.  We have used \verb"#defines" to define constants
 *  which will be equivalent to theirs if we are on a 32 bit machine, but
 *  will be scaled up or down on other machines.  This should make the
 *  function run faster on 16 bit machines (we are counting on the C
 *  compiler to do constant folding, otherwise this might be slow).
\*/

static unsigned hashpjw(
   char *s)                            /* string to be hashed               */

{
unsigned hash_code;                    /* hash code eventually returned     */
unsigned top_four;                     /* top four bits of hash code        */
char *p;                               /* temporary looping variable        */
#define MASK (0x0f << (sizeof(unsigned) * 8 - 4))
                                       /* bit string with four high order   */
                                       /* bits of integer on, others off    */
#define SHIFT (sizeof(unsigned) * 8 - 8);
                                       /* shift distance                    */

   hash_code = 0;
   for (p = s; *p; p++) {

      hash_code = (hash_code << 4) + *p;
      if (top_four = hash_code & MASK) {
         hash_code ^= top_four >> SHIFT;
         hash_code ^= top_four;
      }
   }

   return hash_code % HASH_TABLE_SIZE;

}
