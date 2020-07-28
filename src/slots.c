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
 *  \package{The Slot Table}
 *
 *  \texify{slots.h}
 *
 *  \packagebody{Slot Table}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "mcode.h"                     /* method codes                      */
#include "slots.h"                     /* procedures                        */


#ifdef PLUGIN
#define fprintf plugin_fprintf
#define printf plugin_printf
#endif

/* performance tuning constants */

#define SLOT_BLOCK_SIZE       30       /* slot table block size             */
#define STRING_BLOCK_SIZE    512       /* string table block size           */

/* generic table item structure (slot table use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct slot_item ti_data;        /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[SLOT_BLOCK_SIZE];
                                       /* array of table items              */
};

/* string table block structure */

struct string_block {
   struct string_block *sb_next;       /* next block in list                */
   char sb_data[STRING_BLOCK_SIZE];    /* string table data                 */
};

/* package-global data */

#ifdef TSAFE 

#define TABLE_BLOCK_HEAD plugin_instance->table_block_head
#define TABLE_NEXT_FREE plugin_instance->table_next_free
#define HASH_TABLE plugin_instance->hash_table
#define STRING_BLOCK_HEAD plugin_instance->string_block_head
#define STRING_BLOCK_EOS plugin_instance->string_block_eos
#define STRING_NEXT_FREE plugin_instance->string_next_free

#else

#define TABLE_BLOCK_HEAD table_block_head
#define TABLE_NEXT_FREE table_next_free
#define HASH_TABLE hash_table
#define STRING_BLOCK_HEAD string_block_head
#define STRING_BLOCK_EOS string_block_eos
#define STRING_NEXT_FREE string_next_free

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */
static slot_ptr_type hash_table[SLOTS__HASH_TABLE_SIZE];
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
 *  \function{init\_slottab()}
 *
 *  This function initializes the slot table.  We clear out the table and
 *  install the built-in symbols.
\*/

void init_slots(
   SETL_SYSTEM_PROTO_VOID)

{
struct table_block *tb_ptr;            /* work slot table pointer           */
struct string_block *sb_ptr;           /* work string table pointer         */

int i;                                 /* temporary looping variable        */
static char *mcode_init[] = {          /* initialization table              */
/* ## begin mcode_names */
"InitObj",                             /* initialize instance               */
"Add",                                 /* +                                 */
"Add Right",                           /* + on right                        */
"Subtract",                            /* -                                 */
"Subtract Right",                      /* - on right                        */
"Multiply",                            /* *                                 */
"Multiply Right",                      /* * on right                        */
"Divide",                              /* /                                 */
"Divide Right",                        /* / on right                        */
"Exp",                                 /* **                                */
"Exp Right",                           /* ** on right                       */
"Mod",                                 /* mod                               */
"Mod Right",                           /* mod on right                      */
"Min",                                 /* min                               */
"Min Right",                           /* min on right                      */
"Max",                                 /* max                               */
"Max Right",                           /* max on right                      */
"With",                                /* with                              */
"With Right",                          /* with on right                     */
"Less",                                /* less                              */
"Less Right",                          /* less on right                     */
"Lessf",                               /* lessf                             */
"Lessf Right",                         /* lessf on right                    */
"Npow",                                /* npow                              */
"Npow Right",                          /* npow on right                     */
"Uminus",                              /* unary minus                       */
"Domain",                              /* domain                            */
"Range",                               /* range                             */
"Pow",                                 /* pow                               */
"Arb",                                 /* arb                               */
"Nelt",                                /* #                                 */
"From",                                /* from                              */
"Fromb",                               /* fromb                             */
"Frome",                               /* frome                             */
"Of",                                  /* map, tuple, or string             */
"Ofa",                                 /* multi-valued map                  */
"Slice",                               /* slice                             */
"End",                                 /* string end                        */
"Sof",                                 /* map, tuple, or string assign      */
"Sofa",                                /* mmap sinister assignment          */
"Sslice",                              /* slice assignment                  */
"Send",                                /* string end assignment             */
"Lt",                                  /* <                                 */
"Lt Right",                            /* < on right                        */
"In",                                  /* in                                */
"In Right",                            /* in on right                       */
"CREATE",                              /* create method                     */
"ITERATOR_START",                      /* start iterator method             */
"ITERATOR_NEXT",                       /* iterator next method              */
"SET_ITERATOR_START",                  /* start set iterator method         */
"SET_ITERATOR_NEXT",                   /* set iterator next method          */
"SELFSTR",                             /* printable string method           */
"User",                                /* user method                       */
/* ## end mcode_names */
   NULL};

   /* clear whatever might be in the slot and string tables */

   while (TABLE_BLOCK_HEAD != NULL) {

      tb_ptr = TABLE_BLOCK_HEAD;
      TABLE_BLOCK_HEAD = TABLE_BLOCK_HEAD->tb_next;
      free((void *)tb_ptr);

   }

   TABLE_NEXT_FREE = NULL;

   /* clear the string table */
  
   while (STRING_BLOCK_HEAD != NULL) {
      sb_ptr = STRING_BLOCK_HEAD;
      STRING_BLOCK_HEAD = STRING_BLOCK_HEAD->sb_next;
      free((void *)sb_ptr);

   }

   /* clear the hash table */

   for (i = 0; i < SLOTS__HASH_TABLE_SIZE; i++)
      HASH_TABLE[i] = NULL;

   /*
    *  we allocate an empty string block here, to avoid checking for a
    *  null string block with every string allocation
    */

   STRING_BLOCK_HEAD = (struct string_block *)
                       malloc(sizeof(struct string_block));
   if (STRING_BLOCK_HEAD == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);
   STRING_NEXT_FREE = STRING_BLOCK_HEAD->sb_data;
   STRING_BLOCK_EOS = STRING_BLOCK_HEAD->sb_data + STRING_BLOCK_SIZE;
   STRING_BLOCK_HEAD->sb_next = NULL;

   TOTAL_SLOT_COUNT = 0;
   for (i = 0; mcode_init[i] != NULL; i++)
      get_slot(SETL_SYSTEM mcode_init[i]);
   
    
   return;

}

/*\
 *  \function{get\_slottab()}
 *
 *  This function returns a slot table entry for a symbol.  First we try
 *  to find the symbol in the current slot table.  If a match is found,
 *  we just return a pointer to that item.  Otherwise we allocate a new
 *  item and enter it in the string, slot, and hash tables.
\*/

slot_ptr_type get_slot(
   SETL_SYSTEM_PROTO
   char *string)                       /* string to be found / installed    */

{
int string_hash;                       /* string's hash code                */
slot_ptr_type return_ptr;              /* return value                      */
struct table_block *old_head;          /* slot table block list head        */

   /* first, look up the string in the current slot table */

   string_hash = hashpjw(string);
   for (return_ptr = HASH_TABLE[string_hash];
        return_ptr != NULL && strcmp(return_ptr->sl_name,string) != 0;
        return_ptr = return_ptr->sl_hash_link);
   if (return_ptr != NULL)
      return return_ptr;

   /*
    *  We didn't find the string in the slot table, so we'll have to
    *  install it.  If there are no free items left allocate a new block.
    */

   if (TABLE_NEXT_FREE == NULL) {

      /* allocate a new block */

      old_head = TABLE_BLOCK_HEAD;
      TABLE_BLOCK_HEAD = (struct table_block *)
                         malloc(sizeof(struct table_block));
      if (TABLE_BLOCK_HEAD == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      TABLE_BLOCK_HEAD->tb_next = old_head;

      /* link items on the free list */

      for (TABLE_NEXT_FREE = TABLE_BLOCK_HEAD->tb_data;
           TABLE_NEXT_FREE <=
              TABLE_BLOCK_HEAD->tb_data + SLOT_BLOCK_SIZE - 2;
           TABLE_NEXT_FREE++) {
         TABLE_NEXT_FREE->ti_union.ti_next = TABLE_NEXT_FREE + 1;
      }

      TABLE_NEXT_FREE->ti_union.ti_next = NULL;
      TABLE_NEXT_FREE = TABLE_BLOCK_HEAD->tb_data;
   }

   /* at this point, we know the free list is not empty */

   return_ptr = &(TABLE_NEXT_FREE->ti_union.ti_data);
   TABLE_NEXT_FREE = TABLE_NEXT_FREE->ti_union.ti_next;

   /* initialize the new entry */

   clear_slot(return_ptr);
   return_ptr->sl_number = TOTAL_SLOT_COUNT++;
   return_ptr->sl_hash_link = HASH_TABLE[string_hash];
   HASH_TABLE[string_hash] = return_ptr;
   return_ptr->sl_name = get_strtab(SETL_SYSTEM string);

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

   if (STRING_NEXT_FREE + string_size + 1 > STRING_BLOCK_EOS) {

      old_head = STRING_BLOCK_HEAD;
      STRING_BLOCK_HEAD = (struct string_block *)
                          malloc(sizeof(struct string_block));
      if (STRING_BLOCK_HEAD == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);
      STRING_BLOCK_HEAD->sb_next = old_head;

      STRING_NEXT_FREE = STRING_BLOCK_HEAD->sb_data;
      STRING_BLOCK_EOS = STRING_BLOCK_HEAD->sb_data + STRING_BLOCK_SIZE - 1;

      
   }

   /* we know we have enough space, just install the string and return */

   return_ptr = STRING_NEXT_FREE;
   strcpy(STRING_NEXT_FREE,string);
   STRING_NEXT_FREE += string_size + 1;
   
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

   return hash_code % SLOTS__HASH_TABLE_SIZE;

}
