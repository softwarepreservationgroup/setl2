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
 *  \package{The Symbol Table}
 *
 *  The symbol table is a key data structure for any compiler, and this
 *  one is no exception.  The symbol table package itself is fairly
 *  straightforward.  We provide functions to allocate and deallocate
 *  symbol table items, attach and detach them from the name table, and
 *  to print the symbol table.
 *
 *  We allocate and deallocate symbol tables in blocks using
 *  \verb"malloc()", then do our own allocation and deallocation from
 *  those blocks.  This is an attempt to reduce expensive calls to
 *  \verb"malloc()"
 *
 *  One unusual feature of this symbol table is that we think of symbols
 *  as belonging to a given procedure.  It is important to examine the
 *  procedure table in conjunction with the symbol table.
 *
 *  \texify{symtab.h}
 *
 *  \packagebody{Symbol Table}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "namtab.h"                    /* name table                        */
#include "symtab.h"                    /* symbol table                      */
#include "proctab.h"                   /* procedure table                   */
#include "listing.h"                   /* source and error listings         */


#ifdef PLUGIN
#define fprintf plugin_fprintf
#define fputs   plugin_fputs
#endif


/* performance tuning constants */

#define SYMTAB_BLOCK_SIZE    200       /* symtab block size                 */

/* generic table item structure (symbol table use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct symtab_item ti_data;      /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[SYMTAB_BLOCK_SIZE];
                                       /* array of table items              */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */

/*\
 *  \function{init\_symtab()}
 *
 *  This procedure initializes the symbol table.  All we do is free all
 *  the allocated blocks.
\*/

void init_symtab()

{
struct table_block *tb_ptr;            /* work table pointer                */

   while (table_block_head != NULL) {
      tb_ptr = table_block_head;
      table_block_head = table_block_head->tb_next;
      free((void *)tb_ptr);
   }
   table_next_free = NULL;

}

/*\
 *  \function{get\_symtab()}
 *
 *  This procedure allocates a symbol table node. It is just like most of
 *  the other dynamic table allocation functions in the compiler.
\*/

symtab_ptr_type get_symtab(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *old_head;          /* name table block list head        */
symtab_ptr_type return_ptr;            /* return pointer                    */

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
              table_block_head->tb_data + SYMTAB_BLOCK_SIZE - 2;
           table_next_free++) {
         table_next_free->ti_union.ti_next = table_next_free + 1;
      }

      table_next_free->ti_union.ti_next = NULL;
      table_next_free = table_block_head->tb_data;
   }

   /* at this point, we know the free list is not empty */

   return_ptr = &(table_next_free->ti_union.ti_data);
   table_next_free = table_next_free->ti_union.ti_next;

   /* initialize the new entry and return */

   clear_symtab(return_ptr);

   return return_ptr;

}

/*\
 *  \function{clear\_symtab()}
 *
 *  This procedure clears a single symbol table item.  It's convenient to
 *  do that in a separate procedure, since then I probably won't forget
 *  to clear some field I add later.
\*/

void clear_symtab(
   symtab_ptr_type s)                  /* item to be cleared                */

{

   (s)->st_name_link = NULL;
   (s)->st_thread = NULL;
   (s)->st_namtab_ptr = NULL;
   (s)->st_owner_proc = NULL;
   (s)->st_class = NULL;
   (s)->st_unit_num = -1;
   (s)->st_offset = -1;
   (s)->st_slot_num = -1;
   (s)->st_file_pos.fp_line = 0;
   (s)->st_file_pos.fp_column = 0;
   (s)->st_type = 0;
   (s)->st_is_name_attached = 0;
   (s)->st_is_hidden = 0;
   (s)->st_has_lvalue = 0;
   (s)->st_has_rvalue = 0;
   (s)->st_is_rparam = 0;
   (s)->st_is_wparam = 0;
   (s)->st_is_temp = 0; 
   (s)->st_needs_stored = 0;
   (s)->st_is_alloced = 0;
   (s)->st_is_initialized = 0;
   (s)->st_aux.st_proctab_ptr = NULL;
   (s)->st_is_declared = 0;
   (s)->st_is_public = 0;
   (s)->st_in_spec = 0;
   (s)->st_is_visible_slot = 0;
   (s)->st_global_var = 0;

}

/*\
 *  \function{free\_symtab()}
 *
 *  This function is the complement to \verb"get_symtab()".  All we do is
 *  push the passed symbol table pointer on the free list.
\*/

void free_symtab(
   symtab_ptr_type discard)            /* item to be discarded              */

{

   ((struct table_item *)discard)->ti_union.ti_next = table_next_free;
   table_next_free = (struct table_item *)discard;

}

/*\
 *  \function{enter\_symbol()}
 *
 *  This function enters a symbol in the symbol table.  It attaches the
 *  symbol to the name table and inserts it in the appropriate procedure
 *  list as well.
\*/

symtab_ptr_type enter_symbol(
   SETL_SYSTEM_PROTO
   namtab_ptr_type namtab_ptr,         /* symbol name                       */
   struct proctab_item *proctab_ptr,   /* procedure which 'owns' symbol     */
   struct file_pos_item *file_pos)     /* file position of declaration      */

{
struct table_block *old_head;          /* name table block list head        */
symtab_ptr_type return_ptr;            /* return pointer                    */

   /* first we check for duplicate declarations */

   if (namtab_ptr != NULL) {

      return_ptr = namtab_ptr->nt_symtab_ptr;

      if (return_ptr != NULL && return_ptr->st_owner_proc == proctab_ptr) {

         error_message(SETL_SYSTEM
                       file_pos,
                       msg_dup_declaration,
                       namtab_ptr->nt_name);
         return NULL;

      }
   }

   /* allocate a new symbol table item */

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
              table_block_head->tb_data + SYMTAB_BLOCK_SIZE - 2;
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

   clear_symtab(return_ptr);

   /* if we're given a name, push it on the name's symbol list */

   if (namtab_ptr != NULL) {
      return_ptr->st_name_link = namtab_ptr->nt_symtab_ptr;
      namtab_ptr->nt_symtab_ptr = return_ptr;
      return_ptr->st_is_name_attached = YES;
   }

   if (file_pos != NULL) {
      copy_file_pos(&(return_ptr->st_file_pos),file_pos);
   }

   /* insert the symbol in the appropriate procedure */

   *(proctab_ptr->pr_symtab_tail) = return_ptr;
   proctab_ptr->pr_symtab_tail = &(return_ptr->st_thread);
   return_ptr->st_owner_proc = proctab_ptr;

   return_ptr->st_namtab_ptr = namtab_ptr;

   return return_ptr;

}

/*\
 *  \function{detach\_symtab()}
 *
 *  This function unlinks the symbols in a procedure from the name table.
 *  We use it when we close a scope.  The unlinking is a little tricky,
 *  because we use two levels of pointers.
\*/

void detach_symtab(
   symtab_ptr_type symtab_ptr)         /* head of symbol list               */

{
symtab_ptr_type *indirect_sym_ptr;     /* used to remove symbol from name   */
                                       /* table list                        */

   /* loop over the procedure's symbols */

   for (; symtab_ptr != NULL;
        symtab_ptr = symtab_ptr->st_thread) {

      if (!(symtab_ptr->st_is_name_attached))
         continue;

      /* unlink it from the name table */

#ifdef TRAPS

      for (indirect_sym_ptr =
              &((symtab_ptr->st_namtab_ptr)->nt_symtab_ptr);
           *indirect_sym_ptr != NULL && *indirect_sym_ptr != symtab_ptr;
           indirect_sym_ptr = &((*indirect_sym_ptr)->st_name_link));

      if (*indirect_sym_ptr == NULL)
         trap(__FILE__,__LINE__,msg_unattched_sym,
              (symtab_ptr->st_namtab_ptr)->nt_name);

#else

      for (indirect_sym_ptr =
              &((symtab_ptr->st_namtab_ptr)->nt_symtab_ptr);
           *indirect_sym_ptr != symtab_ptr;
           indirect_sym_ptr = &((*indirect_sym_ptr)->st_name_link));

#endif

      *indirect_sym_ptr = symtab_ptr->st_name_link;
      symtab_ptr->st_is_name_attached = NO;

   }
}

/*\
 *  \function{print\_symtab()}
 *
 *  This function prints the symbol table for a procedure.
\*/

#ifdef DEBUG

void print_symtab(
   SETL_SYSTEM_PROTO
   proctab_ptr_type proctab_ptr)       /* procedure to be printed           */

{
symtab_ptr_type symtab_ptr;            /* temporary looping variable        */
char print_symbol[16];                 /* truncated symbol for printing     */

   fputs("\nSymbol Table\n------------\n\n",DEBUG_FILE);

   for (symtab_ptr = proctab_ptr->pr_symtab_head;
        symtab_ptr != NULL;
        symtab_ptr = symtab_ptr->st_thread) {

      /* build up a junk symbol for temporaries and labels */

#if WATCOM

      if (symtab_ptr->st_namtab_ptr == NULL) {

         if (symtab_ptr->st_type == sym_label)
            sprintf(print_symbol,"$L%p ",symtab_ptr);
         else
            sprintf(print_symbol,"$T%p ",symtab_ptr);

      }

#else

      if (symtab_ptr->st_namtab_ptr == NULL) {

         if (symtab_ptr->st_type == sym_label)
            sprintf(print_symbol,"$L%ld ",(long)symtab_ptr);
         else
            sprintf(print_symbol,"$T%ld ",(long)symtab_ptr);

      }

#endif

      /* we don't allow names to exceed 15 characters */

      else if (symtab_ptr->st_namtab_ptr != NULL) {

         strncpy(print_symbol,(symtab_ptr->st_namtab_ptr)->nt_name,15);
         print_symbol[15] = '\0';

      }
      else {

         strcpy(print_symbol,"unknown");

      }

      fprintf(DEBUG_FILE,"%-15s %-10s  %2d %4ld %3d ",
              print_symbol,
              symtab_desc[symtab_ptr->st_type],
              symtab_ptr->st_unit_num,
              symtab_ptr->st_offset,
              symtab_ptr->st_slot_num);

      /* print some of the flags */

      if (symtab_ptr->st_is_hidden)
         fputs("hidden ",DEBUG_FILE);

      if (symtab_ptr->st_has_lvalue)
         fputs("lvalue ",DEBUG_FILE);

      if (symtab_ptr->st_has_rvalue)
         fputs("rvalue ",DEBUG_FILE);

      if (symtab_ptr->st_is_rparam)
         fputs("rparam ",DEBUG_FILE);

      if (symtab_ptr->st_is_wparam)
         fputs("wparam ",DEBUG_FILE);

      if (symtab_ptr->st_needs_stored)
         fputs("stored ",DEBUG_FILE);

      if (symtab_ptr->st_is_alloced)
         fputs("alloced ",DEBUG_FILE);

      if (symtab_ptr->st_is_public)
         fputs("public ",DEBUG_FILE);

      if (symtab_ptr->st_class != NULL)
         fprintf(DEBUG_FILE,"%s ",
                 ((symtab_ptr->st_class)->pr_namtab_ptr)->nt_name);

      if (symtab_ptr->st_global_var)
         fputs("global ",DEBUG_FILE);


      fputs("\n",DEBUG_FILE);

   }
}

#endif

