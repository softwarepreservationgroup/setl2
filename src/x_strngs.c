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
 *  This package contains definitions of the structures used to implement
 *  infinite length strings, and several low level functions to
 *  manipulate those structures.  We freely confess that we used a very
 *  ugly, non-portable C coding style here, in hopes of getting a fast
 *  implementation.  We have tried to isolate this ugliness to the macros
 *  and functions which allocate and release nodes.  In particular, there
 *  are some unsafe castes, so please make sure this file is compiled
 *  with unsafe optimizations disabled!
 *
 *  \texify{strngs.h}
 *
 *  \packagebody{String Table}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_strngs.h"                  /* strings                           */

/* performance tuning constants */

#define STR_HEADER_BLOCK_SIZE  100     /* string header block size          */
#define STR_CELL_BLOCK_SIZE    100     /* string cell block size            */

/*\
 *  \function{alloc\_string\_headers()}
 *
 *  This function allocates a block of string headers and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste header items to pointers to header items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the header node.
 *  It will work provided a header item is larger than a pointer, which
 *  is the case.
\*/

void alloc_string_headers(
   SETL_SYSTEM_PROTO_VOID)

{
string_h_ptr_type new_block;           /* first header in allocated block   */

   /* allocate a new block */

   new_block = (string_h_ptr_type)malloc((size_t)
         (STR_HEADER_BLOCK_SIZE * sizeof(struct string_h_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (STRING_H_NEXT_FREE = new_block;
        STRING_H_NEXT_FREE < new_block + STR_HEADER_BLOCK_SIZE - 1;
        STRING_H_NEXT_FREE++) {

      *((string_h_ptr_type *)STRING_H_NEXT_FREE) = STRING_H_NEXT_FREE + 1;

   }

   *((string_h_ptr_type *)STRING_H_NEXT_FREE) = NULL;

   /* set next free node to new block */

   STRING_H_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{alloc\_string\_cells()}
 *
 *  This function allocates a block of string cells and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste cell items to pointers to cell items in order to form the free
 *  list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently, while
 *  at the same time avoiding an extra pointer on the cell node.  It
 *  will work provided a cell item is larger than a pointer, which is
 *  the case.
\*/

void alloc_string_cells(
   SETL_SYSTEM_PROTO_VOID)

{
string_c_ptr_type new_block;           /* first cell in allocated block     */

   /* allocate a new block */

   new_block = (string_c_ptr_type)malloc((size_t)
         (STR_CELL_BLOCK_SIZE * sizeof(struct string_c_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (STRING_C_NEXT_FREE = new_block;
        STRING_C_NEXT_FREE < new_block + STR_CELL_BLOCK_SIZE - 1;
        STRING_C_NEXT_FREE++) {

      *((string_c_ptr_type *)STRING_C_NEXT_FREE) = STRING_C_NEXT_FREE + 1;

   }

   *((string_c_ptr_type *)STRING_C_NEXT_FREE) = NULL;

   /* set next free node to new block */

   STRING_C_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{free\_string()}
 *
 *  This functions releases the memory used by a string.
\*/

void free_string(
   SETL_SYSTEM_PROTO
   string_h_ptr_type string_hdr)       /* string root                       */

{
string_c_ptr_type string_cell_1, string_cell_2;
                                       /* string cell pointers              */

   string_cell_1 = string_hdr->s_head;
   while (string_cell_1 != NULL) {

      string_cell_2 = string_cell_1;
      string_cell_1 = string_cell_1->s_next;
      free_string_cell(string_cell_2);

   }

   free_string_header(string_hdr);

   return;

}

/*\
 *  \function{new\_string()}
 *
\*/

string_h_ptr_type new_string(
   SETL_SYSTEM_PROTO
   char *source)                       /* string to be converted            */

{
string_h_ptr_type target_hdr;          /* new structure                     */
string_c_ptr_type target_cell; 

char *target_char_ptr,*target_char_end;
char *p;

   /* allocate a new header */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = strlen(source);
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the argument to the string */

   for (p = source; *p; p++) {

      if (target_char_ptr == target_char_end) {

         get_string_cell(target_cell);
         if (target_hdr->s_tail != NULL)
            (target_hdr->s_tail)->s_next = target_cell;
         target_cell->s_prev = target_hdr->s_tail;
         target_cell->s_next = NULL;
         target_hdr->s_tail = target_cell;
         if (target_hdr->s_head == NULL)
            target_hdr->s_head = target_cell;
         target_char_ptr = target_cell->s_cell_value;
         target_char_end = target_char_ptr + STR_CELL_WIDTH;

      }

      *target_char_ptr++ = *p;

   }

   return target_hdr;

}
/*\
 *  \function{copy\_string()}
 *
 *  This function copies a string structure, and all associated string
 *  cells.
\*/

string_h_ptr_type copy_string(
   SETL_SYSTEM_PROTO
   string_h_ptr_type source)           /* structure to be copied            */

{
string_h_ptr_type target;              /* new structure                     */
string_c_ptr_type s1,t1,t2;            /* used to traverse source and       */
                                       /* target lists                      */
char *s,*t;                            /* temporary looping variables       */

   /* allocate a new header */

   get_string_header(target);

   target->s_use_count = 1;
   target->s_hash_code = source->s_hash_code;
   target->s_length = source->s_length;
   target->s_head = target->s_tail = NULL;

   /* copy each cell node */

   t1 = t2 = NULL;
   for (s1 = source->s_head;
        s1 != NULL;
        s1 = s1->s_next) {

      /* allocate a new cell node */

      get_string_cell(t1);
      if (t2 != NULL)
         t2->s_next = t1;
      else
         target->s_head = t1;

      t1->s_prev = t2;
      t2 = t1;

      for (s = s1->s_cell_value, t = t1->s_cell_value;
           s < s1->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }

   if (t1 != NULL) {

      t1->s_next = NULL;
      target->s_tail = t1;

   }

   return target;

}

/*\
 *  \function{string\_length()}
 *
 *  This function just returns the length of a character string.  It
 *  would be easy for the caller to access this field directly, but we
 *  would like to hide the string structures from built-in functions.
\*/

int32 string_length(
   SETL_SYSTEM_PROTO
   specifier *spec)                    /* specifier                         */

{

   return (spec->sp_val.sp_string_ptr)->s_length;

}

/*\
 *  \function{string\_value()}
 *
 *  This function returns a character string representation of a SETL2
 *  string.  Essentially, it just concatenates the strings in the cells.
 *
 *  We should note that this function uses \verb"malloc()" to allocate a
 *  return buffer.  The caller should free this buffer after it uses the
 *  string.
\*/

char *string_value(
   SETL_SYSTEM_PROTO
   specifier *spec)                    /* specifier                         */

{
char *return_ptr;                      /* returned string pointer           */
string_h_ptr_type h_ptr;               /* string header                     */
string_c_ptr_type s_ptr;               /* string cell pointer               */
char *s,*t;                            /* temporary looping variables       */

   /* extract the header pointer */

   h_ptr = spec->sp_val.sp_string_ptr;

   /* allocate a buffer for the string */

   return_ptr = (char *)malloc((size_t)(h_ptr->s_length + 1));
   if (return_ptr == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* copy each cell */

   t = return_ptr;
   for (s_ptr = h_ptr->s_head;
        s_ptr != NULL;
        s_ptr = s_ptr->s_next) {

      for (s = s_ptr->s_cell_value;
           s < s_ptr->s_cell_value + STR_CELL_WIDTH &&
              t < return_ptr + h_ptr->s_length;
           *t++ = *s++);

   }

   *t = '\0';

   return return_ptr;

}

/*\
 *  \function{string\_multiply()}
 *
 *  This function `multiplies' a string by an integer, which means it
 *  concatenates the string n times.
\*/

void string_multiply(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target specifier                  */
   specifier *source,                  /* string specifier                  */
   int32 copies)                       /* integer copies                    */

{
string_h_ptr_type target_string_hdr, source_string_hdr;
                                      /* string header pointers            */
string_c_ptr_type target_string_cell, source_string_cell;
                                       /* string cell pointers              */
char *target_char_ptr, *source_char_ptr;
                                       /* character pointers                */
char *target_char_end, *source_char_end;
                                       /* character pointers                */
int32 source_string_length;     /* string lengths                    */

   /* we start with a null string */

   get_string_header(target_string_hdr);
   target_string_hdr->s_use_count = 1;
   target_string_hdr->s_hash_code = -1;
   target_string_hdr->s_length = 0;
   target_string_hdr->s_head = target_string_hdr->s_tail = NULL;
   target_string_cell = NULL;
   target_char_ptr = target_char_end = NULL;

   source_string_hdr = source->sp_val.sp_string_ptr;

   /* concatenate copies of the string */

   while (copies--) {

      source_string_cell = source_string_hdr->s_head;

      if (source_string_cell == NULL) {

         source_char_ptr = source_char_end = NULL;

      }
      else {

         source_char_ptr = source_string_cell->s_cell_value;
         source_char_end = source_char_ptr + STR_CELL_WIDTH;

      }

      /* loop over the source string attaching it to the target */

      source_string_length = source_string_hdr->s_length;

      while (source_string_length--) {

         if (source_char_ptr == source_char_end) {

            source_string_cell = source_string_cell->s_next;
            source_char_ptr = source_string_cell->s_cell_value;
            source_char_end = source_char_ptr + STR_CELL_WIDTH;

         }

         if (target_char_ptr == target_char_end) {

            get_string_cell(target_string_cell);
            if (target_string_hdr->s_tail != NULL)
               (target_string_hdr->s_tail)->s_next = target_string_cell;
            target_string_cell->s_prev = target_string_hdr->s_tail;
            target_string_cell->s_next = NULL;
            target_string_hdr->s_tail = target_string_cell;
            if (target_string_hdr->s_head == NULL)
               target_string_hdr->s_head = target_string_cell;
            target_char_ptr = target_string_cell->s_cell_value;
            target_char_end = target_char_ptr + STR_CELL_WIDTH;

         }

         *target_char_ptr++ = *source_char_ptr++;

      }

      target_string_hdr->s_length += source_string_hdr->s_length;

   }

   /* assign our result to the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = target_string_hdr;

   return;

}

/*\
 *  \function{string\_fromb()}
 *
 *  This functions implements the SETL2 \verb"FROMB" operation.  Notice
 *  that all three operands may be modified.
\*/

void string_fromb(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
string_h_ptr_type target_string_hdr, left_string_hdr, right_string_hdr;
                                       /* string header pointers            */
string_c_ptr_type target_string_cell, left_string_cell, right_string_cell;
                                       /* string cell pointers              */
char *target_char_ptr, *left_char_ptr, *right_char_ptr;
                                       /* character pointers                */
char *target_char_end, *left_char_end, *right_char_end;
                                       /* character pointers                */
int32 target_string_length, left_string_length;
                                       /* string lengths                    */

   /* the right operand is the source */

   right_string_hdr = right->sp_val.sp_string_ptr;

   /* we start with null strings */

   get_string_header(left_string_hdr);
   left_string_hdr->s_use_count = 1;
   left_string_hdr->s_hash_code = -1;
   left_string_hdr->s_head = left_string_hdr->s_tail = NULL;
   left_char_ptr = left_char_end = NULL;

   if (right_string_hdr->s_length == 0)
      left_string_length = 0;
   else
      left_string_length = 1;
   left_string_hdr->s_length = left_string_length;

   get_string_header(target_string_hdr);
   target_string_hdr->s_use_count = 1;
   target_string_hdr->s_hash_code = -1;
   target_string_hdr->s_head = target_string_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   target_string_length = right_string_hdr->s_length - 1;
   if (target_string_length < 0)
      target_string_length = 0;
   target_string_hdr->s_length = target_string_length;

   right_string_cell = right_string_hdr->s_head;

   if (right_string_cell == NULL) {

      right_char_ptr = right_char_end = NULL;

   }
   else {

      right_char_ptr = right_string_cell->s_cell_value;
      right_char_end = right_char_ptr + STR_CELL_WIDTH;

   }

   /* copy one character from the source */

   while (left_string_length--) {

      if (right_char_ptr == right_char_end) {

         right_string_cell = right_string_cell->s_next;
         right_char_ptr = right_string_cell->s_cell_value;
         right_char_end = right_char_ptr + STR_CELL_WIDTH;

      }

      if (left_char_ptr == left_char_end) {

         get_string_cell(left_string_cell);
         if (left_string_hdr->s_tail != NULL)
            (left_string_hdr->s_tail)->s_next = left_string_cell;
         left_string_cell->s_prev = left_string_hdr->s_tail;
         left_string_cell->s_next = NULL;
         left_string_hdr->s_tail = left_string_cell;
         if (left_string_hdr->s_head == NULL)
            left_string_hdr->s_head = left_string_cell;
         left_char_ptr = left_string_cell->s_cell_value;
         left_char_end = left_char_ptr + STR_CELL_WIDTH;

      }

      *left_char_ptr++ = *right_char_ptr++;

   }

   /* copy all but one character of the source */

   while (target_string_length--) {

      if (right_char_ptr == right_char_end) {

         right_string_cell = right_string_cell->s_next;
         right_char_ptr = right_string_cell->s_cell_value;
         right_char_end = right_char_ptr + STR_CELL_WIDTH;

      }

      if (target_char_ptr == target_char_end) {

         get_string_cell(target_string_cell);
         if (target_string_hdr->s_tail != NULL)
            (target_string_hdr->s_tail)->s_next = target_string_cell;
         target_string_cell->s_prev = target_string_hdr->s_tail;
         target_string_cell->s_next = NULL;
         target_string_hdr->s_tail = target_string_cell;
         if (target_string_hdr->s_head == NULL)
            target_string_hdr->s_head = target_string_cell;
         target_char_ptr = target_string_cell->s_cell_value;
         target_char_end = target_char_ptr + STR_CELL_WIDTH;

      }

      *target_char_ptr++ = *right_char_ptr++;

   }

   /* assign our results */

   unmark_specifier(left);
   left->sp_form = ft_string;
   left->sp_val.sp_string_ptr = left_string_hdr;

   unmark_specifier(right);
   right->sp_form = ft_string;
   right->sp_val.sp_string_ptr = target_string_hdr;

   if (target != NULL) {

      left_string_hdr->s_use_count++;
      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = left_string_hdr;

   }

   return;

}

/*\
 *  \function{string\_frome()}
 *
 *  This functions implements the SETL2 \verb"FROME" operation.  Notice
 *  that all three operands may be modified.
\*/

void string_frome(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* target operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
string_h_ptr_type target_string_hdr, left_string_hdr, right_string_hdr;
                                       /* string header pointers            */
string_c_ptr_type target_string_cell, left_string_cell, right_string_cell;
                                       /* string cell pointers              */
char *target_char_ptr, *left_char_ptr, *right_char_ptr;
                                       /* character pointers                */
char *target_char_end, *left_char_end, *right_char_end;
                                       /* character pointers                */
int32 target_string_length, left_string_length;
                                       /* string lengths                    */

   /* the right operand is the source */

   right_string_hdr = right->sp_val.sp_string_ptr;

   /* we start with null strings */

   get_string_header(left_string_hdr);
   left_string_hdr->s_use_count = 1;
   left_string_hdr->s_hash_code = -1;
   left_string_hdr->s_head = left_string_hdr->s_tail = NULL;
   left_char_ptr = left_char_end = NULL;

   if (right_string_hdr->s_length == 0)
      left_string_length = 0;
   else
      left_string_length = 1;
   left_string_hdr->s_length = left_string_length;

   get_string_header(target_string_hdr);
   target_string_hdr->s_use_count = 1;
   target_string_hdr->s_hash_code = -1;
   target_string_hdr->s_head = target_string_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   target_string_length = right_string_hdr->s_length - 1;
   if (target_string_length < 0)
      target_string_length = 0;
   target_string_hdr->s_length = target_string_length;

   right_string_cell = right_string_hdr->s_head;

   if (right_string_cell == NULL) {

      right_char_ptr = right_char_end = NULL;

   }
   else {

      right_char_ptr = right_string_cell->s_cell_value;
      right_char_end = right_char_ptr + STR_CELL_WIDTH;

   }

   /* copy all but one character of the source */

   while (target_string_length--) {

      if (right_char_ptr == right_char_end) {

         right_string_cell = right_string_cell->s_next;
         right_char_ptr = right_string_cell->s_cell_value;
         right_char_end = right_char_ptr + STR_CELL_WIDTH;

      }

      if (target_char_ptr == target_char_end) {

         get_string_cell(target_string_cell);
         if (target_string_hdr->s_tail != NULL)
            (target_string_hdr->s_tail)->s_next = target_string_cell;
         target_string_cell->s_prev = target_string_hdr->s_tail;
         target_string_cell->s_next = NULL;
         target_string_hdr->s_tail = target_string_cell;
         if (target_string_hdr->s_head == NULL)
            target_string_hdr->s_head = target_string_cell;
         target_char_ptr = target_string_cell->s_cell_value;
         target_char_end = target_char_ptr + STR_CELL_WIDTH;

      }

      *target_char_ptr++ = *right_char_ptr++;

   }

   /* copy one character from the source */

   while (left_string_length--) {

      if (right_char_ptr == right_char_end) {

         right_string_cell = right_string_cell->s_next;
         right_char_ptr = right_string_cell->s_cell_value;
         right_char_end = right_char_ptr + STR_CELL_WIDTH;

      }

      if (left_char_ptr == left_char_end) {

         get_string_cell(left_string_cell);
         if (left_string_hdr->s_tail != NULL)
            (left_string_hdr->s_tail)->s_next = left_string_cell;
         left_string_cell->s_prev = left_string_hdr->s_tail;
         left_string_cell->s_next = NULL;
         left_string_hdr->s_tail = left_string_cell;
         if (left_string_hdr->s_head == NULL)
            left_string_hdr->s_head = left_string_cell;
         left_char_ptr = left_string_cell->s_cell_value;
         left_char_end = left_char_ptr + STR_CELL_WIDTH;

      }

      *left_char_ptr++ = *right_char_ptr++;

   }

   /* assign our results */

   unmark_specifier(left);
   left->sp_form = ft_string;
   left->sp_val.sp_string_ptr = left_string_hdr;

   unmark_specifier(right);
   right->sp_form = ft_string;
   right->sp_val.sp_string_ptr = target_string_hdr;

   if (target != NULL) {

      left_string_hdr->s_use_count++;
      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = left_string_hdr;

   }

   return;

}

