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
 *  \package{Integers}
 *
 *  This package contains definitions of the structures used to implement
 *  infinite length integers, and several low level functions to
 *  manipulate those structures.  We freely confess that we used a very
 *  ugly, non-portable C coding style here, in hopes of getting a fast
 *  implementation.  We have tried to isolate this ugliness to the macros
 *  and functions which allocate and release nodes.  In particular, there
 *  are some unsafe castes, so please make sure this file is compiled
 *  with unsafe optimizations disabled!
 *
 *  The routines provided in this package use arbitrary precision
 *  arithmetic to perform each operation.  These routines are fairly
 *  slow, so we try to use short arithmetic before resorting to these
 *  algorithms.
 *
 *  \texify{x_integers.h}
 *
 *  \packagebody{Integers}
\*/

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */

/* performance tuning constants */

#define INT_HEADER_BLOCK_SIZE   50     /* integer header block size         */
#define INT_CELL_BLOCK_SIZE    200     /* integer cell block size           */

/*\
 *  \function{alloc\_integer\_headers()}
 *
 *  This function allocates a block of integer headers and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste header items to pointers to header items in order to form the
 *  free list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently,
 *  while at the same time avoiding an extra pointer on the header node.
 *  It will work provided a header item is larger than a pointer, which
 *  is the case.
\*/

void alloc_integer_headers(
   SETL_SYSTEM_PROTO_VOID)

{
integer_h_ptr_type new_block;          /* first header in allocated block   */

   /* allocate a new block */

   new_block = (integer_h_ptr_type)malloc((size_t)
         (INT_HEADER_BLOCK_SIZE * sizeof(struct integer_h_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (INTEGER_H_NEXT_FREE = new_block;
        INTEGER_H_NEXT_FREE < new_block + INT_HEADER_BLOCK_SIZE - 1;
        INTEGER_H_NEXT_FREE++) {

      *((integer_h_ptr_type *)INTEGER_H_NEXT_FREE) = INTEGER_H_NEXT_FREE + 1;

   }

   *((integer_h_ptr_type *)INTEGER_H_NEXT_FREE) = NULL;

   /* set next free node to new block */

   INTEGER_H_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{alloc\_integer\_cells()}
 *
 *  This function allocates a block of integer cells and links them
 *  together into a free list.  Note carefully the castes used here: we
 *  caste cell items to pointers to cell items in order to form the free
 *  list.  This is extremely ugly, and we are not proud of the
 *  implementation, but it seems to be the only reasonable way to allow
 *  the allocation and release macros to be implemented efficiently, while
 *  at the same time avoiding an extra pointer on the cell node.  It
 *  will work provided a cell item is larger than a pointer, which is
 *  the case.
\*/

void alloc_integer_cells(
   SETL_SYSTEM_PROTO_VOID)

{
integer_c_ptr_type new_block;          /* first cell in allocated block     */

   /* allocate a new block */

   new_block = (integer_c_ptr_type)malloc((size_t)
         (INT_CELL_BLOCK_SIZE * sizeof(struct integer_c_item)));
   if (new_block == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* link items on the free list */

   for (INTEGER_C_NEXT_FREE = new_block;
        INTEGER_C_NEXT_FREE < new_block + INT_CELL_BLOCK_SIZE - 1;
        INTEGER_C_NEXT_FREE++) {

      *((integer_c_ptr_type *)INTEGER_C_NEXT_FREE) = INTEGER_C_NEXT_FREE + 1;

   }

   *((integer_c_ptr_type *)INTEGER_C_NEXT_FREE) = NULL;

   /* set next free node to new block */

   INTEGER_C_NEXT_FREE = new_block;

   return;

}

/*\
 *  \function{free\_integer()}
 *
 *  This function releases an entire integer structure.
\*/

void free_interp_integer(
   SETL_SYSTEM_PROTO
   integer_h_ptr_type header)          /* integer to be freed               */

{
integer_c_ptr_type t1,t2;              /* temporary looping variables       */

   t1 = header->i_head;

   while (t1 != NULL) {

      t2 = t1;
      t1 = t1->i_next;
      free_integer_cell(t2);

   }

   free_integer_header(header);

}

/*\
 *  \function{copy\_integer()}
 *
 *  This function copies an integer structure with all associated integer
 *  cells.
\*/

integer_h_ptr_type copy_integer(
   SETL_SYSTEM_PROTO
   integer_h_ptr_type source)          /* structure to be copied            */

{
integer_h_ptr_type target;             /* new integer structure             */
integer_c_ptr_type s1,t1,t2;           /* used to traverse source and       */
                                       /* target lists                      */

   /* allocate a new header */

   get_integer_header(target);

   target->i_use_count = 1;
   target->i_hash_code = source->i_hash_code;
   target->i_cell_count = source->i_cell_count;
   target->i_is_negative = source->i_is_negative;

   /* copy each cell node */

   t2 = NULL;
   for (s1 = source->i_head;
        s1 != NULL;
        s1 = s1->i_next) {

      /* allocate a new cell node */

      get_integer_cell(t1);
      if (t2 != NULL)
         t2->i_next = t1;
      else
         target->i_head = t1;

      t1->i_cell_value = s1->i_cell_value;
      t1->i_prev = t2;
      t2 = t1;

   }
   t1->i_next = NULL;
   target->i_tail = t1;

   return target;

}

/*\
 *  \function{short\_to\_long()}
 *
 *  This function converts a C long value into a SETL2 integer. It is used
 *  primarily when we try to perform short arithmetic and find an
 *  overflow.
\*/

void short_to_long(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* SETL2 integer result              */
   int32 source)                       /* C long value                      */

{
integer_c_ptr_type t1,t2;              /* work integer cell pointers        */
integer_h_ptr_type target_hdr;         /* work integer header pointer       */
int32 hi_bits;                         /* high order bits of short integer  */

   /* if we can use a short integer do so */

   if (!(hi_bits = source & INT_HIGH_BITS) ||
       hi_bits == INT_HIGH_BITS) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = source;

      return;

   }

   /* create a long integer */

   get_integer_header(target_hdr);
   target_hdr->i_use_count = 1;
   target_hdr->i_cell_count = 0;
   target_hdr->i_hash_code = -1;

   /* set the sign of the integer */

   if (source < 0L) {
      target_hdr->i_is_negative = YES;
      source = -source;
   }
   else {
      target_hdr->i_is_negative = NO;
   }

   /* keep adding cells until we use up the source */

   t2 = NULL;
   while (source) {

      /* allocate a new cell node */

      get_integer_cell(t1);
      if (t2 != NULL)
         t2->i_next = t1;
      else
         target_hdr->i_head = t1;

      t1->i_cell_value = source & MAX_INT_CELL;
      source >>= INT_CELL_WIDTH;
      t1->i_prev = t2;
      t2 = t1;

      target_hdr->i_cell_count++;

   }

   t1->i_next = NULL;
   target_hdr->i_tail = t1;

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = target_hdr;

   return;

}

/*\
 *  \function{long\_to\_short()}
 *
 *  This function converts a SETL2 long value into a C long.  We assume
 *  that the long integer will fit into a C long.
\*/

int32 long_to_short(
   SETL_SYSTEM_PROTO
   integer_h_ptr_type source)          /* SETL2 integer source              */

{
int32 return_value;                    /* returned C long                   */
integer_c_ptr_type t1;                 /* work integer cell pointer         */

   t1 = source->i_head;
   return_value = t1->i_cell_value;
   t1 = t1->i_next;

   if (t1 != NULL) {

      return_value |= (t1->i_cell_value << INT_CELL_WIDTH);
      t1 = t1->i_next;

   }

   if (t1 != NULL) {

      return_value |= ((t1->i_cell_value & 0x01) << (INT_CELL_WIDTH * 2));

   }

   if (source->i_is_negative)
      return_value = -return_value;

   return return_value;

}

/*\
 *  \function{long\_to\_double()}
 *
 *  This function converts a SETL2 long integer into a C double. We use it
 *  for explicit type conversions.
\*/

double long_to_double(
   SETL_SYSTEM_PROTO
   specifier *source)                  /* SETL2 long integer                */

{
double return_value;                   /* returned value                    */
integer_c_ptr_type t1;                 /* work cell pointer                 */

   return_value = 0.0;

   /* loop over the cells from high to low order, adding them to the result */

   for (t1 = (source->sp_val.sp_long_ptr)->i_tail;
        t1 != NULL;
        t1 = t1->i_prev) {

      return_value = return_value * (double)(MAX_INT_CELL + 1) +
                     (double)t1->i_cell_value;

   }

   /* set the sign of the result */

   if ((source->sp_val.sp_long_ptr)->i_is_negative)
      return_value = -return_value;

   return return_value;

}

/*\
 *  \function{integer\_string()}
 *
 *  This function returns a character string representation of a SETL2
 *  integer in a given base.  Since we have chosen a binary
 *  representation of integers for computation efficiency, we are stuck
 *  with an inefficient algorithm here.  We accept this since we presume
 *  we will perform arithmetic operations much more frequently.
 *
 *  The general idea is to keep traversing the list, each time dividing
 *  the integer by the base, and taking the remainder as the next digit,
 *  building the string from right to left.  To improve the efficiency a
 *  little, we extract several digits on each pass over the list.
 *
 *  We should note that this function uses \verb"malloc()" to allocate a
 *  return buffer.  The caller should free this buffer after it uses the
 *  string.
\*/

char *integer_string(
   SETL_SYSTEM_PROTO
   specifier *spec,                    /* specifier                         */
   int base)                           /* number base of string             */

{
char *return_ptr;                      /* returned string pointer           */
integer_c_ptr_type cell;               /* used to loop over cell list       */
int digits_per_cell;                   /* number of digits per cell         */
integer_h_ptr_type header;             /* header of work integer list       */
int32 divisor;                         /* largest power of base less than   */
                                       /* the maximum cell value            */
int32 remainder;                       /* remainder of cell / divisor       */
static char digit[] = {                /* digit character table             */
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
   'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
   'u', 'v', 'w', 'x', 'y', 'z'};
char *s,*t;                            /* temporary looping variables       */
int32 i;                               /* temporary looping variable        */

   /* first we calculate the maximum number of digits in each cell */

   i = MAX_INT_CELL;
   digits_per_cell = 0;

   while (i) {
      digits_per_cell++;
      i /= base;
   }

   /* copy the integer, for destructive use */

   header = copy_integer(SETL_SYSTEM spec->sp_val.sp_long_ptr);

   /* allocate a buffer for the string */

   return_ptr = (char *)malloc((size_t)
      (header->i_cell_count * digits_per_cell + 2));
   if (return_ptr == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   /* set the target pointer to the end of the string */

   t = return_ptr + (header->i_cell_count * digits_per_cell) + 1;
   *t-- = '\0';

   /* calculate an optimal divisor for each pass */

   divisor = 1;
   digits_per_cell = 0;
   i = MAX_INT_CELL;
   while (i > base) {
      divisor *= base;
      digits_per_cell++;
      i /= base;
   }

   /* make passes over the list, each time extracting digits_per_cell */

   while (t > return_ptr) {

      /* traverse the list, gathering digits_per_cell in remainder */

      remainder = 0;
      for (cell = header->i_tail;
           cell != NULL;
           cell = cell->i_prev) {

         cell->i_cell_value |= remainder << INT_CELL_WIDTH;

#if GCC & MC68020

         __asm__ volatile("divsll %3,%1:%0" :
                          "=d" (cell->i_cell_value), "=d" (remainder) :
                          "0" (cell->i_cell_value), "d" (divisor));
                          
#else

         remainder = cell->i_cell_value % divisor;
         cell->i_cell_value /= divisor;

#endif

      }

      /* append the digits to the return string */

      for (i = 0; t > return_ptr && i < digits_per_cell; i++) {

#if GCC & MC68020
{int tmp;

         __asm__ volatile("divsll %3,%1:%0" :
                          "=d" (remainder), "=d" (tmp) :
                          "0" (remainder), "d" (base));
         *t-- = digit[tmp];

}                          
#else

         *t-- = digit[remainder % base];
         remainder /= base;

#endif

      }
   }

   /*
    *  At this point we have a right-justified, zero-filled string.
    *  Left-justify it.
    */

   for (s = return_ptr + 1; *s == '0'; s++);
   if (!*s)
      s--;

   t = return_ptr;
   if (header->i_is_negative)
      *t++ = '-';

   while (*s)
      *t++ = *s++;
   *t = '\0';

   /* free the temporary integer (it's zero now) */

   free_interp_integer(SETL_SYSTEM header);

   return return_ptr;

}

/*\
 *  \function{integer\_add()}
 *
 *  This function adds two integers, where the integers may be either
 *  short or long.  It is meant to be called from the core interpreter
 *  for long integers, or from built-in functions for either type
 *  integer.
\*/

void integer_add(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* result operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
integer_h_ptr_type target_hdr, source_hdr;
                                       /* header pointers for result and    */
                                       /* operand                           */
integer_c_ptr_type target_cell, source_cell;
                                       /* cell pointers for result and      */
                                       /* operand                           */
int32 short_result;                    /* result of short arithmetic        */
int32 carry;                           /* amount carried to higher order    */
                                       /* cell                              */
int32 hi_bits;                         /* high order bits of short integer  */
specifier *swap;                       /* used to swap operands             */

   /* if at least one of the operands is short, we can use a fast method */

   if (left->sp_form == ft_short) {

      if (right->sp_form == ft_short) {

         /*
          *  Both operands are short.  We try adding the short values,
          *  and if the result is short we return.  Otherwise we convert
          *  the result to a long.
          */

         short_result = left->sp_val.sp_short_value +
                        right->sp_val.sp_short_value;

         /* check whether the result remains short */

         if (!(hi_bits = short_result & INT_HIGH_BITS) ||
             hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_result;

            return;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_result);

         return;

      }

      /*
       *  The left operand is short but the right is long.  We switch the
       *  two operands.
       */

      swap = left;
      left = right;
      right = swap;

   }

   /*
    *  At this point we know that the left operand is long.  The right
    *  operand can be either short or long.
    */

   /*
    *  If the right operand is short, we need only add the short value to
    *  the low order cell, and carry forward any carry value.
    */

   if (right->sp_form == ft_short) {

      /* we would like to use the left operand destructively */

      if (target == left &&
          target != right &&
          (target->sp_val.sp_long_ptr)->i_use_count == 1) {

         target_hdr = target->sp_val.sp_long_ptr;
         target->sp_form = ft_omega;

      }
      else {

         target_hdr = copy_integer(SETL_SYSTEM left->sp_val.sp_long_ptr);

      }

      /* we get the sign of the carry from the sign of the long */

      if (target_hdr->i_is_negative)
         carry = -(right->sp_val.sp_short_value);
      else
         carry = right->sp_val.sp_short_value;

      target_cell = target_hdr->i_head;

      /* traverse the list of cells until we have no carry value */

      while (carry) {

         /* check whether we must extend the target */

         if (target_cell == NULL) {

            get_integer_cell(target_cell);
            (target_hdr->i_tail)->i_next = target_cell;
            target_cell->i_prev = target_hdr->i_tail;
            target_cell->i_next = NULL;
            target_hdr->i_tail = target_cell;
            target_hdr->i_cell_count++;
            target_cell->i_cell_value = 0;

         }

         /* add in the carry, which could create another */

         target_cell->i_cell_value += carry;
         if (target_cell->i_cell_value < 0) {
            carry = -1;
            target_cell->i_cell_value += (MAX_INT_CELL + 1);
         }
         else {
            carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
            target_cell->i_cell_value &= MAX_INT_CELL;
         }

         /* set up for the next cell */

         target_cell = target_cell->i_next;

      }
   }

   /*
    *  At this point we know we have two long values.
    */

   else {

      /* we would like to use an operand destructively */

      if (target == left && target != right &&
          (target->sp_val.sp_long_ptr)->i_use_count == 1) {

         target_hdr = left->sp_val.sp_long_ptr;
         target->sp_form = ft_omega;
         source_hdr = right->sp_val.sp_long_ptr;

      }
      else if (target == right && target != left &&
               (target->sp_val.sp_long_ptr)->i_use_count == 1) {

         target_hdr = right->sp_val.sp_long_ptr;
         target->sp_form = ft_omega;
         source_hdr = left->sp_val.sp_long_ptr;

      }
      else {

         target_hdr = copy_integer(SETL_SYSTEM left->sp_val.sp_long_ptr);
         source_hdr = right->sp_val.sp_long_ptr;

      }

      /* if the two signs are the same, we add the structures */

      if (target_hdr->i_is_negative == source_hdr->i_is_negative) {

         source_cell = source_hdr->i_head;
         target_cell = target_hdr->i_head;
         carry = 0;

         /* add the source into the target */

         while (source_cell != NULL) {

            /* check whether we must extend the target */

            if (target_cell == NULL) {

               get_integer_cell(target_cell);
               (target_hdr->i_tail)->i_next = target_cell;
               target_cell->i_prev = target_hdr->i_tail;
               target_cell->i_next = NULL;
               target_hdr->i_tail = target_cell;
               target_hdr->i_cell_count++;
               target_cell->i_cell_value = 0;

            }

            /* add in the source cell */

            target_cell->i_cell_value +=
                  source_cell->i_cell_value + carry;
            carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
            target_cell->i_cell_value &= MAX_INT_CELL;

            /* set up for the next cell */

            target_cell = target_cell->i_next;
            source_cell = source_cell->i_next;

         }

         /* we might have a carry left */

         while (carry) {

            /* check whether we must extend the target */

            if (target_cell == NULL) {

               get_integer_cell(target_cell);
               (target_hdr->i_tail)->i_next = target_cell;
               target_cell->i_prev = target_hdr->i_tail;
               target_cell->i_next = NULL;
               target_hdr->i_tail = target_cell;
               target_hdr->i_cell_count++;
               target_cell->i_cell_value = 0;

            }

            /* add in the carry, which could create another */

            target_cell->i_cell_value += carry;
            carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
            target_cell->i_cell_value &= MAX_INT_CELL;

            /* set up for the next cell */

            target_cell = target_cell->i_next;

         }
      }

      /*
       *  The problem is harder when the integers have opposite signs,
       *  since we need to find the absolute value of the difference,
       *  along with the sign of the difference.
       */

      else {

         source_cell = source_hdr->i_head;
         target_cell = target_hdr->i_head;
         carry = 0;

         /* subtract the source from the target */

         while (source_cell != NULL) {

            /* check whether we must extend the target */

            if (target_cell == NULL) {

               get_integer_cell(target_cell);
               (target_hdr->i_tail)->i_next = target_cell;
               target_cell->i_prev = target_hdr->i_tail;
               target_cell->i_next = NULL;
               target_hdr->i_tail = target_cell;
               target_hdr->i_cell_count++;
               target_cell->i_cell_value = 0;

            }

            /* subtract the source cell */

            target_cell->i_cell_value -=
                  (source_cell->i_cell_value + carry);
            if (target_cell->i_cell_value < 0) {
               target_cell->i_cell_value += MAX_INT_CELL + 1;
               carry = 1;
            }
            else {
               carry = 0;
            }

            /* set up for the next cell */

            target_cell = target_cell->i_next;
            source_cell = source_cell->i_next;

         }

         /* if we have a carry, we extend it to the end of the integer */

         while (target_cell != NULL && carry) {

            /* add in the source cell */

            target_cell->i_cell_value -= carry;
            if (target_cell->i_cell_value < 0) {
               target_cell->i_cell_value += MAX_INT_CELL + 1;
               carry = 1;
            }
            else {
               carry = 0;
            }

            /* set up for the next cell */

            target_cell = target_cell->i_next;

         }

         /* if we finished with a carry, flip the sign */

         if (carry) {

            /* we subtract the carry, a high order 1 */

            carry = 1;
            for (target_cell = target_hdr->i_head;
                 target_cell != NULL;
                 target_cell = target_cell->i_next) {

               target_cell->i_cell_value =
                  MAX_INT_CELL - target_cell->i_cell_value + carry;
               carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
               target_cell->i_cell_value &= MAX_INT_CELL;

            }

            /* we might have a carry left */

            while (carry) {

               /* check whether we must extend the target */

               if (target_cell == NULL) {

                  get_integer_cell(target_cell);
                  (target_hdr->i_tail)->i_next = target_cell;
                  target_cell->i_prev = target_hdr->i_tail;
                  target_cell->i_next = NULL;
                  target_hdr->i_tail = target_cell;
                  target_hdr->i_cell_count++;
                  target_cell->i_cell_value = 0;

               }

               /* add in the carry, which could create another */

               target_cell->i_cell_value += carry;
               carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
               target_cell->i_cell_value &= MAX_INT_CELL;

               /* set up for the next cell */

               target_cell = target_cell->i_next;

            }

            /* finally, flip the sign flag */

            target_hdr->i_is_negative = !(target_hdr->i_is_negative);

         }

         /* we eliminate high-order zero cells */

         target_cell = target_hdr->i_tail;
         while (target_cell->i_prev != NULL &&
                !(target_cell->i_cell_value)) {

            target_hdr->i_tail = target_cell->i_prev;
            free_integer_cell(target_cell);
            target_cell = target_hdr->i_tail;
            target_hdr->i_cell_count--;

         }

         target_cell->i_next = NULL;

      }
   }

   /* we eliminate high-order zero cells */

   target_cell = target_hdr->i_tail;
   while (target_cell->i_prev != NULL &&
          !(target_cell->i_cell_value)) {

      target_hdr->i_tail = target_cell->i_prev;
      free_integer_cell(target_cell);
      target_cell = target_hdr->i_tail;
      target_hdr->i_cell_count--;

   }

   target_cell->i_next = NULL;

   /* the target hash code is now invalid */

   target_hdr->i_hash_code = -1;

   /*
    *  Now we have a long value in the target.  We would like to use
    *  short values whenever possible, so we check whether it will fit in
    *  a short.  If so, we convert it.
    */

   if (target_hdr->i_cell_count < 3) {

      /* build up a long value */

      carry = (target_hdr->i_head)->i_cell_value;
      if (target_hdr->i_cell_count == 2) {

         carry += (((target_hdr->i_head)->i_next)->i_cell_value) <<
                    INT_CELL_WIDTH;

      }

      if (target_hdr->i_is_negative)
         carry = -carry;

      /* check whether it will fit in a short */

      if (!(hi_bits = carry & INT_HIGH_BITS) ||
          hi_bits == INT_HIGH_BITS) {

         free_interp_integer(SETL_SYSTEM target_hdr);
         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = carry;

         return;

      }
   }

   /*
    *  We're stuck with a long.  Set the target and return.
    */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = target_hdr;

   return;

}

/*\
 *  \function{integer\_subtract()}
 *
 *  This function subtracts two integers, where the integers may be
 *  either short or long.  It is meant to be called from the core
 *  interpreter for long integers, or from built-in functions for either
 *  type integer.
\*/

void integer_subtract(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* result operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
integer_h_ptr_type target_hdr, source_hdr;
                                       /* header pointers for result and    */
                                       /* operand                           */
integer_c_ptr_type target_cell, source_cell;
                                       /* cell pointers for result and      */
                                       /* operand                           */
int32 short_result;                    /* result of short arithmetic        */
int32 carry;                           /* amount carried to higher order    */
                                       /* cell                              */
int32 hi_bits;                         /* high order bits of short integer  */
specifier *swap;                       /* used to swap operands             */
int reverse_sign;                      /* YES if we should reverse the      */
                                       /* sign of the result                */

   reverse_sign = NO;

   /* if at least one of the operands is short, we can use a fast method */

   if (left->sp_form == ft_short) {

      if (right->sp_form == ft_short) {

         /*
          *  Both operands are short.  We try subtracting the short
          *  values, and if the result is short we return.  Otherwise we
          *  convert the result to a long.
          */

         short_result = left->sp_val.sp_short_value -
                        right->sp_val.sp_short_value;

         /* check whether the result remains short */

         if (!(hi_bits = short_result & INT_HIGH_BITS) ||
             hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_result;

            return;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_result);

         return;

      }

      /*
       *  The left operand is short but the right is long.  We switch the
       *  two operands.
       */

      swap = left;
      left = right;
      right = swap;

      /* since we swapped operands we must reverse the sign of the result */

      reverse_sign = YES;

   }

   /*
    *  At this point we know that the left operand is long.  The right
    *  operand can be either short or long.
    */

   /*
    *  If the right operand is short, we need only add the short value to
    *  the low order cell, and carry forward any carry value.
    */

   if (right->sp_form == ft_short) {

      /* we would like to use the left operand destructively */

      if (target == left &&
          target != right &&
          (target->sp_val.sp_long_ptr)->i_use_count == 1) {

         target_hdr = target->sp_val.sp_long_ptr;
         target->sp_form = ft_omega;

      }
      else {

         target_hdr = copy_integer(SETL_SYSTEM left->sp_val.sp_long_ptr);

      }

      /* we get the sign of the carry from the sign of the long */

      if (target_hdr->i_is_negative)
         carry = right->sp_val.sp_short_value;
      else
         carry = -(right->sp_val.sp_short_value);

      target_cell = target_hdr->i_head;

      /* traverse the list of cells until we have no carry value */

      while (carry) {

         /* check whether we must extend the target */

         if (target_cell == NULL) {

            get_integer_cell(target_cell);
            (target_hdr->i_tail)->i_next = target_cell;
            target_cell->i_prev = target_hdr->i_tail;
            target_cell->i_next = NULL;
            target_hdr->i_tail = target_cell;
            target_hdr->i_cell_count++;
            target_cell->i_cell_value = 0;

         }

         /* add in the carry, which could create another */

         target_cell->i_cell_value += carry;
         if (target_cell->i_cell_value < 0) {
            carry = -1;
            target_cell->i_cell_value += (MAX_INT_CELL + 1);
         }
         else {
            carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
            target_cell->i_cell_value &= MAX_INT_CELL;
         }

         /* set up for the next cell */

         target_cell = target_cell->i_next;

      }
   }

   /*
    *  At this point we know we have two long values.
    */

   else {

      /* we would like to use an operand destructively */

      if (target == left && target != right &&
          (target->sp_val.sp_long_ptr)->i_use_count == 1) {

         target_hdr = left->sp_val.sp_long_ptr;
         target->sp_form = ft_omega;
         source_hdr = right->sp_val.sp_long_ptr;

      }
      else if (target == right && target != left &&
               (target->sp_val.sp_long_ptr)->i_use_count == 1) {

         target_hdr = right->sp_val.sp_long_ptr;
         target->sp_form = ft_omega;
         source_hdr = left->sp_val.sp_long_ptr;

         /* since we swapped operands we must reverse the sign */

         reverse_sign = YES;

      }
      else {

         target_hdr = copy_integer(SETL_SYSTEM left->sp_val.sp_long_ptr);
         source_hdr = right->sp_val.sp_long_ptr;

      }

      /* if the two signs are different, we add the structures */

      if (target_hdr->i_is_negative != source_hdr->i_is_negative) {

         source_cell = source_hdr->i_head;
         target_cell = target_hdr->i_head;
         carry = 0;

         /* add the source into the target */

         while (source_cell != NULL) {

            /* check whether we must extend the target */

            if (target_cell == NULL) {

               get_integer_cell(target_cell);
               (target_hdr->i_tail)->i_next = target_cell;
               target_cell->i_prev = target_hdr->i_tail;
               target_cell->i_next = NULL;
               target_hdr->i_tail = target_cell;
               target_hdr->i_cell_count++;
               target_cell->i_cell_value = 0;

            }

            /* add in the source cell */

            target_cell->i_cell_value +=
                  source_cell->i_cell_value + carry;
            carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
            target_cell->i_cell_value &= MAX_INT_CELL;

            /* set up for the next cell */

            target_cell = target_cell->i_next;
            source_cell = source_cell->i_next;

         }

         /* we might have a carry left */

         while (carry) {

            /* check whether we must extend the target */

            if (target_cell == NULL) {

               get_integer_cell(target_cell);
               (target_hdr->i_tail)->i_next = target_cell;
               target_cell->i_prev = target_hdr->i_tail;
               target_cell->i_next = NULL;
               target_hdr->i_tail = target_cell;
               target_hdr->i_cell_count++;
               target_cell->i_cell_value = 0;

            }

            /* add in the carry, which could create another */

            target_cell->i_cell_value += carry;
            carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
            target_cell->i_cell_value &= MAX_INT_CELL;

            /* set up for the next cell */

            target_cell = target_cell->i_next;

         }
      }

      /*
       *  The problem is harder when the integers have the same signs,
       *  since we need to find the absolute value of the difference,
       *  along with the sign of the difference.
       */

      else {

         source_cell = source_hdr->i_head;
         target_cell = target_hdr->i_head;
         carry = 0;

         /* subtract the source from the target */

         while (source_cell != NULL) {

            /* check whether we must extend the target */

            if (target_cell == NULL) {

               get_integer_cell(target_cell);
               (target_hdr->i_tail)->i_next = target_cell;
               target_cell->i_prev = target_hdr->i_tail;
               target_cell->i_next = NULL;
               target_hdr->i_tail = target_cell;
               target_hdr->i_cell_count++;
               target_cell->i_cell_value = 0;

            }

            /* subtract the source cell */

            target_cell->i_cell_value -=
                  (source_cell->i_cell_value + carry);
            if (target_cell->i_cell_value < 0) {
               target_cell->i_cell_value += MAX_INT_CELL + 1;
               carry = 1;
            }
            else {
               carry = 0;
            }

            /* set up for the next cell */

            target_cell = target_cell->i_next;
            source_cell = source_cell->i_next;

         }

         /* if we have a carry, we extend it to the end of the integer */

         while (target_cell != NULL && carry) {

            /* add in the source cell */

            target_cell->i_cell_value -= carry;
            if (target_cell->i_cell_value < 0) {
               target_cell->i_cell_value += MAX_INT_CELL + 1;
               carry = 1;
            }
            else {
               carry = 0;
            }

            /* set up for the next cell */

            target_cell = target_cell->i_next;

         }

         /* if we finished with a carry, flip the sign */

         if (carry) {

            /* we subtract the carry, a high order 1 */

            carry = 1;
            for (target_cell = target_hdr->i_head;
                 target_cell != NULL;
                 target_cell = target_cell->i_next) {

               target_cell->i_cell_value =
                  MAX_INT_CELL - target_cell->i_cell_value + carry;
               carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
               target_cell->i_cell_value &= MAX_INT_CELL;

            }

            /* we might have a carry left */

            while (carry) {

               /* check whether we must extend the target */

               if (target_cell == NULL) {

                  get_integer_cell(target_cell);
                  (target_hdr->i_tail)->i_next = target_cell;
                  target_cell->i_prev = target_hdr->i_tail;
                  target_cell->i_next = NULL;
                  target_hdr->i_tail = target_cell;
                  target_hdr->i_cell_count++;
                  target_cell->i_cell_value = 0;

               }

               /* add in the carry, which could create another */

               target_cell->i_cell_value += carry;
               carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
               target_cell->i_cell_value &= MAX_INT_CELL;

               /* set up for the next cell */

               target_cell = target_cell->i_next;

            }

            /* finally, flip the sign flag */

            target_hdr->i_is_negative = !(target_hdr->i_is_negative);

         }

         /* we eliminate high-order zero cells */

         target_cell = target_hdr->i_tail;
         while (target_cell->i_prev != NULL &&
                !(target_cell->i_cell_value)) {

            target_hdr->i_tail = target_cell->i_prev;
            free_integer_cell(target_cell);
            target_cell = target_hdr->i_tail;
            target_hdr->i_cell_count--;

         }

         target_cell->i_next = NULL;

      }
   }

   /* flip the sign if necessary */

   target_hdr->i_is_negative ^= reverse_sign;

   /* we eliminate high-order zero cells */

   target_cell = target_hdr->i_tail;
   while (target_cell->i_prev != NULL &&
          !(target_cell->i_cell_value)) {

      target_hdr->i_tail = target_cell->i_prev;
      free_integer_cell(target_cell);
      target_cell = target_hdr->i_tail;
      target_hdr->i_cell_count--;

   }

   target_cell->i_next = NULL;

   /* the target hash code is now invalid */

   target_hdr->i_hash_code = -1;

   /*
    *  Now we have a long value in the target.  We would like to use
    *  short values whenever possible, so we check whether it will fit in
    *  a short.  If so, we convert it.
    */

   if (target_hdr->i_cell_count < 3) {

      /* build up a long value */

      carry = (target_hdr->i_head)->i_cell_value;
      if (target_hdr->i_cell_count == 2) {

         carry += (((target_hdr->i_head)->i_next)->i_cell_value) <<
                    INT_CELL_WIDTH;

      }

      if (target_hdr->i_is_negative)
         carry = -carry;

      /* check whether it will fit in a short */

      if (!(hi_bits = carry & INT_HIGH_BITS) ||
          hi_bits == INT_HIGH_BITS) {

         free_interp_integer(SETL_SYSTEM target_hdr);
         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = carry;

         return;

      }
   }

   /*
    *  We're stuck with a long.  Set the target and return.
    */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = target_hdr;

   return;

}

/*\
 *  \function{integer\_multiply()}
 *
 *  This function multiplies two integers, where the integers may be either
 *  short or long.  It is meant to be called from the core interpreter
 *  for long integers, or for built-in functions for either type integer.
\*/

void integer_multiply(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* result location                   */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
integer_h_ptr_type target_hdr, left_hdr, right_hdr;
                                       /* header pointers for result and    */
                                       /* operands                          */
integer_c_ptr_type target_cell, last_target_cell, left_cell, right_cell;
                                       /* cell pointers for result and      */
                                       /* operands                          */
int32 short_result;                    /* result of short arithmetic        */
int32 hi_bits;                         /* high order bits of short integer  */
int32 carry;                           /* amount carried to higher order    */
                                       /* cell                              */
specifier *swap;                       /* used to swap operands             */

   /* if at least one of the operands is short, we can use a fast method */

   if (left->sp_form == ft_short) {

      if (right->sp_form == ft_short) {

         /*
          *  Both operands are short.  We try multiplying the short values,
          *  and if the result is short we return.  Otherwise we convert
          *  the result to a long.
          */

         short_result = left->sp_val.sp_short_value *
                        right->sp_val.sp_short_value;

         /* check whether the result remains short */

         if (!(hi_bits = short_result & INT_HIGH_BITS) ||
             hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_result;

            return;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_result);

         return;

      }

      /*
       *  The left operand is short but the right is long.  We switch the
       *  two operands.
       */

      swap = left;
      left = right;
      right = swap;

   }

   /*
    *  At this point we know that the left operand is long.  The right
    *  operand can be either short or long.
    */

   /*
    *  If the right operand is short, we need only multiply the short
    *  value from the low order cell, and carry forward any carry value.
    */

   if (right->sp_form == ft_short) {

      /* we would like to use the left operand destructively */

      if (target == left &&
          target != right &&
          (target->sp_val.sp_long_ptr)->i_use_count == 1) {

         target_hdr = target->sp_val.sp_long_ptr;
         target->sp_form = ft_omega;

      }
      else {

         target_hdr = copy_integer(SETL_SYSTEM left->sp_val.sp_long_ptr);

      }

      /* extract the short value and find the sign of the result */

      short_result = right->sp_val.sp_short_value;
      if (short_result < 0) {
         target_hdr->i_is_negative = !(target_hdr->i_is_negative);
         short_result = -short_result;
      }

      target_hdr->i_hash_code = -1;
      target_cell = target_hdr->i_head;
      carry = 0;

      /* traverse the list of cells */

      while (target_cell != NULL) {

         /* multiply one cell */

         target_cell->i_cell_value = target_cell->i_cell_value *
                                     short_result + carry;

         /* carry forward the high order bits */

         carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
         target_cell->i_cell_value &= MAX_INT_CELL;

         /* set up for the next cell */

         target_cell = target_cell->i_next;

      }

      /* we might have a carry left */

      if (carry) {

         get_integer_cell(target_cell);
         (target_hdr->i_tail)->i_next = target_cell;
         target_cell->i_prev = target_hdr->i_tail;
         target_cell->i_next = NULL;
         target_hdr->i_tail = target_cell;
         target_hdr->i_cell_count++;
         target_cell->i_cell_value = carry;

      }
   }

   /*
    *  At this point we know we have two long values.
    */

   else {

      /* create a brand new target with a value 0 */

      get_integer_header(target_hdr);
      target_hdr->i_cell_count = 1;
      target_hdr->i_use_count = 1;
      target_hdr->i_hash_code = -1;
      target_hdr->i_is_negative = (left->sp_val.sp_long_ptr)->i_is_negative ^
                                  (right->sp_val.sp_long_ptr)->i_is_negative;

      get_integer_cell(target_cell);
      target_cell->i_cell_value = 0;
      target_cell->i_next = target_cell->i_prev = NULL;
      target_hdr->i_head = target_hdr->i_tail = target_cell;
      last_target_cell = target_cell;

      left_hdr = left->sp_val.sp_long_ptr;
      right_hdr = right->sp_val.sp_long_ptr;

      /* multiply the left by each cell on the right */

      for (right_cell = right_hdr->i_head;
           right_cell != NULL;
           right_cell = right_cell->i_next) {

         short_result = right_cell->i_cell_value;
         left_cell = (left_hdr)->i_head;
         target_cell = last_target_cell;
         carry = 0;

         /* multiply the target by the source */

         while (left_cell != NULL) {

            /* check whether we must extend the target */

            if (target_cell == NULL) {

               get_integer_cell(target_cell);
               (target_hdr->i_tail)->i_next = target_cell;
               target_cell->i_prev = target_hdr->i_tail;
               target_cell->i_next = NULL;
               target_hdr->i_tail = target_cell;
               target_hdr->i_cell_count++;
               target_cell->i_cell_value = 0;
               if (last_target_cell == NULL)
                  last_target_cell = target_cell;

            }

            /* multiply one cell */

            target_cell->i_cell_value += short_result *
                                      left_cell->i_cell_value +
                                      carry;

            carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
            target_cell->i_cell_value &= MAX_INT_CELL;

            /* set up for the next cell */

            target_cell = target_cell->i_next;
            left_cell = left_cell->i_next;

         }

         /* we might have a carry left */

         while (carry) {

            /* check whether we must extend the target */

            if (target_cell == NULL) {

               get_integer_cell(target_cell);
               (target_hdr->i_tail)->i_next = target_cell;
               target_cell->i_prev = target_hdr->i_tail;
               target_cell->i_next = NULL;
               target_hdr->i_tail = target_cell;
               target_hdr->i_cell_count++;
               target_cell->i_cell_value = 0;
               if (last_target_cell == NULL)
                  last_target_cell = target_cell;

            }

            /* add in the carry, which could create another */

            target_cell->i_cell_value += carry;
            carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
            target_cell->i_cell_value &= MAX_INT_CELL;

            /* set up for the next cell */

            target_cell = target_cell->i_next;

         }

         last_target_cell = last_target_cell->i_next;

      }
   }

   /* we eliminate high-order zero cells */

   target_cell = target_hdr->i_tail;
   while (target_cell->i_prev != NULL &&
          !(target_cell->i_cell_value)) {

      target_hdr->i_tail = target_cell->i_prev;
      free_integer_cell(target_cell);
      target_cell = target_hdr->i_tail;
      target_hdr->i_cell_count--;

   }

   target_cell->i_next = NULL;

   /*
    *  Now we have a long value in the target.  We would like to use
    *  short values whenever possible, so we check whether it will fit in
    *  a short.  If so, we convert it.
    */

   if (target_hdr->i_cell_count < 3) {

      /* build up a long value */

      carry = (target_hdr->i_head)->i_cell_value;
      if (target_hdr->i_cell_count == 2) {

         carry += (((target_hdr->i_head)->i_next)->i_cell_value) <<
                    INT_CELL_WIDTH;

      }

      if (target_hdr->i_is_negative)
         carry = -carry;

      /* check whether it will fit in a short */

      if (!(hi_bits = carry & INT_HIGH_BITS) ||
          hi_bits == INT_HIGH_BITS) {

         free_interp_integer(SETL_SYSTEM target_hdr);
         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = carry;

         return;

      }
   }

   /*
    *  We're stuck with a long.  Set the target and return.
    */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = target_hdr;

   return;

}

/*\
 *  \function{integer\_divide()}
 *
 *  This function divides two integers, where the integers may be either
 *  short or long.  It is meant to be called from the core interpreter
 *  for long integers, or for built-in functions for either type integer.
\*/

void integer_divide(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* result operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
integer_h_ptr_type target_hdr, left_hdr, right_hdr;
                                       /* header pointers for result and    */
                                       /* operands                          */
integer_c_ptr_type target_cell, left_cell, left_cell_head, right_cell;
                                       /* cell pointers for result and      */
                                       /* operands                          */
int32 carry;                           /* amount carried to higher order    */
                                       /* cell                              */
int32 hi_bits;                         /* high order bits of short integer  */
int32 save_carry;                      /* saved carry                       */
int32 short_result;                    /* amount to divide by               */
unsigned int32 cell_multiplier;        /* multiplied by divisor during      */
                                       /* division operation                */
int passes;                            /* number of passes through          */
                                       /* algorithm                         */
int i;                                 /* temporary looping variable        */

   /* if at least one of the operands is short, we can use a fast method */

   if (left->sp_form == ft_short) {

      if (right->sp_form == ft_short) {

         /*
          *  Both operands are short.  We try dividing the short values,
          *  and if the result is short we return.  Otherwise we convert
          *  the result to a long.
          */

         if (right->sp_val.sp_short_value == 0)
            abend(SETL_SYSTEM msg_zero_divide);

         short_result = left->sp_val.sp_short_value /
                        right->sp_val.sp_short_value;

         /* check whether the result remains short */

         if (!(hi_bits = short_result & INT_HIGH_BITS) ||
             hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_result;

            return;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_result);

         return;

      }

      /*
       *  The left operand is short but the right is long.  If the long
       *  is two cells long we can divide.  Otherwise we return zero.
       */

      right_hdr = right->sp_val.sp_long_ptr;
      if (right_hdr->i_cell_count > 2) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = 0;

         return;

      }

      short_result = left->sp_val.sp_short_value /
         ((((right_hdr->i_head)->i_next)->i_cell_value << INT_CELL_WIDTH) +
         (right_hdr->i_head)->i_cell_value);
      if (right_hdr->i_is_negative)
         short_result = -short_result;

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = short_result;

      return;

   }

   /*
    *  At this point we know that the left operand is long.  The right
    *  operand can be either short or long.
    */

   /*
    *  If the right operand is short, we need only divide the long by
    *  the short value.
    */

   if (right->sp_form == ft_short) {

      /* check for divide by zero */

      if (right->sp_val.sp_short_value == 0)
         abend(SETL_SYSTEM msg_zero_divide);

      /* we would like to use the left operand destructively */

      if (target == left &&
          target != right &&
          (target->sp_val.sp_long_ptr)->i_use_count == 1) {

         target_hdr = target->sp_val.sp_long_ptr;
         target->sp_form = ft_omega;

      }
      else {

         target_hdr = copy_integer(SETL_SYSTEM left->sp_val.sp_long_ptr);

      }

      /* extract the short value and find the sign of the result */

      short_result = right->sp_val.sp_short_value;
      if (short_result < 0) {
         target_hdr->i_is_negative = !(target_hdr->i_is_negative);
         short_result = -short_result;
      }

      target_cell = target_hdr->i_tail;
      carry = 0;

      /* traverse the list of cells */

      while (target_cell != NULL) {

         /* divide one cell */

         save_carry = carry;
         carry =
            (target_cell->i_cell_value + (save_carry << INT_CELL_WIDTH)) %
                  short_result;

         target_cell->i_cell_value =
            (target_cell->i_cell_value + (save_carry << INT_CELL_WIDTH)) /
                  short_result;

         /* set up for the next cell */

         target_cell = target_cell->i_prev;

      }
   }

   /*
    *  At this point we know we have two long values.
    */

   else {

      /* create a brand new target with a value 0 */

      get_integer_header(target_hdr);
      target_hdr->i_cell_count = 1;
      target_hdr->i_use_count = 1;
      target_hdr->i_hash_code = -1;
      target_hdr->i_is_negative = (left->sp_val.sp_long_ptr)->i_is_negative ^
                                  (right->sp_val.sp_long_ptr)->i_is_negative;

      get_integer_cell(target_cell);
      target_cell->i_cell_value = 0;
      target_cell->i_next = target_cell->i_prev = NULL;
      target_hdr->i_head = target_hdr->i_tail = target_cell;

      left_hdr = copy_integer(SETL_SYSTEM left->sp_val.sp_long_ptr);
      right_hdr = right->sp_val.sp_long_ptr;

      /* first, find the number of passes we have to make */

      passes = left_hdr->i_cell_count - right_hdr->i_cell_count + 1;
      while (passes-- > 0) {

         /*
          *  we will work on a chain the length of the divisor, but
          *  at the high order end
          */

         left_cell_head = left_hdr->i_tail;
         for (i = 1; i < right_hdr->i_cell_count; i++)
            left_cell_head = left_cell_head->i_prev;

         /* keep working on this frame until we can not continue */

         for (;;) {

            /*
             *  The following truly horrible expression makes a
             *  fairly good estimate of the largest multiple of the
             *  right_hdr we can subtract from this frame.
             */

            left_cell = left_hdr->i_tail;
            right_cell = right_hdr->i_tail;

            cell_multiplier = (unsigned int32)(left_cell->i_cell_value << 2);
            cell_multiplier += (left_cell->i_prev)->i_cell_value >>
                                    (INT_CELL_WIDTH - 2);
            cell_multiplier /= ((right_cell->i_cell_value << 2) +
                  ((right_cell->i_prev)->i_cell_value >>
                     (INT_CELL_WIDTH - 2)) + 1);

            /*
             *  we sometimes make one extra pass over the frame,
             *  but it's tough to anticipate those situations
             */

            if (cell_multiplier == 0) {
               if ((right_hdr->i_tail)->i_cell_value <=
                   (left_hdr->i_tail)->i_cell_value)
                  cell_multiplier = 1;
               else
                  break;
            }

            /*
             *  Now we're ready to process the current frame.  We
             *  subtract cell_multiplier times the divisor from the
             *  frame.
             */

            carry = 0;
            left_cell = left_cell_head;
            right_cell = right_hdr->i_head;

            while (right_cell != NULL) {

               /* subtract the divisor cell */

               left_cell->i_cell_value -= (carry +
                     right_cell->i_cell_value * cell_multiplier);

               /* check for a carry */

               if (left_cell->i_cell_value < 0) {
                  if (left_cell->i_cell_value % (MAX_INT_CELL + 1) != 0)
                     carry =
                        (left_cell->i_cell_value / -(MAX_INT_CELL + 1)) + 1;
                  else
                     carry =
                        left_cell->i_cell_value / -(MAX_INT_CELL + 1);
                  left_cell->i_cell_value += (MAX_INT_CELL + 1) * carry;
               }
               else {
                  carry = 0;
               }

               /* set up for the next cell */

               right_cell = right_cell->i_next;
               left_cell = left_cell->i_next;

            }

            if (carry) {

#ifdef TRAPS

               /*
                *  We should only have reduced the dividend by one
                *  extra multiple of the divisor.
                */

               if (cell_multiplier != 1)
                  trap(__FILE__,__LINE__,msg_divide_error);

#endif

               /* back out one multiple of the divisor */

               cell_multiplier = 0;
               carry = 0;
               left_cell = left_cell_head;
               right_cell = right_hdr->i_head;

               while (right_cell != NULL) {

                  /* subtract the divisor cell */

                  left_cell->i_cell_value +=
                        (carry + right_cell->i_cell_value);

                  carry = left_cell->i_cell_value >> INT_CELL_WIDTH;
                  left_cell->i_cell_value &= MAX_INT_CELL;

                  /* set up for the next cell */

                  right_cell = right_cell->i_next;
                  left_cell = left_cell->i_next;

               }

               break;

            }

            /*
             *  At this point, we've subtracted some multiple of
             *  the divisor from the remainder.  We have to update
             *  the quotient by adding in that multiple.
             */

            target_cell = target_hdr->i_head;
            carry = cell_multiplier;

            while (carry) {

               /* check whether we must extend the quotient */

               if (target_cell == NULL) {

                  get_integer_cell(target_cell);
                  (target_hdr->i_tail)->i_next = target_cell;
                  target_cell->i_prev = target_hdr->i_tail;
                  target_cell->i_next = NULL;
                  target_hdr->i_tail = target_cell;
                  target_hdr->i_cell_count++;
                  target_cell->i_cell_value = 0;

               }

               /* add in the carry, which could create another */

               target_cell->i_cell_value += carry;
               carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
               target_cell->i_cell_value &= MAX_INT_CELL;

               /* set up for the next cell */

               target_cell = target_cell->i_next;

            }
         }

         /*
          *  Finally, we've finished one pass.  If there are more
          *  passes, we shift the quotient by adding a low-order
          *  cell.
          */

         if (passes) {

            get_integer_cell(target_cell);
            (target_hdr->i_head)->i_prev = target_cell;
            target_cell->i_next = target_hdr->i_head;
            target_cell->i_prev = NULL;
            target_hdr->i_head = target_cell;
            target_hdr->i_cell_count++;
            target_cell->i_cell_value = 0;

            /* shorten the left_hdr by one cell */

            left_cell = left_hdr->i_tail;
            (left_cell->i_prev)->i_cell_value +=
                  left_cell->i_cell_value << INT_CELL_WIDTH;

            left_hdr->i_tail = left_cell->i_prev;
            free_integer_cell(left_cell);
            (left_hdr->i_tail)->i_next = NULL;
            left_hdr->i_cell_count--;

         }
      }

      free_interp_integer(SETL_SYSTEM left_hdr);

   }

   /* we eliminate high-order zero cells */

   target_cell = target_hdr->i_tail;
   while (target_cell->i_prev != NULL &&
          !(target_cell->i_cell_value)) {

      target_hdr->i_tail = target_cell->i_prev;
      free_integer_cell(target_cell);
      target_cell = target_hdr->i_tail;
      target_hdr->i_cell_count--;

   }

   target_cell->i_next = NULL;

   /* the target hash code is now invalid */

   target_hdr->i_hash_code = -1;

   /*
    *  Now we have a long value in the target.  We would like to use
    *  short values whenever possible, so we check whether it will fit in
    *  a short.  If so, we convert it.
    */

   if (target_hdr->i_cell_count < 3) {

      /* build up a long value */

      carry = (target_hdr->i_head)->i_cell_value;
      if (target_hdr->i_cell_count == 2) {

         carry += (((target_hdr->i_head)->i_next)->i_cell_value) <<
                    INT_CELL_WIDTH;

      }

      if (target_hdr->i_is_negative)
         carry = -carry;

      /* check whether it will fit in a short */

      if (!(hi_bits = carry & INT_HIGH_BITS) ||
          hi_bits == INT_HIGH_BITS) {

         free_interp_integer(SETL_SYSTEM target_hdr);
         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = carry;

         return;

      }
   }

   /*
    *  We're stuck with a long.  Set the target and return.
    */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = target_hdr;

   return;

}

/*\
 *  \function{integer\_power()}
 *
 *  This function raises one integer to the power of the other.  It is
 *  used both by the core interpreter and built-in functions.
\*/

void integer_power(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* result location                   */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
specifier result;                      /* working result                    */
specifier multiplier;                  /* working multiplier                */
integer_c_ptr_type next_cell;          /* next cell in exponent             */
int32 current_cell_value;              /* current long in exponent          */
int current_bit;                       /* current bit in exponent           */

   /* we initialize the exponent work areas from the right operand */

   if (right->sp_form == ft_short) {

      current_cell_value = right->sp_val.sp_short_value;
      next_cell = NULL;

      if (current_cell_value < 0) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = 0;

         return;

      }
   }
   else {

      if ((right->sp_val.sp_long_ptr)->i_is_negative) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = 0;

         return;

      }

      next_cell = (right->sp_val.sp_long_ptr)->i_head;
      current_cell_value = next_cell->i_cell_value;
      next_cell = next_cell->i_next;

   }
   current_bit = 0;

   /* we start with a result of 1 */

   result.sp_form = ft_short;
   result.sp_val.sp_short_value = 1;

   /* the multiplier is initially the left operand */

   multiplier.sp_form = left->sp_form;
   multiplier.sp_val.sp_biggest = left->sp_val.sp_biggest;
   mark_specifier(&multiplier);

   /* keep processing bits in the exponent */

   for (;;) {

      /* if the current bit is one, multiply result by multiplier */

      if (current_cell_value & 0x01)
         integer_multiply(SETL_SYSTEM &result,&result,&multiplier);

      /* set up for the next bit */

      current_bit++;
      if (current_bit < INT_CELL_WIDTH) {

         current_cell_value >>= 1;

      }
      else {

         if (next_cell != NULL) {

            current_cell_value = next_cell->i_cell_value;
            next_cell = next_cell->i_next;

         }
         else {

            current_cell_value = 0;

         }

         current_bit = 0;

      }

      /* break if the exponent is exhausted */

      if (!current_cell_value && next_cell == NULL)
         break;

      /* square the multiplier */

      integer_multiply(SETL_SYSTEM &multiplier,&multiplier,&multiplier);

   }

   /* we set the target to our result */

   unmark_specifier(target);
   target->sp_form = result.sp_form;
   target->sp_val.sp_biggest = result.sp_val.sp_biggest;
   unmark_specifier(&multiplier);

   return;

}

/*\
 *  \function{integer\_mod()}
 *
 *  This function divides two integers, returning the remainder.  It is
 *  meant to be called from the core interpreter for long integers, or
 *  for built-in functions for either type integer.
\*/

void integer_mod(
   SETL_SYSTEM_PROTO
   specifier *target,                  /* result operand                    */
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
integer_h_ptr_type target_hdr, right_hdr;
                                       /* header pointers for result and    */
                                       /* operands                          */
integer_c_ptr_type target_cell, target_cell_head, right_cell;
                                       /* cell pointers for result and      */
                                       /* operands                          */
int32 carry;                           /* amount carried to higher order    */
                                       /* cell                              */
int32 hi_bits;                         /* high order bits of short integer  */
int32 save_carry;                      /* saved carry                       */
int32 short_result;                    /* amount to divide by               */
unsigned int32 cell_multiplier;        /* multiplied by divisor during      */
                                       /* division operation                */
int passes;                            /* number of passes through          */
                                       /* algorithm                         */
int i;                                 /* temporary looping variable        */
specifier spare1;                      /* spare specifier for sign          */
                                       /* adjustment                        */

   /* if at least one of the operands is short, we can use a fast method */

   if (left->sp_form == ft_short) {

      if (right->sp_form == ft_short) {

         /*
          *  Both operands are short.  We try dividing the short values,
          *  and if the result is short we return.  Otherwise we convert
          *  the result to a long.
          */

         if (right->sp_val.sp_short_value == 0)
            abend(SETL_SYSTEM msg_zero_divide);

         short_result = labs(left->sp_val.sp_short_value) %
                        labs(right->sp_val.sp_short_value);

         /* make sure mod is positive */

         if (short_result != 0) {

            if (left->sp_val.sp_short_value < 0 &&
                right->sp_val.sp_short_value > 0)
               short_result = right->sp_val.sp_short_value - short_result;

            if (left->sp_val.sp_short_value >= 0 &&
                right->sp_val.sp_short_value < 0)
               short_result = -right->sp_val.sp_short_value - short_result;

         }

         /* check whether the result remains short */

         if (!(hi_bits = short_result & INT_HIGH_BITS) ||
             hi_bits == INT_HIGH_BITS) {

            unmark_specifier(target);
            target->sp_form = ft_short;
            target->sp_val.sp_short_value = short_result;

            return;

         }

         /* if we exceed the maximum short, convert to long */

         short_to_long(SETL_SYSTEM target,short_result);

         return;

      }

      /*
       *  The left operand is short but the right is long.  If the long
       *  is two cells long we can divide.  Otherwise we return the left.
       */

      right_hdr = right->sp_val.sp_long_ptr;

      if (right_hdr->i_cell_count > 2) {

         /* notice that right > left */

         if ((left->sp_val.sp_short_value < 0 &&
             !(right_hdr->i_is_negative)) ||
             (left->sp_val.sp_short_value > 0 &&
              right_hdr->i_is_negative)) {

            integer_add(SETL_SYSTEM target,right,left);
            if (target->sp_form == ft_short) {

               target->sp_val.sp_short_value =
                  labs(target->sp_val.sp_short_value);

            }
            else {

               (target->sp_val.sp_long_ptr)->i_is_negative = NO;

            }

            return;

         }

         short_result = labs(left->sp_val.sp_short_value);

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_result;

         return;

      }

      /* if there are only two cells in right, find mod normally */

      short_result = labs(left->sp_val.sp_short_value) %
         ((((right_hdr->i_head)->i_next)->i_cell_value << INT_CELL_WIDTH) +
         (right_hdr->i_head)->i_cell_value);

      /* make sure mod is positive */

      if (short_result != 0) {

         if (left->sp_val.sp_short_value < 0 &&
             !(right_hdr->i_is_negative)) {

            short_result =
                  ((((right_hdr->i_head)->i_next)->i_cell_value <<
                     INT_CELL_WIDTH) +
                  (right_hdr->i_head)->i_cell_value) -
                  short_result;

         }

         if (left->sp_val.sp_short_value > 0 &&
             right_hdr->i_is_negative) {

            short_result =
                  ((((right_hdr->i_head)->i_next)->i_cell_value <<
                     INT_CELL_WIDTH) +
                  (right_hdr->i_head)->i_cell_value) -
                  short_result;

         }
      }

      /* check whether the result remains short */

      if (!(hi_bits = short_result & INT_HIGH_BITS) ||
          hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_result;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_result);

      return;

   }

   /*
    *  At this point we know that the left operand is long.  The right
    *  operand can be either short or long.
    */

   /*
    *  If the right operand is short, we need only divide the long by
    *  the short value.
    */

   if (right->sp_form == ft_short) {

      /* check for divide by zero */

      if (right->sp_val.sp_short_value == 0)
         abend(SETL_SYSTEM msg_zero_divide);

      /* we would like to use the left operand destructively */

      if (target == left &&
          target != right &&
          (target->sp_val.sp_long_ptr)->i_use_count == 1) {

         target_hdr = target->sp_val.sp_long_ptr;
         target->sp_form = ft_omega;

      }
      else {

         target_hdr = copy_integer(SETL_SYSTEM left->sp_val.sp_long_ptr);

      }

      /* extract the short value and find the sign of the result */

      short_result = right->sp_val.sp_short_value;
      if (short_result < 0) {
         short_result = -short_result;
      }

      target_cell = target_hdr->i_tail;
      carry = 0;

      /* traverse the list of cells */

      while (target_cell != NULL) {

         /* divide one cell */

         save_carry = carry;
         carry =
            (target_cell->i_cell_value + (save_carry << INT_CELL_WIDTH)) %
                  short_result;

         target_cell->i_cell_value =
            (target_cell->i_cell_value + (save_carry << INT_CELL_WIDTH)) /
                  short_result;

         /* set up for the next cell */

         target_cell = target_cell->i_prev;

      }

      /* the carry contains the remainder */

      short_result = carry;

      free_interp_integer(SETL_SYSTEM target_hdr);

      /* make sure mod is positive */

      if (short_result != 0) {

         if ((left->sp_val.sp_long_ptr)->i_is_negative &&
             right->sp_val.sp_short_value > 0)
            short_result = right->sp_val.sp_short_value - short_result;

         if (!((left->sp_val.sp_long_ptr)->i_is_negative) &&
             right->sp_val.sp_short_value < 0)
            short_result = -right->sp_val.sp_short_value - short_result;

      }

      if (!(hi_bits = short_result & INT_HIGH_BITS) ||
          hi_bits == INT_HIGH_BITS) {

         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = short_result;

         return;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM target,short_result);

      return;

   }

   /*
    *  At this point we know we have two long values.
    */

   /* we would like to use the left operand destructively */

   if (target == left &&
       target != right &&
       (target->sp_val.sp_long_ptr)->i_use_count == 1) {

      target_hdr = target->sp_val.sp_long_ptr;
      target->sp_form = ft_omega;

   }
   else {

      target_hdr = copy_integer(SETL_SYSTEM left->sp_val.sp_long_ptr);

   }

   right_hdr = right->sp_val.sp_long_ptr;

   /* find the number of passes we have to make */

   passes = target_hdr->i_cell_count - right_hdr->i_cell_count + 1;
   while (passes-- > 0) {

      /*
       *  we will work on a chain the length of the divisor, but
       *  at the high order end
       */

      target_cell_head = target_hdr->i_tail;
      for (i = 1; i < right_hdr->i_cell_count; i++)
         target_cell_head = target_cell_head->i_prev;

      /* keep working on this frame until we can not continue */

      for (;;) {

         /*
          *  The following truly horrible expression makes a
          *  fairly good estimate of the largest multiple of the
          *  right_hdr we can subtract from this frame.
          */

         target_cell = target_hdr->i_tail;
         right_cell = right_hdr->i_tail;

         cell_multiplier = (unsigned int32)(target_cell->i_cell_value << 2);
         cell_multiplier += (target_cell->i_prev)->i_cell_value >>
                                 (INT_CELL_WIDTH - 2);
         cell_multiplier /= ((right_cell->i_cell_value << 2) +
               ((right_cell->i_prev)->i_cell_value >>
                  (INT_CELL_WIDTH - 2)) + 1);

         /*
          *  we sometimes make one extra pass over the frame,
          *  but it's tough to anticipate those situations
          */

         if (cell_multiplier == 0) {
            if ((right_hdr->i_tail)->i_cell_value <=
                (target_hdr->i_tail)->i_cell_value)
               cell_multiplier = 1;
            else
               break;
         }

         /*
          *  Now we're ready to process the current frame.  We
          *  subtract cell_multiplier times the divisor from the
          *  frame.
          */

         carry = 0;
         target_cell = target_cell_head;
         right_cell = right_hdr->i_head;

         while (right_cell != NULL) {

            /* subtract the divisor cell */

            target_cell->i_cell_value -= (carry +
                  right_cell->i_cell_value * cell_multiplier);

            /* check for a carry */

            if (target_cell->i_cell_value < 0) {
               if (target_cell->i_cell_value % (MAX_INT_CELL + 1) != 0)
                  carry =
                     (target_cell->i_cell_value / -(MAX_INT_CELL + 1)) + 1;
               else
                  carry =
                     target_cell->i_cell_value / -(MAX_INT_CELL + 1);
               target_cell->i_cell_value += (MAX_INT_CELL + 1) * carry;
            }
            else {
               carry = 0;
            }

            /* set up for the next cell */

            right_cell = right_cell->i_next;
            target_cell = target_cell->i_next;

         }

         if (carry) {

#ifdef TRAPS

            /*
             *  We should only have reduced the dividend by one
             *  extra multiple of the divisor.
             */

            if (cell_multiplier != 1)
               trap(__FILE__,__LINE__,msg_divide_error);

#endif

            /* back out one multiple of the divisor */

            cell_multiplier = 0;
            carry = 0;
            target_cell = target_cell_head;
            right_cell = right_hdr->i_head;

            while (right_cell != NULL) {

               /* subtract the divisor cell */

               target_cell->i_cell_value +=
                     (carry + right_cell->i_cell_value);

               carry = target_cell->i_cell_value >> INT_CELL_WIDTH;
               target_cell->i_cell_value &= MAX_INT_CELL;

               /* set up for the next cell */

               right_cell = right_cell->i_next;
               target_cell = target_cell->i_next;

            }

            break;

         }
      }

      /*
       *  Finally, we've finished one pass.  We need to eliminate the
       *  high-order cell of the target.
       */

      if (passes) {

         target_cell = target_hdr->i_tail;
         (target_cell->i_prev)->i_cell_value +=
               target_cell->i_cell_value << INT_CELL_WIDTH;

         target_hdr->i_tail = target_cell->i_prev;
         free_integer_cell(target_cell);
         (target_hdr->i_tail)->i_next = NULL;
         target_hdr->i_cell_count--;

      }
   }

   /* adjust the sign of the result */

   target_hdr->i_is_negative ^= right_hdr->i_is_negative;

   /* the target hash code is now invalid */

   target_hdr->i_hash_code = -1;

   /* we eliminate high-order zero cells */

   target_cell = target_hdr->i_tail;
   while (target_cell->i_prev != NULL &&
          !(target_cell->i_cell_value)) {

      target_hdr->i_tail = target_cell->i_prev;
      free_integer_cell(target_cell);
      target_cell = target_hdr->i_tail;
      target_hdr->i_cell_count--;

   }

   target_cell->i_next = NULL;

   /*
    *  If the left and right had different signs the result will be
    *  negative.  Now we have to add or subtract the divisor to make the
    *  result positive.
    */

   if (target_hdr->i_is_negative &&
       (target_hdr->i_cell_count > 1 ||
          (target_hdr->i_head)->i_cell_value != 0)) {

      spare1.sp_form = ft_long;
      spare1.sp_val.sp_long_ptr = target_hdr;

      if (right_hdr->i_is_negative)
         integer_subtract(SETL_SYSTEM target,right,&spare1);
      else
         integer_add(SETL_SYSTEM target,right,&spare1);

      /* reverse the sign */

      if (target->sp_form == ft_short) {

         target->sp_val.sp_short_value =
            labs(target->sp_val.sp_short_value);

      }
      else {

         (target->sp_val.sp_long_ptr)->i_is_negative = NO;

      }

      unmark_specifier(&spare1);

      return;

   }

   /*
    *  Now we have a long value in the target.  We would like to use
    *  short values whenever possible, so we check whether it will fit in
    *  a short.  If so, we convert it.
    */

   if (target_hdr->i_cell_count < 3) {

      /* build up a long value */

      carry = (target_hdr->i_head)->i_cell_value;
      if (target_hdr->i_cell_count == 2) {

         carry += (((target_hdr->i_head)->i_next)->i_cell_value) <<
                    INT_CELL_WIDTH;

      }

      if (target_hdr->i_is_negative)
         carry = -carry;

      /* check whether it will fit in a short */

      if (!(hi_bits = carry & INT_HIGH_BITS) ||
          hi_bits == INT_HIGH_BITS) {

         free_interp_integer(SETL_SYSTEM target_hdr);
         unmark_specifier(target);
         target->sp_form = ft_short;
         target->sp_val.sp_short_value = carry;

         return;

      }
   }

   /*
    *  We're stuck with a long.  Set the target and return.
    */

   unmark_specifier(target);
   target->sp_form = ft_long;
   target->sp_val.sp_long_ptr = target_hdr;

   return;

}

/*\
 *  \function{integer\_lt()}
 *
 *  This function compares two integers for less than, where the integers
 *  may be either short or long.  It is meant to be called from the core
 *  interpreter for long integers, or for built-in functions for either
 *  type integer.  It returns YES if left is less than right.
\*/

int integer_lt(
   SETL_SYSTEM_PROTO
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
integer_h_ptr_type left_hdr, right_hdr;
                                       /* header pointers for left and      */
                                       /* right operands                    */
integer_c_ptr_type left_cell, right_cell;
                                       /* cell pointers for left and right  */
                                       /* operands                          */

   if (left->sp_form == ft_short) {

      if (right->sp_form == ft_short) {

         /*
          *  Since both operands are short, we just compare the values.
          */

         return left->sp_val.sp_short_value < right->sp_val.sp_short_value;

      }

      /*
       *  The left operand is short but the right is long.  The long
       *  must have a greater magnitude, we only need check the signs.
       */

      if ((right->sp_val.sp_long_ptr)->i_is_negative)
         return NO;

      return YES;

   }

   if (right->sp_form == ft_short) {

      /*
       *  The right operand is short but the left is long.  The long
       *  must have a greater magnitude, we only need check the signs.
       */

      if ((left->sp_val.sp_long_ptr)->i_is_negative)
         return YES;

      return NO;

   }

   /*
    *  Both operands are long.  We use the signs if they are different,
    *  otherwise we compare the cells.
    */

   left_hdr = left->sp_val.sp_long_ptr;
   right_hdr = right->sp_val.sp_long_ptr;

   if (left_hdr->i_is_negative) {

      if (!(right_hdr->i_is_negative))
         return YES;

      if (left_hdr->i_cell_count < right_hdr->i_cell_count)
         return NO;

      if (left_hdr->i_cell_count > right_hdr->i_cell_count)
         return YES;

      for (left_cell = left_hdr->i_tail, right_cell = right_hdr->i_tail;
           left_cell != NULL &&
              left_cell->i_cell_value == right_cell->i_cell_value;
           left_cell = left_cell->i_prev, right_cell = right_cell->i_prev);

      if (left_cell == NULL ||
          left_cell->i_cell_value < right_cell->i_cell_value)
         return NO;

      return YES;

   }

   if (right_hdr->i_is_negative)
      return NO;

   if (left_hdr->i_cell_count > right_hdr->i_cell_count)
      return NO;

   if (left_hdr->i_cell_count < right_hdr->i_cell_count)
      return YES;

   for (left_cell = left_hdr->i_tail, right_cell = right_hdr->i_tail;
        left_cell != NULL &&
           left_cell->i_cell_value == right_cell->i_cell_value;
        left_cell = left_cell->i_prev, right_cell = right_cell->i_prev);

   if (left_cell == NULL ||
       left_cell->i_cell_value > right_cell->i_cell_value)
      return NO;

   return YES;

}

/*\
 *  \function{integer\_le()}
 *
 *  This function compares two integers for less than, where the integers
 *  may be either short or long.  It is meant to be called from the core
 *  interpreter for long integers, or for built-in functions for either
 *  type integer.  It returns YES if left is less than or equal right.
\*/

int integer_le(
   SETL_SYSTEM_PROTO
   specifier *left,                    /* left operand                      */
   specifier *right)                   /* right operand                     */

{
integer_h_ptr_type left_hdr, right_hdr;
                                       /* header pointers for left and      */
                                       /* right operands                    */
integer_c_ptr_type left_cell, right_cell;
                                       /* cell pointers for left and right  */
                                       /* operands                          */

   if (left->sp_form == ft_short) {

      if (right->sp_form == ft_short) {

         /*
          *  Since both operands are short, we just compare the values.
          */

         return left->sp_val.sp_short_value <= right->sp_val.sp_short_value;

      }

      /*
       *  The left operand is short but the right is long.  The long
       *  must have a greater magnitude, we only need check the signs.
       */

      if ((right->sp_val.sp_long_ptr)->i_is_negative)
         return NO;

      return YES;

   }

   if (right->sp_form == ft_short) {

      /*
       *  The right operand is short but the left is long.  The long
       *  must have a greater magnitude, we only need check the signs.
       */

      if ((left->sp_val.sp_long_ptr)->i_is_negative)
         return YES;

      return NO;

   }

   /*
    *  Both operands are long.  We use the signs if they are different,
    *  otherwise we compare the cells.
    */

   left_hdr = left->sp_val.sp_long_ptr;
   right_hdr = right->sp_val.sp_long_ptr;

   if (left_hdr->i_is_negative) {

      if (!(right_hdr->i_is_negative))
         return YES;

      if (left_hdr->i_cell_count < right_hdr->i_cell_count)
         return NO;

      if (left_hdr->i_cell_count > right_hdr->i_cell_count)
         return YES;

      for (left_cell = left_hdr->i_tail, right_cell = right_hdr->i_tail;
           left_cell != NULL &&
              left_cell->i_cell_value == right_cell->i_cell_value;
           left_cell = left_cell->i_prev, right_cell = right_cell->i_prev);

      if (left_cell != NULL &&
          left_cell->i_cell_value < right_cell->i_cell_value)
         return NO;

      return YES;

   }

   if (right_hdr->i_is_negative)
      return NO;

   if (left_hdr->i_cell_count > right_hdr->i_cell_count)
      return NO;

   if (left_hdr->i_cell_count < right_hdr->i_cell_count)
      return YES;

   for (left_cell = left_hdr->i_tail, right_cell = right_hdr->i_tail;
        left_cell != NULL &&
           left_cell->i_cell_value == right_cell->i_cell_value;
        left_cell = left_cell->i_prev, right_cell = right_cell->i_prev);

   if (left_cell != NULL &&
       left_cell->i_cell_value > right_cell->i_cell_value)
      return NO;

   return YES;

}


