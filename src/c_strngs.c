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
 *  \package{The String Table}
 *
 *  This table stores string literals from the time they are scanned or
 *  read from a library until the module is written to a library.  It
 *  contains low-level functions for manipulating these structures and a
 *  function to convert character strings into string structures.
 *
 *  \texify{strngs.h}
 *
 *  \packagebody{String Table}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "compiler.h"                  /* SETL2 compiler constants          */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "chartab.h"                   /* character type table              */
#include "c_strngs.h"                    /* string literals                   */

/* performance tuning constants */

#define STRING_BLOCK_SIZE    200       /* string table block size           */
#define STRDAT_BLOCK_SIZE   4096       /* string data table block size      */

/* generic table item structure (string table use) */

struct table_item {
   union {
      struct table_item *ti_next;      /* next free item                    */
      struct string_item ti_data;      /* data area when in use             */
   } ti_union;
};

/* block of table items */

struct table_block {
   struct table_block *tb_next;        /* next block of data                */
   struct table_item tb_data[STRING_BLOCK_SIZE];
                                       /* array of table items              */
};

/* string data table block structure */

struct string_block {
   struct string_block *sb_next;       /* next block in list                */
   char sb_data[STRDAT_BLOCK_SIZE];    /* string table data                 */
};

/* package-global data */

static struct table_block *table_block_head = NULL;
                                       /* list of table blocks              */
static struct table_item *table_next_free = NULL;
                                       /* list of free table items          */
static struct string_block *string_block_head = NULL;
                                       /* head of list of string blocks     */
static char *string_block_eos = NULL;  /* end of allocated block            */
static char *string_next_free = NULL;  /* next free character in block      */

/* forward declarations */

static char *get_strtab(SETL_SYSTEM_PROTO char *,int);   
                                       /* allocate string table             */

/*\
 *  \function{init\_strings()}
 *
 *  This function initializes the string table.  First we delete the
 *  current string and data tables.  In most cases, they will already be
 *  empty.  We only initialize the string table before scanning a source
 *  file, and we expect there to be only one source file per execution,
 *  in most cases.
\*/

void init_strings(SETL_SYSTEM_PROTO_VOID)

{
struct table_block *tb_ptr;            /* work name table pointer           */
struct string_block *sb_ptr;           /* work string table pointer         */

   /* clear whatever might be in the string and data tables */

   while (table_block_head != NULL) {
      tb_ptr = table_block_head;
      table_block_head = table_block_head->tb_next;
      free((void *)tb_ptr);
   }
   table_next_free = NULL;

   /* clear the data table */

   while (string_block_head != NULL) {
      sb_ptr = string_block_head;
      string_block_head = string_block_head->sb_next;
      free((void *)sb_ptr);
   }

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

   return;

}

/*\
 *  \function{get\_string()}
 *
 *  This procedure allocates a string node. It is just like most of the
 *  other dynamic table allocation functions in the compiler.
\*/

string_ptr_type get_string(
   SETL_SYSTEM_PROTO
   char *string,                       /* string value                      */
   int length)                         /* length of string                  */

{
string_ptr_type return_ptr;            /* return value                      */
struct table_block *old_head;          /* name table block list head        */

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
              table_block_head->tb_data + STRING_BLOCK_SIZE - 2;
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

   clear_string(return_ptr);
   return_ptr->s_value = get_strtab(SETL_SYSTEM string,length);
   return_ptr->s_length = length;

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
   char *string,                       /* string to be installed            */
   int length)                         /* length of string to be installed  */

{
char *return_ptr;                      /* return value                      */
struct string_block *old_head;         /* string block head before alloc    */

   /* allocate a new block, if necessary */

   if (string_next_free + length + 1 > string_block_eos) {

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
   memcpy((void *)string_next_free,(void *)string,length);
   string_next_free += length + 1;

   return return_ptr;

}

/*\
 *  \function{char\_to\_string()}
 *
 *  This function converts lexical strings into an internal form.  The
 *  lexical analyzer should have validated the string, so all we do here
 *  is decode escape sequences.
\*/

string_ptr_type char_to_string(
   SETL_SYSTEM_PROTO
   char *in_string)                    /* string to be converted            */

{
string_ptr_type return_ptr;            /* return value                      */
char out_string[MAX_TOK_LEN + 1];      /* output string buffer              */
char *s,*t;                            /* source and target pointers        */

   /* note : we skip the leading quote */

   for (s = in_string + 1, t = out_string;
        *s;
        s++) {

      /* decode escape sequences */

      if (*s == '\\') {

         s++;

         switch (*s) {

            case '\\' :
               *t++ = '\\';
               break;

            case '0' :
               *t++ = '\0';
               break;

            case 'n' :
               *t++ = '\n';
               break;

            case 'r' :
               *t++ = '\r';
               break;

            case 'f' :
               *t++ = '\f';
               break;

            case 't' :
               *t++ = '\t';
               break;

            case '\"' :
               *t++ = '\"';
               break;

            case 'x' : case 'X' :

               s += 2;
               *t++ = numeric_val(*(s-1)) * 16 + numeric_val(*s);
               break;

         }

         continue;
      }

      *t++ = *s;

   }

   /* back up over the trailing quote */

   t--;

   return_ptr = get_string(SETL_SYSTEM out_string,(int)(t-out_string));

   return return_ptr;

}

