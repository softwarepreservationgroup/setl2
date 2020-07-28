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
 *  \package{The Procedure Table}
 *
 *  The name of this package is a little confusing.  It really holds more
 *  than just procedures --- it also includes package specifications,
 *  package bodies, and programs.  The common thread is that each
 *  introduces a new name scope. It is a little unusual, but quite
 *  central to this compiler.  Since SETL2 allows nearly unrestricted
 *  forward references, the cleanest way to handle these references is to
 *  make one pass over the program building the symbol table before we
 *  try to associate names with objects.  This tree facilitates that.  On
 *  the first pass, we build a symbol table and abstract syntax trees,
 *  but there is no correlation between the two.  On the second pass we
 *  match objects in the symbol table with the names in the AST.
 *
 *  \texify{proctab.h}
 *
 *  \packagebody{Procedure Table}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "builtins.h"                  /* built-in symbols                  */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "c_integers.h"                  /* integer literals                  */
#include "lex.h"                       /* lexical analyzer                  */


#ifdef PLUGIN
#define fprintf plugin_fprintf
#define fputs   plugin_fputs
#endif

/* performance tuning constants */

#define PROCTAB_BLOCK_SIZE   200       /* procedure table block size        */

/* generic table item structure (procedure table use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct proctab_item ti_data;     /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[PROCTAB_BLOCK_SIZE];
                                       /* array of table items              */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */

/* forward declarations */

#ifdef DEBUG
static void print_subtree(SETL_SYSTEM_PROTO int, proctab_ptr_type);
                                       /* subtree of procedure table        */
#endif

/*\
 *  \function{init\_proctab()}
 *
 *  This function initializes the procedure table.  We allocate a node
 *  for predefined symbols and load all the built-in symbols.
\*/

void init_proctab(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *tb_ptr;            /* work procedure table pointer      */
namtab_ptr_type namtab_ptr;            /* name pointer for built-in symbol  */
symtab_ptr_type symtab_ptr;            /* built-in symbol pointer           */
symtab_ptr_type formal_ptr;            /* formal parameter pointer          */
proctab_ptr_type proctab_ptr;          /* built-in procedure pointer        */
int i;                                 /* temporary looping variable        */
char *p;                               /* temporary looping variable        */

   /* clear whatever might be in the procedure table */

   while (table_block_head != NULL) {
      tb_ptr = table_block_head;
      table_block_head = table_block_head->tb_next;
      free((void *)tb_ptr);
   }
   table_next_free = NULL;

   /* create a procedure for built-in symbols */

   predef_proctab_ptr = get_proctab(SETL_SYSTEM_VOID);

   /* load the built-in symbols */

   for (i = 0; c_built_in_tab[i].bi_form != -1; i++) {

      /* enter the name and symbol */

      namtab_ptr = get_namtab(SETL_SYSTEM c_built_in_tab[i].bi_name);
      symtab_ptr = enter_symbol(SETL_SYSTEM
                                namtab_ptr,
                                predef_proctab_ptr,
                                NULL);
      symtab_ptr->st_unit_num = 0;
      symtab_ptr->st_offset = i;

      if (c_built_in_tab[i].bi_symtab_ptr)
         *(c_built_in_tab[i].bi_symtab_ptr) = symtab_ptr;

      switch (c_built_in_tab[i].bi_form) {

         case ft_omega :      case ft_atom :

            namtab_ptr->nt_token_class = tok_id;
            namtab_ptr->nt_token_subclass = tok_id;
            symtab_ptr->st_type = sym_id;
            symtab_ptr->st_has_rvalue = YES;
            symtab_ptr->st_is_initialized = YES;

            break;

         case ft_long :

            namtab_ptr->nt_token_class = tok_literal;
            namtab_ptr->nt_token_subclass = tok_integer;
            symtab_ptr->st_type = sym_integer;
            symtab_ptr->st_has_rvalue = YES;
            symtab_ptr->st_is_initialized = YES;
            symtab_ptr->st_aux.st_integer_ptr =
               char_to_int(SETL_SYSTEM c_built_in_tab[i].bi_name);

            break;

         case ft_proc :

            namtab_ptr->nt_token_class = tok_id;
            namtab_ptr->nt_token_subclass = tok_id;
            symtab_ptr->st_type = sym_procedure;
            symtab_ptr->st_has_rvalue = YES;
            symtab_ptr->st_is_initialized = YES;
            proctab_ptr = get_proctab(SETL_SYSTEM_VOID);
            proctab_ptr->pr_namtab_ptr = namtab_ptr;
            proctab_ptr->pr_formal_count = c_built_in_tab[i].bi_formal_count;
            proctab_ptr->pr_var_args = c_built_in_tab[i].bi_var_args;
            symtab_ptr->st_aux.st_proctab_ptr = proctab_ptr;

            /* create dummy formal parameters */

            for (p = c_built_in_tab[i].bi_arg_mode; *p; p++) {

               /* enter the symbol */

               formal_ptr = enter_symbol(SETL_SYSTEM
                                         NULL,
                                         proctab_ptr,
                                         NULL);
               formal_ptr->st_type = sym_id;

               if (*p == '1') {

                  formal_ptr->st_is_rparam = YES;

               }
               else if (*p == '2') {

                  formal_ptr->st_is_wparam = YES;
                  symtab_ptr->st_has_rvalue = NO;

               }
               else if (*p == '3') {

                  formal_ptr->st_is_rparam = YES;
                  formal_ptr->st_is_wparam = YES;
                  symtab_ptr->st_has_rvalue = NO;

               }
            }

            break;

      }
   }

   /* some fixup */

   sym_memory->st_has_lvalue = YES;
   sym_abendtrap->st_has_lvalue = YES;

}

/*\
 *  \function{get\_proctab()}
 *
 *  This procedure allocates a procedure table node. It is just like most
 *  of the other dynamic table allocation functions in the compiler.
\*/

proctab_ptr_type get_proctab(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *old_head;          /* name table block list head        */
proctab_ptr_type return_ptr;           /* return pointer                    */

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
              table_block_head->tb_data + PROCTAB_BLOCK_SIZE - 2;
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

   clear_proctab(return_ptr);

   return return_ptr;

}

/*\
 *  \function{clear\_proctab()}
 *
 *  This procedure clears a single procedure table item.  It's convenient
 *  to do that in a separate procedure, since then I probably won't
 *  forget to clear some field I add later.
\*/

void clear_proctab(
   proctab_ptr_type p)                 /* item to be initialized            */

{

   (p)->pr_type = -1;
   (p)->pr_child = NULL;
   (p)->pr_parent = NULL;
   (p)->pr_next = NULL;
   (p)->pr_tail = &((p)->pr_child);
   (p)->pr_namtab_ptr = NULL;
   (p)->pr_symtab_head = NULL;
   (p)->pr_symtab_tail = &((p)->pr_symtab_head);
   (p)->pr_symtab_count = 0;
   (p)->pr_init_count = 0;
   (p)->pr_sinit_count = 0; 
   (p)->pr_body_count = 0;
   (p)->pr_formal_count = 0;
   (p)->pr_init_offset = -1;
   (p)->pr_body_offset = -1; 
   (p)->pr_entry_offset = -1; 
   (p)->pr_spec_offset = -1; 
   (p)->pr_import_list = NULL;
   (p)->pr_inherit_list = NULL;
   (p)->pr_unit_count = 1;
   (p)->pr_file_pos.fp_line = -1;
   (p)->pr_file_pos.fp_line = -1;
   (p)->pr_var_args = 0;
   (p)->pr_label_count = 0;
   (p)->pr_method_code = -1;

}

/*\
 *  \function{free\_proctab()}
 *
 *  This function is the complement to \verb"get_proctab()". All we do is
 *  push the passed procedure table pointer on the free list.
\*/

void free_proctab(
   proctab_ptr_type discard)           /* item to be discarded              */

{

   ((struct table_item *)discard)->ti_union.ti_next = table_next_free;
   table_next_free = (struct table_item *)discard;

}

/*\
 *  \function{print\_proctab()}
 *
 *  The following two functions print the procedure table. It is simpler to
 *  use a recursive procedure for this, so the first is just an entry
 *  procedure which prints the heading and calls the recursive procedure
 *  which prints subtrees.  We only use this during debugging, and only
 *  if requested.
\*/

#ifdef DEBUG

void print_proctab(SETL_SYSTEM_PROTO_VOID)

{

   fputs("\nProcedure Table\n---------------\n",DEBUG_FILE);
   print_subtree(SETL_SYSTEM 0,predef_proctab_ptr->pr_child);

}

static void print_subtree(
   SETL_SYSTEM_PROTO
   int level,                          /* nesting level                     */
   proctab_ptr_type root)              /* subtree to be printed             */

{
int i;                                 /* temporary looping variable        */

   if (root == NULL)
      return;

   /* space over, to indicate nesting level */

   for (i = 0; i < level; i++)
      fputs("   ",DEBUG_FILE);

   fprintf(DEBUG_FILE,"%s : %s\n",
           (root->pr_namtab_ptr)->nt_name,
           proctab_desc[root->pr_type]);

   /* print children and siblings */

   print_subtree(SETL_SYSTEM level + 1,root->pr_child);
   print_subtree(SETL_SYSTEM level,root->pr_next);

}

#endif
