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
 *  \package{The Unit Table}
 *
 *  The unit table will be useful when this is implemented as a macro
 *  language.  We would eventually like to have the ability to load and
 *  unload units dynamically.  At present, we simply load all imported
 *  packages before loading the main program and execute the main
 *  program.
 *
 *  \texify{unittab.h}
 *
 *  \packagebody{Unit Table}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "mcode.h"                     /* method codes                      */
#include "pcode.h"                     /* pseudo code                       */
#include "builtins.h"                  /* built-in symbols                  */
#include "unittab.h"                   /* unit table                        */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "procs.h"                     /* procedures                        */

/* performance tuning constants */

#define UNITTAB_BLOCK_SIZE    30       /* unit table block size             */
#define STRING_BLOCK_SIZE    512       /* string table block size           */

/* generic table item structure (unit table use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct unittab_item ti_data;     /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[UNITTAB_BLOCK_SIZE];
                                       /* array of table items              */
};

/* string table block structure */

struct string_block {
   struct string_block *sb_next;       /* next block in list                */
   char sb_data[STRING_BLOCK_SIZE];    /* string table data                 */
};

/* package-global data */
#ifdef TSAFE

#define UNITTAB__TABLE_BLOCK_HEAD plugin_instance->unittab__table_block_head
#define UNITTAB__TABLE_NEXT_FREE plugin_instance->unittab__table_next_free
#define UNITTAB__HASH_TABLE plugin_instance->unittab__hash_table
#define UNITTAB__STRING_BLOCK_HEAD plugin_instance->unittab__string_block_head
#define UNITTAB__STRING_BLOCK_EOS plugin_instance->unittab__string_block_eos
#define UNITTAB__STRING_NEXT_FREE plugin_instance->unittab__string_next_free

#else

#define UNITTAB__TABLE_BLOCK_HEAD table_block_head
#define UNITTAB__TABLE_NEXT_FREE table_next_free
#define UNITTAB__HASH_TABLE hash_table
#define UNITTAB__STRING_BLOCK_HEAD string_block_head
#define UNITTAB__STRING_BLOCK_EOS string_block_eos
#define UNITTAB__STRING_NEXT_FREE string_next_free

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */
static unittab_ptr_type hash_table[UNITTAB__HASH_TABLE_SIZE];
                                       /* hash table                        */
static struct string_block *string_block_head = NULL;
                                       /* head of list of string blocks     */
static char *string_block_eos = NULL;  /* end of allocated block            */
static char *string_next_free = NULL;  /* next free character in block      */

#endif

/* forward declarations */

static char *get_strtab(SETL_SYSTEM_PROTO char *); 
                                       /* allocate string table             */
static unsigned hashpjw(char *);       /* string hash function              */

/*\
 *  \function{init\_unittab()}
 *
 *  This function initializes the unit table.  We clear out the table and
 *  install the built-in symbols.
\*/

void init_unittab(
  SETL_SYSTEM_PROTO_VOID)

{
struct table_block *tb_ptr;            /* work unit table pointer           */
struct string_block *sb_ptr;           /* work string table pointer         */
unittab_ptr_type unittab_ptr;          /* work unit table pointer           */
proc_ptr_type proc_ptr;                /* procedure pointer                 */
int i;                                 /* temporary looping variable        */

   /* clear whatever might be in the unit and string tables */

   while (UNITTAB__TABLE_BLOCK_HEAD != NULL) {

      tb_ptr = UNITTAB__TABLE_BLOCK_HEAD;
      UNITTAB__TABLE_BLOCK_HEAD = UNITTAB__TABLE_BLOCK_HEAD->tb_next;
      free((void *)tb_ptr);

   }

   UNITTAB__TABLE_NEXT_FREE = NULL;

   /* clear the string table */

   while (UNITTAB__STRING_BLOCK_HEAD != NULL) {

      sb_ptr = UNITTAB__STRING_BLOCK_HEAD;
      UNITTAB__STRING_BLOCK_HEAD = UNITTAB__STRING_BLOCK_HEAD->sb_next;
      free((void *)sb_ptr);

   }

   /* clear the hash table */

   for (i = 0; i < UNITTAB__HASH_TABLE_SIZE; i++)
      UNITTAB__HASH_TABLE[i] = NULL;


   /*
    *  we allocate an empty string block here, to avoid checking for a
    *  null string block with every string allocation
    */

   UNITTAB__STRING_BLOCK_HEAD = (struct string_block *)
                       malloc(sizeof(struct string_block));
   if (UNITTAB__STRING_BLOCK_HEAD == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   UNITTAB__STRING_NEXT_FREE = UNITTAB__STRING_BLOCK_HEAD->sb_data;
   UNITTAB__STRING_BLOCK_EOS = UNITTAB__STRING_BLOCK_HEAD->sb_data + STRING_BLOCK_SIZE;
   UNITTAB__STRING_BLOCK_HEAD->sb_next = NULL;

   /* open up a unit for built-in symbols */

   unittab_ptr = get_unittab(SETL_SYSTEM "$predefined");

   /* install the built-in symbols */

   for (i = 1; i_built_in_tab[i].bi_form != -1; i++);
   unittab_ptr->ut_data_ptr = get_specifiers(SETL_SYSTEM (int32)i);

   for (i = 0; i_built_in_tab[i].bi_form != -1; i++) {

      if (i_built_in_tab[i].bi_spec_ptr != NULL)
         *(i_built_in_tab[i].bi_spec_ptr) = unittab_ptr->ut_data_ptr + i;

      switch (i_built_in_tab[i].bi_form) {

         case ft_omega :

            (unittab_ptr->ut_data_ptr + i)->sp_form = ft_omega;

            break;

         case ft_atom :

            (unittab_ptr->ut_data_ptr + i)->sp_form = ft_atom;
            setl2_newat(SETL_SYSTEM 0,NULL,unittab_ptr->ut_data_ptr + i);

            break;

         case ft_short :

            (unittab_ptr->ut_data_ptr + i)->sp_form = ft_short;
            (unittab_ptr->ut_data_ptr + i)->sp_val.sp_short_value =
                  i_built_in_tab[i].bi_int_value;

            break;

         case ft_proc :

            get_proc(proc_ptr);
            proc_ptr->p_type = BUILTIN_PROC;
            proc_ptr->p_func_ptr = i_built_in_tab[i].bi_func_ptr;
            proc_ptr->p_formal_count = i_built_in_tab[i].bi_formal_count;
            proc_ptr->p_var_args = i_built_in_tab[i].bi_var_args;
            proc_ptr->p_self_ptr = NULL;
            proc_ptr->p_use_count = 1;
            (unittab_ptr->ut_data_ptr + i)->sp_form = ft_proc;
            (unittab_ptr->ut_data_ptr + i)->sp_val.sp_proc_ptr = proc_ptr;

            break;

      }
   }

   /* initialize the empty set and tuple */

   spec_nullset->sp_form = ft_set;
   get_set_header(spec_nullset->sp_val.sp_set_ptr);
   (spec_nullset->sp_val.sp_set_ptr)->s_use_count = 1;
   (spec_nullset->sp_val.sp_set_ptr)->s_hash_code = 0;
   (spec_nullset->sp_val.sp_set_ptr)->s_ntype.s_root.s_cardinality = 0;
   (spec_nullset->sp_val.sp_set_ptr)->s_ntype.s_root.s_height = 0;
   for (i = 0;
        i < SET_HASH_SIZE;
        (spec_nullset->sp_val.sp_set_ptr)->s_child[i++].s_cell = NULL);

   spec_nulltup->sp_form = ft_tuple;
   get_tuple_header(spec_nulltup->sp_val.sp_tuple_ptr);
   (spec_nulltup->sp_val.sp_tuple_ptr)->t_use_count = 1;
   (spec_nulltup->sp_val.sp_tuple_ptr)->t_hash_code = 0;
   (spec_nulltup->sp_val.sp_tuple_ptr)->t_ntype.t_root.t_length = 0;
   (spec_nulltup->sp_val.sp_tuple_ptr)->t_ntype.t_root.t_height = 0;
   for (i = 0;
        i < TUP_HEADER_SIZE;
        (spec_nulltup->sp_val.sp_tuple_ptr)->t_child[i++].t_cell = NULL);

   /* initialize _memory */

   spec_memory->sp_form = ft_map;
   get_map_header(spec_memory->sp_val.sp_map_ptr);
   (spec_memory->sp_val.sp_map_ptr)->m_use_count = 1;
   (spec_memory->sp_val.sp_map_ptr)->m_hash_code = 0;
   (spec_memory->sp_val.sp_map_ptr)->m_ntype.m_root.m_cardinality = 0;
   (spec_memory->sp_val.sp_map_ptr)->m_ntype.m_root.m_cell_count = 0;
   (spec_memory->sp_val.sp_map_ptr)->m_ntype.m_root.m_height = 0;
   for (i = 0;
        i < SET_HASH_SIZE;
        (spec_memory->sp_val.sp_map_ptr)->m_child[i++].m_cell = NULL);

   return;

}

/*\
 *  \function{get\_unittab()}
 *
 *  This function returns a unit table entry for a symbol.  First we try
 *  to find the symbol in the current unit table.  If a match is found,
 *  we just return a pointer to that item.  Otherwise we allocate a new
 *  item and enter it in the string, unit, and hash tables.
\*/

unittab_ptr_type get_unittab(
   SETL_SYSTEM_PROTO
   char *string)                       /* string to be found / installed    */

{
int string_hash;                       /* string's hash code                */
unittab_ptr_type return_ptr;           /* return value                      */
struct table_block *old_head;          /* unit table block list head        */

   /* first, look up the string in the current unit table */

   string_hash = hashpjw(string);
   for (return_ptr = UNITTAB__HASH_TABLE[string_hash];
        return_ptr != NULL && strcmp(return_ptr->ut_name,string) != 0;
        return_ptr = return_ptr->ut_hash_link);
   if (return_ptr != NULL)
      return return_ptr;

   /*
    *  We didn't find the string in the unit table, so we'll have to
    *  install it.  If there are no free items left allocate a new block.
    */

   if (UNITTAB__TABLE_NEXT_FREE == NULL) {

      /* allocate a new block */

      old_head = UNITTAB__TABLE_BLOCK_HEAD;
      UNITTAB__TABLE_BLOCK_HEAD = (struct table_block *)
                         malloc(sizeof(struct table_block));
      if (UNITTAB__TABLE_BLOCK_HEAD == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      UNITTAB__TABLE_BLOCK_HEAD->tb_next = old_head;

      /* link items on the free list */

      for (UNITTAB__TABLE_NEXT_FREE = UNITTAB__TABLE_BLOCK_HEAD->tb_data;
           UNITTAB__TABLE_NEXT_FREE <=
              UNITTAB__TABLE_BLOCK_HEAD->tb_data + UNITTAB_BLOCK_SIZE - 2;
           UNITTAB__TABLE_NEXT_FREE++) {
         UNITTAB__TABLE_NEXT_FREE->ti_union.ti_next = UNITTAB__TABLE_NEXT_FREE + 1;
      }

      UNITTAB__TABLE_NEXT_FREE->ti_union.ti_next = NULL;
      UNITTAB__TABLE_NEXT_FREE = UNITTAB__TABLE_BLOCK_HEAD->tb_data;
   }

   /* at this point, we know the free list is not empty */

   return_ptr = &(UNITTAB__TABLE_NEXT_FREE->ti_union.ti_data);
   UNITTAB__TABLE_NEXT_FREE = UNITTAB__TABLE_NEXT_FREE->ti_union.ti_next;

   /* initialize the new entry */

   clear_unittab(return_ptr);
   return_ptr->ut_hash_link = UNITTAB__HASH_TABLE[string_hash];
   UNITTAB__HASH_TABLE[string_hash] = return_ptr;
   return_ptr->ut_name = get_strtab(SETL_SYSTEM string);

   return return_ptr;

}

/*\
 *  \function{get\_strtab()}
 *
 *  This function installs a string in the string table, and returns a
 *  pointer to that string.
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

   if (UNITTAB__STRING_NEXT_FREE + string_size + 1 > UNITTAB__STRING_BLOCK_EOS) {

      old_head = UNITTAB__STRING_BLOCK_HEAD;
      UNITTAB__STRING_BLOCK_HEAD = (struct string_block *)
                          malloc(sizeof(struct string_block));
      if  (UNITTAB__STRING_BLOCK_HEAD == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      UNITTAB__STRING_BLOCK_HEAD->sb_next = old_head;

      UNITTAB__STRING_NEXT_FREE = UNITTAB__STRING_BLOCK_HEAD->sb_data;
      UNITTAB__STRING_BLOCK_EOS = UNITTAB__STRING_BLOCK_HEAD->sb_data + STRING_BLOCK_SIZE - 1;

   }

   /* we know we have enough space, just install the string and return */

   return_ptr = UNITTAB__STRING_NEXT_FREE;
   strcpy(UNITTAB__STRING_NEXT_FREE,string);
   UNITTAB__STRING_NEXT_FREE += string_size + 1;

   return return_ptr;

}

/*\
 *  \function{hashpjw()}
 *
 *  This function is an implementation of a hash code function due to
 *  P.~J.~Weinberger taken from \cite{dragon}.  According to that text,
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

   return hash_code % UNITTAB__HASH_TABLE_SIZE;

}
